#include "smp.h"
#include "lapic.h"
#include "../process/process.h"
#include "../cpu/context.h"

extern int printf(const char *fmt, ...);
extern void *memset(void *s, int c, unsigned long n);
extern void *memcpy(void *d, const void *s, unsigned long n);
extern char *strcpy(char *d, const char *s);
extern void storeflags(unsigned int *flags);
extern void restoreflags(unsigned int flags);
extern void stopints(void);
extern void smp_install_trampoline(void);
extern void loadregisters(void);
extern void ap_load_kernel_gdt(void);
extern volatile u64 ap_pml4;
extern volatile u64 ap_stack_top;
extern volatile u64 ap_entry;
extern u64 boot_pml4;
extern DWORD *pagedir1;

cpu_local cpus[MAX_CPUS];
int cpu_count = 1;
spinlock_t sched_lock;

static u8 ap_stacks[MAX_CPUS][65536] __attribute__((aligned(16)));
static PCB386 ap_idle_pcb[MAX_CPUS];
static u8 ap_idle_stacks[MAX_CPUS][8192] __attribute__((aligned(16)));
static volatile int ap_started = 0;
volatile int smp_sched_enabled = 0;

int smp_cpu_id(void) {
    u32 id;
    int i;
    if (!lapic_mmio)
        return 0;
    id = lapic_get_id();
    for (i = 0; i < cpu_count; i++) {
        if (cpus[i].online && cpus[i].apic_id == id)
            return i;
    }
    return 0;
}

cpu_local *smp_this_cpu(void) {
    return &cpus[smp_cpu_id()];
}

void smp_enable_scheduling(void) {
    smp_sched_enabled = 1;
    /* Give APs time to leave the park loop and enter idle, then nudge. */
    {
        volatile u32 t;
        for (t = 0; t < 1000000; t++)
            __asm__ __volatile__("pause");
    }
    smp_reschedule_others();
}

void smp_cpu_idle(void) {
    /* Arm LAPIC timer only once we are on the idle stack with kernel GDT. */
    if (smp_cpu_id() != 0)
        smp_ap_enable_timer();
    for (;;) {
        __asm__ __volatile__("sti; hlt");
    }
}

/* IPI handler: nudge this CPU to pick up runnable work. */
void smp_reschedule_ipi(void) {
    lapic_eoi();
    if (smp_sched_enabled)
        schedule_from_timer();
}

static volatile int ap_work_done = 0;

static void ap_work_smoke(void) {
    printf("SMP: AP work-steal OK (cpu=%d)\n", smp_cpu_id());
    ap_work_done = 1;
    for (;;)
        __asm__ __volatile__("sti; hlt");
}

void smp_start_ap_work_smoke(void) {
    extern DWORD createkthread(void *ptr, char *name, DWORD stacksize);
    extern void ps_set_affinity(int pid, int cpu);
    DWORD wid;
    volatile u32 t;
    if (cpu_count < 2)
        return;
    ap_work_done = 0;
    wid = createkthread((void *)ap_work_smoke, "ap_work", 8192);
    if (!wid)
        return;
    ps_set_affinity((int)wid, 1);
    smp_reschedule_others();
    for (t = 0; t < 200000000u && !ap_work_done; t++)
        __asm__ __volatile__("pause");
    if (!ap_work_done)
        printf("SMP: AP work-steal TIMEOUT\n");
}

void smp_ap_enable_timer(void) {
    /* Local APIC already mapped; enable SVR + periodic timer on this CPU. */
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | 0xFF);
    lapic_write(LAPIC_TPR, 0);
    lapic_timer_init(100);
}

void smp_reschedule_others(void) {
    int i, me;
    if (!smp_sched_enabled || cpu_count < 2 || !lapic_mmio)
        return;
    me = smp_cpu_id();
    for (i = 0; i < cpu_count; i++) {
        if (i == me || !cpus[i].online)
            continue;
        lapic_send_ipi(cpus[i].apic_id, IPI_RESCHEDULE);
    }
}

static void ap_prepare_idle(int id) {
    PCB386 *idle = &ap_idle_pcb[id];
    memset(idle, 0, sizeof(*idle));
    idle->processid = 0xFFFF0000u | (DWORD)id;
    idle->accesslevel = ACCESS_SYS;
    idle->status = PS_ATTB_LOCKED | PS_ATTB_UNLOADABLE;
    idle->priority = 0;
    idle->pagedirloc = pagedir1;
    strcpy(idle->name, "cpu_idle");
    idle->regs.EIP = (DWORD)(uintptr)smp_cpu_idle;
    idle->regs.ESP = (DWORD)(uintptr)&ap_idle_stacks[id][8192];
    idle->regs.EFLAGS = 0x202;
    idle->regs.CS = 0x08; /* SYS_CODE_SEL */
    idle->regs.SS = 0x10; /* SYS_DATA_SEL */
    idle->regs.DS = 0x10;
    idle->ctx.rip = (u64)(uintptr)smp_cpu_idle;
    {
        uintptr top = (uintptr)&ap_idle_stacks[id][8192];
        top &= ~(uintptr)15;
        top -= 8;
        idle->regs.ESP = (DWORD)top;
        idle->ctx.rsp = (u64)top;
    }
    idle->ctx.rflags = 0x202;
    idle->ctx.cs = 0x08;
    idle->ctx.ss = 0x10;
    idle->ctx.cr3 = (u64)(uintptr)pagedir1;
    fpu_init_default(&idle->fpu);
    cpus[id].idle = idle;
    cpus[id].current = idle;
    idle->cpu_affinity = id;
    idle->on_cpu = id;
    ps_enqueue(idle);
}

void ap_main(void) {
    int id;

    /* Leave the trampoline GDT; IDT gates use kernel CS selectors. */
    ap_load_kernel_gdt();
    loadregisters(); /* same IDT as BSP */

    id = __sync_fetch_and_add(&ap_started, 1);
    cpus[id].cpu_id = id;
    cpus[id].apic_id = lapic_get_id();
    cpus[id].online = 1;
    cpus[id].kernel_stack = &ap_stacks[id][65536];

    /* Enable APIC software enable + spurious vector before parking. */
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | 0xFF);
    lapic_write(LAPIC_TPR, 0);

    ap_prepare_idle(id);

    printf("SMP: CPU %d (APIC %d) online (parked)\n", id, cpus[id].apic_id);

    /* Wait until BSP enables multi-core scheduling. */
    while (!smp_sched_enabled)
        __asm__ __volatile__("pause");

    /* Enter idle with IF clear; idle arms the timer then sti/hlt. */
    fpu_restore(&ap_idle_pcb[id].fpu);
    __asm__ __volatile__("cli");
    context_load(&ap_idle_pcb[id].ctx);
    /* not reached */
    smp_cpu_idle();
}

void smp_init(void) {
    spin_init(&sched_lock);
    memset(cpus, 0, sizeof(cpus));
    cpus[0].cpu_id = 0;
    cpus[0].apic_id = lapic_get_id();
    cpus[0].online = 1;
    /* BSP current is set by main/process_init; keep slot ready. */
    {
        extern PCB386 sPCB;
        cpus[0].current = &sPCB;
    }
    cpu_count = 1;
    ap_started = 1; /* BSP occupies slot 0 */
    printf("SMP: BSP APIC id=%d\n", cpus[0].apic_id);
}

void smp_start_aps(void) {
    int i;
    unsigned int flags;

    storeflags(&flags);
    stopints();

    smp_install_trampoline();
    ap_pml4 = (u64)(uintptr)&boot_pml4;
    ap_entry = (u64)(uintptr)ap_main;
    *(volatile u64 *)0x8F10 = ap_pml4;
    *(volatile u64 *)0x8F20 = ap_entry;

    for (i = 1; i < MAX_CPUS; i++) {
        int before = ap_started;
        ap_stack_top = (u64)(uintptr)&ap_stacks[i][65536];
        *(volatile u64 *)0x8F18 = ap_stack_top;
        cpus[i].kernel_stack = &ap_stacks[i][65536];

        lapic_send_init((u32)i);
        {
            volatile u32 t;
            for (t = 0; t < 1000000; t++)
                __asm__ __volatile__("pause");
        }
        lapic_send_sipi((u32)i, 0x08);
        {
            volatile u32 t;
            for (t = 0; t < 8000000; t++)
                __asm__ __volatile__("pause");
        }
        if (ap_started > before) {
            cpu_count++;
            printf("SMP: APIC %d started (cpu_count=%d)\n", i, cpu_count);
        } else {
            break;
        }
    }
    printf("SMP: %d CPUs online\n", cpu_count);
    restoreflags(flags);
}

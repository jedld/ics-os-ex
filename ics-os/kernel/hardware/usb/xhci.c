/* Minimal polling xHCI backend for the legacy USB MSC/BOT layer. */

#define XHCI_TRBS             256
#define XHCI_EVENT_TRBS       256
#define XHCI_MAX_SLOTS        8
#define XHCI_TIMEOUT          4000000

#define XHCI_CMD_RUN          (1u << 0)
#define XHCI_CMD_RESET        (1u << 1)
#define XHCI_STS_HALTED       (1u << 0)
#define XHCI_STS_CNR          (1u << 11)
#define XHCI_PORT_CCS         (1u << 0)
#define XHCI_PORT_PED         (1u << 1)
#define XHCI_PORT_RESET       (1u << 4)
#define XHCI_PORT_SPEED(v)    (((v) >> 10) & 0xF)
#define XHCI_PORT_CHANGE      ((1u << 17) | (1u << 18) | (1u << 20) | \
                               (1u << 21) | (1u << 22))

#define XHCI_TRB_CYCLE        (1u << 0)
#define XHCI_TRB_CHAIN        (1u << 4)
#define XHCI_TRB_IOC          (1u << 5)
#define XHCI_TRB_IDT          (1u << 6)
#define XHCI_TRB_DIR_IN       (1u << 16)
#define XHCI_TRB_TYPE(n)      ((DWORD)(n) << 10)
#define XHCI_TRB_NORMAL       XHCI_TRB_TYPE(1)
#define XHCI_TRB_SETUP        XHCI_TRB_TYPE(2)
#define XHCI_TRB_DATA         XHCI_TRB_TYPE(3)
#define XHCI_TRB_STATUS       XHCI_TRB_TYPE(4)
#define XHCI_TRB_LINK         XHCI_TRB_TYPE(6)
#define XHCI_TRB_ENABLE_SLOT  XHCI_TRB_TYPE(9)
#define XHCI_TRB_ADDRESS      XHCI_TRB_TYPE(11)
#define XHCI_TRB_CONFIG_EP    XHCI_TRB_TYPE(12)
#define XHCI_TRB_TRANSFER_EV  32
#define XHCI_TRB_COMMAND_EV   33
#define XHCI_CC_SUCCESS       1
#define XHCI_CC_SHORT_PACKET  13

typedef struct __attribute__((packed, aligned(16))) {
    volatile u64 parameter;
    volatile DWORD status;
    volatile DWORD control;
} xhci_trb;

typedef struct __attribute__((packed, aligned(16))) {
    u64 address;
    DWORD size;
    DWORD reserved;
} xhci_erst_entry;

typedef struct {
    xhci_trb *trbs;
    DWORD enqueue;
    DWORD cycle;
    DWORD link_chain;
} xhci_ring;

static u64 xhci_dcbaa[256] __attribute__((aligned(64)));
static xhci_trb xhci_cmd_trbs[XHCI_TRBS] __attribute__((aligned(64)));
static xhci_trb xhci_event_trbs[XHCI_EVENT_TRBS] __attribute__((aligned(64)));
static xhci_trb xhci_ep0_trbs[XHCI_TRBS] __attribute__((aligned(64)));
static xhci_trb xhci_ep_in_trbs[XHCI_TRBS] __attribute__((aligned(64)));
static xhci_trb xhci_ep_out_trbs[XHCI_TRBS] __attribute__((aligned(64)));
static xhci_erst_entry xhci_erst __attribute__((aligned(64)));
static BYTE xhci_input_ctx[33 * 64] __attribute__((aligned(64)));
static BYTE xhci_device_ctx[32 * 64] __attribute__((aligned(64)));

static volatile BYTE *xhci_mmio;
static volatile BYTE *xhci_op;
static volatile BYTE *xhci_runtime;
static volatile DWORD *xhci_doorbell;
static DWORD xhci_context_size;
static DWORD xhci_max_ports;
static DWORD xhci_port;
static DWORD xhci_speed;
static DWORD xhci_slot;
static DWORD xhci_ep0_mps;
static DWORD xhci_event_dequeue;
static DWORD xhci_event_cycle;
static xhci_ring xhci_cmd_ring;
static xhci_ring xhci_ep0_ring;
static xhci_ring xhci_ep_in_ring;
static xhci_ring xhci_ep_out_ring;
static void *xhci_scratchpad_array_alloc;
static void *xhci_scratchpad_pages_alloc;
static DWORD xhci_scratchpad_count;
static volatile int xhci_recovery_needed;
static volatile int xhci_fault_drop_next;
static volatile int xhci_fault_fail_init;

extern void *mmio_map(u64 phys, u64 len);
extern void *malloc(unsigned int size);
extern char kernel_cmdline[];

static void xhci_mb(void)
{
    asm volatile ("mfence" ::: "memory");
}

static DWORD xhci_r32(volatile BYTE *base, DWORD off)
{
    return *(volatile DWORD *)(base + off);
}

static u64 xhci_r64(volatile BYTE *base, DWORD off)
{
    return *(volatile u64 *)(base + off);
}

static void xhci_w32(volatile BYTE *base, DWORD off, DWORD value)
{
    *(volatile DWORD *)(base + off) = value;
    xhci_mb();
}

static void xhci_w64(volatile BYTE *base, DWORD off, u64 value)
{
    *(volatile u64 *)(base + off) = value;
    xhci_mb();
}

static int xhci_wait32(volatile BYTE *base, DWORD off, DWORD mask,
                       DWORD expected)
{
    DWORD spins;
    for (spins = 0; spins < XHCI_TIMEOUT; spins++) {
        if ((xhci_r32(base, off) & mask) == expected)
            return 1;
        usb_io_delay();
    }
    return 0;
}

static int xhci_find_controller(BYTE *bus, BYTE *slot, BYTE *func)
{
    BYTE b, s, f;
    for (b = 0; b < 8; b++) {
        for (s = 0; s < 32; s++) {
            WORD vendor = pci_read16(b, s, 0, 0);
            BYTE maxf = 1;
            if (vendor == 0xFFFF)
                continue;
            if (pci_read32(b, s, 0, 0x0C) & 0x800000)
                maxf = 8;
            for (f = 0; f < maxf; f++) {
                DWORD classreg = pci_read32(b, s, f, 0x08);
                if ((classreg >> 8) == 0x0C0330) {
                    *bus = b;
                    *slot = s;
                    *func = f;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int xhci_pci_bar(BYTE bus, BYTE slot, BYTE func,
                        u64 *base, u64 *size)
{
    WORD command = pci_read16(bus, slot, func, 0x04);
    DWORD lo = pci_read32(bus, slot, func, 0x10);
    DWORD hi = 0, mask_lo, mask_hi = 0;
    int is_64 = (lo & 6) == 4;
    if (lo & 1)
        return 0;
    if (is_64)
        hi = pci_read32(bus, slot, func, 0x14);
    pci_write16(bus, slot, func, 0x04, command & (WORD)~0x06);
    pci_write32(bus, slot, func, 0x10, 0xFFFFFFFFu);
    if (is_64)
        pci_write32(bus, slot, func, 0x14, 0xFFFFFFFFu);
    mask_lo = pci_read32(bus, slot, func, 0x10);
    if (is_64)
        mask_hi = pci_read32(bus, slot, func, 0x14);
    pci_write32(bus, slot, func, 0x10, lo);
    if (is_64)
        pci_write32(bus, slot, func, 0x14, hi);
    pci_write16(bus, slot, func, 0x04, command);
    *base = ((u64)hi << 32) | (lo & ~0xFu);
    if (is_64) {
        u64 mask = ((u64)mask_hi << 32) | (mask_lo & ~0xFu);
        *size = (~mask) + 1;
    } else {
        *size = (u64)((~(mask_lo & ~0xFu)) + 1);
    }
    if (strstr(kernel_cmdline, "xhci-high-bar-test")) {
        DWORD id = pci_read32(bus, slot, func, 0x00);
        if (!is_64 || id != 0x000D1B36u) {
            printf("xhci: high BAR test requires QEMU qemu-xhci\n");
            return 0;
        }
        pci_write16(bus, slot, func, 0x04, command & (WORD)~0x06);
        pci_write32(bus, slot, func, 0x10, lo & 0xFu);
        pci_write32(bus, slot, func, 0x14, 1);
        pci_write16(bus, slot, func, 0x04, command);
        *base = ((u64)pci_read32(bus, slot, func, 0x14) << 32) |
                (pci_read32(bus, slot, func, 0x10) & ~0xFu);
        if (*base != 0x100000000ULL) {
            printf("xhci: high BAR test relocation failed base=0x%llx\n",
                   (unsigned long long)*base);
            return 0;
        }
        printf("xhci: test BAR relocated above 4 GiB\n");
    }
    return *base != 0 && *size != 0;
}

static void xhci_ring_init(xhci_ring *ring, xhci_trb *trbs, DWORD link_chain)
{
    memset(trbs, 0, sizeof(xhci_trb) * XHCI_TRBS);
    ring->trbs = trbs;
    ring->enqueue = 0;
    ring->cycle = 1;
    ring->link_chain = link_chain;
    trbs[XHCI_TRBS - 1].parameter = (u64)usb_phys(trbs);
    trbs[XHCI_TRBS - 1].control = XHCI_TRB_LINK | (1u << 1) |
                                    link_chain | XHCI_TRB_CYCLE;
}

static xhci_trb *xhci_ring_put(xhci_ring *ring, u64 parameter, DWORD status,
                               DWORD control)
{
    xhci_trb *trb = &ring->trbs[ring->enqueue];
    trb->parameter = parameter;
    trb->status = status;
    xhci_mb();
    trb->control = control | ring->cycle;
    xhci_mb();
    ring->enqueue++;
    if (ring->enqueue == XHCI_TRBS - 1) {
        ring->trbs[XHCI_TRBS - 1].control = XHCI_TRB_LINK | (1u << 1) |
                                            ring->link_chain | ring->cycle;
        ring->enqueue = 0;
        ring->cycle ^= 1;
    }
    return trb;
}

static xhci_trb *xhci_ring_buffer(xhci_ring *ring, BYTE *data, DWORD len,
                                  DWORD control, DWORD final_control)
{
    u64 phys = (u64)usb_phys(data);
    xhci_trb *last = 0;
    while (len) {
        DWORD chunk = 0x10000u - (DWORD)(phys & 0xFFFF);
        DWORD trb_control = control;
        if (chunk > len)
            chunk = len;
        if (chunk < len)
            trb_control |= XHCI_TRB_CHAIN;
        else
            trb_control |= final_control;
        last = xhci_ring_put(ring, phys, chunk, trb_control);
        phys += chunk;
        len -= chunk;
    }
    return last;
}

static int xhci_next_event(DWORD wanted_type, u64 wanted_ptr,
                           DWORD *completion, DWORD *slot)
{
    DWORD spins;
    volatile BYTE *intr = xhci_runtime + 0x20;
    for (spins = 0; spins < XHCI_TIMEOUT; spins++) {
        xhci_trb *event = &xhci_event_trbs[xhci_event_dequeue];
        DWORD control = event->control;
        if ((control & 1) == xhci_event_cycle) {
            DWORD type = (control >> 10) & 0x3F;
            u64 pointer = event->parameter & ~(u64)0xF;
            DWORD cc = event->status >> 24;
            DWORD event_slot = control >> 24;
            xhci_event_dequeue++;
            if (xhci_event_dequeue == XHCI_EVENT_TRBS) {
                xhci_event_dequeue = 0;
                xhci_event_cycle ^= 1;
            }
            xhci_w64(intr, 0x18,
                     (u64)usb_phys(&xhci_event_trbs[xhci_event_dequeue]) | 8);
            if (type == wanted_type && (!wanted_ptr || pointer == wanted_ptr)) {
                if (completion)
                    *completion = cc;
                if (slot)
                    *slot = event_slot;
                return 1;
            }
        } else {
            usb_io_delay();
        }
    }
    return 0;
}

static int xhci_command(u64 parameter, DWORD status, DWORD control,
                        DWORD *slot)
{
    DWORD cc = 0;
    xhci_trb *trb = xhci_ring_put(&xhci_cmd_ring, parameter, status, control);
    asm volatile ("wbinvd");
    xhci_doorbell[0] = 0;
    xhci_mb();
    if (!xhci_next_event(XHCI_TRB_COMMAND_EV, (u64)usb_phys(trb), &cc, slot)) {
        printf("xhci: command timeout type=%u\n", (control >> 10) & 0x3F);
        xhci_recovery_needed = 1;
        return 0;
    }
    if (cc != XHCI_CC_SUCCESS) {
        printf("xhci: command failed type=%u cc=%u\n",
               (control >> 10) & 0x3F, cc);
         xhci_recovery_needed = 1;
        return 0;
    }
    return 1;
}

static DWORD *xhci_input_context(DWORD index)
{
    return (DWORD *)(xhci_input_ctx + index * xhci_context_size);
}

static DWORD *xhci_device_context(DWORD index)
{
    return (DWORD *)(xhci_device_ctx + index * xhci_context_size);
}

static void xhci_fill_slot_context(DWORD *slot, DWORD entries)
{
    memset(slot, 0, xhci_context_size);
    slot[0] = (xhci_speed << 20) | (entries << 27);
    slot[1] = xhci_port << 16;
}

static void xhci_fill_ep_context(DWORD *ep, DWORD type, DWORD max_packet,
                                 DWORD max_burst, xhci_ring *ring,
                                 DWORD average)
{
    u64 dequeue = (u64)usb_phys(ring->trbs) | 1;
    memset(ep, 0, xhci_context_size);
        ep[1] = (3u << 1) | (type << 3) | (max_burst << 8) |
            (max_packet << 16);
    ep[2] = (DWORD)dequeue;
    ep[3] = (DWORD)(dequeue >> 32);
    ep[4] = average;
}

static int xhci_setup_scratchpads(DWORD hcs2)
{
    DWORD high = (hcs2 >> 21) & 0x1F;
    DWORD low = (hcs2 >> 27) & 0x1F;
    DWORD count = (high << 5) | low;
    u64 *array;
    BYTE *pages;
    DWORD i;
    if (!count)
        return 1;
    if (count > 256) {
        printf("xhci: unsupported scratchpad count=%u\n", count);
        return 0;
    }
    if (!xhci_scratchpad_array_alloc || !xhci_scratchpad_pages_alloc) {
        xhci_scratchpad_array_alloc = malloc(count * sizeof(u64) + 63);
        xhci_scratchpad_pages_alloc = malloc(count * 4096 + 4095);
        if (!xhci_scratchpad_array_alloc || !xhci_scratchpad_pages_alloc) {
            printf("xhci: scratchpad allocation failed count=%u\n", count);
            return 0;
        }
        xhci_scratchpad_count = count;
    } else if (count != xhci_scratchpad_count) {
        printf("xhci: scratchpad count changed old=%u new=%u\n",
               xhci_scratchpad_count, count);
        return 0;
    }
    array = (u64 *)(((unsigned long)xhci_scratchpad_array_alloc + 63) & ~63UL);
    pages = (BYTE *)(((unsigned long)xhci_scratchpad_pages_alloc + 4095) &
                     ~4095UL);
    if (((u64)(unsigned long)array >> 32) ||
        ((u64)(unsigned long)(pages + count * 4096 - 1) >> 32)) {
        printf("xhci: scratchpads are outside 32-bit DMA range\n");
        return 0;
    }
    memset(array, 0, count * sizeof(u64));
    memset(pages, 0, count * 4096);
    for (i = 0; i < count; i++)
        array[i] = (u64)usb_phys(pages + i * 4096);
    xhci_dcbaa[0] = (u64)usb_phys(array);
    printf("xhci: scratchpads=%u\n", count);
    return 1;
}

static int xhci_address_device(int block_set_address)
{
    DWORD *control;
    DWORD *slot;
    DWORD *ep0;
    memset(xhci_input_ctx, 0, sizeof(xhci_input_ctx));
    control = xhci_input_context(0);
    slot = xhci_input_context(1);
    ep0 = xhci_input_context(2);
    control[1] = 3;
    xhci_fill_slot_context(slot, 1);
    xhci_fill_ep_context(ep0, 4, xhci_ep0_mps, 0, &xhci_ep0_ring, 8);
    return xhci_command((u64)usb_phys(xhci_input_ctx), 0,
                        XHCI_TRB_ADDRESS | (block_set_address ? (1u << 9) : 0) |
                        (xhci_slot << 24), 0);
}

static int xhci_set_ep0_packet_size(BYTE encoded)
{
    DWORD size = xhci_speed >= 4 ? (1u << encoded) : encoded;
    if (size != 8 && size != 16 && size != 32 && size != 64 && size != 512) {
        printf("xhci: invalid EP0 packet size=%u encoded=%u\n", size, encoded);
        return 0;
    }
    xhci_ep0_mps = size;
    return 1;
}

static int xhci_transfer(xhci_ring *ring, DWORD endpoint, BYTE *data,
                         DWORD len, DWORD control)
{
    DWORD cc = 0;
    int drop = xhci_fault_drop_next;
    xhci_trb *last = xhci_ring_buffer(ring, data, len, control,
                                      XHCI_TRB_IOC);
    if (!last)
        return 0;
    asm volatile ("wbinvd");
    if (drop) {
        xhci_fault_drop_next = 0;
        printf("xhci: test dropping bulk doorbell ep=%u\n", endpoint);
    } else {
        xhci_doorbell[xhci_slot] = endpoint;
        xhci_mb();
    }
    if (!xhci_next_event(XHCI_TRB_TRANSFER_EV, (u64)usb_phys(last), &cc, 0)) {
        printf("xhci: transfer timeout ep=%u len=%u\n", endpoint, len);
        xhci_recovery_needed = 1;
        return 0;
    }
    asm volatile ("wbinvd");
    if (cc != XHCI_CC_SUCCESS && cc != XHCI_CC_SHORT_PACKET) {
        printf("xhci: transfer failed ep=%u cc=%u\n", endpoint, cc);
        xhci_recovery_needed = 1;
        return 0;
    }
    return 1;
}

static int xhci_control(usb_setup *setup, void *data, int len)
{
    xhci_trb *last;
    u64 setup_value = 0;
    DWORD trt = len ? ((setup->bmRequestType & 0x80) ? 3 : 2) : 0;
    memcpy(&setup_value, setup, 8);
    xhci_ring_put(&xhci_ep0_ring, setup_value, 8,
                  XHCI_TRB_SETUP | XHCI_TRB_IDT | XHCI_TRB_CHAIN |
                  (trt << 16));
    if (len) {
        xhci_ring_buffer(&xhci_ep0_ring, data, (DWORD)len,
                         XHCI_TRB_DATA |
                         ((setup->bmRequestType & 0x80) ? XHCI_TRB_DIR_IN : 0),
                         XHCI_TRB_CHAIN);
    }
    last = xhci_ring_put(&xhci_ep0_ring, 0, 0,
                         XHCI_TRB_STATUS | XHCI_TRB_IOC |
                         ((!len || !(setup->bmRequestType & 0x80))
                          ? XHCI_TRB_DIR_IN : 0));
    asm volatile ("wbinvd");
    xhci_doorbell[xhci_slot] = 1;
    xhci_mb();
    {
        DWORD cc = 0;
        if (!xhci_next_event(XHCI_TRB_TRANSFER_EV, (u64)usb_phys(last), &cc, 0)) {
            printf("xhci: control timeout request=%u\n", setup->bRequest);
            xhci_recovery_needed = 1;
            return 0;
        }
        asm volatile ("wbinvd");
        if (cc != XHCI_CC_SUCCESS && cc != XHCI_CC_SHORT_PACKET) {
            printf("xhci: control failed request=%u cc=%u\n",
                   setup->bRequest, cc);
                 xhci_recovery_needed = 1;
            return 0;
        }
    }
    return 1;
}

static DWORD xhci_endpoint_id(BYTE endpoint, int in)
{
    return (DWORD)endpoint * 2 + (in ? 1 : 0);
}

static int xhci_bulk(BYTE endpoint, int in, BYTE *data, int len)
{
    DWORD endpoint_id = xhci_endpoint_id(endpoint, in);
    xhci_ring *ring = in ? &xhci_ep_in_ring : &xhci_ep_out_ring;
    return xhci_transfer(ring, endpoint_id, data, (DWORD)len, XHCI_TRB_NORMAL);
}

static int xhci_configure_endpoints(BYTE ep_in, WORD mps_in,
                                    BYTE burst_in, BYTE ep_out, WORD mps_out,
                                    BYTE burst_out)
{
    DWORD in_id = xhci_endpoint_id(ep_in, 1);
    DWORD out_id = xhci_endpoint_id(ep_out, 0);
    DWORD entries = in_id > out_id ? in_id : out_id;
    DWORD *control;
    memset(xhci_input_ctx, 0, sizeof(xhci_input_ctx));
    xhci_ring_init(&xhci_ep_in_ring, xhci_ep_in_trbs, XHCI_TRB_CHAIN);
    xhci_ring_init(&xhci_ep_out_ring, xhci_ep_out_trbs, XHCI_TRB_CHAIN);
    control = xhci_input_context(0);
    control[1] = (1u << 0) | (1u << in_id) | (1u << out_id);
    xhci_fill_slot_context(xhci_input_context(1), entries);
    xhci_fill_ep_context(xhci_input_context(in_id + 1), 6, mps_in, burst_in,
                         &xhci_ep_in_ring, USB_BULK_MAX);
    xhci_fill_ep_context(xhci_input_context(out_id + 1), 2, mps_out, burst_out,
                         &xhci_ep_out_ring, USB_BULK_MAX);
    if (!xhci_command((u64)usb_phys(xhci_input_ctx), 0,
                      XHCI_TRB_CONFIG_EP | (xhci_slot << 24), 0))
        return 0;
    printf("xhci: configured bulk endpoints in=%u out=%u\n", in_id, out_id);
    return 1;
}

static int xhci_legacy_handoff(DWORD hcc, u64 bar_size)
{
    DWORD off = ((hcc >> 16) & 0xFFFF) * 4;
    DWORD guard = 64;
    while (off && (u64)off + 8 <= bar_size && guard--) {
        DWORD cap = xhci_r32(xhci_mmio, off);
        DWORD id = cap & 0xFF;
        DWORD next = (cap >> 8) & 0xFF;
        if (id == 1) {
            DWORD value = cap | (1u << 24);
            xhci_w32(xhci_mmio, off, value);
            if (!xhci_wait32(xhci_mmio, off, 1u << 16, 0)) {
                printf("xhci: firmware ownership handoff timed out\n");
                return 0;
            }
            xhci_w32(xhci_mmio, off + 4, 0);
            return 1;
        }
        if (!next)
            break;
        off += next * 4;
    }
    return 1;
}

static int xhci_reset_port(void)
{
    DWORD p;
    DWORD off = 0x400 + (xhci_port - 1) * 0x10;
    p = xhci_r32(xhci_op, off);
    if (!(p & XHCI_PORT_CCS))
        return 0;
    xhci_w32(xhci_op, off, (p & ~XHCI_PORT_CHANGE) | XHCI_PORT_RESET);
    if (!xhci_wait32(xhci_op, off, XHCI_PORT_RESET, 0))
        return 0;
    p = xhci_r32(xhci_op, off);
    xhci_speed = XHCI_PORT_SPEED(p);
    if (xhci_speed >= 4)
        xhci_ep0_mps = 512;
    else if (xhci_speed == 3)
        xhci_ep0_mps = 64;
    else
        xhci_ep0_mps = 8;
    return (p & (XHCI_PORT_CCS | XHCI_PORT_PED)) ==
           (XHCI_PORT_CCS | XHCI_PORT_PED);
}

static int xhci_init_controller(void)
{
    BYTE bus, slot, func;
    u64 bar = 0, bar_size = 0, required;
    DWORD caplen, hcs1, hcs2, hcc, dboff, rtsoff, found = 0;
    DWORD port;
    volatile BYTE *intr;
    if (!xhci_find_controller(&bus, &slot, &func)) {
        printf("xhci: no controller found\n");
        return 0;
    }
    if (xhci_fault_fail_init) {
        xhci_fault_fail_init = 0;
        printf("xhci: test forcing recovery initialization failure\n");
        return 0;
    }
    if (!xhci_pci_bar(bus, slot, func, &bar, &bar_size) ||
        bar + bar_size < bar) {
        printf("xhci: unsupported BAR 0x%llx\n", (unsigned long long)bar);
        return 0;
    }
    pci_write16(bus, slot, func, 0x04,
                pci_read16(bus, slot, func, 0x04) | 0x06);
    xhci_mmio = (volatile BYTE *)mmio_map(bar, bar_size);
    if (!xhci_mmio) {
        printf("xhci: cannot map BAR 0x%llx size=%llu\n",
               (unsigned long long)bar, (unsigned long long)bar_size);
        return 0;
    }
    caplen = *(volatile BYTE *)xhci_mmio;
    hcs1 = xhci_r32(xhci_mmio, 0x04);
    hcs2 = xhci_r32(xhci_mmio, 0x08);
    hcc = xhci_r32(xhci_mmio, 0x10);
    dboff = xhci_r32(xhci_mmio, 0x14) & ~3u;
    rtsoff = xhci_r32(xhci_mmio, 0x18) & ~0x1Fu;
    xhci_op = xhci_mmio + caplen;
    xhci_runtime = xhci_mmio + rtsoff;
    xhci_doorbell = (volatile DWORD *)(xhci_mmio + dboff);
    xhci_context_size = (hcc & (1u << 2)) ? 64 : 32;
    xhci_max_ports = (hcs1 >> 24) & 0xFF;
    required = caplen + 0x400 + xhci_max_ports * 0x10;
    if ((u64)required > bar_size || (u64)dboff + 0x24 > bar_size ||
        (u64)rtsoff + 0x40 > bar_size) {
        printf("xhci: register offsets exceed BAR size=%llu\n",
               (unsigned long long)bar_size);
        return 0;
    }
    if (!xhci_legacy_handoff(hcc, bar_size))
        return 0;

    xhci_w32(xhci_op, 0x00, xhci_r32(xhci_op, 0x00) & ~XHCI_CMD_RUN);
    if (!xhci_wait32(xhci_op, 0x04, XHCI_STS_HALTED, XHCI_STS_HALTED))
        return 0;
    xhci_w32(xhci_op, 0x00, XHCI_CMD_RESET);
    if (!xhci_wait32(xhci_op, 0x00, XHCI_CMD_RESET, 0) ||
        !xhci_wait32(xhci_op, 0x04, XHCI_STS_CNR, 0)) {
        printf("xhci: reset timed out\n");
        return 0;
    }
    if (!(xhci_r32(xhci_op, 0x08) & 1)) {
        printf("xhci: 4K pages unsupported\n");
        return 0;
    }

    memset(xhci_dcbaa, 0, sizeof(xhci_dcbaa));
    if (!xhci_setup_scratchpads(hcs2))
        return 0;
    memset(xhci_event_trbs, 0, sizeof(xhci_event_trbs));
    memset(xhci_device_ctx, 0, sizeof(xhci_device_ctx));
    xhci_ring_init(&xhci_cmd_ring, xhci_cmd_trbs, 0);
    xhci_ring_init(&xhci_ep0_ring, xhci_ep0_trbs, XHCI_TRB_CHAIN);
    xhci_event_dequeue = 0;
    xhci_event_cycle = 1;
    xhci_erst.address = (u64)usb_phys(xhci_event_trbs);
    xhci_erst.size = XHCI_EVENT_TRBS;
    xhci_erst.reserved = 0;
    intr = xhci_runtime + 0x20;
    xhci_w32(intr, 0x08, 1);
    xhci_w64(intr, 0x10, (u64)usb_phys(&xhci_erst));
    xhci_w64(intr, 0x18, (u64)usb_phys(xhci_event_trbs));
    xhci_w64(xhci_op, 0x30, (u64)usb_phys(xhci_dcbaa));
    xhci_w64(xhci_op, 0x18, (u64)usb_phys(xhci_cmd_trbs) | 1);
    xhci_w32(xhci_op, 0x38,
             ((hcs1 & 0xFF) < XHCI_MAX_SLOTS) ? (hcs1 & 0xFF) : XHCI_MAX_SLOTS);
    xhci_w32(xhci_op, 0x00, XHCI_CMD_RUN);
    if (!xhci_wait32(xhci_op, 0x04, XHCI_STS_HALTED, 0)) {
        printf("xhci: controller failed to run\n");
        return 0;
    }

    for (port = 1; port <= xhci_max_ports; port++) {
        DWORD p = xhci_r32(xhci_op, 0x400 + (port - 1) * 0x10);
        if (p & XHCI_PORT_CCS) {
            xhci_port = port;
            if (xhci_reset_port()) {
                found = 1;
                break;
            }
        }
    }
    if (!found) {
        printf("xhci: no enabled device port\n");
        return 0;
    }
    xhci_slot = 0;
    if (!xhci_command(0, 0, XHCI_TRB_ENABLE_SLOT, &xhci_slot) || !xhci_slot)
        return 0;
    xhci_dcbaa[xhci_slot] = (u64)usb_phys(xhci_device_ctx);
    if (!xhci_address_device(1))
        return 0;
    printf("xhci: controller at PCI %u:%u.%u port=%u speed=%u slot=%u ctx=%u\n",
           bus, slot, func, xhci_port, xhci_speed, xhci_slot,
           xhci_context_size);
    return 1;
}

static void xhci_stop_controller(void)
{
    if (xhci_op)
        xhci_w32(xhci_op, 0x00, xhci_r32(xhci_op, 0x00) & ~XHCI_CMD_RUN);
}
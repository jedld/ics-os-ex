#include "vtd.h"

extern int printf(const char *format, ...);
extern char kernel_cmdline[];
extern char *strstr(const char *haystack, const char *needle);
extern void serial_puts(const char *text);
extern void *mmio_map(unsigned long long physical, unsigned long long length);
extern unsigned int *mempop(void);
extern void mempush(unsigned int physical);

#define VTD_REG_CAP 0x08
#define VTD_REG_ECAP 0x10
#define VTD_REG_GCMD 0x18
#define VTD_REG_GSTS 0x1C
#define VTD_REG_RTADDR 0x20
#define VTD_REG_CCMD 0x28
#define VTD_REG_FSTS 0x34
#define VTD_REG_PMEN 0x64
#define VTD_GCMD_TE (1U << 31)
#define VTD_GCMD_SRTP (1U << 30)
#define VTD_GSTS_TES (1U << 31)
#define VTD_GSTS_RTPS (1U << 30)
#define VTD_GSTS_QIES (1U << 26)
#define VTD_GSTS_IRES (1U << 25)
#define VTD_CCMD_ICC (1ULL << 63)
#define VTD_CCMD_GLOBAL (1ULL << 61)
#define VTD_IOTLB_IVT (1ULL << 63)
#define VTD_IOTLB_GLOBAL (1ULL << 60)
#define VTD_FAULT_PENDING 0x13U
#define VTD_WAIT_SPINS 4000000U

typedef struct {
    vtd_legacy_entry *root;
    vtd_legacy_entry *contexts[VTD_ROOT_ENTRY_COUNT];
    volatile unsigned char *registers;
    int active;
} vtd_runtime_unit;

static vtd_runtime_unit vtd_runtime_units[VTD_MAX_DRHDS];

static unsigned int vtd_mmio_read32(volatile unsigned char *registers,
                                    unsigned int offset)
{
    return *(volatile unsigned int *)(registers + offset);
}

static unsigned long long vtd_mmio_read64(volatile unsigned char *registers,
                                          unsigned int offset)
{
    return *(volatile unsigned long long *)(registers + offset);
}

static void vtd_mmio_write32(volatile unsigned char *registers,
                             unsigned int offset, unsigned int value)
{
    *(volatile unsigned int *)(registers + offset) = value;
    asm volatile ("mfence" ::: "memory");
}

static void vtd_mmio_write64(volatile unsigned char *registers,
                             unsigned int offset, unsigned long long value)
{
    *(volatile unsigned long long *)(registers + offset) = value;
    asm volatile ("mfence" ::: "memory");
}

static int vtd_wait32(volatile unsigned char *registers, unsigned int offset,
                      unsigned int mask, unsigned int expected)
{
    unsigned int spins;
    for (spins = 0; spins < VTD_WAIT_SPINS; spins++) {
        if ((vtd_mmio_read32(registers, offset) & mask) == expected)
            return 1;
        asm volatile ("pause");
    }
    return 0;
}

static int vtd_wait64_clear(volatile unsigned char *registers,
                            unsigned int offset, unsigned long long mask)
{
    unsigned int spins;
    for (spins = 0; spins < VTD_WAIT_SPINS; spins++) {
        if (!(vtd_mmio_read64(registers, offset) & mask))
            return 1;
        asm volatile ("pause");
    }
    return 0;
}

static void vtd_release_tables(vtd_runtime_unit *unit)
{
    unsigned int bus;
    if (!unit)
        return;
    for (bus = 0; bus < VTD_ROOT_ENTRY_COUNT; bus++) {
        if (unit->contexts[bus]) {
            mempush((unsigned int)(unsigned long)unit->contexts[bus]);
            unit->contexts[bus] = 0;
        }
    }
    if (unit->root) {
        mempush((unsigned int)(unsigned long)unit->root);
        unit->root = 0;
    }
    unit->active = 0;
}

static void vtd_zero_page(void *page)
{
    unsigned int index;
    unsigned long long *words = (unsigned long long *)page;
    for (index = 0; index < VTD_PAGE_SIZE / sizeof(*words); index++)
        words[index] = 0;
}

static int vtd_allocate_context_bus(vtd_runtime_unit *unit,
                                    unsigned int bus)
{
    if (unit->contexts[bus])
        return 1;
    unit->contexts[bus] = (vtd_legacy_entry *)mempop();
    if (!unit->contexts[bus])
        return 0;
    vtd_zero_page(unit->contexts[bus]);
    return vtd_legacy_root_entry(&unit->root[bus],
                                 (unsigned long long)(unsigned long)
                                 unit->contexts[bus]);
}

static int vtd_allocate_tables(const vtd_dmar_info *info,
                               unsigned int drhd_index,
                               vtd_runtime_unit *unit,
                               unsigned long long capability,
                               unsigned long long extended_capability)
{
    unsigned int bus;
    const vtd_drhd_info *drhd;
    if (!info || drhd_index >= info->drhd_count || !unit || unit->root)
        return 0;
    drhd = &info->drhds[drhd_index];
    unit->root = (vtd_legacy_entry *)mempop();
    if (!unit->root)
        return 0;
    vtd_zero_page(unit->root);
    if (drhd->flags & 1) {
        for (bus = 0; bus < VTD_ROOT_ENTRY_COUNT; bus++) {
            if (!vtd_allocate_context_bus(unit, bus) ||
                !vtd_legacy_context_table(unit->contexts[bus], capability,
                                          extended_capability,
                                          (unsigned short)(bus + 1)))
                goto fail;
        }
    } else {
        unsigned int scope;
        for (scope = drhd->first_scope;
             scope < drhd->first_scope + drhd->scope_count; scope++) {
            const vtd_scope_info *device = &info->scopes[scope];
            unsigned int devfn;
            if (device->drhd != drhd_index || device->path_count != 1 ||
                (device->type != 1 && device->type != 3))
                goto fail;
            bus = device->start_bus;
            devfn = ((unsigned int)device->last_device << 3) |
                    device->last_function;
            if (!vtd_allocate_context_bus(unit, bus) ||
                !vtd_legacy_context_entry(&unit->contexts[bus][devfn],
                                          capability, extended_capability,
                                          (unsigned short)(bus + 1)))
                goto fail;
        }
    }
    asm volatile ("mfence" ::: "memory");
    return 1;

fail:
    vtd_release_tables(unit);
    return 0;
}

static void vtd_deactivate(vtd_runtime_unit *unit)
{
    if (!unit)
        return;
    if (unit->active && unit->registers) {
        vtd_mmio_write32(unit->registers, VTD_REG_GCMD, 0);
        vtd_wait32(unit->registers, VTD_REG_GSTS, VTD_GSTS_TES, 0);
    }
    unit->registers = 0;
    vtd_release_tables(unit);
}

static int vtd_activate_drhd(const vtd_dmar_info *info,
                             unsigned int drhd_index,
                             vtd_runtime_unit *unit)
{
    const vtd_drhd_info *drhd;
    volatile unsigned char *registers;
    unsigned long long capability;
    unsigned long long extended_capability;
    unsigned int iotlb_offset;
    unsigned int status;

    if (!info || drhd_index >= info->drhd_count || !unit)
        return 0;
    drhd = &info->drhds[drhd_index];
    if (drhd->segment != 0 || (!(drhd->flags & 1) && !drhd->scope_count))
        return 0;
    registers = (volatile unsigned char *)mmio_map(drhd->register_base,
                                                   VTD_PAGE_SIZE);
    if (!registers)
        return 0;
    capability = vtd_mmio_read64(registers, VTD_REG_CAP);
    extended_capability = vtd_mmio_read64(registers, VTD_REG_ECAP);
    status = vtd_mmio_read32(registers, VTD_REG_GSTS);
    if ((status & (VTD_GSTS_TES | VTD_GSTS_QIES | VTD_GSTS_IRES)) ||
        (vtd_mmio_read32(registers, VTD_REG_PMEN) & (1U << 31)) ||
        !vtd_iotlb_register_offset(extended_capability, &iotlb_offset) ||
        !vtd_allocate_tables(info, drhd_index, unit, capability,
                     extended_capability))
        return 0;

    vtd_mmio_write64(registers, VTD_REG_RTADDR,
                     (unsigned long long)(unsigned long)unit->root);
    vtd_mmio_write32(registers, VTD_REG_GCMD, VTD_GCMD_SRTP);
    if (!vtd_wait32(registers, VTD_REG_GSTS, VTD_GSTS_RTPS,
                    VTD_GSTS_RTPS))
        goto fail;
    vtd_mmio_write64(registers, VTD_REG_CCMD,
                     VTD_CCMD_ICC | VTD_CCMD_GLOBAL);
    if (!vtd_wait64_clear(registers, VTD_REG_CCMD, VTD_CCMD_ICC))
        goto fail;
    vtd_mmio_write64(registers, iotlb_offset + 8,
                     VTD_IOTLB_IVT | VTD_IOTLB_GLOBAL);
    if (!vtd_wait64_clear(registers, iotlb_offset + 8, VTD_IOTLB_IVT))
        goto fail;
    vtd_mmio_write32(registers, VTD_REG_GCMD, VTD_GCMD_TE);
    if (!vtd_wait32(registers, VTD_REG_GSTS, VTD_GSTS_TES,
                    VTD_GSTS_TES) ||
        (vtd_mmio_read32(registers, VTD_REG_FSTS) & VTD_FAULT_PENDING))
        goto disable;
    unit->registers = registers;
    unit->active = 1;
    return 1;

disable:
    vtd_mmio_write32(registers, VTD_REG_GCMD, 0);
    vtd_wait32(registers, VTD_REG_GSTS, VTD_GSTS_TES, 0);
fail:
    vtd_release_tables(unit);
    return 0;
}

static int vtd_activate(const vtd_dmar_info *info)
{
    unsigned int drhd;
    if (!info || !info->drhd_count)
        return 0;
    for (drhd = 0; drhd < info->drhd_count; drhd++) {
        if (!vtd_activate_drhd(info, drhd,
                               &vtd_runtime_units[drhd])) {
            while (drhd > 0) {
                drhd--;
                vtd_deactivate(&vtd_runtime_units[drhd]);
            }
            return 0;
        }
    }
    return 1;
}

typedef struct __attribute__((packed)) {
    char signature[8];
    unsigned char checksum;
    char oem_id[6];
    unsigned char revision;
    unsigned int rsdt_address;
    unsigned int length;
    unsigned long long xsdt_address;
    unsigned char extended_checksum;
    unsigned char reserved[3];
} vtd_rsdp;

static const vtd_acpi_header *vtd_map_sdt(unsigned long long physical)
{
    const vtd_acpi_header *header;
    if (!physical || physical > 0xFFFFFFFFULL)
        return 0;
    header = (const vtd_acpi_header *)(unsigned long)physical;
    if (!header || header->length < sizeof(vtd_acpi_header) ||
        header->length > 1024 * 1024)
        return 0;
    if ((unsigned long long)physical + header->length > 0x100000000ULL ||
        !vtd_checksum_valid(header, header->length))
        return 0;
    return header;
}

int vtd_discover(const void *rsdp_pointer, unsigned int rsdp_length)
{
    const vtd_rsdp *rsdp = (const vtd_rsdp *)rsdp_pointer;
    const vtd_acpi_header *root;
    unsigned long long root_address;
    unsigned int entry_size;
    unsigned int entry_count;
    unsigned int index;

    if (!rsdp || rsdp_length < 20 ||
        !vtd_bytes_equal(rsdp->signature, "RSD PTR ", 8) ||
        !vtd_checksum_valid(rsdp, 20))
        return 0;
    if (rsdp->revision >= 2) {
        if (rsdp_length < sizeof(vtd_rsdp) ||
            rsdp->length < sizeof(vtd_rsdp) ||
            rsdp->length > rsdp_length ||
            !vtd_checksum_valid(rsdp, rsdp->length))
            return 0;
        root_address = rsdp->xsdt_address;
        entry_size = 8;
    } else {
        root_address = rsdp->rsdt_address;
        entry_size = 4;
    }
    root = vtd_map_sdt(root_address);
    if (!root || root->length < sizeof(vtd_acpi_header) ||
        !vtd_bytes_equal(root->signature,
                         entry_size == 8 ? "XSDT" : "RSDT", 4) ||
        (root->length - sizeof(vtd_acpi_header)) % entry_size)
        return 0;
    entry_count = (root->length - sizeof(vtd_acpi_header)) / entry_size;
    for (index = 0; index < entry_count; index++) {
        const unsigned char *entry = (const unsigned char *)root +
                                     sizeof(vtd_acpi_header) + index * entry_size;
        unsigned long long address = entry_size == 8 ? vtd_read64(entry) :
                                                       vtd_read32(entry);
        const vtd_acpi_header *table = vtd_map_sdt(address);
        vtd_dmar_info info;
        unsigned int drhd;
        if (!table || !vtd_bytes_equal(table->signature, "DMAR", 4))
            continue;
        if (!vtd_parse_dmar(table, table->length, &info)) {
            printf("vtd: invalid DMAR table\n");
            return 0;
        }
        printf("vtd: DMAR haw=%u flags=0x%x drhds=%u scopes=%u\n",
               info.host_address_width, info.flags, info.drhd_count,
               info.scope_count);
        for (drhd = 0; drhd < info.drhd_count; drhd++) {
            unsigned int scope;
            printf("vtd: DRHD segment=%u base=0x%llx flags=0x%x scopes=%u\n",
                   info.drhds[drhd].segment,
                   info.drhds[drhd].register_base,
                   info.drhds[drhd].flags,
                   info.drhds[drhd].scope_count);
            for (scope = info.drhds[drhd].first_scope;
                 scope < info.drhds[drhd].first_scope +
                         info.drhds[drhd].scope_count; scope++) {
                printf("vtd: scope type=%u bus=%u path=%u device=%u function=%u\n",
                       info.scopes[scope].type, info.scopes[scope].start_bus,
                       info.scopes[scope].path_count,
                       info.scopes[scope].last_device,
                       info.scopes[scope].last_function);
            }
        }
        if (strstr(kernel_cmdline, "vtd-discovery-test"))
            serial_puts("VTD_DMAR_OK\n");
        if (strstr(kernel_cmdline, "vtd-activation-test")) {
            if (!vtd_activate(&info)) {
                printf("vtd: legacy pass-through activation failed\n");
                serial_puts("VTD_TRANSLATION_PT_FAIL\n");
                return 0;
            }
            printf("vtd: legacy pass-through translation enabled drhds=%u\n",
                   info.drhd_count);
            serial_puts("VTD_TRANSLATION_PT_OK\n");
        }
        return 1;
    }
    printf("vtd: DMAR table not found; IOMMU isolation unavailable\n");
    return 0;
}
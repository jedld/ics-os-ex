#ifndef ICSOS_VTD_H
#define ICSOS_VTD_H

#define VTD_MAX_DRHDS 8
#define VTD_MAX_SCOPES 32

#define VTD_PAGE_SIZE 4096
#define VTD_ROOT_ENTRY_COUNT 256
#define VTD_CONTEXT_ENTRY_COUNT 256
#define VTD_ECAP_PASS_THROUGH (1ULL << 6)
#define VTD_CAP_SAGAW_SHIFT 8
#define VTD_CAP_SAGAW_MASK (0x1FULL << VTD_CAP_SAGAW_SHIFT)
#define VTD_ENTRY_PRESENT 1ULL
#define VTD_CONTEXT_PASS_THROUGH (2ULL << 2)

typedef struct {
    unsigned long long low;
    unsigned long long high;
} vtd_legacy_entry;

static int vtd_legacy_select_agaw(unsigned long long capability,
                                  unsigned int *agaw)
{
    unsigned int value;
    if (!agaw || !(capability & VTD_CAP_SAGAW_MASK))
        return 0;
    for (value = 0; value < 5; value++) {
        if (capability & (1ULL << (VTD_CAP_SAGAW_SHIFT + value))) {
            *agaw = value;
            return 1;
        }
    }
    return 0;
}

static int vtd_iotlb_register_offset(unsigned long long extended_capability,
                                     unsigned int *offset)
{
    unsigned int value;
    if (!offset)
        return 0;
    value = (unsigned int)((extended_capability >> 8) & 0x3FFULL) << 4;
    if (!value || value > VTD_PAGE_SIZE - 16)
        return 0;
    *offset = value;
    return 1;
}

static int vtd_legacy_root_entry(vtd_legacy_entry *entry,
                                 unsigned long long context_address)
{
    if (!entry || !context_address ||
        (context_address & (VTD_PAGE_SIZE - 1)))
        return 0;
    entry->low = context_address | VTD_ENTRY_PRESENT;
    entry->high = 0;
    return 1;
}

static int vtd_legacy_context_entry(vtd_legacy_entry *entry,
                                    unsigned long long capability,
                                    unsigned long long extended_capability,
                                    unsigned short domain_id)
{
    unsigned int agaw;
    if (!entry || !(extended_capability & VTD_ECAP_PASS_THROUGH) ||
        !vtd_legacy_select_agaw(capability, &agaw))
        return 0;
    entry->low = VTD_ENTRY_PRESENT | VTD_CONTEXT_PASS_THROUGH;
    entry->high = agaw | ((unsigned long long)domain_id << 8);
    return 1;
}

static int vtd_legacy_context_table(vtd_legacy_entry *entries,
                                    unsigned long long capability,
                                    unsigned long long extended_capability,
                                    unsigned short domain_id)
{
    unsigned int index;
    if (!entries)
        return 0;
    for (index = 0; index < VTD_CONTEXT_ENTRY_COUNT; index++) {
        if (!vtd_legacy_context_entry(&entries[index], capability,
                                      extended_capability, domain_id))
            return 0;
    }
    return 1;
}

typedef struct __attribute__((packed)) {
    char signature[4];
    unsigned int length;
    unsigned char revision;
    unsigned char checksum;
    char oem_id[6];
    char oem_table_id[8];
    unsigned int oem_revision;
    unsigned int creator_id;
    unsigned int creator_revision;
} vtd_acpi_header;

typedef struct {
    unsigned short segment;
    unsigned char flags;
    unsigned long long register_base;
    unsigned int first_scope;
    unsigned int scope_count;
} vtd_drhd_info;

typedef struct {
    unsigned int drhd;
    unsigned char type;
    unsigned char enumeration_id;
    unsigned char start_bus;
    unsigned char path_count;
    unsigned char last_device;
    unsigned char last_function;
} vtd_scope_info;

typedef struct {
    unsigned char host_address_width;
    unsigned char flags;
    unsigned int drhd_count;
    unsigned int scope_count;
    vtd_drhd_info drhds[VTD_MAX_DRHDS];
    vtd_scope_info scopes[VTD_MAX_SCOPES];
} vtd_dmar_info;

static unsigned short vtd_read16(const unsigned char *data)
{
    return (unsigned short)data[0] | ((unsigned short)data[1] << 8);
}

static unsigned int vtd_read32(const unsigned char *data)
{
    return (unsigned int)data[0] | ((unsigned int)data[1] << 8) |
           ((unsigned int)data[2] << 16) | ((unsigned int)data[3] << 24);
}

static unsigned long long vtd_read64(const unsigned char *data)
{
    return (unsigned long long)vtd_read32(data) |
           ((unsigned long long)vtd_read32(data + 4) << 32);
}

static int vtd_bytes_equal(const char *left, const char *right,
                           unsigned int length)
{
    unsigned int index;
    for (index = 0; index < length; index++) {
        if (left[index] != right[index])
            return 0;
    }
    return 1;
}

static int vtd_checksum_valid(const void *table, unsigned int length)
{
    const unsigned char *bytes = (const unsigned char *)table;
    unsigned char sum = 0;
    unsigned int index;
    if (!table || !length)
        return 0;
    for (index = 0; index < length; index++)
        sum = (unsigned char)(sum + bytes[index]);
    return sum == 0;
}

static int vtd_parse_dmar(const void *table, unsigned int available,
                          vtd_dmar_info *info)
{
    const unsigned char *bytes = (const unsigned char *)table;
    unsigned int length;
    unsigned int offset;
    vtd_dmar_info parsed = { 0 };

    if (!table || !info || available < 48 ||
        !vtd_bytes_equal((const char *)bytes, "DMAR", 4))
        return 0;
    length = vtd_read32(bytes + 4);
    if (length < 48 || length > available || !vtd_checksum_valid(table, length))
        return 0;
    parsed.host_address_width = bytes[36];
    parsed.flags = bytes[37];
    offset = 48;
    while (offset < length) {
        unsigned short type;
        unsigned short structure_length;
        if (length - offset < 4)
            return 0;
        type = vtd_read16(bytes + offset);
        structure_length = vtd_read16(bytes + offset + 2);
        if (structure_length < 4 || structure_length > length - offset)
            return 0;
        if (type == 0) {
            unsigned int scope_offset;
            unsigned int drhd_index = parsed.drhd_count;
            vtd_drhd_info *drhd;
            if (structure_length < 16 || drhd_index >= VTD_MAX_DRHDS)
                return 0;
            drhd = &parsed.drhds[drhd_index];
            drhd->flags = bytes[offset + 4];
            drhd->segment = vtd_read16(bytes + offset + 6);
            drhd->register_base = vtd_read64(bytes + offset + 8);
            if (!drhd->register_base || (drhd->register_base & 0xFFF))
                return 0;
            drhd->first_scope = parsed.scope_count;
            scope_offset = offset + 16;
            while (scope_offset < offset + structure_length) {
                unsigned int scope_length;
                unsigned int path_count;
                vtd_scope_info *scope;
                if (offset + structure_length - scope_offset < 8 ||
                    parsed.scope_count >= VTD_MAX_SCOPES)
                    return 0;
                scope_length = bytes[scope_offset + 1];
                if (scope_length < 8 || (scope_length - 6) & 1 ||
                    scope_length > offset + structure_length - scope_offset)
                    return 0;
                path_count = (scope_length - 6) / 2;
                scope = &parsed.scopes[parsed.scope_count++];
                scope->drhd = drhd_index;
                scope->type = bytes[scope_offset];
                scope->enumeration_id = bytes[scope_offset + 4];
                scope->start_bus = bytes[scope_offset + 5];
                scope->path_count = (unsigned char)path_count;
                scope->last_device = bytes[scope_offset + 6 +
                                           (path_count - 1) * 2];
                scope->last_function = bytes[scope_offset + 7 +
                                             (path_count - 1) * 2];
                if (scope->last_device > 31 || scope->last_function > 7)
                    return 0;
                scope_offset += scope_length;
            }
            drhd->scope_count = parsed.scope_count - drhd->first_scope;
            parsed.drhd_count++;
        }
        offset += structure_length;
    }
    if (offset != length || !parsed.drhd_count)
        return 0;
    *info = parsed;
    return 1;
}

int vtd_discover(const void *rsdp, unsigned int rsdp_length);

#endif
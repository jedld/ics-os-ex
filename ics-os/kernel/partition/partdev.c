/*
  Name: partdev.c
  Description: Generic partition block device (see partdev.h).
*/
#include "partdev.h"
#include "devmgr/dex32_devmgr.h"

static partdev_entry partdev_table[PARTDEV_MAX];
static int partdev_num = 0;

static partdev_disk partdev_disks[PARTDEV_MAX_DISKS];
static int partdev_disk_num = 0;

static const partdev_entry *partdev_lookup(int deviceid)
{
    int i;
    for (i = 0; i < partdev_num; i++)
        if (partdev_table[i].mydeviceid == deviceid)
            return &partdev_table[i];
    return 0;
}

static int partdev_count_for_parent(int parent_deviceid)
{
    int i, n = 0;
    for (i = 0; i < partdev_num; i++)
        if (partdev_table[i].parent_deviceid == parent_deviceid)
            n++;
    return n;
}

static u64 partdev_entry_size(const partdev_entry *e)
{
    return (e->endlba > e->startlba) ? (e->endlba - e->startlba) : 0;
}

int partdev_partition_read(int deviceid, u64 block, char *blockbuff, DWORD numblocks)
{
    const partdev_entry *e = partdev_lookup(deviceid);
    u64 lba;
    if (!e) return 0;
    if (block >= partdev_entry_size(e)) return 0;
    if ((u64)numblocks > partdev_entry_size(e) - block) return 0;
    lba = block + e->startlba;
    return e->read_raw(e->parent_deviceid, lba, blockbuff, numblocks);
}

int partdev_partition_write(int deviceid, u64 block, char *blockbuff, DWORD numblocks)
{
    const partdev_entry *e = partdev_lookup(deviceid);
    u64 lba;
    if (!e || !e->write_raw) return 0;
    if (block >= partdev_entry_size(e)) return 0;
    if ((u64)numblocks > partdev_entry_size(e) - block) return 0;
    lba = block + e->startlba;
    return e->write_raw(e->parent_deviceid, lba, blockbuff, numblocks);
}

static int partdev_read_block(u64 block, char *blockbuff, DWORD numblocks)
{
    return partdev_partition_read(devmgr_getcontext(), block, blockbuff, numblocks);
}

static int partdev_write_block(u64 block, char *blockbuff, DWORD numblocks)
{
    return partdev_partition_write(devmgr_getcontext(), block, blockbuff, numblocks);
}

static u64 partdev_total_blocks(void)
{
    const partdev_entry *e = partdev_lookup(devmgr_getcontext());
    if (!e) return 0;
    return partdev_entry_size(e);
}

static int partdev_get_block_size(void)
{
    const partdev_entry *e = partdev_lookup(devmgr_getcontext());
    if (!e) return 0;
    return e->block_size;
}

int partdev_register(int parent_deviceid, const char *name, const char *description,
                     int block_size, u64 startlba, u64 endlba,
                     partdev_raw_fn read_raw, partdev_raw_fn write_raw,
                     int read_only, const char *type_name, int entry_index,
                     u64 attrs, const char *pname)
{
    devmgr_block_desc part;
    partdev_entry *e;
    int devid;

    if (partdev_num >= PARTDEV_MAX) return -1;
    if (partdev_count_for_parent(parent_deviceid) >= PARTDEV_MAX_PER_DISK) return -1;
    if (!read_raw) return -1;

    e = &partdev_table[partdev_num];
    e->mydeviceid = -1;
    e->parent_deviceid = parent_deviceid;
    e->startlba = startlba;
    e->endlba = endlba;
    e->block_size = block_size;
    e->read_only = read_only;
    e->read_raw = read_raw;
    e->write_raw = write_raw;
    e->entry_index = entry_index;
    e->attrs = attrs;
    if (type_name) {
        const char *s = type_name;
        int n = 0;
        while (s[n] && n < (int)sizeof(e->type_name) - 1) {
            e->type_name[n] = s[n];
            n++;
        }
        e->type_name[n] = 0;
    } else
        e->type_name[0] = 0;
    if (pname) {
        const char *s = pname;
        int n = 0;
        while (s[n] && n < (int)sizeof(e->pname) - 1) {
            e->pname[n] = s[n];
            n++;
        }
        e->pname[n] = 0;
    } else
        e->pname[0] = 0;

    memset(&part, 0, sizeof(part));
    part.hdr.size = sizeof(part);
    part.hdr.type = DEVMGR_BLOCK;
    sprintf(part.hdr.name, "%s", name);
    if (description)
        sprintf(part.hdr.description, "%s", description);
    part.read_block = partdev_read_block;
    part.write_block = partdev_write_block;
    part.total_blocks = partdev_total_blocks;
    part.get_block_size = partdev_get_block_size;

    devid = devmgr_register((devmgr_generic *)&part);
    if (devid < 0) return -1;
    e->mydeviceid = devid;
    partdev_num++;
    return devid;
}

void partdev_remove(int parent_deviceid)
{
    int i = 0, j = 0;
    while (i < partdev_num) {
        if (partdev_table[i].parent_deviceid == parent_deviceid)
            i++;
        else {
            partdev_table[j] = partdev_table[i];
            i++;
            j++;
        }
    }
    partdev_num = j;
}

void partdev_set_disk(int parent_deviceid, const char *disk_name, int table_type,
                      const unsigned char disk_guid[16], int used_backup, int gpt_entries)
{
    int i;
    for (i = 0; i < partdev_disk_num; i++) {
        if (partdev_disks[i].parent_deviceid == parent_deviceid) {
            if (disk_name)
                sprintf(partdev_disks[i].disk_name, "%s", disk_name);
            partdev_disks[i].table_type = table_type;
            if (disk_guid)
                memcpy(partdev_disks[i].disk_guid, disk_guid, 16);
            partdev_disks[i].used_backup = used_backup;
            partdev_disks[i].gpt_entries = gpt_entries;
            return;
        }
    }
    if (partdev_disk_num >= PARTDEV_MAX_DISKS) return;
    memset(&partdev_disks[partdev_disk_num], 0, sizeof(partdev_disk));
    partdev_disks[partdev_disk_num].parent_deviceid = parent_deviceid;
    if (disk_name)
        sprintf(partdev_disks[partdev_disk_num].disk_name, "%s", disk_name);
    partdev_disks[partdev_disk_num].table_type = table_type;
    if (disk_guid)
        memcpy(partdev_disks[partdev_disk_num].disk_guid, disk_guid, 16);
    partdev_disks[partdev_disk_num].used_backup = used_backup;
    partdev_disks[partdev_disk_num].gpt_entries = gpt_entries;
    partdev_disk_num++;
}

int partdev_count(void)
{
    return partdev_num;
}

const partdev_entry *partdev_get(int index)
{
    if (index < 0 || index >= partdev_num) return 0;
    return &partdev_table[index];
}

int partdev_is_child(int deviceid, int parent_deviceid)
{
    int i;
    for (i = 0; i < partdev_num; i++)
        if (partdev_table[i].mydeviceid == deviceid &&
            partdev_table[i].parent_deviceid == parent_deviceid)
            return 1;
    return 0;
}

int partdev_count_children(int parent_deviceid)
{
    return partdev_count_for_parent(parent_deviceid);
}

int partdev_first_child(int parent_deviceid)
{
    int i;
    for (i = 0; i < partdev_num; i++)
        if (partdev_table[i].parent_deviceid == parent_deviceid)
            return partdev_table[i].mydeviceid;
    return -1;
}

int partdev_disk_count(void)
{
    return partdev_disk_num;
}

const partdev_disk *partdev_disk_get(int index)
{
    if (index < 0 || index >= partdev_disk_num) return 0;
    return &partdev_disks[index];
}

void partdev_format_guid(const unsigned char *guid, char *buf, int buflen)
{
    static const char hex[] = "0123456789abcdef";
    /* Canonical mixed-endian order: Data1 LE32, Data2 LE16, Data3 LE16,
       then Data4 (4+12) big-endian. */
    static const int n[16] = {3,2,1,0, 5,4, 7,6, 8,9,10,11,12,13,14,15};
    int i, p = 0;
    if (buflen < 37) { if (buflen > 0) buf[0] = 0; return; }
    for (i = 0; i < 16; i++) {
        if (p + 2 >= buflen) break;
        buf[p++] = hex[guid[n[i]] >> 4];
        buf[p++] = hex[guid[n[i]] & 0xf];
        if (i == 3 || i == 5 || i == 7) { if (p + 1 < buflen) buf[p++] = '-'; }
    }
    buf[p] = 0;
}

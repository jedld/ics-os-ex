#ifndef ICSOS_DMA_H
#define ICSOS_DMA_H

typedef struct {
    void *cpu_addr;
    void *allocation_base;
    unsigned long long dma_addr;
    unsigned int length;
    unsigned int alignment;
} dma_region;

typedef void *(*dma_allocator)(unsigned int size);
typedef void (*dma_releaser)(void *allocation);

typedef struct {
    unsigned long long dma_mask;
    unsigned int bounce_alignment;
    int force_bounce;
    dma_allocator allocate;
    dma_releaser release;
} dma_device;

#define DMA_TO_DEVICE      1
#define DMA_FROM_DEVICE    2
#define DMA_BIDIRECTIONAL  3

typedef struct {
    void *cpu_addr;
    void *device_cpu_addr;
    void *allocation_base;
    unsigned long long dma_addr;
    unsigned int length;
    unsigned int direction;
    dma_releaser release;
    int active;
} dma_mapping;

#define DMA_MAX_SEGMENTS 32

typedef struct {
    void *cpu_addr;
    unsigned int length;
} dma_segment;

typedef struct {
    dma_mapping mappings[DMA_MAX_SEGMENTS];
    unsigned int count;
    unsigned int direction;
    int active;
} dma_sg_mapping;

static int dma_identity_map(const void *cpu_addr, unsigned int length,
                            unsigned int alignment,
                            unsigned long long dma_mask,
                            unsigned long long *dma_addr)
{
    unsigned long long start = (unsigned long long)(unsigned long)cpu_addr;
    unsigned long long end;

    if (!cpu_addr || !length || !dma_addr || !alignment ||
        (alignment & (alignment - 1)) != 0 ||
        (start & (alignment - 1)) != 0)
        return 0;
    end = start + length - 1;
    if (end < start || end > dma_mask)
        return 0;
    *dma_addr = start;
    return 1;
}

static int dma_region_init(dma_region *region, void *cpu_addr,
                           unsigned int length, unsigned int alignment,
                           unsigned long long dma_mask)
{
    unsigned long long dma_addr;

    if (!region || !dma_identity_map(cpu_addr, length, alignment, dma_mask,
                                     &dma_addr))
        return 0;
    region->cpu_addr = cpu_addr;
    region->allocation_base = 0;
    region->dma_addr = dma_addr;
    region->length = length;
    region->alignment = alignment;
    return 1;
}

static int dma_alloc_coherent(dma_region *region, unsigned int length,
                              unsigned int alignment,
                              unsigned long long dma_mask,
                              dma_allocator allocate,
                              dma_releaser release)
{
    unsigned int allocation_length;
    unsigned long raw;
    unsigned long aligned;
    unsigned int i;
    void *allocation;

    if (!region || !length || !alignment ||
        (alignment & (alignment - 1)) != 0 || !allocate || !release ||
        length > ~0u - (alignment - 1))
        return 0;
    allocation_length = length + alignment - 1;
    allocation = allocate(allocation_length);
    if (!allocation)
        return 0;
    raw = (unsigned long)allocation;
    aligned = (raw + alignment - 1) & ~(unsigned long)(alignment - 1);
    if (!dma_region_init(region, (void *)aligned, length, alignment,
                         dma_mask)) {
        release(allocation);
        return 0;
    }
    region->allocation_base = allocation;
    for (i = 0; i < length; i++)
        ((unsigned char *)region->cpu_addr)[i] = 0;
    return 1;
}

static void dma_free_coherent(dma_region *region, dma_releaser release)
{
    void *allocation;

    if (!region)
        return;
    allocation = region->allocation_base;
    region->cpu_addr = 0;
    region->allocation_base = 0;
    region->dma_addr = 0;
    region->length = 0;
    region->alignment = 0;
    if (allocation && release)
        release(allocation);
}

static int dma_region_map(const dma_region *region, const void *cpu_addr,
                          unsigned int length,
                          unsigned long long *dma_addr)
{
    unsigned long long base;
    unsigned long long start;
    unsigned long long offset;

    if (!region || !region->cpu_addr || !cpu_addr || !length || !dma_addr)
        return 0;
    base = (unsigned long long)(unsigned long)region->cpu_addr;
    start = (unsigned long long)(unsigned long)cpu_addr;
    if (start < base)
        return 0;
    offset = start - base;
    if (offset >= region->length || length > region->length - offset)
        return 0;
    *dma_addr = region->dma_addr + offset;
    return 1;
}

static void dma_copy_bytes(void *destination, const void *source,
                           unsigned int length)
{
    unsigned int index;
    for (index = 0; index < length; index++)
        ((unsigned char *)destination)[index] =
            ((const unsigned char *)source)[index];
}

static int dma_map_single_device(dma_mapping *mapping, void *cpu_addr,
                                 unsigned int length,
                                 unsigned int direction,
                                 const dma_device *device)
{
    unsigned long long dma_addr;
    unsigned int allocation_length;
    unsigned long raw;
    unsigned long aligned;
    void *allocation = 0;
    void *device_cpu_addr = cpu_addr;

    if (!mapping || mapping->active || !cpu_addr || !length ||
        (direction != DMA_TO_DEVICE && direction != DMA_FROM_DEVICE &&
         direction != DMA_BIDIRECTIONAL) ||
        !device || !device->dma_mask)
        return 0;
    if (device->force_bounce) {
        if (!device->allocate || !device->release ||
            !device->bounce_alignment ||
            (device->bounce_alignment & (device->bounce_alignment - 1)) != 0 ||
            length > ~0u - (device->bounce_alignment - 1))
            return 0;
        allocation_length = length + device->bounce_alignment - 1;
        allocation = device->allocate(allocation_length);
        if (!allocation)
            return 0;
        raw = (unsigned long)allocation;
        aligned = (raw + device->bounce_alignment - 1) &
                  ~(unsigned long)(device->bounce_alignment - 1);
        device_cpu_addr = (void *)aligned;
    }
    if (!dma_identity_map(device_cpu_addr, length,
                          device->force_bounce ? device->bounce_alignment : 1,
                          device->dma_mask, &dma_addr)) {
        if (allocation)
            device->release(allocation);
        return 0;
    }
    if (allocation &&
        (direction == DMA_TO_DEVICE || direction == DMA_BIDIRECTIONAL))
        dma_copy_bytes(device_cpu_addr, cpu_addr, length);
    __sync_synchronize();
    mapping->cpu_addr = cpu_addr;
    mapping->device_cpu_addr = device_cpu_addr;
    mapping->allocation_base = allocation;
    mapping->dma_addr = dma_addr;
    mapping->length = length;
    mapping->direction = direction;
    mapping->release = allocation ? device->release : 0;
    mapping->active = 1;
    return 1;
}

static int dma_map_single(dma_mapping *mapping, void *cpu_addr,
                          unsigned int length, unsigned int direction,
                          unsigned long long dma_mask)
{
    dma_device device = { dma_mask, 1, 0, 0, 0 };
    return dma_map_single_device(mapping, cpu_addr, length, direction,
                                 &device);
}

static int dma_unmap_single(dma_mapping *mapping)
{
    void *allocation;
    dma_releaser release;

    if (!mapping || !mapping->active)
        return 0;
    __sync_synchronize();
    allocation = mapping->allocation_base;
    release = mapping->release;
    if (allocation &&
        (mapping->direction == DMA_FROM_DEVICE ||
         mapping->direction == DMA_BIDIRECTIONAL))
        dma_copy_bytes(mapping->cpu_addr, mapping->device_cpu_addr,
                       mapping->length);
    mapping->cpu_addr = 0;
    mapping->device_cpu_addr = 0;
    mapping->allocation_base = 0;
    mapping->dma_addr = 0;
    mapping->length = 0;
    mapping->direction = 0;
    mapping->release = 0;
    mapping->active = 0;
    if (allocation && release)
        release(allocation);
    return 1;
}

static int dma_map_sg(dma_sg_mapping *mapping,
                      const dma_segment *segments, unsigned int count,
                      unsigned int direction, unsigned long long dma_mask)
{
    unsigned int index;

    if (!mapping || mapping->active || !segments || !count ||
        count > DMA_MAX_SEGMENTS)
        return 0;
    mapping->count = 0;
    mapping->direction = 0;
    for (index = 0; index < count; index++) {
        if (!dma_map_single(&mapping->mappings[index],
                            segments[index].cpu_addr,
                            segments[index].length, direction, dma_mask)) {
            while (index)
                dma_unmap_single(&mapping->mappings[--index]);
            mapping->count = 0;
            return 0;
        }
        mapping->count++;
    }
    mapping->direction = direction;
    mapping->active = 1;
    return 1;
}

static int dma_map_sg_device(dma_sg_mapping *mapping,
                             const dma_segment *segments,
                             unsigned int count, unsigned int direction,
                             const dma_device *device)
{
    unsigned int index;

    if (!mapping || mapping->active || !segments || !count ||
        count > DMA_MAX_SEGMENTS)
        return 0;
    mapping->count = 0;
    mapping->direction = 0;
    for (index = 0; index < count; index++) {
        if (!dma_map_single_device(&mapping->mappings[index],
                                   segments[index].cpu_addr,
                                   segments[index].length, direction,
                                   device)) {
            while (index)
                dma_unmap_single(&mapping->mappings[--index]);
            mapping->count = 0;
            return 0;
        }
        mapping->count++;
    }
    mapping->direction = direction;
    mapping->active = 1;
    return 1;
}

static int dma_unmap_sg(dma_sg_mapping *mapping)
{
    unsigned int index;
    int result = 1;

    if (!mapping || !mapping->active || !mapping->count ||
        mapping->count > DMA_MAX_SEGMENTS)
        return 0;
    for (index = mapping->count; index; index--)
        result &= dma_unmap_single(&mapping->mappings[index - 1]);
    mapping->count = 0;
    mapping->direction = 0;
    mapping->active = 0;
    return result;
}

#endif
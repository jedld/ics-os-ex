#ifndef ICSOS_IOMMU_H
#define ICSOS_IOMMU_H

#define IOMMU_PAGE_SIZE       4096ULL
#define IOMMU_MAX_MAPPINGS    64

#define IOMMU_DOMAIN_IDENTITY   1
#define IOMMU_DOMAIN_TRANSLATED 2
#define IOMMU_DOMAIN_BLOCKED    3

#define IOMMU_READ  1
#define IOMMU_WRITE 2

typedef struct {
    unsigned long long iova;
    unsigned long long physical;
    unsigned long long length;
    unsigned int permissions;
    int active;
} iommu_mapping;

typedef struct {
    unsigned int type;
    unsigned int requester_id;
    int attached;
    int faulted;
    unsigned int fault_count;
    unsigned long long iova_start;
    unsigned long long iova_end;
    iommu_mapping mappings[IOMMU_MAX_MAPPINGS];
} iommu_domain;

static int iommu_range_valid(unsigned long long start,
                             unsigned long long length,
                             unsigned long long end_limit)
{
    unsigned long long end;
    if (!length)
        return 0;
    end = start + length - 1;
    return end >= start && end <= end_limit;
}

static int iommu_domain_init(iommu_domain *domain, unsigned int type,
                             unsigned long long iova_start,
                             unsigned long long iova_end)
{
    unsigned int index;
    if (!domain ||
        (type != IOMMU_DOMAIN_IDENTITY &&
         type != IOMMU_DOMAIN_TRANSLATED &&
         type != IOMMU_DOMAIN_BLOCKED))
        return 0;
    if (type == IOMMU_DOMAIN_TRANSLATED &&
        (iova_start > iova_end ||
         (iova_start & (IOMMU_PAGE_SIZE - 1)) ||
         ((iova_end + 1) & (IOMMU_PAGE_SIZE - 1))))
        return 0;
    domain->type = type;
    domain->requester_id = 0;
    domain->attached = 0;
    domain->faulted = 0;
    domain->fault_count = 0;
    domain->iova_start = iova_start;
    domain->iova_end = iova_end;
    for (index = 0; index < IOMMU_MAX_MAPPINGS; index++)
        domain->mappings[index].active = 0;
    return 1;
}

static int iommu_domain_attach(iommu_domain *domain,
                               unsigned int requester_id)
{
    if (!domain || domain->attached || requester_id > 0xFFFF)
        return 0;
    domain->requester_id = requester_id;
    domain->attached = 1;
    return 1;
}

static int iommu_ranges_overlap(unsigned long long first_start,
                                unsigned long long first_length,
                                unsigned long long second_start,
                                unsigned long long second_length)
{
    unsigned long long first_end = first_start + first_length - 1;
    unsigned long long second_end = second_start + second_length - 1;
    return first_start <= second_end && second_start <= first_end;
}

static int iommu_domain_map(iommu_domain *domain, unsigned int requester_id,
                            unsigned long long physical,
                            unsigned long long length,
                            unsigned int permissions,
                            unsigned long long *iova)
{
    unsigned int index;
    unsigned int slot = IOMMU_MAX_MAPPINGS;
    unsigned long long candidate;
    int collision;

    if (!domain || !domain->attached || domain->faulted ||
        domain->requester_id != requester_id || !iova ||
        (permissions != IOMMU_READ && permissions != IOMMU_WRITE &&
         permissions != (IOMMU_READ | IOMMU_WRITE)) ||
        (physical & (IOMMU_PAGE_SIZE - 1)) ||
        (length & (IOMMU_PAGE_SIZE - 1)) || !length ||
        domain->type == IOMMU_DOMAIN_BLOCKED)
        return 0;
    for (index = 0; index < IOMMU_MAX_MAPPINGS; index++) {
        if (!domain->mappings[index].active && slot == IOMMU_MAX_MAPPINGS)
            slot = index;
    }
    if (slot == IOMMU_MAX_MAPPINGS)
        return 0;
    if (domain->type == IOMMU_DOMAIN_IDENTITY) {
        candidate = physical;
        if (!iommu_range_valid(candidate, length, ~0ULL))
            return 0;
        for (index = 0; index < IOMMU_MAX_MAPPINGS; index++) {
            iommu_mapping *mapping = &domain->mappings[index];
            if (mapping->active &&
                iommu_ranges_overlap(candidate, length, mapping->iova,
                                     mapping->length))
                return 0;
        }
    } else {
        candidate = domain->iova_start;
        do {
            collision = 0;
            if (candidate < domain->iova_start ||
                !iommu_range_valid(candidate, length, domain->iova_end))
                return 0;
            for (index = 0; index < IOMMU_MAX_MAPPINGS; index++) {
                iommu_mapping *mapping = &domain->mappings[index];
                if (mapping->active &&
                    iommu_ranges_overlap(candidate, length, mapping->iova,
                                         mapping->length)) {
                    candidate = mapping->iova + mapping->length;
                    collision = 1;
                    break;
                }
            }
        } while (collision);
    }
    domain->mappings[slot].iova = candidate;
    domain->mappings[slot].physical = physical;
    domain->mappings[slot].length = length;
    domain->mappings[slot].permissions = permissions;
    domain->mappings[slot].active = 1;
    *iova = candidate;
    return 1;
}

static int iommu_domain_unmap(iommu_domain *domain,
                              unsigned int requester_id,
                              unsigned long long iova,
                              unsigned long long length)
{
    unsigned int index;
    if (!domain || !domain->attached ||
        domain->requester_id != requester_id || !length)
        return 0;
    for (index = 0; index < IOMMU_MAX_MAPPINGS; index++) {
        iommu_mapping *mapping = &domain->mappings[index];
        if (mapping->active && mapping->iova == iova &&
            mapping->length == length) {
            mapping->active = 0;
            return 1;
        }
    }
    return 0;
}

static int iommu_domain_detach(iommu_domain *domain,
                               unsigned int requester_id)
{
    unsigned int index;
    if (!domain || !domain->attached ||
        domain->requester_id != requester_id)
        return 0;
    for (index = 0; index < IOMMU_MAX_MAPPINGS; index++) {
        if (domain->mappings[index].active)
            return 0;
    }
    domain->attached = 0;
    domain->requester_id = 0;
    return 1;
}

static int iommu_domain_fault(iommu_domain *domain,
                              unsigned int requester_id)
{
    if (!domain || !domain->attached ||
        domain->requester_id != requester_id)
        return 0;
    domain->fault_count++;
    domain->faulted = 1;
    return 1;
}

#endif
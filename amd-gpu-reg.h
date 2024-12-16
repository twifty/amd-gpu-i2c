/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_AMD_GPU_REGISTRY_H
#define _UAPI_AMD_GPU_REGISTRY_H

#include <linux/pci.h>
#include <linux/types.h>
#include "include/types.h"

struct reg_fields {
    uint32_t    mask;
    uint32_t    value;
    uint8_t     shift;
    uint32_t    *read;
};

struct amd_reg_service {
    void *private;
};



int32_t reg_read (
    struct amd_reg_service *service,
    uint32_t reg
);
void reg_write (
    struct amd_reg_service *service,
    uint32_t reg,
    uint32_t value
);

uint32_t reg_get_field_value(
    const struct reg_fields *field
);
uint32_t reg_set_field_value(
    uint32_t value,
    struct reg_fields *field
);

#define reg_field_get_value_ex  reg_get_field_value
#define reg_field_set_value_ex  reg_set_field_value

#define reg_field_get_value(value, reg, field)                  \
    reg_field_get_value_ex((struct reg_fields[]){               \
        REG_FIELD(reg, field, value),                           \
    })
#define reg_field_set_value(set_value, value, reg, field)       \
    reg_field_set_value_ex(set_value, (struct reg_fields[]){    \
        REG_FIELD(reg, field, value),                           \
    })

uint32_t reg_update_ex(
    struct amd_reg_service *service,
    uint32_t addr,
    const struct reg_fields *fields,
    ssize_t cnt
);
uint32_t reg_update_seq_ex(
    struct amd_reg_service *service,
    uint32_t addr,
    const struct reg_fields *fields,
    ssize_t cnt
);
uint32_t reg_set_ex(
    struct amd_reg_service *service,
    uint32_t addr,
    uint32_t init,
    const struct reg_fields *fields,
    ssize_t cnt
);
uint32_t reg_get_ex(
    struct amd_reg_service *service,
    uint32_t addr,
    struct reg_fields *fields,
    ssize_t cnt
);
uint32_t reg_get_value(
    struct amd_reg_service *service,
    uint32_t addr,
    struct reg_fields *fields,
    uint32_t *value
);
void reg_wait_ex(
    struct amd_reg_service *service,
    uint32_t addr,
    struct reg_fields *field,
    uint32_t attempts,
    uint32_t timeout
);

struct amd_reg_service *amd_gpu_reg_create(
    struct pci_dev *pci_dev
);
void amd_gpu_reg_destroy (
    struct amd_reg_service *service
);

#endif

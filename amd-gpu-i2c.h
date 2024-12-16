/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_AMD_GPU_I2C_H
#define _UAPI_AMD_GPU_I2C_H

#include <linux/i2c.h>

#include "amd-gpu-pci.h"
#include "amd-gpu-reg.h"
#include "amd-i2c-base.h"

enum amd_gpu_i2c_engine {
    AURA_GPU_I2C_NONE,
    AURA_GPU_I2C_DCN,
    AURA_GPU_I2C_DCE
};

struct amd_gpu_i2c_context {
    struct amd_pci_asic     asic;
    struct amd_reg_service  *reg_service;
    struct i2c_adapter      adapter;
    bool                    registered;

    enum amd_gpu_i2c_engine engine;
    struct amd_gpu_i2c      *gpu_ctx;
};

void amd_gpu_i2c_destroy_context (
    struct amd_gpu_i2c_context *context
);

struct amd_gpu_i2c_context *amd_gpu_i2c_create_context(
    const struct amd_pci_entry *const gpu,
    uint8_t index
);

#endif
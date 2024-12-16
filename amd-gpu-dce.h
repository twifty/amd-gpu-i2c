#ifndef _UAPI_AMD_GPU_DCE_I2C_H
#define _UAPI_AMD_GPU_DCE_I2C_H

#include <linux/i2c.h>

#include "amd-pci-ids.h"

struct amd_gpu_i2c *amd_gpu_dce_create (
    const struct amd_pci_asic *asic,
    struct amd_reg_service *reg_service
);

#endif

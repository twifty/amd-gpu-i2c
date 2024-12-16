#ifndef _UAPI_AMD_GPU_DCN_I2C_H
#define _UAPI_AMD_GPU_DCN_I2C_H

#include <linux/i2c.h>

#include "amd-pci-ids.h"

//struct amd_gpu_dcn_context {
//    void *private;
//};

struct amd_gpu_i2c *amd_gpu_dcn_create (
    const struct amd_pci_asic *asic,
    struct amd_reg_service *reg_service
);

//void amd_gpu_dcn_destroy (
//    struct amd_gpu_dcn_context *context
//);
//
//int amd_gpu_dcn_xfer(
//    struct amd_gpu_dcn_context *context,
//	struct i2c_msg *msgs,
//    int num
//);

#endif
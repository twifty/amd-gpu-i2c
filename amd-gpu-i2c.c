/* SPDX-License-Identifier: GPL-2.0 */
#include "include/debug.h"
#include "amd-gpu-i2c.h"
#include "amd-gpu-pci.h"
#include "amd-gpu-dce.h"
#include "amd-gpu-dcn.h"

static int amd_gpu_i2c_xfer(
    struct i2c_adapter *i2c_adap,
    struct i2c_msg *msgs,
    int num
){
    struct amd_gpu_i2c_context *context = i2c_get_adapdata(i2c_adap);
    
    return context->gpu_ctx->funcs->transfer(context->gpu_ctx, msgs, num);
}

static uint32_t amd_gpu_i2c_func(
    struct i2c_adapter *adap
){
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm amdgpu_dm_i2c_algo = {
    .master_xfer   = amd_gpu_i2c_xfer,
    .functionality = amd_gpu_i2c_func,
};

void amd_gpu_i2c_destroy_context (
    struct amd_gpu_i2c_context *context
){
    if (context->gpu_ctx)
        context->gpu_ctx->funcs->destroy(context->gpu_ctx);

    if (context->reg_service)
        amd_gpu_reg_destroy(context->reg_service);

    if (context->registered) {
        i2c_set_adapdata(&context->adapter, NULL);
        i2c_del_adapter(&context->adapter);
    }

    kfree(context);
}

struct amd_gpu_i2c_context *amd_gpu_i2c_create_context(
    const struct amd_pci_entry *const gpu,
    uint8_t index
){
    struct amd_gpu_i2c_context *context;
    error_t err;

    context = kzalloc(sizeof(struct amd_gpu_i2c_context), GFP_KERNEL);
    if (!context)
        return ERR_PTR(-ENOMEM);

    context->asic = gpu->asic;

    if (context->asic.version > DCE_VERSION_UNKNOWN && context->asic.version < DCN_VERSION_MAX) {
        if (context->asic.version < DCE_VERSION_MAX)
            context->engine = AURA_GPU_I2C_DCE;
        else
            context->engine = AURA_GPU_I2C_DCN;
    } else {
        goto error_free_all;
    }

    context->reg_service = amd_gpu_reg_create(gpu->pci);
    if (IS_ERR_OR_NULL(context->reg_service)) {
        err = CLEAR_ERR(context->reg_service);
        goto error_free_all;
    }

    switch (context->engine)
    {
    case AURA_GPU_I2C_DCN:
        context->gpu_ctx = amd_gpu_dcn_create(&context->asic, context->reg_service);
        break;

    case AURA_GPU_I2C_DCE:
        context->gpu_ctx = amd_gpu_dce_create(&context->asic, context->reg_service);
        break;
    
    default:
        goto error_free_all;
    }

    if (IS_ERR_OR_NULL(context->gpu_ctx)) {
        err = CLEAR_ERR(context->gpu_ctx);
        goto error_free_all;
    }
    
    context->adapter.owner = THIS_MODULE;
    context->adapter.algo = &amdgpu_dm_i2c_algo;
    context->adapter.dev.parent = &gpu->pci->dev;

    snprintf(context->adapter.name, sizeof(context->adapter.name),
        "AMD-GPU-I2C hidden bus %d", index);

    i2c_set_adapdata(&context->adapter, context);
    i2c_add_adapter(&context->adapter);

    context->registered = true;

    return context;

error_free_all:
    amd_gpu_i2c_destroy_context(context);

    return ERR_PTR(err);
}

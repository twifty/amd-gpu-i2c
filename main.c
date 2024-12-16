// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>

#include "include/debug.h"
#include "amd-gpu-pci.h"
#include "amd-gpu-i2c.h"

#define MAX_GPUS 2

static struct amd_gpu_i2c_context *amd_i2cs[MAX_GPUS] = {0};
static uint32_t amd_count = 0;

static void destroy_i2cs (
    void
){
    uint32_t i;

    for (i = 0; i < amd_count; i++) {
        amd_gpu_i2c_destroy_context(amd_i2cs[i]);
        amd_i2cs[i] = NULL;
    }

    amd_count = 0;
}

static int __init amd_module_init (
    void
){
    struct amd_pci_entry entries[MAX_GPUS];
    uint32_t num_pcis, i;
    int err = 0;

    num_pcis = find_pci_devices(entries, MAX_GPUS);

    for (i = 0; i < num_pcis; i++) {
        amd_i2cs[i] = amd_gpu_i2c_create_context(&entries[i], i);
        if (IS_ERR_OR_NULL(amd_i2cs[i])) {
            err = CLEAR_ERR(amd_i2cs[i]);
            goto error;
        }
    }

    amd_count = num_pcis;

    return 0;
error:
    destroy_i2cs();

    return err;
}

static void __exit amd_module_exit (
    void
){
    destroy_i2cs();
}

module_init(amd_module_init);
module_exit(amd_module_exit);


MODULE_AUTHOR("Owen Parry <waldermort@gmail.com>");
MODULE_DESCRIPTION("AMD GPU I2C driver");
MODULE_LICENSE("GPL");

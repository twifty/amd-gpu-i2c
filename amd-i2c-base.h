/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_AMD_I2C_BASE_H
#define _UAPI_AMD_I2C_BASE_H
#include <linux/i2c.h>

#define MS_WFIELDS(_base,_field,_value) {	\
    .mask = _base->masks->_field,           \
    .shift = _base->shifts->_field,         \
    .value = _value }
#define MS_RFIELDS(_base,_field,_ptr) {     \
    .mask = _base->masks->_field,           \
    .shift = _base->shifts->_field,         \
    .read = _ptr }

struct amd_gpu_i2c;

typedef void (*i2c_destroy_t)(
        struct amd_gpu_i2c *gpu
    );
typedef int (*i2c_transfer_t)(
        struct amd_gpu_i2c *gpu,
        struct i2c_msg *msgs,
        int num
    );

struct amd_gpu_i2c_funcs {
    i2c_destroy_t	destroy;
    i2c_transfer_t	transfer;
};

struct amd_gpu_i2c {
    const struct amd_gpu_i2c_funcs *funcs;
};

#endif

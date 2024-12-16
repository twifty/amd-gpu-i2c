/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_AMD_I2C_BASE_H
#define _UAPI_AMD_I2C_BASE_H
#include <linux/i2c.h>

#define MS_WFIELDS(_base,_field,_value) { 	\
	.mask = _base->masks->_field, 			\
	.shift = _base->shifts->_field, 		\
	.value = _value }
#define MS_RFIELDS(_base,_field,_ptr) { 	\
	.mask = _base->masks->_field, 			\
	.shift = _base->shifts->_field, 		\
	.read = _ptr }

//struct i2c_msg {
//	uint16_t addr;
//	uint16_t flags;
//#define I2C_M_RD		0x0001	/* guaranteed to be 0x0001! */
//#define I2C_M_TEN		0x0010	/* use only if I2C_FUNC_10BIT_ADDR */
//#define I2C_M_DMA_SAFE		0x0200	/* use only in kernel space */
//#define I2C_M_RECV_LEN		0x0400	/* use only if I2C_FUNC_SMBUS_READ_BLOCK_DATA */
//#define I2C_M_NO_RD_ACK		0x0800	/* use only if I2C_FUNC_PROTOCOL_MANGLING */
//#define I2C_M_IGNORE_NAK	0x1000	/* use only if I2C_FUNC_PROTOCOL_MANGLING */
//#define I2C_M_REV_DIR_ADDR	0x2000	/* use only if I2C_FUNC_PROTOCOL_MANGLING */
//#define I2C_M_NOSTART		0x4000	/* use only if I2C_FUNC_NOSTART */
//#define I2C_M_STOP		0x8000	/* use only if I2C_FUNC_PROTOCOL_MANGLING */
//	uint16_t len;
//	uint8_t *buf;
//};

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
	i2c_destroy_t 	destroy;
	i2c_transfer_t 	transfer;
};

struct amd_gpu_i2c {
	const struct amd_gpu_i2c_funcs *funcs;
};

#endif
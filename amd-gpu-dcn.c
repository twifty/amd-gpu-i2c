// SPDX-License-Identifier: GPL-2.0
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/err.h>

#include "include/debug.h"
#include "amd-i2c-base.h"
#include "amd-gpu-reg.h"
#include "amd-gpu-dcn.h"

#define GPIO_WFIELD(_base,_pin,_val) {		\
	.mask = _base->_pin ## _mask,			\
	.shift = _base->_pin ## _shift,			\
	.value = _val }
#define GPIO_RFIELD(_base,_pin,_ptr) {		\
	.mask = _base->_pin ## _mask,			\
	.shift = _base->_pin ## _shift,			\
	.read = _ptr }

enum {
	DCE_I2C_DEFAULT_I2C_SW_SPEED = 50,
	I2C_SW_RETRIES = 10,
	I2C_SW_TIMEOUT_DELAY = 3000,
};

enum gpio_pin_id {
	GPIO_PIN_UNKNOWN,
	GPIO_PIN_DATA,
	GPIO_PIN_CLOCK,
};

struct dcn_i2c_registers {
	uint32_t MASK_reg;
	uint32_t MASK_mask;
	uint32_t MASK_shift;
	uint32_t A_reg;
	uint32_t A_mask;
	uint32_t A_shift;
	uint32_t EN_reg;
	uint32_t EN_mask;
	uint32_t EN_shift;
	uint32_t Y_reg;
	uint32_t Y_mask;
	uint32_t Y_shift;
};
struct dcn_i2c_sh_mask {
	/* i2c_dd_setup */
	uint32_t DC_I2C_DDC1_ENABLE;
	uint32_t DC_I2C_DDC1_EDID_DETECT_ENABLE;
	uint32_t DC_I2C_DDC1_EDID_DETECT_MODE;
	/* ddc1_mask */
	uint32_t DC_GPIO_DDC1DATA_PD_EN;
	uint32_t DC_GPIO_DDC1CLK_PD_EN;
	uint32_t AUX_PAD1_MODE;
	/* i2cpad_mask */
	// uint32_t DC_GPIO_SDA_PD_DIS;
	// uint32_t DC_GPIO_SCL_PD_DIS;
	// //phy_aux_cntl
	// uint32_t AUX_PAD_RXSEL;
	// uint32_t DDC_PAD_I2CMODE;
};

struct dcn_i2c_pin {
	enum gpio_pin_id id;
	bool opened;

	struct {
		uint32_t mask;
		uint32_t a;
		uint32_t en;
		uint32_t mux;
	} store;

	const struct dcn_i2c_registers *regs;
	const struct dcn_i2c_sh_mask *shifts;
	const struct dcn_i2c_sh_mask *masks;

    struct amd_reg_service *reg_service;
};
struct dcn_i2c_context {
	struct amd_gpu_i2c	gpu_context;
	struct dcn_i2c_pin 	pin_data;
	struct dcn_i2c_pin 	pin_clock;

	uint32_t 			delay;
	uint32_t 			speed_khz;
	uint16_t 			msecs;
};

#define dcn_from_gpu(ptr) ( \
    container_of(ptr, struct dcn_i2c_context, gpu_context) \
)

static const struct dcn_i2c_registers amd_dcn2_data = {
	.MASK_reg 	= 0x000034C0 + 0x28e8,
	.EN_reg 	= 0x000034C0 + 0x28ea,
	.A_reg 		= 0x000034C0 + 0x28e9,
	.Y_reg 		= 0x000034C0 + 0x28eb,
	.MASK_mask 	= 0x00000100L,
	.EN_mask 	= 0x00000100L,
	.A_mask 	= 0x00000100L,
	.Y_mask 	= 0x00000100L,
	.MASK_shift = 0x8,
	.EN_shift 	= 0x8,
	.A_shift 	= 0x8,
	.Y_shift 	= 0x8,
};
static const struct dcn_i2c_registers amd_dcn2_clock = {
	.MASK_reg 	= 0x000034C0 + 0x28e8,
	.EN_reg 	= 0x000034C0 + 0x28ea,
	.A_reg 		= 0x000034C0 + 0x28e9,
	.Y_reg 		= 0x000034C0 + 0x28eb,
	.MASK_mask 	= 0x00000001L,
	.EN_mask 	= 0x00000001L,
	.A_mask 	= 0x00000001L,
	.Y_mask 	= 0x00000001L,
	.MASK_shift = 0x0,
	.EN_shift 	= 0x0,
	.A_shift 	= 0x0,
	.Y_shift 	= 0x0,
};
static const struct dcn_i2c_sh_mask amd_dcn2_shift = {
	.DC_I2C_DDC1_ENABLE = 0x6,
	.DC_I2C_DDC1_EDID_DETECT_ENABLE = 0x4,
	.DC_I2C_DDC1_EDID_DETECT_MODE = 0x5,
	.DC_GPIO_DDC1DATA_PD_EN = 0xc,
	.DC_GPIO_DDC1CLK_PD_EN = 0x4,
	.AUX_PAD1_MODE = 0x10,
};
static const struct dcn_i2c_sh_mask amd_dcn2_mask = {
	.DC_I2C_DDC1_ENABLE = 0x00000040L,
	.DC_I2C_DDC1_EDID_DETECT_ENABLE = 0x00000010L,
	.DC_I2C_DDC1_EDID_DETECT_MODE = 0x00000020L,
	.DC_GPIO_DDC1DATA_PD_EN = 0x00001000L,
	.DC_GPIO_DDC1CLK_PD_EN = 0x00000010L,
	.AUX_PAD1_MODE = 0x00010000L,
};

static error_t pin_init(
	struct dcn_i2c_pin *pin,
    struct amd_reg_service *reg_service,
	enum gpio_pin_id id
){
	pin->id = id;
	pin->opened = false;

	pin->store.mask = 0;
	pin->store.a = 0;
	pin->store.en = 0;
	pin->store.mux = 0;

    pin->reg_service = reg_service;

	switch (pin->id) {
	case GPIO_PIN_DATA:
		pin->regs = &amd_dcn2_data;
		break;
	case GPIO_PIN_CLOCK:
		pin->regs = &amd_dcn2_clock;
		break;
	default:
		return -EINVAL;
	}

	pin->shifts = &amd_dcn2_shift;
	pin->masks = &amd_dcn2_mask;

	return 0;
}

static void pin_close(
	struct dcn_i2c_pin *pin
){
    const struct dcn_i2c_registers *reg = pin->regs;

	// Restore the register states
	reg_update_ex(pin->reg_service, reg->MASK_reg, (struct reg_fields[]){
		GPIO_WFIELD(reg, MASK, pin->store.mask),
	}, 1);
	reg_update_ex(pin->reg_service, reg->A_reg, (struct reg_fields[]){
		GPIO_WFIELD(reg, A, pin->store.a),
	}, 1);
	reg_update_ex(pin->reg_service, reg->EN_reg, (struct reg_fields[]){
		GPIO_WFIELD(reg, EN, pin->store.en),
	}, 1);

	pin->opened = false;
}

static void pin_open(
	struct dcn_i2c_pin *pin
){
    const struct dcn_i2c_registers *reg = pin->regs;

	// Store the register states
    reg_get_ex(pin->reg_service, reg->MASK_reg, (struct reg_fields[]){
        GPIO_RFIELD(reg, MASK, &pin->store.mask),
    }, 1);
    reg_get_ex(pin->reg_service, reg->A_reg, (struct reg_fields[]){
        GPIO_RFIELD(reg, A, &pin->store.a),
    }, 1);
    reg_get_ex(pin->reg_service, reg->EN_reg, (struct reg_fields[]){
        GPIO_RFIELD(reg, EN, &pin->store.en),
    }, 1);

	//pin->opened = (dal_hw_gpio_change_mode(pin) == GPIO_RESULT_OK);

	// Increase rise time by grounding the A register and using EN
	reg_update_ex(pin->reg_service, reg->A_reg, (struct reg_fields[]){
		GPIO_WFIELD(reg, A, 0),
	}, 1);
	reg_update_ex(pin->reg_service, reg->MASK_reg, (struct reg_fields[]){
		GPIO_WFIELD(reg, MASK, 1),
	}, 1);

	pin->opened = true;

	//return pin->opened;
}

static void pin_set_config(
	struct dcn_i2c_pin *pin
){
    const struct dcn_i2c_registers *reg = pin->regs;

	uint32_t regval;
	uint32_t ddc_data_pd_en = 0;
	uint32_t ddc_clk_pd_en = 0;
	//uint32_t aux_pad_mode = 0;

    regval = reg_get_ex(pin->reg_service, reg->MASK_reg, (struct reg_fields[]){
        MS_RFIELDS(pin, DC_GPIO_DDC1DATA_PD_EN, &ddc_data_pd_en),
        MS_RFIELDS(pin, DC_GPIO_DDC1CLK_PD_EN, &ddc_clk_pd_en),
        //MS_RFIELDS(pin, AUX_PAD1_MODE, &aux_pad_mode),
	}, 2);

	if (!ddc_data_pd_en || !ddc_clk_pd_en) {
		reg_set_ex(pin->reg_service, reg->MASK_reg, regval, (struct reg_fields[]){
			MS_WFIELDS(pin, DC_GPIO_DDC1DATA_PD_EN, 1),
		}, 1);
	}
}

static inline bool pin_read(
	struct dcn_i2c_pin *pin
){
	uint32_t value = 0;

	reg_get_ex(pin->reg_service, pin->regs->Y_reg, (struct reg_fields[]){
		GPIO_RFIELD(pin->regs, Y, &value),
	}, 1);

	return (value != 0);
}

static inline void pin_write(
	struct dcn_i2c_pin *pin,
	bool bit
){
    const struct dcn_i2c_registers *reg = pin->regs;
	uint32_t value = bit ? 1 : 0;

	reg_update_ex(pin->reg_service, reg->EN_reg, (struct reg_fields[]){
		GPIO_WFIELD(reg, EN, ~value),
	}, 1);
}


static void close_engine(
	struct dcn_i2c_context *ctx
){
	pin_close(&ctx->pin_clock);
	pin_close(&ctx->pin_data);
}

static void open_engine(
	struct dcn_i2c_context *ctx
){
	pin_open(&ctx->pin_data);
	pin_open(&ctx->pin_clock);

	pin_set_config(&ctx->pin_data/*, &config_data*/);
}


static bool dcn_i2c_wait_clock(
	struct dcn_i2c_context *ctx,
	uint16_t scl_delay
){
	uint32_t scl_retry = 0;
	uint32_t scl_retry_max = I2C_SW_TIMEOUT_DELAY / scl_delay;

	udelay(scl_delay);

	do {
		if (pin_read(&ctx->pin_clock))
			return true;

		udelay(scl_delay);

		++scl_retry;
	} while (scl_retry <= scl_retry_max);

	return false;
}

static bool dcn_i2c_read_byte(
	struct dcn_i2c_context *ctx,
	uint8_t *byte,
	bool more
){
	struct dcn_i2c_pin *sda = &ctx->pin_data;
	struct dcn_i2c_pin *scl = &ctx->pin_clock;
	uint16_t msecs = ctx->msecs;
	int32_t shift = 7;

	uint8_t data = 0;

	/* The data bits are read from MSB to LSB;
	 * bit is read while SCL is high
	 */

	do {
		pin_write(scl, true);

		if (!dcn_i2c_wait_clock(ctx, msecs))
			return false;

		if (pin_read(sda))
			data |= (1 << shift);

		pin_write(scl, false);

		udelay(msecs << 1);

		--shift;
	} while (shift >= 0);

	/* read only whole byte */

	*byte = data;

	udelay(msecs);

	/* send the acknowledge bit:
	 * SDA low means ACK, SDA high means NACK
	 */

	pin_write(sda, !more);

	udelay(msecs);

	pin_write(scl, true);

	if (!dcn_i2c_wait_clock(ctx, msecs))
		return false;

	pin_write(scl, false);

	udelay(msecs);

	pin_write(sda, true);

	udelay(msecs);

	return true;
}

static bool dcn_i2c_write_byte(
	struct dcn_i2c_context *ctx,
	uint8_t byte
){
	struct dcn_i2c_pin *sda = &ctx->pin_data;
	struct dcn_i2c_pin *scl = &ctx->pin_clock;
	uint16_t msecs = ctx->msecs;
	int32_t shift = 7;
	bool ack;

	/* bits are transmitted serially, starting from MSB */

	do {
		udelay(msecs);

		pin_write(sda, (byte >> shift) & 1);

		udelay(msecs);

		pin_write(scl, true);

		if (!dcn_i2c_wait_clock(ctx, msecs))
			return false;

		pin_write(scl, false);

		--shift;
	} while (shift >= 0);

	/* The display sends ACK by preventing the SDA from going high
	 * after the SCL pulse we use to send our last data bit.
	 * If the SDA goes high after that bit, it's a NACK
	 */

	udelay(msecs);

	pin_write(sda, true);

	udelay(msecs);

	pin_write(scl, true);

	if (!dcn_i2c_wait_clock(ctx, msecs))
		return false;

	/* read ACK bit */

	ack = !pin_read(sda);

	udelay(msecs << 1);

	pin_write(scl, false);

	udelay(msecs << 1);

	return ack;
}

static bool dcn_i2c_read(
	struct dcn_i2c_context *ctx,
	uint8_t address,
	uint32_t length,
	uint8_t *data
){
	uint32_t i = 0;

	if (!dcn_i2c_write_byte(ctx, address))
		return false;

	while (i < length) {
		if (!dcn_i2c_read_byte(ctx, data + i, i < length - 1))
			return false;
		++i;
	}

	return true;
}

static bool dcn_i2c_write(
	struct dcn_i2c_context *ctx,
	uint8_t address,
	uint32_t length,
	const uint8_t *data
){
	uint32_t i = 0;

	if (!dcn_i2c_write_byte(ctx, address))
		return false;

	while (i < length) {
		if (!dcn_i2c_write_byte(ctx, data[i]))
			return false;
		++i;
	}

	return true;
}

static bool dcn_i2c_start(
	struct dcn_i2c_context *ctx
){
	struct dcn_i2c_pin *sda = &ctx->pin_data;
	struct dcn_i2c_pin *scl = &ctx->pin_clock;
	uint16_t msecs = ctx->msecs;
	uint32_t retry = 0;

	/* The I2C communications start signal is:
	 * the SDA going low from high, while the SCL is high.
	 */

	pin_write(scl, true);

	udelay(msecs);

	do {
		pin_write(sda, true);

		if (!pin_read(sda)) {
			++retry;
			continue;
		}

		udelay(msecs);

		pin_write(scl, true);

		if (!dcn_i2c_wait_clock(ctx, msecs))
			break;

		pin_write(sda, false);

		udelay(msecs);

		pin_write(scl, false);

		udelay(msecs);

		return true;
	} while (retry <= I2C_SW_RETRIES);

	return false;
}

static bool dcn_i2c_stop(
	struct dcn_i2c_context *ctx
){
	struct dcn_i2c_pin *sda = &ctx->pin_data;
	struct dcn_i2c_pin *scl = &ctx->pin_clock;
	uint16_t msecs = ctx->msecs;
	uint32_t retry = 0;

	/* The I2C communications stop signal is:
	 * the SDA going high from low, while the SCL is high.
	 */

	pin_write(scl, false);

	udelay(msecs);

	pin_write(sda, false);

	udelay(msecs);

	pin_write(scl, true);

	if (!dcn_i2c_wait_clock(ctx, msecs))
		return false;

	pin_write(sda, true);

	do {
		udelay(msecs);

		if (pin_read(sda))
			return true;

		++retry;
	} while (retry <= 2);

	return false;
}

static bool dcn_i2c_submit_payload(
    struct dcn_i2c_context *ctx,
	struct i2c_msg *msg,
	bool middle_of_transaction
){
	bool write;
	uint8_t address;
	bool result;

	write = !(msg->flags & I2C_M_RD);
	address = (uint8_t) ((msg->addr << 1) | !write);
	result = dcn_i2c_start(ctx);

	/* process payload */
	if (result) {
		if (write)
			result = dcn_i2c_write(ctx, address, msg->len, msg->buf);
		else
			result = dcn_i2c_read(ctx, address, msg->len, msg->buf);
	}

	if (!result || !middle_of_transaction)
		if (!dcn_i2c_stop(ctx))
			result = false;

	return result;
}

static int amd_gpu_dcn_xfer(
    struct amd_gpu_i2c *gpu,
	struct i2c_msg *msgs,
    int num
){
	struct dcn_i2c_context *ctx = dcn_from_gpu(gpu);

	uint8_t index = 0;
	bool result = true;

	//ctx->speed_khz = cmd->speed_khz ? cmd->speed_khz : DCE_I2C_DEFAULT_I2C_SW_SPEED;
	//ctx->delay = 1000 / ctx->speed_khz;
	//if (ctx->delay < 12)
	//	ctx->delay = 12;
	//ctx->msecs = ctx->delay >> 2;

	open_engine(ctx);

	while (index < num) {
		bool mot = (index != num - 1);

		if (!dcn_i2c_submit_payload(ctx, &msgs[index], mot)) {
			result = false;
			break;
		}

		++index;
	}

	close_engine(ctx);

	return result ? num : -EIO;
}

static void amd_gpu_dcn_destroy (
    struct amd_gpu_i2c *gpu
){
	struct dcn_i2c_context *context = dcn_from_gpu(gpu);

    kfree(context);
}

static const struct amd_gpu_i2c_funcs funcs = {
    .transfer = amd_gpu_dcn_xfer,
    .destroy = amd_gpu_dcn_destroy,
};

struct amd_gpu_i2c *amd_gpu_dcn_create (
    const struct amd_pci_asic *asic,
    struct amd_reg_service *reg_service
){
	struct dcn_i2c_context *context;

	context = kzalloc(sizeof(struct dcn_i2c_context), GFP_KERNEL);
    if (!context)
        return ERR_PTR(-ENOMEM);
	
    pin_init(&context->pin_data, reg_service, GPIO_PIN_DATA);
    pin_init(&context->pin_clock, reg_service, GPIO_PIN_CLOCK);

	context->msecs = 3;
	context->gpu_context.funcs = &funcs;

	return &context->gpu_context;
}

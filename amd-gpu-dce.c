// SPDX-License-Identifier: GPL-2.0
#include <linux/i2c.h>
#include <linux/delay.h>

#include "include/debug.h"
#include "amd-i2c-base.h"
#include "amd-gpu-reg.h"
#include "amd-gpu-dce.h"


 enum {
    GPU_I2C_TIMEOUT_DELAY    = 1000,
    GPU_I2C_TIMEOUT_INTERVAL = 10,
 };

 enum amd_i2c_result {
    OPERATION_SUCCEEDED,
    OPERATION_FAILED,
    OPERATION_ENGINE_BUSY,
    OPERATION_TIMEOUT,
    OPERATION_NO_RESPONSE
 };

 static const char *amd_i2c_result_str[] = {
    "OPERATION_SUCCEEDED",
    "OPERATION_FAILED",
    "OPERATION_ENGINE_BUSY",
    "OPERATION_TIMEOUT",
    "OPERATION_NO_RESPONSE"
 };

// enum amd_i2c_action {
//     DCE_I2C_TRANSACTION_ACTION_I2C_WRITE = 0x00,
//     DCE_I2C_TRANSACTION_ACTION_I2C_READ = 0x10,
//    // DCE_I2C_TRANSACTION_ACTION_I2C_STATUS_REQUEST = 0x20,
//
//     DCE_I2C_TRANSACTION_ACTION_I2C_WRITE_MOT = 0x40,
//     DCE_I2C_TRANSACTION_ACTION_I2C_READ_MOT = 0x50,
//    // DCE_I2C_TRANSACTION_ACTION_I2C_STATUS_REQUEST_MOT = 0x60,
//
//    // DCE_I2C_TRANSACTION_ACTION_DP_WRITE = 0x80,
//    // DCE_I2C_TRANSACTION_ACTION_DP_READ = 0x90
// };

// enum amd_i2c_status {
//     DC_I2C_STATUS__DC_I2C_STATUS_IDLE,
//     DC_I2C_STATUS__DC_I2C_STATUS_USED_BY_SW,
//     DC_I2C_STATUS__DC_I2C_STATUS_USED_BY_HW,
//     DC_I2C_REG_RW_CNTL_STATUS_DMCU_ONLY = 2,
// };

struct amd_dce_registers {
    uint32_t SETUP;
    uint32_t SPEED;
    uint32_t STATUS;
    uint32_t CONTROL;
    uint32_t TRANSACTION;
    uint32_t DATA;
    uint32_t INTERRUPT_CONTROL;
    uint32_t PIN_SELECTION;
};
struct amd_dce_sh_mask {
    uint32_t GO;
    uint32_t SOFT_RESET;
    uint32_t SEND_RESET;
    uint32_t ENABLE;
    uint32_t DBG_REF_SEL;
    uint32_t STATUS;
    uint32_t DONE;
    uint32_t ABORTED;
    uint32_t TIMEOUT;
    uint32_t STOPPED_ON_NACK;
    uint32_t NACK;
    uint32_t THRESHOLD;
    uint32_t DISABLE_FILTER_DURING_STALL;
    uint32_t START_STOP_TIMING_CNTL;
    uint32_t PRESCALE;
    uint32_t DATA_DRIVE_EN;
    uint32_t DATA_DRIVE_SEL;
    uint32_t CLK_DRIVE_EN;
    uint32_t INTRA_BYTE_DELAY;
    uint32_t TIME_LIMIT;
    uint32_t RW;
    uint32_t STOP_ON_NACK;
    uint32_t ACK_ON_READ;
    uint32_t START;
    uint32_t STOP;
    uint32_t COUNT;
    uint32_t DATA_RW;
    uint32_t DATA;
    uint32_t INDEX;
    uint32_t INDEX_WRITE;
    uint32_t SCL_PIN_SEL;
    uint32_t SDA_PIN_SEL;
    uint32_t DONE_INT;
    uint32_t DONE_ACK;
    uint32_t DONE_MASK;
};

struct dce_i2c_context {
    struct amd_gpu_i2c      gpu_context;
    struct amd_reg_service  *reg_service;
    // enum amd_asic_type         asic_type;

    // uint32_t                    original_speed;
    // uint32_t                    default_speed;

    uint32_t                    timeout_delay;
    uint32_t                    timeout_interval;

    struct amd_dce_registers const *registers;
    struct amd_dce_sh_mask const   *shifts;
    struct amd_dce_sh_mask const   *masks;

    // struct i2c_adapter          i2c_adapter;
    // struct mutex                mutex;
};

#define dce_from_gpu(ptr) ( \
    container_of(ptr, struct dce_i2c_context, gpu_context) \
)


//struct amd_i2c_payload {
//    bool        write;
//    uint8_t     address;
//    uint32_t    length;
//    uint8_t     *data;
// };

//struct amd_i2c_transaction {
//    //enum amd_i2c_action   action;
//    //enum amd_i2c_result   status;
//    uint8_t                address;
//    uint32_t               length;
//    uint8_t                *data;
//};


static void clear_ack (
    struct dce_i2c_context *ctx
){
    struct amd_dce_registers const *reg = ctx->registers;

    reg_update_ex(ctx->reg_service, reg->INTERRUPT_CONTROL, (struct reg_fields[]){
        MS_WFIELDS(ctx, DONE_ACK, 1),
    }, 1);
}

static void open_engine (
    struct dce_i2c_context *ctx
){
    struct amd_dce_registers const * const reg = ctx->registers;

    reg_update_ex(ctx->reg_service, reg->CONTROL, (struct reg_fields[]){
        MS_WFIELDS(ctx, ENABLE, 1),
    }, 1);

    reg_update_ex(ctx->reg_service, reg->PIN_SELECTION, (struct reg_fields[]){
        MS_WFIELDS(ctx, SCL_PIN_SEL, 0x29),
        MS_WFIELDS(ctx, SDA_PIN_SEL, 0x28),
    }, 2);
}

static void close_engine (
    struct dce_i2c_context *ctx
){
    struct amd_dce_registers const * reg = ctx->registers;

    reg_update_ex(ctx->reg_service, reg->PIN_SELECTION, (struct reg_fields[]){
        MS_WFIELDS(ctx, SCL_PIN_SEL, 0),
        MS_WFIELDS(ctx, SDA_PIN_SEL, 0),
    }, 2);

    reg_update_ex(ctx->reg_service, reg->CONTROL, (struct reg_fields[]){
        MS_WFIELDS(ctx, ENABLE, 0),
        MS_WFIELDS(ctx, SOFT_RESET, 1),
    }, 2);

    reg_update_ex(ctx->reg_service, reg->CONTROL, (struct reg_fields[]){
        MS_WFIELDS(ctx, SOFT_RESET, 0),
    }, 1);
}


static bool process_transaction (
    struct dce_i2c_context const *ctx,
    //struct amd_i2c_transaction *request
    struct i2c_msg *msg
){
    struct amd_dce_registers const *reg = ctx->registers;
    uint32_t length = msg->len;
    uint8_t *buffer = msg->buf;
    bool write = !(msg->flags & I2C_M_RD);
    //uint32_t value = 0;

    //write = !(msg->flags & I2C_M_RD);

    reg_update_ex(ctx->reg_service, reg->TRANSACTION, (struct reg_fields[]){
        MS_WFIELDS(ctx, RW, write/*0 != (request->action & DCE_I2C_TRANSACTION_ACTION_I2C_READ)*/),
        MS_WFIELDS(ctx, STOP_ON_NACK, 1),
        MS_WFIELDS(ctx, ACK_ON_READ, 0),
        MS_WFIELDS(ctx, START, 1),
        MS_WFIELDS(ctx, STOP, true ? 1 : 0),
        MS_WFIELDS(ctx, COUNT, length),
    }, 6);

    reg_set_ex(ctx->reg_service, reg->DATA, 0, (struct reg_fields[]){
        MS_WFIELDS(ctx, DATA_RW, false),
        MS_WFIELDS(ctx, DATA, msg->addr),
        MS_WFIELDS(ctx, INDEX, 0),
        MS_WFIELDS(ctx, INDEX_WRITE, 1),
    }, 4);

    //if (!(request->action & DCE_I2C_TRANSACTION_ACTION_I2C_READ)) {
    if (write){
        uint8_t index = 1;
        while (length) {
            reg_set_ex(ctx->reg_service, reg->DATA, 0, (struct reg_fields[]){
                MS_WFIELDS(ctx, INDEX, index),
                MS_WFIELDS(ctx, DATA, *buffer++),
            }, 2);

            ++index;
            --length;
        }
    }

    return true;
}

static void execute_transaction (
    struct dce_i2c_context const *ctx
){
    struct amd_dce_registers const *reg = ctx->registers;

    reg_update_ex(ctx->reg_service, reg->CONTROL, (struct reg_fields[]){
        MS_WFIELDS(ctx, GO, 1),
    }, 1);
}

static void process_reply (
    struct dce_i2c_context *ctx,
    struct i2c_msg *msg
    //struct amd_i2c_payload *reply
){
    struct amd_dce_registers const *reg = ctx->registers;
    uint32_t length = msg->len;
    uint8_t *buffer = msg->buf;
    uint8_t index = 1;
    struct reg_fields data[] = {
        MS_WFIELDS(ctx, INDEX, 1),
        MS_WFIELDS(ctx, DATA, 0)
    };

    reg_set_ex(ctx->reg_service, reg->DATA, 0, (struct reg_fields[]){
        MS_WFIELDS(ctx, DATA_RW, 1),
        MS_WFIELDS(ctx, INDEX, 1),
        MS_WFIELDS(ctx, INDEX_WRITE, 1),
    }, 3);

    while (length) {
        data[0].value = ++index;
        reg_get_ex(ctx->reg_service, reg->DATA, data, 2);
        *buffer++ = data[1].value;

        --length;
    }

    clear_ack(ctx);
}

static enum amd_i2c_result get_channel_status (
    struct dce_i2c_context const *ctx
){
    struct amd_dce_registers const *reg = ctx->registers;
    
    struct reg_fields status = MS_WFIELDS(ctx, STATUS, 0);
    uint32_t value = reg_get_ex(ctx->reg_service, reg->STATUS, &status, 1);

    //if (value & ctx->masks->DONE)
    //    return OPERATION_SUCCEEDED;
    if (status.value || value == 0)
        return OPERATION_ENGINE_BUSY;
    if (value & ctx->masks->STOPPED_ON_NACK || value & ctx->masks->NACK)
        return OPERATION_NO_RESPONSE;
    if (value & ctx->masks->TIMEOUT)
        return OPERATION_TIMEOUT;
    if (value & ctx->masks->ABORTED)
        return OPERATION_FAILED;

    return OPERATION_SUCCEEDED;
}

static bool poll_engine (
    struct dce_i2c_context *ctx
){
    enum amd_i2c_result result;
    uint32_t timeout = ctx->timeout_interval;

    do {
        result = get_channel_status(ctx);
        if (result != OPERATION_ENGINE_BUSY)
            break;

        udelay(ctx->timeout_delay);
    } while (timeout--);

    clear_ack(ctx);

    switch (result) {
    case OPERATION_SUCCEEDED:
        return true;

    case OPERATION_FAILED:
    case OPERATION_ENGINE_BUSY:
    case OPERATION_TIMEOUT:
    case OPERATION_NO_RESPONSE:
        I2C_DBG("Poll failed with: %s", amd_i2c_result_str[result]);
        break;
    }

    return false;
}


//static bool submit_transaction (
//    struct dce_i2c_context *ctx,
//    struct amd_i2c_transaction *request
//){
//    if (!process_transaction(ctx, request)) {
//        I2C_DBG("Failed to process transaction");
//        return false;
//    }
//
//    execute_transaction(ctx);
//
//    return true;
//}

static bool submit_payload (
    struct dce_i2c_context *ctx,
    //struct amd_i2c_payload *payload,
    struct i2c_msg *msg
    //,
    //bool middle_of_transaction
){
    //struct amd_i2c_transaction request;
    //enum amd_i2c_result operation_result;
    bool write;
//
    write = !(msg->flags & I2C_M_RD);
//    if (!write) {
//        request.action = middle_of_transaction ?
//            DCE_I2C_TRANSACTION_ACTION_I2C_READ_MOT :
//            DCE_I2C_TRANSACTION_ACTION_I2C_READ;
//    } else {
//        request.action = middle_of_transaction ?
//            DCE_I2C_TRANSACTION_ACTION_I2C_WRITE_MOT :
//            DCE_I2C_TRANSACTION_ACTION_I2C_WRITE;
//    }
//
//    request.address = (uint8_t) ((payload->address << 1) | !payload->write);
//    request.length  = payload->length;
//    request.data    = payload->data;

    //if (!submit_transaction(ctx, &request))
    //    return false;

    if (!process_transaction(ctx, msg)) {
        I2C_DBG("Failed to process transaction");
        return false;
    }

    // Send GO
    execute_transaction(ctx);

    /* wait until transaction proceed */
    //operation_result = poll_engine(ctx);

    /* update transaction status */
    if (poll_engine(ctx)) {
        if (!write)
            process_reply(ctx, msg);

        return true;
    }

    return false;
}

static const struct amd_dce_registers dce_11_2 = {
    .CONTROL            = 0x16f4,
    .INTERRUPT_CONTROL  = 0x16f5,
    .STATUS             = 0x16f6,
    .SPEED              = 0x16f7,
    .SETUP              = 0x16f8,
    .TRANSACTION        = 0x16f9,
    .DATA               = 0x16fa,
    .PIN_SELECTION      = 0x16fb,
};
static const struct amd_dce_registers dce_12_X = {
    .CONTROL            = 0x34c0 + 0x15a4,
    .INTERRUPT_CONTROL  = 0x34c0 + 0x15a5,
    .STATUS             = 0x34c0 + 0x15a6,
    .SPEED              = 0x34c0 + 0x15a7,
    .SETUP              = 0x34c0 + 0x15a8,
    .TRANSACTION        = 0x34c0 + 0x15a9,
    .DATA               = 0x34c0 + 0x15aa,
    .PIN_SELECTION      = 0x34c0 + 0x15ab,
};
static const struct amd_dce_sh_mask dce_masks = {
    .GO                             = 0x1,
    .SOFT_RESET                     = 0x2,
    .SEND_RESET                     = 0x4,
    .ENABLE                         = 0x8,
    .STATUS                         = 0xf,
    .DONE                           = 0x10,
    .ABORTED                        = 0x20,
    .TIMEOUT                        = 0x40,
    .STOPPED_ON_NACK                = 0x200,
    .NACK                           = 0x400,
    .THRESHOLD                      = 0x3,
    .DISABLE_FILTER_DURING_STALL    = 0x10,
    .START_STOP_TIMING_CNTL         = 0x300,
    .PRESCALE                       = 0xffff0000,
    .DATA_DRIVE_EN                  = 0x1,
    .DATA_DRIVE_SEL                 = 0x2,
    .CLK_DRIVE_EN                   = 0x80,
    .INTRA_BYTE_DELAY               = 0xff00,
    .TIME_LIMIT                     = 0xff000000,
    .RW                             = 0x1,
    .STOP_ON_NACK                   = 0x100,
    .ACK_ON_READ                    = 0x200,
    .START                          = 0x1000,
    .STOP                           = 0x2000,
    .COUNT                          = 0xf0000,
    .DATA_RW                        = 0x1,
    .DATA                           = 0xff00,
    .INDEX                          = 0xf0000,
    .INDEX_WRITE                    = 0x80000000,
    .SDA_PIN_SEL                    = 0x7f00,
    .SCL_PIN_SEL                    = 0x7f,
    .DONE_INT                       = 0x1,
    .DONE_ACK                       = 0x2,
    .DONE_MASK                      = 0x4,
};
static const struct amd_dce_sh_mask dce_shifts = {
    .GO                             = 0x0,
    .SOFT_RESET                     = 0x1,
    .SEND_RESET                     = 0x2,
    .ENABLE                         = 0x3,
    .STATUS                         = 0x0,
    .DONE                           = 0x4,
    .ABORTED                        = 0x5,
    .TIMEOUT                        = 0x6,
    .STOPPED_ON_NACK                = 0x9,
    .NACK                           = 0xa,
    .THRESHOLD                      = 0x0,
    .DISABLE_FILTER_DURING_STALL    = 0x4,
    .START_STOP_TIMING_CNTL         = 0x8,
    .PRESCALE                       = 0x10,
    .DATA_DRIVE_EN                  = 0x0,
    .DATA_DRIVE_SEL                 = 0x1,
    .CLK_DRIVE_EN                   = 0x7,
    .INTRA_BYTE_DELAY               = 0x8,
    .TIME_LIMIT                     = 0x18,
    .RW                             = 0x0,
    .STOP_ON_NACK                   = 0x8,
    .ACK_ON_READ                    = 0x9,
    .START                          = 0xc,
    .STOP                           = 0xd,
    .COUNT                          = 0x10,
    .DATA_RW                        = 0x0,
    .DATA                           = 0x8,
    .INDEX                          = 0x10,
    .INDEX_WRITE                    = 0x1f,
    .SDA_PIN_SEL                    = 0x8,
    .SCL_PIN_SEL                    = 0x0,
    .DONE_INT                       = 0x0,
    .DONE_ACK                       = 0x1,
    .DONE_MASK                      = 0x2,
};

static int amd_gpu_dce_xfer(
    struct amd_gpu_i2c *gpu,
    struct i2c_msg *msgs,
    int num
){
    struct dce_i2c_context *ctx = dce_from_gpu(gpu);
    //struct amd_i2c_payload payload;
    bool result = true;
    int i;

    open_engine(ctx);

    for (i = 0; i < num; i++) {
        //bool mot = (i != num - 1);
        //payload.write   = !(msgs[i].flags & I2C_M_RD);
        //payload.address = msgs[i].addr;
        //payload.length  = msgs[i].len;
        //payload.data    = msgs[i].buf;

        if (!submit_payload(ctx, &msgs[i])) {
            result = false;
            break;
        }
    }

    close_engine(ctx);

    return result ? num : -EIO;
}

static void amd_gpu_dce_destroy (
    struct amd_gpu_i2c *gpu
){
    struct dce_i2c_context *context = dce_from_gpu(gpu);

    kfree(context);
}

static const struct amd_gpu_i2c_funcs funcs = {
    .transfer = amd_gpu_dce_xfer,
    .destroy = amd_gpu_dce_destroy,
};

static bool dce_i2c_init (
    enum amd_asic_type asic,
    struct dce_i2c_context *ctx
){
    ctx->masks = &dce_masks;
    ctx->shifts = &dce_shifts;
    ctx->gpu_context.funcs = &funcs;

    switch (asic)
    {
    case CHIP_POLARIS10:
    case CHIP_POLARIS11:
    case CHIP_POLARIS12:
        ctx->registers = &dce_11_2;
        return true;
    
    case CHIP_VEGA10:
    case CHIP_VEGA12:
    case CHIP_VEGA20:
        ctx->registers = &dce_12_X;
        return true;

    default:
        break;
    }

    return false;
}

struct amd_gpu_i2c *amd_gpu_dce_create (
    const struct amd_pci_asic *asic,
    struct amd_reg_service *reg_service
){
    struct dce_i2c_context *context;

    context = kzalloc(sizeof(struct dce_i2c_context), GFP_KERNEL);
    if (!context)
        return ERR_PTR(-ENOMEM);
    
    if (!dce_i2c_init(asic->type, context)) {
        kfree(context);
        return NULL;
    }

    context->reg_service = reg_service;

    return &context->gpu_context;
}

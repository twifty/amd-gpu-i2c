/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_INCLUDE_DEBUG_H
#define _UAPI_INCLUDE_DEBUG_H

#include "err.h"
#include "types.h"

#define __I2C_PREFIX "AMD-GPU-I2C : "

#define I2C_ERR(_fmt, ...)({ \
    pr_err(__I2C_PREFIX "[%s:%d] " _fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define I2C_WARN(_fmt, ...) ({ \
    pr_warn(__I2C_PREFIX "[%s:%d] " _fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define I2C_DBG(_fmt, ...) ({ \
    pr_debug(__I2C_PREFIX "[%s:%d] " _fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define I2C_INFO(_fmt, ...) ({ \
    pr_info(__I2C_PREFIX "[%s:%d] " _fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define I2C_TRACE(_fmt, ...) ({ \
    pr_debug(__I2C_PREFIX "[%s:%d] " _fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
})

#define I2C_DUMP(p) ({ \
    print_hex_dump_bytes(__I2C_PREFIX "\n", DUMP_PREFIX_NONE, (p), sizeof(*(p)) ); \
})

#define ERR_NAME strerr

#define CLEAR_ERR(_var) ({ \
    error_t ___err = PTR_ERR(_var); \
    if (___err == 0) ___err = -ENOMEM; \
    I2C_DBG("Failed to assign '%s': %s", #_var, ERR_NAME(___err)); \
    _var = NULL; \
    ___err; \
})

#endif

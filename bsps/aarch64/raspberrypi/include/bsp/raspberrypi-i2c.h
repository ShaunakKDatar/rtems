/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup raspberrypi_4_i2c
 *
 * @brief Raspberry Pi specific I2C definitions.
 */

/*
 * Copyright (C) 2025 Shaunak Datar
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBBSP_AARCH64_RASPBERRYPI_4_SPI_H
#define LIBBSP_AARCH64_RASPBERRYPI_4_SPI_H

#include <bsp/utility.h>
#include <bsp/rpi-gpio.h>
#include <bsp/raspberrypi.h>

typedef struct
{
    uint32_t CONTROL;
    uint32_t STATUS;
    uint32_t DATA_LENGTH;
    uint32_t SLAVE_ADDRESS;
    uint32_t FIFO;
    uint32_t DIV;
    uint32_t DELAY;
    uint32_t CLOCK_STRETCH;
} raspberrypi_i2c;

// Control Register Bits
#define C_I2CEN BSP_BIT32(15)
#define C_INTR BSP_BIT32(10)
#define C_INTT BSP_BIT32(9)
#define C_INTD BSP_BIT32(8)
#define C_ST BSP_BIT32(7)
#define C_CLEAR BSP_BIT32(5)
#define C_READ BSP_BIT32(0)

// Status Register Bits
#define S_CLKT BSP_BIT32(9)
#define S_ERR BSP_BIT32(8)
#define S_RXF BSP_BIT32(7)
#define S_TXE BSP_BIT32(6)
#define S_RXD BSP_BIT32(5)
#define S_TXD BSP_BIT32(4)
#define S_RXR BSP_BIT32(3)
#define S_TXW BSP_BIT32(2)
#define S_DONE BSP_BIT32(1)
#define S_TA BSP_BIT32(0)

typedef enum
{
    raspberrypi_i2c0,
    raspberrypi_i2c1,
    raspberrypi_i2c2
} raspberrypi_i2c_device;

rtems_status_code i2c_init(raspberrypi_i2c_context ctx);

rtems_status_code i2c_recv(raspberrypi_i2c_context *ctx, u8 address, u8 *buffer, u32 size);

rtems_status_code i2c_send(raspberrypi_i2c_context *ctx, u8 address, u8 *buffer, u32 size);

#endif /* LIBBSP_AARCH64_RASPBERRYPI_4_SPI_H */
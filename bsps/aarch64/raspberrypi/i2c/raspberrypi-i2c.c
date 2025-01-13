/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSBSPsAArch64Raspberrypi4
 *
 * @brief I2C Driver
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

#include <bsp/irq.h>
#include <bsp/raspberrypi.h>
#include <bsp/raspberrypi-i2c.h>
#include <bsp/rpi-gpio.h>
#include <bspopts.h>

#define I2C_SPEED 100000

volatile raspberrypi_i2c *regs;

typedef struct
{
    volatile raspberrypi_i2c *regs;
    raspberrypi_i2c_device device;
} raspberrypi_i2c_context;

static rtems_status_code
raspberrypi_i2c_gpio_init(raspberrypi_i2c_device device)
{

    switch (device)
    {
    case raspberrypi_i2c0:
        raspberrypi_gpio_set_function(0, GPIO_AF0);
        raspberrypi_gpio_set_function(1, GPIO_AF0);
        break;
    case raspberrypi_i2c1:
        raspberrypi_gpio_set_function(2, GPIO_AF0);
        raspberrypi_gpio_set_function(3, GPIO_AF0);
        break;
    case raspberrypi_i2c2:
        raspberrypi_i2c_gpio_set_function(28, GPIO_AF0);
        raspberrypi_gpio_set_pull(28, GPIO_PULL_NONE);
        raspberrypi_gpio_set_function(29, GPIO_AF0);
        raspberrypi_gpio_set_pull(29, GPIO_PULL_NONE);
        break;
    default:
        return RTEMS_INVALID_NUMBER;
    }
    return RTEMS_SUCCESSFUL;
}

rtems_status_code i2c_init(raspberrypi_i2c_context *ctx)
{
    switch (ctx->device)
    {
    case raspberrypi_i2c0:
        ctx->regs = (volatile raspberrypi_i2c *)BCM2711_I2C0_BASE;
        break;
    case raspberrypi_i2c1:
        ctx->regs = (volatile raspberrypi_i2c *)BCM2711_I2C1_BASE;
        break;
    case raspberrypi_i2c2:
        ctx->regs = (volatile raspberrypi_i2c *)BCM2711_I2C2_BASE;
        break;
    default:
        return RTEMS_INVALID_NUMBER;
        break;
    }

    // Initialise GPIO pins according to which bsc master is selected
    int *eno = raspberrypi_i2c_gpio_init(device);
    if (eno != 0)
    {
        return RTEMS_INVALID_NUMBER;
    }

    ctx->regs->DIV = GPU_CORE_CLOCK_RATE / I2C_SPEED;

    return RTEMS_SUCCESSFUL;
}

rtems_status_code i2c_recv(raspberrypi_i2c_context *ctx, u8 address, u8 *buffer, u32 size)
{
    if (ctx == NULL || buffer == NULL)
    {
        return RTEMS_INVALID_ADDRESS;
    }

    int count = 0;
    ctx->regs->SLAVE_ADDRESS = address;
    ctx->regs->CONTROL = C_CLEAR;
    ctx->regs->STATUS = S_CLKT | S_ERR | S_DONE;
    ctx->regs->DATA_LENGTH = size;
    ctx->regs->CONTROL = C_I2CEN | C_READ | C_ST;

    while (!(ctx->regs->STATUS & S_DONE))
    {
        while (ctx->regs->STATUS & S_RXD)
        {
            *buffer++ = ctx->regs->FIFO & 0xFF;
            count++;
        }
    }

    while (count < size && ctx->regs->STATUS & S_RXD)
    {
        *buffer++ = ctx->regs->FIFO & 0xFF;
        count++;
    }

    if (ctx->regs->STATUS & S_ERR)
    {
        return RTEMS_IO_ERROR;
    }
    else if (ctx->regs->STATUS & S_CLKT)
    {
        return RTEMS_TIMEOUT;
    }
    else if (count < size)
    {
        return RTEMS_UNSATISFIED;
    }
    ctx->regs->STATUS = S_DONE;

    return RTEMS_SUCCESSFUL;
}

rtems_status_code i2c_send(raspberrypi_i2c_context *ctx, u8 address, u8 *buffer, u32 size)
{
    int count = 0;
    ctx->regs->SLAVE_ADDRESS = address;
    ctx->regs->CONTROL = C_CLEAR;
    ctx->regs->STATUS = S_CLKT | S_ERR | S_DONE;
    ctx->regs->DATA_LENGTH = size;
    ctx->regs->CONTROL = C_I2CEN | C_ST;

    while (!(ctx->regs->STATUS & S_DONE))
    {
        while (count < size && ctx->regs->STATUS & S_TXD)
        {
            ctx->regs->FIFO = *buffer++;
            count++;
        }
    }

    if (ctx->regs->STATUS & S_ERR)
    {
        return RTEMS_IO_ERROR;
    }
    else if (ctx->regs->STATUS & S_CLKT)
    {
        return RTEMS_TIMEOUT;
    }
    else if (count < size)
    {
        return RTEMS_UNSATISFIED;
    }
    ctx->regs->STATUS = S_DONE;

    return RTEMS_SUCCESSFUL;
}

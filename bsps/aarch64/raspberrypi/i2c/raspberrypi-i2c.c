
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

#include <bsp/raspberrypi-i2c.h>
#include <bsp/raspberrypi.h>
#include <bsp/rpi-gpio.h>
#include <dev/i2c/i2c.h>
#include <bsp/irq.h>

typedef struct
{
    i2c_bus base;
    uint32_t input_clock;
    rtems_id task_id;
    uint32_t base_address;
    raspberrypi_bsc_masters device;
    uint32_t remaining_bytes;
    uint32_t remaining_transfers;
    uint8_t *current_buffer;
    uint32_t current_buffer_size;
    bool read_transfer;
} raspberrypi_i2c_bus;

#define I2C_POLLING(condition) \
    while (condition)          \
    {                          \
        ;                      \
    }

static int rpi_i2c_bus_transfer(raspberrypi_i2c_bus *bus)
{
    while (bus->remaining_bytes >= 1)
    {
        if (bus->read_transfer)
        {
            I2C_POLLING((BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) & (1 << 5)) == 0);
            *(bus->current_buffer) = BCM2835_REG(bus->base_address + BCM2711_I2C_FIFO) & 0xFF;
        }
        else
        {
            I2C_POLLING((BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) & (1 << 2)) == 0);
            BCM2835_REG(bus->base_address + BCM2711_I2C_FIFO) = *(bus->current_buffer);
        }

        ++bus->current_buffer;
        if (
            (BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) & (1 << 8)) ||
            (BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) & (1 << 9)))
        {
            return -EIO;
        }
        --bus->remaining_bytes;
        --bus->current_buffer_size;
    }

    return 0;
}

static int rpi_i2c_setup_transfer(raspberrypi_i2c_bus *bus)
{
    int rv;
    while (bus->remaining_transfers > 0)
    {
        bus->remaining_bytes = bus->remaining_transfers > 1 ? 0xFFFF : (bus->current_buffer_size & 0xFFFF);
        BCM2835_REG(bus->base_address + BCM2711_I2C_DLEN) = bus->remaining_bytes;
        BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) |= (3 << 8);
        BCM2835_REG(bus->base_address + BCM2711_I2C_CONTROL) |= (1 << 7);
        if ((BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) & (1 << 8)) != 0)
        {
            return -EIO;
        }

        rv = rpi_i2c_bus_transfer(bus);

        if (rv < 0)
        {
            return rv;
        }

        I2C_POLLING((BCM2835_REG(bus->base_address + BCM2711_I2C_STATUS) & (1 << 1)) == 0);
        --bus->remaining_transfers;
    }
    return 0;
}

static int rpi_i2c_transfer(i2c_bus *base, i2c_msg *msgs, uint32_t msg_count)
{
    raspberrypi_i2c_bus *bus = (raspberrypi_i2c_bus *)base;
    int rv = 0;
    uint32_t i;

    for (i = 0; i < msg_count; ++i)
    {
        if (msgs[i].flags & I2C_M_RECV_LEN)
        {
            return RTEMS_INVALID_NUMBER;
        }
    }

    for (i = 0; i < msg_count; i++)
    {
        bus->current_buffer = msgs[i].buf;
        bus->current_buffer_size = msgs[i].len;
        bus->remaining_transfers = (bus->current_buffer_size + 0xFFFFE) / 0xFFFFF;

        if (msgs[i].flags & I2C_M_TEN) // 10-bit slave address
        {
            /* Add the 8 lsbs of the 10-bit slave address to the fifo register*/
            BCM2835_REG(bus->base_address + BCM2711_I2C_FIFO) = msgs[i].addr & 0xFF;
            uint8_t msbs = msgs[i].addr >> 8;
            BCM2835_REG(bus->base_address + BCM2711_I2C_SLAVE_ADDRESS) = (0x1E << 2) | msbs;
        }
        BCM2835_REG(bus->base_address + BCM2711_I2C_SLAVE_ADDRESS) = msgs[i].addr;

        if (msgs[i].flags & I2C_M_RD)
        {
            BCM2835_REG(bus->base_address + BCM2711_I2C_CONTROL) |= (1 << 0);
            bus->read_transfer = true;
        }
        else
        {
            BCM2835_REG(bus->base_address + BCM2711_I2C_CONTROL) &= ~(1 << 0);
            bus->read_transfer = false;
        }

        rv = rpi_i2c_setup_transfer(bus);
        if (rv < 0)
        {
            return rv;
        }
    }

    return rv;
}

static rtems_status_code i2c_gpio_init(raspberrypi_bsc_masters device, raspberrypi_i2c_bus *bus)
{
    switch (device)
    {
    case raspberrypi_bscm0:
        raspberrypi_gpio_set_function(0, GPIO_AF0);
        raspberrypi_gpio_set_function(1, GPIO_AF0);
        bus->base_address = 0x00205000;
        break;
    case raspberrypi_bscm1:
        raspberrypi_gpio_set_function(2, GPIO_AF0);
        raspberrypi_gpio_set_function(3, GPIO_AF0);
        bus->base_address = 0x00804000;
        break;
    case raspberrypi_bscm2:
        raspberrypi_gpio_set_function(28, GPIO_AF0);
        raspberrypi_gpio_set_pull(28, GPIO_PULL_NONE);
        raspberrypi_gpio_set_function(29, GPIO_AF0);
        raspberrypi_gpio_set_pull(29, GPIO_PULL_NONE);
        bus->base_address = 0x00805000;
        break;
    default:
        return RTEMS_INVALID_ADDRESS;
    }
    return RTEMS_SUCCESSFUL;
}

rtems_status_code rpi_i2c_init(raspberrypi_bsc_masters device, uint32_t bus_clock)
{
    raspberrypi_i2c_bus *bus;
    rtems_status_code sc;
    const char *bus_path;

    bus = (raspberrypi_i2c_bus *)i2c_bus_alloc_and_init(sizeof(*bus));
    if (bus == NULL)
    {
        return RTEMS_NO_MEMORY;
    }

    /* Enable I2C */
    BCM2835_REG(bus->base_address + BCM2711_I2C_CONTROL) |= (1 << 15);

    sc = rpi_i2c_set_clock(&bus->base, bus_clock);
    if (sc != RTEMS_SUCCESSFUL)
    {
        i2c_bus_destroy_and_free(&bus->base);
        return sc;
    }

    bus->base.transfer = rpi_i2c_transfer;
    bus->base.set_clock = rpi_i2c_set_clock;
    bus->base.destroy = rpi_i2c_destroy;
    bus->base.functionality = I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR;

    switch (device)
    {
    case raspberrypi_bscm0:
        bus_path = "/dev/i2c1";
        break;
    case raspberrypi_bscm1:
        bus_path = "/dev/i2c2";
        break;
    case raspberrypi_bscm2:
        bus_path = "/dev/i2c3";
        break;
    default:
        i2c_bus_destroy_and_free(&bus->base);
        return RTEMS_INVALID_NUMBER;
    }

    sc = i2c_gpio_init(device, bus);
    if (sc != RTEMS_SUCCESSFUL)
    {
        i2c_bus_destroy_and_free(&bus->base);
        return sc;
    }

    return i2c_bus_register(&bus->base, bus_path);
}
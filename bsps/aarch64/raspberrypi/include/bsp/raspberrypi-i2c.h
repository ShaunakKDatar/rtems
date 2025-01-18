
#ifndef LIBBSP_AARCH64_RASPBERRYPI_I2C_H
#define LIBBSP_AARCH64_RASPBERRYPI_I2C_H

#include <bsp/utility.h>
#include <dev/i2c/i2c.h>
#include <bsp/rpi-gpio.h>
#include <bsp/raspberrypi.h>

typedef enum
{
    raspberrypi_bscm0,
    raspberrypi_bscm1,
    raspberrypi_bscm2
} raspberrypi_bsc_masters;

#define BCM2711_I2C_CONTROL 0x0
#define BCM2711_I2C_STATUS 0x4
#define BCM2711_I2C_DLEN 0x8
#define BCM2711_I2C_SLAVE_ADDRESS 0xc
#define BCM2711_I2C_FIFO 0x10
#define BCM2711_DIV 0x14
#define BCM2711_DELAY 0x18

#define BSC_MASTER0 (RPI_PERIPHERAL_BASE + 0x00205000)
#define BSC_MASTER1 (RPI_PERIPHERAL_BASE + 0x00804000)
#define BSC_MASTER2 (RPI_PERIPHERAL_BASE + 0x00805000)

rtems_status_code rpi_i2c_init(raspberrypi_bsc_masters master);

#endif
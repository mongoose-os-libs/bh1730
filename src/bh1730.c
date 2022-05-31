// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bh1730.h"

#include "mgos.h"
#include "mgos_i2c.h"

/* Mongoose OS C driver (mJS bindable) for Rohm BH1730 light sensor */

struct bh1730_t {
    int addr;
    int itime;
    int gain;
};

#define BH1730_CMD_MAGIC 0x80
#define BH1730_CMD_SETADDR 0x0
#define BH1730_CMD_SPECCMD 0x60

#define BH1730_SPECCMD_INTRESET 0x1
#define BH1730_SPECCMD_STOPMEAS 0x2
#define BH1730_SPECCMD_STARTMEAS 0x3
#define BH1730_SPECCMD_RESET 0x4

#define BH1730_ADDR_CONTROL 0x00
#define BH1730_ADDR_TIMING 0x01
#define BH1730_ADDR_INTERRUPT 0x02
#define BH1730_ADDR_THLLOW 0x03
#define BH1730_ADDR_THLHIGH 0x04
#define BH1730_ADDR_THHLOW 0x05
#define BH1730_ADDR_THHIGH 0x06
#define BH1730_ADDR_GAIN 0x07
#define BH1730_ADDR_ID 0x12
#define BH1730_ADDR_DATA0LOW 0x14
#define BH1730_ADDR_DATA0HIGH 0x15
#define BH1730_ADDR_DATA1LOW 0x16
#define BH1730_ADDR_DATA1HIGH 0x17


#define BH1730_CTL_ADC_INTR 0x20
#define BH1730_CTL_ADC_VALID 0x10
#define BH1730_CTL_ONE_TIME 0x08
#define BH1730_CTL_DATA_SEL 0x04
#define BH1730_CTL_ADC_EN 0x02
#define BH1730_CTL_POWER 0x01


#define ACK_CHECK_EN 1
#define ACK_VAL    0x0
#define NACK_VAL   0x1

static bool send_cmd(bh1730_t *d, int cmdbyte, int val)
{
    const uint8_t seq[] = { (uint8_t)cmdbyte, (uint8_t)val };
    size_t seq_len = (val == -1) ? 1 : 2;
    struct mgos_i2c *i2c = mgos_i2c_get_global();
    return mgos_i2c_write(i2c, d->addr, seq, seq_len, true);
}

static bool read_reg(bh1730_t *d, int reg, int *retval)
{
    if (!send_cmd(d, reg | BH1730_CMD_MAGIC | BH1730_CMD_SETADDR, -1)) {
        return false;
    }
    struct mgos_i2c *i2c = mgos_i2c_get_global();
    *retval = 0; // we're only reading into the LSB
    return mgos_i2c_read(i2c, d->addr, retval, 1, true);
}

#define T_INT_US 2.8 //uS, typical, from datasheet

float bh1730_read_lux(bh1730_t *d)
{
    int r = send_cmd(d, BH1730_CMD_MAGIC | BH1730_CMD_SETADDR | BH1730_ADDR_CONTROL, BH1730_CTL_POWER | BH1730_CTL_ADC_EN | BH1730_CTL_ONE_TIME);
    if (!r) {
        goto err;
    }

    int d0h, d0l, d1h, d1l, s;
    int tmo = 100;
    do {
        r = read_reg(d, BH1730_ADDR_CONTROL, &s);
        if (!r) {
            goto err;
        }
        mgos_msleep(20);
        if ((tmo--) == 0) {
            goto err;
        }
    } while ((s & BH1730_CTL_ADC_VALID) == 0);

    r = read_reg(d, BH1730_ADDR_DATA0LOW, &d0l);
    r &= read_reg(d, BH1730_ADDR_DATA0HIGH, &d0h);
    r &= read_reg(d, BH1730_ADDR_DATA1LOW, &d1l);
    r &= read_reg(d, BH1730_ADDR_DATA1HIGH, &d1h);
    if (!r) {
        goto err;
    }

    float data0 = (d0h << 8) + d0l;
    float data1 = (d1h << 8) + d1l;
    float itime_ms = (T_INT_US * 964 * (256 - d->itime)) / 1000.0;

    float lux = 0;
    if (data0 != 0) {
        if (data1 / data0 < 0.26) {
            lux = (1.290 * data0 - 2.733 * data1) / d->gain * 102.6 / itime_ms;
        } else if (data1 / data0 < 0.55) {
            lux = (0.795 * data0 - 0.859 * data1) / d->gain * 102.6 / itime_ms;
        } else if (data1 / data0 < 1.09) {
            lux = (0.510 * data0 - 0.345 * data1) / d->gain * 102.6 / itime_ms;
        } else if (data1 / data0 < 2.13) {
            lux = (0.276 * data0 - 0.130 * data1) / d->gain * 102.6 / itime_ms;
        }
    }
    LOG(LL_DEBUG, ("Lux reading (C-side) %.2f", lux));
    return lux;
err:
    LOG(LL_ERROR, ("I2C communications error"));
    return -1;
}


bh1730_t *bh1730_init(int addr)
{
    bh1730_t *ret = malloc(sizeof(bh1730_t));
    ret->addr = addr;
    ret->itime = 0xDA; //reset val
    ret->gain = 1; //reset val

    int id;
    int r = read_reg(ret, BH1730_ADDR_ID, &id);
    if (!r) {
        LOG(LL_ERROR, ("Read ID reg error for addr 0x%X", addr));
        goto err;
    }
    if ((id & 0xF0) != 0x70) {
        LOG(LL_ERROR, ("Read ID: didn't receive 0x7x but 0x%X", id));
        goto err;
    }

    //Reset chip
    r = send_cmd(ret, BH1730_CMD_MAGIC | BH1730_CMD_SPECCMD | BH1730_SPECCMD_RESET, -1);
    if (!r) {
        LOG(LL_ERROR, ("Chip reset reg error for addr 0x%X", addr));
        goto err;
    }

    LOG(LL_INFO, ("Device at 0x%X initialized, ID=%X.", addr, id));

    return ret;
err:
    free(ret);
    return NULL;
}


void bh1730_free(bh1730_t *d) {
  free(d);
}

bool mgos_bh1730_init(void) {
  return true;
}


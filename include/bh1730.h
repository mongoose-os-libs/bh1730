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

/* Mongoose OS C driver (mJS bindable) for Rohm BH1730 light sensor */

#pragma once

typedef struct bh1730_t bh1730_t;

/*
 * Create and return a BH1730 instance; `addr` is an I2C address of the BH1730
 * sensor.
 */
bh1730_t *bh1730_init(int addr);

/*
 * Return the ambient light level in lux, or -1 in case of a failure.
 */
float bh1730_read_lux(bh1730_t *d);

/*
 * Free memory occupied by the BH1730 instance.
 */
void bh1730_free(bh1730_t *d);

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

let BH1730 = {

	_create: ffi('void *bh1730_init(int)'),
	_close: ffi('void bh1730_free(void *)'),
	_read_lux: ffi('float bh1730_read_lux(void *)'),

  // ## **`BH1730.create(addr)`**
  // Create a BH1730 instance: an object with the methods described below.
  // `addr` is an i2c address of the BH1730 sensor.
  create: function(addr) {
    let obj = Object.create(BH1730._proto);
    obj.bh1730 = BH1730._create(addr);
    return obj.bh1730 ? obj : null;
  },

	_proto : {
    // ## **`myBH.read_lux()`**
    // Return the ambient light level in lux, or -1 in case of a failure.
		read_lux: function() {
			return BH1730._read_lux(this.bh1730);
		}
	},
};


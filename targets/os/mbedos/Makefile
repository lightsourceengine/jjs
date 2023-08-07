# Copyright JS Foundation and other contributors, http://js.foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# default board: FRDM-K64F
BOARD ?= K64F

# defult toolchain
TOOLCHAIN ?= GCC_ARM

# path to jjs/targets/os/mbedos5
JJS_TARGET_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# path to the root JJS directory
JJS_ROOT_DIR = $(realpath $(JJS_TARGET_DIR)/../../..)

# default build path
BUILD_DIR ?= $(JJS_ROOT_DIR)/build/mbed-os

# default JJS heap size (in KB)
JJS_HEAP_SIZE ?= 70

define MBED_CLI_FLAGS
	--clean
	--build $(BUILD_DIR)
	--source $(MBED_OS_DIR)
	--source $(JJS_ROOT_DIR)
	--source $(JJS_TARGET_DIR)
	--toolchain $(TOOLCHAIN)
	--target $(BOARD)
	--artifact-name mbedos
	--macro JJS_GLOBAL_HEAP_SIZE=$(JJS_HEAP_SIZE)
endef

.PHONY: all
all: .build

.PHONY: clean
clean: .mbedignore-remove
	rm -rf $(JJS_ROOT_DIR)/build/mbed-os

.PHONY: flash
flash: .mbed-set-flash-flag .build

.PHONY: .mbed-set-flash-flag
.mbed-set-flash-flag:
	$(eval MBED_CLI_FLAGS += --flash)

.PHONY: .build
.build: .mbedignore-copy .mbed-build .mbedignore-remove

.PHONY: .mbed-build
.mbed-build:
	mbed config -G MBED_OS_DIR $(MBED_OS_DIR)
	mbed compile $(strip $(MBED_CLI_FLAGS))

.PHONY: .mbedignore-copy
.mbedignore-copy:
	cp mbedignore.txt $(JJS_ROOT_DIR)/.mbedignore

.PHONY: .mbedignore-remove
.mbedignore-remove:
	rm -f $(JJS_ROOT_DIR)/.mbedignore

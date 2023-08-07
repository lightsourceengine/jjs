# Copyright JS Foundation and other contributors, http://js.foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# application name
APPLICATION = riot_jjsscript

# default BOARD enviroment
BOARD ?= stm32f4discovery

# path to jjs/targets/os/riot
TARGET_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# path to the JJS directory
JJSDIR ?= $(TARGET_DIR)/../../..

# path to the RIOT base directory
RIOTBASE ?= $(JJSDIR)/../RIOT

# path to the application directory
APPDIR ?= $(JJSDIR)/targets/os/riot/source

# path to the binary directory
BUILDDIR ?= $(JJSDIR)/build/riot-stm32f4

# path to the binary directory
BINDIR ?= $(BUILDDIR)/bin

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

INCLUDES += -I$(JJSDIR)/jjs-core/include
INCLUDES += -I$(JJSDIR)/jjs-ext/include

ARCHIVES += $(JJSDIR)/build/lib/libjjs-core.a
ARCHIVES += $(JJSDIR)/build/lib/libjjs-ext.a
ARCHIVES += $(JJSDIR)/build/lib/libjjs-port.a

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += posix_sleep

.PHONY: libjjs
libjjs:
	$(JJSDIR)/tools/build.py \
		--clean \
		--lto=OFF \
		--jjs-cmdline=OFF \
		--jjs-math=ON \
		--amalgam=ON \
		--mem-heap=70 \
		--profile=es.next \
		--compile-flag=-fshort-enums \
		--toolchain=$(abspath $(JJSDIR)/cmake/toolchain_mcu_stm32f4.cmake)

.PHONY: clear-cflags
clear-cflags:
	$(eval BACKUP := $(CFLAGS))
	$(eval CFLAGS := )

.PHONY: restore-cflags
restore-cflags:
	$(eval CFLAGS := $(BACKUP))

# CFLAGS is filled by Makefile.include that is only for RIOT.
# Clear CFLAGS temporarily while compiling JJS.
all: clear-cflags libjjs restore-cflags

include $(RIOTBASE)/Makefile.include

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

include $(APPDIR)/Make.defs

# JJS built-in application information.
PROGNAME  = jjs
PRIORITY  = $(CONFIG_INTERPRETERS_JJS_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_JJS_STACKSIZE)

# Path to the JJS and NuttX projects. If not specified, it is
# supposed that JJS is located next to the nuttx folder.
JJS_ROOT_DIR ?= $(abspath ../../../../jjs)
NUTTX_INCLUDE_DIR ?= $(abspath $(JJS_ROOT_DIR)/../nuttx/include)

CFLAGS += -std=c99
CFLAGS += -I$(JJS_ROOT_DIR)/jjs-core/include
CFLAGS += -I$(JJS_ROOT_DIR)/jjs-ext/include
CFLAGS += -I$(JJS_ROOT_DIR)/jjs-math/include

# These libs should be copied from the JJS project.
LIBS = libjjs-core.a libjjs-ext.a libjjs-math.a libjjs-port.a

ASRCS = setjmp.S
CSRCS = jjs-port.c
MAINSRC = jjs-main.c

.PHONY: copylibs
copylibs:
	cp $(JJS_ROOT_DIR)/build/lib/lib*.a .

$(LIBS): copylibs
	$(firstword $(AR)) x $@

.PHONY: updateobjs
updateobjs:
	$(eval OBJS += $(shell find . -name "*.obj"))

.PHONY: cleanlibs
cleanlibs: updateobjs
	rm -f $(OBJS)

.PHONY: libjjs
libjjs:
	$(JJS_ROOT_DIR)/tools/build.py \
		--clean \
		--lto=OFF \
		--jjs-cmdline=OFF \
		--jjs-math=ON \
		--amalgam=ON \
		--mem-heap=70 \
		--profile=es.next \
		--compile-flag="-I${NUTTX_INCLUDE_DIR}" \
		--toolchain=$(abspath $(JJS_ROOT_DIR)/cmake/toolchain_mcu_stm32f4.cmake)

clean:: cleanlibs

archive: libjjs $(LIBS) updateobjs

include $(APPDIR)/Application.mk

/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JJSX_DEBUGGER_H
#define JJSX_DEBUGGER_H

#include "jjs-debugger-transport.h"
#include "jjs.h"

JJS_C_API_BEGIN

void jjsx_debugger_after_connect (jjs_context_t *context_p, bool success);

/*
 * Message transmission interfaces.
 */
bool jjsx_debugger_tcp_create (jjs_context_t *context_p, uint16_t port);
bool jjsx_debugger_serial_create (jjs_context_t *context_p, const char *config);

/*
 * Message encoding interfaces.
 */
bool jjsx_debugger_ws_create (jjs_context_t *context_p);
bool jjsx_debugger_rp_create (jjs_context_t *context_p);

bool jjsx_debugger_is_reset (jjs_context_t *context_p, jjs_value_t value);

JJS_C_API_END

#endif /* !JJSX_HANDLER_H */

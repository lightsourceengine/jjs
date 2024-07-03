/* Copyright Light Source Software, LLC and other contributors.
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

#include "jjs-types.h"

/*
 * Platform API Stubs
 *
 * Setting a JJS_PLATFORM_API_* flag to 2 will compile JJS with an api implementation that
 * returns an error. The use case is an application that does not load modules from paths
 * can remove cwd and realpath platform implementations from the build. You can do this
 * with JJS_PLATFORM_API_* == 0, but you will need to implement the api stub yourself to
 * compile.
 *
 * This file is formatted for easy copy and paste of api implementation functions. In user
 * land, the bare minimum include is #include <jjs.h>.
 */

#if JJS_PLATFORM_API_PATH_CWD == 2

jjs_status_t
jjsp_path_cwd_impl (const jjs_allocator_t* allocator, jjs_platform_buffer_view_t* buffer_view_p)
{
  (void) allocator, (void) buffer_view_p;
  return JJS_STATUS_NOT_IMPLEMENTED;
}
#endif /* JJS_PLATFORM_API_PATH_CWD */

#if JJS_PLATFORM_API_TIME_SLEEP == 2

jjs_status_t
jjsp_time_sleep_impl (uint32_t sleep_time_ms)
{
  (void) sleep_time_ms;
  return JJS_STATUS_NOT_IMPLEMENTED;
}

#endif /* JJS_PLATFORM_API_TIME_SLEEP */

#if JJS_PLATFORM_API_TIME_LOCAL_TZA == 2

jjs_status_t
jjsp_time_local_tza_impl (double unix_ms, int32_t* out_p)
{
  (void) ms, (void) out_p;
  return JJS_STATUS_NOT_IMPLEMENTED;
}

#endif /* JJS_PLATFORM_API_TIME_LOCAL_TZA */

#if JJS_PLATFORM_API_TIME_NOW_MS == 2

jjs_status_t
jjsp_time_now_ms_impl (double* out_p)
{
  (void) out_p;
  return JJS_STATUS_NOT_IMPLEMENTED;
}

#endif /* JJS_PLATFORM_API_TIME_NOW_MS */

#if JJS_PLATFORM_API_PATH_REALPATH == 2

jjs_status_t
jjsp_path_realpath_impl (const jjs_allocator_t* allocator,
                         jjs_platform_path_t* path_p,
                         jjs_platform_buffer_view_t* buffer_view_p)
{
  (void) allocator, (void) path_p, (void) buffer_view_p;
  return JJS_STATUS_NOT_IMPLEMENTED;
}

#endif /* JJS_PLATFORM_API_PATH_REALPATH */

#if JJS_PLATFORM_API_FS_READ_FILE == 2

jjs_status_t
jjsp_fs_read_file_impl (const jjs_allocator_t* allocator, jjs_platform_path_t* path_p, jjs_platform_buffer_t* out_p)
{
  (void) allocator, (void) path_p, (void) out_p;
  return JJS_STATUS_NOT_IMPLEMENTED;
}

#endif /* JJS_PLATFORM_API_FS_READ_FILE */

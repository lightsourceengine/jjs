#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

/*
 * Module Notes
 *
 * The default JJS module implementation is not suitable for.. anything.
 *
 * For the repl and test262 runner, we need to be able to load modules,
 * both static and dynamic, from files (absolute and relative specifiers).
 * The default JJS module implementation is just not suitable, nor
 * configurable, for these basic tasks. Implementing the ECMA module spec
 * is a decent amount of effort and a low priority for this project.
 * However, we still need to load modules.
 *
 * This implementation is a patch. It's not to spec. It's synchronous. It
 * does not handle cycles, etc. However, it works well for primary use cases
 * and happy paths of importing modules from file specifiers.
 *
 * Supported Use Cases:
 *
 * import('./foo.js');
 * import('../foo.js');
 * import('/abspath/foo.js');
 *
 * import * from './foo.js';
 * import * from '../foo.js';
 * import * from '/abspath/foo.js';
 *
 * Notes:
 *
 * - import() works from the repl, non-module files, and module files.
 * - import works from module files
 * - nesting works, modules can import other modules
 * - spec calls for evaluation to be dfs order. this implementation has
 *   a consistent evaluation order, but it is not to spec!
 * - module cycles, as defined by the spec, are not handled correctly
 * - no import.meta.url
 * - no top level await
 * - import and import() function asynchronously to spec, but the underlying
 *   implementation is synchronous.
 */

#include "jjs.h"

void main_module_init(void);
void main_module_cleanup(void);

jjs_value_t main_module_run_esm (const char* path_p);
jjs_value_t main_module_run (const char* path_p);

#endif /* !MAIN_MODULE_H */

// Zig + JJS Example
//
// Example code of JJS running in a Zig program. It takes a simple javascript expression,
// parses it and evaluates it.
//
// Setup/Run
//
// 1) From the source root directory, build with:
//    - tools/build.py --amalgam ON
// 2) Check build/amalgam/jjs-config.h for desired configuration.
//    - jjs-config.h can be configured through tools/build.py by adding flags (ex: --error-messages ON)
//    - use compile definitions (-D flags) in build.zig addCSource().
// 3) From this directory, run: zig build run

const std = @import("std");

// Import JJS types and functions from C header file (resolved by build.zig include path)
const jjs = @cImport(@cInclude("jjs.h"));

pub fn main() anyerror!void {
    // simple script to evaluate
    const script = "5 * 5;";

    // initialize JJS with the default heap size (usually configure to 512KB). Use jjs_init_ex()
    // for more control of memory configuration.
    jjs.jjs_init(0);
    defer jjs.jjs_cleanup();

    // parse the script
    const compiled = jjs.jjs_parse(script, script.len, null);
    defer jjs.jjs_value_free(compiled);

    if (jjs.jjs_value_is_exception(compiled)) {
        std.log.err("parse error", .{});
        std.process.exit(1);
    }

    // run the compiled byte code
    const result = jjs.jjs_run(compiled);
    defer jjs.jjs_value_free(result);

    if (jjs.jjs_value_is_exception(result)) {
        std.log.err("eval error", .{});
        std.process.exit(1);
    }

    // the result is the last thing on the stack, the result of the multiplication.
    std.log.info("script result: {}", .{jjs.jjs_value_as_int32(result)});
}

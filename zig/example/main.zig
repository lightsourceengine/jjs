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
    var ctx: ?*jjs.jjs_context_t = null;
    const status = jjs.jjs_context_new(null, &ctx);
    defer jjs.jjs_context_free(ctx);

    if (status != jjs.JJS_STATUS_OK) {
      std.log.err("failed to init context", .{});
      std.process.exit(1);
    }

    // parse the script
    const compiled = jjs.jjs_parse(ctx, script, script.len, null);
    defer jjs.jjs_value_free(ctx, compiled);

    if (jjs.jjs_value_is_exception(ctx, compiled)) {
        std.log.err("parse error", .{});
        std.process.exit(1);
    }

    // run the compiled byte code
    const result = jjs.jjs_run(ctx, compiled);
    defer jjs.jjs_value_free(ctx, result);

    if (jjs.jjs_value_is_exception(ctx, result)) {
        std.log.err("eval error", .{});
        std.process.exit(1);
    }

    // the result is the last thing on the stack, the result of the multiplication.
    std.log.info("script result: {}", .{jjs.jjs_value_as_int32(ctx, result)});
}

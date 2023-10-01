const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "example",
        .root_source_file = .{ .path = "main.zig" },
        .target = target,
        .optimize = optimize,
    });

    // include path used by cImport to find jjs.h
    // - jjs.h includes "jjs-config.h", which contains JJS compile time settings
    exe.addIncludePath(.{ .path = "../../build/amalgam" });

    // add jjs.c and jjs-port.c
    // - jjs-port.c is a generic port implemented in C. In the future, the port methods
    //   will be implemented in zig.
    // - Depending on how jjs-config.h is setup, you can also pass compile time settings via
    //   compile definitions: -DJJS_HEAP_SIZE=8192
    exe.addCSourceFiles(&.{ "../../build/amalgam/jjs.c", "../../build/amalgam/jjs-port.c" }, &.{"-std=gnu99"});

    exe.linkLibC();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}

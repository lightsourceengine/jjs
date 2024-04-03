const std = @import("std");

pub fn build(b: *std.Build) !void {
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

    var jjs_flags = std.ArrayList([]const u8).init(b.allocator);
    defer jjs_flags.deinit();

    try jjs_flags.append("-std=c99");

    if (target.isLinux() or (target.isWindows() and target.isGnuLibC())) {
      try jjs_flags.append("-D_BSD_SOURCE");
      try jjs_flags.append("-D_DEFAULT_SOURCE");
    }

    if (optimize != .Debug) {
      try jjs_flags.append("-DJJS_NDEBUG");
    }

    // add jjs.c
    exe.addCSourceFiles(&.{ "../../build/amalgam/jjs.c" }, jjs_flags.items);

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

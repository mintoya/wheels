const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    const exe = b.addExecutable(.{
        .name = "wheels",
        .root_module = b.createModule(.{
            .target = target,
            .link_libcpp = true,
            .optimize = b.standardOptimizeOption(.{
                .preferred_optimize_mode = .Debug,
            }),
            .strip = false,
        }),
        .use_llvm = false,
    });

    const cfile = b.option(
        []const u8,
        "file",
        "which file to run",
    ) orelse "tests.c";

    exe.root_module.addCSourceFile(.{
        .file = b.path(cfile),
        .flags = &.{
            "-g",
            "-w",
            "-fno-sanitize=vla-bound",
        },
        .language = .cpp,
    });

    if (target.result.os.tag == .windows)
        exe.root_module.linkSystemLibrary("dbghelp", .{});
    exe.rdynamic = true;

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args|
        run_cmd.addArgs(args);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);
}

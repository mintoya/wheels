const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "wheels",
        .root_module = b.createModule(.{
            .target = target,
            // .valgrind = true,
            .link_libc = true,
            .optimize = optimize,
        }),
    });

    exe.root_module.addCSourceFile(.{
        .file = b.path("examples/main.c"),
        .flags = &.{
            "-w",
            "-std=c23",
        },
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);
}

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    const exe = b.addExecutable(.{
        .name = "wheels",
        .root_module = b.createModule(.{
            .target = target,
            .omit_frame_pointer = false, // Enable this explicitly
            .link_libc = true,
            .optimize = b.standardOptimizeOption(.{
                .preferred_optimize_mode = .Debug,
            }),
            .strip = false, // Ensure symbols aren't stripped
        }),
    });

    exe.root_module.addCSourceFile(.{
        .file = b.path("examples/main.c"),
        .flags = &.{
            "-g",
            "-w",
            "-std=c23",
        },
        .language = .c,
    });

    exe.linkage = .dynamic;
    exe.rdynamic = true;

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args|
        run_cmd.addArgs(args);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);
}

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "a",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    const sourcefile = "examples/main.c";

    exe.linkLibC();

    if (target.result.os.tag != .windows) {
        exe.addCSourceFile(.{
            .file = b.path(sourcefile),
            .flags = &.{
                "-w",
                "-g",
                "-fsanitize=address",
                "-D_GNU_SOURCE",
            },
        });
        exe.linkSystemLibrary("dl");
        exe.linkSystemLibrary("asan");
    } else {
        exe.addCSourceFile(.{
            .file = b.path(sourcefile),
            .flags = &.{
                "-w",
                "-g",
            },
        });
        exe.linkSystemLibrary("dbghelp");
    }

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);

    // Create profile step (perf on Linux)
    if (target.result.os.tag == .linux) {
        const perf_cmd = b.addSystemCommand(&.{
            "perf", "record",
        });
        perf_cmd.addArtifactArg(exe);
        perf_cmd.step.dependOn(b.getInstallStep());

        const profile_step = b.step("profile", "Profile with perf (Linux only)");
        profile_step.dependOn(&perf_cmd.step);

        // Add echo for instructions
        const echo = b.addSystemCommand(&.{ "echo", "Run 'perf report' to view results" });
        echo.step.dependOn(&perf_cmd.step);
        profile_step.dependOn(&echo.step);
    }
}

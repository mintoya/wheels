const std = @import("std");

pub fn build(b: *std.Build) void {
    _ = std.process.run(b.allocator, b.graph.io, .{
        .argv = &.{
            "git",
            "clone",
            "https://github.com/jtsiomb/c11threads",
        },
    }) catch null;
    const target = b.standardTargetOptions(.{});

    const exe = b.addExecutable(.{
        .name = "wheels",
        .root_module = b.createModule(.{
            .target = target,
            .link_libc = true,
            .optimize = b.standardOptimizeOption(.{
                .preferred_optimize_mode = .Debug,
            }),
            .strip = false,
        }),
        // .use_llvm = false,
    });

    const cfile = b.option(
        []const u8,
        "file",
        "which file to run",
    ) orelse "tests.h";

    exe.root_module.addCSourceFile(.{
        .file = b.path(cfile),
        .flags = &.{
            "-g",
            "-w",
            "-std=c2y",
            "-fdefer-ts",
            "-fno-sanitize=vla-bound",
            "-fsanitize=alignment",
        },
        .language = .c,
    });

    if (target.result.os.tag == .windows) {
        exe.root_module.linkSystemLibrary("dbghelp", .{});

        exe.root_module.addCSourceFile(.{
            .file = b.path("c11threads/c11threads_win32.c"),
            .flags = &.{
                "-g",
                "-w",
                "-std=c2y",
                "-fdefer-ts",
                "-fno-sanitize=vla-bound",
                "-fsanitize=alignment",
            },
            .language = .c,
        });
    }
    exe.rdynamic = true;

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    // if (b.args) |args|
    //     run_cmd.addArgs(args);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);
}

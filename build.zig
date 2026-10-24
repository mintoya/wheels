const std = @import("std");

pub fn build(b: *std.Build) void {
    _ = std.process.run(b.allocator, b.graph.io, .{
        .argv = &.{ "git", "clone", "https://github.com/jtsiomb/c11threads" },
    }) catch @panic("couldnt clone needed dependency");
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "wheels",
        .root_module = b.createModule(.{
            .target = target,
            .link_libc = true,
            .optimize = optimize,
            .strip = false,
        }),
        // .use_llvm = false,
    });
    const cfile = b.option(
        []const u8,
        "file",
        "which file to run",
    ) orelse "tests.h";

    const compileflags = &.{
        "-std=c2y",
        "-fdefer-ts",
        "-fno-sanitize=vla-bound",
        "-fsanitize=alignment",
        "-finstrument-functions",
    };
    const calloc = b.addTranslateC(.{
        .root_source_file = b.path("allocator.h"),
        .target = target,
        .optimize = optimize,
    });

    const bridge_tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("bridge.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    bridge_tests.root_module.addImport("calloc", calloc.createModule());

    const run_bridge_tests = b.addRunArtifact(bridge_tests);
    const test_step = b.step("test", "Run the Zig tests");
    test_step.dependOn(&run_bridge_tests.step);

    exe.root_module.addCSourceFile(.{
        .file = b.path(cfile),
        .flags = compileflags,
        .language = .c,
    });

    if (target.result.os.tag == .windows) {
        exe.root_module.linkSystemLibrary("dbghelp", .{});

        exe.root_module.addCSourceFile(.{
            .file = b.path("c11threads/c11threads_win32.c"),
            .flags = compileflags,
            .language = .c,
        });
    }
    exe.rdynamic = true;

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);
}

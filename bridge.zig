const std = @import("std");
const CAlloc = @import("calloc");

const Allocator = std.mem.Allocator;

const c_alignment = @alignOf(CAlloc.myAlign);
const byte_alignment = std.mem.Alignment.fromByteUnits(c_alignment);

pub const Bridge = struct {
    allocator: Allocator,
    table: CAlloc.allocfns,

    pub fn init(allocator: Allocator) Bridge {
        return .{
            .allocator = allocator,
            .table = .{ .@"fn" = allocatorFn },
        };
    }

    pub fn cAllocator(self: *Bridge) CAlloc.allocfn {
        return &self.table;
    }
};

fn ptrSlice(ptr: *anyopaque, len: usize) []align(c_alignment) u8 {
    const bytes: [*]u8 = @ptrCast(ptr);
    return @as([*]align(c_alignment) u8, @alignCast(bytes))[0..len];
}

fn allocBytes(self: *const Bridge, len: usize) ?*anyopaque {
    const memory = self.allocator.alignedAlloc(u8, byte_alignment, len) catch return null;
    const payload: *anyopaque = @ptrCast(memory.ptr);
    return payload;
}

fn resizeBytes(self: *const Bridge, ptr: *anyopaque, old_len: usize, new_len: usize) ?*anyopaque {
    const old = ptrSlice(ptr, old_len);
    if (self.allocator.resize(old, new_len)) return ptr;

    const memory = self.allocator.alignedAlloc(u8, byte_alignment, new_len) catch return null;
    const keep = @min(old_len, new_len);
    @memcpy(memory[0..keep], old[0..keep]);
    self.allocator.free(old);
    const payload: *anyopaque = @ptrCast(memory.ptr);
    return payload;
}

fn freeBytes(self: *const Bridge, ptr: *anyopaque, len: usize) void {
    self.allocator.free(ptrSlice(ptr, len));
}

fn allocatorFn(
    table: CAlloc.allocfn,
    op: ?*anyopaque,
    from: CAlloc.usize,
    to: CAlloc.usize,
    file: [*c]const u8,
    line: CAlloc.uint,
) callconv(.c) ?*anyopaque {
    const field: *const CAlloc.allocfns = @ptrCast(table);
    const self: *const Bridge = @alignCast(@fieldParentPtr("table", field));

    return switch ((@as(u2, @intFromBool(from != 0)) << 1) | @intFromBool(to != 0)) {
        0b01 => allocBytes(self, @intCast(to)),
        0b11 => resizeBytes(self, op.?, @intCast(from), @intCast(to)),
        0b10 => br: {
            freeBytes(self, op.?, @intCast(from));
            break :br null;
        },
        else => {
            const source = if (file == null) "<unknown>" else std.mem.span(file);
            std.debug.panic(
                "allocator bridge: invalid call from {s}:{d} (from 0, to 0)",
                .{ source, line },
            );
        },
    };
}

test "C allocator over a Zig allocator" {
    var bridge = Bridge.init(std.testing.allocator);
    const c_allocator = bridge.cAllocator();
    const c_fn = c_allocator[0].@"fn".?;

    const bytes = c_fn(c_allocator, null, 0, 64, "bridge.zig", 0).?;
    try std.testing.expectEqual(@as(usize, 0), @intFromPtr(bytes) % c_alignment);

    const payload: [*]u8 = @ptrCast(bytes);
    for (0..64) |i| payload[i] = @intCast(i);

    // Grow through realloc, which may relocate the allocation.
    const grown = c_fn(c_allocator, bytes, 64, 256, "bridge.zig", 0).?;
    const grown_payload: [*]u8 = @ptrCast(grown);
    for (0..64) |i| try std.testing.expectEqual(@as(u8, @intCast(i)), grown_payload[i]);

    // Shrink, keeping only the prefix.
    const shrunk = c_fn(c_allocator, grown, 256, 32, "bridge.zig", 0).?;
    const shrunk_payload: [*]u8 = @ptrCast(shrunk);
    for (0..32) |i| try std.testing.expectEqual(@as(u8, @intCast(i)), shrunk_payload[i]);

    try std.testing.expectEqual(
        @as(?*anyopaque, null),
        c_fn(c_allocator, shrunk, 32, 0, "bridge.zig", 0),
    );
}

test "resize relocates when the Zig allocator refuses in-place growth" {
    var failing = std.testing.FailingAllocator.init(std.testing.allocator, .{
        .resize_fail_index = 0,
    });
    var bridge = Bridge.init(failing.allocator());
    const c_allocator = bridge.cAllocator();
    const c_fn = c_allocator[0].@"fn".?;

    const bytes = c_fn(c_allocator, null, 0, 32, "bridge.zig", 0).?;
    const payload: [*]u8 = @ptrCast(bytes);
    for (0..32) |i| payload[i] = @intCast(i);

    const grown = c_fn(c_allocator, bytes, 32, 96, "bridge.zig", 0).?;
    try std.testing.expect(grown != bytes);
    const grown_payload: [*]u8 = @ptrCast(grown);
    for (0..32) |i| try std.testing.expectEqual(@as(u8, @intCast(i)), grown_payload[i]);

    _ = c_fn(c_allocator, grown, 96, 0, "bridge.zig", 0);
}

test "two live allocations keep separate sizes" {
    var bridge = Bridge.init(std.testing.allocator);
    const c_allocator = bridge.cAllocator();
    const c_fn = c_allocator[0].@"fn".?;

    const a = c_fn(c_allocator, null, 0, 16, "bridge.zig", 0).?;
    const b = c_fn(c_allocator, null, 0, 128, "bridge.zig", 0).?;
    const a_payload: [*]u8 = @ptrCast(a);
    const b_payload: [*]u8 = @ptrCast(b);
    a_payload[0] = 0xAA;
    b_payload[127] = 0xBB;

    try std.testing.expectEqual(@as(u8, 0xAA), a_payload[0]);
    try std.testing.expectEqual(@as(u8, 0xBB), b_payload[127]);

    _ = c_fn(c_allocator, a, 16, 0, "bridge.zig", 0);
    try std.testing.expectEqual(@as(u8, 0xBB), b_payload[127]);
    _ = c_fn(c_allocator, b, 128, 0, "bridge.zig", 0);
}

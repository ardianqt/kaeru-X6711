#!/usr/bin/env python3
"""Find missing offsets for kaeru config."""
import struct
import re

data = open(r'C:\Users\My PC\Downloads\my-stock-v764-x6711\lk.img', 'rb').read()
base = 0x48200000

print("=== SEARCHING FOR MISSING OFFSETS ===")
print()

# 1. Find 'starting app' string and nearby code
idx = data.find(b'starting app')
if idx >= 0:
    print(f"'starting app' string @ file offset 0x{idx:08X}")

# 2. Find 'platform_init' string
idx = data.find(b'platform_init')
if idx >= 0:
    print(f"'platform_init' string @ file offset 0x{idx:08X}")

# 3. Search for app() function patterns
app_patterns = [
    bytes.fromhex('6448f8b5644e7844def734fb'),
    bytes.fromhex('424870b5424d'),
    bytes.fromhex('b84b2de9f04f'),
    bytes.fromhex('4948f8b5494e'),
]

print("\n=== APP FUNCTION PATTERNS ===")
for i, pat in enumerate(app_patterns):
    matches = [m for m in re.finditer(re.escape(pat), data)]
    if matches:
        for m in matches:
            print(f"  Pattern {i}: found @ 0x{m.start():08X} (VA 0x{m.start()+base:08X})")

# 4. Search for 'platform_init' caller pattern
# The caller should have a BL instruction to platform_init
print("\n=== PLATFORM_INIT CALLER ===")
pi_offset = 0x5627C  # From parse.py output: 0x4825647C - base = 0x5647C
# Actually let's use the detected address
pi_va = 0x4825647C
pi_file = pi_va - base
print(f"platform_init @ VA 0x{pi_va:08X} (file 0x{pi_file:08X})")

# Search backwards for BL to platform_init
for i in range(max(0, pi_file - 512), pi_file, 2):
    hw = struct.unpack_from('<H', data, i)[0]
    # Thumb BL: F000 F800 pattern (two halfwords)
    if i + 4 <= len(data):
        hw2 = struct.unpack_from('<H', data, i+2)[0]
        # Check for BL (F000-F7FF range for first half, F800-FBFF for second)
        if (hw & 0xF800) == 0xF000 and (hw2 & 0xF800) == 0xF800:
            # Calculate BL target
            imm10 = hw & 0x7FF
            imm11 = hw2 & 0x7FF
            J1 = (hw2 >> 13) & 1
            J2 = (hw2 >> 11) & 1
            imm32 = (imm10 << 12) | (imm11 << 1)
            if J1:
                imm32 |= 0x800000
            if J2:
                imm32 |= 0x400000
            if imm32 & 0x400000:
                imm32 |= ~0x7FFFFF  # Sign extend
            target = i + 4 + imm32
            if target == pi_file or target == pi_file + 1:
                print(f"  BL to platform_init @ 0x{i:08X}")

# 5. Search for BOOTMODE address
# Usually near fastboot_continue, look for STR/LDR patterns
print("\n=== BOOTMODE SEARCH ===")
# Search for 'g_boot_mode' or similar strings
for s in [b'g_boot_mode', b'boot_mode', b'BOOTMODE']:
    idx = data.find(s)
    if idx >= 0:
        print(f"  '{s.decode()}' string @ 0x{idx:08X}")

# 6. Look for APP_ADDRESS by finding where 'app' function is
# In LK, app() is often called from bootstrap2
# Let's search for 'app' string reference
print("\n=== APP FUNCTION SEARCH ===")
# Search for the string 'app' (short string, might be in format strings)
for s in [b'app', b'app ']:
    for m in re.finditer(re.escape(s), data):
        # Check if this is in a format string
        if m.start() > 0 and data[m.start()-1:m.start()] == b'%':
            print(f"  Format string with 'app' @ 0x{m.start():08X}")

# 7. Search for function prologues near key areas
print("\n=== FUNCTION PROLOGUES NEAR KEY AREAS ===")
# Look for PUSH {R4-R7, LR} (B5F0) patterns near where we expect functions
for area_name, start, end in [
    ("bootloader code", 0x10000, 0x80000),
    ("fastboot area", 0x20000, 0x40000),
]:
    push_count = 0
    for i in range(start, min(end, len(data)), 2):
        hw = struct.unpack_from('<H', data, i)[0]
        if hw == 0xB5F0:  # PUSH {R4-R7, LR}
            push_count += 1
    print(f"  {area_name}: {push_count} PUSH {{R4-R7, LR}} functions")

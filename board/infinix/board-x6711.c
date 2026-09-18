//
// SPDX-FileCopyrightText: 2026 coolishsecv2 <coolishsecv2@users.noreply.github.com>
// SPDX-License-Identifier: AGPL-3.0-or-later
//

#include <board_ops.h>
#include <lib/fastboot.h>
#include <lib/common.h>

void board_early_init(void) {}

static void cmd_purple(const char* arg, void* data, unsigned sz) {
    fastboot_info("Infinix X6711 - Kaeru Patched");
    fastboot_info("MT6833 | Bootloader Unlocked");
    fastboot_info("Orange State: Disabled");
    fastboot_info("Green State: Spoofed");
    fastboot_info("VBMeta: Locked Spoof");
    fastboot_okay("");
}

static void cmd_info(const char* arg, void* data, unsigned sz) {
    fastboot_info("Device: Infinix X6711");
    fastboot_info("SoC: MediaTek MT6833");
    fastboot_info("Bootloader Base: 0x48200000");
    fastboot_info("Patch: Dynamic (OTA Survivable)");
    fastboot_info("Features: Orange/Green/VBMeta Spoof");
    fastboot_okay("");
}

void board_late_init(void) {
    // Register custom fastboot commands
    fastboot_register("oem purple", cmd_purple, 1);
    fastboot_register("oem info", cmd_info, 1);

    // ---------------------------------------------------------
    // DYNAMIC PATCHING (OTA SURVIVABLE)
    // ---------------------------------------------------------

    // Disable Orange State Warning dynamically
    uint32_t orange_addr = SEARCH_PATTERN(CONFIG_BOOTLOADER_BASE, CONFIG_BOOTLOADER_BASE + CONFIG_BOOTLOADER_SIZE,
                                          0xb508, 0xf7ea, 0xff7f, 0xf7ea, 0xff77, 0x2100);
    if (orange_addr) {
        FORCE_RETURN(orange_addr, 0);
    }

    // Hardware Volume Key Boot Mode Overrides
    if (mtk_detect_key(0)) {
        set_bootmode(BOOTMODE_FASTBOOT);
    }

    bootmode_t mode = get_bootmode();

    // If booting to Normal System (Not Recovery/TWRP):
    if (mode != BOOTMODE_RECOVERY) {
        // Spoof verifiedbootstate to green
        uint32_t green_addr = SEARCH_PATTERN(CONFIG_BOOTLOADER_BASE, CONFIG_BOOTLOADER_BASE + CONFIG_BOOTLOADER_SIZE,
                                             0x4b18, 0x447b, 0x681b, 0x681b, 0x2b03, 0xd807);
        if (green_addr) {
            WRITE16(green_addr + 6, 0x2300);
        }

        // Spoof VBMeta to locked
        uint32_t lock_addr = SEARCH_PATTERN(CONFIG_BOOTLOADER_BASE, CONFIG_BOOTLOADER_BASE + CONFIG_BOOTLOADER_SIZE,
                                            0x2801, 0xd0f5, 0xb9a0, 0x9b09);
        if (lock_addr) {
            WRITE16(lock_addr + 8, 0xBF00);
        }
    }
}

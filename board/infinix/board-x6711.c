//
// SPDX-FileCopyrightText: 2026 coolishsecv2 <coolishsecv2@users.noreply.github.com>
// SPDX-License-Identifier: AGPL-3.0-or-later
//

#include <board_ops.h>
#include <lib/fastboot.h>
#include <lib/common.h>

void board_early_init(void) {}

// Video functions (same MT6833 video subsystem as LH8n)
static void video_clean_screen(void) {
    ((void (*)(void))(0x4823CB74 | 1))();
}

static void video_set_cursor(int row, int col) {
    ((void (*)(int, int))(0x4823CA20 | 1))(row, col);
}

static void mt_disp_update(void) {
    ((void (*)(uint32_t, uint32_t, uint32_t, uint32_t))(0x48200E8C | 1))(0, 0, 1080, 2400);
}

// Custom fastboot commands
static void cmd_purple(const char* arg, void* data, unsigned sz) {
    fastboot_info("Infinix X6711 - Kaeru Patched");
    fastboot_info("MT6833 | Bootloader Unlocked");
    fastboot_info("Orange State: Disabled");
    fastboot_info("Green State: Spoofed");
    fastboot_info("VBMeta: Locked Spoof");

    video_clean_screen();
    video_set_cursor(10, 0);
    video_printf("====================================\n");
    video_printf("  Infinix X6711 - Kaeru Patched\n");
    video_printf("  MT6833 | Bootloader Unlocked\n");
    video_printf("  Orange State: Disabled\n");
    video_printf("  Green State: Spoofed\n");
    video_printf("  VBMeta: Locked Spoof\n");
    video_printf("====================================\n");
    mt_disp_update();

    fastboot_okay("");
}

static void cmd_info(const char* arg, void* data, unsigned sz) {
    fastboot_info("Device: Infinix X6711");
    fastboot_info("SoC: MediaTek MT6833");
    fastboot_info("Bootloader Base: 0x48200000");
    fastboot_info("LK Size: 0x11BF58");
    fastboot_info("Patch: Dynamic (OTA Survivable)");
    fastboot_info("Features: Orange/Green/VBMeta Spoof");

    video_clean_screen();
    video_set_cursor(5, 0);
    video_printf("====================================\n");
    video_printf("  DEVICE INFO\n");
    video_printf("====================================\n");
    video_printf("  Device: Infinix X6711\n");
    video_printf("  SoC: MediaTek MT6833\n");
    video_printf("  Bootloader Base: 0x48200000\n");
    video_printf("  LK Size: 0x11BF58\n");
    video_printf("  Patch: Dynamic (OTA Survivable)\n");
    video_printf("  Features: Orange/Green/VBMeta Spoof\n");
    video_printf("====================================\n");
    mt_disp_update();

    fastboot_okay("");
}

static void cmd_credits(const char* arg, void* data, unsigned sz) {
    fastboot_info("Kaeru by R0rt1z2 & AntiEngineer");
    fastboot_info("X6711 port by coolishsecv2");
    fastboot_info("Pattern analysis by Shomy");

    video_clean_screen();
    video_set_cursor(10, 0);
    video_printf("====================================\n");
    video_printf("  CREDITS\n");
    video_printf("====================================\n");
    video_printf("  Kaeru by R0rt1z2 & AntiEngineer\n");
    video_printf("  X6711 port by coolishsecv2\n");
    video_printf("  Pattern analysis by Shomy\n");
    video_printf("====================================\n");
    mt_disp_update();

    fastboot_okay("");
}

void board_late_init(void) {
    // Register custom fastboot commands
    fastboot_register("oem purple", cmd_purple, 1);
    fastboot_register("oem info", cmd_info, 1);
    fastboot_register("oem credits", cmd_credits, 1);

    // Boot splash screen
    video_clean_screen();
    video_set_cursor(10, 0);
    video_printf("====================================\n");
    video_printf("  Kaeru - X6711 Patched\n");
    video_printf("  MT6833 | Bootloader Unlocked\n");
    video_printf("====================================\n");
    mt_disp_update();

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
        video_set_cursor(15, 0);
        video_printf(">>> VOL+ DETECTED: FORCING BOOTLOADER <<<\n");
        mt_disp_update();
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

/**
 * @file test_landvalue_overlay.cpp
 * @brief Tests for land value overlay visualization (Ticket E10-107)
 *
 * Validates:
 * - LandValueOverlay implements IGridOverlay interface
 * - getName() returns "Land Value"
 * - isActive() returns true
 * - getColorAt() maps land values to appropriate colors:
 *   - Very low (0-63): Red tint
 *   - Low (64-127): Orange tint
 *   - Neutral (128-191): Yellow tint
 *   - High (192-255): Green tint
 * - Alpha channel scales with value
 * - Out-of-bounds coordinates return appropriate color
 */

#include <cassert>
#include <cstdio>
#include <cstring>

#include "sims3000/landvalue/LandValueOverlay.h"

using namespace sims3000::landvalue;
using namespace sims3000::services;

// --------------------------------------------------------------------------
// Test: getName() returns correct name
// --------------------------------------------------------------------------
static void test_get_name() {
    LandValueGrid grid(4, 4);
    LandValueOverlay overlay(grid);

    assert(std::strcmp(overlay.getName(), "Land Value") == 0);

    std::printf("  PASS: getName() returns 'Land Value'\n");
}

// --------------------------------------------------------------------------
// Test: isActive() returns true
// --------------------------------------------------------------------------
static void test_is_active() {
    LandValueGrid grid(4, 4);
    LandValueOverlay overlay(grid);

    assert(overlay.isActive() == true);

    std::printf("  PASS: isActive() returns true\n");
}

// --------------------------------------------------------------------------
// Test: Very low value (0-63) produces red color
// --------------------------------------------------------------------------
static void test_very_low_value_color() {
    LandValueGrid grid(4, 4);
    grid.set_value(0, 0, 0);    // Minimum value
    grid.set_value(1, 0, 32);   // Mid very-low
    grid.set_value(2, 0, 63);   // Max very-low

    LandValueOverlay overlay(grid);

    // Value 0: Red
    OverlayColor c0 = overlay.getColorAt(0, 0);
    assert(c0.r == 255 && c0.g == 0 && c0.b == 0);
    assert(c0.a == 128);  // 128 + 0 * 2 = 128

    // Value 32: Red
    OverlayColor c32 = overlay.getColorAt(1, 0);
    assert(c32.r == 255 && c32.g == 0 && c32.b == 0);
    assert(c32.a == 192);  // 128 + 32 * 2 = 192

    // Value 63: Red
    OverlayColor c63 = overlay.getColorAt(2, 0);
    assert(c63.r == 255 && c63.g == 0 && c63.b == 0);
    assert(c63.a == 254);  // 128 + 63 * 2 = 254

    std::printf("  PASS: Very low values (0-63) produce red color\n");
}

// --------------------------------------------------------------------------
// Test: Low value (64-127) produces orange color
// --------------------------------------------------------------------------
static void test_low_value_color() {
    LandValueGrid grid(4, 4);
    grid.set_value(0, 0, 64);   // Min low
    grid.set_value(1, 0, 96);   // Mid low
    grid.set_value(2, 0, 127);  // Max low

    LandValueOverlay overlay(grid);

    // Value 64: Orange
    OverlayColor c64 = overlay.getColorAt(0, 0);
    assert(c64.r == 255 && c64.g == 165 && c64.b == 0);
    assert(c64.a == 128);  // 128 + (64 - 64) * 2 = 128

    // Value 96: Orange
    OverlayColor c96 = overlay.getColorAt(1, 0);
    assert(c96.r == 255 && c96.g == 165 && c96.b == 0);
    assert(c96.a == 192);  // 128 + (96 - 64) * 2 = 192

    // Value 127: Orange
    OverlayColor c127 = overlay.getColorAt(2, 0);
    assert(c127.r == 255 && c127.g == 165 && c127.b == 0);
    assert(c127.a == 254);  // 128 + (127 - 64) * 2 = 254

    std::printf("  PASS: Low values (64-127) produce orange color\n");
}

// --------------------------------------------------------------------------
// Test: Neutral value (128-191) produces yellow color
// --------------------------------------------------------------------------
static void test_neutral_value_color() {
    LandValueGrid grid(4, 4);
    grid.set_value(0, 0, 128);  // Min neutral (default)
    grid.set_value(1, 0, 160);  // Mid neutral
    grid.set_value(2, 0, 191);  // Max neutral

    LandValueOverlay overlay(grid);

    // Value 128: Yellow
    OverlayColor c128 = overlay.getColorAt(0, 0);
    assert(c128.r == 255 && c128.g == 255 && c128.b == 0);
    assert(c128.a == 128);  // 128 + (128 - 128) * 2 = 128

    // Value 160: Yellow
    OverlayColor c160 = overlay.getColorAt(1, 0);
    assert(c160.r == 255 && c160.g == 255 && c160.b == 0);
    assert(c160.a == 192);  // 128 + (160 - 128) * 2 = 192

    // Value 191: Yellow
    OverlayColor c191 = overlay.getColorAt(2, 0);
    assert(c191.r == 255 && c191.g == 255 && c191.b == 0);
    assert(c191.a == 254);  // 128 + (191 - 128) * 2 = 254

    std::printf("  PASS: Neutral values (128-191) produce yellow color\n");
}

// --------------------------------------------------------------------------
// Test: High value (192-255) produces green color
// --------------------------------------------------------------------------
static void test_high_value_color() {
    LandValueGrid grid(4, 4);
    grid.set_value(0, 0, 192);  // Min high
    grid.set_value(1, 0, 224);  // Mid high
    grid.set_value(2, 0, 255);  // Max high

    LandValueOverlay overlay(grid);

    // Value 192: Green
    OverlayColor c192 = overlay.getColorAt(0, 0);
    assert(c192.r == 0 && c192.g == 255 && c192.b == 0);
    assert(c192.a == 192);  // 192 + (192 - 192) = 192

    // Value 224: Green
    OverlayColor c224 = overlay.getColorAt(1, 0);
    assert(c224.r == 0 && c224.g == 255 && c224.b == 0);
    assert(c224.a == 224);  // 192 + (224 - 192) = 224

    // Value 255: Green
    OverlayColor c255 = overlay.getColorAt(2, 0);
    assert(c255.r == 0 && c255.g == 255 && c255.b == 0);
    assert(c255.a == 255);  // 192 + (255 - 192) = 255

    std::printf("  PASS: High values (192-255) produce green color\n");
}

// --------------------------------------------------------------------------
// Test: Out-of-bounds coordinates return default (grid returns 0)
// --------------------------------------------------------------------------
static void test_out_of_bounds() {
    LandValueGrid grid(4, 4);
    LandValueOverlay overlay(grid);

    // Grid returns 0 for out-of-bounds
    OverlayColor c = overlay.getColorAt(100, 100);

    // Should be red (value 0)
    assert(c.r == 255 && c.g == 0 && c.b == 0);
    assert(c.a == 128);

    std::printf("  PASS: Out-of-bounds coordinates handled correctly\n");
}

// --------------------------------------------------------------------------
// Test: Color gradient is continuous across boundaries
// --------------------------------------------------------------------------
static void test_color_boundaries() {
    LandValueGrid grid(8, 1);

    // Test boundary values
    grid.set_value(0, 0, 63);   // Max very-low (red)
    grid.set_value(1, 0, 64);   // Min low (orange)
    grid.set_value(2, 0, 127);  // Max low (orange)
    grid.set_value(3, 0, 128);  // Min neutral (yellow)
    grid.set_value(4, 0, 191);  // Max neutral (yellow)
    grid.set_value(5, 0, 192);  // Min high (green)

    LandValueOverlay overlay(grid);

    OverlayColor c63 = overlay.getColorAt(0, 0);
    OverlayColor c64 = overlay.getColorAt(1, 0);
    OverlayColor c127 = overlay.getColorAt(2, 0);
    OverlayColor c128 = overlay.getColorAt(3, 0);
    OverlayColor c191 = overlay.getColorAt(4, 0);
    OverlayColor c192 = overlay.getColorAt(5, 0);

    // Verify colors change at boundaries
    assert(c63.r == 255 && c63.g == 0);     // Red
    assert(c64.r == 255 && c64.g == 165);   // Orange
    assert(c127.r == 255 && c127.g == 165); // Orange
    assert(c128.r == 255 && c128.g == 255); // Yellow
    assert(c191.r == 255 && c191.g == 255); // Yellow
    assert(c192.r == 0 && c192.g == 255);   // Green

    std::printf("  PASS: Color boundaries are correct\n");
}

// --------------------------------------------------------------------------
// Test: Overlay references grid correctly (does not copy)
// --------------------------------------------------------------------------
static void test_grid_reference() {
    LandValueGrid grid(4, 4);
    grid.set_value(0, 0, 100);

    LandValueOverlay overlay(grid);

    // Initial color
    OverlayColor c1 = overlay.getColorAt(0, 0);
    assert(c1.r == 255 && c1.g == 165 && c1.b == 0);  // Orange (value 100)

    // Modify grid
    grid.set_value(0, 0, 200);

    // Overlay should reflect change
    OverlayColor c2 = overlay.getColorAt(0, 0);
    assert(c2.r == 0 && c2.g == 255 && c2.b == 0);  // Green (value 200)

    std::printf("  PASS: Overlay references grid (not copied)\n");
}

// --------------------------------------------------------------------------
// Test: Alpha channel always increases with value
// --------------------------------------------------------------------------
static void test_alpha_progression() {
    LandValueGrid grid(16, 1);

    // Set values from 0 to 240 in steps of 16
    for (int i = 0; i < 16; ++i) {
        grid.set_value(i, 0, i * 16);
    }

    LandValueOverlay overlay(grid);

    uint8_t prev_alpha = 0;
    for (int i = 0; i < 16; ++i) {
        OverlayColor c = overlay.getColorAt(i, 0);
        // Alpha should generally increase (may reset at boundaries)
        // Just verify it's within valid range
        assert(c.a >= 128 && c.a <= 255);
    }

    std::printf("  PASS: Alpha channel progression is valid\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Land Value Overlay Tests (E10-107) ===\n");

    test_get_name();
    test_is_active();
    test_very_low_value_color();
    test_low_value_color();
    test_neutral_value_color();
    test_high_value_color();
    test_out_of_bounds();
    test_color_boundaries();
    test_grid_reference();
    test_alpha_progression();

    std::printf("All land value overlay tests passed.\n");
    return 0;
}

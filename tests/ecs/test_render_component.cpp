/**
 * @file test_render_component.cpp
 * @brief Unit tests for RenderComponent (Ticket 2-031).
 *
 * Tests:
 * - Default construction with proper initial values
 * - Field accessors and mutators
 * - Render layer assignment
 * - Emissive properties for bioluminescent glow
 * - Component size constraint
 */

#include "sims3000/ecs/Components.h"
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace {

int g_testsPassed = 0;
int g_testsFailed = 0;

void test(bool condition, const char* name) {
    if (condition) {
        std::printf("[PASS] %s\n", name);
        ++g_testsPassed;
    } else {
        std::printf("[FAIL] %s\n", name);
        ++g_testsFailed;
    }
}

// Floating point comparison with epsilon
bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

bool approxEqual(const glm::vec4& a, const glm::vec4& b, float epsilon = 0.0001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon) &&
           approxEqual(a.w, b.w, epsilon);
}

} // anonymous namespace

// =============================================================================
// Test: Default Construction
// =============================================================================

void test_DefaultConstruction() {
    using namespace sims3000;

    RenderComponent rc;

    // Asset references default to nullptr
    test(rc.model == nullptr, "model defaults to nullptr");
    test(rc.texture == nullptr, "texture defaults to nullptr");

    // Tint color defaults to white (1,1,1,1)
    glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
    test(approxEqual(rc.tint_color, white), "tint_color defaults to white");

    // Emissive color defaults to teal (0.0, 1.0, 0.8)
    glm::vec3 teal{0.0f, 1.0f, 0.8f};
    test(approxEqual(rc.emissive_color, teal), "emissive_color defaults to teal");

    // Scale defaults to 1.0
    test(approxEqual(rc.scale, 1.0f), "scale defaults to 1.0");

    // Emissive intensity defaults to 0.0 (unpowered)
    test(approxEqual(rc.emissive_intensity, 0.0f), "emissive_intensity defaults to 0.0");

    // Layer defaults to Buildings
    test(rc.layer == RenderLayer::Buildings, "layer defaults to Buildings");

    // Visibility defaults to true
    test(rc.visible == true, "visible defaults to true");
}

// =============================================================================
// Test: Model Handle Reference
// =============================================================================

void test_ModelHandleReference() {
    using namespace sims3000;

    RenderComponent rc;

    // Can set model handle
    Model dummyModel;
    rc.model = &dummyModel;
    test(rc.model == &dummyModel, "model handle can be assigned");

    // Can clear model handle
    rc.model = nullptr;
    test(rc.model == nullptr, "model handle can be cleared");
}

// =============================================================================
// Test: Texture Handle Reference
// =============================================================================

void test_TextureHandleReference() {
    using namespace sims3000;

    RenderComponent rc;

    // Can set texture handle
    Texture dummyTexture;
    rc.texture = &dummyTexture;
    test(rc.texture == &dummyTexture, "texture handle can be assigned");

    // Can clear texture handle
    rc.texture = nullptr;
    test(rc.texture == nullptr, "texture handle can be cleared");
}

// =============================================================================
// Test: Render Layer Assignment
// =============================================================================

void test_RenderLayerAssignment() {
    using namespace sims3000;

    RenderComponent rc;

    // Test each layer can be assigned
    rc.layer = RenderLayer::Underground;
    test(rc.layer == RenderLayer::Underground, "can assign Underground layer");

    rc.layer = RenderLayer::Terrain;
    test(rc.layer == RenderLayer::Terrain, "can assign Terrain layer");

    rc.layer = RenderLayer::Water;
    test(rc.layer == RenderLayer::Water, "can assign Water layer");

    rc.layer = RenderLayer::Roads;
    test(rc.layer == RenderLayer::Roads, "can assign Roads layer");

    rc.layer = RenderLayer::Buildings;
    test(rc.layer == RenderLayer::Buildings, "can assign Buildings layer");

    rc.layer = RenderLayer::Units;
    test(rc.layer == RenderLayer::Units, "can assign Units layer");

    rc.layer = RenderLayer::Effects;
    test(rc.layer == RenderLayer::Effects, "can assign Effects layer");

    rc.layer = RenderLayer::DataOverlay;
    test(rc.layer == RenderLayer::DataOverlay, "can assign DataOverlay layer");

    rc.layer = RenderLayer::UIWorld;
    test(rc.layer == RenderLayer::UIWorld, "can assign UIWorld layer");

    // Verify layer ordering (enum values)
    test(static_cast<int>(RenderLayer::Underground) < static_cast<int>(RenderLayer::Terrain),
         "Underground renders before Terrain");
    test(static_cast<int>(RenderLayer::Buildings) < static_cast<int>(RenderLayer::Effects),
         "Buildings render before Effects");
}

// =============================================================================
// Test: Visibility Flag
// =============================================================================

void test_VisibilityFlag() {
    using namespace sims3000;

    RenderComponent rc;

    // Default is visible
    test(rc.visible == true, "visible defaults to true");

    // Can hide
    rc.visible = false;
    test(rc.visible == false, "visibility can be set to false");

    // Can show
    rc.visible = true;
    test(rc.visible == true, "visibility can be set to true");
}

// =============================================================================
// Test: Tint Color
// =============================================================================

void test_TintColor() {
    using namespace sims3000;

    RenderComponent rc;

    // Default is white
    glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
    test(approxEqual(rc.tint_color, white), "tint_color defaults to white");

    // Can set custom color
    glm::vec4 red{1.0f, 0.0f, 0.0f, 1.0f};
    rc.tint_color = red;
    test(approxEqual(rc.tint_color, red), "tint_color can be set to red");

    // Can set semi-transparent
    glm::vec4 semiTransparent{0.5f, 0.5f, 0.5f, 0.5f};
    rc.tint_color = semiTransparent;
    test(approxEqual(rc.tint_color, semiTransparent), "tint_color can be set to semi-transparent");
}

// =============================================================================
// Test: Scale Factor
// =============================================================================

void test_ScaleFactor() {
    using namespace sims3000;

    RenderComponent rc;

    // Default is 1.0
    test(approxEqual(rc.scale, 1.0f), "scale defaults to 1.0");

    // Can set larger scale
    rc.scale = 2.0f;
    test(approxEqual(rc.scale, 2.0f), "scale can be set to 2.0");

    // Can set smaller scale
    rc.scale = 0.5f;
    test(approxEqual(rc.scale, 0.5f), "scale can be set to 0.5");

    // Can set fractional scale for variety
    rc.scale = 1.15f;
    test(approxEqual(rc.scale, 1.15f), "scale can be set to 1.15 for variety");
}

// =============================================================================
// Test: Emissive Intensity (Bioluminescent Glow)
// =============================================================================

void test_EmissiveIntensity() {
    using namespace sims3000;

    RenderComponent rc;

    // Default is 0.0 (unpowered)
    test(approxEqual(rc.emissive_intensity, 0.0f), "emissive_intensity defaults to 0.0 (unpowered)");

    // Powered building: intensity > 0
    rc.emissive_intensity = 0.7f;
    test(approxEqual(rc.emissive_intensity, 0.7f), "emissive_intensity can be set to 0.7 (powered)");
    test(rc.emissive_intensity > 0.0f, "powered building has intensity > 0");

    // Max intensity
    rc.emissive_intensity = 1.0f;
    test(approxEqual(rc.emissive_intensity, 1.0f), "emissive_intensity can be set to 1.0 (max glow)");

    // Unpowered building: intensity = 0
    rc.emissive_intensity = 0.0f;
    test(approxEqual(rc.emissive_intensity, 0.0f), "unpowered building has intensity = 0");
}

// =============================================================================
// Test: Emissive Color (Per-instance Glow Color)
// =============================================================================

void test_EmissiveColor() {
    using namespace sims3000;

    RenderComponent rc;

    // Default is teal (bioluminescent theme)
    glm::vec3 teal{0.0f, 1.0f, 0.8f};
    test(approxEqual(rc.emissive_color, teal), "emissive_color defaults to teal");

    // Can set custom glow color
    glm::vec3 magenta{1.0f, 0.0f, 1.0f};
    rc.emissive_color = magenta;
    test(approxEqual(rc.emissive_color, magenta), "emissive_color can be set to magenta");

    // Can set orange for energy systems
    glm::vec3 orange{1.0f, 0.6f, 0.0f};
    rc.emissive_color = orange;
    test(approxEqual(rc.emissive_color, orange), "emissive_color can be set to orange");
}

// =============================================================================
// Test: Component Is Trivially Copyable
// =============================================================================

void test_TriviallyCopyable() {
    using namespace sims3000;

    // RenderComponent must be trivially copyable for network serialization
    // Note: While the component uses pointer handles, the pointers themselves
    // are trivially copyable. The asset management is handled externally.
    test(std::is_trivially_copyable<RenderComponent>::value,
         "RenderComponent is trivially copyable");

    // RenderLayer enum is also trivially copyable
    test(std::is_trivially_copyable<RenderLayer>::value,
         "RenderLayer is trivially copyable");
}

// =============================================================================
// Test: Component Metadata (SyncPolicy::None)
// =============================================================================

void test_ComponentMetadata() {
    using namespace sims3000;

    // RenderComponent should have SyncPolicy::None (client-only)
    test(ComponentMeta<RenderComponent>::syncPolicy == SyncPolicy::None,
         "RenderComponent has SyncPolicy::None");

    // RenderComponent should not be interpolated
    test(ComponentMeta<RenderComponent>::interpolated == false,
         "RenderComponent is not interpolated");

    // Verify component name
    test(std::strcmp(ComponentMeta<RenderComponent>::name, "RenderComponent") == 0,
         "ComponentMeta name is 'RenderComponent'");
}

// =============================================================================
// Test: Powered vs Unpowered Building State
// =============================================================================

void test_PoweredUnpoweredState() {
    using namespace sims3000;

    RenderComponent rc;

    // Unpowered state (default)
    test(rc.emissive_intensity == 0.0f, "default is unpowered (intensity = 0)");

    // Simulate powering on
    rc.emissive_intensity = 0.8f;
    bool isPowered = rc.emissive_intensity > 0.0f;
    test(isPowered, "powered state: emissive_intensity > 0");

    // Simulate power loss
    rc.emissive_intensity = 0.0f;
    isPowered = rc.emissive_intensity > 0.0f;
    test(!isPowered, "unpowered state: emissive_intensity = 0");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::printf("=== RenderComponent Tests (Ticket 2-031) ===\n\n");

    test_DefaultConstruction();
    test_ModelHandleReference();
    test_TextureHandleReference();
    test_RenderLayerAssignment();
    test_VisibilityFlag();
    test_TintColor();
    test_ScaleFactor();
    test_EmissiveIntensity();
    test_EmissiveColor();
    test_TriviallyCopyable();
    test_ComponentMetadata();
    test_PoweredUnpoweredState();

    std::printf("\n=== Results ===\n");
    std::printf("Passed: %d\n", g_testsPassed);
    std::printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed > 0 ? 1 : 0;
}

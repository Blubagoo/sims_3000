/**
 * @file test_terminal_component.cpp
 * @brief Unit tests for TerminalComponent (Epic 7, Ticket E7-031)
 */

#include <sims3000/transport/TerminalComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_terminal_component_size() {
    printf("Testing TerminalComponent size...\n");

    assert(sizeof(TerminalComponent) == 10);

    printf("  PASS: TerminalComponent is 10 bytes\n");
}

void test_terminal_trivially_copyable() {
    printf("Testing TerminalComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<TerminalComponent>::value);

    printf("  PASS: TerminalComponent is trivially copyable\n");
}

void test_terminal_default_initialization() {
    printf("Testing default initialization...\n");

    TerminalComponent terminal{};
    assert(terminal.type == TerminalType::SurfaceStation);
    assert(terminal.capacity == 200);
    assert(terminal.current_usage == 0);
    assert(terminal.coverage_radius == 8);
    assert(terminal.is_powered == false);
    assert(terminal.is_active == false);

    printf("  PASS: Default initialization works correctly\n");
}

void test_terminal_custom_values() {
    printf("Testing custom value assignment...\n");

    TerminalComponent terminal{};
    terminal.type = TerminalType::IntermodalHub;
    terminal.capacity = 500;
    terminal.current_usage = 350;
    terminal.coverage_radius = 12;
    terminal.is_powered = true;
    terminal.is_active = true;

    assert(terminal.type == TerminalType::IntermodalHub);
    assert(terminal.capacity == 500);
    assert(terminal.current_usage == 350);
    assert(terminal.coverage_radius == 12);
    assert(terminal.is_powered == true);
    assert(terminal.is_active == true);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_terminal_type_enum() {
    printf("Testing TerminalType enum values...\n");

    assert(static_cast<uint8_t>(TerminalType::SurfaceStation) == 0);
    assert(static_cast<uint8_t>(TerminalType::SubterraStation) == 1);
    assert(static_cast<uint8_t>(TerminalType::IntermodalHub) == 2);
    assert(sizeof(TerminalType) == 1);

    printf("  PASS: TerminalType enum values are correct\n");
}

void test_terminal_copy() {
    printf("Testing copy semantics...\n");

    TerminalComponent original{};
    original.type = TerminalType::SubterraStation;
    original.capacity = 400;
    original.current_usage = 200;
    original.coverage_radius = 16;
    original.is_powered = true;
    original.is_active = true;

    TerminalComponent copy = original;
    assert(copy.type == TerminalType::SubterraStation);
    assert(copy.capacity == 400);
    assert(copy.current_usage == 200);
    assert(copy.coverage_radius == 16);
    assert(copy.is_powered == true);
    assert(copy.is_active == true);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_terminal_all_types() {
    printf("Testing all terminal types can be assigned...\n");

    TerminalComponent terminal{};

    terminal.type = TerminalType::SurfaceStation;
    assert(terminal.type == TerminalType::SurfaceStation);

    terminal.type = TerminalType::SubterraStation;
    assert(terminal.type == TerminalType::SubterraStation);

    terminal.type = TerminalType::IntermodalHub;
    assert(terminal.type == TerminalType::IntermodalHub);

    printf("  PASS: All terminal types can be assigned\n");
}

int main() {
    printf("=== TerminalComponent Unit Tests (Epic 7, Ticket E7-031) ===\n\n");

    test_terminal_component_size();
    test_terminal_trivially_copyable();
    test_terminal_default_initialization();
    test_terminal_custom_values();
    test_terminal_type_enum();
    test_terminal_copy();
    test_terminal_all_types();

    printf("\n=== All TerminalComponent Tests Passed ===\n");
    return 0;
}

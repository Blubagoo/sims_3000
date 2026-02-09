/**
 * @file test_service_types.cpp
 * @brief Unit tests for ServiceTypes enums and config (Epic 9, Tickets E9-001, E9-030)
 *
 * Tests cover:
 * - ServiceType enum values (0-3)
 * - ServiceTier enum values (1-3)
 * - String conversion functions (to_string, from_string)
 * - Enum underlying type sizes (1 byte each)
 * - Count constants
 * - get_service_config() for all type+tier combos
 * - Enforcer-specific config values (E9-030)
 * - Enforcer suppression multiplier constant
 * - Validity check functions
 */

#include <sims3000/services/ServiceTypes.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>

using namespace sims3000::services;

void test_service_type_enum_values() {
    printf("Testing ServiceType enum values...\n");

    assert(static_cast<uint8_t>(ServiceType::Enforcer)       == 0);
    assert(static_cast<uint8_t>(ServiceType::HazardResponse) == 1);
    assert(static_cast<uint8_t>(ServiceType::Medical)        == 2);
    assert(static_cast<uint8_t>(ServiceType::Education)      == 3);

    printf("  PASS: ServiceType enum values correct\n");
}

void test_service_type_count() {
    printf("Testing ServiceType count...\n");

    assert(SERVICE_TYPE_COUNT == 4);

    printf("  PASS: SERVICE_TYPE_COUNT correct\n");
}

void test_service_tier_enum_values() {
    printf("Testing ServiceTier enum values...\n");

    assert(static_cast<uint8_t>(ServiceTier::Post)    == 1);
    assert(static_cast<uint8_t>(ServiceTier::Station) == 2);
    assert(static_cast<uint8_t>(ServiceTier::Nexus)   == 3);

    printf("  PASS: ServiceTier enum values correct\n");
}

void test_service_tier_count() {
    printf("Testing ServiceTier count...\n");

    assert(SERVICE_TIER_COUNT == 3);

    printf("  PASS: SERVICE_TIER_COUNT correct\n");
}

void test_service_type_to_string() {
    printf("Testing service_type_to_string...\n");

    assert(strcmp(service_type_to_string(ServiceType::Enforcer), "Enforcer") == 0);
    assert(strcmp(service_type_to_string(ServiceType::HazardResponse), "HazardResponse") == 0);
    assert(strcmp(service_type_to_string(ServiceType::Medical), "Medical") == 0);
    assert(strcmp(service_type_to_string(ServiceType::Education), "Education") == 0);

    // Test unknown value
    assert(strcmp(service_type_to_string(static_cast<ServiceType>(255)), "Unknown") == 0);

    printf("  PASS: service_type_to_string works correctly\n");
}

void test_service_type_from_string() {
    printf("Testing service_type_from_string...\n");

    ServiceType type;

    assert(service_type_from_string("Enforcer", type));
    assert(type == ServiceType::Enforcer);

    assert(service_type_from_string("HazardResponse", type));
    assert(type == ServiceType::HazardResponse);

    assert(service_type_from_string("Medical", type));
    assert(type == ServiceType::Medical);

    assert(service_type_from_string("Education", type));
    assert(type == ServiceType::Education);

    // Invalid strings
    assert(!service_type_from_string("InvalidType", type));
    assert(!service_type_from_string("", type));
    assert(!service_type_from_string(nullptr, type));

    printf("  PASS: service_type_from_string works correctly\n");
}

void test_service_tier_to_string() {
    printf("Testing service_tier_to_string...\n");

    assert(strcmp(service_tier_to_string(ServiceTier::Post), "Post") == 0);
    assert(strcmp(service_tier_to_string(ServiceTier::Station), "Station") == 0);
    assert(strcmp(service_tier_to_string(ServiceTier::Nexus), "Nexus") == 0);

    // Test unknown value
    assert(strcmp(service_tier_to_string(static_cast<ServiceTier>(0)), "Unknown") == 0);
    assert(strcmp(service_tier_to_string(static_cast<ServiceTier>(255)), "Unknown") == 0);

    printf("  PASS: service_tier_to_string works correctly\n");
}

void test_service_tier_from_string() {
    printf("Testing service_tier_from_string...\n");

    ServiceTier tier;

    assert(service_tier_from_string("Post", tier));
    assert(tier == ServiceTier::Post);

    assert(service_tier_from_string("Station", tier));
    assert(tier == ServiceTier::Station);

    assert(service_tier_from_string("Nexus", tier));
    assert(tier == ServiceTier::Nexus);

    // Invalid strings
    assert(!service_tier_from_string("InvalidTier", tier));
    assert(!service_tier_from_string("", tier));
    assert(!service_tier_from_string(nullptr, tier));

    printf("  PASS: service_tier_from_string works correctly\n");
}

void test_enum_underlying_type_sizes() {
    printf("Testing enum underlying type sizes...\n");

    assert(sizeof(ServiceType) == 1);
    assert(sizeof(ServiceTier) == 1);

    printf("  PASS: All enum underlying type sizes correct (1 byte each)\n");
}

void test_all_service_types_have_strings() {
    printf("Testing all service types have valid string representations...\n");

    for (uint8_t i = 0; i < SERVICE_TYPE_COUNT; ++i) {
        ServiceType type = static_cast<ServiceType>(i);
        assert(strcmp(service_type_to_string(type), "Unknown") != 0);
    }

    printf("  PASS: All service types have valid strings\n");
}

void test_all_service_tiers_have_strings() {
    printf("Testing all service tiers have valid string representations...\n");

    for (uint8_t i = 1; i <= SERVICE_TIER_COUNT; ++i) {
        ServiceTier tier = static_cast<ServiceTier>(i);
        assert(strcmp(service_tier_to_string(tier), "Unknown") != 0);
    }

    printf("  PASS: All service tiers have valid strings\n");
}

// ============================================================================
// E9-030: Enforcer Config Tests
// ============================================================================

void test_enforcer_post_config() {
    printf("Testing Enforcer Post config (E9-030)...\n");

    ServiceConfig cfg = get_service_config(ServiceType::Enforcer, ServiceTier::Post);
    assert(cfg.base_radius == 8);
    assert(cfg.base_effectiveness == 100);
    assert(cfg.capacity == 0);
    assert(cfg.footprint_width == 1);
    assert(cfg.footprint_height == 1);

    printf("  PASS: Enforcer Post config correct\n");
}

void test_enforcer_station_config() {
    printf("Testing Enforcer Station config (E9-030)...\n");

    ServiceConfig cfg = get_service_config(ServiceType::Enforcer, ServiceTier::Station);
    assert(cfg.base_radius == 12);
    assert(cfg.base_effectiveness == 100);
    assert(cfg.capacity == 0);
    assert(cfg.footprint_width == 2);
    assert(cfg.footprint_height == 2);

    printf("  PASS: Enforcer Station config correct\n");
}

void test_enforcer_nexus_config() {
    printf("Testing Enforcer Nexus config (E9-030)...\n");

    ServiceConfig cfg = get_service_config(ServiceType::Enforcer, ServiceTier::Nexus);
    assert(cfg.base_radius == 16);
    assert(cfg.base_effectiveness == 100);
    assert(cfg.capacity == 0);
    assert(cfg.footprint_width == 3);
    assert(cfg.footprint_height == 3);

    printf("  PASS: Enforcer Nexus config correct\n");
}

void test_enforcer_suppression_multiplier() {
    printf("Testing Enforcer suppression multiplier (E9-030)...\n");

    assert(ENFORCER_SUPPRESSION_MULTIPLIER > 0.69f);
    assert(ENFORCER_SUPPRESSION_MULTIPLIER < 0.71f);
    // Exact float comparison
    assert(ENFORCER_SUPPRESSION_MULTIPLIER == 0.7f);

    printf("  PASS: Enforcer suppression multiplier correct (0.7)\n");
}

// ============================================================================
// All types+tiers config tests
// ============================================================================

void test_all_type_tier_configs() {
    printf("Testing get_service_config for all type+tier combinations...\n");

    // Verify all valid combinations return valid configs
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t tier = 1; tier <= SERVICE_TIER_COUNT; ++tier) {
            ServiceConfig cfg = get_service_config(
                static_cast<ServiceType>(t),
                static_cast<ServiceTier>(tier)
            );
            // Radius-based services (Enforcer, HazardResponse) have radius > 0
            // Global/capacity-based services (Medical, Education) have radius = 0
            ServiceType st = static_cast<ServiceType>(t);
            if (st == ServiceType::Enforcer || st == ServiceType::HazardResponse) {
                assert(cfg.base_radius > 0);
            } else {
                assert(cfg.base_radius == 0);
            }
            assert(cfg.base_effectiveness > 0);
            assert(cfg.footprint_width > 0);
            assert(cfg.footprint_height > 0);
        }
    }

    // Verify higher tiers have larger (or equal) radius and footprint
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        ServiceType type = static_cast<ServiceType>(t);
        ServiceConfig post    = get_service_config(type, ServiceTier::Post);
        ServiceConfig station = get_service_config(type, ServiceTier::Station);
        ServiceConfig nexus   = get_service_config(type, ServiceTier::Nexus);

        assert(station.base_radius >= post.base_radius);
        assert(nexus.base_radius >= station.base_radius);
        assert(station.footprint_width >= post.footprint_width);
        assert(nexus.footprint_width >= station.footprint_width);
    }

    printf("  PASS: All type+tier configs valid with correct tier progression\n");
}

void test_invalid_tier_returns_zero_config() {
    printf("Testing invalid tier returns zero config...\n");

    ServiceConfig cfg = get_service_config(ServiceType::Enforcer, static_cast<ServiceTier>(0));
    assert(cfg.base_radius == 0);
    assert(cfg.base_effectiveness == 0);
    assert(cfg.capacity == 0);
    assert(cfg.footprint_width == 0);
    assert(cfg.footprint_height == 0);

    printf("  PASS: Invalid tier returns zero config\n");
}

void test_validity_checks() {
    printf("Testing validity check functions...\n");

    // ServiceType validity
    assert(isValidServiceType(0));
    assert(isValidServiceType(1));
    assert(isValidServiceType(2));
    assert(isValidServiceType(3));
    assert(!isValidServiceType(4));
    assert(!isValidServiceType(255));

    // ServiceTier validity
    assert(isValidServiceTier(1));
    assert(isValidServiceTier(2));
    assert(isValidServiceTier(3));
    assert(!isValidServiceTier(0));
    assert(!isValidServiceTier(4));
    assert(!isValidServiceTier(255));

    printf("  PASS: Validity check functions correct\n");
}

int main() {
    printf("=== ServiceTypes Unit Tests (Epic 9, Tickets E9-001, E9-030) ===\n\n");

    test_service_type_enum_values();
    test_service_type_count();
    test_service_tier_enum_values();
    test_service_tier_count();
    test_service_type_to_string();
    test_service_type_from_string();
    test_service_tier_to_string();
    test_service_tier_from_string();
    test_enum_underlying_type_sizes();
    test_all_service_types_have_strings();
    test_all_service_tiers_have_strings();
    test_enforcer_post_config();
    test_enforcer_station_config();
    test_enforcer_nexus_config();
    test_enforcer_suppression_multiplier();
    test_all_type_tier_configs();
    test_invalid_tier_returns_zero_config();
    test_validity_checks();

    printf("\n=== All ServiceTypes Tests Passed ===\n");
    return 0;
}

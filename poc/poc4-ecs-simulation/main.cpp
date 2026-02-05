#include <entt/entt.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <vector>

// ---------------------------------------------------------------------------
// Timing helpers
// ---------------------------------------------------------------------------

using Clock = std::chrono::high_resolution_clock;

struct BenchResult {
    double min_ms;
    double max_ms;
    double avg_ms;
};

template <typename Fn>
BenchResult benchmark(Fn&& fn, int iterations = 100) {
    double total = 0.0;
    double min_val = 1e9;
    double max_val = 0.0;

    // Warm-up run
    fn();

    for (int i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        fn();
        auto end = Clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        total += ms;
        if (ms < min_val) min_val = ms;
        if (ms > max_val) max_val = ms;
    }

    return { min_val, max_val, total / iterations };
}

// ---------------------------------------------------------------------------
// Representative Components (from systems.yaml / components.yaml)
// All components are trivially copyable pure data structs
// ---------------------------------------------------------------------------

// Core spatial component - nearly all entities have this
struct PositionComponent {
    float x;
    float y;
    float z;
};
static_assert(sizeof(PositionComponent) == 12, "PositionComponent size check");

// Building ownership and state
struct BuildingComponent {
    uint32_t building_type;
    uint32_t owner_player_id;
    uint8_t  level;
    uint8_t  health;
    uint8_t  flags;
    uint8_t  padding;
};
static_assert(sizeof(BuildingComponent) == 12, "BuildingComponent size check");

// Energy system participation
struct EnergyComponent {
    int32_t consumption;  // Negative = produces
    int32_t capacity;
    uint8_t connected;
    uint8_t padding[3];
};
static_assert(sizeof(EnergyComponent) == 12, "EnergyComponent size check");

// Population for residential buildings
struct PopulationComponent {
    uint16_t current;
    uint16_t capacity;
    uint8_t  happiness;
    uint8_t  employment_rate;
    uint8_t  padding[2];
};
static_assert(sizeof(PopulationComponent) == 8, "PopulationComponent size check");

// Economic activity
struct TaxableComponent {
    int32_t income;
    int32_t tax_paid;
    uint8_t tax_bracket;
    uint8_t padding[3];
};
static_assert(sizeof(TaxableComponent) == 12, "TaxableComponent size check");

// Zone assignment
struct ZoneComponent {
    uint8_t zone_type;     // residential, commercial, industrial, etc.
    uint8_t density;       // low, medium, high
    uint8_t desirability;
    uint8_t padding;
};
static_assert(sizeof(ZoneComponent) == 4, "ZoneComponent size check");

// Transport network participation
struct TransportComponent {
    uint32_t road_connection_id;
    uint16_t traffic_load;
    uint8_t  accessibility;
    uint8_t  padding;
};
static_assert(sizeof(TransportComponent) == 8, "TransportComponent size check");

// Service coverage tracking
struct ServiceCoverageComponent {
    uint8_t police;
    uint8_t fire;
    uint8_t health;
    uint8_t education;
    uint8_t parks;
    uint8_t padding[3];
};
static_assert(sizeof(ServiceCoverageComponent) == 8, "ServiceCoverageComponent size check");

// ---------------------------------------------------------------------------
// Simple Event System (fire-and-forget pattern from patterns.yaml)
// ---------------------------------------------------------------------------

struct Event {
    uint32_t type;
    uint32_t entity_id;
    int32_t  data1;
    int32_t  data2;
};

class EventDispatcher {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(uint32_t event_type, Handler handler) {
        handlers_[event_type].push_back(std::move(handler));
    }

    void emit(const Event& event) {
        pending_.push_back(event);
    }

    void dispatch_all() {
        for (const auto& event : pending_) {
            auto it = handlers_.find(event.type);
            if (it != handlers_.end()) {
                for (auto& handler : it->second) {
                    handler(event);
                }
            }
        }
        pending_.clear();
    }

    size_t pending_count() const { return pending_.size(); }

private:
    std::unordered_map<uint32_t, std::vector<Handler>> handlers_;
    std::vector<Event> pending_;
};

// Event types
constexpr uint32_t EVENT_BUILDING_PLACED = 1;
constexpr uint32_t EVENT_BUILDING_DESTROYED = 2;
constexpr uint32_t EVENT_ZONE_CHANGED = 3;
constexpr uint32_t EVENT_POPULATION_CHANGED = 4;
constexpr uint32_t EVENT_ENERGY_UPDATED = 5;

// ---------------------------------------------------------------------------
// Simulation Systems (simplified versions for benchmark)
// ---------------------------------------------------------------------------

volatile int32_t sink = 0; // Prevent optimizer from eliminating work

// System 1: Energy System - updates energy consumption/production
void energy_system_tick(entt::registry& registry, float dt) {
    int32_t total_consumption = 0;
    int32_t total_production = 0;

    auto view = registry.view<EnergyComponent, BuildingComponent>();
    for (auto entity : view) {
        auto& energy = view.get<EnergyComponent>(entity);
        auto& building = view.get<BuildingComponent>(entity);

        if (energy.consumption < 0) {
            total_production -= energy.consumption;
        } else {
            total_consumption += energy.consumption;
        }

        // Simulate some work
        energy.connected = (total_production >= total_consumption) ? 1 : 0;
    }
    sink = total_consumption;
}

// System 2: Population System - updates population happiness/employment
void population_system_tick(entt::registry& registry, float dt) {
    int32_t total_pop = 0;

    auto view = registry.view<PopulationComponent, BuildingComponent, ServiceCoverageComponent>();
    for (auto entity : view) {
        auto& pop = view.get<PopulationComponent>(entity);
        auto& services = view.get<ServiceCoverageComponent>(entity);

        // Calculate happiness based on services
        int avg_service = (services.police + services.fire + services.health +
                          services.education + services.parks) / 5;
        pop.happiness = static_cast<uint8_t>(avg_service);

        total_pop += pop.current;
    }
    sink = total_pop;
}

// System 3: Economy System - calculates taxes
void economy_system_tick(entt::registry& registry, float dt) {
    int32_t total_tax = 0;

    auto view = registry.view<TaxableComponent, BuildingComponent, PopulationComponent>();
    for (auto entity : view) {
        auto& tax = view.get<TaxableComponent>(entity);
        auto& pop = view.get<PopulationComponent>(entity);

        // Calculate tax based on population and income
        tax.tax_paid = (tax.income * tax.tax_bracket) / 100;
        total_tax += tax.tax_paid;
    }
    sink = total_tax;
}

// System 4: Transport System - updates traffic
void transport_system_tick(entt::registry& registry, float dt) {
    uint32_t total_traffic = 0;

    auto view = registry.view<TransportComponent, PositionComponent>();
    for (auto entity : view) {
        auto& transport = view.get<TransportComponent>(entity);
        auto& pos = view.get<PositionComponent>(entity);

        // Simulate traffic calculation
        transport.traffic_load = static_cast<uint16_t>(
            (static_cast<uint32_t>(pos.x) + static_cast<uint32_t>(pos.y)) % 100
        );
        total_traffic += transport.traffic_load;
    }
    sink = total_traffic;
}

// System 5: Zone System - updates desirability
void zone_system_tick(entt::registry& registry, float dt) {
    auto view = registry.view<ZoneComponent, PositionComponent, ServiceCoverageComponent>();
    for (auto entity : view) {
        auto& zone = view.get<ZoneComponent>(entity);
        auto& services = view.get<ServiceCoverageComponent>(entity);

        // Desirability based on service coverage
        zone.desirability = static_cast<uint8_t>(
            (services.police + services.fire + services.health) / 3
        );
    }
}

// Full simulation tick - runs all systems in priority order
void simulation_tick(entt::registry& registry, EventDispatcher& events, float dt) {
    // Priority order matches systems.yaml
    zone_system_tick(registry, dt);        // priority 30
    energy_system_tick(registry, dt);      // priority 10 (but depends on buildings)
    transport_system_tick(registry, dt);   // priority 45
    population_system_tick(registry, dt);  // priority 50
    economy_system_tick(registry, dt);     // priority 60

    // Dispatch any events generated during tick
    events.dispatch_all();
}

// ---------------------------------------------------------------------------
// Benchmark: Entity Creation and Component Assignment
// ---------------------------------------------------------------------------

void create_entities(entt::registry& registry, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        auto entity = registry.create();

        // All entities get position
        registry.emplace<PositionComponent>(entity, PositionComponent{
            static_cast<float>(i % 512),
            static_cast<float>(i / 512),
            0.0f
        });

        // 80% are buildings
        if (i % 5 != 0) {
            registry.emplace<BuildingComponent>(entity, BuildingComponent{
                i % 10,                              // building_type
                i % 4,                               // owner_player_id
                static_cast<uint8_t>(1 + (i % 3)),   // level
                100,                                 // health
                0,                                   // flags
                0                                    // padding
            });

            registry.emplace<EnergyComponent>(entity, EnergyComponent{
                static_cast<int32_t>((i % 100) - 20),  // consumption (some negative = producers)
                100,                                    // capacity
                1,                                      // connected
                {0, 0, 0}                               // padding
            });

            registry.emplace<ZoneComponent>(entity, ZoneComponent{
                static_cast<uint8_t>(i % 4),   // zone_type
                static_cast<uint8_t>(i % 3),   // density
                50,                             // desirability
                0                               // padding
            });

            registry.emplace<ServiceCoverageComponent>(entity, ServiceCoverageComponent{
                static_cast<uint8_t>(50 + (i % 50)),  // police
                static_cast<uint8_t>(50 + (i % 50)),  // fire
                static_cast<uint8_t>(50 + (i % 50)),  // health
                static_cast<uint8_t>(50 + (i % 50)),  // education
                static_cast<uint8_t>(50 + (i % 50)),  // parks
                {0, 0, 0}                              // padding
            });

            // 60% of buildings have population (residential)
            if (i % 5 < 3) {
                registry.emplace<PopulationComponent>(entity, PopulationComponent{
                    static_cast<uint16_t>(10 + (i % 90)),  // current
                    100,                                    // capacity
                    75,                                     // happiness
                    80,                                     // employment_rate
                    {0, 0}                                  // padding
                });

                registry.emplace<TaxableComponent>(entity, TaxableComponent{
                    static_cast<int32_t>(1000 + (i % 9000)),  // income
                    0,                                         // tax_paid
                    static_cast<uint8_t>(10 + (i % 20)),       // tax_bracket
                    {0, 0, 0}                                  // padding
                });
            }

            // 50% have transport connections
            if (i % 2 == 0) {
                registry.emplace<TransportComponent>(entity, TransportComponent{
                    i,                           // road_connection_id
                    0,                           // traffic_load
                    50,                          // accessibility
                    0                            // padding
                });
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Benchmark: Event Dispatch
// ---------------------------------------------------------------------------

BenchResult bench_event_dispatch(int event_count) {
    EventDispatcher dispatcher;

    // Subscribe handlers (5 event types, 2 handlers each)
    int handler_calls = 0;
    for (uint32_t type = 1; type <= 5; ++type) {
        dispatcher.subscribe(type, [&handler_calls](const Event& e) {
            handler_calls++;
        });
        dispatcher.subscribe(type, [&handler_calls](const Event& e) {
            handler_calls++;
        });
    }

    return benchmark([&]() {
        handler_calls = 0;

        // Emit events
        for (int i = 0; i < event_count; ++i) {
            dispatcher.emit(Event{
                static_cast<uint32_t>(1 + (i % 5)),  // type
                static_cast<uint32_t>(i),            // entity_id
                i,                                    // data1
                i * 2                                 // data2
            });
        }

        // Dispatch all
        dispatcher.dispatch_all();
        sink = handler_calls;
    }, 100);
}

// ---------------------------------------------------------------------------
// Benchmark: Component Queries
// ---------------------------------------------------------------------------

BenchResult bench_query_3_components(entt::registry& registry) {
    return benchmark([&]() {
        int32_t count = 0;
        auto view = registry.view<PositionComponent, BuildingComponent, EnergyComponent>();
        for (auto entity : view) {
            auto& pos = view.get<PositionComponent>(entity);
            auto& building = view.get<BuildingComponent>(entity);
            auto& energy = view.get<EnergyComponent>(entity);
            count += static_cast<int32_t>(pos.x) + building.level + energy.consumption;
        }
        sink = count;
    }, 100);
}

BenchResult bench_query_5_components(entt::registry& registry) {
    return benchmark([&]() {
        int32_t count = 0;
        auto view = registry.view<PositionComponent, BuildingComponent, EnergyComponent,
                                   ZoneComponent, ServiceCoverageComponent>();
        for (auto entity : view) {
            auto& pos = view.get<PositionComponent>(entity);
            auto& zone = view.get<ZoneComponent>(entity);
            count += static_cast<int32_t>(pos.x) + zone.desirability;
        }
        sink = count;
    }, 100);
}

// ---------------------------------------------------------------------------
// Memory Measurement
// ---------------------------------------------------------------------------

struct MemoryStats {
    size_t total_bytes;
    size_t entity_count;
    double bytes_per_entity;
};

MemoryStats measure_memory(entt::registry& registry) {
    // EnTT doesn't expose internal memory directly, so we estimate based on
    // component sizes and entity count

    size_t entity_count = registry.storage<entt::entity>().size();

    // Count components
    size_t pos_count = registry.storage<PositionComponent>().size();
    size_t building_count = registry.storage<BuildingComponent>().size();
    size_t energy_count = registry.storage<EnergyComponent>().size();
    size_t pop_count = registry.storage<PopulationComponent>().size();
    size_t tax_count = registry.storage<TaxableComponent>().size();
    size_t zone_count = registry.storage<ZoneComponent>().size();
    size_t transport_count = registry.storage<TransportComponent>().size();
    size_t service_count = registry.storage<ServiceCoverageComponent>().size();

    // Calculate total component memory
    size_t component_bytes =
        pos_count * sizeof(PositionComponent) +
        building_count * sizeof(BuildingComponent) +
        energy_count * sizeof(EnergyComponent) +
        pop_count * sizeof(PopulationComponent) +
        tax_count * sizeof(TaxableComponent) +
        zone_count * sizeof(ZoneComponent) +
        transport_count * sizeof(TransportComponent) +
        service_count * sizeof(ServiceCoverageComponent);

    // Add entity storage overhead (EnTT uses ~4 bytes per entity for ID)
    size_t entity_overhead = entity_count * sizeof(entt::entity);

    // Add sparse set overhead (approximately 8 bytes per component instance for indexing)
    size_t sparse_overhead = (pos_count + building_count + energy_count + pop_count +
                              tax_count + zone_count + transport_count + service_count) * 8;

    size_t total = component_bytes + entity_overhead + sparse_overhead;

    return {
        total,
        entity_count,
        static_cast<double>(total) / entity_count
    };
}

// ---------------------------------------------------------------------------
// Pass/Fail helpers
// ---------------------------------------------------------------------------

const char* pass_fail(double value, double target, double failure) {
    if (value <= target) return "PASS";
    if (value <= failure) return "WARN";
    return "FAIL";
}

const char* pass_fail_mem(double bytes, double target, double failure) {
    if (bytes <= target) return "PASS";
    if (bytes <= failure) return "WARN";
    return "FAIL";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    printf("=============================================================\n");
    printf("  POC-4: ECS Simulation Loop (EnTT)\n");
    printf("=============================================================\n\n");

    const uint32_t entity_counts[] = { 10000, 25000, 50000 };
    const int num_counts = 3;

    for (int c = 0; c < num_counts; ++c) {
        uint32_t count = entity_counts[c];
        bool is_target = (count == 50000);

        printf("-------------------------------------------------------------\n");
        printf("  Entity Count: %u\n", count);
        printf("-------------------------------------------------------------\n\n");

        // Create registry and populate
        entt::registry registry;
        create_entities(registry, count);

        EventDispatcher events;
        // Subscribe some handlers
        events.subscribe(EVENT_BUILDING_PLACED, [](const Event&) {});
        events.subscribe(EVENT_ENERGY_UPDATED, [](const Event&) {});

        // Benchmark 1: Full Simulation Tick
        auto tick_result = benchmark([&]() {
            simulation_tick(registry, events, 0.05f);  // 50ms = 20 ticks/sec
        }, 100);

        printf("  [1] Full Simulation Tick (5 systems)\n");
        printf("      Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms",
               tick_result.min_ms, tick_result.avg_ms, tick_result.max_ms);
        if (is_target) {
            printf("  [%s]", pass_fail(tick_result.avg_ms, 25.0, 50.0));
        }
        printf("\n\n");

        // Benchmark 2: Event Dispatch (1000 events)
        auto event_result = bench_event_dispatch(1000);
        printf("  [2] Event Dispatch (1000 events, 2 handlers each)\n");
        printf("      Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms",
               event_result.min_ms, event_result.avg_ms, event_result.max_ms);
        if (is_target) {
            printf("  [%s]", pass_fail(event_result.avg_ms, 1.0, 5.0));
        }
        printf("\n\n");

        // Benchmark 3: Component Queries
        auto query3_result = bench_query_3_components(registry);
        printf("  [3] Query (3 components: Position, Building, Energy)\n");
        printf("      Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms",
               query3_result.min_ms, query3_result.avg_ms, query3_result.max_ms);
        if (is_target) {
            printf("  [%s]", pass_fail(query3_result.avg_ms, 1.0, 5.0));
        }
        printf("\n\n");

        auto query5_result = bench_query_5_components(registry);
        printf("  [4] Query (5 components: Position, Building, Energy, Zone, Service)\n");
        printf("      Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms\n\n",
               query5_result.min_ms, query5_result.avg_ms, query5_result.max_ms);

        // Benchmark 4: Memory Usage
        auto mem = measure_memory(registry);
        printf("  [5] Memory Usage\n");
        printf("      Total: %zu bytes (%.2f MB)\n", mem.total_bytes, mem.total_bytes / (1024.0 * 1024.0));
        printf("      Entities: %zu\n", mem.entity_count);
        printf("      Bytes/entity: %.2f", mem.bytes_per_entity);
        if (is_target) {
            printf("  [%s]", pass_fail_mem(mem.bytes_per_entity, 64.0, 128.0));
        }
        printf("\n\n");
    }

    // Final summary
    printf("=============================================================\n");
    printf("  POC-4 Target Thresholds (50K entities)\n");
    printf("=============================================================\n");
    printf("  Metric                    | Target   | Failure\n");
    printf("  --------------------------+----------+---------\n");
    printf("  Total tick time           | <= 25ms  | > 50ms\n");
    printf("  Event dispatch (1000)     | <= 1ms   | > 5ms\n");
    printf("  Query time (3 components) | <= 1ms   | > 5ms\n");
    printf("  Memory per entity         | <= 64 B  | > 128 B\n");
    printf("=============================================================\n");

    return 0;
}

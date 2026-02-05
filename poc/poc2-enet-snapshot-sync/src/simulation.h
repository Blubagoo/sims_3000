#pragma once

#include "entity_store.h"
#include <cstdint>

class Simulation {
public:
    explicit Simulation(EntityStore& store, uint32_t seed = 42);

    // Run one simulation tick: mutate ~2% of entities, mark dirty
    void tick();

    uint32_t current_tick() const { return tick_count_; }

private:
    EntityStore& store_;
    uint32_t rng_state_;
    uint32_t tick_count_ = 0;

    uint32_t next_rng();
    float next_float();
};

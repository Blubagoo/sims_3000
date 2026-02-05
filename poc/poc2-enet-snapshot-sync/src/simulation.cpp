#include "simulation.h"

Simulation::Simulation(EntityStore& store, uint32_t seed)
    : store_(store)
    , rng_state_(seed)
{
}

uint32_t Simulation::next_rng() {
    rng_state_ ^= rng_state_ << 13;
    rng_state_ ^= rng_state_ >> 17;
    rng_state_ ^= rng_state_ << 5;
    return rng_state_;
}

float Simulation::next_float() {
    return static_cast<float>(next_rng() % 10000) / 100.0f;
}

void Simulation::tick() {
    store_.clear_dirty();
    tick_count_++;

    uint32_t entity_count = store_.count();
    // Mutate ~2% of entities per tick
    uint32_t mutations = entity_count / 50;

    for (uint32_t m = 0; m < mutations; ++m) {
        uint32_t entity_id = next_rng() % entity_count;
        // Pick which fields to mutate (1-3 fields)
        uint32_t field_roll = next_rng() % 100;

        uint8_t dirty_mask = 0;

        if (field_roll < 60) {
            // 60%: position change only
            uint32_t axis = next_rng() % 3;
            float delta = (static_cast<float>(next_rng() % 200) - 100.0f) / 100.0f;
            auto& pos = store_.position(entity_id);
            switch (axis) {
                case 0: pos.x += delta; dirty_mask |= FIELD_POS_X; break;
                case 1: pos.y += delta; dirty_mask |= FIELD_POS_Y; break;
                case 2: pos.z += delta; dirty_mask |= FIELD_POS_Z; break;
            }
        } else if (field_roll < 85) {
            // 25%: position + value change
            auto& pos = store_.position(entity_id);
            pos.x += (static_cast<float>(next_rng() % 200) - 100.0f) / 100.0f;
            pos.z += (static_cast<float>(next_rng() % 200) - 100.0f) / 100.0f;
            dirty_mask |= FIELD_POS_X | FIELD_POS_Z;

            auto& dat = store_.data(entity_id);
            dat.value = next_float();
            dirty_mask |= FIELD_VALUE;
        } else {
            // 15%: flags change
            auto& dat = store_.data(entity_id);
            dat.flags = next_rng() % 256;
            dirty_mask |= FIELD_FLAGS;
        }

        store_.mark_dirty(entity_id, dirty_mask);
    }
}

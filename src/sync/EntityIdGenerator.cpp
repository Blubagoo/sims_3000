/**
 * @file EntityIdGenerator.cpp
 * @brief Implementation of EntityIdGenerator.
 *
 * Most methods are inline in the header. This file exists for:
 * - Compile-time verification of the header
 * - Future expansion if needed (e.g., persistence callbacks)
 */

#include "sims3000/sync/EntityIdGenerator.h"

namespace sims3000 {

// All methods are inline in the header.
// This file ensures the header compiles correctly and provides a place
// for future non-inline implementations if needed.

// Static assertions to verify assumptions
static_assert(NULL_ENTITY_ID == 0, "NULL_ENTITY_ID must be 0");
static_assert(sizeof(EntityID) == 4, "EntityID must be 4 bytes");

} // namespace sims3000

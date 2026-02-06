/**
 * @file SyncSystem.h
 * @brief Change detection system for network synchronization.
 *
 * SyncSystem tracks dirty entities using EnTT signals (on_construct, on_update,
 * on_destroy). It maintains a set of entities that have changed since the last
 * sync, along with the type of change (created, updated, destroyed).
 *
 * Key design:
 * - Subscribes to component modification signals automatically
 * - O(1) per change via dirty flag pattern (not full state diffing)
 * - Respects SyncPolicy metadata (components with SyncPolicy::None are excluded)
 * - Dirty set is cleared after delta generation via flush()
 *
 * IMPORTANT: For on_update signals to fire, modifications MUST use
 * registry.patch() or registry.replace(). Direct member access does NOT
 * trigger signals.
 *
 * Usage:
 *   SyncSystem sync(registry);
 *   sync.subscribeAll();  // Subscribe to all syncable component types
 *
 *   // ... simulation tick modifies components via registry.patch() ...
 *
 *   // Generate delta from dirty entities
 *   const auto& dirty = sync.getDirtyEntities();
 *   for (const auto& [entity, change] : dirty) {
 *       // Serialize entity based on change type
 *   }
 *
 *   sync.flush();  // Clear dirty set after delta generation
 */

#ifndef SIMS3000_SYNC_SYNCSYSTEM_H
#define SIMS3000_SYNC_SYNCSYSTEM_H

#include "sims3000/core/ISimulatable.h"
#include "sims3000/core/Serialization.h"
#include "sims3000/core/types.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/net/NetworkBuffer.h"

#include <entt/entt.hpp>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <memory>
#include <atomic>
#include <future>
#include <deque>
#include <mutex>

namespace sims3000 {

// Forward declarations
class StateUpdateMessage;
class SnapshotStartMessage;
class SnapshotChunkMessage;
class SnapshotEndMessage;

/**
 * @enum ChangeType
 * @brief Type of change detected for an entity.
 */
enum class ChangeType : std::uint8_t {
    Created = 1,    ///< Entity was created (new entity or component added)
    Updated = 2,    ///< Entity's component(s) were modified
    Destroyed = 3   ///< Entity was destroyed
};

/**
 * @enum DeltaApplicationResult
 * @brief Result of applying a delta to the client registry.
 */
enum class DeltaApplicationResult : std::uint8_t {
    Applied = 0,      ///< Delta was applied successfully
    Duplicate = 1,    ///< Delta was a duplicate (already processed)
    OutOfOrder = 2,   ///< Delta was out of order (older than last processed)
    Error = 3         ///< Error occurred during application
};

/**
 * @enum SnapshotState
 * @brief State of snapshot transfer on the client.
 */
enum class SnapshotState : std::uint8_t {
    None = 0,         ///< No snapshot in progress
    Receiving = 1,    ///< Receiving snapshot chunks
    Applying = 2,     ///< Applying snapshot to registry
    Complete = 3      ///< Snapshot complete
};

/**
 * @struct SnapshotProgress
 * @brief Progress information for snapshot transfer.
 */
struct SnapshotProgress {
    SimulationTick tick = 0;           ///< Tick when snapshot was taken
    std::uint32_t totalChunks = 0;     ///< Total chunks expected
    std::uint32_t receivedChunks = 0;  ///< Chunks received so far
    std::uint32_t totalBytes = 0;      ///< Total uncompressed bytes
    std::uint32_t entityCount = 0;     ///< Entities in snapshot
    SnapshotState state = SnapshotState::None;

    float getProgress() const {
        if (totalChunks == 0) return 0.0f;
        return static_cast<float>(receivedChunks) / static_cast<float>(totalChunks);
    }
};

/// Maximum delta updates to buffer during snapshot transfer (100 ticks = 5 seconds at 20Hz)
constexpr std::size_t MAX_BUFFERED_DELTAS = 100;

/**
 * @struct EntityChange
 * @brief Tracks change information for a dirty entity.
 */
struct EntityChange {
    ChangeType type = ChangeType::Updated;
    std::uint32_t componentMask = 0;  ///< Bitmask of changed component type IDs

    /**
     * @brief Mark a component as changed.
     * @param componentTypeId The component's type ID from ComponentTypeID namespace.
     */
    void markComponent(std::uint8_t componentTypeId) {
        if (componentTypeId < 32) {
            componentMask |= (1u << componentTypeId);
        }
    }

    /**
     * @brief Check if a component type was changed.
     * @param componentTypeId The component's type ID to check.
     * @return true if the component was changed.
     */
    bool hasComponent(std::uint8_t componentTypeId) const {
        if (componentTypeId >= 32) return false;
        return (componentMask & (1u << componentTypeId)) != 0;
    }
};

/**
 * @class SyncSystem
 * @brief Tracks dirty entities for network synchronization.
 *
 * Subscribes to EnTT component signals and maintains a dirty set of entities
 * that have changed since the last flush.
 */
class SyncSystem : public ISimulatable {
public:
    /**
     * @brief Construct a SyncSystem.
     * @param registry Reference to the ECS registry to monitor.
     */
    explicit SyncSystem(Registry& registry);

    ~SyncSystem() override;

    // Non-copyable, non-movable (due to signal connections)
    SyncSystem(const SyncSystem&) = delete;
    SyncSystem& operator=(const SyncSystem&) = delete;
    SyncSystem(SyncSystem&&) = delete;
    SyncSystem& operator=(SyncSystem&&) = delete;

    // =========================================================================
    // ISimulatable interface
    // =========================================================================

    /**
     * @brief Called each simulation tick.
     *
     * SyncSystem's tick() is a no-op. Change detection happens via signals,
     * and delta generation is triggered by the network layer calling
     * getDirtyEntities() and flush().
     */
    void tick(const ISimulationTime& time) override;

    /**
     * @brief Get system priority.
     *
     * SyncSystem runs after all simulation systems to ensure all changes
     * are captured. High priority number = runs later.
     */
    int getPriority() const override { return 900; }

    /**
     * @brief Get system name.
     */
    const char* getName() const override { return "SyncSystem"; }

    // =========================================================================
    // Signal subscription
    // =========================================================================

    /**
     * @brief Subscribe to signals for a specific component type.
     *
     * Automatically skips components with SyncPolicy::None.
     *
     * @tparam T Component type to subscribe to.
     */
    template<typename T>
    void subscribe() {
        // Skip components with SyncPolicy::None
        if constexpr (ComponentMeta<T>::syncPolicy == SyncPolicy::None) {
            return;
        }

        auto& raw = m_registry.raw();

        // on_construct - entity created or component added
        raw.on_construct<T>().template connect<&SyncSystem::onConstruct<T>>(this);

        // on_update - component modified via patch() or replace()
        raw.on_update<T>().template connect<&SyncSystem::onUpdate<T>>(this);

        // on_destroy - component removed or entity destroyed
        raw.on_destroy<T>().template connect<&SyncSystem::onDestroy<T>>(this);

        m_subscribedTypes.insert(typeid(T).hash_code());
    }

    /**
     * @brief Subscribe to all known syncable component types.
     *
     * Call this after construction to enable change detection for all
     * component types defined in Components.h.
     */
    void subscribeAll();

    /**
     * @brief Unsubscribe from all signals.
     *
     * Called automatically in destructor.
     */
    void unsubscribeAll();

    // =========================================================================
    // Dirty entity access
    // =========================================================================

    /**
     * @brief Get all dirty entities and their change types.
     * @return Const reference to the dirty entities map.
     */
    const std::unordered_map<EntityID, EntityChange>& getDirtyEntities() const {
        return m_dirtyEntities;
    }

    /**
     * @brief Get entities that were created since last flush.
     * @return Set of created entity IDs.
     */
    std::unordered_set<EntityID> getCreatedEntities() const;

    /**
     * @brief Get entities that were updated (but not created) since last flush.
     * @return Set of updated entity IDs.
     */
    std::unordered_set<EntityID> getUpdatedEntities() const;

    /**
     * @brief Get entities that were destroyed since last flush.
     * @return Set of destroyed entity IDs.
     */
    std::unordered_set<EntityID> getDestroyedEntities() const;

    /**
     * @brief Check if an entity is dirty.
     * @param entity Entity to check.
     * @return true if the entity has pending changes.
     */
    bool isDirty(EntityID entity) const {
        return m_dirtyEntities.find(entity) != m_dirtyEntities.end();
    }

    /**
     * @brief Get the change type for a specific entity.
     * @param entity Entity to query.
     * @return EntityChange struct, or default (Updated, mask=0) if not dirty.
     */
    EntityChange getChange(EntityID entity) const {
        auto it = m_dirtyEntities.find(entity);
        if (it != m_dirtyEntities.end()) {
            return it->second;
        }
        return EntityChange{};
    }

    /**
     * @brief Get total number of dirty entities.
     */
    std::size_t getDirtyCount() const { return m_dirtyEntities.size(); }

    /**
     * @brief Clear the dirty set after delta generation.
     *
     * MUST be called after generating and sending the delta to reset
     * tracking for the next tick.
     */
    void flush();

    // =========================================================================
    // Delta Generation (Server-side)
    // =========================================================================

    /**
     * @brief Generate a StateUpdateMessage from dirty entities.
     *
     * Builds a delta message containing all entities that have changed since
     * the last flush. The message includes:
     * - Created entities with all their syncable components
     * - Updated entities with only their changed components
     * - Destroyed entities (just the entity ID)
     *
     * @param tick The current simulation tick number.
     * @return Populated StateUpdateMessage ready for transmission.
     *
     * @note Call flush() after sending the message to clear the dirty set.
     */
    std::unique_ptr<StateUpdateMessage> generateDelta(SimulationTick tick);

    // =========================================================================
    // Delta Application (Client-side)
    // =========================================================================

    /**
     * @brief Apply a StateUpdateMessage to the local registry.
     *
     * Processes the delta message and updates the local ECS registry:
     * - Creates new entities with server-assigned IDs
     * - Updates existing entity components
     * - Destroys entities as directed
     *
     * Handles out-of-order and duplicate messages by comparing tick numbers.
     *
     * @param message The StateUpdateMessage to apply.
     * @return Result indicating success, duplicate, out-of-order, or error.
     */
    DeltaApplicationResult applyDelta(const StateUpdateMessage& message);

    /**
     * @brief Get the last processed tick number (client-side).
     * @return The tick number of the last successfully applied delta.
     */
    SimulationTick getLastProcessedTick() const { return m_lastProcessedTick; }

    /**
     * @brief Reset the last processed tick (for reconnection scenarios).
     * @param tick The tick to reset to (usually 0).
     */
    void resetLastProcessedTick(SimulationTick tick = 0) { m_lastProcessedTick = tick; }

    /**
     * @brief Manually mark an entity as dirty.
     *
     * Useful for forcing sync of entities that weren't modified through
     * the normal component update path.
     *
     * @param entity Entity to mark dirty.
     * @param type Type of change (defaults to Updated).
     */
    void markDirty(EntityID entity, ChangeType type = ChangeType::Updated);

    /**
     * @brief Manually mark a specific component on an entity as dirty.
     *
     * @param entity Entity to mark dirty.
     * @param componentTypeId Component type ID from ComponentTypeID namespace.
     * @param type Type of change (defaults to Updated).
     */
    void markComponentDirty(EntityID entity, std::uint8_t componentTypeId,
                            ChangeType type = ChangeType::Updated);

    // =========================================================================
    // Full State Snapshot Generation (Server-side)
    // =========================================================================

    /**
     * @brief Start asynchronous snapshot generation.
     *
     * Generates a complete snapshot of all entities with SyncPolicy != None.
     * Uses copy-on-write pattern to ensure consistency during generation.
     * The snapshot is generated in a background thread, non-blocking.
     *
     * @param tick The simulation tick for the snapshot.
     * @return true if snapshot generation started, false if already in progress.
     */
    bool startSnapshotGeneration(SimulationTick tick);

    /**
     * @brief Check if snapshot generation is in progress.
     * @return true if generating, false otherwise.
     */
    bool isSnapshotGenerating() const { return m_snapshotGenerating.load(); }

    /**
     * @brief Check if snapshot generation is complete and data is ready.
     * @return true if snapshot data is available.
     */
    bool isSnapshotReady() const;

    /**
     * @brief Get the generated snapshot messages (SnapshotStart, chunks, SnapshotEnd).
     *
     * Call after isSnapshotReady() returns true.
     * Clears the snapshot data after retrieval.
     *
     * @param outStart Output: SnapshotStartMessage
     * @param outChunks Output: Vector of SnapshotChunkMessages
     * @param outEnd Output: SnapshotEndMessage
     * @return true if snapshot was retrieved, false if not ready.
     */
    bool getSnapshotMessages(SnapshotStartMessage& outStart,
                             std::vector<SnapshotChunkMessage>& outChunks,
                             SnapshotEndMessage& outEnd);

    /**
     * @brief Notify that a component was modified during snapshot generation.
     *
     * Called by the simulation when a component is modified while a snapshot
     * is being generated. Implements copy-on-write by storing the old value.
     *
     * @param entity Entity being modified.
     * @param componentTypeId Component type ID.
     * @param oldData Serialized old component data.
     */
    void notifySnapshotCOW(EntityID entity, std::uint8_t componentTypeId,
                           const std::vector<std::uint8_t>& oldData);

    // =========================================================================
    // Full State Snapshot Reception (Client-side)
    // =========================================================================

    /**
     * @brief Handle SnapshotStartMessage from server.
     *
     * Initializes snapshot reception state and prepares buffers.
     *
     * @param message The SnapshotStartMessage.
     */
    void handleSnapshotStart(const SnapshotStartMessage& message);

    /**
     * @brief Handle SnapshotChunkMessage from server.
     *
     * Buffers the chunk data. Chunks may arrive out of order.
     *
     * @param message The SnapshotChunkMessage.
     */
    void handleSnapshotChunk(const SnapshotChunkMessage& message);

    /**
     * @brief Handle SnapshotEndMessage from server.
     *
     * Verifies checksum and triggers snapshot application.
     *
     * @param message The SnapshotEndMessage.
     * @return true if snapshot was applied successfully, false on error.
     */
    bool handleSnapshotEnd(const SnapshotEndMessage& message);

    /**
     * @brief Buffer a delta update during snapshot reception.
     *
     * Deltas received while receiving a snapshot are buffered and applied
     * after the snapshot is complete.
     *
     * @param message The StateUpdateMessage to buffer.
     * @return true if buffered, false if buffer is full (overflow).
     */
    bool bufferDeltaDuringSnapshot(const StateUpdateMessage& message);

    /**
     * @brief Check if currently receiving a snapshot.
     * @return true if in snapshot reception mode.
     */
    bool isReceivingSnapshot() const { return m_snapshotProgress.state == SnapshotState::Receiving; }

    /**
     * @brief Get current snapshot reception progress.
     * @return SnapshotProgress struct with current state.
     */
    const SnapshotProgress& getSnapshotProgress() const { return m_snapshotProgress; }

    /**
     * @brief Clear local ECS state in preparation for snapshot.
     *
     * Destroys all entities except system entities (if any).
     */
    void clearLocalState();

private:
    // SFINAE helper to check if T has get_type_id()
    template<typename T, typename = void>
    struct has_get_type_id : std::false_type {};

    template<typename T>
    struct has_get_type_id<T, std::void_t<decltype(T::get_type_id())>> : std::true_type {};

    // Helper to mark component in change (with SFINAE)
    template<typename T>
    void markComponentIfHasTypeId(EntityChange& change, std::true_type) {
        change.markComponent(T::get_type_id());
    }

    template<typename T>
    void markComponentIfHasTypeId(EntityChange& /*change*/, std::false_type) {
        // Component has no type ID - skip marking
    }

    // Signal handlers (static member function templates for EnTT connection)
    template<typename T>
    void onConstruct(entt::registry& reg, entt::entity entity) {
        (void)reg; // Unused
        EntityID id = static_cast<EntityID>(entity);

        auto& change = m_dirtyEntities[id];
        // Only set Created if not already tracking this entity
        if (change.componentMask == 0) {
            change.type = ChangeType::Created;
        }

        // Mark the specific component that was added
        markComponentIfHasTypeId<T>(change, has_get_type_id<T>{});
    }

    template<typename T>
    void onUpdate(entt::registry& reg, entt::entity entity) {
        (void)reg;
        EntityID id = static_cast<EntityID>(entity);

        auto& change = m_dirtyEntities[id];
        // Don't downgrade Created to Updated
        if (change.type != ChangeType::Created) {
            change.type = ChangeType::Updated;
        }

        // Mark the specific component that was updated
        markComponentIfHasTypeId<T>(change, has_get_type_id<T>{});
    }

    template<typename T>
    void onDestroy(entt::registry& reg, entt::entity entity) {
        (void)reg;
        EntityID id = static_cast<EntityID>(entity);

        // Destroyed overrides all other states
        m_dirtyEntities[id].type = ChangeType::Destroyed;
        // Clear component mask - entity is being destroyed
        m_dirtyEntities[id].componentMask = 0;
    }

    // =========================================================================
    // Component Serialization Helpers (for delta generation)
    // =========================================================================

    /**
     * @brief Serialize all syncable components of an entity to a buffer.
     *
     * Used for Created entities to include all components.
     *
     * @param entity Entity to serialize.
     * @param buffer Output buffer for serialized component data.
     */
    void serializeAllComponents(EntityID entity, std::vector<std::uint8_t>& buffer);

    /**
     * @brief Serialize only changed components of an entity to a buffer.
     *
     * Uses the componentMask to only serialize components that changed.
     *
     * @param entity Entity to serialize.
     * @param componentMask Bitmask of changed component type IDs.
     * @param buffer Output buffer for serialized component data.
     */
    void serializeChangedComponents(EntityID entity, std::uint32_t componentMask,
                                    std::vector<std::uint8_t>& buffer);

    /**
     * @brief Deserialize and apply components to an entity.
     *
     * @param entity Entity to apply components to.
     * @param componentData Serialized component data.
     * @param isNewEntity True if this is a newly created entity.
     * @return true if deserialization succeeded.
     */
    bool deserializeAndApplyComponents(EntityID entity,
                                       const std::vector<std::uint8_t>& componentData,
                                       bool isNewEntity);

    // =========================================================================
    // Snapshot Generation Helpers (Server-side)
    // =========================================================================

    /**
     * @brief Generate snapshot data synchronously (called from background thread).
     *
     * Serializes all entities with syncable components.
     */
    void generateSnapshotData();

    /**
     * @brief Serialize all entities to snapshot buffer.
     *
     * @param buffer Output buffer for serialized snapshot data.
     * @return Number of entities serialized.
     */
    std::uint32_t serializeAllEntities(std::vector<std::uint8_t>& buffer);

    /**
     * @brief Calculate CRC32 checksum of data.
     *
     * @param data Data to checksum.
     * @return CRC32 checksum value.
     */
    static std::uint32_t calculateCRC32(const std::vector<std::uint8_t>& data);

    // =========================================================================
    // Snapshot Application Helpers (Client-side)
    // =========================================================================

    /**
     * @brief Apply buffered snapshot data to registry.
     *
     * @return true if successful, false on error.
     */
    bool applySnapshotToRegistry();

    /**
     * @brief Apply buffered deltas after snapshot application.
     *
     * Applies deltas in tick order, skipping those before snapshot tick.
     */
    void applyBufferedDeltas();

    Registry& m_registry;
    std::unordered_map<EntityID, EntityChange> m_dirtyEntities;
    std::unordered_set<std::size_t> m_subscribedTypes;  // hash_code of subscribed types
    SimulationTick m_lastProcessedTick = 0;  // Client-side: last applied tick

    // =========================================================================
    // Snapshot Generation State (Server-side)
    // =========================================================================
    std::atomic<bool> m_snapshotGenerating{false};
    std::atomic<bool> m_snapshotReady{false};
    std::future<void> m_snapshotFuture;
    SimulationTick m_snapshotTick = 0;

    // Snapshot output (protected by m_snapshotMutex)
    mutable std::mutex m_snapshotMutex;
    std::vector<std::uint8_t> m_snapshotData;           // Uncompressed serialized data
    std::vector<std::uint8_t> m_snapshotCompressed;     // LZ4 compressed data
    std::uint32_t m_snapshotEntityCount = 0;
    std::uint32_t m_snapshotChecksum = 0;

    // Copy-on-write buffer for snapshot consistency (entityId -> (componentTypeId -> oldData))
    std::unordered_map<EntityID, std::unordered_map<std::uint8_t, std::vector<std::uint8_t>>> m_cowBuffer;
    std::mutex m_cowMutex;

    // =========================================================================
    // Snapshot Reception State (Client-side)
    // =========================================================================
    SnapshotProgress m_snapshotProgress;
    std::vector<std::vector<std::uint8_t>> m_snapshotChunks;  // Indexed by chunkIndex
    std::deque<std::unique_ptr<StateUpdateMessage>> m_bufferedDeltas;
    std::mutex m_deltaBufferMutex;
};

} // namespace sims3000

#endif // SIMS3000_SYNC_SYNCSYSTEM_H

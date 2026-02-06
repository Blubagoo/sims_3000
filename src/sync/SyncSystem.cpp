/**
 * @file SyncSystem.cpp
 * @brief Implementation of change detection for network synchronization.
 */

#include "sims3000/sync/SyncSystem.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/core/Logger.h"
#include <algorithm>

namespace sims3000 {

// CRC32 lookup table (IEEE 802.3 polynomial)
static const std::uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd706b3, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

SyncSystem::SyncSystem(Registry& registry)
    : m_registry(registry)
{
    // Reserve space for typical dirty entity counts
    m_dirtyEntities.reserve(256);
}

SyncSystem::~SyncSystem() {
    unsubscribeAll();
}

void SyncSystem::tick(const ISimulationTime& /*time*/) {
    // No-op: Change detection happens via signals.
    // Delta generation is triggered externally via getDirtyEntities() and flush().
}

void SyncSystem::subscribeAll() {
    // Subscribe to all syncable component types defined in Components.h
    // Components with SyncPolicy::None are automatically skipped by subscribe<T>()

    subscribe<PositionComponent>();
    subscribe<OwnershipComponent>();
    subscribe<TransformComponent>();
    subscribe<RenderComponent>();      // Will be skipped (SyncPolicy::None)
    subscribe<BuildingComponent>();
    subscribe<EnergyComponent>();
    subscribe<PopulationComponent>();
    subscribe<ZoneComponent>();
    subscribe<TransportComponent>();
    subscribe<ServiceCoverageComponent>();
    subscribe<TaxableComponent>();
}

void SyncSystem::unsubscribeAll() {
    auto& raw = m_registry.raw();

    // Disconnect all signal handlers for subscribed types.
    //
    // DESIGN NOTE: This function uses explicit per-type disconnection rather than
    // a template/variadic approach for several reasons:
    //
    // 1. EnTT signals require knowing the exact type at compile time. A type-erased
    //    approach would need a component type registry with virtual dispatch.
    //
    // 2. The component list is small (~11 types) and changes infrequently. The
    //    boilerplate cost is acceptable compared to the infrastructure needed
    //    for a generic solution.
    //
    // 3. Explicit code is easier to debug - you can set breakpoints on specific
    //    component disconnections.
    //
    // If the component count grows significantly (>20), consider implementing a
    // compile-time component list using std::tuple and fold expressions.

    // C++17 compatible: Use a helper macro to reduce repetition.
    // (C++20 would allow generic lambdas with explicit template params)
    #define UNSUBSCRIBE_TYPE(ComponentType) \
        if (m_subscribedTypes.count(typeid(ComponentType).hash_code())) { \
            raw.on_construct<ComponentType>().disconnect(this); \
            raw.on_update<ComponentType>().disconnect(this); \
            raw.on_destroy<ComponentType>().disconnect(this); \
        }

    UNSUBSCRIBE_TYPE(PositionComponent)
    UNSUBSCRIBE_TYPE(OwnershipComponent)
    UNSUBSCRIBE_TYPE(TransformComponent)
    UNSUBSCRIBE_TYPE(RenderComponent)  // SyncPolicy::None, but safe to attempt
    UNSUBSCRIBE_TYPE(BuildingComponent)
    UNSUBSCRIBE_TYPE(EnergyComponent)
    UNSUBSCRIBE_TYPE(PopulationComponent)
    UNSUBSCRIBE_TYPE(ZoneComponent)
    UNSUBSCRIBE_TYPE(TransportComponent)
    UNSUBSCRIBE_TYPE(ServiceCoverageComponent)
    UNSUBSCRIBE_TYPE(TaxableComponent)

    #undef UNSUBSCRIBE_TYPE

    m_subscribedTypes.clear();
}

std::unordered_set<EntityID> SyncSystem::getCreatedEntities() const {
    std::unordered_set<EntityID> result;
    for (const auto& [entity, change] : m_dirtyEntities) {
        if (change.type == ChangeType::Created) {
            result.insert(entity);
        }
    }
    return result;
}

std::unordered_set<EntityID> SyncSystem::getUpdatedEntities() const {
    std::unordered_set<EntityID> result;
    for (const auto& [entity, change] : m_dirtyEntities) {
        if (change.type == ChangeType::Updated) {
            result.insert(entity);
        }
    }
    return result;
}

std::unordered_set<EntityID> SyncSystem::getDestroyedEntities() const {
    std::unordered_set<EntityID> result;
    for (const auto& [entity, change] : m_dirtyEntities) {
        if (change.type == ChangeType::Destroyed) {
            result.insert(entity);
        }
    }
    return result;
}

void SyncSystem::flush() {
    m_dirtyEntities.clear();
}

void SyncSystem::markDirty(EntityID entity, ChangeType type) {
    auto& change = m_dirtyEntities[entity];
    // Don't downgrade Created to Updated, but Destroyed overrides all
    if (type == ChangeType::Destroyed) {
        change.type = ChangeType::Destroyed;
        change.componentMask = 0;
    } else if (change.type != ChangeType::Created) {
        change.type = type;
    }
}

void SyncSystem::markComponentDirty(EntityID entity, std::uint8_t componentTypeId,
                                     ChangeType type) {
    auto& change = m_dirtyEntities[entity];
    // Don't downgrade Created to Updated, but Destroyed overrides all
    if (type == ChangeType::Destroyed) {
        change.type = ChangeType::Destroyed;
        change.componentMask = 0;
    } else if (change.type != ChangeType::Created) {
        change.type = type;
    }
    change.markComponent(componentTypeId);
}

// =============================================================================
// Delta Generation (Server-side)
// =============================================================================

std::unique_ptr<StateUpdateMessage> SyncSystem::generateDelta(SimulationTick tick) {
    auto message = std::make_unique<StateUpdateMessage>();
    message->tick = tick;

    for (const auto& [entity, change] : m_dirtyEntities) {
        std::vector<std::uint8_t> componentData;

        switch (change.type) {
            case ChangeType::Created:
                // New entity: serialize all syncable components
                serializeAllComponents(entity, componentData);
                message->addCreate(entity, componentData);
                break;

            case ChangeType::Updated:
                // Modified entity: serialize only changed components
                serializeChangedComponents(entity, change.componentMask, componentData);
                if (!componentData.empty()) {
                    message->addUpdate(entity, componentData);
                }
                break;

            case ChangeType::Destroyed:
                // Destroyed entity: just the ID, no component data
                message->addDestroy(entity);
                break;
        }
    }

    return message;
}

void SyncSystem::serializeAllComponents(EntityID entity, std::vector<std::uint8_t>& buffer) {
    NetworkBuffer netBuf;
    auto ent = static_cast<entt::entity>(entity);
    auto& raw = m_registry.raw();

    // Serialize each component type if present (with type ID prefix)
    if (raw.all_of<PositionComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Position);
        raw.get<PositionComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<OwnershipComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Ownership);
        raw.get<OwnershipComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<TransformComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Transform);
        raw.get<TransformComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<BuildingComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Building);
        raw.get<BuildingComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<EnergyComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Energy);
        raw.get<EnergyComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<PopulationComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Population);
        raw.get<PopulationComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<ZoneComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Zone);
        raw.get<ZoneComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<TransportComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Transport);
        raw.get<TransportComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<ServiceCoverageComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::ServiceCoverage);
        raw.get<ServiceCoverageComponent>(ent).serialize_net(netBuf);
    }
    if (raw.all_of<TaxableComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Taxable);
        raw.get<TaxableComponent>(ent).serialize_net(netBuf);
    }

    // Copy buffer to output vector
    buffer.assign(netBuf.data(), netBuf.data() + netBuf.size());
}

void SyncSystem::serializeChangedComponents(EntityID entity, std::uint32_t componentMask,
                                            std::vector<std::uint8_t>& buffer) {
    NetworkBuffer netBuf;
    auto ent = static_cast<entt::entity>(entity);
    auto& raw = m_registry.raw();

    // Only serialize components that are in the mask AND still present on entity
    if ((componentMask & (1u << ComponentTypeID::Position)) && raw.all_of<PositionComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Position);
        raw.get<PositionComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Ownership)) && raw.all_of<OwnershipComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Ownership);
        raw.get<OwnershipComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Transform)) && raw.all_of<TransformComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Transform);
        raw.get<TransformComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Building)) && raw.all_of<BuildingComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Building);
        raw.get<BuildingComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Energy)) && raw.all_of<EnergyComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Energy);
        raw.get<EnergyComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Population)) && raw.all_of<PopulationComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Population);
        raw.get<PopulationComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Zone)) && raw.all_of<ZoneComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Zone);
        raw.get<ZoneComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Transport)) && raw.all_of<TransportComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Transport);
        raw.get<TransportComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::ServiceCoverage)) && raw.all_of<ServiceCoverageComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::ServiceCoverage);
        raw.get<ServiceCoverageComponent>(ent).serialize_net(netBuf);
    }
    if ((componentMask & (1u << ComponentTypeID::Taxable)) && raw.all_of<TaxableComponent>(ent)) {
        netBuf.write_u8(ComponentTypeID::Taxable);
        raw.get<TaxableComponent>(ent).serialize_net(netBuf);
    }

    // Copy buffer to output vector
    buffer.assign(netBuf.data(), netBuf.data() + netBuf.size());
}

// =============================================================================
// Delta Application (Client-side)
// =============================================================================

DeltaApplicationResult SyncSystem::applyDelta(const StateUpdateMessage& message) {
    // Handle out-of-order messages
    if (message.tick < m_lastProcessedTick) {
        return DeltaApplicationResult::OutOfOrder;
    }

    // Handle duplicate messages (idempotent)
    if (message.tick == m_lastProcessedTick && m_lastProcessedTick != 0) {
        return DeltaApplicationResult::Duplicate;
    }

    auto& raw = m_registry.raw();

    for (const auto& delta : message.deltas) {
        switch (delta.type) {
            case EntityDeltaType::Create: {
                // Create entity with server-assigned ID
                auto ent = static_cast<entt::entity>(delta.entityId);

                // If entity already exists (reconnect scenario), destroy and recreate
                if (raw.valid(ent)) {
                    raw.destroy(ent);
                }

                // Create with specific ID using EnTT's create(hint) overload
                // EnTT will use this ID if available
                auto created = raw.create(ent);
                if (static_cast<EntityID>(created) != delta.entityId) {
                    // This shouldn't happen with proper entity management
                    // but handle it gracefully by destroying the created one
                    raw.destroy(created);
                    return DeltaApplicationResult::Error;
                }

                // Deserialize and apply all components
                if (!deserializeAndApplyComponents(delta.entityId, delta.componentData, true)) {
                    return DeltaApplicationResult::Error;
                }
                break;
            }

            case EntityDeltaType::Update: {
                auto ent = static_cast<entt::entity>(delta.entityId);

                // Entity must exist for update
                if (!raw.valid(ent)) {
                    // Entity doesn't exist - log warning for debugging
                    // This could happen if we missed a create message due to packet loss
                    LOG_WARN("Client received update for unknown entity ID %u - "
                             "possibly missed create message, auto-creating entity",
                             delta.entityId);

                    // Recover by creating the entity
                    auto created = raw.create(ent);
                    if (static_cast<EntityID>(created) != delta.entityId) {
                        LOG_ERROR("Failed to create entity with server ID %u", delta.entityId);
                        raw.destroy(created);
                        return DeltaApplicationResult::Error;
                    }
                }

                // Deserialize and apply changed components
                if (!deserializeAndApplyComponents(delta.entityId, delta.componentData, false)) {
                    return DeltaApplicationResult::Error;
                }
                break;
            }

            case EntityDeltaType::Destroy: {
                auto ent = static_cast<entt::entity>(delta.entityId);

                // Only destroy if exists (idempotent)
                if (raw.valid(ent)) {
                    raw.destroy(ent);
                } else {
                    // Log warning for unknown entity - could be duplicate destroy
                    // or missed create. This is not an error (idempotent destroy).
                    LOG_WARN("Client received destroy for unknown entity ID %u - "
                             "ignoring (idempotent)", delta.entityId);
                }
                break;
            }
        }
    }

    // Update last processed tick
    m_lastProcessedTick = message.tick;

    return DeltaApplicationResult::Applied;
}

bool SyncSystem::deserializeAndApplyComponents(EntityID entity,
                                               const std::vector<std::uint8_t>& componentData,
                                               bool isNewEntity) {
    if (componentData.empty()) {
        return true;
    }

    NetworkBuffer buffer(componentData.data(), componentData.size());
    auto ent = static_cast<entt::entity>(entity);
    auto& raw = m_registry.raw();

    try {
        while (!buffer.at_end()) {
            std::uint8_t typeId = buffer.read_u8();

            switch (typeId) {
                case ComponentTypeID::Position: {
                    auto comp = PositionComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<PositionComponent>(ent)) {
                        raw.emplace<PositionComponent>(ent, comp);
                    } else {
                        raw.replace<PositionComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Ownership: {
                    auto comp = OwnershipComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<OwnershipComponent>(ent)) {
                        raw.emplace<OwnershipComponent>(ent, comp);
                    } else {
                        raw.replace<OwnershipComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Transform: {
                    auto comp = TransformComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<TransformComponent>(ent)) {
                        raw.emplace<TransformComponent>(ent, comp);
                    } else {
                        raw.replace<TransformComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Building: {
                    auto comp = BuildingComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<BuildingComponent>(ent)) {
                        raw.emplace<BuildingComponent>(ent, comp);
                    } else {
                        raw.replace<BuildingComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Energy: {
                    auto comp = EnergyComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<EnergyComponent>(ent)) {
                        raw.emplace<EnergyComponent>(ent, comp);
                    } else {
                        raw.replace<EnergyComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Population: {
                    auto comp = PopulationComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<PopulationComponent>(ent)) {
                        raw.emplace<PopulationComponent>(ent, comp);
                    } else {
                        raw.replace<PopulationComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Zone: {
                    auto comp = ZoneComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<ZoneComponent>(ent)) {
                        raw.emplace<ZoneComponent>(ent, comp);
                    } else {
                        raw.replace<ZoneComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Transport: {
                    auto comp = TransportComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<TransportComponent>(ent)) {
                        raw.emplace<TransportComponent>(ent, comp);
                    } else {
                        raw.replace<TransportComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::ServiceCoverage: {
                    auto comp = ServiceCoverageComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<ServiceCoverageComponent>(ent)) {
                        raw.emplace<ServiceCoverageComponent>(ent, comp);
                    } else {
                        raw.replace<ServiceCoverageComponent>(ent, comp);
                    }
                    break;
                }
                case ComponentTypeID::Taxable: {
                    auto comp = TaxableComponent::deserialize_net(buffer);
                    if (isNewEntity || !raw.all_of<TaxableComponent>(ent)) {
                        raw.emplace<TaxableComponent>(ent, comp);
                    } else {
                        raw.replace<TaxableComponent>(ent, comp);
                    }
                    break;
                }
                default:
                    // Unknown component type - skip or error
                    // For forward compatibility, we could try to skip
                    // but we don't know the size, so return error
                    return false;
            }
        }
    } catch (const BufferOverflowError&) {
        return false;
    }

    return true;
}

// =============================================================================
// Full State Snapshot Generation (Server-side)
// =============================================================================

bool SyncSystem::startSnapshotGeneration(SimulationTick tick) {
    // Check if already generating
    bool expected = false;
    if (!m_snapshotGenerating.compare_exchange_strong(expected, true)) {
        LOG_WARN("Snapshot generation already in progress");
        return false;
    }

    // Clear previous snapshot data
    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_snapshotData.clear();
        m_snapshotCompressed.clear();
        m_snapshotEntityCount = 0;
        m_snapshotChecksum = 0;
    }

    // Clear COW buffer
    {
        std::lock_guard<std::mutex> lock(m_cowMutex);
        m_cowBuffer.clear();
    }

    m_snapshotTick = tick;
    m_snapshotReady.store(false);

    // Start async generation
    m_snapshotFuture = std::async(std::launch::async, [this]() {
        generateSnapshotData();
    });

    LOG_INFO("Snapshot generation started for tick %llu", tick);
    return true;
}

bool SyncSystem::isSnapshotReady() const {
    return m_snapshotReady.load();
}

void SyncSystem::generateSnapshotData() {
    std::vector<std::uint8_t> data;
    data.reserve(1024 * 1024);  // Reserve 1MB initially

    // Serialize all entities
    std::uint32_t entityCount = serializeAllEntities(data);

    // Compress with LZ4
    std::vector<std::uint8_t> compressed;
    if (!compressLZ4(data, compressed)) {
        LOG_ERROR("Failed to compress snapshot data");
        m_snapshotGenerating.store(false);
        return;
    }

    // Calculate checksum of uncompressed data
    std::uint32_t checksum = calculateCRC32(data);

    // Store results
    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_snapshotData = std::move(data);
        m_snapshotCompressed = std::move(compressed);
        m_snapshotEntityCount = entityCount;
        m_snapshotChecksum = checksum;
    }

    m_snapshotReady.store(true);
    m_snapshotGenerating.store(false);

    LOG_INFO("Snapshot generation complete: %u entities, %zu bytes (compressed: %zu bytes)",
             entityCount, m_snapshotData.size(), m_snapshotCompressed.size());
}

std::uint32_t SyncSystem::serializeAllEntities(std::vector<std::uint8_t>& buffer) {
    NetworkBuffer netBuf;
    auto& raw = m_registry.raw();
    std::uint32_t entityCount = 0;

    // First pass: count entities with syncable components
    // We'll write entity count at the start
    std::size_t countPos = netBuf.size();
    netBuf.write_u32(0);  // Placeholder for entity count

    // Iterate all entities using storage iteration (EnTT 3.x compatible)
    for (auto ent : raw.storage<entt::entity>()) {
        EntityID entityId = static_cast<EntityID>(ent);

        // Check if entity has any syncable components
        bool hasSyncable = false;
        if (raw.all_of<PositionComponent>(ent) ||
            raw.all_of<OwnershipComponent>(ent) ||
            raw.all_of<TransformComponent>(ent) ||
            raw.all_of<BuildingComponent>(ent) ||
            raw.all_of<EnergyComponent>(ent) ||
            raw.all_of<PopulationComponent>(ent) ||
            raw.all_of<ZoneComponent>(ent) ||
            raw.all_of<TransportComponent>(ent) ||
            raw.all_of<ServiceCoverageComponent>(ent) ||
            raw.all_of<TaxableComponent>(ent)) {
            hasSyncable = true;
        }

        if (!hasSyncable) {
            continue;  // Skip entities with no syncable components
        }

        // Check COW buffer for this entity
        std::unordered_map<std::uint8_t, std::vector<std::uint8_t>> cowData;
        {
            std::lock_guard<std::mutex> lock(m_cowMutex);
            auto it = m_cowBuffer.find(entityId);
            if (it != m_cowBuffer.end()) {
                cowData = it->second;
            }
        }

        // Write entity ID
        netBuf.write_u32(entityId);

        // Count components placeholder
        std::size_t compCountPos = netBuf.size();
        netBuf.write_u8(0);
        std::uint8_t componentCount = 0;

        // Helper to serialize component (uses COW data if available)
        auto serializeComponent = [&](std::uint8_t typeId, auto& component) {
            auto cowIt = cowData.find(typeId);
            if (cowIt != cowData.end()) {
                // Use COW data (old value)
                netBuf.write_u8(typeId);
                netBuf.write_bytes(cowIt->second.data(), cowIt->second.size());
            } else {
                // Use current value
                netBuf.write_u8(typeId);
                component.serialize_net(netBuf);
            }
            componentCount++;
        };

        // Serialize each component type
        if (raw.all_of<PositionComponent>(ent)) {
            serializeComponent(ComponentTypeID::Position, raw.get<PositionComponent>(ent));
        }
        if (raw.all_of<OwnershipComponent>(ent)) {
            serializeComponent(ComponentTypeID::Ownership, raw.get<OwnershipComponent>(ent));
        }
        if (raw.all_of<TransformComponent>(ent)) {
            serializeComponent(ComponentTypeID::Transform, raw.get<TransformComponent>(ent));
        }
        if (raw.all_of<BuildingComponent>(ent)) {
            serializeComponent(ComponentTypeID::Building, raw.get<BuildingComponent>(ent));
        }
        if (raw.all_of<EnergyComponent>(ent)) {
            serializeComponent(ComponentTypeID::Energy, raw.get<EnergyComponent>(ent));
        }
        if (raw.all_of<PopulationComponent>(ent)) {
            serializeComponent(ComponentTypeID::Population, raw.get<PopulationComponent>(ent));
        }
        if (raw.all_of<ZoneComponent>(ent)) {
            serializeComponent(ComponentTypeID::Zone, raw.get<ZoneComponent>(ent));
        }
        if (raw.all_of<TransportComponent>(ent)) {
            serializeComponent(ComponentTypeID::Transport, raw.get<TransportComponent>(ent));
        }
        if (raw.all_of<ServiceCoverageComponent>(ent)) {
            serializeComponent(ComponentTypeID::ServiceCoverage, raw.get<ServiceCoverageComponent>(ent));
        }
        if (raw.all_of<TaxableComponent>(ent)) {
            serializeComponent(ComponentTypeID::Taxable, raw.get<TaxableComponent>(ent));
        }

        // Update component count
        netBuf.raw()[compCountPos] = componentCount;
        entityCount++;
    }

    // Update entity count at the beginning
    auto* countPtr = reinterpret_cast<std::uint32_t*>(netBuf.raw().data() + countPos);
    *countPtr = entityCount;

    buffer.assign(netBuf.data(), netBuf.data() + netBuf.size());
    return entityCount;
}

std::uint32_t SyncSystem::calculateCRC32(const std::vector<std::uint8_t>& data) {
    std::uint32_t crc = 0xFFFFFFFF;
    for (std::uint8_t byte : data) {
        crc = CRC32_TABLE[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

bool SyncSystem::getSnapshotMessages(SnapshotStartMessage& outStart,
                                      std::vector<SnapshotChunkMessage>& outChunks,
                                      SnapshotEndMessage& outEnd) {
    if (!m_snapshotReady.load()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_snapshotMutex);

    // Split compressed data into chunks
    auto chunks = splitIntoChunks(m_snapshotCompressed, SNAPSHOT_CHUNK_SIZE);

    // Build SnapshotStartMessage
    outStart.tick = m_snapshotTick;
    outStart.totalChunks = static_cast<std::uint32_t>(chunks.size());
    outStart.totalBytes = static_cast<std::uint32_t>(m_snapshotData.size());
    outStart.compressedBytes = static_cast<std::uint32_t>(m_snapshotCompressed.size());
    outStart.entityCount = m_snapshotEntityCount;

    // Build SnapshotChunkMessages
    outChunks.clear();
    outChunks.reserve(chunks.size());
    for (std::uint32_t i = 0; i < chunks.size(); ++i) {
        SnapshotChunkMessage chunk;
        chunk.chunkIndex = i;
        chunk.data = std::move(chunks[i]);
        outChunks.push_back(std::move(chunk));
    }

    // Build SnapshotEndMessage
    outEnd.checksum = m_snapshotChecksum;

    // Clear snapshot data after retrieval
    m_snapshotData.clear();
    m_snapshotCompressed.clear();
    m_snapshotReady.store(false);

    return true;
}

void SyncSystem::notifySnapshotCOW(EntityID entity, std::uint8_t componentTypeId,
                                    const std::vector<std::uint8_t>& oldData) {
    if (!m_snapshotGenerating.load()) {
        return;  // No snapshot in progress, ignore
    }

    std::lock_guard<std::mutex> lock(m_cowMutex);

    // Only store if not already stored (first modification wins)
    auto& entityCow = m_cowBuffer[entity];
    if (entityCow.find(componentTypeId) == entityCow.end()) {
        entityCow[componentTypeId] = oldData;
    }
}

// =============================================================================
// Full State Snapshot Reception (Client-side)
// =============================================================================

void SyncSystem::handleSnapshotStart(const SnapshotStartMessage& message) {
    LOG_INFO("Receiving snapshot: tick=%llu, chunks=%u, bytes=%u, entities=%u",
             message.tick, message.totalChunks, message.totalBytes, message.entityCount);

    m_snapshotProgress.tick = message.tick;
    m_snapshotProgress.totalChunks = message.totalChunks;
    m_snapshotProgress.receivedChunks = 0;
    m_snapshotProgress.totalBytes = message.totalBytes;
    m_snapshotProgress.entityCount = message.entityCount;
    m_snapshotProgress.state = SnapshotState::Receiving;

    // Resize chunk buffer
    m_snapshotChunks.clear();
    m_snapshotChunks.resize(message.totalChunks);

    // Clear delta buffer
    {
        std::lock_guard<std::mutex> lock(m_deltaBufferMutex);
        m_bufferedDeltas.clear();
    }
}

void SyncSystem::handleSnapshotChunk(const SnapshotChunkMessage& message) {
    if (m_snapshotProgress.state != SnapshotState::Receiving) {
        LOG_WARN("Received snapshot chunk while not in receiving state");
        return;
    }

    if (message.chunkIndex >= m_snapshotChunks.size()) {
        LOG_ERROR("Invalid chunk index: %u (expected < %zu)",
                  message.chunkIndex, m_snapshotChunks.size());
        return;
    }

    // Store chunk (may arrive out of order)
    if (m_snapshotChunks[message.chunkIndex].empty()) {
        m_snapshotChunks[message.chunkIndex] = message.data;
        m_snapshotProgress.receivedChunks++;
        LOG_DEBUG("Received chunk %u/%u (%.1f%%)",
                  m_snapshotProgress.receivedChunks,
                  m_snapshotProgress.totalChunks,
                  m_snapshotProgress.getProgress() * 100.0f);
    }
}

bool SyncSystem::handleSnapshotEnd(const SnapshotEndMessage& message) {
    if (m_snapshotProgress.state != SnapshotState::Receiving) {
        LOG_WARN("Received snapshot end while not in receiving state");
        return false;
    }

    // Verify all chunks received
    if (m_snapshotProgress.receivedChunks != m_snapshotProgress.totalChunks) {
        LOG_ERROR("Snapshot incomplete: received %u/%u chunks",
                  m_snapshotProgress.receivedChunks, m_snapshotProgress.totalChunks);
        m_snapshotProgress.state = SnapshotState::None;
        return false;
    }

    m_snapshotProgress.state = SnapshotState::Applying;

    // Reassemble chunks
    auto compressed = reassembleChunks(m_snapshotChunks);

    // Decompress
    std::vector<std::uint8_t> data;
    if (!decompressLZ4(compressed, data)) {
        LOG_ERROR("Failed to decompress snapshot data");
        m_snapshotProgress.state = SnapshotState::None;
        return false;
    }

    // Verify checksum
    std::uint32_t checksum = calculateCRC32(data);
    if (checksum != message.checksum) {
        LOG_ERROR("Snapshot checksum mismatch: expected %08X, got %08X",
                  message.checksum, checksum);
        m_snapshotProgress.state = SnapshotState::None;
        return false;
    }

    // Store decompressed data temporarily and apply
    m_snapshotChunks.clear();  // Free memory

    // Clear local state before applying
    clearLocalState();

    // Apply snapshot to registry
    NetworkBuffer buffer(data.data(), data.size());
    try {
        std::uint32_t entityCount = buffer.read_u32();
        auto& raw = m_registry.raw();

        for (std::uint32_t i = 0; i < entityCount; ++i) {
            EntityID entityId = buffer.read_u32();
            std::uint8_t componentCount = buffer.read_u8();

            // Create entity with specific ID
            auto ent = static_cast<entt::entity>(entityId);
            auto created = raw.create(ent);
            if (static_cast<EntityID>(created) != entityId) {
                LOG_ERROR("Failed to create entity with ID %u", entityId);
                raw.destroy(created);
                m_snapshotProgress.state = SnapshotState::None;
                return false;
            }

            // Apply components
            for (std::uint8_t j = 0; j < componentCount; ++j) {
                std::uint8_t typeId = buffer.read_u8();

                switch (typeId) {
                    case ComponentTypeID::Position: {
                        auto comp = PositionComponent::deserialize_net(buffer);
                        raw.emplace<PositionComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Ownership: {
                        auto comp = OwnershipComponent::deserialize_net(buffer);
                        raw.emplace<OwnershipComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Transform: {
                        auto comp = TransformComponent::deserialize_net(buffer);
                        raw.emplace<TransformComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Building: {
                        auto comp = BuildingComponent::deserialize_net(buffer);
                        raw.emplace<BuildingComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Energy: {
                        auto comp = EnergyComponent::deserialize_net(buffer);
                        raw.emplace<EnergyComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Population: {
                        auto comp = PopulationComponent::deserialize_net(buffer);
                        raw.emplace<PopulationComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Zone: {
                        auto comp = ZoneComponent::deserialize_net(buffer);
                        raw.emplace<ZoneComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Transport: {
                        auto comp = TransportComponent::deserialize_net(buffer);
                        raw.emplace<TransportComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::ServiceCoverage: {
                        auto comp = ServiceCoverageComponent::deserialize_net(buffer);
                        raw.emplace<ServiceCoverageComponent>(ent, comp);
                        break;
                    }
                    case ComponentTypeID::Taxable: {
                        auto comp = TaxableComponent::deserialize_net(buffer);
                        raw.emplace<TaxableComponent>(ent, comp);
                        break;
                    }
                    default:
                        LOG_ERROR("Unknown component type %u in snapshot", typeId);
                        m_snapshotProgress.state = SnapshotState::None;
                        return false;
                }
            }
        }
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("Buffer overflow while applying snapshot: %s", e.what());
        m_snapshotProgress.state = SnapshotState::None;
        return false;
    }

    // Update last processed tick to snapshot tick
    m_lastProcessedTick = m_snapshotProgress.tick;

    // Apply buffered deltas
    applyBufferedDeltas();

    m_snapshotProgress.state = SnapshotState::Complete;
    LOG_INFO("Snapshot applied successfully: %u entities, tick=%llu",
             m_snapshotProgress.entityCount, m_snapshotProgress.tick);

    return true;
}

bool SyncSystem::bufferDeltaDuringSnapshot(const StateUpdateMessage& message) {
    if (m_snapshotProgress.state != SnapshotState::Receiving) {
        // Not receiving snapshot, shouldn't buffer
        return false;
    }

    std::lock_guard<std::mutex> lock(m_deltaBufferMutex);

    // Check buffer limit
    if (m_bufferedDeltas.size() >= MAX_BUFFERED_DELTAS) {
        LOG_WARN("Delta buffer overflow during snapshot: discarding delta at tick %llu",
                 message.tick);
        return false;  // Buffer full
    }

    // Copy message to buffer
    auto buffered = std::make_unique<StateUpdateMessage>();
    buffered->tick = message.tick;
    buffered->deltas = message.deltas;
    buffered->compressed = message.compressed;

    m_bufferedDeltas.push_back(std::move(buffered));
    return true;
}

void SyncSystem::clearLocalState() {
    LOG_INFO("Clearing local ECS state");
    m_registry.clear();
    m_dirtyEntities.clear();
}

void SyncSystem::applyBufferedDeltas() {
    std::lock_guard<std::mutex> lock(m_deltaBufferMutex);

    // Sort deltas by tick (should already be mostly sorted)
    std::sort(m_bufferedDeltas.begin(), m_bufferedDeltas.end(),
              [](const auto& a, const auto& b) { return a->tick < b->tick; });

    std::size_t appliedCount = 0;
    for (const auto& delta : m_bufferedDeltas) {
        // Skip deltas before or at snapshot tick
        if (delta->tick <= m_snapshotProgress.tick) {
            continue;
        }

        auto result = applyDelta(*delta);
        if (result == DeltaApplicationResult::Applied) {
            appliedCount++;
        } else if (result == DeltaApplicationResult::Error) {
            LOG_WARN("Error applying buffered delta at tick %llu", delta->tick);
        }
    }

    LOG_INFO("Applied %zu buffered deltas after snapshot", appliedCount);
    m_bufferedDeltas.clear();
}

} // namespace sims3000

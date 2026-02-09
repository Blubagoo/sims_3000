/**
 * @file TerminologyLookup.cpp
 * @brief Implementation of the terminology translation system.
 *
 * All canonical mappings from docs/canon/terminology.yaml are hardcoded in
 * load_defaults().  When YAML parsing is added later, load() will populate
 * m_terms from a file instead.
 */

#include "sims3000/ui/TerminologyLookup.h"

#include <algorithm>
#include <cctype>

namespace sims3000 {
namespace ui {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Convert a string to lowercase in-place and return it.
std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TerminologyLookup::TerminologyLookup() {
    load_defaults();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool TerminologyLookup::load(const std::string& /*path*/) {
    // YAML parsing not implemented yet.  Returns false to signal that built-in
    // defaults are still in use.
    return false;
}

const std::string& TerminologyLookup::get(const std::string& human_term) const {
    std::string key = to_lower(human_term);
    auto it = m_terms.find(key);
    if (it != m_terms.end()) {
        return it->second;
    }
    // No mapping found -- return the original term.
    // We return from the map-end sentinel trick: store the lowered key
    // temporarily is unsafe, so we return the caller's reference instead.
    return human_term;
}

bool TerminologyLookup::has(const std::string& human_term) const {
    return m_terms.find(to_lower(human_term)) != m_terms.end();
}

size_t TerminologyLookup::count() const {
    return m_terms.size();
}

TerminologyLookup& TerminologyLookup::instance() {
    static TerminologyLookup s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// load_defaults -- all canonical mappings from docs/canon/terminology.yaml
// ---------------------------------------------------------------------------

void TerminologyLookup::load_defaults() {
    // Reserve a generous bucket count to avoid rehashing.
    m_terms.reserve(200);

    // -----------------------------------------------------------------------
    // World terms
    // -----------------------------------------------------------------------
    m_terms["city"]            = "colony";
    m_terms["town"]            = "settlement";
    m_terms["village"]         = "outpost";
    m_terms["downtown"]        = "core_sector";
    m_terms["neighborhood"]    = "district";
    m_terms["suburb"]          = "outer_district";

    m_terms["citizen"]         = "being";
    m_terms["citizens"]        = "beings";
    m_terms["population"]      = "colony_population";
    m_terms["people"]          = "beings";
    m_terms["resident"]        = "inhabitant";
    m_terms["residents"]       = "inhabitants";
    m_terms["worker"]          = "laborer";
    m_terms["workers"]         = "laborers";

    m_terms["mayor"]           = "overseer";
    m_terms["city_hall"]       = "command_nexus";
    m_terms["government"]      = "administration";
    m_terms["council"]         = "elder_circle";
    m_terms["player"]          = "overseer";

    // -----------------------------------------------------------------------
    // Terrain terms
    // -----------------------------------------------------------------------
    m_terms["flat_ground"]     = "substrate";
    m_terms["hills"]           = "ridges";
    m_terms["ocean"]           = "deep_void";
    m_terms["river"]           = "flow_channel";
    m_terms["lake"]            = "still_basin";
    m_terms["forest"]          = "biolume_grove";

    m_terms["crystal_fields"]  = "prisma_fields";
    m_terms["spore_plains"]    = "spore_flats";
    m_terms["toxic_marshes"]   = "blight_mires";
    m_terms["volcanic_rock"]   = "ember_crust";

    m_terms["clear_terrain"]   = "purge_terrain";
    m_terms["level_terrain"]   = "grade_terrain";
    m_terms["terraform"]       = "reclaim_terrain";
    m_terms["spawn_point"]     = "emergence_site";

    // -----------------------------------------------------------------------
    // Zone terms
    // -----------------------------------------------------------------------
    m_terms["residential"]             = "habitation";
    m_terms["commercial"]              = "exchange";
    m_terms["industrial"]              = "fabrication";

    m_terms["light_residential"]       = "low_density_habitation";
    m_terms["dense_residential"]       = "high_density_habitation";
    m_terms["light_commercial"]        = "low_density_exchange";
    m_terms["dense_commercial"]        = "high_density_exchange";
    m_terms["light_industrial"]        = "low_density_fabrication";
    m_terms["dense_industrial"]        = "high_density_fabrication";

    m_terms["zoned"]                   = "designated";
    m_terms["unzoned"]                 = "undesignated";
    m_terms["rezoning"]                = "redesignation";

    // -----------------------------------------------------------------------
    // Infrastructure terms
    // -----------------------------------------------------------------------
    // Power/Energy
    m_terms["power"]           = "energy";
    m_terms["electricity"]     = "energy";
    m_terms["power_plant"]     = "energy_nexus";
    m_terms["power_line"]      = "energy_conduit";
    m_terms["power_grid"]      = "energy_matrix";
    m_terms["blackout"]        = "grid_collapse";
    m_terms["brownout"]        = "energy_deficit";

    // Water/Fluid
    m_terms["water"]           = "fluid";
    m_terms["water_system"]    = "fluid_network";
    m_terms["water_pump"]      = "fluid_extractor";
    m_terms["water_tower"]     = "fluid_reservoir";
    m_terms["pipes"]           = "fluid_conduits";
    m_terms["water_pressure"]  = "fluid_pressure";

    // Transportation
    m_terms["road"]            = "pathway";
    m_terms["roads"]           = "pathways";
    m_terms["highway"]         = "transit_corridor";
    m_terms["street"]          = "pathway";
    m_terms["traffic"]         = "flow";
    m_terms["congestion"]      = "flow_blockage";
    m_terms["intersection"]    = "junction";

    // Rail
    m_terms["railroad"]        = "rail_line";
    m_terms["train"]           = "rail_transport";
    m_terms["train_station"]   = "rail_terminal";
    m_terms["subway"]          = "subterra_rail";
    m_terms["subway_station"]  = "subterra_terminal";

    // Other Transit
    m_terms["bus"]             = "transport_pod";
    m_terms["bus_depot"]       = "pod_hub";
    m_terms["airport"]         = "aero_port";
    m_terms["seaport"]         = "aqua_port";

    // -----------------------------------------------------------------------
    // Services terms
    // -----------------------------------------------------------------------
    // Safety
    m_terms["police"]          = "enforcers";
    m_terms["police_station"]  = "enforcer_post";
    m_terms["fire_department"] = "hazard_response";
    m_terms["fire_station"]    = "hazard_response_post";
    m_terms["firefighter"]     = "hazard_responder";

    // Health
    m_terms["hospital"]        = "medical_nexus";
    m_terms["clinic"]          = "medical_post";
    m_terms["health"]          = "vitality";
    m_terms["life_expectancy"] = "longevity";

    // Education
    m_terms["school"]          = "learning_center";
    m_terms["college"]         = "advanced_learning_center";
    m_terms["university"]      = "knowledge_nexus";
    m_terms["library"]         = "archive";
    m_terms["museum"]          = "heritage_vault";
    m_terms["education"]       = "knowledge_quotient";

    // Recreation
    m_terms["park"]            = "green_zone";
    m_terms["stadium"]         = "arena";
    m_terms["zoo"]             = "creature_preserve";
    m_terms["marina"]          = "watercraft_dock";

    // -----------------------------------------------------------------------
    // Economy terms
    // -----------------------------------------------------------------------
    m_terms["money"]           = "credits";
    m_terms["dollar"]          = "credit";
    m_terms["dollars"]         = "credits";
    m_terms["budget"]          = "colony_treasury";
    m_terms["taxes"]           = "tribute";
    m_terms["tax_rate"]        = "tribute_rate";
    m_terms["income"]          = "revenue";
    m_terms["expenses"]        = "expenditure";
    m_terms["debt"]            = "deficit";
    m_terms["loan"]            = "credit_advance";
    m_terms["bond"]            = "treasury_bond";

    // -----------------------------------------------------------------------
    // Simulation terms
    // -----------------------------------------------------------------------
    // Metrics
    m_terms["happiness"]       = "harmony";
    m_terms["crime"]           = "disorder";
    m_terms["crime_rate"]      = "disorder_index";
    m_terms["pollution"]       = "contamination";
    m_terms["land_value"]      = "sector_value";
    m_terms["property_value"]  = "sector_value";

    // Demand
    m_terms["demand"]          = "growth_pressure";
    m_terms["rci_demand"]      = "zone_pressure";

    // Time
    m_terms["year"]            = "cycle";
    m_terms["month"]           = "phase";
    m_terms["day"]             = "rotation";

    // -----------------------------------------------------------------------
    // Disaster terms
    // -----------------------------------------------------------------------
    m_terms["disaster"]        = "catastrophe";
    m_terms["earthquake"]      = "seismic_event";
    m_terms["flood"]           = "inundation";
    m_terms["tornado"]         = "vortex_storm";
    m_terms["hurricane"]       = "mega_storm";
    m_terms["fire"]            = "conflagration";
    m_terms["riot"]            = "civil_unrest";
    m_terms["nuclear_meltdown"] = "core_breach";
    m_terms["monster_attack"]  = "titan_emergence";
    m_terms["cosmic_event"]    = "void_anomaly";
    m_terms["radiation_event"] = "cosmic_radiation";

    // -----------------------------------------------------------------------
    // Building terms
    // -----------------------------------------------------------------------
    m_terms["building"]        = "structure";
    m_terms["skyscraper"]      = "tower";
    m_terms["house"]           = "dwelling";
    m_terms["apartment"]       = "hab_unit";
    m_terms["office"]          = "work_center";
    m_terms["factory"]         = "fabricator";
    m_terms["warehouse"]       = "storage_hub";

    // Building states
    m_terms["under_construction"] = "materializing";
    m_terms["abandoned"]       = "derelict";
    m_terms["demolished"]      = "deconstructed";
    m_terms["rubble"]          = "debris";

    // -----------------------------------------------------------------------
    // Progression terms
    // -----------------------------------------------------------------------
    m_terms["mayors_mansion"]  = "overseers_sanctum";
    // city_hall already mapped above
    m_terms["statue"]          = "monument";
    m_terms["military_base"]   = "defense_installation";
    m_terms["arcology"]        = "arcology";

    m_terms["milestone"]       = "achievement";
    m_terms["population_milestone"] = "growth_achievement";

    m_terms["edict"]           = "edict";
    m_terms["edict_slot"]      = "guidance_channel";
    m_terms["decree"]          = "edict";

    m_terms["transcendence"]   = "transcendence";
    m_terms["transcendence_monument"] = "transcendence_marker";

    m_terms["plymouth_arcology"] = "plymouth_arcology";
    m_terms["forest_arcology"] = "forest_arcology";
    m_terms["darco_arcology"]  = "darco_arcology";
    m_terms["launch_arcology"] = "launch_arcology";

    m_terms["colony_emergence"]      = "colony_emergence";
    m_terms["colony_establishment"]  = "colony_establishment";
    m_terms["colony_identity"]       = "colony_identity";
    m_terms["colony_security"]       = "colony_security";
    m_terms["colony_wonder"]         = "colony_wonder";
    m_terms["colony_ascension"]      = "colony_ascension";
    m_terms["colony_transcendence"]  = "colony_transcendence";

    m_terms["eternity_marker"]       = "eternity_marker";
    m_terms["aegis_complex"]         = "aegis_complex";
    m_terms["resonance_spire"]       = "resonance_spire";

    // -----------------------------------------------------------------------
    // Multiplayer terms
    // -----------------------------------------------------------------------
    m_terms["game_master"]     = "world_keeper";
    m_terms["host"]            = "prime_overseer";
    m_terms["client"]          = "connected_overseer";
    m_terms["lobby"]           = "staging_area";
    m_terms["player_color"]    = "faction_color";
    m_terms["territory"]       = "domain";
    m_terms["shared_infrastructure"] = "mutual_works";

    // -----------------------------------------------------------------------
    // Ownership terms
    // -----------------------------------------------------------------------
    m_terms["unclaimed"]       = "uncharted";
    m_terms["claimed"]         = "chartered";
    // abandoned already mapped above (buildings section)
    m_terms["ghost_town"]      = "remnant";
    m_terms["purchase"]        = "charter";
    m_terms["tile"]            = "sector";

    // -----------------------------------------------------------------------
    // UI terms
    // -----------------------------------------------------------------------
    m_terms["classic_mode"]    = "legacy_interface";
    m_terms["holographic_mode"] = "holo_interface";
    m_terms["minimap"]         = "sector_scan";
    m_terms["toolbar"]         = "command_array";
    m_terms["info_panel"]      = "data_readout";
    m_terms["overlay"]         = "scan_layer";
    m_terms["query_tool"]      = "probe_function";
    m_terms["tooltip"]         = "hover_data";
    m_terms["notification"]    = "alert_pulse";
    m_terms["dialog"]          = "comm_panel";
    m_terms["popup"]           = "info_burst";
    m_terms["budget_window"]   = "colony_treasury_panel";
    m_terms["slider"]          = "adjustment_control";
    m_terms["status_bar"]      = "colony_status";
    m_terms["demand_meter"]    = "zone_pressure_indicator";

    // -----------------------------------------------------------------------
    // Map terms
    // -----------------------------------------------------------------------
    m_terms["small_map"]       = "compact_region";
    m_terms["medium_map"]      = "standard_region";
    m_terms["large_map"]       = "expanse_region";
    m_terms["map_size"]        = "region_scale";
}

} // namespace ui
} // namespace sims3000

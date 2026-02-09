/**
 * @file TerminologyLookup.h
 * @brief Translates human terms to alien equivalents for player-facing UI.
 *
 * Provides a mapping from standard city-builder terminology (city, citizen,
 * residential, etc.) to the alien-themed equivalents defined in the project's
 * canonical terminology file (docs/canon/terminology.yaml).
 *
 * All lookups are case-insensitive.  Keys are stored in lowercase internally.
 *
 * Usage:
 * @code
 *   // Via convenience function
 *   std::string label = sims3000::ui::term("city");  // "colony"
 *
 *   // Via singleton
 *   auto& tl = sims3000::ui::TerminologyLookup::instance();
 *   if (tl.has("residential"))
 *       draw_text(tl.get("residential"));  // "habitation"
 * @endcode
 */

#ifndef SIMS3000_UI_TERMINOLOGY_LOOKUP_H
#define SIMS3000_UI_TERMINOLOGY_LOOKUP_H

#include <string>
#include <unordered_map>

namespace sims3000 {
namespace ui {

/**
 * @class TerminologyLookup
 * @brief Translates human terms to alien equivalents for player-facing UI.
 *
 * Maintains a dictionary of human-term -> alien-term mappings loaded from
 * built-in defaults.  A future load() method will support reading from a
 * YAML file.
 */
class TerminologyLookup {
public:
    TerminologyLookup();

    /**
     * Load terminology from a YAML file path (not implemented yet, uses built-in defaults).
     * @param path Filesystem path to a terminology YAML file
     * @return true if loaded successfully
     */
    bool load(const std::string& path);

    /**
     * Get the alien term for a human term.
     * Lookup is case-insensitive (input is lowered before searching).
     * @param human_term The human/standard term to look up
     * @return Reference to the alien equivalent, or the input term itself if no mapping found
     */
    const std::string& get(const std::string& human_term) const;

    /**
     * Check if a term has a mapping.
     * Lookup is case-insensitive.
     * @param human_term The human/standard term to check
     * @return true if a mapping exists
     */
    bool has(const std::string& human_term) const;

    /**
     * Get the number of loaded term mappings.
     * @return Number of entries in the dictionary
     */
    size_t count() const;

    /**
     * Access the global singleton instance.
     * Thread-safe in C++11 and later (magic statics).
     * @return Reference to the singleton TerminologyLookup
     */
    static TerminologyLookup& instance();

private:
    /// Populate m_terms with all hardcoded canonical mappings.
    void load_defaults();

    /// Human-term (lowercase) -> alien-term mapping.
    std::unordered_map<std::string, std::string> m_terms;
};

/**
 * Convenience function: look up the alien term for a human term.
 * Equivalent to TerminologyLookup::instance().get(human_term).
 * @param human_term The human/standard term
 * @return Reference to the alien equivalent
 */
inline const std::string& term(const std::string& human_term) {
    return TerminologyLookup::instance().get(human_term);
}

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_TERMINOLOGY_LOOKUP_H

#include "config/config_identity.h"
#include "config/config_profile.h"
#include "game/sha256.h"

#include <limits>
#include <algorithm>

namespace config {
    namespace {

        class CanonicalWriter {
          public:
            explicit CanonicalWriter(std::vector<std::uint8_t>* output)
                : output_(output) {
            }

            void write_u8(std::uint8_t value) {
                output_->push_back(value);
            }

            void write_u32(std::uint32_t value) {
                for (int shift = 24; shift >= 0; shift -= 8) {
                    write_u8(static_cast<std::uint8_t>(value >> shift));
                }
            }

            void write_u64(std::uint64_t value) {
                for (int shift = 56; shift >= 0; shift -= 8) {
                    write_u8(static_cast<std::uint8_t>(value >> shift));
                }
            }

            bool write_bytes(const void* data, std::size_t size) {
                if (size > std::numeric_limits<std::uint32_t>::max()) {
                    return false;
                }
                write_u32(static_cast<std::uint32_t>(size));
                if (size == 0) {
                    return true;
                }
                const auto* bytes = static_cast<const std::uint8_t*>(data);
                output_->insert(output_->end(), bytes, bytes + size);
                return true;
            }

          private:
            std::vector<std::uint8_t>* output_;
        };

        bool fail(const std::string& message, std::string* diagnostic) {
            if (diagnostic != nullptr) {
                *diagnostic = message;
            }
            return false;
        }

        bool write_value(CanonicalWriter* writer, const s_typed_value& value, const s_type_descriptor& type) {
            writer->write_u8(static_cast<std::uint8_t>(type.value_type));
            switch (type.value_type) {
                case CONFIG_VALUE_BOOL:
                    writer->write_u8(value.bool_value ? 1 : 0);
                    return true;
                case CONFIG_VALUE_INT8:
                case CONFIG_VALUE_UINT8:
                case CONFIG_VALUE_INT16:
                case CONFIG_VALUE_UINT16:
                case CONFIG_VALUE_INT32:
                case CONFIG_VALUE_UINT32:
                    writer->write_u64(static_cast<std::uint64_t>(value.integer_value));
                    return true;
                case CONFIG_VALUE_STRING:
                    return writer->write_bytes(value.string_value.data(),
                        value.string_value.size());
                case CONFIG_VALUE_BITMAP:
                    return writer->write_bytes(value.bitmap_value.data(),
                        value.bitmap_value.size());
                case CONFIG_VALUE_RATIO:
                    writer->write_u64(static_cast<std::uint64_t>(value.ratio_numerator));
                    writer->write_u64(static_cast<std::uint64_t>(value.ratio_denominator));
                    return true;
            }
            return false;
        }

    } // namespace

    namespace {
        struct s_identity_baseline {
            s_config_values<s_typed_value> values;
            std::vector<uint8_t> bytes;
            std::vector<size_t> offsets;
            std::vector<uint32_t> ordinals;
            std::vector<uint32_t> leaf_counts;
            uint64_t entry_count = 0;
        };

        bool Write_Identity_Variable(CanonicalWriter* writer, t_variable_id id, const s_typed_value& value) {
            const size_t count = value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values.size() : 1;
            for (uint32_t leaf = 0; leaf < count; ++leaf) {
                std::string path;
                const auto* type = Get_Variable_Leaf_Type(id, leaf);
                const auto& entry = value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values[leaf] : value;
                if (!Get_Variable_Leaf_Name(id, leaf, &path) || path.empty() || type == nullptr ||
                    entry.type != type->value_type || !writer->write_bytes(path.data(), path.size()) ||
                    !write_value(writer, entry, *type)) return false;
            }
            return true;
        }

        bool Build_Cached_Identity(const s_config_snapshot_descriptor* descriptor,
            const s_config_state* state, std::vector<uint8_t>* output) {
            static s_identity_baseline saved;
            static s_identity_baseline synchronized;
            auto& cache = descriptor == &saved_config_snapshot ? saved : synchronized;
            const auto* defaults = Get_Default_State();
            if (defaults == nullptr) return false;
            if (!cache.values.Shares_Baseline(defaults->values)) {
                cache = {};
                cache.ordinals.assign(variable_count, UINT32_MAX);
                cache.leaf_counts.assign(variable_count, 0);
                CanonicalWriter writer(&cache.bytes);
                for (uint32_t ordinal = 0; ordinal < descriptor->variable_count; ++ordinal) {
                    const auto id = descriptor->variables[ordinal];
                    cache.ordinals[id] = ordinal;
                    cache.offsets.push_back(cache.bytes.size());
                    const auto& value = defaults->values[id];
                    cache.leaf_counts[id] = static_cast<uint32_t>(value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values.size() : 1);
                    cache.entry_count += cache.leaf_counts[id];
                    if (!Write_Identity_Variable(&writer, id, value)) return false;
                }
                cache.offsets.push_back(cache.bytes.size());
                cache.values = defaults->values;
            }
            std::vector<t_variable_id> edits;
            state->values.Visit_Stored([&](size_t id, const s_typed_value&) {
                if (cache.ordinals[id] != UINT32_MAX) edits.push_back(static_cast<t_variable_id>(id));
                return true;
            });
            std::sort(edits.begin(), edits.end(), [&](auto left, auto right) {
                return cache.ordinals[left] < cache.ordinals[right];
            });
            // Keep the version-1 byte stream for existing saves and peers. Native
            // default paths/values are encoded once; only edited variables resolve again.
            std::vector<uint8_t> candidate;
            candidate.reserve(cache.bytes.size() + 12);
            CanonicalWriter writer(&candidate);
            candidate.insert(candidate.end(), {'O', '2', 'C', 'I'});
            writer.write_u32(CONFIG_IDENTITY_FORMAT_VERSION);
            uint64_t count = cache.entry_count;
            for (auto id : edits) {
                const auto& value = state->values[id];
                count -= cache.leaf_counts[id];
                count += value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values.size() : 1;
            }
            if (count > UINT32_MAX) return false;
            writer.write_u32(static_cast<uint32_t>(count));
            size_t offset = 0;
            for (auto id : edits) {
                const auto ordinal = cache.ordinals[id];
                candidate.insert(candidate.end(), cache.bytes.begin() + offset, cache.bytes.begin() + cache.offsets[ordinal]);
                if (!Write_Identity_Variable(&writer, id, state->values[id])) return false;
                offset = cache.offsets[ordinal + 1];
            }
            candidate.insert(candidate.end(), cache.bytes.begin() + offset, cache.bytes.end());
            *output = std::move(candidate);
            return true;
        }
    }

    bool Build_Sparse_Config_Identity(const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state, std::vector<uint8_t>* output, std::string* diagnostic) {
        s_config_profile profile("sparse-identity");
        if (state == nullptr || output == nullptr ||
            (descriptor != &synchronized_config_snapshot && descriptor != &saved_config_snapshot) ||
            !Validate_State(state)) return fail("invalid sparse configuration identity", diagnostic);
        const auto* defaults = Get_Default_State();
        struct s_baseline_identity {
            s_config_values<s_typed_value> values;
            orion2re::crypto::Sha256Digest digest;
        };
        static s_baseline_identity saved, synchronized;
        auto& baseline = descriptor == &saved_config_snapshot ? saved : synchronized;
        if (!baseline.values.Shares_Baseline(defaults->values)) {
            std::vector<uint8_t> bytes;
            CanonicalWriter writer(&bytes);
            writer.write_u32(descriptor->group_mask);
            writer.write_u32(static_cast<uint32_t>(schema_node_count));
            for (uint32_t index = 0; index < schema_node_count; ++index) {
                const auto& node = schema_nodes[index];
                writer.write_bytes(node.id, std::char_traits<char>::length(node.id));
                writer.write_u32(static_cast<uint32_t>(node.kind));
                writer.write_u32(node.parent);
                writer.write_u32(node.first_child);
                writer.write_u32(node.next_sibling);
                writer.write_u32(node.first_variable);
                writer.write_u32(node.last_variable);
                writer.write_u32(node.first_leaf);
                writer.write_u32(node.last_leaf);
                writer.write_u32(node.leaf_index);
                writer.write_u32(static_cast<uint32_t>(node.array_index_base));
                writer.write_u32(node.array_element_count);
                writer.write_u32(node.array_element_variable_count);
            }
            for (uint32_t index = 0; index < descriptor->variable_count; ++index) {
                const auto id = descriptor->variables[index];
                const auto& value = defaults->values[id];
                const auto& runtime = runtime_variables[id];
                writer.write_u32(id);
                writer.write_bytes(runtime.name, std::char_traits<char>::length(runtime.name));
                const uint32_t count = static_cast<uint32_t>(value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values.size() : 1);
                writer.write_u32(count);
                for (uint32_t leaf = 0; leaf < count; ++leaf) {
                    const auto* type = Get_Variable_Leaf_Type(id, leaf);
                    if (type == nullptr || !write_value(&writer, value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values[leaf] : value, *type)) return false;
                }
            }
            baseline.digest = orion2re::crypto::sha256(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
            baseline.values = defaults->values;
        }
        struct s_entry { t_variable_id id; uint32_t leaf; const s_typed_value* value; };
        std::vector<s_entry> entries;
        std::vector<std::pair<t_variable_id, uint32_t>> shapes;
        const auto collect = [&](size_t index, const s_typed_value& value) {
            const auto id = static_cast<t_variable_id>(index);
            if (!Snapshot_Contains_Variable(descriptor, id)) return true;
            const auto& base = defaults->values[id];
            if (value.type != CONFIG_VALUE_AGGREGATE) {
                if (!Config_Values_Equal(value, base)) entries.push_back({id, 0, &value});
                return true;
            }
            if (value.aggregate_values.size() != base.aggregate_values.size()) shapes.emplace_back(id, static_cast<uint32_t>(value.aggregate_values.size()));
            const auto collect_leaf = [&](size_t leaf, const s_typed_value& entry) {
                if (leaf >= base.aggregate_values.size() || !Config_Values_Equal(entry, base.aggregate_values[leaf])) entries.push_back({id, static_cast<uint32_t>(leaf), &entry});
                return true;
            };
            if (value.aggregate_values.Shares_Baseline(base.aggregate_values)) value.aggregate_values.Visit_Stored(collect_leaf);
            else for (size_t leaf = 0; leaf < value.aggregate_values.size(); ++leaf) collect_leaf(leaf, value.aggregate_values[leaf]);
            return true;
        };
        if (state->values.Shares_Baseline(defaults->values)) state->values.Visit_Stored(collect);
        else for (size_t id = 0; id < variable_count; ++id) collect(id, state->values[id]);
        std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
            return left.id < right.id || (left.id == right.id && left.leaf < right.leaf);
        });
        std::sort(shapes.begin(), shapes.end());
        std::vector<uint8_t> bytes;
        CanonicalWriter writer(&bytes);
        bytes.insert(bytes.end(), {'O', '2', 'C', 'I'});
        writer.write_u32(CONFIG_SPARSE_IDENTITY_FORMAT_VERSION);
        writer.write_bytes(baseline.digest.data(), baseline.digest.size());
        writer.write_u32(static_cast<uint32_t>(shapes.size()));
        for (const auto& shape : shapes) { writer.write_u32(shape.first); writer.write_u32(shape.second); }
        writer.write_u32(static_cast<uint32_t>(entries.size()));
        for (const auto& entry : entries) {
            writer.write_u32(entry.id);
            writer.write_u32(entry.leaf);
            if (!write_value(&writer, *entry.value, *Get_Variable_Leaf_Type(entry.id, entry.leaf))) return false;
        }
        *output = std::move(bytes);
        if (diagnostic != nullptr) diagnostic->clear();
        return true;
    }

    bool Build_Canonical_Config_Identity(
        const s_config_snapshot_descriptor* descriptor, const s_config_state* state, std::vector<std::uint8_t>* serialization, std::string* diagnostic) {
        s_config_profile profile("identity");
        if (descriptor == nullptr || descriptor->variables == nullptr ||
            descriptor->variable_count == 0 || state == nullptr ||
            state->values.size() != variable_count || serialization == nullptr ||
            !Validate_State(state)) {
            return fail("invalid synchronized configuration identity input", diagnostic);
        }

        if ((descriptor == &saved_config_snapshot || descriptor == &synchronized_config_snapshot) &&
            state->values.Shares_Baseline(Get_Default_State()->values)) {
            if (!Build_Cached_Identity(descriptor, state, serialization)) return fail("invalid configuration identity", diagnostic);
            if (diagnostic != nullptr) diagnostic->clear();
            return true;
        }

        std::vector<std::uint8_t> candidate;
        CanonicalWriter writer(&candidate);
        candidate.insert(candidate.end(), {'O', '2', 'C', 'I'});
        writer.write_u32(CONFIG_IDENTITY_FORMAT_VERSION);
        std::uint32_t entry_count = 0;
        std::vector<bool> seen(variable_count, false);
        std::string previous_path;
        for (std::uint32_t index = 0; index < descriptor->variable_count; ++index) {
            const t_variable_id variable_id = descriptor->variables[index];
            std::string path;
            if (variable_id >= variable_count || seen[variable_id] ||
                (descriptor->group_mask &
                    (UINT32_C(1) << Get_Variable_Group(variable_id))) == 0 ||
                !Get_Variable_Name(variable_id, &path) || path.empty() ||
                (!previous_path.empty() && previous_path >= path)) {
                return fail("invalid synchronized configuration identity descriptor",
                    diagnostic);
            }
            seen[variable_id] = true;
            previous_path = std::move(path);
            const s_typed_value& value = state->values[variable_id];
            const std::size_t count = value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values.size() : 1;
            if (count > std::numeric_limits<std::uint32_t>::max() - entry_count) {
                return fail("synchronized configuration identity is too large", diagnostic);
            }
            entry_count += static_cast<std::uint32_t>(count);
        }
        writer.write_u32(entry_count);
        for (std::uint32_t index = 0; index < descriptor->variable_count; ++index) {
            const t_variable_id variable_id = descriptor->variables[index];
            const s_typed_value& value = state->values[variable_id];
            const std::size_t leaf_count = value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values.size() : 1;
            for (std::uint32_t leaf = 0; leaf < leaf_count; ++leaf) {
                std::string path;
                if (!Get_Variable_Leaf_Name(variable_id, leaf, &path) || path.empty()) {
                    return fail("invalid synchronized configuration identity descriptor",
                        diagnostic);
                }
                const s_type_descriptor* type = Get_Variable_Leaf_Type(variable_id, leaf);
                const s_typed_value& leaf_value = value.type == CONFIG_VALUE_AGGREGATE ? value.aggregate_values[leaf] : value;
                if (type == nullptr || leaf_value.type != type->value_type ||
                    !writer.write_bytes(path.data(), path.size()) ||
                    !write_value(&writer, leaf_value, *type)) {
                    return fail("invalid synchronized configuration identity value",
                        diagnostic);
                }
            }
        }

        if (diagnostic != nullptr) {
            diagnostic->clear();
        }
        *serialization = std::move(candidate);
        return true;
    }

} // namespace config
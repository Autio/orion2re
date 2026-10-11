#include "config_lifecycle.h"

#include "config/config_bits.h"
#include "config/config_decode.h"

namespace config {
    namespace {
        bool Fail(const s_source_location& location, const std::string& message, s_cfg_diagnostic* diagnostic) {
            diagnostic->location = location;
            diagnostic->message = message;
            return false;
        }

        void Reset_Metadata(s_effective_config* effective) {
            effective->provenance.Initialize_Empty(variable_count);
            effective->prior_values.Initialize_Empty(variable_count);
        }

        void Clear_Delta(s_config_delta* delta) {
            delta->variables.assign((variable_count + 63) / 64, 0);
            delta->variable_ids.clear();
            delta->aggregates.clear();
        }

        s_config_delta::s_aggregate* Find_Aggregate(
            s_config_delta* delta,
            t_variable_id variable_id) {
            for (s_config_delta::s_aggregate& aggregate : delta->aggregates) {
                if (aggregate.variable_id == variable_id) return &aggregate;
            }
            return nullptr;
        }

        const s_config_delta::s_aggregate* Find_Aggregate(
            const s_config_delta* delta,
            t_variable_id variable_id) {
            for (const s_config_delta::s_aggregate& aggregate :
                delta->aggregates) {
                if (aggregate.variable_id == variable_id) return &aggregate;
            }
            return nullptr;
        }

        void Record_Delta(s_config_delta* delta,
            t_variable_id variable_id,
            uint32_t leaf_index) {
            const uint64_t mask = UINT64_C(1) << (variable_id % 64);
            uint64_t& word = delta->variables[variable_id / 64];
            if ((word & mask) == 0) {
                word |= mask;
                delta->variable_ids.push_back(variable_id);
            }
            if (!runtime_variables[variable_id].aggregate) return;
            s_config_delta::s_aggregate* aggregate =
                Find_Aggregate(delta, variable_id);
            if (aggregate == nullptr) {
                delta->aggregates.emplace_back();
                aggregate = &delta->aggregates.back();
                aggregate->variable_id = variable_id;
            }
            const size_t word_index = leaf_index / 64;
            if (aggregate->leaves.size() <= word_index) {
                aggregate->leaves.resize(word_index + 1, 0);
            }
            aggregate->leaves[word_index] |= UINT64_C(1) << (leaf_index % 64);
        }

        void Record_State_Transition(s_config_delta* delta,
            const s_config_state* prior,
            const s_config_state* current,
            t_variable_id variable_id) {
            const s_typed_value& prior_value = prior->values[variable_id];
            const s_typed_value& current_value = current->values[variable_id];
            if (prior_value.type != CONFIG_VALUE_AGGREGATE ||
                current_value.type != CONFIG_VALUE_AGGREGATE) {
                if (!Config_Values_Equal(prior_value, current_value)) {
                    Record_Delta(delta, variable_id, 0);
                }
                return;
            }
            const bool resized = prior_value.aggregate_values.size() !=
                current_value.aggregate_values.size();
            if (State_Tracks_Non_Default_Values(prior) &&
                State_Tracks_Non_Default_Values(current)) {
                const s_non_default_aggregate* prior_aggregate =
                    Get_State_Non_Default_Aggregate(prior, variable_id);
                const s_non_default_aggregate* current_aggregate =
                    Get_State_Non_Default_Aggregate(current, variable_id);
                static const std::vector<uint64_t> empty;
                const std::vector<uint64_t>& prior_leaves =
                    prior_aggregate != nullptr
                    ? prior_aggregate->leaves
                    : empty;
                const std::vector<uint64_t>& current_leaves =
                    current_aggregate != nullptr
                    ? current_aggregate->leaves
                    : empty;
                const size_t word_count = std::max(
                    prior_leaves.size(), current_leaves.size());
                for (size_t word_index = 0;
                     word_index < word_count;
                     ++word_index) {
                    uint64_t word = word_index < prior_leaves.size()
                        ? prior_leaves[word_index]
                        : 0;
                    if (word_index < current_leaves.size()) {
                        word |= current_leaves[word_index];
                    }
                    while (word != 0) {
                        const unsigned int bit = Count_Trailing_Zero_Bits(word);
                        const size_t leaf = word_index * 64 + bit;
                        if (leaf >= prior_value.aggregate_values.size() ||
                            leaf >= current_value.aggregate_values.size() ||
                            !Config_Values_Equal(
                                prior_value.aggregate_values[leaf],
                                current_value.aggregate_values[leaf])) {
                            Record_Delta(delta,
                                variable_id,
                                static_cast<uint32_t>(leaf));
                        }
                        word &= word - 1;
                    }
                }
            } else {
                const size_t maximum_size = std::max(
                    prior_value.aggregate_values.size(),
                    current_value.aggregate_values.size());
                for (size_t leaf = 0; leaf < maximum_size; ++leaf) {
                    if (leaf >= prior_value.aggregate_values.size() ||
                        leaf >= current_value.aggregate_values.size() ||
                        !Config_Values_Equal(prior_value.aggregate_values[leaf],
                            current_value.aggregate_values[leaf])) {
                        Record_Delta(delta,
                            variable_id,
                            static_cast<uint32_t>(leaf));
                    }
                }
            }
            if (resized) {
                s_config_delta::s_aggregate* aggregate =
                    Find_Aggregate(delta, variable_id);
                if (aggregate == nullptr) {
                    Record_Delta(delta, variable_id, 0);
                    aggregate = Find_Aggregate(delta, variable_id);
                }
                if (aggregate != nullptr) aggregate->vector_length = true;
            }
        }

        bool Apply_Document(const s_cfg_document* document,
            const char* package_id,
            s_effective_config* candidate,
            s_config_delta* touched,
            s_cfg_diagnostic* diagnostic) {
            if (document == nullptr || package_id == nullptr || candidate == nullptr ||
                diagnostic == nullptr) {
                return false;
            }
            *diagnostic = {};
            if (candidate->state.values.size() != variable_count ||
                candidate->provenance.size() != variable_count ||
                candidate->prior_values.size() != variable_count) {
                diagnostic->message = "effective configuration is not initialized";
                return false;
            }

            for (size_t statement_index = 0;
                 statement_index < document->statements.size();
                 ++statement_index) {
                const s_cfg_statement& statement = document->statements[statement_index];
                if (statement.kind == CFG_STATEMENT_STOP) break;
                if (statement.kind == CFG_STATEMENT_INCLUDE) {
                    return Fail(statement.location,
                        "include execution requires the configuration loader",
                        diagnostic);
                }
                if (statement.kind == CFG_STATEMENT_ENABLE) continue;

                s_config_write_set statement_writes;
                if (!Decode_Assignment(&statement, &statement_writes, diagnostic)) {
                    return false;
                }
                std::vector<s_prior_value> statement_prior_values;
                statement_prior_values.reserve(statement_writes.writes.size());
                std::vector<size_t> statement_prior_sizes;
                statement_prior_sizes.reserve(statement_writes.writes.size());
                for (const s_config_write& write : statement_writes.writes) {
                    const size_t variable_index = write.variable_id;
                    const s_typed_value& variable =
                        candidate->state.values.Read(write.variable_id);
                    statement_prior_sizes.push_back(
                        variable.type == CONFIG_VALUE_AGGREGATE
                            ? variable.aggregate_values.size()
                            : 1);
                    statement_prior_values.emplace_back();
                    s_prior_value& prior = statement_prior_values.back();
                    prior.leaf_index = write.leaf_index;
                    if (!Read_State_Leaf_Value(&candidate->state,
                            write.variable_id,
                            write.leaf_index,
                            &prior.value)) {
                        const s_runtime_variable_descriptor& descriptor =
                            runtime_variables[write.variable_id];
                        if (!descriptor.vector ||
                            !generated::Get_Default_Leaf_Value(
                                write.variable_id,
                                write.leaf_index,
                                &prior.value)) {
                            return Fail(write.provenance,
                                "configuration state accessor is missing",
                                diagnostic);
                        }
                    }
                    prior.provenance = candidate->provenance.Read(variable_index);
                }

                if (!Apply_Write_Set(
                        &statement_writes, &candidate->state, diagnostic)) {
                    return false;
                }

                for (size_t write_index = 0;
                     write_index < statement_writes.writes.size();
                     ++write_index) {
                    const s_config_write& write =
                        statement_writes.writes[write_index];
                    const size_t variable_index = write.variable_id;
                    candidate->prior_values[variable_index].push_back(
                        std::move(statement_prior_values[write_index]));
                    s_value_provenance provenance;
                    provenance.has_override = true;
                    provenance.package_id = package_id;
                    provenance.location = write.provenance;
                    provenance.statement = statement.statement_number != 0
                        ? statement.statement_number
                        : static_cast<uint32_t>(statement_index + 1);
                    candidate->provenance[variable_index] = std::move(provenance);
                    if (touched != nullptr) {
                        Record_Delta(touched,
                            write.variable_id,
                            write.leaf_index);
                        const s_runtime_variable_descriptor& descriptor =
                            runtime_variables[write.variable_id];
                        if (descriptor.vector) {
                            s_config_delta::s_aggregate* aggregate =
                                Find_Aggregate(touched, write.variable_id);
                            if (aggregate != nullptr &&
                                statement_prior_sizes[write_index] !=
                                    candidate->state.values.Read(write.variable_id)
                                        .aggregate_values.size()) {
                                aggregate->vector_length = true;
                            }
                        }
                    }
                }
            }
            return true;
        }
    } // namespace

    bool Initialize_Effective_Config(s_effective_config* effective) {
        if (effective == nullptr ||
            !Initialize_Tracked_Defaults(&effective->state)) {
            return false;
        }
        Reset_Metadata(effective);
        return true;
    }

    bool Read_Effective_Native_Config(s_effective_config* effective) {
        if (effective == nullptr || !Read_Current_State(&effective->state) ||
            !Track_State_Non_Default_Values(&effective->state)) {
            return false;
        }
        Reset_Metadata(effective);
        return true;
    }

    bool Freeze_Config_Baseline(s_effective_config* effective,
        s_config_baseline* baseline) {
        if (effective == nullptr || baseline == nullptr ||
            !Validate_State(&effective->state)) {
            return false;
        }
        baseline->effective = std::move(*effective);
        return true;
    }

    bool Begin_Config_Stage(const s_config_baseline* baseline,
        s_config_stage* stage) {
        if (baseline == nullptr || stage == nullptr ||
            baseline->effective.state.values.size() != variable_count) {
            return false;
        }
        stage->baseline = baseline;
        stage->effective = baseline->effective;
        Clear_Delta(&stage->touched);
        return true;
    }

    bool Begin_Default_Config_Stage(s_config_stage* stage) {
        if (stage == nullptr || !Initialize_Effective_Config(&stage->effective)) {
            return false;
        }
        stage->baseline = nullptr;
        Clear_Delta(&stage->touched);
        return true;
    }

    bool Rollback_Config_Stage(s_config_stage* stage) {
        if (stage == nullptr || stage->baseline == nullptr) return false;
        for (t_variable_id variable_id : stage->touched.variable_ids) {
            const s_typed_value& baseline_value =
                stage->baseline->effective.state.values[variable_id];
            if (!runtime_variables[variable_id].aggregate) {
                if (!Set_State_Value(&stage->effective.state,
                        variable_id,
                        baseline_value)) {
                    return false;
                }
            } else {
                const s_config_delta::s_aggregate* aggregate =
                    Find_Aggregate(&stage->touched, variable_id);
                if (aggregate == nullptr || aggregate->vector_length ||
                    stage->effective.state.values.Read(variable_id)
                            .aggregate_values.size() !=
                        baseline_value.aggregate_values.size()) {
                    if (!Set_State_Value(&stage->effective.state,
                            variable_id,
                            baseline_value)) {
                        return false;
                    }
                } else {
                    for (size_t word_index = 0;
                         word_index < aggregate->leaves.size();
                         ++word_index) {
                        uint64_t word = aggregate->leaves[word_index];
                        while (word != 0) {
                            const unsigned int bit = Count_Trailing_Zero_Bits(word);
                            const uint32_t leaf = static_cast<uint32_t>(
                                word_index * 64 + bit);
                            if (leaf >= baseline_value.aggregate_values.size() ||
                                !Set_State_Leaf_Value(&stage->effective.state,
                                    variable_id,
                                    leaf,
                                    baseline_value.aggregate_values[leaf])) {
                                return false;
                            }
                            word &= word - 1;
                        }
                    }
                }
            }
            stage->effective.provenance[variable_id] =
                stage->baseline->effective.provenance[variable_id];
            stage->effective.prior_values[variable_id] =
                stage->baseline->effective.prior_values[variable_id];
        }
        Clear_Delta(&stage->touched);
        stage->effective.state.validated = stage->baseline->effective.state.validated;
        return true;
    }

    bool Freeze_Config_Stage(s_config_stage* stage,
        s_config_baseline* baseline) {
        if (stage == nullptr || baseline == nullptr ||
            !Validate_State(&stage->effective.state)) {
            return false;
        }
        baseline->effective = std::move(stage->effective);
        stage->baseline = nullptr;
        Clear_Delta(&stage->touched);
        return true;
    }

    bool Finish_Config_Stage(s_config_stage* stage,
        s_effective_config* effective) {
        if (stage == nullptr || effective == nullptr ||
            !Validate_State(&stage->effective.state)) {
            return false;
        }
        *effective = std::move(stage->effective);
        stage->baseline = nullptr;
        Clear_Delta(&stage->touched);
        return true;
    }

    bool Config_Delta_Contains(const s_config_delta* delta,
        t_variable_id variable_id,
        uint32_t leaf_index) {
        if (delta == nullptr || variable_id >= variable_count ||
            delta->variables.size() != (variable_count + 63) / 64 ||
            (delta->variables[variable_id / 64] &
                (UINT64_C(1) << (variable_id % 64))) == 0) {
            return false;
        }
        if (!runtime_variables[variable_id].aggregate) return leaf_index == 0;
        const s_config_delta::s_aggregate* aggregate =
            Find_Aggregate(delta, variable_id);
        return aggregate != nullptr &&
            leaf_index / 64 < aggregate->leaves.size() &&
            (aggregate->leaves[leaf_index / 64] &
                (UINT64_C(1) << (leaf_index % 64))) != 0;
    }

    bool Apply_Config_State_To_Stage(
        const s_config_snapshot_descriptor* descriptor,
        const s_config_state* state,
        s_config_stage* stage) {
        if (descriptor == nullptr || descriptor->variables == nullptr ||
            descriptor->variable_count == 0 || state == nullptr ||
            stage == nullptr || state->values.size() != variable_count ||
            stage->effective.state.values.size() != variable_count) {
            return false;
        }
        if (State_Tracks_Non_Default_Values(state) &&
            State_Tracks_Non_Default_Values(&stage->effective.state)) {
            std::vector<uint64_t> included((variable_count + 63) / 64, 0);
            for (uint32_t index = 0;
                 index < descriptor->variable_count;
                 ++index) {
                const t_variable_id variable_id = descriptor->variables[index];
                if (variable_id >= variable_count ||
                    (descriptor->group_mask &
                        (UINT32_C(1) << Get_Variable_Group(variable_id))) == 0) {
                    return false;
                }
                included[variable_id / 64] |=
                    UINT64_C(1) << (variable_id % 64);
            }
            std::vector<uint64_t> visited(included.size(), 0);
            for (t_variable_id variable_id :
                stage->effective.state.non_default_variable_ids) {
                const uint64_t mask = UINT64_C(1) << (variable_id % 64);
                if ((included[variable_id / 64] & mask) == 0) continue;
                Record_State_Transition(&stage->touched,
                    &stage->effective.state,
                    state,
                    variable_id);
                visited[variable_id / 64] |= mask;
            }
            for (t_variable_id variable_id : state->non_default_variable_ids) {
                const uint64_t mask = UINT64_C(1) << (variable_id % 64);
                if ((included[variable_id / 64] & mask) == 0 ||
                    (visited[variable_id / 64] & mask) != 0) {
                    continue;
                }
                Record_State_Transition(&stage->touched,
                    &stage->effective.state,
                    state,
                    variable_id);
            }
            return Overlay_Config_State(
                descriptor, state, &stage->effective.state);
        }
        for (uint32_t index = 0;
             index < descriptor->variable_count;
             ++index) {
            const t_variable_id variable_id = descriptor->variables[index];
            if (variable_id >= variable_count ||
                (descriptor->group_mask &
                    (UINT32_C(1) << Get_Variable_Group(variable_id))) == 0) {
                return false;
            }
            Record_State_Transition(&stage->touched,
                &stage->effective.state,
                state,
                variable_id);
        }
        return Overlay_Config_State(
            descriptor, state, &stage->effective.state);
    }

    bool Apply_Config_Document_To_Stage(const s_cfg_document* document,
        const char* package_id,
        s_config_stage* stage,
        s_cfg_diagnostic* diagnostic) {
        return stage != nullptr &&
            Apply_Document(document,
                package_id,
                &stage->effective,
                &stage->touched,
                diagnostic);
    }

    bool Apply_Config_Document_To_Candidate(const s_cfg_document* document,
        const char* package_id,
        s_effective_config* candidate,
        s_cfg_diagnostic* diagnostic) {
        return Apply_Document(document,
            package_id,
            candidate,
            nullptr,
            diagnostic);
    }

    bool Apply_Config_Document(const s_cfg_document* document, const char* package_id, s_effective_config* effective, s_cfg_diagnostic* diagnostic) {
        if (document == nullptr || package_id == nullptr || effective == nullptr ||
            diagnostic == nullptr) {
            return false;
        }
        s_effective_config candidate = *effective;
        if (!Apply_Config_Document_To_Candidate(document, package_id, &candidate, diagnostic)) {
            return false;
        }

        static_assert(CONFIG_GROUP_COUNT <= 32);
        const uint32_t group_mask = CONFIG_GROUP_COUNT == 32
            ? UINT32_MAX
            : (UINT32_C(1) << CONFIG_GROUP_COUNT) - 1;
        if (!Commit_Config_Transition(
                group_mask, &effective->state, &candidate.state)) {
            diagnostic->message = "unable to commit configuration state";
            return false;
        }
        *effective = std::move(candidate);
        return true;
    }

    bool Merge_Config_Group(t_config_group group, const s_effective_config* candidate, s_effective_config* effective) {
        if (group < 0 || group >= CONFIG_GROUP_COUNT || candidate == nullptr ||
            effective == nullptr || candidate->state.values.size() != variable_count ||
            candidate->provenance.size() != variable_count ||
            candidate->prior_values.size() != variable_count ||
            effective->state.values.size() != variable_count ||
            effective->provenance.size() != variable_count ||
            effective->prior_values.size() != variable_count) {
            return false;
        }
        if (candidate == effective) return true;
        if (candidate->state.values.Shares_Baseline(effective->state.values)) {
            std::vector<t_variable_id> ids;
            const auto collect = [&](size_t index, const auto&) {
                const auto id = static_cast<t_variable_id>(index);
                if (Get_Variable_Group(id) == group) ids.push_back(id);
                return true;
            };
            candidate->state.values.Visit_Stored(collect);
            effective->state.values.Visit_Stored(collect);
            candidate->provenance.Visit_Stored(collect);
            effective->provenance.Visit_Stored(collect);
            candidate->prior_values.Visit_Stored(collect);
            effective->prior_values.Visit_Stored(collect);
            std::sort(ids.begin(), ids.end());
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            for (auto id : ids) {
                if (!Copy_State_Value(&effective->state, &candidate->state, id)) return false;
            }
            effective->provenance.Erase_Stored_If([&](size_t id) { return Get_Variable_Group(static_cast<t_variable_id>(id)) == group; });
            effective->prior_values.Erase_Stored_If([&](size_t id) { return Get_Variable_Group(static_cast<t_variable_id>(id)) == group; });
            candidate->provenance.Visit_Stored([&](size_t id, const auto& value) {
                if (Get_Variable_Group(static_cast<t_variable_id>(id)) == group) effective->provenance[id] = value;
                return true;
            });
            candidate->prior_values.Visit_Stored([&](size_t id, const auto& value) {
                if (Get_Variable_Group(static_cast<t_variable_id>(id)) == group) effective->prior_values[id] = value;
                return true;
            });
            return true;
        }
        for (t_variable_id variable_id = 0; variable_id < variable_count; ++variable_id) {
            if (Get_Variable_Group(variable_id) != group) continue;
            if (!Copy_State_Value(&effective->state,
                    &candidate->state,
                    variable_id)) {
                return false;
            }
            effective->provenance[variable_id] = candidate->provenance[variable_id];
            effective->prior_values[variable_id] = candidate->prior_values[variable_id];
        }
        return true;
    }

    bool Apply_Config_Group(t_config_group group, const s_effective_config* candidate, s_effective_config* effective) {
        if (effective == nullptr) {
            return false;
        }
        s_effective_config merged = *effective;
        if (!Merge_Config_Group(group, candidate, &merged) ||
            !Commit_Config_Transition(UINT32_C(1) << group,
                &effective->state,
                &merged.state)) {
            return false;
        }
        *effective = std::move(merged);
        return true;
    }

} // namespace config
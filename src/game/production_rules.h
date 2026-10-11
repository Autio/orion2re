#pragma once

namespace production_rules {
    constexpr int32_t METRIC_COUNT = 40;
    constexpr int32_t FLAG_COUNT = 24;
    constexpr int32_t VALUE_CONDITION_COUNT = 2;
    constexpr int32_t METRIC_CONDITION_COUNT = 2;
    constexpr int32_t SCALED_CONDITION_COUNT = 2;
    constexpr int32_t ALTERNATIVE_COUNT = 2;
    constexpr int32_t METRIC_TERM_COUNT = 3;
    constexpr int32_t VALUE_TERM_COUNT = 3;

    enum e_metric : uint8_t {
        METRIC_NONE,
        METRIC_POPULATION,
        METRIC_POPULATION_LIMIT,
        METRIC_FREE_POPULATION_CAPACITY,
        METRIC_COLONY_FOOD,
        METRIC_COLONY_FOOD_DEMAND,
        METRIC_PRODUCTION,
        METRIC_INDUSTRY,
        METRIC_POLLUTION,
        METRIC_FOOD_PER_FARMER,
        METRIC_GRAVITY_MISMATCH,
        METRIC_CLIMATE,
        METRIC_MINERAL_CLASS,
        METRIC_INCOMING_HUMAN_SHIPS,
        METRIC_ENEMY_PRESENCE,
        METRIC_NEUTRAL_PRESENCE,
        METRIC_ALLIED_PRESENCE,
        METRIC_OUT_OF_RANGE_PRESENCE,
        METRIC_EMPIRE_FOOD_SURPLUS,
        METRIC_EMPIRE_TREASURY,
        METRIC_EMPIRE_BC_SURPLUS,
        METRIC_COMMAND_POINT_DEFICIT,
        METRIC_WAR_LEVEL,
        METRIC_CURRENT_STRENGTH,
        METRIC_DESIRED_STRENGTH,
        METRIC_COMBAT_NEED,
        METRIC_FREIGHTER_NEED,
        METRIC_SPY_NEED,
        METRIC_TRANSPORT_NEED,
        METRIC_REFIT_NEED,
        METRIC_EXISTING_COLONY_SHIPS,
        METRIC_TREASURY_SCORE,
        METRIC_LOCAL_THREAT,
        METRIC_EXTENSION_1,
        METRIC_EXTENSION_2,
        METRIC_EXTENSION_3,
        METRIC_EXTENSION_4,
        METRIC_EXTENSION_5,
        METRIC_EXTENSION_6,
        METRIC_EXTENSION_7,
    };

    enum e_flag : uint8_t {
        FLAG_CONQUERED,
        FLAG_BUILDING_FOCUS,
        FLAG_RESEARCH_ENABLED,
        FLAG_THREATENED,
        FLAG_AT_WAR,
        FLAG_IS_HOMEWORLD,
        FLAG_EMPIRE_FOOD_DEFICIT,
        FLAG_EMPIRE_LOW_MONEY,
        FLAG_OBJECTIVE_ECOLOGIST,
        FLAG_OBJECTIVE_INDUSTRIALIST,
        FLAG_OBJECTIVE_MILITARIST,
        FLAG_OBJECTIVE_TECHNOLOGIST,
        FLAG_FOOD_BUILDINGS_APPLICABLE,
        FLAG_POLLUTION_CONTROLS_APPLICABLE,
        FLAG_ORBITAL_DEFENSE_APPLICABLE,
        FLAG_PLANETARY_DEFENSE_APPLICABLE,
        FLAG_COMBAT_SHIP_AVAILABLE,
        FLAG_TRANSPORT_DESIGN_AVAILABLE,
        FLAG_REFIT_AVAILABLE,
        FLAG_COLONIZABLE_PLANET_AVAILABLE,
        FLAG_SAME_SYSTEM_TARGET_AVAILABLE,
        FLAG_POPULATION_OR_WEALTH_GATE,
        FLAG_MISSING_ARMOR_BARRACKS_FOR_GOVERNMENT,
        FLAG_MISSING_MARINE_BARRACKS_FOR_GOVERNMENT,
    };

    enum e_requirement : uint8_t {
        REQUIREMENT_IGNORED,
        REQUIREMENT_REQUIRED,
        REQUIREMENT_FORBIDDEN,
    };

    enum e_comparison : uint8_t {
        COMPARISON_IGNORE,
        COMPARISON_EQ,
        COMPARISON_NE,
        COMPARISON_LT,
        COMPARISON_LE,
        COMPARISON_GT,
        COMPARISON_GE,
    };

    enum e_transform : uint8_t {
        TRANSFORM_LINEAR,
        TRANSFORM_SQRT,
    };

    enum e_cutoff_mode : uint8_t {
        CUTOFF_DISABLED,
        CUTOFF_RELATIVE,
    };

    enum e_cutoff_stage : uint8_t {
        CUTOFF_STAGE_NORMAL,
        CUTOFF_STAGE_PROTECTED,
        CUTOFF_STAGE_AFTER,
    };

    enum e_weight_formula : uint8_t {
        WEIGHT_FORMULA_ADDITIVE,
        WEIGHT_FORMULA_LEGACY,
        WEIGHT_FORMULA_COMBAT_NEED_RATIO,
        WEIGHT_FORMULA_STRATEGIC_NEED,
        WEIGHT_FORMULA_DEFER,
    };

    enum e_wartime_factor : uint8_t {
        WARTIME_FACTOR_NONE,
        WARTIME_FACTOR_RATIO,
        WARTIME_FACTOR_INVERSE_WAR_LEVEL_SQUARED,
    };

    struct s_context {
        int32_t metrics[METRIC_COUNT];
        uint8_t flags[FLAG_COUNT];
        int8_t government_family;
        int8_t climate;
        int8_t mineral_class;
    };

    struct s_clamp {
        int32_t minimum;
        int32_t maximum;
    };

    struct s_threshold {
        uint8_t operator_;
        int32_t value;
    };

    struct s_range {
        s_threshold minimum;
        s_threshold maximum;
    };

    struct s_value_condition {
        uint8_t metric;
        uint8_t operator_;
        int32_t value;
    };

    struct s_metric_condition {
        uint8_t left;
        uint8_t operator_;
        uint8_t right;
    };

    struct s_scaled_condition {
        uint8_t left_metric;
        int32_t left_factor;
        uint8_t operator_;
        uint8_t right_metric;
        int32_t right_factor;
    };

    struct s_enum_condition {
        uint8_t operator_;
        int8_t value;
    };

    struct s_condition_group {
        uint8_t flags[FLAG_COUNT];
        s_threshold thresholds[METRIC_COUNT];
        s_range ranges[METRIC_COUNT];
        s_value_condition value_conditions[VALUE_CONDITION_COUNT];
        s_metric_condition metric_conditions[METRIC_CONDITION_COUNT];
        s_scaled_condition scaled_conditions[SCALED_CONDITION_COUNT];
        s_enum_condition government_family;
        s_enum_condition climate;
        s_enum_condition mineral_class;
    };

    struct s_eligibility {
        s_condition_group all;
        s_condition_group alternatives[ALTERNATIVE_COUNT];
    };

    struct s_term_condition {
        uint8_t flags[FLAG_COUNT];
        s_threshold thresholds[METRIC_COUNT];
    };

    struct s_metric_term {
        uint8_t metric;
        uint8_t transform;
        s_ratio16 factor;
        s_clamp clamp;
        s_term_condition when;
    };

    struct s_value_term {
        int32_t value;
        s_clamp clamp;
        s_term_condition when;
    };

    struct s_combat_need_ratio {
        int32_t scale;
        int32_t wartime_multiplier;
    };

    struct s_strategic_need {
        uint8_t metric;
        int32_t base_need;
        int32_t need_multiplier;
        int32_t result_multiplier;
        uint8_t wartime_factor;
        s_ratio16 wartime_ratio;
    };

    struct s_defer_weight {
        int32_t peace;
        int32_t wartime_base;
        int32_t wartime_penalty_per_level;
    };

    struct s_weight {
        int32_t base;
        uint8_t formula;
        s_clamp clamp;
        s_metric_term metric_terms[METRIC_TERM_COUNT];
        s_value_term value_terms[VALUE_TERM_COUNT];
        s_combat_need_ratio combat_need_ratio;
        s_strategic_need strategic_need;
        s_defer_weight defer;
    };

    struct s_candidate {
        bool enabled;
        uint8_t cutoff_stage;
        s_eligibility eligibility;
        s_weight weight;
    };

    struct s_cutoff {
        uint8_t mode;
        int32_t multiplier[6];
    };

    bool Evaluate_Eligibility(const s_context* context, const s_eligibility* eligibility);
    int32_t Evaluate_Weight(const s_context* context, const s_weight* weight, int32_t legacy_weight);
    void Apply_Cutoff(const s_cutoff* cutoff, int32_t multiplier_index, const s_candidate* candidates, int32_t* weights, int32_t count);
} // namespace production_rules
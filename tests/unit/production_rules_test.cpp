#include "pch.h"

#include "doctest/doctest.h"

namespace {
    production_rules::s_clamp Full_Clamp_() {
        return {INT32_MIN, INT32_MAX};
    }
} // namespace

DOCTEST_TEST_CASE("production eligibility evaluates flags comparisons ranges and alternatives") {
    production_rules::s_context context = {};
    production_rules::s_eligibility eligibility = {};
    context.flags[production_rules::FLAG_RESEARCH_ENABLED] = 1;
    context.metrics[production_rules::METRIC_INDUSTRY] = 20;
    context.metrics[production_rules::METRIC_POPULATION] = 4;
    context.metrics[production_rules::METRIC_POPULATION_LIMIT] = 12;

    DOCTEST_CHECK(production_rules::Evaluate_Eligibility(&context, &eligibility));

    eligibility.all.flags[production_rules::FLAG_RESEARCH_ENABLED] = production_rules::REQUIREMENT_REQUIRED;
    eligibility.all.flags[production_rules::FLAG_BUILDING_FOCUS] = production_rules::REQUIREMENT_FORBIDDEN;
    eligibility.all.thresholds[production_rules::METRIC_INDUSTRY] = {production_rules::COMPARISON_GE, 15};
    eligibility.all.ranges[production_rules::METRIC_POPULATION].minimum = {production_rules::COMPARISON_GE, 2};
    eligibility.all.ranges[production_rules::METRIC_POPULATION].maximum = {production_rules::COMPARISON_LE, 8};
    eligibility.all.scaled_conditions[0] = {
        production_rules::METRIC_POPULATION, 3, production_rules::COMPARISON_LE, production_rules::METRIC_POPULATION_LIMIT, 1};
    DOCTEST_CHECK(production_rules::Evaluate_Eligibility(&context, &eligibility));

    eligibility.alternatives[0].flags[production_rules::FLAG_CONQUERED] = production_rules::REQUIREMENT_REQUIRED;
    eligibility.alternatives[1].flags[production_rules::FLAG_AT_WAR] = production_rules::REQUIREMENT_REQUIRED;
    eligibility.alternatives[1].thresholds[production_rules::METRIC_INCOMING_HUMAN_SHIPS] = {
        production_rules::COMPARISON_GT, 0};
    DOCTEST_CHECK_FALSE(production_rules::Evaluate_Eligibility(&context, &eligibility));
    context.flags[production_rules::FLAG_AT_WAR] = 1;
    context.metrics[production_rules::METRIC_INCOMING_HUMAN_SHIPS] = 2;
    DOCTEST_CHECK(production_rules::Evaluate_Eligibility(&context, &eligibility));
}

DOCTEST_TEST_CASE("production weights use deterministic factors transforms conditions and clamps") {
    production_rules::s_context context = {};
    production_rules::s_weight weight = {};
    weight.clamp = {0, 30};
    weight.base = 10;
    context.metrics[production_rules::METRIC_POPULATION] = 4;
    context.metrics[production_rules::METRIC_FREE_POPULATION_CAPACITY] = 12;
    context.flags[production_rules::FLAG_OBJECTIVE_INDUSTRIALIST] = 1;

    weight.metric_terms[0].metric = production_rules::METRIC_POPULATION;
    weight.metric_terms[0].factor = {3, 2};
    weight.metric_terms[0].clamp = Full_Clamp_();
    weight.metric_terms[1].metric = production_rules::METRIC_FREE_POPULATION_CAPACITY;
    weight.metric_terms[1].transform = production_rules::TRANSFORM_SQRT;
    weight.metric_terms[1].factor = {3, 1};
    weight.metric_terms[1].clamp = Full_Clamp_();
    weight.value_terms[0].value = 8;
    weight.value_terms[0].clamp = Full_Clamp_();
    weight.value_terms[0].when.flags[production_rules::FLAG_OBJECTIVE_INDUSTRIALIST] =
        production_rules::REQUIREMENT_REQUIRED;

    DOCTEST_CHECK(production_rules::Evaluate_Weight(&context, &weight, 0) == 30);
    weight.value_terms[0].value = -4;
    DOCTEST_CHECK(production_rules::Evaluate_Weight(&context, &weight, 0) == 18);
}

DOCTEST_TEST_CASE("production cutoff supports normal protected and after candidates") {
    production_rules::s_cutoff cutoff = {};
    cutoff.mode = production_rules::CUTOFF_RELATIVE;
    cutoff.multiplier[0] = 2;
    production_rules::s_candidate candidates[4] = {};
    candidates[0].cutoff_stage = production_rules::CUTOFF_STAGE_NORMAL;
    candidates[1].cutoff_stage = production_rules::CUTOFF_STAGE_NORMAL;
    candidates[2].cutoff_stage = production_rules::CUTOFF_STAGE_PROTECTED;
    candidates[3].cutoff_stage = production_rules::CUTOFF_STAGE_AFTER;
    int32_t weights[4] = {1000, 499, 1, 2};

    production_rules::Apply_Cutoff(&cutoff, 0, candidates, weights, 4);
    DOCTEST_CHECK(weights[0] == 1000);
    DOCTEST_CHECK(weights[1] == 0);
    DOCTEST_CHECK(weights[2] == 1);
    DOCTEST_CHECK(weights[3] == 2);
}

DOCTEST_TEST_CASE("disabled production cutoff leaves weights unchanged") {
    production_rules::s_cutoff cutoff = {};
    cutoff.mode = production_rules::CUTOFF_DISABLED;
    production_rules::s_candidate candidates[2] = {};
    int32_t weights[2] = {1200, -25};

    production_rules::Apply_Cutoff(&cutoff, 0, candidates, weights, 2);
    DOCTEST_CHECK(weights[0] == 1200);
    DOCTEST_CHECK(weights[1] == -25);
}
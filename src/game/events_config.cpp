#include "pch.h"

namespace EVENTS {
    uint8_t force_event_type = 0;
    uint8_t all_events_delay = 50;
    uint8_t between_events_delay = 5;
    s_event_roll event_roll = {512, 1000};
    s_donation_bc donation_bc = {20, 100, 100};
    s_pirate_raid pirate_raid = {21, 29};
    s_supernova supernova = {0};
    s_computer_virus computer_virus = {50, 50};
    s_event_duration duration = {4, 4, 4, 20, 20, 20};

    uint16_t event_delay[EVENT_RANDOM_COUNT] = {
        0,
        0,
        200,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        100,
        200,
        300,
        150,
        250,
        200,
        0,
        0,
        0,
        0,
    };
} // namespace EVENTS
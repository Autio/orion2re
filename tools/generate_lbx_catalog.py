#!/usr/bin/env python3

import argparse
import json
import re
import struct
import zipfile
from pathlib import Path


LBX_HEADER_SIZE = 2048
LBX_MAGIC = 0xFEAD
LANGUAGES = ("english", "german", "french", "spanish", "italian", "polish")
RACES = (
    "human", "alkari", "bulrathi", "darlok", "elerian", "gnolam", "klackon",
    "meklar", "mrrshan", "psilon", "sakkra", "silicoid", "trilarian",
)
PLAYER_COLORS = ("red", "yellow", "green", "white", "blue", "brown", "purple", "orange")


ARCHIVE_ROLES = {
    "ANTARMSG.LBX": "Antaran room messages",
    "ANTAROOM.LBX": "Antaran room graphics",
    "APP_PICS.LBX": "technology application pictures",
    "BEAMS.LBX": "combat beam effects",
    "BILLTEX2.LBX": "interface messages",
    "BILLTEXT.LBX": "interface messages",
    "BUFFER0.LBX": "galaxy and system interface graphics",
    "CMBTFGTR.LBX": "combat fighter graphics",
    "CMBTMISL.LBX": "combat missile graphics",
    "CMBTPLNT.LBX": "combat planet pictures",
    "CMBTSFX.LBX": "combat special effects",
    "CMBTSHP.LBX": "combat ship pictures",
    "COLBLDG.LBX": "colony building interface graphics",
    "COLGCBT.LBX": "colony ground combat graphics",
    "COLONY.LBX": "colony screen graphics",
    "COLONY2.LBX": "colony production graphics",
    "COLPUPS.LBX": "colony popup graphics",
    "COLPUSP.LBX": "colony popup graphics variant",
    "COLREFIT.LBX": "colony refit popup graphics",
    "COLROADS.LBX": "colony road tiles",
    "COLSUM.LBX": "colony summary graphics",
    "COLSYSDI.LBX": "colony system display graphics",
    "COLVEGGI.LBX": "colony vegetation tiles",
    "COMBAT.LBX": "combat interface graphics",
    "CONFIRM.LBX": "confirmation popup graphics",
    "COUNCIL.LBX": "Galactic Council graphics",
    "COUNCMSG.LBX": "Galactic Council messages",
    "CREDITS.LBX": "game credits",
    "CUSTMSTR.LBX": "custom race strings and costs",
    "DESIGN.LBX": "ship design interface graphics",
    "DIPLOMAT.LBX": "diplomacy character graphics",
    "DIPSTARS.LBX": "diplomacy starfield graphics",
    "ENGMSG.LBX": "English keyed messages",
    "ESTRINGS.LBX": "English runtime strings",
    "EVENTS.LBX": "random event graphics",
    "FRETECD.LBX": "French technology descriptions",
    "FRE_HELP.LBX": "French help database",
    "FILEDATA.LBX": "installation file manifest",
    "FIREPTS.LBX": "ship weapon firing point tables",
    "FLEET.LBX": "fleet interface graphics",
    "FLTICONS.LBX": "fleet race icons",
    "FONTS.LBX": "fonts and font palettes",
    "GAME.LBX": "load, save, and game popup graphics",
    "GERTECD.LBX": "German technology descriptions",
    "GER_HELP.LBX": "German help database",
    "GSTAR.LBX": "galaxy star icons",
    "HELP.LBX": "English help database",
    "HERODATA.LBX": "leader records",
    "HESTRNGS.LBX": "English leader and interface strings",
    "IFONTS.LBX": "alternate fonts and font palettes",
    "INBOX.LBX": "text input popup graphics",
    "INFO.LBX": "information screen graphics",
    "INSTALL.LBX": "installer interface graphics",
    "ITATECD.LBX": "Italian technology descriptions",
    "ITA_HELP.LBX": "Italian help database",
    "JIMTEXT.LBX": "localized interface messages",
    "JIMTEXT2.LBX": "localized diplomacy messages",
    "KENTEXT.LBX": "localized game messages",
    "KENTEXT1.LBX": "localized game messages continuation",
    "LOADSAVE.LBX": "load and save screen graphics",
    "LOGO.LBX": "developer logo graphics",
    "MAINMENU.LBX": "main menu graphics",
    "MAINPUPS.LBX": "galaxy popup graphics",
    "MAINTEXT.LBX": "main game string tables",
    "MONSTER.LBX": "space monster combat pictures",
    "MSGENG.LBX": "English message database",
    "MULTIGM.LBX": "multiplayer interface graphics",
    "NDATA.LBX": "new game data",
    "NEWGAME.LBX": "new game interface graphics",
    "NEXTPLYR.LBX": "next player popup graphics",
    "OCPOL.LBX": "occupation policy popup graphics",
    "OFFICER.LBX": "leader interface graphics and portraits",
    "PATCH13.LBX": "race screen patch graphics",
    "PLANETS.LBX": "planet pictures",
    "PLAYSPEC.LBX": "player configuration records",
    "PLNTSUM.LBX": "planet summary graphics",
    "RACEICON.LBX": "race and empire icons",
    "RACENAME.LBX": "ruler name records",
    "RACEOPT.LBX": "custom race interface graphics",
    "RACERPRT.LBX": "race report graphics",
    "RACES.LBX": "race screen graphics",
    "RACESEL.LBX": "race selection graphics",
    "RACESTUF.LBX": "race option strings and records",
    "REFITPUP.LBX": "ship refit popup graphics",
    "RSTRING0.LBX": "English race and colony strings",
    "SCIENCE.LBX": "science screen graphics",
    "SCORE.LBX": "score and hall of fame graphics",
    "SHIPNAME.LBX": "ship name records",
    "SHIPS.LBX": "strategic ship pictures and palettes",
    "SKILDESC.LBX": "leader skill names and descriptions",
    "SOUND.LBX": "sound effects",
    "SPHERSFX.LBX": "spherical combat effects",
    "SPATECD.LBX": "Spanish technology descriptions",
    "SPA_HELP.LBX": "Spanish help database",
    "STARBG.LBX": "galaxy starfield and nebula graphics",
    "STARNAME.LBX": "star name records",
    "STREAM.LBX": "streamed music and speech",
    "STREAMHD.LBX": "additional streamed music and speech",
    "SYSDISP.LBX": "system display graphics",
    "TECHDESC.LBX": "technology descriptions",
    "TECHNAME.LBX": "localized technology names",
    "TECHSEL.LBX": "technology selection graphics",
    "TEXTBOX.LBX": "text box graphics",
    "TURNSUM.LBX": "turn summary graphics",
    "WARNING.LBX": "warning popup graphics",
}

VIDEO_NAMES = {
    "AMEBAFIN.LBX": ("amoeba_victory_ending.smk", "Victory ending after defeating the space amoeba."),
    "ANATKFIN.LBX": ("antaran_attack_ending.smk", "Ending cinematic for an Antaran attack."),
    "ANWINFIN.LBX": ("antaran_victory_ending.smk", "Victory ending after conquering Antares."),
    "DIMTVFIN.LBX": ("dimensional_portal_victory_ending.smk", "Dimensional portal victory ending."),
    "GENWINFN.LBX": ("galactic_victory_ending.smk", "General galactic victory ending."),
    "INTRO.LBX": ("intro.smk", "Game introduction cinematic."),
    "LOSERFIN.LBX": ("defeat_ending.smk", "Game defeat ending."),
    "ORIONFIN.LBX": ("orion_victory_ending.smk", "Orion victory ending."),
    "PLNTDFIN.LBX": ("planet_destruction_ending.smk", "Planet destruction ending cinematic."),
    "WININFIN.LBX": ("final_victory_ending.smk", "Final victory ending cinematic."),
}

SOUND_NAMES = {
    1: "laser_beam", 2: "disruptor_beam", 3: "gyro_spin", 4: "lightning_field",
    5: "monster_breath", 8: "explosion_tail", 9: "fusion_beam", 10: "phase_effect",
    11: "graviton_beam", 12: "mass_driver", 13: "plasma_beam", 14: "ion_pulse",
    15: "gauss_beam", 18: "special_beam_46", 19: "phasor_beam",
    20: "stellar_converter", 21: "death_ray", 22: "neutron_beam",
    23: "particle_beam", 24: "explosion_impact", 25: "quantum_detonation",
    27: "plasma_web_travel", 28: "missile_intercept", 29: "beam_hit",
    30: "large_explosion", 31: "troop_move", 32: "missile_launch",
    33: "alert_chime", 38: "shield_hit", 40: "ui_click", 41: "ui_confirm",
    42: "reflected_shield", 43: "supernova_explosion", 44: "plasma_web_impact",
    47: "warp_core_explosion", 51: "combat_alert", 52: "retreat_warp",
    53: "menu_intro", 54: "menu_intro_end", 55: "teleport", 57: "pod_landing",
    58: "pod_descent", 59: "plasma_torpedo", 60: "proton_torpedo",
    61: "antimatter_torpedo", 62: "hydra_death", 63: "amoeba_death",
    64: "eel_death", 65: "dragon_death", 66: "crystal_death",
    67: "boarding_result", 68: "plasma_web_hit",
}

FIXED_NAMES = {
    "ANTAROOM.LBX": {0: "antaran_room_background", 1: "antaran_room"},
    "COLSUM.LBX": {0: "colony_summary_background", 21: "colony_summary_palette"},
    "CONFIRM.LBX": {0: "confirm_background", 1: "confirm_yes_button", 2: "confirm_no_button"},
    "DIPSTARS.LBX": {0: "diplomacy_starfield_background"},
    "EVENTS.LBX": {0: "event_background", 1: "event_frame"},
    "INBOX.LBX": {0: "input_box_background", 1: "input_box_cursor"},
    "LOGO.LBX": {0: "developer_logo_background", 1: "developer_logo"},
    "RACEOPT.LBX": {
        0: "custom_race_background", 1: "special_ability_button", 2: "default_race_button",
        3: "accept_race_button", 4: "race_option_popup", 5: "disabled_option_button",
        6: "race_name_popup",
    },
    "SCIENCE.LBX": {0: "science_room_background", 1: "scientist", 2: "science_room_overlay"},
    "TEXTBOX.LBX": {0: "text_box_top", 1: "text_box_middle", 2: "text_box_bottom", 3: "text_box_close_button", 4: "text_box_palette"},
    "TURNSUM.LBX": {0: "turn_summary_background", 1: "turn_summary_scroll_up", 2: "turn_summary_scroll_down", 3: "turn_summary_ok_button"},
    "WARNING.LBX": {0: "warning_popup"},
}

BUFFER_NAMES = {
    0: "main_screen", 13: "main_screen_status_lights", 15: "diplomacy_request_popup",
    16: "diplomacy_accept_button", 17: "diplomacy_invoke_button", 18: "diplomacy_wait_button",
    21: "dimensional_portal_small", 22: "dimensional_portal_large",
    33: "artemis_net_small", 34: "artemis_net_large", 51: "large_title",
    52: "small_title", 53: "large_grid", 54: "small_grid", 55: "large_bottom",
    56: "small_bottom", 57: "scrollbar", 58: "fleet_scroll_up", 59: "fleet_scroll_down",
    60: "fleet_select_all", 73: "system_popup_background", 75: "planet_scan_highlight",
    82: "movable_box", 91: "asteroid", 281: "neutral_system_ship_icon",
}


def parse_arguments():
    parser = argparse.ArgumentParser(description="Generate a complete semantic catalog of MOO2 LBX resources.")
    parser.add_argument("source", type=Path, help="Directory containing the original LBX files")
    parser.add_argument("output", type=Path, help="Catalog JSON to write")
    return parser.parse_args()


def slug(value):
    value = re.sub(r"[^a-z0-9]+", "_", value.lower()).strip("_")
    return value or "resource"


def classify_entry(data):
    if not data:
        return "empty"
    if len(data) >= 44 and data[:4] == b"RIFF" and data[8:12] == b"WAVE":
        return "sound"
    if len(data) >= 16:
        width, height, _, frame_count = struct.unpack_from("<hhhh", data)
        table_end = 12 + (frame_count + 1) * 4
        if 0 < width <= 640 and 0 < height <= 480 and 0 <= frame_count <= 2000 and table_end <= len(data):
            offsets = struct.unpack_from(f"<{frame_count + 1}I", data, 12)
            if offsets[0] >= table_end and all(a <= b <= len(data) for a, b in zip(offsets, offsets[1:])):
                return "image" if frame_count <= 1 else "animation"
    return "data"


def extension(resource_type):
    return {
        "animation": ".ani", "data": ".dat", "empty": ".empty", "font": ".fnt",
        "image": ".img", "palette": ".pal", "sound": ".wav", "text": ".txt",
        "video": ".smk",
    }[resource_type]


def text_archive_type(archive, index):
    if archive in {
        "ANTARMSG.LBX", "BILLTEX2.LBX", "BILLTEXT.LBX", "COUNCMSG.LBX", "CREDITS.LBX",
        "DIPLOMSE.LBX", "DIPLOMSF.LBX", "DIPLOMSG.LBX", "DIPLOMSS.LBX", "ENGMSG.LBX",
        "ESTRINGS.LBX", "EVENTMSE.LBX", "EVENTMSF.LBX", "EVENTMSG.LBX", "EVENTMSS.LBX",
        "EVENTMSI.LBX", "FRETECD.LBX", "GERTECD.LBX", "ITATECD.LBX", "SPATECD.LBX",
        "HESTRNGS.LBX", "JIMTEXT.LBX", "JIMTEXT2.LBX", "KENTEXT.LBX", "KENTEXT1.LBX",
        "MAINTEXT.LBX", "MSGENG.LBX", "RACENAME.LBX", "RSTRING0.LBX", "SHIPNAME.LBX",
        "SKILDESC.LBX", "STARNAME.LBX", "TECHDESC.LBX", "TECHNAME.LBX",
    }:
        return "text"
    if archive == "RACESTUF.LBX" and index not in {6, 7}:
        return "text"
    if archive == "CUSTMSTR.LBX" and index < 3:
        return "text"
    return None


def family_semantics(archive, index, resource_type):
    stem = archive[:-4].lower()

    match = re.fullmatch(r"TANM_(\d{3})\.LBX", archive)
    if match:
        app_id = int(match.group(1))
        return f"technology_discovery_{app_id:03d}", f"Technology application {app_id} discovery animation."

    match = re.fullmatch(r"SR_R([0-9A-C])_(SC|SP|TR)\.LBX", archive)
    if match:
        race = RACES[int(match.group(1), 16)]
        role = {"SC": "scientist", "SP": "spy", "TR": "trooper"}[match.group(2)]
        return f"{race}_{role}", f"{race.title()} {role} animation."

    match = re.fullmatch(r"BLDG([0-5])\.LBX", archive)
    if match:
        style = int(match.group(1))
        return f"colony_style_{style}_building_{index:03d}", f"Colony style {style} building resource {index}."

    if archive == "APP_PICS.LBX":
        return f"technology_application_{index:03d}", f"Technology application {index} picture."
    if archive.startswith("DIPLOMS"):
        return f"diplomacy_statement_{index:03d}", f"Diplomatic statement record {index}."
    if archive.startswith("EVENTMS"):
        return f"event_message_{index:03d}", f"Random event message record {index}."
    if archive in {"JIMTEXT.LBX", "JIMTEXT2.LBX", "KENTEXT.LBX", "KENTEXT1.LBX", "BILLTEXT.LBX", "BILLTEX2.LBX"}:
        base = index // 6
        if archive == "KENTEXT1.LBX":
            base += 80
        return f"message_{base:03d}_{LANGUAGES[index % 6]}", f"Message {base}, {LANGUAGES[index % 6]} localization."
    if archive == "CMBTSHP.LBX":
        return f"player_color_{index // 45}_combat_ship_{index % 45:02d}", f"Combat ship picture {index % 45} for player color {index // 45}."
    if archive == "CMBTFGTR.LBX":
        return f"fighter_{index:02d}", f"Combat fighter animation {index}."
    if archive == "CMBTMISL.LBX":
        return f"combat_projectile_{index:03d}", f"Combat missile or projectile resource {index}."
    if archive == "CMBTPLNT.LBX":
        return f"combat_planet_{index:02d}", f"Combat planet picture {index}."
    if archive == "COLROADS.LBX":
        return f"colony_road_tile_{index:03d}", f"Colony road tile {index}."
    if archive == "COLVEGGI.LBX":
        return f"colony_vegetation_tile_{index:03d}", f"Colony vegetation tile {index}."
    if archive == "PLANETS.LBX":
        return f"planet_{index:02d}", f"Strategic planet picture {index}."
    if archive == "SHIPS.LBX":
        if index < 400:
            color, picture = divmod(index, 50)
            return f"{PLAYER_COLORS[color]}_strategic_ship_{picture:02d}", f"Strategic ship picture {picture} for the {PLAYER_COLORS[color]} player."
        if index == 413:
            return "antaran_ship_palette", "Palette used by Antaran ships."
        if 414 <= index <= 419:
            return f"monster_ship_palette_{index - 414}", f"Space monster ship palette {index - 414}."
    if archive == "SOUND.LBX":
        if index == 0:
            return "reserved_silence", "Reserved sound resource; the sound player treats ID 0 as silence."
        name = SOUND_NAMES.get(index, f"sound_effect_{index:02d}")
        return name, f"Sound effect {index}{' (' + name.replace('_', ' ') + ')' if index in SOUND_NAMES else ''}."
    if archive == "STREAM.LBX":
        return f"stream_track_{index:02d}", f"Streamed music, speech, or transition track {index}."
    if archive == "STREAMHD.LBX":
        return f"stream_track_{index + 100:03d}", f"Streamed music, speech, or transition track {index + 100}."
    if archive in {"FONTS.LBX", "IFONTS.LBX"}:
        if index == 0:
            return f"{stem}_glyphs", "Font glyph data."
        return f"{stem}_palette_{index:02d}", f"Font palette {index}."
    if archive == "OFFICER.LBX":
        if 21 <= index <= 87:
            return f"leader_portrait_{index - 21:02d}", f"Leader portrait {index - 21}."
        if 88 <= index <= 114:
            return f"leader_skill_icon_{index - 88:02d}", f"Leader skill icon {index - 88}."
        if 115 <= index <= 122:
            return f"small_ship_icon_{index - 115}", f"Small player ship icon {index - 115}."
        if 123 <= index <= 129:
            return f"monster_ship_icon_{index - 123}", f"Small monster ship icon {index - 123}."
        if 130 <= index <= 140:
            return f"fleet_star_icon_{index - 130}", f"Fleet galaxy star icon {index - 130}."
        if 210 <= index <= 276:
            return f"leader_portrait_bright_{index - 210:02d}", f"Bright leader portrait {index - 210}."
        if 277 <= index <= 343:
            return f"leader_portrait_dim_{index - 277:02d}", f"Dimmed leader portrait {index - 277}."
    if archive == "BUFFER0.LBX":
        if index in BUFFER_NAMES:
            return BUFFER_NAMES[index], f"{BUFFER_NAMES[index].replace('_', ' ').capitalize()}."
        if 35 <= index <= 42:
            return f"{PLAYER_COLORS[index - 35]}_colony_marker", f"{PLAYER_COLORS[index - 35].title()} player colony marker."
        if 43 <= index <= 50:
            return f"{PLAYER_COLORS[index - 43]}_outpost_marker", f"{PLAYER_COLORS[index - 43].title()} player outpost marker."
        if 77 <= index <= 81:
            return f"planet_orbit_{index - 77}", f"Planet orbital ring {index - 77}."
        if 83 <= index <= 89:
            return f"star_class_{index - 83}", f"Star spectral class {index - 83}."
        if 92 <= index <= 141:
            return f"rotating_planet_{index - 92:02d}", f"Rotating planet picture {index - 92}."
        if 143 <= index <= 147:
            return f"planet_ring_mask_{index - 143}", f"Ringed planet mask for size {index - 143}."
        if 205 <= index <= 236:
            return f"player_ship_icon_{index - 205:02d}", f"Zoomed player ship icon {index - 205}."
        if 237 <= index <= 240:
            return f"monster_ship_icon_{index - 237}", f"Zoomed monster ship icon {index - 237}."
        if 241 <= index <= 264:
            return f"antaran_ship_icon_{index - 241:02d}", f"Zoomed Antaran ship icon {index - 241}."
        if 265 <= index <= 272:
            return f"{PLAYER_COLORS[index - 265]}_player_light", f"{PLAYER_COLORS[index - 265].title()} player status light."
        if 273 <= index <= 280:
            return f"{PLAYER_COLORS[index - 273]}_system_ship_icon", f"{PLAYER_COLORS[index - 273].title()} system ship icon."
        if 282 <= index <= 287:
            return f"special_system_ship_icon_{index - 282}", f"Special system ship owner icon {index - 282}."
    if archive == "STARBG.LBX":
        if index < 3:
            distance = ("far", "middle", "near")[index]
            return f"{distance}_starfield", f"{distance.title()} parallax starfield."
        if 6 <= index <= 53:
            nebula, frame = divmod(index - 6, 4)
            return f"nebula_{nebula:02d}_frame_{frame}", f"Nebula {nebula}, animation frame {frame}."
    if archive == "PLNTSUM.LBX":
        if 15 <= index <= 25:
            return f"galaxy_star_icon_{index - 15}", f"Galaxy star icon {index - 15}."
        if 26 <= index <= 75:
            return f"planet_summary_planet_{index - 26:02d}", f"Planet summary planet picture {index - 26}."
        if 76 <= index <= 83:
            return f"{PLAYER_COLORS[index - 76]}_ship_icon", f"{PLAYER_COLORS[index - 76].title()} player ship icon."
    if archive == "RACEICON.LBX":
        if index < 169:
            race, variant = divmod(index, 13)
            if race < len(RACES):
                return f"{RACES[race]}_race_icon_{variant:02d}", f"{RACES[race].title()} race icon variant {variant}."
        return f"race_icon_special_{index - 169}", f"Special race icon {index - 169}."
    if archive == "COUNCIL.LBX":
        if 2 <= index <= 79:
            race, frame = divmod(index - 2, 6)
            return f"{RACES[race]}_council_member_{frame}", f"{RACES[race].title()} council member frame {frame}."
        if 80 <= index <= 157:
            race, frame = divmod(index - 80, 6)
            return f"{RACES[race]}_council_member_alt_{frame}", f"Alternate {RACES[race].title()} council member frame {frame}."
        if 158 <= index <= 183:
            race, frame = divmod(index - 158, 2)
            return f"{RACES[race]}_council_nominee_{frame}", f"{RACES[race].title()} council nominee frame {frame}."
        if 184 <= index <= 196:
            return f"{RACES[index - 184]}_council_animation", f"{RACES[index - 184].title()} council animation."
    if archive == "GSTAR.LBX" and 1 <= index <= 11:
        return f"galaxy_star_{index:02d}", f"Galaxy star icon {index}."
    if archive == "MONSTER.LBX":
        return f"space_monster_{index:02d}", f"Space monster combat picture {index}."
    if archive == "FLTICONS.LBX" and index < len(RACES):
        return f"{RACES[index]}_fleet_icon", f"{RACES[index].title()} fleet icon."
    if archive == "SHIPNAME.LBX":
        return "ship_names", "Localized fixed-size ship name records."
    if archive == "STARNAME.LBX":
        name = "home_star_names" if index == 0 else "generated_star_names"
        return name, "Home-system star names." if index == 0 else "Procedurally selected star names."
    if archive == "SKILDESC.LBX":
        return ("leader_skill_names", "Leader skill names.") if index == 0 else ("leader_skill_descriptions", "Leader skill descriptions.")
    if archive in {"TECHDESC.LBX", "FRETECD.LBX", "GERTECD.LBX", "ITATECD.LBX", "SPATECD.LBX"}:
        names = ("special_system_names", "special_system_descriptions", "weapon_names", "weapon_descriptions")
        return names[index], names[index].replace("_", " ").capitalize() + "."
    if archive == "TECHNAME.LBX":
        return f"technology_names_{LANGUAGES[index]}", f"{LANGUAGES[index].title()} technology and component names."
    if archive == "RACESTUF.LBX":
        if index == 6:
            return "custom_race_trait_costs", "Point costs for custom race trait levels."
        if index == 7:
            return "custom_race_template", "Default custom race configuration record."
        language = LANGUAGES[index if index < 6 else index - 8]
        section = "race_option_names" if index < 7 else "race_stat_labels"
        return f"{section}_{language}", f"{language.title()} {section.replace('_', ' ')}."
    if archive == "FIREPTS.LBX":
        return "ship_weapon_firing_points", "Ship weapon firing point records."
    if archive == "HERODATA.LBX":
        return "leader_records", "Leader names, titles, skills, and attributes."
    if archive == "RACENAME.LBX":
        return "ruler_names", "Localized ruler name records by race."

    role = ARCHIVE_ROLES.get(archive, stem.replace("_", " "))
    return f"{slug(role)}_{index:03d}", f"{role.capitalize()} resource {index}."


def catalog_entry(archive, index, data):
    resource_type = classify_entry(data)
    forced_type = text_archive_type(archive, index)
    if forced_type is not None:
        resource_type = forced_type
    if archive in {"FONTS.LBX", "IFONTS.LBX"}:
        resource_type = "font" if index == 0 else "palette"
    if archive == "COLSUM.LBX" and index == 21:
        resource_type = "palette"

    fixed_name = FIXED_NAMES.get(archive, {}).get(index)
    if fixed_name is not None:
        semantic_name = fixed_name
        description = fixed_name.replace("_", " ").capitalize() + "."
    else:
        semantic_name, description = family_semantics(archive, index, resource_type)

    return {
        "name": semantic_name + extension(resource_type),
        "type": resource_type,
        "description": description,
    }


def parse_archive(archive, data, source_label):
    if data[:4] in {b"SMK2", b"SMK4"}:
        semantic_name, description = VIDEO_NAMES.get(
            archive, (slug(Path(archive).stem) + ".smk", f"Smacker video stored as {archive}."))
        return {
            "format": "smacker",
            "entry_count": 1,
            "sources": [source_label],
            "entries": {
                "0": {
                    "name": semantic_name,
                    "type": "video",
                    "description": description,
                }
            },
        }
    if len(data) < LBX_HEADER_SIZE:
        raise ValueError(f"{source_label}: truncated LBX header")
    entry_count, magic = struct.unpack_from("<HH", data)
    if magic != LBX_MAGIC or entry_count >= 510:
        raise ValueError(f"{source_label}: invalid LBX header")
    offsets = struct.unpack_from(f"<{entry_count + 1}I", data, 8)
    if offsets[0] < LBX_HEADER_SIZE or offsets[-1] != len(data):
        raise ValueError(f"{source_label}: invalid LBX offsets")

    entries = {}
    for index in range(entry_count):
        start, end = offsets[index:index + 2]
        if end < start:
            raise ValueError(f"{source_label}: invalid entry {index}")
        entries[str(index)] = catalog_entry(archive, index, data[start:end])
    return {"format": "lbx", "entry_count": entry_count, "sources": [source_label], "entries": entries}


def generate(source):
    candidates = []
    for path in sorted(source.rglob("*"), key=lambda item: str(item).lower()):
        if not path.is_file():
            continue
        relative_path = path.relative_to(source).as_posix()
        if path.suffix.lower() == ".lbx":
            candidates.append((path.name.upper(), path.read_bytes(), relative_path))
        elif path.suffix.lower() == ".zip":
            with zipfile.ZipFile(path) as archive:
                for member in sorted(archive.namelist(), key=str.lower):
                    if member.lower().endswith(".lbx"):
                        candidates.append((Path(member).name.upper(), archive.read(member), f"{relative_path}::{member}"))
    if not candidates:
        raise ValueError(f"{source}: no LBX files found")
    catalog = {}
    for archive, data, source_label in candidates:
        variant = parse_archive(archive, data, source_label)
        if archive not in catalog:
            catalog[archive] = variant
            continue
        existing = catalog[archive]
        if existing["format"] != variant["format"] or existing["entry_count"] != variant["entry_count"]:
            raise ValueError(f"{source_label}: incompatible variant of {archive}")
        existing["sources"].append(source_label)
    return dict(sorted(catalog.items()))


def main():
    arguments = parse_arguments()
    catalog = generate(arguments.source)
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    entry_count = sum(archive["entry_count"] for archive in catalog.values())
    print(f"{arguments.output}: wrote {len(catalog)} archives and {entry_count} resources")


if __name__ == "__main__":
    main()
#include "enchanter.h"

#include "activity_actor_definitions.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "character.h"
#include "cursesdef.h"
#include "flag.h"
#include "game.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "generic_readers.h"
#include "iexamine.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "mapdata.h"
#include "messages.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "relic.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_utils.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"

#include <algorithm>
#include <string>
#include <vector>

namespace enchanter {
requirement_data total_requirements(enchant_info& info) {
    return std::accumulate(
        info.requirements.begin(), info.requirements.end(), requirement_data(),
        [](const requirement_data& lhs, const std::pair<requirement_id, int>& rhs) {
            return lhs + (*rhs.first * rhs.second);
        });
}

std::vector<std::string> enchantment_info(
    const enchant_info& info, Character& crafter, int fold_width, item& itm) {
    units::volume vol = itm.base_volume();
    std::ostringstream oss = std::ostringstream();

    oss << string_format(_("Result Info:\n "));

    for (const std::string& str : info.to_enchant_with->get_effect_string(true)) {
        oss << "  " << str << "\n";
    }

    if (info.max_count > 0) {
        int count = itm.get_var<int>(info.count_var, 0);
        oss << string_format(_("Applications Left: %d"), info.max_count - count);
    }

    if (info.required_skills.size() > 0) {
        oss << "\n";
        oss << string_format(_("Required Skills:\n"));
        for (const auto& [skill_id, level] : info.required_skills) {
            oss << string_format("  <color_cyan>%s</color>: %d\n", skill_id->name(), level);
        }
    }

    oss << "\n";

    oss << string_format(
        _("Time to complete: <color_cyan>%s</color>\n"), to_string(info.time_to_enchant));

    if (info.requirements.size() > 0) {
        auto crafting_inv = crafter.crafting_inventory(true);
        std::vector<std::string> tools;
        std::vector<std::string> comps;
        for (const auto& [id, count] : info.requirements) {
            auto real_req = (*id) * (info.volume_batch_effect ? vol / info.volume_per_batch : 1);
            tools.append_range(
                real_req.get_folded_tools_list(fold_width, c_white, crafting_inv, count));
            comps.append_range(real_req.get_folded_components_list(
                fold_width, c_white, crafting_inv, return_true<item>, count, ""));
        }
        oss << "\n";
        for (const std::string& str : tools) { oss << str << "\n"; }
        for (const std::string& str : comps) { oss << str << "\n"; }
    }

    std::vector<std::string> result = foldstring(oss.str(), fold_width);
    return result;
}

int enchantment_selector_menu(std::vector<enchant_info> options, Character& user, item& itm) {
    units::volume vol = itm.base_volume();
    auto crafting_inv = user.crafting_inventory(true);
    int width = 0;
    int height = 0;
    int item_info_width = 0;
    catacurses::window w_ench;
    catacurses::window w_info;
    ui_adaptor ui;

    input_context ctxt("CRAFTING");
    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("UP");
    ctxt.register_action("DOWN");
    ctxt.register_action("PAGE_UP", to_translation("Fast scroll up"));
    ctxt.register_action("PAGE_DOWN", to_translation("Fast scroll down"));
    ctxt.register_action("HELP_KEYBINDINGS");

    ui.on_screen_resize([&](ui_adaptor& ui) {
        width = TERMX / 2 - 2;
        height = TERMY - 2;

        w_ench = catacurses::newwin(TERMY, TERMX / 2, point(0, 0));
        w_info = catacurses::newwin(TERMY, TERMX / 2, point(TERMX / 2, 0));

        ui.position(point(0, 0), point(TERMX, TERMY));
    });
    ui.mark_resize();

    std::vector<std::string> names;
    for (const auto& ench_info : options) { names.push_back(ench_info.name); }
    int line = 0;
    int names_scroll_min = 0;
    int names_scroll_max = 0;
    int info_scroll = 0;
    int num_options = names.size();
    ui.on_redraw([&](ui_adaptor& ui) {
        werase(w_ench);
        calcStartPos(names_scroll_min, line, height, num_options);
        names_scroll_max = std::min(num_options, names_scroll_min + height);
        for (int i = names_scroll_min; i < names_scroll_max; ++i) {
            const bool highlight = i == line;
            const point print_from(2, i - names_scroll_min + 1);
            nc_color col = highlight ? c_white : c_dark_gray;
            if (highlight) { ui.set_cursor(w_ench, print_from); }
            trim_and_print(w_ench, print_from, width, col, names[i]);
        }
        draw_scrollbar(w_ench, line, TERMY, num_options, point(width, 0));
        draw_border(w_ench, c_white, "Enchantment Options", c_white);
        wnoutrefresh(w_ench);
        werase(w_info);
        draw_scrollbar(w_info, line, height, num_options, point(width, 0));
        draw_border(w_info, c_white, "Enchantment Info", c_white);
        if (num_options != 0) {
            const std::vector<std::string>& info =
                enchantment_info(options[line], user, width, itm);
            const int total_lines = info.size();
            for (int i = 0; i < total_lines; ++i) {
                // NOTE: needed because it expects an lvalue
                auto dummy = c_white;
                trim_and_print(w_info, point(2, i + 1), width, c_white, info[i]);
            }

            if (total_lines > height) {
                scrollbar()
                    .offset_x(width)
                    .content_size(total_lines)
                    .viewport_pos(info_scroll)
                    .viewport_size(TERMY)
                    .apply(w_info);
            }
        }
        wnoutrefresh(w_info);
    });
    while (true) {
        ui_manager::redraw();
        const int scroll_info_lines = catacurses::getmaxy(w_info) - 4;
        const std::string action = ctxt.handle_input();
        if (action == "PAGE_UP") {
            info_scroll -= scroll_info_lines;
        } else if (action == "PAGE_DOWN") {
            info_scroll += scroll_info_lines;
        } else if (action == "DOWN") {
            line++;
        } else if (action == "UP") {
            line--;
        } else if (action == "CONFIRM") {
            auto info = options[line];
            auto total_reqs =
                total_requirements(info)
                * std::max(1L, (info.volume_batch_effect ? vol / info.volume_per_batch : 1));
            if (!total_reqs.can_make_with_inventory(crafting_inv, return_true<item>)) {
                popup("You have insufficient items to make this enchantment.");
            } else if (itm.get_var<int>(info.count_var, 0) >= info.max_count) {
                popup("You have already applied this enchantment too many times.");
            } else {
                return line;
            }
        } else if (action == "QUIT") {
            return -1;
        }
        if (line < 0) {
            line = num_options - 1;
        } else if (line >= static_cast<int>(num_options)) {
            line = 0;
        }
    }
    // This should never be reached really
    return line;
}

} // namespace enchanter

void enchant_info::deserialize(JsonIn& jin) {
    JsonObject obj = jin.get_object();
    mandatory(obj, false, "id", id);
    mandatory(obj, false, "name", name);
    mandatory(obj, false, "enchant", to_enchant_with);
    mandatory(obj, false, "time_to_enchant", time_to_enchant, time_reader());
    optional(obj, false, "volume_per_batch", volume_per_batch, volume_reader());
    optional(obj, false, "volume_batch_effect", volume_batch_effect, false);
    optional(obj, false, "applied_flag", applied_flag_id, flag_NULL);
    optional(obj, false, "count_var", count_var, "ENCH_COUNT");
    optional(obj, false, "max_count", max_count, -1);
    optional(obj, false, "can_make", can_make, "");
    optional(obj, false, "can_use_on", can_use_on, "");
    // Requirements
    if (obj.has_string("using")) {
        requirements = {{requirement_id(obj.get_string("using")), 1}};
    } else if (obj.has_array("using")) {
        requirements.clear();
        for (JsonArray cur : obj.get_array("using")) {
            requirements.emplace_back(requirement_id(cur.get_string(0)), cur.get_int(1));
        }
    } else {
        requirements.clear();
        // Construct a requirement to capture "components", "qualities", and
        // "tools" that might be listed.
        requirement_id req_id;
        int i = 0;
        do {
            req_id = requirement_id(string_format("inline_enchanter_requirements_%s_%d", name, i));
        } while (req_id.is_valid());
        requirement_data::load_requirement(obj, req_id);
        requirements.emplace_back(req_id, 1);
    }
    // Skills
    if (obj.has_array("skill_levels")) {
        for (JsonValue val : obj.get_array("skill_levels")) {
            JsonObject iobj = val.get_object();
            required_skills[skill_id(iobj.get_string("skill"))] = iobj.get_int("level");
        }
    }
}

void iexamine::enchanter(player& p, const tripoint_bub_ms& pos) {
    map& here = get_map();
    const furn_id& furn_id = here.furn(pos);
    if (furn_id->enchanter.size() == 0) { debugmsg("Enchanter iuse has no enchanter info"); }
    item& to_ench = p.primary_weapon();
    int index = enchanter::enchantment_selector_menu(furn_id->enchanter, p, to_ench);
    if (index != -1) {
        auto info = furn_id->enchanter[index];
        p.assign_activity(
            std::make_unique<player_activity>(std::make_unique<enchant_activity_actor>(
                to_ench, furn_id.id(), info.id, to_moves<int>(info.time_to_enchant))));
    }
}

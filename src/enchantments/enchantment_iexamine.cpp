#include "enchantment_iexamine.h"

#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "character.h"
#include "character_functions.h"
#include "color.h"
#include "crafting.h"
#include "crafting_quality.h"
#include "cursesdef.h"
#include "game.h"
#include "game_inventory.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "mod_manager.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "string_utils.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace enchantment_iexamine {
std::vector<std::string> enchantment_info(
    const enchant_info& info, Character& crafter, int fold_width) {
    std::ostringstream oss = std::ostringstream();

    oss << string_format(_("Result Info:\n "));

    for (const std::string& str : info.to_enchant_with->get_effect_string(true)) {
        oss << "  " << str << "\n";
    }

    oss << string_format(_("Required Skills:\n"));
    for (const auto& [skill_id, level] : info.required_skills) {
        oss << string_format("  <color_cyan>%s</color>: %d\n", skill_id->name(), level);
    }

    oss << string_format(
        _("Time to complete: <color_cyan>%s</color>\n"), to_string(info.time_to_enchant));

    auto crafting_inv = crafter.crafting_inventory(true);
    const requirement_data& req = *info.requirements;
    const std::vector<std::string> tools =
        req.get_folded_tools_list(fold_width, c_white, crafting_inv, info.requirements_count);
    const std::vector<std::string> comps = req.get_folded_components_list(
        fold_width, c_white, crafting_inv, return_true<item>, info.requirements_count, "");
    for (const std::string& str : tools) { oss << str; }
    for (const std::string& str : comps) { oss << str; }

    std::vector<std::string> result = foldstring(oss.str(), fold_width);

    return result;
}

int enchantment_selector_menu(std::vector<enchant_info> options, Character& user) {
    int width = 0;
    int height = 0;
    int item_info_width = 0;
    catacurses::window w_ench;
    catacurses::window w_info;
    ui_adaptor ui;

    input_context ctxt("CRAFTING");
    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("PAGE_UP", to_translation("Fast scroll up"));
    ctxt.register_action("PAGE_DOWN", to_translation("Fast scroll down"));
    ctxt.register_action("HELP_KEYBINDINGS");

    ui.on_screen_resize([&](ui_adaptor& ui) {
        width = TERMX / 2 - 2;
        height = TERMY - 2;

        w_ench = catacurses::newwin(height, width, point(1, 1));
        w_info = catacurses::newwin(height, width, point(width + 3, 1));

        ui.position(point(0, 0), point(TERMX, TERMY));
    });

    std::vector<std::string> names;
    for (const auto& ench_info : options) { names.push_back(ench_info.name); }
    int line = 0;
    int names_scroll_min = 0;
    int names_scroll_max = 0;
    int info_scroll;
    int num_options = names.size();
    ui.on_redraw([&](ui_adaptor& ui) {
        werase(w_ench);
        werase(w_ench);
        calcStartPos(names_scroll_min, line, height, num_options);
        names_scroll_max = std::min(num_options, names_scroll_min + height);
        for (int i = names_scroll_min; i < names_scroll_max; ++i) {
            const bool highlight = i == line;
            const point print_from(2, i - names_scroll_min);
            nc_color col = highlight ? c_white : c_dark_gray;
            if (highlight) { ui.set_cursor(w_ench, print_from); }
            trim_and_print(w_ench, print_from, width, col, names[i]);
        }
        draw_scrollbar(w_ench, line, height, num_options, point_zero);
        wnoutrefresh(w_ench);
        if (num_options != 0) {
            const std::vector<std::string>& info = enchantment_info(options[line], user, width);
            const int total_lines = info.size();
            const int xpos = width + 3;
            if (info_scroll < 0) {
                info_scroll = 0;
            } else if (info_scroll + height > total_lines) {
                info_scroll = std::max(0, total_lines - height);
            }
            for (int i = info_scroll; i < std::min(info_scroll + height, total_lines); ++i) {
                // NOTE: needed because it expects an lvalue
                auto dummy = c_white;
                print_colored_text(w_ench, point(xpos, i - info_scroll), dummy, c_white, info[line]);
            }

            if (total_lines > height) {
                scrollbar()
                    .offset_x(xpos + width + 1)
                    .content_size(total_lines)
                    .viewport_pos(info_scroll)
                    .viewport_size(height)
                    .apply(w_info);
            }
        }
    });
    draw_scrollbar(w_info, line, height, num_options, point_zero);
    wnoutrefresh(w_info);
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
            return line;
        } else if (action == "QUIT") {
            break;
        }
        if (line < 0) {
            line = num_options - 1;
        } else if (line >= static_cast<int>(num_options)) {
            line = 0;
        }
    }
    return -1;
}
}
}

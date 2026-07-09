#include "enchantment_condition.h"

#include "assign.h"
#include "character.h"
#include "debug.h"
#include "generic_factory.h"
#include "item.h"
#include "type_id_implement.h"

#include <optional>
#include <vector>

namespace {
generic_factory<enchantment_condition> all_enchantment_conditions("Enchantment flags");
}

IMPLEMENT_STRING_AND_INT_IDS(enchantment_condition, all_enchantment_conditions);

void enchantment_condition::load_enchantment_conditions(
    const JsonObject& jo, const std::string& src) {
    all_enchantment_conditions.load(jo, src);
}

void enchantment_condition::load(const JsonObject& jo, const std::string& src) {
    optional(jo, was_loaded, "item_condition", is_item_condition, false);
    optional(jo, was_loaded, "condition_function", condition_function, "always");
}

void enchantment_condition::check() const {}

void enchantment_condition::check_consistency() { all_enchantment_conditions.check(); }


std::vector<enchantment_condition> enchantment_condition::get_all() {
    return all_enchantment_conditions.get_all();
}

void enchantment_condition::reset() { all_enchantment_conditions.reset(); }

bool enchantment_condition::item_condition(const Character& guy, const item& it) const {
    return condition_functions[condition_function]->check_item_condition(guy, it);
}

bool enchantment_condition::generic_condition(const Character& guy, const bool active) const {
    return condition_functions[condition_function]->check_generic_condition(guy, active);
}

class enchantment_condition_always: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool /*active*/) const override {
        return true;
    }
};

class enchantment_condition_dawn: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool /*active*/) const override {
        return is_dawn(calendar::turn);
    }
};

class enchantment_condition_day: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool /*active*/) const override {
        return is_day(calendar::turn);
    }
};

class enchantment_condition_dusk: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool /*active*/) const override {
        return is_dusk(calendar::turn);
    }
};

class enchantment_condition_night: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool /*active*/) const override {
        return is_night(calendar::turn);
    }
};

class enchantment_condition_active: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool active) const override {
        return active;
    }
};

class enchantment_condition_inactive: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& /*guy*/, const bool active) const override {
        return !active;
    }
};

class enchantment_condition_inside: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& guy, const bool /*active*/) const override {
        return !get_map().is_outside(guy.bub_pos());
    }
};

class enchantment_condition_outside: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& guy, const bool /*active*/) const override {
        return get_map().is_outside(guy.bub_pos());
    }
};

class enchantment_condition_underground: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& guy, const bool /*active*/) const override {
        return guy.bub_pos().z() < 0;
    }
};

class enchantment_condition_aboveground: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& guy, const bool /*active*/) const override {
        return guy.bub_pos().z() >= 0;
    }
};

class enchantment_condition_underwater: public virtual enchantment_condition_function {
public:
    bool check_generic_condition(const Character& guy, const bool /*active*/) const override {
        return get_map().is_divable(guy.bub_pos());
    }
};

class enchantment_condition_has: public virtual enchantment_condition_function {
public:
    bool check_item_condition(const Character& guy, const item& it) const override {
        return guy.has_item(it);
    }
};

class enchantment_condition_wield: public virtual enchantment_condition_function {
public:
    bool check_item_condition(const Character& guy, const item& it) const override {
        return guy.is_wielding(it);
    }
};

class enchantment_condition_worn: public virtual enchantment_condition_function {
public:
    bool check_item_condition(const Character& guy, const item& it) const override {
        return guy.is_worn(it);
    }
};

class enchantment_condition_lua: public virtual enchantment_condition_function {
public:
    std::string lua_func;
    enchantment_condition_lua(std::string func): lua_func(func) {}
    bool check_item_condition(const Character& guy, const item& it) const override;
    bool check_generic_condition(const Character& guy, const bool active) const override;
};

auto enchantment_condition::condition_functions = std::map<
    std::string, std::shared_ptr<enchantment_condition_function>>{
    {"always", std::make_shared<enchantment_condition_always>()},
    {"dawn", std::make_shared<enchantment_condition_dawn>()},
    {"day", std::make_shared<enchantment_condition_day>()},
    {"dusk", std::make_shared<enchantment_condition_dusk>()},
    {"night", std::make_shared<enchantment_condition_night>()},
    {"active", std::make_shared<enchantment_condition_active>()},
    {"inactive", std::make_shared<enchantment_condition_inactive>()},
    {"inside", std::make_shared<enchantment_condition_inside>()},
    {"outside", std::make_shared<enchantment_condition_outside>()},
    {"underground", std::make_shared<enchantment_condition_underground>()},
    {"aboveground", std::make_shared<enchantment_condition_aboveground>()},
    {"underwater", std::make_shared<enchantment_condition_underwater>()},
    {"has", std::make_shared<enchantment_condition_has>()},
    {"wield", std::make_shared<enchantment_condition_wield>()},
    {"worn", std::make_shared<enchantment_condition_worn>()}};

#pragma once

#include "calendar.h"
#include "character.h"
#include "item.h"
#include "json.h"
#include "string_id.h"
#include "type_id.h"


class enchantment_condition_function {
public:
    enchantment_condition_function() = default;
    virtual ~enchantment_condition_function() = default;
    virtual bool check_item_condition(const Character& /*guy*/, const item& /*it*/) const {
        return false;
    }
    virtual bool check_generic_condition(const Character& /*guy*/, const bool /*active*/) const {
        return false;
    }
};

class enchantment_condition {
public:
    enchantment_condition() = default;

    static void load_enchantment_conditions(const JsonObject& jo, const std::string& src);

    void load(const JsonObject& jo, const std::string& src);

    static void check_consistency();

    void check() const;

    static std::vector<enchantment_condition> get_all();

    static void reset();

    // Returns the result of the condition_function's item condition
    // If is_item_condition is false, it returns false
    bool item_condition(const Character& guy, const item& it) const;

    // Returns the result of the condition_function's generic condition
    // If is_item_condition is true, it returns false
    bool generic_condition(const Character& guy, const bool active) const;

    // Generic Factory stuff
    enchantment_condition_id id;
    bool was_loaded = false;

    // Should check_item_condition or check_generic_condition be called?
    bool is_item_condition = false;

    // String used as key for condition_function map
    std::string condition_function;

    // String used for display of enchantment conditions in item info
    std::string condition_info;
    // First: String id of the condition function
    // Second: The condition, called via check_item_condition for item conditions
    // Or check_generic_condition for everything else
    static std::map<std::string, std::shared_ptr<enchantment_condition_function>>
        condition_functions;
};

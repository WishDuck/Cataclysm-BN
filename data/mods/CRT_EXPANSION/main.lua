local bionics = require("bionics/bionics")
local crt_lab_veh_house = require("mapgen/crtvehlab/house")
local crt_lab_veh_hall = require("mapgen/crtvehlab/hall")
local crt_lab_veh_main = require("mapgen/crtvehlab/lab")
local veilsight = require("items/armor/veilsight")
local mod = game.mod_runtime[game.current_mod]

mod.crt_lab_veh_house = crt_lab_veh_house
mod.crt_lab_veh_hall = crt_lab_veh_hall
mod.crt_lab_veh_main = crt_lab_veh_main
mod.on_creature_melee_attacked = bionics.on_creature_melee_attacked
mod.crt_veilsight_on_wear = veilsight.crt_veilsight_on_wear
mod.crt_veilsight_on_takeoff = veilsight.crt_veilsight_on_takeoff
mod.crt_veilsight_on_tick = veilsight.crt_veilsight_on_tick

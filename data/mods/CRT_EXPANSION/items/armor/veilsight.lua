local wear = {}
-- Ids
local BodypartNull = BodyPartTypeId.NULL_ID():int_id()
local CrtVeilsightEffect = EffectTypeId.new("crt_veilsight")
local CrtVeilsightQuickSwapsEffect = EffectTypeId.new("crt_veilsight_quick_swaps")

-- Tick counts
local crt_veilsight_tick = 0

wear.crt_veilsight_on_tick = function(params)
  local user = params.user
  if (not user:is_avatar() and not user:is_npc()) or not user:is_worn(params.item) then return 0 end
  crt_veilsight_tick = crt_veilsight_tick + 1
  -- Ten times per minute
  if crt_veilsight_tick ~= 6 then return 0 end
  crt_veilsight_tick = 0
  user:add_effect(CrtVeilsightEffect, TimeDuration.from_minutes(1), BodypartNull, 1)
  user:mod_pain_noresist(1)
  return 0
end

wear.crt_veilsight_on_takeoff = function(params) params.user:remove_effect(CrtVeilsightEffect) end

wear.crt_veilsight_on_wear = function(params)
  local user = params.user
  local item = params.item
  local intensity = user:get_effect_int(CrtVeilsightQuickSwapsEffect)
  if intensity > 1 then
    user:mod_pain_noresist(10 * (user:get_effect_int(CrtVeilsightQuickSwapsEffect) - 1))
    gapi.add_msg("Staring in and out... It's almost more disorenting then staying focused in the pain")
  elseif intensity == 1 then
    user:mod_pain_noresist(5)
    gapi.add_msg(
      "What you see at the start appears to get harsher and harsher, you don't know if you can handle it many more times..."
    )
  end
  user:add_effect(CrtVeilsightQuickSwapsEffect, TimeDuration.from_minutes(60))
  return 0
end

return wear

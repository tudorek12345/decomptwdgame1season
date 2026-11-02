-- Isolated runtime probe; does not bypass registration or change original saves.
local function log(message)
  local f=io.open("@REPORT_PATH@","a")
  if f then f:write(tostring(message).."\n");f:close() end
end
log("boot_probe_enter")
require("recovery_original_boot.lua")
log("original_boot_loaded")
Callback_OnGameSceneClose:Add(function(name) log("scene_close_next="..tostring(name)) end)
Callback_SceneOpenFinished:Add(function(scene)
  local ok,name=pcall(ResourceGetName,scene)
  log("scene_open="..tostring(ok and name or scene))
end)
local previous=EnginePostUpdate
local frames=0
function EnginePostUpdate(...)
  if previous then previous(unpack(arg)) end
  frames=frames+1
  if frames%300==0 then
    local ok,name=pcall(function() return ResourceGetName(SceneGetBottomScene()) end)
    log("frame="..frames.." bottom_scene="..tostring(ok and name or "unavailable"))
  end
end
ThreadStart(function()
  Sleep(5)
    log("registered="..tostring(IsRegistered()).." steam="..tostring(IsRunningSteam()))
    if IsRegistered() then
      log("request_start_slot_1")
      SaveSlots_StartPlaying(1)
    else
      log("registration_not_ready_no_scene_requested")
    end
  -- NewGame can replace the current scene/thread; do not put this yielding
  -- engine transition inside pcall (Lua 5.1 cannot yield across that boundary).
  for i=1,70 do
    Sleep(1)
    local success,value=pcall(function()
      local scene=SceneGetBottomScene()
      local player=Game_GetPlayer and Game_GetPlayer()
      return "sample="..i.." scene="..tostring(scene).." player="..tostring(player)
    end)
    log(success and value or ("sample_error="..tostring(value)))
  end
end)

-- Loaded through _system so the monitor survives NewGame's script-state reset.
local function log(message)
  local f=io.open("@REPORT_PATH@","a")
  if f then f:write(tostring(message).."\n");f:close() end
end
log("system_monitor_loaded")
Callback_OnGameSceneClose:Add(function(name) log("system_scene_close_next="..tostring(name)) end)
Callback_SceneOpenFinished:Add(function(scene)
  local ok,name=pcall(ResourceGetName,scene)
  log("system_scene_open="..tostring(ok and name or scene))
end)
local previous=EnginePostUpdate
local frames=0
function EnginePostUpdate(...)
  if previous then previous(unpack(arg)) end
  frames=frames+1
  if frames%300==0 then
    local ok,name=pcall(function() return ResourceGetName(SceneGetBottomScene()) end)
    log("system_frame="..frames.." bottom_scene="..tostring(ok and name or "unavailable"))
  end
end

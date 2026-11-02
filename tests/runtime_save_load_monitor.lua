-- Opt-in acceptance fixture in the isolated TWD-Rebuild-01 namespace.
-- A phase file survives LoadGame replacing the Lua state; no original saves used.
local kSave="recovery_hash_symbols.save"
local kKey="RE - Recovery Sentinel"
local kValue="source-hash-symbols-20261102"
local function log(message)
  local f=io.open("@REPORT_PATH@","a")
  if f then f:write(tostring(message).."\n");f:close() end
end
local function phase(value)
  local f=io.open("@PHASE_PATH@",value and "w" or "r")
  if not f then return nil end
  if value then f:write(value);f:close();return value end
  local result=f:read("*a");f:close();return result
end
local previous=EnginePostUpdate
local frames=0
local scheduled=false
local verified=false
log("save_monitor_loaded phase="..tostring(phase()))
function EnginePostUpdate(...)
  if previous then previous(unpack(arg)) end
  frames=frames+1
  if frames%300~=0 then return end
  local ok,name=pcall(function() return ResourceGetName(SceneGetBottomScene()) end)
  if not ok or name~="adv_streetoutskirts.scene" then return end
  if phase()=="load_requested" and SaveLoad_IsFromLoad and SaveLoad_IsFromLoad() then
    local agent=AgentFind("logic_checkpoint")
    local value=agent and agent.mProps[kKey]
    log("reload_observation frame="..frames.." scene="..name.." loaded="..tostring(SaveLoad_IsFromLoad()).." sentinel="..tostring(value))
    if value==kValue then phase("complete");verified=true;log("save_reload_sentinel_verified") end
  elseif phase()=="complete" then
    log("post_reload_frame="..frames.." scene="..name)
  elseif not scheduled and frames>=1800 and not phase() then
    scheduled=true
    ThreadStart(function()
      -- Respect the game's own save lock and initialization conditions.
      for attempt=1,90 do
        if SaveLoad_IsEnabled and SaveLoad_IsEnabled() and AgentFind("logic_checkpoint") then break end
        log("waiting_for_save_enabled attempt="..attempt)
        Sleep(1)
      end
      if not SaveLoad_IsEnabled() then log("save_disabled_no_request");return end
      local props=AgentFind("logic_checkpoint").mProps
      if not PropertyExists(props,kKey) then PropertyCreate(props,kKey,"String",kValue) else props[kKey]=kValue end
      log("save_request filename="..kSave.." user_directory="..tostring(FileGetUserDirectory()).." sentinel="..tostring(props[kKey]))
      phase("save_requested")
      local saved=SaveLoad_Save(kSave)
      log("save_return="..tostring(saved))
      if not saved then phase("save_failed");return end
      Sleep(3)
      log("load_request filename="..kSave)
      phase("load_requested")
      SaveLoad_Load(kSave)
    end)
  end
end

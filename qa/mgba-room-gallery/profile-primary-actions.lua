-- Profile the primary action menu on the PROFILE ROM.
--
-- Load build/zork-trilogy-profile.gba in mGBA, open Tools > Scripting, and run:
--   dofile('/Users/jeff/GitHub/zork/qa/mgba-room-gallery/profile-primary-actions.lua')
--
-- Each record restores a saved room, focuses a known target, presses Act, and
-- saves the VERBS frame. The PROFILE overlay shows "<probes>p <effects>e <ms>ms".
-- Screenshots go to build/mgba-profile-shots/.

local root = profileRoot or '/Users/jeff/GitHub/zork'
local vm = dofile(root .. '/build/mgba-profile/read-state.lua')
local outDir = root .. '/build/mgba-profile-shots/'

local records = {
  {label='z1-living-sword',    save='qa/mgba-room-gallery/route-data/zork-i-mine-audit/route-75-lit.sav',  chapter=0, room=75,  steps=0},
  {label='z1-living-lantern',  save='qa/mgba-room-gallery/route-data/zork-i-mine-audit/route-75-lit.sav',  chapter=0, room=75,  steps=2},
  {label='z1-cellar-trapdoor', save='qa/mgba-room-gallery/route-data/zork-i-mine-audit/route-33-lit.sav',  chapter=0, room=33,  steps=0},
  {label='z1-treasure-bag',    save='qa/mgba-room-gallery/route-data/zork-i-mine-audit/route-82-lit.sav',  chapter=0, room=82,  steps=2},
  {label='z2-gazebo-teapot',   save='qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/route-16-lit.sav',  chapter=1, room=16,  steps=1},
  {label='z2-riddle-door',     save='qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/route-238-lit.sav', chapter=1, room=238, steps=0},
  {label='z2-west-wall',       save='qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/route-204-lit.sav', chapter=1, room=204, steps=0},
  {label='z3-parapet-dm',      save='qa/mgba-room-gallery/route-data/zork-iii-audit/route-36-lit.sav',   chapter=2, room=36,  steps=0},
  {label='z3-prison-dm',       save='qa/mgba-room-gallery/route-data/zork-iii-audit/route-45-lit.sav',   chapter=2, room=45,  steps=0},
  {label='z3-tech-gold',       save='qa/mgba-room-gallery/route-data/zork-iii-audit/route-5-lit.sav',    chapter=2, room=5,   steps=3},
  {label='z3-museum-wooden',   save='qa/mgba-room-gallery/route-data/zork-iii-audit/route-173-lit.sav',  chapter=2, room=173, steps=2},
  {label='z3-hallway-wall',    save='qa/mgba-room-gallery/route-data/zork-iii-audit/route-54-lit.sav',   chapter=2, room=54,  steps=3},
}

local maxRecords = profileList and #profileList or profileMaxRecords or #records
local firstRecord = profileList and 1 or profileStart or 1
local cursor = firstRecord - 1
local function recordAt(i) return profileList and records[profileList[i]] or records[i] end
local record
local phase, frames, phaseFrames, hold, gap, pending, focusLeft, builds0, builtAt, firstAt
local captureSuffix, cachedOnce, firstShot
local captured, failed = 0, 0

local logFile = nil
if profileLog then
  local ok, file = pcall(io.open, profileLog, 'w')
  if ok then logFile = file end
end
local function log(msg)
  console:log('PROFILE ' .. msg)
  if logFile then logFile:write(msg .. '\n'); logFile:flush() end
end

local function pulse(key)
  pending = key
  hold = 30
  gap = 20
  frames = 0
end

local function startNext()
  cursor = cursor + 1
  if cursor > maxRecords then
    log('RUN COMPLETE captured=' .. captured .. ' failed=' .. failed)
    phase = 'done'
    return
  end
  record = recordAt(cursor)
  local ok = pcall(function()
    emu:reset()
    return emu:loadSaveFile(root .. '/' .. record.save, true)
  end)
  if not ok then
    log('SAVE LOAD FAILED ' .. record.label)
    failed = failed + 1
    phase = 'advance'
    return
  end
  phase, frames, phaseFrames, hold, gap, pending, focusLeft, builds0, builtAt, firstAt = 'boot', 0, 0, 0, 0, 0, record.steps, nil, nil, nil
  captureSuffix, cachedOnce, firstShot = '', false, false
  log('START ' .. record.label)
end

callbacks:add('frame', function()
  if phase == 'done' then return end
  if not record then startNext(); return end
  emu:setKeys(pending or 0)
  frames = frames + 1
  phaseFrames = phaseFrames + 1

  -- A key pulse is held for a few polls and then released; the game polls once
  -- per loop and looks for a newly pressed edge.
  if phase == 'act' and vm.screen() == 5 then
    if not firstAt then firstAt = phaseFrames end
    if phaseFrames >= firstAt + 20 and not firstShot then
      firstShot = true
      emu:screenshot(outDir .. record.label .. (captureSuffix or '') .. '-first.png')
      log('MENU-FIRST ' .. record.label .. (captureSuffix or '') ..
          ' frame=' .. tostring(firstAt) ..
          ' probes=' .. tostring(vm.menu_probes and vm.menu_probes() or -1) ..
          ' effects=' .. tostring(vm.menu_effects and vm.menu_effects() or -1) ..
              ' tok=' .. tostring(vm.tokenize and vm.tokenize() or -1) ..
              ' hash=' .. tostring(vm.hash_ticks and vm.hash_ticks() or -1) ..
              ' restore=' .. tostring(vm.restore_ticks and vm.restore_ticks() or -1) ..
              ' slowpc=' .. tostring(vm.slow_pc and vm.slow_pc() or -1) ..
              ' slowt=' .. tostring(vm.slow_ticks and vm.slow_ticks() or -1) ..
              ' slowop=' .. tostring(vm.slow_opcode and vm.slow_opcode() or -1) ..
          ' cache=' .. tostring(vm.menu_cache and vm.menu_cache() or -1))
    end
  end
  if hold > 0 then
    hold = hold - 1
    if hold == 0 then pending = 0 end
    emu:setKeys(pending or 0)
    if profileVerbose and phase == 'act' then
      log('HOLD ' .. record.label .. ' hold=' .. tostring(hold) ..
          ' screen=' .. tostring(vm.screen()) ..
          ' probes=' .. tostring(vm.menu_probes and vm.menu_probes() or -1) ..
          ' effects=' .. tostring(vm.menu_effects and vm.menu_effects() or -1) ..
              ' tok=' .. tostring(vm.tokenize and vm.tokenize() or -1) ..
              ' hash=' .. tostring(vm.hash_ticks and vm.hash_ticks() or -1) ..
              ' restore=' .. tostring(vm.restore_ticks and vm.restore_ticks() or -1) ..
              ' slowpc=' .. tostring(vm.slow_pc and vm.slow_pc() or -1) ..
              ' slowt=' .. tostring(vm.slow_ticks and vm.slow_ticks() or -1) ..
              ' slowop=' .. tostring(vm.slow_opcode and vm.slow_opcode() or -1) ..
          ' builds=' .. tostring(vm.menu_builds and vm.menu_builds() or -1))
    end
    return
  end
  if gap > 0 then
    gap = gap - 1
    if profileVerbose and phase == 'act' then
      log('GAP ' .. record.label .. ' gap=' .. tostring(gap) ..
          ' screen=' .. tostring(vm.screen()) ..
          ' probes=' .. tostring(vm.menu_probes and vm.menu_probes() or -1) ..
          ' effects=' .. tostring(vm.menu_effects and vm.menu_effects() or -1) ..
              ' tok=' .. tostring(vm.tokenize and vm.tokenize() or -1) ..
              ' hash=' .. tostring(vm.hash_ticks and vm.hash_ticks() or -1) ..
              ' restore=' .. tostring(vm.restore_ticks and vm.restore_ticks() or -1) ..
              ' slowpc=' .. tostring(vm.slow_pc and vm.slow_pc() or -1) ..
              ' slowt=' .. tostring(vm.slow_ticks and vm.slow_ticks() or -1) ..
              ' slowop=' .. tostring(vm.slow_opcode and vm.slow_opcode() or -1) ..
          ' builds=' .. tostring(vm.menu_builds and vm.menu_builds() or -1))
    end
    return
  end
  if phase == 'advance' then
    if phaseFrames > 20 then startNext() end
    return
  end
  if (phase == 'focus' or phase == 'dismiss' or phase == 'act') and phaseFrames % 30 == 0 then
    log('STATE ' .. record.label .. ' phase=' .. phase .. ' screen=' .. tostring(screen) ..
        ' room=' .. tostring(room) .. ' page=' .. tostring(vm.page and vm.page() or -1) ..
        '/' .. tostring(vm.page_count and vm.page_count() or -1) ..
        ' busy=' .. tostring(vm.busy()))
  end

  local screen = vm.screen()
  local chapter = vm.chapter()
  local room = vm.room()

  if phase == 'boot' then
    if phaseFrames > 80 then phase = 'chapter'; frames, phaseFrames = 0, 0 end
  elseif phase == 'chapter' then
    if screen == 0 and chapter ~= record.chapter then
      pulse(128) -- Down: next chapter
    elseif screen == 0 and chapter == record.chapter then
      pulse(1) -- A: open chapter
    elseif screen == 1 then
      phase = 'continue'; frames, phaseFrames = 0, 0
    end
    if phaseFrames > 600 then
      log('CHAPTER TIMEOUT ' .. record.label .. ' screen=' .. tostring(screen) .. ' chapter=' .. tostring(chapter))
      failed = failed + 1; phase = 'advance'; frames = 0
    end
  elseif phase == 'continue' then
    if screen == 1 then
      pulse(1) -- A: Continue saved game
    elseif screen == 2 and room == record.room and vm.busy() == 0 then
      phase = 'settle'; frames, phaseFrames = 0, 0
    end
    if phaseFrames > 600 then
      log('ROOM TIMEOUT ' .. record.label .. ' screen=' .. tostring(screen) .. ' room=' .. tostring(room))
      failed = failed + 1; phase = 'advance'; frames = 0
    end
  elseif phase == 'settle' then
    if phaseFrames > 120 then phase = 'focus'; frames, phaseFrames = 0, 0 end
  elseif phase == 'focus' then
    if focusLeft > 0 then
      pulse(256) -- R: next target; the room starts on the name row
      focusLeft = focusLeft - 1
    else
      builds0 = vm.menu_builds and vm.menu_builds() or -1
      builtAt, firstAt = nil, nil
      phase = 'act'; frames, phaseFrames = 0, 0
      pulse(1) -- A: Act
    end
  elseif phase == 'cacheback' then
    pulse(1) -- A: reopen the same target; the cache should answer instantly
    phase = 'cacheact'; frames, phaseFrames = 0, 0
    builds0 = vm.menu_builds and vm.menu_builds() or -1
    firstAt, builtAt, firstShot = nil, nil, false
  elseif phase == 'cacheact' then
    phase = 'act'; frames, phaseFrames = 0, 0
  elseif phase == 'act' then
    local builds = vm.menu_builds and vm.menu_builds() or -1
    local built = builds ~= -1 and builds ~= builds0
    if screen == 5 then
      if built then
        if not builtAt then builtAt = phaseFrames end
        if phaseFrames - builtAt >= 10 then
          emu:screenshot(outDir .. record.label .. (captureSuffix or '') .. '.png')
          log('CAPTURED ' .. record.label .. (captureSuffix or '') ..
              ' room=' .. tostring(room) ..
              ' first=' .. tostring(firstAt) .. ' settled=' .. tostring(phaseFrames) ..
              ' probes=' .. tostring(vm.menu_probes and vm.menu_probes() or -1) ..
              ' effects=' .. tostring(vm.menu_effects and vm.menu_effects() or -1) ..
              ' tok=' .. tostring(vm.tokenize and vm.tokenize() or -1) ..
              ' hash=' .. tostring(vm.hash_ticks and vm.hash_ticks() or -1) ..
              ' restore=' .. tostring(vm.restore_ticks and vm.restore_ticks() or -1) ..
              ' slowpc=' .. tostring(vm.slow_pc and vm.slow_pc() or -1) ..
              ' slowt=' .. tostring(vm.slow_ticks and vm.slow_ticks() or -1) ..
              ' slowop=' .. tostring(vm.slow_opcode and vm.slow_opcode() or -1) ..
              ' ticks=' .. tostring(vm.menu_ticks and vm.menu_ticks() or -1) ..
              ' peak=' .. tostring(vm.menu_peak and vm.menu_peak() or -1) ..
              ' cache=' .. tostring(vm.menu_cache and vm.menu_cache() or -1) ..
              ' builds=' .. tostring(builds))
          captured = captured + 1
          if profileCache and not cachedOnce then
            cachedOnce = true
            captureSuffix = '-cache'
            phase = 'cacheback'; frames, phaseFrames = 0, 0
            pulse(2) -- B: back to the room without a story command
          else
            phase = 'advance'; frames, phaseFrames = 0, 0
          end
        end
      end
    elseif phaseFrames > 1800 then
      log('ACT TIMEOUT ' .. record.label .. ' screen=' .. tostring(screen) ..
          ' builds=' .. tostring(builds) .. '/' .. tostring(builds0))
      failed = failed + 1; phase = 'advance'; frames, phaseFrames = 0, 0
    end
  end
end)

log('Using profile addresses from build/mgba-profile/read-state.lua')
startNext()

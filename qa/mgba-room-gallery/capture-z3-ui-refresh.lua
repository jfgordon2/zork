-- Native mGBA replay for the Royal Puzzle restore and Zork III ending UI.
-- Load the current trilogy ROM, then run this file from mGBA's Scripting window
-- with galleryCaptureRoot set to the repository root.
local root = assert(galleryCaptureRoot, 'Set galleryCaptureRoot before loading this script')
local gallery = root .. '/qa/mgba-room-gallery/'
local routeRoot = gallery .. 'route-data/zork-iii-audit/'
local vm = dofile(root .. '/build/mgba-room-captures/read-state.lua')
local reportPath = gallery .. 'z3-ui-refresh-results.csv'
local report = assert(io.open(reportPath, 'w'))
report:write('screenshot,status,label,chapter,expected_room,actual_room\n')
report:flush()

local key = {A=1, B=2, START=8, RIGHT=16, LEFT=32, UP=64, DOWN=128, R=256}
local keyChars = "abcdefghijklmnopqrstuvwxyz0123456789.,'\"-!?:/()&"
local steps, cursor = {}, 1
local activeKey, held, released, injected = 0, 0, 0, 0
local waitFrames, polls, done = 0, 0, false

local function press(value)
  steps[#steps + 1] = {key=value}
end

local function pressUntil(value, label, predicate, limit)
  steps[#steps + 1] = {repeatKey=value, label=label, predicate=predicate, limit=limit or 1800}
end

local function waitFor(label, predicate, limit)
  steps[#steps + 1] = {wait=true, label=label, predicate=predicate, limit=limit or 1800}
end

local function chooseZorkIII()
  steps[#steps + 1] = {chooseChapter=true}
  waitFor('Zork III slot', function() return vm.screen() == 1 end)
  press(key.A)
end

local function continueAt(room)
  chooseZorkIII()
  waitFor('restored room ' .. room, function()
    return vm.screen() == 2 and vm.room() == room and vm.busy() == 0
  end)
end

local function typeCommand(command)
  pressUntil(key.START, 'pause menu for ' .. command, function() return vm.screen() == 17 end)
  for _ = 1, 6 do press(key.DOWN) end -- Enter command...
  press(key.A)
  waitFor('command keyboard', function() return vm.screen() == 22 end)
  local position = 0
  for i = 1, #command do
    local char = command:sub(i, i)
    if char == ' ' then
      press(key.R)
    else
      local target = string.find(keyChars, char, 1, true)
      assert(target, 'No on-screen key for ' .. char)
      target = target - 1
      while position + 10 <= target do press(key.DOWN); position = position + 10 end
      while position - 10 >= target do press(key.UP); position = position - 10 end
      while position < target do press(key.RIGHT); position = position + 1 end
      while position > target do press(key.LEFT); position = position - 1 end
      press(key.A)
    end
  end
  pressUntil(key.START, 'command result: ' .. command, function()
    return vm.screen() == 2 and vm.busy() == 0
  end)
end

local function capture(path, label, room, screen)
  steps[#steps + 1] = {capture=path, label=label, room=room, screen=screen}
end

local function resetFrom(save, room)
  steps[#steps + 1] = {reset=true, save=save}
  local launcherPolls = 0
  waitFor('launcher after SRAM settle', function()
    launcherPolls = launcherPolls + 1
    return launcherPolls >= 80 and vm.screen() == 0
  end, 1800)
  continueAt(room)
end

-- Open the Royal Puzzle save in Continue, invoke the game's Restore action,
-- and verify it returns to the same puzzle room without advancing the story.
resetFrom(routeRoot .. 'puzzle-initial.sav', 194)
pressUntil(key.START, 'pause before restore', function() return vm.screen() == 17 end)
for _ = 1, 3 do press(key.DOWN) end -- Restore game
press(key.A)
waitFor('Royal Puzzle restored', function()
  return vm.screen() == 2 and vm.room() == 194 and vm.busy() == 0
end)
capture(gallery .. 'zork-iii/states/puzzle-restored.png', 'Royal Puzzle after in-game Restore', 194, 2)

-- Follow the real last-stage route from the prison checkpoint through the
-- bronze door and into the Treasury. Enter commands with the GBA keyboard.
resetFrom(routeRoot .. 'route-204-lit.sav', 204)
typeCommand('unlock bronze door with key')
typeCommand('open bronze door')
typeCommand('south')
waitFor('successful ending text', function()
  return vm.screen() == 25 and vm.room() == 46 and vm.busy() == 0
end, 3600)
capture(gallery .. 'zork-iii/states/ending-story.png', 'Zork III successful ending', 46, 25)

-- Continue the narrated ending until the GBA Restart / Restore / Quit choices
-- appear. Stop before accepting a choice.
steps[#steps + 1] = {advanceEnding=true}
waitFor('ending choice menu', function() return vm.screen() == 26 and vm.room() == 46 end, 2400)
capture(gallery .. 'zork-iii/states/ending-options.png', 'Zork III ending options', 46, 26)

local function writeResult(step, status, actualRoom)
  local relative = step.capture:sub(#gallery + 1)
  report:write(relative, ',', status, ',', step.label, ',2,', step.room, ',', actualRoom or '', '\n')
  report:flush()
end

local function fail(message)
  console:error('Z3 UI ROUTE FAILED: ' .. message)
  report:close()
  done = true
end

callbacks:add('keysRead', function()
  polls = polls + 1
  if not done then emu:setKeys(injected) end
end)

callbacks:add('frame', function()
  if done then return end
  if held > 0 then
    injected = activeKey
    held = held - 1
    emu:setKeys(injected)
    return
  end
  if released > 0 then
    injected = 0
    released = released - 1
    emu:setKeys(0)
    return
  end
  emu:setKeys(0)

  local step = steps[cursor]
  if not step then
    report:close()
    console:log('Z3 UI ROUTE COMPLETE; puzzle restore and ending options captured')
    done = true
    return
  end

  if step.key ~= nil then
    activeKey, held, released = step.key, 14, 22
    injected = activeKey
    held = held - 1
    emu:setKeys(injected)
    cursor = cursor + 1
    return
  end
  if step.repeatKey ~= nil then
    if step.predicate() then
      console:log('Z3 UI wait passed: ' .. step.label .. ' screen=' .. vm.screen() .. ' room=' .. vm.room())
      waitFrames = 0
      cursor = cursor + 1
      return
    end
    waitFrames = waitFrames + 1
    if waitFrames > step.limit then
      fail(step.label .. ' timed out at screen=' .. vm.screen() .. ' room=' .. vm.room())
      return
    end
    injected = waitFrames % 36 <= 14 and step.repeatKey or 0
    emu:setKeys(injected)
    return
  end
  if step.wait then
    waitFrames = waitFrames + 1
    if step.predicate() then
      console:log('Z3 UI wait passed: ' .. step.label .. ' screen=' .. vm.screen() .. ' room=' .. vm.room())
      waitFrames = 0
      cursor = cursor + 1
    elseif waitFrames > step.limit then
      fail(step.label .. ' timed out at screen=' .. vm.screen() .. ' room=' .. vm.room())
    end
    return
  end
  if step.chooseChapter then
    if vm.screen() == 0 and vm.chapter() == 2 then
      activeKey, held, released = key.A, 14, 22
      injected = activeKey
      held = held - 1
      emu:setKeys(injected)
      cursor = cursor + 1
    elseif vm.screen() == 0 then
      activeKey, held, released = key.DOWN, 14, 22
      injected = activeKey
      held = held - 1
      emu:setKeys(injected)
    end
    return
  end
  if step.reset then
    emu:reset()
    local ok, loaded = pcall(function() return emu:loadSaveFile(step.save, true) end)
    if not ok or not loaded then fail('could not load ' .. step.save); return end
    cursor = cursor + 1
    return
  end
  if step.advanceEnding then
    if vm.screen() == 26 then
      cursor = cursor + 1
    elseif vm.screen() == 25 then
      activeKey, held, released = key.A, 14, 22
      injected = activeKey
      held = held - 1
      emu:setKeys(injected)
      -- Keep this step active; it advances one ending page at a time.
    elseif vm.screen() ~= 25 then
      fail('ending left the text screen before choices appeared'); return
    end
    return
  end
  if step.capture then
    if not step.settlePolls then
      step.settleFrames = 24
      step.settlePolls = polls + 2
      return
    end
    if step.settleFrames > 0 then
      step.settleFrames = step.settleFrames - 1
      return
    end
    if polls < step.settlePolls then return end
    if vm.room() ~= step.room or vm.screen() ~= step.screen or vm.busy() ~= 0 then
      writeResult(step, 'capture-state-mismatch', vm.room())
      fail(step.label .. ' expected screen=' .. step.screen .. ' room=' .. step.room ..
           ', got screen=' .. vm.screen() .. ' room=' .. vm.room())
      return
    end
    emu:screenshot(step.capture)
    writeResult(step, 'captured', vm.room())
    console:log('Z3 UI captured ' .. step.label .. ' room=' .. vm.room())
    cursor = cursor + 1
  end
end)

console:log('Z3 UI route replay started')

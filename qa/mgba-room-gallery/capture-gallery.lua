-- mGBA Lua queue for the route-based room gallery.
-- Capture only after live RAM confirms the expected chapter and room.
local root=assert(galleryCaptureRoot,'Set galleryCaptureRoot before loading capture-gallery.lua')
local gallery=root..'/qa/mgba-room-gallery/'
local vm=dofile(root..'/build/mgba-room-captures/read-state.lua')
assert(roomCapturePlan,'Load build/mgba-room-captures/plan.lua before this script')

local first=galleryCaptureStart or 1
local last=galleryCaptureEnd or #roomCapturePlan
local reportPath=galleryCaptureReportPath or (gallery..'capture-results.csv')
local report=assert(io.open(reportPath,'w'))
report:write('screenshot,status,label,chapter,expected_room,actual_room\n')
report:flush()

local cursor=first-1
local record,phase,elapsed,pauseFrames,keys,routeStep
-- The game polls keys once per main-loop pass, after it finishes drawing. Room changes
-- land in RAM before the redraw, so a capture also waits for two fresh polls.
local polls,settlePolls=0,0
local captured,failed=0,0
local done=false

local function writeResult(status,actualRoom)
  local relative=record.out:sub(#gallery+1)
  report:write(relative,',',status,',',record.label,',',record.chapter,',',record.room,',',actualRoom or '', '\n')
  report:flush()
end

local function finishRecord(status,actualRoom)
  writeResult(status,actualRoom)
  if status=='captured' then
    captured=captured+1
    if captured%20==0 then
      console:log('GALLERY PROGRESS captured='..captured..' failed='..failed..' last='..record.label)
    end
  else
    failed=failed+1
    console:error('GALLERY '..status..' expected='..record.label..' chapter='..record.chapter..' room='..record.room..' actual='..tostring(actualRoom))
  end
  cursor=cursor+1
  phase=nil
  pauseFrames=1
end

local function startNext()
  if cursor>=last then
    done=true
    report:close()
    console:log('GALLERY RUN COMPLETE range='..first..'-'..last..' captured='..captured..' failed='..failed)
    return
  end
  record=roomCapturePlan[cursor+1]
  local ok,loaded=pcall(function()
    emu:reset()
    return emu:loadSaveFile(record.save,true)
  end)
  if not ok or not loaded then
    console:error('GALLERY save-load failed '..record.label..' '..tostring(loaded))
    writeResult('save-load-failed','')
    failed=failed+1
    cursor=cursor+1
    pauseFrames=1
    return
  end
  phase='select-chapter'
  elapsed=0
  keys=0
  pauseFrames=80
end

callbacks:add('keysRead',function()
  polls=polls+1
  if not done then emu:setKeys(keys or 0) end
end)

callbacks:add('frame',function()
  if done then return end
  if not phase then startNext();return end
  if pauseFrames and pauseFrames>0 then
    pauseFrames=pauseFrames-1
    keys=0
    emu:setKeys(0)
    return
  end
  if phase=='settle' then
    if polls<settlePolls then return end
    emu:screenshot(record.out)
    console:log('GALLERY captured '..record.label..' room='..vm.room())
    finishRecord('captured',vm.room())
    return
  end

  elapsed=elapsed+1
  local screen,chapter,room=vm.screen(),vm.chapter(),vm.room()
  local idle=screen==2 and vm.busy()==0
  local function ready()
    if record.capture_view=='bag' then
      phase='open-bag';elapsed=0;keys=0
    else
      phase='settle';elapsed=0;pauseFrames=24;keys=0;settlePolls=polls+2
    end
    console:log('GALLERY ready '..record.label..' chapter='..chapter..' room='..room)
  end
  -- A record with `route` restores at `start_room`, then walks each {dir=, room=} step
  -- through the Paths menu before capturing (for states that have no save of their own).
  local step=record.route and record.route[routeStep]
  if phase=='select-chapter' and chapter==record.chapter then
    phase='open-chapter';elapsed=0;keys=0
  elseif phase=='open-chapter' and screen==1 then
    phase='continue-save';elapsed=0;keys=0
  elseif phase=='continue-save' and idle and chapter==record.chapter and room==(record.start_room or record.room) then
    if record.route then
      phase='route-open';routeStep=1;elapsed=0;keys=0
      console:log('GALLERY route start '..record.label..' room='..room)
    else
      ready()
    end
  elseif phase=='route-open' and screen==16 then
    phase='route-select';elapsed=0;keys=0
  elseif phase=='route-select' and screen~=16 then
    phase='route-await';elapsed=0;keys=0
  elseif phase=='route-await' and idle and room==step.room then
    routeStep=routeStep+1
    if record.route[routeStep] then phase='route-open';elapsed=0;keys=0 else ready() end
  elseif phase=='route-await' and idle and elapsed>180 then
    finishRecord('route-mismatch',room)
    return
  elseif phase=='open-bag' and screen==3 then
    phase='settle';elapsed=0;pauseFrames=24;keys=0;settlePolls=polls+2
    console:log('GALLERY bag ready '..record.label)
  elseif phase=='continue-save' and idle and elapsed>180 then
    finishRecord('room-mismatch',room)
    return
  elseif elapsed>900 then
    finishRecord('route-timeout',room)
    return
  end

  local key=0
  if phase=='select-chapter' then
    if screen==0 and chapter~=record.chapter then key=128 end
  elseif phase=='open-chapter' then
    if screen==0 then key=1 end
  elseif phase=='continue-save' then
    if screen==1 then key=1 end
  elseif phase=='route-open' then
    if idle then key=64 end
  elseif phase=='route-select' then
    local target
    for i=0,vm.path_count()-1 do
      if vm.path(i)==step.dir then target=i end
    end
    if not target then finishRecord('route-exit-missing',room);return end
    local selected=vm.selected()
    key=selected<target and 128 or selected>target and 64 or 1
  elseif phase=='open-bag' then
    key=4
  end
  keys=elapsed%36<14 and key or 0
  emu:setKeys(keys)
end)

startNext()

-- Re-capture a named subset of the gallery with the current ROM in mGBA.
--
-- In mGBA's Scripting window, after loading the rebuilt cartridge and running
-- `python3 qa/mgba-room-gallery/build_plan.py`:
--   galleryCaptureRoot='/path/to/zork'; galleryRefreshSet='troll'
--   dofile(galleryCaptureRoot..'/qa/mgba-room-gallery/refresh-sets.lua')
-- For a one-off subset, set galleryRefreshLabels={...} and galleryRefreshReport='name'
-- instead of galleryRefreshSet. Results go to <name>-refresh-results.csv beside this file.
-- For the whole gallery, use refresh-gallery.lua.
local root = assert(galleryCaptureRoot, 'Set galleryCaptureRoot before loading refresh-sets.lua')

local sets = {
  ['art-followup'] = {  -- Art/state pairs changed in the latest consistency pass.
    'zork-i-route-room-16-room-16', 'zork-i-route-room-31-room-31', 'zork-i-route-room-72-room-72',
    'zork-i-route-room-86-room-86', 'zork-i-route-room-167-room-167', 'zork-i-route-room-202-room-202',
    'zork-ii-route-room-15-room-15', 'zork-ii-story-state-46-wooden-door-shut',
    'zork-ii-story-state-46-wooden-door-open', 'zork-ii-route-room-59-room-59',
    'zork-ii-story-state-59-wooden-door-entered',
  },
  ['boat-interior'] = {  -- The shared boat interior in every aboard state.
    'zork-i-story-state-37-boat-afloat', 'zork-i-story-state-37-boat-with-lantern',
    'zork-i-story-state-37-river-launched', 'zork-i-story-state-111-inside-boat',
    'zork-i-story-state-174-cargo-downstream',
  },
  ['cliff-man'] = {'zork-iii-story-state-218-cliff-man'},  -- Upper-shelf Cliff Man.
  ['door-pair'] = {'zork-ii-story-state-28-guarded-open', 'zork-ii-story-state-45-workshop-open'},
  ['followup'] = {  -- Visual review states that were still open.
    'zork-i-story-state-78-cyclops-awake', 'zork-i-story-state-111-inside-boat',
    'zork-i-story-state-178-dam-sluice-open', 'zork-i-story-state-178-dam-sluice-closed',
    'zork-i-story-state-241-egypt-open', 'zork-i-story-state-241-egypt-empty',
    'zork-ii-route-room-28-room-28', 'zork-ii-story-state-28-guarded-open',
    'zork-ii-story-state-64-banker-appeared', 'zork-ii-story-state-64-banker-timeout',
    'zork-ii-story-state-87-robot-square-outcome', 'zork-ii-story-state-159-menhir-enchanted',
    'zork-ii-story-state-159-menhir-floated', 'zork-ii-story-state-162-cerberus-collared',
    'zork-ii-story-state-162-cerberus-guarding', 'zork-iii-route-room-172-room-172',
    'zork-iii-story-state-6-beam-before', 'zork-iii-story-state-6-beam-after',
    'zork-iii-story-state-54-guardians-before-potion', 'zork-iii-story-state-54-guardians-after-potion',
    'zork-iii-story-state-181-sailor-visible', 'zork-iii-story-state-217-junction-sword-taken',
    'zork-iii-story-state-218-cliff-man',
  },
  ['grating'] = {  -- Zork I grate backgrounds above and below.
    'zork-i-story-state-160-room-160', 'zork-i-story-state-160-grating-exposed',
    'zork-i-story-state-160-grating-open-above', 'zork-i-story-state-216-room-216',
    'zork-i-story-state-216-grating-open-below', 'zork-i-story-state-216-grating-return',
  },
  ['ice-frozen'] = {'zork-ii-story-state-161-ice-frozen'},  -- Route replay: 206 -> north -> down.
  ['red-mist'] = {  -- Story-dependent dark/visible views through the sphere.
    'zork-ii-death-route-224-room-224', 'zork-ii-story-state-224-bomb-detonated-here',
    'zork-ii-story-state-224-bomb-stayed', 'zork-ii-story-state-224-cage-timeout',
    'zork-ii-story-state-224-cage-without-robot', 'zork-ii-story-state-224-dragon-attack-outcome',
    'zork-ii-story-state-224-orange-cake-outcome', 'zork-ii-story-state-224-red-cake-outcome',
    'zork-ii-story-state-224-round-button-outcome', 'zork-ii-story-state-224-square-button-outcome',
  },
  ['troll'] = {'zork-i-story-state-127-troll-defeated'},  -- Troll disappears after defeat.
}

local name = galleryRefreshSet or galleryRefreshReport
local labels = galleryRefreshLabels or sets[name or '']
if not labels then
  local names = {}
  for key in pairs(sets) do names[#names + 1] = key end
  table.sort(names)
  error('Set galleryRefreshSet to one of: ' .. table.concat(names, ', ') ..
        ' (or galleryRefreshLabels and galleryRefreshReport)')
end
assert(name, 'Set galleryRefreshReport to name the results file')

dofile(root .. '/build/mgba-room-captures/plan.lua')
local wanted = {}
for _, label in ipairs(labels) do wanted[label] = true end
local targets = {}
for _, record in ipairs(roomCapturePlan) do
  if wanted[record.label] then
    targets[#targets + 1] = record
    wanted[record.label] = nil
  end
end
for label in pairs(wanted) do error('Capture label missing from plan: ' .. label) end

roomCapturePlan = targets
galleryCaptureStart, galleryCaptureEnd = 1, #targets
galleryCaptureReportPath = root .. '/qa/mgba-room-gallery/' .. name .. '-refresh-results.csv'
galleryRefreshSet, galleryRefreshLabels, galleryRefreshReport = nil, nil, nil
dofile(root .. '/qa/mgba-room-gallery/capture-gallery.lua')

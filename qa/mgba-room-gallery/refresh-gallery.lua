-- Run this file from mGBA's Scripting window after loading the freshly built ROM.
-- It restores each traced SRAM checkpoint, verifies the live chapter and room,
-- then captures the full 240x160 frame through capture-gallery.lua.
local root=assert(galleryCaptureRoot,'Set galleryCaptureRoot before loading refresh-gallery.lua')

dofile(root..'/build/mgba-room-captures/plan.lua')
galleryCaptureStart=1
galleryCaptureEnd=#roomCapturePlan
galleryCaptureReportPath=galleryCaptureReportPath or root..'/qa/mgba-room-gallery/capture-results-refresh.csv'
dofile(root..'/qa/mgba-room-gallery/capture-gallery.lua')

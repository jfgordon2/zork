#include "room_visuals.h"
#include "zork1_scenes.h"
#include "zork2_scenes.h"
#include "visual_state.h"

typedef struct {
    uint8_t width, height;
    const Color *pixels;
} SceneSprite;

#include "encounter_art.h"
#include "item_art.h"

#define SHADOW RGB(19, 25, 24)
#define EDGE RGB(92, 104, 97)
#define STEEL RGB(146, 170, 167)
#define CREAM RGB(236, 221, 178)
#define GOLD RGB(249, 192, 93)
#define INK RGB(22, 32, 31)
#define TEAL RGB(147, 190, 158)
#define BROWN RGB(151, 102, 60)

#define Z3_OPAQUE(color) ((Color)(0x8000 | (color)))
static const Color ArtZ3HoodFloorPixels[] = {
    0,0,0,0,Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(117,96,149)),Z3_OPAQUE(RGB(117,96,149)),Z3_OPAQUE(RGB(92,76,122)),0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(92,76,122)),0,0,0,
    0,0,Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(143,119,177)),0,0,0,0,Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(92,76,122)),0,0,
    0,Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(143,119,177)),0,0,0,0,0,0,Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(92,76,122)),0,
    0,Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(143,119,177)),0,0,0,0,0,0,Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(92,76,122)),0,
    Z3_OPAQUE(RGB(71,62,96)),Z3_OPAQUE(RGB(104,87,137)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(104,87,137)),Z3_OPAQUE(RGB(71,62,96)),0,
    0,Z3_OPAQUE(RGB(71,62,96)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(166,143,196)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(71,62,96)),0,
    0,0,Z3_OPAQUE(RGB(71,62,96)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(117,96,149)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(143,119,177)),Z3_OPAQUE(RGB(117,96,149)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(71,62,96)),0,0,
    0,0,0,Z3_OPAQUE(RGB(71,62,96)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(117,96,149)),Z3_OPAQUE(RGB(117,96,149)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(71,62,96)),0,0,0,
    0,0,0,0,Z3_OPAQUE(RGB(71,62,96)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(92,76,122)),Z3_OPAQUE(RGB(71,62,96)),0,0,0,0,
    0,0,0,0,0,Z3_OPAQUE(RGB(71,62,96)),Z3_OPAQUE(RGB(71,62,96)),0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
};
static const SceneSprite ArtZ3HoodFloor = {12, 12, ArtZ3HoodFloorPixels};
_Static_assert(sizeof(ArtZ3HoodFloorPixels) / sizeof(Color) == 12 * 12, "ArtZ3HoodFloor rows must match its size");

static const Color ArtZ3BrokenTimberPixels[] = {
    0,0,0,Z3_OPAQUE(RGB(110,69,43)),Z3_OPAQUE(RGB(150,94,55)),Z3_OPAQUE(RGB(176,113,63)),Z3_OPAQUE(RGB(154,91,51)),Z3_OPAQUE(RGB(115,72,43)),0,0,0,0,0,0,0,0,0,0,
    0,0,Z3_OPAQUE(RGB(104,65,41)),Z3_OPAQUE(RGB(147,92,53)),Z3_OPAQUE(RGB(188,125,70)),Z3_OPAQUE(RGB(170,103,56)),Z3_OPAQUE(RGB(141,84,48)),Z3_OPAQUE(RGB(111,68,41)),Z3_OPAQUE(RGB(88,54,36)),0,0,0,0,0,0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(93,57,37)),Z3_OPAQUE(RGB(145,88,49)),Z3_OPAQUE(RGB(165,101,56)),Z3_OPAQUE(RGB(132,77,46)),Z3_OPAQUE(RGB(95,58,38)),Z3_OPAQUE(RGB(72,47,34)),Z3_OPAQUE(RGB(147,91,50)),Z3_OPAQUE(RGB(184,119,65)),Z3_OPAQUE(RGB(148,89,50)),Z3_OPAQUE(RGB(109,67,40)),0,0,0,0,0,
    0,0,0,0,Z3_OPAQUE(RGB(86,53,36)),Z3_OPAQUE(RGB(117,71,42)),Z3_OPAQUE(RGB(78,49,34)),Z3_OPAQUE(RGB(122,73,42)),Z3_OPAQUE(RGB(171,105,58)),Z3_OPAQUE(RGB(193,130,71)),Z3_OPAQUE(RGB(163,98,53)),Z3_OPAQUE(RGB(117,69,42)),Z3_OPAQUE(RGB(81,51,35)),0,0,0,0,0,
    0,0,0,0,0,Z3_OPAQUE(RGB(82,51,34)),Z3_OPAQUE(RGB(96,58,37)),Z3_OPAQUE(RGB(153,94,51)),Z3_OPAQUE(RGB(183,116,62)),Z3_OPAQUE(RGB(143,85,46)),Z3_OPAQUE(RGB(102,61,37)),Z3_OPAQUE(RGB(78,49,34)),Z3_OPAQUE(RGB(143,88,49)),Z3_OPAQUE(RGB(114,69,42)),0,0,0,0,
    0,0,0,0,Z3_OPAQUE(RGB(86,54,36)),Z3_OPAQUE(RGB(124,77,45)),Z3_OPAQUE(RGB(170,108,59)),Z3_OPAQUE(RGB(187,125,69)),Z3_OPAQUE(RGB(157,94,51)),Z3_OPAQUE(RGB(106,63,38)),Z3_OPAQUE(RGB(81,50,34)),Z3_OPAQUE(RGB(100,60,38)),Z3_OPAQUE(RGB(155,95,51)),Z3_OPAQUE(RGB(130,77,43)),0,0,0,0,
    0,0,0,0,0,Z3_OPAQUE(RGB(92,56,37)),Z3_OPAQUE(RGB(140,84,47)),Z3_OPAQUE(RGB(159,96,53)),Z3_OPAQUE(RGB(124,74,43)),Z3_OPAQUE(RGB(87,53,36)),0,0,Z3_OPAQUE(RGB(112,68,41)),Z3_OPAQUE(RGB(136,81,45)),0,0,0,0,
    0,0,0,0,0,0,Z3_OPAQUE(RGB(94,57,37)),Z3_OPAQUE(RGB(114,68,41)),Z3_OPAQUE(RGB(85,52,35)),0,0,0,0,Z3_OPAQUE(RGB(91,55,36)),0,0,0,0,
};
static const SceneSprite ArtZ3BrokenTimber = {18, 8, ArtZ3BrokenTimberPixels};
_Static_assert(sizeof(ArtZ3BrokenTimberPixels) / sizeof(Color) == 18 * 8, "ArtZ3BrokenTimber rows must match its size");

static const Color ArtCoalPilePixels[] = {
    0,0,0,0,0,0,Z3_OPAQUE(RGB(67,64,63)),Z3_OPAQUE(RGB(81,77,74)),0,0,0,0,0,0,0,0,
    0,0,0,0,Z3_OPAQUE(RGB(55,53,53)),Z3_OPAQUE(RGB(92,87,82)),Z3_OPAQUE(RGB(111,105,99)),Z3_OPAQUE(RGB(67,64,63)),0,0,0,0,0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(57,55,54)),Z3_OPAQUE(RGB(86,81,77)),Z3_OPAQUE(RGB(117,109,101)),Z3_OPAQUE(RGB(94,88,82)),Z3_OPAQUE(RGB(60,57,56)),Z3_OPAQUE(RGB(80,75,72)),0,0,0,0,0,0,0,
    0,0,Z3_OPAQUE(RGB(48,47,48)),Z3_OPAQUE(RGB(80,76,72)),Z3_OPAQUE(RGB(99,93,86)),Z3_OPAQUE(RGB(59,56,56)),Z3_OPAQUE(RGB(50,49,50)),Z3_OPAQUE(RGB(102,96,88)),Z3_OPAQUE(RGB(85,80,75)),Z3_OPAQUE(RGB(48,47,48)),0,0,0,0,0,0,
    0,Z3_OPAQUE(RGB(46,45,46)),Z3_OPAQUE(RGB(73,70,68)),Z3_OPAQUE(RGB(88,83,78)),Z3_OPAQUE(RGB(52,50,50)),Z3_OPAQUE(RGB(47,46,47)),Z3_OPAQUE(RGB(73,69,66)),Z3_OPAQUE(RGB(92,86,79)),Z3_OPAQUE(RGB(60,57,56)),Z3_OPAQUE(RGB(43,43,44)),0,0,0,0,0,0,
    0,Z3_OPAQUE(RGB(43,42,43)),Z3_OPAQUE(RGB(57,55,54)),Z3_OPAQUE(RGB(48,47,48)),Z3_OPAQUE(RGB(42,42,43)),Z3_OPAQUE(RGB(43,42,43)),Z3_OPAQUE(RGB(55,52,51)),Z3_OPAQUE(RGB(64,60,58)),Z3_OPAQUE(RGB(47,46,47)),Z3_OPAQUE(RGB(39,39,40)),0,0,0,0,0,0,
    0,0,Z3_OPAQUE(RGB(43,42,43)),Z3_OPAQUE(RGB(41,40,41)),Z3_OPAQUE(RGB(39,39,40)),Z3_OPAQUE(RGB(40,40,41)),Z3_OPAQUE(RGB(45,43,43)),Z3_OPAQUE(RGB(47,46,47)),Z3_OPAQUE(RGB(39,39,40)),0,0,0,0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(38,38,39)),Z3_OPAQUE(RGB(39,39,40)),Z3_OPAQUE(RGB(40,39,40)),Z3_OPAQUE(RGB(39,39,40)),Z3_OPAQUE(RGB(37,37,38)),0,0,0,0,0,0,0,0,
};
static const SceneSprite ArtCoalPile = {16, 8, ArtCoalPilePixels};
_Static_assert(sizeof(ArtCoalPilePixels) / sizeof(Color) == 16 * 8, "ArtCoalPile rows must match its size");

static const Color ArtBrokenStaffPixels[] = {
    0,0,0,0,0,Z3_OPAQUE(RGB(99,62,39)),Z3_OPAQUE(RGB(151,94,54)),Z3_OPAQUE(RGB(165,103,57)),Z3_OPAQUE(RGB(130,77,45)),0,0,0,0,0,
    0,0,0,0,Z3_OPAQUE(RGB(92,55,36)),Z3_OPAQUE(RGB(146,89,51)),Z3_OPAQUE(RGB(178,112,62)),Z3_OPAQUE(RGB(144,84,47)),Z3_OPAQUE(RGB(81,49,34)),0,0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(75,46,33)),Z3_OPAQUE(RGB(125,73,42)),Z3_OPAQUE(RGB(152,91,51)),Z3_OPAQUE(RGB(98,58,36)),0,0,0,0,0,0,0,
    0,0,Z3_OPAQUE(RGB(77,48,34)),Z3_OPAQUE(RGB(128,77,45)),Z3_OPAQUE(RGB(161,98,54)),Z3_OPAQUE(RGB(116,68,40)),Z3_OPAQUE(RGB(76,46,33)),0,0,0,0,0,0,0,
    0,Z3_OPAQUE(RGB(75,47,34)),Z3_OPAQUE(RGB(127,75,43)),Z3_OPAQUE(RGB(163,97,52)),Z3_OPAQUE(RGB(131,76,43)),Z3_OPAQUE(RGB(83,50,34)),0,0,0,0,0,0,0,0,
    Z3_OPAQUE(RGB(75,47,34)),Z3_OPAQUE(RGB(125,74,43)),Z3_OPAQUE(RGB(143,84,47)),Z3_OPAQUE(RGB(94,57,37)),Z3_OPAQUE(RGB(73,46,33)),0,0,0,0,0,0,0,0,0,
};
static const SceneSprite ArtBrokenStaff = {14, 6, ArtBrokenStaffPixels};
_Static_assert(sizeof(ArtBrokenStaffPixels) / sizeof(Color) == 14 * 6, "ArtBrokenStaff rows must match its size");

static const Color ArtBurnedTorchPixels[] = {
    0,0,0,0,0,0,0,0,0,0,Z3_OPAQUE(RGB(61,48,39)),0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,Z3_OPAQUE(RGB(74,53,38)),Z3_OPAQUE(RGB(54,43,36)),Z3_OPAQUE(RGB(92,65,43)),0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,Z3_OPAQUE(RGB(101,70,44)),Z3_OPAQUE(RGB(166,108,59)),Z3_OPAQUE(RGB(117,73,44)),Z3_OPAQUE(RGB(82,54,38)),Z3_OPAQUE(RGB(117,79,46)),0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,Z3_OPAQUE(RGB(110,75,47)),Z3_OPAQUE(RGB(165,106,57)),Z3_OPAQUE(RGB(125,78,45)),Z3_OPAQUE(RGB(91,58,38)),Z3_OPAQUE(RGB(137,90,51)),Z3_OPAQUE(RGB(151,100,57)),0,0,0,0,0,0,0,
    0,0,0,0,0,0,Z3_OPAQUE(RGB(113,76,47)),Z3_OPAQUE(RGB(161,104,57)),Z3_OPAQUE(RGB(132,81,46)),Z3_OPAQUE(RGB(104,64,39)),Z3_OPAQUE(RGB(151,97,54)),Z3_OPAQUE(RGB(162,107,59)),Z3_OPAQUE(RGB(116,73,44)),0,0,0,0,0,0,0,
    0,0,0,0,0,Z3_OPAQUE(RGB(106,70,44)),Z3_OPAQUE(RGB(151,96,54)),Z3_OPAQUE(RGB(125,76,43)),Z3_OPAQUE(RGB(98,61,39)),Z3_OPAQUE(RGB(141,89,50)),Z3_OPAQUE(RGB(153,99,56)),Z3_OPAQUE(RGB(108,67,41)),0,0,0,0,0,0,0,0,
    0,0,0,0,Z3_OPAQUE(RGB(96,63,41)),Z3_OPAQUE(RGB(138,87,49)),Z3_OPAQUE(RGB(115,70,41)),Z3_OPAQUE(RGB(88,56,38)),Z3_OPAQUE(RGB(126,78,45)),Z3_OPAQUE(RGB(133,83,47)),Z3_OPAQUE(RGB(92,58,38)),0,0,0,0,0,0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(84,56,38)),Z3_OPAQUE(RGB(119,75,44)),Z3_OPAQUE(RGB(95,60,38)),Z3_OPAQUE(RGB(77,51,36)),Z3_OPAQUE(RGB(104,65,40)),Z3_OPAQUE(RGB(110,69,41)),Z3_OPAQUE(RGB(78,51,36)),0,0,0,0,0,0,0,0,0,0,
    0,0,Z3_OPAQUE(RGB(72,50,37)),Z3_OPAQUE(RGB(97,63,39)),Z3_OPAQUE(RGB(79,52,36)),Z3_OPAQUE(RGB(67,47,36)),Z3_OPAQUE(RGB(84,55,37)),Z3_OPAQUE(RGB(88,57,38)),Z3_OPAQUE(RGB(66,46,35)),0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,Z3_OPAQUE(RGB(67,47,35)),Z3_OPAQUE(RGB(77,52,36)),Z3_OPAQUE(RGB(66,47,35)),Z3_OPAQUE(RGB(61,44,34)),Z3_OPAQUE(RGB(68,48,35)),Z3_OPAQUE(RGB(71,50,36)),Z3_OPAQUE(RGB(59,43,33)),0,0,0,0,0,0,0,0,0,0,
};
static const SceneSprite ArtBurnedTorch = {20, 10, ArtBurnedTorchPixels};
_Static_assert(sizeof(ArtBurnedTorchPixels) / sizeof(Color) == 20 * 10, "ArtBurnedTorch rows must match its size");

static const Color ArtLunchPixels[] = {
    0,0,Z3_OPAQUE(RGB(119,75,46)),Z3_OPAQUE(RGB(153,99,61)),Z3_OPAQUE(RGB(166,111,71)),Z3_OPAQUE(RGB(139,88,52)),0,0,0,0,0,0,
    0,Z3_OPAQUE(RGB(112,69,43)),Z3_OPAQUE(RGB(166,108,66)),Z3_OPAQUE(RGB(183,124,76)),Z3_OPAQUE(RGB(159,98,56)),Z3_OPAQUE(RGB(118,72,43)),Z3_OPAQUE(RGB(97,59,38)),0,0,0,0,0,
    Z3_OPAQUE(RGB(104,64,41)),Z3_OPAQUE(RGB(154,96,58)),Z3_OPAQUE(RGB(175,116,71)),Z3_OPAQUE(RGB(149,91,54)),Z3_OPAQUE(RGB(115,69,43)),Z3_OPAQUE(RGB(141,88,53)),Z3_OPAQUE(RGB(155,98,59)),Z3_OPAQUE(RGB(110,67,42)),0,0,0,0,
    Z3_OPAQUE(RGB(95,58,38)),Z3_OPAQUE(RGB(136,83,50)),Z3_OPAQUE(RGB(158,99,60)),Z3_OPAQUE(RGB(137,82,49)),Z3_OPAQUE(RGB(102,62,40)),Z3_OPAQUE(RGB(126,76,47)),Z3_OPAQUE(RGB(143,88,53)),Z3_OPAQUE(RGB(98,60,39)),0,0,0,0,
    0,Z3_OPAQUE(RGB(83,52,37)),Z3_OPAQUE(RGB(122,75,46)),Z3_OPAQUE(RGB(139,85,51)),Z3_OPAQUE(RGB(106,65,41)),Z3_OPAQUE(RGB(91,56,38)),Z3_OPAQUE(RGB(108,66,41)),Z3_OPAQUE(RGB(91,56,38)),0,0,0,0,
};
static const SceneSprite ArtLunch = {12, 5, ArtLunchPixels};
_Static_assert(sizeof(ArtLunchPixels) / sizeof(Color) == 12 * 5, "ArtLunch rows must match its size");

static const Color ArtOilLampPixels[] = {
    0,0,0,0,0,0,0,0,0,Z3_OPAQUE(RGB(146,96,47)),0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,Z3_OPAQUE(RGB(118,76,40)),Z3_OPAQUE(RGB(190,134,66)),Z3_OPAQUE(RGB(126,80,42)),0,0,0,0,0,
    0,0,0,0,0,0,0,Z3_OPAQUE(RGB(121,79,42)),Z3_OPAQUE(RGB(202,149,75)),Z3_OPAQUE(RGB(181,119,57)),Z3_OPAQUE(RGB(119,74,40)),Z3_OPAQUE(RGB(110,73,44)),0,0,0,0,
    0,0,Z3_OPAQUE(RGB(93,63,39)),Z3_OPAQUE(RGB(144,94,49)),Z3_OPAQUE(RGB(197,138,66)),Z3_OPAQUE(RGB(178,115,53)),Z3_OPAQUE(RGB(115,73,39)),Z3_OPAQUE(RGB(92,61,39)),0,0,Z3_OPAQUE(RGB(125,82,43)),0,0,0,0,0,
    0,Z3_OPAQUE(RGB(91,61,39)),Z3_OPAQUE(RGB(152,101,54)),Z3_OPAQUE(RGB(187,126,62)),Z3_OPAQUE(RGB(142,87,42)),Z3_OPAQUE(RGB(104,66,39)),Z3_OPAQUE(RGB(119,79,44)),Z3_OPAQUE(RGB(88,59,39)),0,Z3_OPAQUE(RGB(125,82,43)),0,0,0,0,0,0,
    Z3_OPAQUE(RGB(91,61,40)),Z3_OPAQUE(RGB(158,108,58)),Z3_OPAQUE(RGB(173,116,56)),Z3_OPAQUE(RGB(118,73,39)),Z3_OPAQUE(RGB(91,60,39)),Z3_OPAQUE(RGB(106,70,42)),Z3_OPAQUE(RGB(91,61,40)),Z3_OPAQUE(RGB(125,82,43)),0,Z3_OPAQUE(RGB(125,82,43)),0,0,0,0,0,0,
    0,Z3_OPAQUE(RGB(80,54,38)),Z3_OPAQUE(RGB(129,86,47)),Z3_OPAQUE(RGB(143,93,46)),Z3_OPAQUE(RGB(103,66,39)),Z3_OPAQUE(RGB(88,59,39)),Z3_OPAQUE(RGB(99,67,41)),Z3_OPAQUE(RGB(78,53,38)),0,Z3_OPAQUE(RGB(113,74,40)),0,0,0,0,0,0,
    0,0,Z3_OPAQUE(RGB(79,54,39)),Z3_OPAQUE(RGB(105,71,42)),Z3_OPAQUE(RGB(87,59,39)),Z3_OPAQUE(RGB(78,54,39)),Z3_OPAQUE(RGB(82,57,40)),0,0,0,0,0,0,0,0,0,
};
static const SceneSprite ArtOilLamp = {16, 8, ArtOilLampPixels};
_Static_assert(sizeof(ArtOilLampPixels) / sizeof(Color) == 16 * 8, "ArtOilLamp rows must match its size");

static const Color ArtZ3PedestalPixels[] = {
    0,0,Z3_OPAQUE(RGB(171,143,104)),Z3_OPAQUE(RGB(207,180,135)),Z3_OPAQUE(RGB(222,197,152)),Z3_OPAQUE(RGB(211,182,136)),Z3_OPAQUE(RGB(183,153,111)),0,
    0,Z3_OPAQUE(RGB(124,103,77)),Z3_OPAQUE(RGB(175,148,113)),Z3_OPAQUE(RGB(208,185,148)),Z3_OPAQUE(RGB(212,187,148)),Z3_OPAQUE(RGB(176,145,105)),Z3_OPAQUE(RGB(129,105,76)),0,
    Z3_OPAQUE(RGB(95,87,76)),Z3_OPAQUE(RGB(120,114,104)),Z3_OPAQUE(RGB(151,144,128)),Z3_OPAQUE(RGB(163,151,132)),Z3_OPAQUE(RGB(151,139,119)),Z3_OPAQUE(RGB(126,115,98)),Z3_OPAQUE(RGB(98,91,79)),Z3_OPAQUE(RGB(78,73,65)),
    Z3_OPAQUE(RGB(87,79,68)),Z3_OPAQUE(RGB(117,108,94)),Z3_OPAQUE(RGB(140,132,117)),Z3_OPAQUE(RGB(140,132,117)),Z3_OPAQUE(RGB(130,122,108)),Z3_OPAQUE(RGB(110,101,88)),Z3_OPAQUE(RGB(88,80,70)),Z3_OPAQUE(RGB(70,64,56)),
    0,Z3_OPAQUE(RGB(81,74,64)),Z3_OPAQUE(RGB(115,107,93)),Z3_OPAQUE(RGB(136,127,110)),Z3_OPAQUE(RGB(136,127,110)),Z3_OPAQUE(RGB(106,97,84)),Z3_OPAQUE(RGB(78,70,61)),0,
    0,Z3_OPAQUE(RGB(75,69,60)),Z3_OPAQUE(RGB(108,101,88)),Z3_OPAQUE(RGB(129,121,106)),Z3_OPAQUE(RGB(122,113,99)),Z3_OPAQUE(RGB(99,91,80)),Z3_OPAQUE(RGB(71,65,57)),0,
    0,Z3_OPAQUE(RGB(80,73,63)),Z3_OPAQUE(RGB(111,103,90)),Z3_OPAQUE(RGB(135,125,109)),Z3_OPAQUE(RGB(135,125,109)),Z3_OPAQUE(RGB(110,100,87)),Z3_OPAQUE(RGB(78,71,62)),0,
    Z3_OPAQUE(RGB(67,61,53)),Z3_OPAQUE(RGB(93,86,75)),Z3_OPAQUE(RGB(120,110,95)),Z3_OPAQUE(RGB(130,120,103)),Z3_OPAQUE(RGB(121,111,96)),Z3_OPAQUE(RGB(96,88,76)),Z3_OPAQUE(RGB(69,63,55)),Z3_OPAQUE(RGB(51,47,42)),
};
static const SceneSprite ArtZ3Pedestal = {8, 8, ArtZ3PedestalPixels};
_Static_assert(sizeof(ArtZ3PedestalPixels) / sizeof(Color) == 8 * 8, "ArtZ3Pedestal rows must match its size");
#undef Z3_OPAQUE

typedef enum {
    MARK, ITEM, PERSON, KEY_ICON, PAPER_ICON, LAMP, SWORD, DOOR_ICON, BOX_ICON,
    POT, CAKE, GEM, HANDLE_ICON, MATCHBOOK, FOLDED_PAPER, COFFIN, CHALICE, TRIDENT, STAND,
    CYCLOPS, VIAL, WIZARD, DEMON, WAND, GOLD_POT, TROLL, THIEF, BAT, GHOSTS, ROBOT, DRAGON,
    DRAGON_DOWN, CERBERUS, PRINCESS, UNICORN, GNOME, SERPENT, OLD_MAN, HOODED,
    CLIFF_MAN, DUNGEON_MASTER, AXE, HOOD, CLOAK, GARLIC_ICON
} Icon;

// Pixels use a shared outline, shadow, midtone, and highlight. A is a material accent.
static const char icons[][STORY_ICON_SIZE][STORY_ICON_SIZE + 1] = {
    [MARK] = {
        "         ", "         ", "    L    ", "   LGL   ", "  LGAAL  ",
        "   LAL   ", "    L    ", "         ", "         "
    },
    [ITEM] = {
        "         ", "   KKK   ", "  KGALK  ", " KGALLLK ", " KGGALLK ",
        " KGGAAAK ", "  KGGAK  ", "   KKK   ", "         "
    },
    [PERSON] = {
        "   KKK   ", "  KMLMK  ", "  KMMAK  ", "   KKK   ", "  KAKAK  ",
        " KMAAAMK ", " KMAAAAK ", " KAAAAAK ", "  KKKKK  "
    },
    [KEY_ICON] = {
        "     KKK ", "    KGLGK", "    KG KG", "   KGALAK", "  KGGAKK ",
        " KGGAK   ", "KLGAK    ", "KGA K    ", " KK      "
    },
    [PAPER_ICON] = {
        " KKKKK   ", " KLLLKK  ", " KLMLLMK ", " KLLLLLK ", " KLMMMLK ",
        " KLLLLLK ", " KLMMMLK ", " KLLLLLK ", " KKKKKKK "
    },
    [LAMP] = {
        "   KGK   ", "  KG GK  ", "  KKKKK  ", "  KGAGK  ", "  KAAAK  ",
        "  KGAGK  ", "  KGAGK  ", " KGGGGGK ", "  KKKKK  "
    },
    [SWORD] = {
        "      KL ", "     KLM ", "    KLMK ", "   KLMK  ", " KKKMK   ",
        "KGGLKK   ", " KAKGK   ", "KAK KGK  ", " K   K   "
    },
    [DOOR_ICON] = {
        " KKKKKKK ", " KMLLLGK ", " KMGAGGK ", " KMGAGGK ", " KMGALGK ",
        " KMGAGGK ", " KMGAGGK ", " KMGAGGK ", "KKKKKKKKK"
    },
    [BOX_ICON] = {
        "         ", "  KKKKK  ", " KGAGAGK ", "KGGGGGGGK", "KAAALAAAK",
        "KGGGLGGGK", "KAGGGGGAK", " KKKKKKK ", "         "
    },
    [POT] = {
        "    K    ", "  KKLK   ", "K KLLLKK ", "KMKLLLLAK", " KMLLMK K",
        " KMLLLKAK", "  KMMMAK ", "   KKK   ", "         "
    },
    [CAKE] = {
        "         ", "  KKKKK  ", " KLLALAK ", "KLALALALK", "KLLLLLLLK",
        "KGBGBGBGK", " KKKKKKK ", "         ", "         "
    },
    [GEM] = {
        "         ", "  KKKKK  ", " KLGGAMK ", "KGGLLAAAK", " KGAAAAMK",
        "  KAAAMK ", "   KAMK  ", "    K    ", "         "
    },
    [HANDLE_ICON] = {
        "         ", "    KKK  ", "  KKMMLK ", " KMALAMK ", "KMALMAAK ",
        " KAAAMK  ", "  KKKK   ", "    K    ", "         "
    },
    [MATCHBOOK] = {
        "         ", "         ", "  KKKKK  ", " KGGAAAK ", " KAGLLAK ",
        "KAGAABK  ", "KBBBKK   ", " KKK     ", "         "
    },
    [FOLDED_PAPER] = {
        "         ", "         ", " KKKK    ", " KLLLKK  ", " KLLMLLK ",
        " KMLMLLK ", " KLMMAK  ", "  KKKK   ", "         "
    },
    [COFFIN] = {
        "         ", "         ", "  KKKK   ", " KGLGGKK ", " KGAGGLKK",
        " KAGGGAAK", "  KKKKKK ", "         ", "         "
    },
    [CHALICE] = {
        " KKKKKKK ", "KLMLMLMLK", " KMLALMK ", "  KLAMK  ", "   KLK   ",
        "   KMK   ", "  KMLAK  ", " KMLLLMK ", "  KKKKK  "
    },
    [TRIDENT] = {
        " K  K  K ", " KL KL KL", " KL KL KL", " KL KL KL", "  KLLLMK ",
        "    KLK  ", "    KLK  ", "    KMK  ", "    KMK  "
    },
    [STAND] = {
        "         ", "  KKKKK  ", " KAMMAAK ", "  KLLMK  ", "   KAK   ",
        "   KAK   ", "  KAAAK  ", " KMMAMMK ", "  KKKKK  "
    },
    [CYCLOPS] = {
        "   KKK   ", "  KGAGK  ", "  KMLMK  ", "  KAAAK  ", " KBGGBGK ",
        "KBBAABBBK", "KBGAAAGBK", "  KKBKK  ", "  KB KB  "
    },
    [VIAL] = {
        "         ", "   KGK   ", "   KMK   ", "   KLK   ", "  KLMKK  ",
        " KMLLMAK ", " KMLLAAK ", "  KMAAK  ", "   KKK   "
    },
    [WIZARD] = {
        "    K    ", "   KK    ", "   KAK   ", " KKAAMKK ", "  KLBLK  ",
        "  KLLAK  ", " KALLAMK ", " KAAAMAK ", "KKKKKKKKK"
    },
    [DEMON] = {
        " K     K ", " KMKKKMK ", "  KAAAK  ", " KAGAGAK ", " KAMAMAK ",
        "KKKAAAKKK", "KAAKAKAAK", "KAAMAMAAK", " KKKKKKK "
    },
    [WAND] = {
        "        L", "       GG", "      GK ", "     AK  ", "    AK   ",
        "   AK    ", "  BK     ", " KB      ", "KK       "
    },
    [GOLD_POT] = {
        "         ", "     GL  ", "  LGGLGG ", " KGGLLGAK", " KKKKKKKK",
        " KAMMMMAK", " KAMAAAAK", "  KAAAAK ", "   KKKK  "
    },
    [TROLL] = {
        "  K K KML", "  KAK KML", " KGLAK K ", " KAAA K  ", "KBAABKKB ",
        "KAABAAKB ", " KBBBKB  ", " KAKA K  ", " KBKBK   "
    },
    [THIEF] = {
        "    KK   ", "   KAAK  ", "  KKLLKK ", "  KLKKLK ", " KKAAA K ",
        "KBBKAL K ", "KBBKAAKKL", " KKAABK  ", "  KKKKK  "
    },
    [BAT] = {
        "         ", "K       K", "KAK K KAK", "KAABKBBAK", " KBBGGBK ",
        "  KKKKK  ", "    K    ", "         ", "         "
    },
    [GHOSTS] = {
        " KK   KK ", "KMK  KAMK", "KALK KLMK", "KAMK KMAK", "KLMAKLMMK",
        " KAKKAMK ", " KAMKAMK ", "  KAKAK  ", "   K K   "
    },
    [ROBOT] = {
        "    G    ", "  KKMKK  ", "  KMLMK  ", "  KKKKK  ", "KMMAAMMMK",
        "KAKLGKAMK", "K KAMAK K", "  KMKMK  ", " KKK KKK "
    },
    [DRAGON] = {
        " K K     ", "KAAK  K  ", "KGA KAAK ", " KBAAAAK ", "KKAAABAAK",
        "KLBAABBBA", " KAAAABK ", " KAKKAAK ", " KK  KK  "
    },
    [DRAGON_DOWN] = {
        "         ", "         ", "         ", "         ", " K    K  ",
        "KAK  KAAK", "KBAKABBAK", " KKBBBBKK", "  KKKKK  "
    },
    [CERBERUS] = {
        "KK KK KK ", "KAKGAKGAK", "KLKLLKLAK", "KAAKBKAAK", " KABAABK ",
        "  KAAAK K", "  KABAAK ", "  KAKAK  ", "  KK KK  "
    },
    [PRINCESS] = {
        "    G    ", "   KBK   ", "   KLK   ", "  KBLAK  ", " KLAAALK ",
        "  KAAAK  ", " KALAAAK ", " KAAMAAK ", "KKKKKKKKK"
    },
    [UNICORN] = {
        "   G     ", " KKG     ", "KLMAK    ", " KLAKKKK ", "  KLLLAAK",
        "  KLGLLAK", "  KLLAK K", "  KK KK  ", "  KK KK  "
    },
    [GNOME] = {
        "         ", "         ", "  KKKK   ", "  KLMLK  ", "  KLALK  ",
        " KALAKKK ", " KABAKAGK", "  KAAKKKK", "  KKKK   "
    },
    [SERPENT] = {
        "   KKK   ", "  KGAAL  ", "  KABKK  ", "   KAK   ", "   KMK  K",
        "   KAKKKM", "KAKKAAA K", " KAAAABK ", "  KKKKK  "
    },
    [OLD_MAN] = {
        "         ", "         ", "   KKK   ", "  KMLMK  ", "  KLALK  ",
        " KAMLAK  ", "KAAAMABK ", "KAABABBAK", " KKKKKKK "
    },
    [HOODED] = {
        "    K    ", "   KAK   ", "  KAAAK L", "  KKMKK L", " KAABAAKL",
        "KAAAABAAL", " KABAALKL", " KABAAK  ", "KKKKKKKK "
    },
    [CLIFF_MAN] = {
        "   KKK   ", "  KBBAK  ", "  KLLLK  ", "   KBK  B", "  KAAK KB",
        " KAAGKLKB", " KLAAKB K", "  KBBK  B", "  KK KK  "
    },
    [DUNGEON_MASTER] = {
        "    K  G ", "   KAK B ", "  KAAK B ", "  KMLK B ", "  KLLKLB ",
        " KAGAKBB ", " KAAAAMB ", " KAAAMKB ", "KKKKKKKB "
    },
    [AXE] = {
        "      ML ", "     MMLK", "   BMMMK ", "  BKMMK  ", " BK KK   ",
        " BK      ", "BK       ", "K        ", "         "
    },
    [HOOD] = {
        "         ", "    K    ", "   KAK   ", "  KAMAK  ", "  KAKAK  ",
        " KAAKAAK ", " KAKKKAK ", "  KKKKK  ", "         "
    },
    [CLOAK] = {
        "   KKK   ", "  KAGAK  ", "  KAAMK  ", " KAKAAMK ", " KAKAAAK ",
        "KAMKAAAAK", "KAAKAMAAK", " KKKKKKK ", "         "
    },
    [GARLIC_ICON] = {
        "    B    ", "   KAK   ", "  KAAAK  ", " KALAAAK ", "KALAAAALK",
        "KALAAAALK", " KALAAAK ", "  KAAAK  ", "   KKK   "
    },
};

static void sprite(Icon kind, int x, int y, int scale, Color accent) {
    const Color colors[] = {SHADOW, STEEL, CREAM, GOLD, BROWN, accent};
    const char keys[] = "KMLGBA";
    for (int row = 0; row < STORY_ICON_SIZE; row++) {
        for (int col = 0; col < STORY_ICON_SIZE; col++) {
            char ch = icons[kind][row][col];
            if (ch == ' ') continue;
            for (unsigned i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
                if (keys[i] == ch) {
                    fill(x + col * scale, y + row * scale, scale, scale, colors[i]);
                    break;
                }
            }
        }
    }
}

typedef struct {
    uint8_t x, y;
} ScenePoint;

static void scene_segment(ScenePoint a, ScenePoint b, Color color);

typedef enum { FACE_PLAYER, FACE_LEFT, FACE_RIGHT } Facing;
typedef enum { ENCOUNTER_GROUNDED, ENCOUNTER_FLOATING, ENCOUNTER_HANGING, ENCOUNTER_SWIMMING } EncounterMotion;

typedef struct {
    uint8_t chapter, object, home_room;
    ScenePoint at;
    Facing facing;
    EncounterMotion motion;
    uint8_t hover;
    bool floor_shadow;
    Icon icon;
    Color accent;
    const SceneSprite *art;
} Encounter;

// `at` is the floor contact for grounded figures and the projected floor point
// for floaters. Use a non-grounded motion only when the story puts the figure
// in the air, on a ceiling, or in water; seated and resting figures stay grounded.
// Hanging and swimming poses use their own attachment/waterline.
// The object IDs refer to the three exact story releases bundled in the ROM.
#define GROUNDED(ch, obj, room, x, y, face, icon, color, pose) \
    {ch, obj, room, {x, y}, face, ENCOUNTER_GROUNDED, 0, true, icon, color, pose}
#define FLOATING(ch, obj, room, x, y, face, height, shadow, icon, color, pose) \
    {ch, obj, room, {x, y}, face, ENCOUNTER_FLOATING, height, shadow, icon, color, pose}
#define HANGING(ch, obj, room, x, y, face, icon, color, pose) \
    {ch, obj, room, {x, y}, face, ENCOUNTER_HANGING, 0, false, icon, color, pose}
#define SWIMMING(ch, obj, room, x, y, face, icon, color, pose) \
    {ch, obj, room, {x, y}, face, ENCOUNTER_SWIMMING, 0, false, icon, color, pose}
static const Encounter encounters[] = {
    GROUNDED(0, ZORK1_TROLL, 127, 124, 82, FACE_LEFT, TROLL, RGB(130, 165, 91), &ArtTrollArmed),
    GROUNDED(0, 164, 82, 153, 87, FACE_RIGHT, THIEF, RGB(102, 127, 121), &ArtThiefEncounter),
    HANGING(0, 70, 235, 50, 26, FACE_PLAYER, BAT, RGB(127, 102, 125), &ArtBatEncounter), // clings to cave ceiling
    // Keep the Cyclops clear of the stair flight at the room's right edge.
    GROUNDED(0, 199, 78, 145, 85, FACE_PLAYER, CYCLOPS, RGB(172, 126, 85), &ArtCyclopsAwake),
    FLOATING(0, 211, 38, 120, 83, FACE_PLAYER, 10, false, GHOSTS, RGB(155, 191, 190), &ArtGhostsEncounter), // incorporeal

    FLOATING(1, 18, 0, 171, 83, FACE_RIGHT, 13, true, WIZARD, RGB(117, 126, 190), &ArtWizardEncounter), // described floating
    GROUNDED(1, 26, 0, 155, 82, FACE_RIGHT, UNICORN, RGB(166, 182, 184), &ArtUnicornEncounter),
    SWIMMING(1, ZORK2_SERPENT, ZORK2_AQUARIUM_ROOM, 98, 53, FACE_LEFT, SERPENT, RGB(110, 164, 147), &ArtSerpentSwimming), // swims in aquarium
    GROUNDED(1, 57, 231, 111, 85, FACE_LEFT, DRAGON_DOWN, RGB(146, 103, 80), &ArtDragonFallen),
    GROUNDED(1, 58, 162, 145, 86, FACE_PLAYER, CERBERUS, RGB(138, 107, 94), &ArtCerberusEncounter),
    GROUNDED(1, 98, 87, 112, 82, FACE_PLAYER, ROBOT, RGB(129, 148, 160), &ArtRobotEncounter),
    GROUNDED(1, 133, ZORK2_BANK_SMALL_ROOM, 122, 84, FACE_RIGHT, GNOME, RGB(110, 152, 125), &ArtBankerEncounter),
    GROUNDED(1, 150, ZORK2_DRAGONS_LAIR, 156, 85, FACE_RIGHT, PRINCESS, RGB(173, 113, 147), &ArtPrincessEncounter),
    GROUNDED(1, ZORK2_DRAGON, ZORK2_DRAGON_ROOM, 184, 73, FACE_LEFT, DRAGON, RGB(185, 77, 60), &ArtDragonStanding),
    FLOATING(1, 229, 91, 119, 83, FACE_PLAYER, 10, true, DEMON, RGB(149, 83, 123), &ArtDemonEncounter), // appears in the air
    GROUNDED(1, 239, 0, 59, 80, FACE_RIGHT, GNOME, RGB(162, 110, 70), &ArtVolcanoGnomeEncounter),

    GROUNDED(2, 1, 0, 91, 84, FACE_LEFT, DUNGEON_MASTER, RGB(157, 128, 90), &ArtMasterEncounter),
    GROUNDED(2, 11, 0, 91, 84, FACE_LEFT, DUNGEON_MASTER, RGB(157, 128, 90), &ArtMasterEncounter),
    GROUNDED(2, ZORK3_OLD_MAN, ZORK3_ENGRAVINGS_ROOM, 68, 86, FACE_RIGHT, OLD_MAN, RGB(170, 133, 97), &ArtOldManAwake),
    GROUNDED(2, ZORK3_HOODED_FIGURE, 0, 75, 84, FACE_RIGHT, HOODED, RGB(115, 123, 167), &ArtHoodedStanding),
    GROUNDED(2, 139, 109, 132, 84, FACE_RIGHT, CLIFF_MAN, RGB(169, 123, 79), &ArtCliffManEncounter),
    // Story places him at the cliff edge above the player's lower ledge. This
    // small pose stands on the distinct upper shelf and looks down toward them.
    GROUNDED(2, ZORK3_CLIFF_MAN, ZORK3_CLIFF_LEDGE, 205, 34, FACE_RIGHT, CLIFF_MAN, RGB(169, 123, 79), &ArtCliffManLookingDown),
};
#undef GROUNDED
#undef FLOATING
#undef HANGING
#undef SWIMMING

static const SceneSprite *encounter_pose(const Encounter *figure, unsigned room) {
    switch (figure->icon) {
        case TROLL: {
            const uint8_t *strength;
            // A negative STRENGTH marks an unconscious troll in Zork I.
            if (z_property(figure->object, 7, &strength) == 2 && (strength[0] & 0x80)) return &ArtTrollDown;
            if (z_parent(36) != figure->object) return &ArtTrollUnarmed;
            break;
        }
        case CYCLOPS:
            if (z_global(10)) return &ArtCyclopsAsleep;
            break;
        case BAT:
            if (room == 235) return &ArtBatHanging;
            break;
        case DRAGON:
            if (room == ZORK2_DRAGON_ROOM)
                return z_global(121) >= 2 ? &ArtDragonStanding : &ArtDragonResting;
            break;
        case PRINCESS:
            // Release 63 sets this flag when the princess comes out of her trance.
            if (room == ZORK2_DRAGONS_LAIR && !z_global(82)) return &ArtPrincessSeated;
            break;
        case CERBERUS:
            if (z_parent(72) == 58) return &ArtCerberusFriendlyEncounter;
            break;
        case UNICORN:
            if (room == 16) return &ArtUnicornGarden;
            break;
        case OLD_MAN:
            if (!z_global(116)) return &ArtOldManAsleep;
            break;
        case HOODED:
            if (story_figure_defenseless()) return &ArtHoodedKneeling;
            break;
        default: break;
    }
    return figure->art;
}

void draw_story_icon(unsigned chapter, unsigned object, int x, int y) {
    for (unsigned i = 0; i < sizeof(encounters) / sizeof(encounters[0]); i++) {
        if (encounters[i].chapter != chapter || encounters[i].object != object) continue;
        sprite(encounters[i].icon, x, y, 1, encounters[i].accent);
        return;
    }
    char name[Z_NAME_MAX];
    story_noun(object, name);
    Icon kind = MARK;
    Color accent = TEAL;
    if (text_has(name, "garlic")) {
        kind = GARLIC_ICON;
        accent = RGB(225, 211, 170);
    } else if (text_has(name, "cake")) {
        kind = CAKE;
        if (text_has(name, "red")) accent = RGB(201, 105, 88);
        else if (text_has(name, "blue")) accent = RGB(96, 155, 198);
        else if (text_has(name, "orange")) accent = RGB(218, 144, 89);
    } else if (text_has(name, "teapot")) kind = POT;
    else if (text_equal(name, "vial")) kind = VIAL;
    else if (text_equal(name, "pot of gold")) kind = GOLD_POT;
    else if (text_equal(name, "wand")) kind = WAND;
    else if (text_equal(name, "bloody axe")) kind = AXE;
    else if (text_equal(name, "hood")) kind = HOOD;
    else if (text_equal(name, "cloak")) kind = CLOAK;
    else if (text_equal(name, "match")) {
        kind = MATCHBOOK;
        accent = RGB(161, 78, 60);
    }
    else if (text_has(name, "lamp") || text_has(name, "lantern") || text_has(name, "torch") ||
             z_attr(object, ATTR_LIGHT)) {
        kind = LAMP;
        accent = z_attr(object, ATTR_LIT) ? CREAM : BROWN;
    } else if (z_attr(object, ATTR_ACTOR)) kind = PERSON;
    else if (text_has(name, "key") && !text_has(name, "keyhole")) kind = KEY_ICON;
    else if (text_has(name, "door") || text_has(name, "window")) kind = DOOR_ICON;
    else if (text_has(name, "chalice")) kind = CHALICE;
    else if (text_has(name, "trident")) kind = TRIDENT;
    else if (z_attr(object, ATTR_WEAPON)) kind = SWORD;
    else if (text_has(name, "ring") || text_has(name, "jewel") || text_has(name, "sphere") || text_has(name, "gem")) kind = GEM;
    else if (z_attr(object, ATTR_CONTAINER)) kind = BOX_ICON;
    else if (z_attr(object, ATTR_READABLE)) kind = PAPER_ICON;
    else if (z_attr(object, ATTR_PORTABLE)) kind = ITEM;
    sprite(kind, x, y, 1, accent);
}

typedef struct {
    uint8_t chapter, room, object, x, y, width, height;
} RoomAnchor;

static const RoomAnchor anchors[] = {
    {0, 11, 57, 25, 48, 185, 15},
    {0, ZORK1_LADDER_TOP, ZORK1_MINE_LADDER, 163, 45, 47, 43},
    {0, ZORK1_LADDER_TOP, ZORK1_STAIRS, 10, 18, 57, 50},
    {0, 18, 79, 87, 57, 73, 22},
    {0, 33, 240, 101, 17, 36, 18},
    {0, ZORK1_GAS_ROOM, ZORK1_STAIRS, 7, 23, 57, 47},
    {0, 52, 183, 39, 22, 159, 40},
    {0, 52, 234, 39, 22, 159, 40},
    {0, ZORK1_LADDER_BOTTOM, ZORK1_MINE_LADDER, 164, 17, 24, 65},
    {0, 89, 101, 198, 40, 8, 9},
    {0, 75, 100, 7, 17, 34, 43},
    {0, 75, 240, 81, 70, 97, 17},
    {0, 85, 243, 45, 21, 101, 50},
    {0, 106, 222, 207, 35, 25, 23},
    {0, 110, 234, 39, 22, 159, 40},
    {0, 110, 183, 39, 22, 159, 40},
    {0, 123, 6, 8, 51, 224, 20},
    {0, 129, 35, 74, 64, 91, 17},
    {0, 245, 203, 60, 27, 54, 47},
    {0, 249, 203, 94, 48, 49, 31},
    {1, 238, 7, 90, 29, 118, 59},
    {1, 87, 1, 202, 26, 8, 10},
    {1, 87, 4, 211, 26, 8, 10},
    {1, 87, 27, 220, 26, 8, 10},
    {1, 63, 129, 174, 18, 50, 55},
    {1, 63, 236, 161, 54, 11, 7},
    {1, 63, 175, 71, 20, 31, 23},
    {1, 63, 96, 70, 43, 36, 20},
    {1, 63, 85, 112, 47, 39, 15},
    {1, 63, 10, 113, 25, 40, 15},
    {1, 204, 50, 23, 27, 91, 24},
    {1, 204, 145, 150, 15, 90, 50}, // east wall
    {1, 204, 211, 0, 15, 25, 50},   // west wall edge
    {1, 240, 240, 168, 53, 68, 37},
    {2, 36, 60, 110, 65, 24, 10},
    {2, 103, 10, 70, 24, 100, 46},
    {2, 181, ZORK3_VIKING_SHIP, 26, 32, 45, 31},
    {2, ZORK3_GUARDIANS_HALL, 152, 25, 23, 190, 59},
    {2, ZORK3_GUARDIANS_SOUTH, 152, 50, 27, 141, 39},
    {2, ZORK3_GUARDIANS_NORTH, 152, 46, 28, 149, 44},
    {2, ZORK3_NARROW_GUARDIANS_EAST, 152, 70, 20, 99, 47},
    {2, ZORK3_NARROW_GUARDIANS_WEST, 152, 70, 20, 99, 47},
    {2, ZORK3_NARROW_STATUES_EAST, 152, 103, 34, 34, 22},
    {2, ZORK3_NARROW_STATUES_WEST, 152, 103, 34, 34, 22},
    {2, ZORK3_PUZZLE_ENTRANCE, 154, 92, 17, 47, 36},
    {2, ZORK3_NARROW_GUARDIANS_EAST, 106, 145, 15, 82, 52},
    {2, ZORK3_NARROW_GUARDIANS_EAST, 29, 145, 15, 82, 52},
    {2, ZORK3_NARROW_GUARDIANS_WEST, 106, 13, 15, 82, 52},
    {2, ZORK3_NARROW_GUARDIANS_WEST, 29, 13, 15, 82, 52},
    {2, ZORK3_NARROW_STATUES_EAST, 106, 145, 15, 82, 52},
    {2, ZORK3_NARROW_STATUES_EAST, 29, 145, 15, 82, 52},
    {2, ZORK3_NARROW_STATUES_WEST, 106, 13, 15, 82, 52},
    {2, ZORK3_NARROW_STATUES_WEST, 29, 13, 15, 82, 52},
    {2, 8, 106, 145, 15, 82, 52}, {2, 8, 29, 145, 15, 82, 52},
    {2, 51, 106, 13, 15, 82, 52}, {2, 51, 29, 13, 15, 82, 52},
    {2, 100, 106, 145, 15, 82, 52}, {2, 100, 29, 145, 15, 82, 52},
    {2, 178, 106, 145, 15, 82, 52}, {2, 178, 29, 145, 15, 82, 52},
    {2, 119, 106, 145, 15, 82, 52}, {2, 119, 29, 145, 15, 82, 52},
    {2, 221, 106, 13, 15, 82, 52}, {2, 221, 29, 13, 15, 82, 52},
    {2, ZORK3_DUNGEON_ENTRANCE, ZORK3_DUNGEON_DOOR, 46, 24, 68, 52},
    {2, ZORK3_SCENIC_VISTA, 17, 8, 43, 45, 31},
    {2, ZORK3_SCENIC_VISTA, 3, 12, 17, 26, 24},
    {2, ZORK3_BUTTON_ROOM, 191, 166, 38, 24, 24},
    {2, ZORK3_PARAPET, 88, 68, 44, 130, 43},
    {2, ZORK3_NORTH_CORRIDOR, 183, 42, 20, 36, 44},
    {2, ZORK3_NORTH_CORRIDOR, 206, 84, 42, 108, 12},
    {2, ZORK3_NORTH_CORRIDOR, 4, 84, 17, 108, 29},
    {2, 143, 215, 97, 53, 46, 31},
    {2, 143, 154, 92, 17, 47, 36},
    {2, 45, 183, 166, 25, 35, 43},
    {2, 204, 153, 186, 17, 42, 36},
    // Zork III room 113: the shallow channel and the compass rose around its hole.
    {2, 113, 96, 70, 72, 104, 19},
    {2, 113, 107, 118, 48, 6, 22},
    // The other guardian hallways show the same floor features. 44/224 use the
    // long guardian-hallway art; 54 uses the mirror-hall view.
    {2, 44, 96, 70, 72, 104, 19},
    {2, 44, 107, 118, 48, 6, 22},
    {2, 54, 96, 83, 74, 83, 17},
    {2, 54, 107, 121, 68, 8, 23},
    {2, 224, 96, 70, 72, 104, 19},
    {2, 224, 107, 118, 48, 6, 22},
    {2, 204, 209, 84, 21, 74, 53},
    {2, ZORK3_LADDER_TOP, 68, 158, 59, 37, 31},
    {2, ZORK3_LADDER_TOP, 154, 5, 19, 66, 59},
    {2, ZORK3_LADDER_BOTTOM, 68, 137, 23, 39, 63},
    {2, ZORK3_SIDE_ROOM, 117, 179, 23, 39, 57},
    {2, ZORK3_SIDE_ROOM, 154, 49, 35, 65, 52},
    {2, ZORK3_SIDE_ROOM_ALT, 117, 179, 23, 39, 57},
    {2, ZORK3_SIDE_ROOM_ALT, 154, 49, 35, 65, 52},

    // Selectable scenery drawn directly into the Zork III backgrounds.
    {2, ZORK3_BEAM_ROOM, 27, 101, 57, 38, 2},
    {2, ZORK3_UNDERWATER, 94, 126, 75, 12, 8},
    {2, ZORK3_UNDERWATER, 114, 9, 62, 222, 27},
    {2, ZORK3_UNDERWATER, 144, 5, 48, 230, 35},
    {2, ZORK3_UNDERWATER, 223, 10, 35, 220, 38},
    {2, ZORK3_MUSEUM_ENTRANCE, 79, 183, 47, 27, 30},
    {2, ZORK3_MUSEUM_ENTRANCE, 82, 20, 28, 32, 38},
    {2, ZORK3_MUSEUM_ENTRANCE, 199, 102, 28, 34, 32},
    {2, ZORK3_MUSEUM_ENTRANCE, 205, 181, 31, 27, 35},
    {2, ZORK3_PARAPET, 4, 8, 27, 224, 29},
    {2, ZORK3_PARAPET, 206, 4, 65, 232, 20},
    {2, 45, 209, 188, 29, 33, 43},
    {2, ZORK3_BUTTON_ROOM, 154, 84, 57, 72, 27},
    {2, ZORK3_BUTTON_ROOM, 222, 181, 39, 25, 30},
    {2, 103, 25, 112, 59, 26, 16},
    {2, 103, 131, 100, 24, 26, 15},
    {2, ZORK3_DUNGEON_ENTRANCE, 80, 8, 34, 57, 44},
    {2, ZORK3_DUNGEON_ENTRANCE, 147, 176, 33, 42, 34},
    {2, ZORK3_CRYSTAL_GROTTO, 50, 10, 30, 67, 46},
    {2, ZORK3_CRYSTAL_GROTTO, 198, 140, 51, 34, 19},
    {2, ZORK3_DAMP_PASSAGE, 14, 8, 30, 48, 35},
    {2, ZORK3_DAMP_PASSAGE, 71, 66, 52, 108, 23},
    {2, ZORK3_DAMP_PASSAGE, 198, 72, 64, 96, 18},
    {2, 109, 14, 8, 24, 45, 30},
    {2, 109, 95, 15, 37, 48, 43},
    {2, 109, 193, 82, 28, 139, 25},
    {2, 109, 220, 113, 27, 31, 43},
    {2, 112, 14, 8, 30, 47, 32},
    {2, 112, 193, 34, 40, 173, 37},
    {2, ZORK3_LAKE, 154, 183, 57, 43, 31},
    {2, ZORK3_LAKE, 223, 9, 45, 206, 35},
    // Mark the projecting shelf edge above the path instead of a patch of wall.
    {2, 73, 31, 11, 17, 171, 27},
    {2, 5, 219, 8, 28, 18, 22},
    {2, 66, 219, 197, 31, 17, 21},
    {2, 176, 219, 2, 36, 17, 17},
    {2, 78, 14, 8, 25, 49, 33},
    {2, ZORK3_ROYAL_HALL, 38, 175, 20, 42, 36},
    {2, ZORK3_GREAT_DOOR, 89, 6, 18, 88, 57},
    {2, ZORK3_GREAT_DOOR, 199, 160, 24, 78, 57},
    {2, ZORK3_LAND_OF_SHADOW_69, 14, 94, 46, 62, 31},
    {2, ZORK3_LAND_OF_SHADOW_69, 193, 5, 20, 230, 54},
    {2, ZORK3_WEST_SHORE, 67, 146, 66, 34, 21},
    {2, ZORK3_WEST_SHORE, 223, 106, 45, 126, 32},
    {2, ZORK3_NARROW_GUARDIANS_EAST, 147, 13, 30, 55, 38},
    {2, ZORK3_NARROW_GUARDIANS_WEST, 207, 13, 30, 55, 38},
    {2, ZORK3_ENGRAVINGS_ROOM, 32, 41, 23, 62, 32},
    {2, ZORK3_ENGRAVINGS_ROOM, 154, 103, 36, 34, 44},
    {2, ZORK3_ENGRAVINGS_ROOM, 222, 178, 31, 32, 43},
    {2, 159, 207, 106, 30, 28, 35},
    {2, ZORK3_HIGH_ARCH, 71, 12, 57, 216, 24},
    {2, ZORK3_HIGH_ARCH, 156, 70, 38, 111, 39},
    {2, ZORK3_HIGH_ARCH, 198, 4, 31, 232, 29},
    {2, ZORK3_AQUEDUCT, 71, 17, 61, 206, 27},
    {2, ZORK3_AQUEDUCT, 156, 64, 31, 111, 40},
    {2, ZORK3_AQUEDUCT, 198, 8, 32, 224, 30},
    {2, ZORK3_AQUEDUCT_VIEW, 154, 5, 25, 48, 43},
    {2, ZORK3_WATER_SLIDE, 71, 18, 59, 204, 27},
    {2, ZORK3_WATER_SLIDE, 156, 69, 33, 105, 38},
    {2, ZORK3_WATER_SLIDE, 198, 11, 33, 218, 34},
    {2, ZORK3_CLIFF_LEDGE, 14, 8, 26, 54, 34},
    {2, ZORK3_CLIFF_LEDGE, 31, 101, 60, 42, 20},
    {2, ZORK3_CLIFF_LEDGE, 220, 145, 25, 63, 33},
    {2, ZORK3_CLIFF_LEDGE, 137, 160, 17, 18, 62},
    {2, ZORK3_ENDLESS_STAIR, 154, 50, 40, 120, 48},
    {2, ZORK3_SOUTH_SHORE, 154, 6, 36, 76, 51},
    {2, 159, 147, 14, 30, 74, 36},
    {2, 169, 29, 165, 25, 28, 42},
    {2, 169, 80, 165, 25, 28, 42},
    {2, 169, 96, 83, 74, 83, 17},
    {2, 169, 106, 165, 25, 28, 42},
    {2, 169, 107, 121, 68, 8, 23},
    // Jewel Room (present): the cage, its plaque, and the pedestal.
    {2, 16, 10, 55, 15, 145, 75},
    {2, 16, 131, 103, 20, 40, 14},
    {2, 16, 61, 110, 70, 27, 20},
    // Land of Shadow clifftop: the ruined north wall and the rubble crossing.
    {2, 170, 193, 140, 18, 80, 30},
    {2, 170, 14, 140, 36, 60, 18},
    // Museum Entrance: west iron door, open north wooden door, east stone door.
    {2, 173, 199, 30, 20, 52, 46},
    {2, 173, 82, 85, 15, 65, 50},
    {2, 173, 205, 185, 25, 40, 40},
    // Technology Museum: grey machine, black machine, open wooden door.
    {2, 5, 92, 25, 25, 60, 45},
    {2, 5, 132, 175, 25, 55, 50},
    {2, 5, 82, 85, 18, 65, 50},
    // Aqueduct View: the aqueduct arches across the east side.
    {2, 75, 156, 115, 15, 125, 70},
    {2, 179, 61, 104, 60, 31, 13},
    {2, 179, 79, 164, 54, 35, 26},
    {2, 179, 205, 200, 28, 34, 58},
    {2, 190, 209, 91, 28, 58, 33},
    {2, 216, 223, 128, 32, 109, 44},
    {2, ZORK3_LAKE, 94, 119, 63, 25, 12},
    {2, ZORK3_MINE_DEAD_END, 85, 119, 50, 22, 20},

    // Zork I canyon, waterfall, and rainbow landmarks.
    {0, 22, 98, 100, 25, 135, 33},
    {0, 22, 6, 16, 25, 220, 49},
    {0, 22, 226, 150, 60, 68, 28},
    {0, 22, 59, 68, 55, 45, 20},
    {0, 21, 98, 19, 49, 117, 29},
    {0, 21, 6, 16, 26, 220, 48},
    {0, 21, 226, 122, 66, 72, 22},
    {0, 74, 6, 2, 62, 105, 26},
    {0, 74, 98, 16, 20, 220, 49},
    {0, 74, 226, 16, 22, 220, 49},
    {0, 125, 6, 48, 59, 122, 16},
    {0, 125, 59, 95, 28, 110, 45},
    {0, 125, 226, 25, 52, 155, 36},
    {0, 47, 59, 16, 26, 220, 52},
    {0, 47, 226, 20, 32, 216, 40},
    {0, 202, 59, 16, 37, 218, 39},
    {0, 111, 6, 20, 67, 200, 20}, {0, 111, 226, 20, 67, 200, 20},
    {0, 37, 6, 20, 65, 200, 22}, {0, 37, 226, 20, 65, 200, 22},
    {0, 174, 6, 20, 65, 200, 22}, {0, 174, 226, 20, 65, 200, 22},
    {0, 45, 6, 20, 65, 200, 22}, {0, 45, 226, 20, 65, 200, 22},
    {0, 182, 6, 20, 65, 200, 22}, {0, 182, 226, 20, 65, 200, 22},
    {0, 30, 6, 2, 44, 100, 22}, {0, 30, 226, 2, 44, 100, 22},
    {0, 40, 6, 20, 67, 200, 20}, {0, 40, 226, 20, 67, 200, 20},
    {0, 47, 6, 20, 67, 200, 20},

    // Zork II bridge and stream landmarks.
    {1, 168, 74, 76, 48, 108, 20},
    {1, 168, 105, 10, 38, 220, 42},
    {1, 168, 104, 20, 66, 200, 17},
    {1, 168, 217, 6, 20, 228, 50},
    {1, 21, 74, 84, 47, 83, 18},
    {1, 21, 105, 8, 37, 224, 40},
    {1, 183, 217, 8, 17, 224, 59},
    {1, 53, 5, 19, 71, 186, 17},
    // In each mist room the player is inside the crystal sphere. Frame its
    // curved shell around the scene rather than an absent loose sphere sprite.
    {1, 224, 188, 4, 18, 232, 72},
    {1, 227, 188, 4, 18, 232, 72},

    // Zork III lake-side stair and water surface.
    {2, 102, 223, 62, 33, 127, 50},
    {2, 102, 154, 182, 29, 44, 28},

    // The room inside the mirror has several small, individually selectable props.
    {2, 160, 2, 18, 40, 14, 18},
    {2, 160, 37, 10, 22, 48, 58},
    {2, 160, 52, 3, 17, 10, 68},
    {2, 160, 62, 184, 22, 52, 58},
    {2, 160, 87, 121, 27, 58, 42},
    {2, 160, 96, 70, 74, 94, 17},
    {2, 160, 107, 113, 78, 14, 12}, // floor channel below the compass depression
    {2, 160, 127, 113, 70, 14, 14},
    {2, 160, 157, 20, 50, 200, 9},
    {2, 160, 175, 106, 72, 28, 11},
    {2, 160, 196, 114, 17, 12, 62},
    {2, 160, 197, 58, 27, 61, 42},
    {2, 160, 212, 228, 17, 10, 68},
    {2, 66, 79, 175, 44, 39, 31},
    {2, 66, 82, 216, 36, 22, 36},
    {2, 66, 92, 30, 39, 39, 29},
    {2, 66, 132, 175, 44, 39, 31},
    {2, 176, 79, 175, 44, 39, 31},
    {2, 176, 82, 100, 26, 41, 34},
    {2, 176, 92, 30, 39, 39, 29},
    {2, 176, 132, 175, 44, 39, 31},
    {2, 227, 79, 175, 44, 39, 31},
    {2, 227, 82, 100, 26, 41, 34},
    {2, 227, 89, 9, 34, 36, 28},
    {2, ZORK3_MUSEUM_ENTRANCE, 154, 89, 65, 62, 20},
    {2, ZORK3_MUSEUM_ENTRANCE_ALT, 154, 89, 65, 62, 20},
    {2, 227, 154, 89, 65, 62, 20},
    {2, 227, 199, 29, 27, 25, 35},
    {2, 227, 205, 181, 27, 25, 35},

    // Zork I house and forest landmarks.
    {0, 64, 121, 130, 50, 20, 28},
    {0, 64, 65, 80, 20, 145, 66},
    {0, 64, 77, 130, 52, 20, 24},
    {0, 64, 1, 8, 18, 85, 66},
    {0, 209, 213, 28, 20, 44, 42},
    {0, 209, 77, 30, 23, 40, 36},
    {0, 209, 65, 2, 17, 153, 70},
    {0, 209, 1, 157, 17, 81, 70},
    {0, 85, 65, 2, 17, 156, 70},
    {0, 85, 1, 162, 17, 76, 70},
    {0, 137, 213, 186, 31, 44, 48},
    {0, 137, 77, 188, 35, 40, 38},
    {0, 137, 65, 119, 24, 96, 60},
    {0, 137, 1, 7, 17, 90, 67},
    {0, 238, 107, 100, 22, 138, 45},
    {0, 238, 56, 106, 45, 18, 16},
    {0, 238, 65, 151, 28, 63, 47},
    {0, 238, 1, 6, 18, 226, 64},
    {0, 5, 107, 38, 38, 143, 43},
    {0, 5, 56, 112, 47, 18, 15},
    {0, 5, 65, 115, 27, 42, 31},
    {0, 5, 1, 7, 17, 226, 65},
    {0, 128, 107, 29, 23, 78, 59},
    {0, 128, 56, 132, 39, 15, 13},
    {0, 128, 65, 164, 29, 52, 39},
    {0, 128, 1, 6, 17, 92, 65},
    {0, 87, 107, 18, 24, 62, 57},
    {0, 87, 56, 119, 35, 16, 14},
    {0, 87, 65, 154, 27, 54, 41},
    {0, 87, 1, 6, 17, 95, 67},
    {0, 27, 169, 41, 59, 120, 20},
    {0, 27, 243, 178, 19, 46, 34},
    {0, 27, 43, 8, 18, 34, 39},
    // Kitchen's two stair treads sit at the far right edge and continue offscreen.
    {0, 27, 177, 214, 63, 24, 25},
    // The living-room rug is a separate target in both arrangements; its
    // position is chosen in draw_room_target_anchor from the trap-door state.
    {0, 75, 197, 181, 27, 41, 37},
    {0, 75, 177, 105, 60, 31, 21},
    {0, 187, 250, 104, 52, 124, 23},
    {0, 33, 177, 92, 22, 55, 41},
    {0, 78, 177, 172, 28, 66, 60},
    {0, 82, 177, 2, 48, 30, 40},
    {0, 31, 231, 88, 24, 59, 37},
    // The troll holds this axe in some states; its floor sprite is framed by
    // the ordinary item renderer once the story drops it into the room.
    {0, 127, 36, 100, 54, 16, 18},
    {0, 178, 9, 82, 51, 74, 31},
    {0, 178, 223, 64, 19, 112, 48},
    {0, 178, 6, 2, 28, 236, 30},
    {0, 95, 6, 25, 63, 190, 23},
    {0, 191, 6, 154, 58, 81, 22},
    {0, 244, 113, 34, 18, 200, 60},
    {0, 244, 190, 170, 22, 42, 56},
    {0, 89, 207, 171, 28, 50, 40},
    {0, 155, 131, 44, 66, 53, 15},

    // Zork I stairs, engravings, and temple fixtures.
    {0, 130, ZORK1_STAIRS, 91, 64, 58, 22},
    {0, 4, ZORK1_STAIRS, 188, 47, 48, 41},
    {0, 18, ZORK1_STAIRS, 6, 26, 44, 34},
    {0, 2, ZORK1_STAIRS, 190, 58, 48, 30},
    {0, 241, ZORK1_STAIRS, 3, 39, 34, 43},
    {0, 176, ZORK1_STAIRS, 174, 53, 48, 31},
    {0, 205, ZORK1_STAIRS, 195, 55, 40, 31},
    {0, 62, ZORK1_STAIRS, 172, 55, 49, 29},
    {0, 93, ZORK1_STAIRS, 88, 63, 62, 25},
    {0, 69, 84, 21, 38, 198, 34},
    {0, 2, 71, 173, 43, 47, 27},

    // Zork II garden scene landmarks, including the unicorn's carried items.
    {1, 234, 235, 170, 27, 49, 54},
    {1, 234, 217, 17, 24, 206, 26},
    {1, 234, 48, 22, 58, 76, 17},
    {1, 16, 178, 45, 47, 148, 29},
    {1, 16, 235, 137, 26, 89, 52},
    {1, 218, 120, 13, 18, 214, 57},
    {1, 218, 154, 168, 26, 40, 36},
    {1, 218, 217, 10, 18, 220, 57},
    {1, 218, 48, 12, 60, 79, 17},
    {1, 157, 217, 8, 19, 224, 48},
    {1, 157, 48, 23, 56, 72, 16},
    {1, 157, 154, 34, 40, 32, 29},
    {1, 234, 73, 158, 53, 14, 10},
    {1, 234, 127, 158, 55, 12, 10},

    // Zork II stream, well, pool, and room fixtures.
    {1, 212, 74, 16, 60, 207, 28},
    {1, 212, 105, 19, 27, 213, 56},
    {1, 212, 104, 22, 20, 34, 68},
    {1, 119, 9, 18, 22, 205, 46},
    {1, 119, 6, 50, 43, 139, 38},
    {1, 119, 186, 128, 50, 20, 8},
    {1, 8, 191, 10, 17, 160, 20},
    {1, 8, 6, 4, 34, 150, 56},
    {1, 190, 155, 28, 48, 184, 30},
    {1, 122, 195, 7, 18, 226, 62},
    {1, 122, 105, 73, 37, 91, 41},
    {1, 82, 39, 43, 62, 153, 23},
    {1, 82, 47, 113, 17, 20, 34},
    {1, 106, 11, 7, 18, 226, 69},
    {1, 30, 104, 92, 24, 58, 42},
    {1, 231, 5, 9, 67, 222, 17},
    {1, 231, 105, 7, 18, 226, 67},
    {1, 231, 104, 9, 67, 222, 17},
    {1, 124, 105, 2, 20, 50, 68},
    {1, 46, 137, 128, 18, 50, 70},
    {1, 46, 173, 143, 31, 20, 18},
    {1, 46, 105, 17, 18, 219, 66},
    {1, 46, 130, 130, 53, 10, 11},
    {1, 46, 112, 126, 50, 20, 19},
    {1, 59, 108, 18, 22, 57, 55},
    {1, 59, 77, 182, 33, 31, 21},
    {1, 59, 251, 39, 57, 157, 27},
    {1, 59, 137, 28, 20, 26, 58},
    {1, 59, 173, 17, 20, 37, 24},
    {1, 248, 74, 76, 45, 108, 24},
    {1, 80, 74, 60, 46, 130, 25},
    {1, 80, 105, 8, 20, 222, 61},
    {1, 172, 74, 45, 54, 145, 22},
    {1, 94, 76, 16, 50, 208, 33},
    {1, 94, 203, 177, 34, 48, 35},
    {1, 233, 250, 34, 16, 154, 60},
    {1, 182, 163, 195, 50, 17, 9},
    {1, 182, 132, 150, 30, 46, 38},
    {1, 182, 101, 143, 62, 71, 13},
    {1, 200, 20, 34, 18, 172, 60},
    {1, 200, 160, 72, 44, 97, 39},
    {1, 200, 50, 8, 19, 222, 62},
    {1, 64, 145, 166, 24, 55, 58}, {1, 64, 211, 16, 24, 55, 58},
    {1, 64, 50, 75, 17, 90, 25}, {1, 64, 199, 75, 61, 90, 24},
    {1, 2, 145, 166, 24, 55, 58}, {1, 2, 211, 16, 24, 55, 58},
    {1, 2, 50, 75, 17, 90, 25}, {1, 2, 199, 75, 61, 90, 24},
    {1, 102, 145, 166, 24, 55, 58}, {1, 102, 211, 16, 24, 55, 58},
    {1, 102, 50, 75, 17, 90, 25}, {1, 102, 199, 75, 61, 90, 24},
    {1, 91, 209, 48, 55, 145, 34},
    {1, 91, 166, 38, 31, 45, 48},
    {1, 91, 208, 98, 33, 52, 44},
    {1, 91, 222, 175, 31, 49, 30},
    {1, 240, 180, 157, 48, 77, 39},
    {1, 19, 180, 2, 20, 70, 68},
    {1, 19, 216, 165, 30, 51, 42},
    {1, 162, 180, 15, 30, 72, 58},
    {1, 194, 245, 102, 30, 50, 59},
    {1, 125, 116, 55, 34, 139, 33},
    {1, 125, 118, 18, 45, 204, 28},
    {1, 125, 245, 87, 22, 68, 52},

    // Zork III floor items and prison landmarks.
    {2, 45, 11, 80, 34, 32, 48},
    {2, 45, 206, 200, 26, 32, 30},
    {2, 45, 4, 202, 22, 30, 18},
    {2, 204, 4, 177, 52, 50, 34},


    // Zork II balloon hooks are fixed fittings at the cable endpoints.
    {1, 13, 88, 70, 34, 18, 15},
    {1, 143, 65, 58, 38, 9, 12},

    // The Crypt's sarcophagus, wall heads, and threshold.

    // Zork III's cloak and hood lie on the barren ground near the player.
    {2, 30, 59, 62, 40, 26, 46},
    {2, 30, 148, 68, 24, 14, 14},
};

enum {
    ZORK1_TOUCHED = 3,
    ZORK1_PICKUP_SCORE = 13,
    ZORK1_BOTTLE_WATER = 186,
    ZORK2_WORKROOM = 94,
    ZORK2_RUBY_SPHERE = 29,
    ZORK2_DIAMOND_SPHERE = 31,
    ZORK2_SAPPHIRE_SPHERE = 33,
    ZORK2_OBSIDIAN_SPHERE = 36,
    ZORK2_RUBY_STAND = 89,
    ZORK2_DIAMOND_STAND = 99,
    ZORK2_SAPPHIRE_STAND = 219,
    ZORK2_OBSIDIAN_STAND = 197,
    ZORK3_JUNCTION = 217,
    ZORK3_SWORD_ROCK = 146,
    ZORK3_SWORD = 202,
    ZORK3_TIME_MACHINE = 122,
};

typedef enum {
    ITEM_DEFAULT, ITEM_WHEN_LIT, ITEM_WHEN_OPEN, ITEM_WHEN_BOTTLE_FULL, ITEM_WHEN_SWORD_EMBEDDED,
    ITEM_WHEN_LIZARD_ASLEEP
} ItemState;

typedef struct {
    uint8_t chapter, object;
    const SceneSprite *art, *alternate;
    ItemState state;
} RoomItem;

#define ART(chapter, object, name) {chapter, object, &Art##name, NULL, ITEM_DEFAULT}
#define VARIANT(chapter, object, name, alternate, state) \
    {chapter, object, &Art##name, &Art##alternate, state}

static const RoomItem items[] = {
    ART(0, 10, NewPropsJadeIdol),
    ART(0, 12, TorchFlame),
    ART(0, 14, Lunch),
    ART(0, 17, MatchbookFolded),
    ART(0, 23, TridentFloor),
    ART(0, 24, NewPropsMap),
    VARIANT(0, 26, CandlesOff, CandlesOn, ITEM_WHEN_LIT),
    ART(0, 36, AxeFloor),
    ART(0, 41, BookWhiteClosed),
    ART(0, 58, RopeCoiled),
    ART(0, 60, CoalPile),
    ART(0, 68, BoatPunctured),
    ART(0, 76, LeafletFolded),
    ART(0, 80, NewPropsCanary),
    ART(0, 83, BellBrass),
    ART(0, 91, LanternOff),
    ART(0, 92, NewPropsPainting),
    ART(0, 96, RustyKeyIron),
    ART(0, 97, CoinsBag),
    ART(0, 99, SackTied),
    VARIANT(0, 105, EggJewelled, EggBrokenOpen, ITEM_WHEN_OPEN),
    ART(0, 108, BasketWicker),
    ART(0, 109, PumpHandheld),
    ART(0, 114, ScrewdriverFloor),
    ART(0, 115, EggBrokenOpen),
    ART(0, 119, NewPropsEmerald),
    ART(0, 126, NewPropsScarab),
    ART(0, 132, ShovelFloor),
    ART(0, 133, NewPropsBauble),
    ART(0, 135, NestWoven),
    ART(0, 136, NewPropsViscous),
    VARIANT(0, 138, BottleEmpty, BottleFull, ITEM_WHEN_BOTTLE_FULL),
    ART(0, 140, SkullCrystal),
    ART(0, 142, NewPropsSlag),
    ART(0, 144, PileBodiesPile),
    VARIANT(0, 146, LanternOff, LanternOn, ITEM_WHEN_LIT),
    ART(0, 149, NewPropsBrokenCanary),
    ART(0, 154, PotOfGoldFull),
    ART(0, 157, LeavesPile),
    ART(0, 163, BrokenLanternFloor),
    ART(0, 165, KnifeNastyFloor),
    ART(0, 168, BellHotRed),
    VARIANT(0, 179, CoffinClosed, CoffinOpen, ITEM_WHEN_OPEN),
    ART(0, 180, BoatLabelTan),
    ART(0, 181, Z3BrokenTimber),
    ART(0, 185, BuoyRed),
    ART(0, 186, NewPropsWater),
    ART(0, 192, NewPropsDiamond),
    ART(0, 201, NewPropsPlatinumBar),
    ART(0, 195, TubeFolded),
    ART(0, 196, RitualBookClosed),
    ART(0, 204, SceptreFloor),
    ART(0, 206, SkeletonSeated),
    ART(0, 208, BoatInflated),
    ART(0, 210, BoltMounted),
    ART(0, ZORK1_BRACELET, BraceletSapphire),
    ART(0, 215, BrochureDam),
    ART(0, 217, NewPropsGarlic),
    ART(0, 219, BoatFolded),
    ART(0, 227, SwordFloor),
    ART(0, 229, TrunkJewels),
    VARIANT(0, 230, MailboxClosed, MailboxOpen, ITEM_WHEN_OPEN),
    ART(0, 232, KnifeOld),
    ART(0, 236, WrenchSteel),
    ART(0, 237, ToolChestsCorroded),
    ART(0, 242, ChaliceSilver),
    ART(0, 246, NewPropsStiletto),

    ART(1, 3, StatuetteGold),
    ART(1, 14, RustyKeyIron),
    ART(1, 17, DeadSerpentFloor),
    VARIANT(1, 22, SteelBoxClosedRest, SteelBoxOpenRest, ITEM_WHEN_OPEN),
    ART(1, 23, MatchbookFolded),
    ART(1, 24, WandFloor),
    ART(1, ZORK2_RUBY_SPHERE, SphereRed),
    ART(1, ZORK2_DIAMOND_SPHERE, SphereClear),
    ART(1, ZORK2_SAPPHIRE_SPHERE, SphereBlue),
    ART(1, 35, CakeOrange),
    ART(1, ZORK2_OBSIDIAN_SPHERE, SphereBlack),
    VARIANT(1, 41, BookGreenClosed, BookOpenPages, ITEM_WHEN_OPEN),
    ART(1, 42, VialClear),
    ART(1, 55, LeafletFolded),
    ART(1, 56, PortraitFloorRest),
    VARIANT(1, 61, BookBlueClosed, BookOpenPages, ITEM_WHEN_OPEN),
    ART(1, 68, ClubWooden),
    ART(1, 72, CollarLeather),
    ART(1, 78, StampFlathead),
    VARIANT(1, 79, LizardHeadAwakeEmbedded, LizardHeadAsleepEmbedded, ITEM_WHEN_LIZARD_ASLEEP),
    VARIANT(1, 81, ChestClosed, ChestOpen, ITEM_WHEN_OPEN),
    ART(1, 83, CakeGreen),
    ART(1, ZORK2_RUBY_STAND, StandRuby),
    ART(1, ZORK2_DIAMOND_STAND, StandDiamond),
    ART(1, 97, RubyGem),
    ART(1, 100, PlaceMatWoven),
    ART(1, 103, CakeBlue),
    ART(1, 107, TeapotChina),
    ART(1, 110, RoseFresh),
    VARIANT(1, 115, SteelBoxClosedRest, SteelBoxOpenRest, ITEM_WHEN_OPEN),
    ART(1, 127, KeyGold),
    ART(1, 131, CandyPackage),
    ART(1, 134, NecklacePearl),
    ART(1, 138, BlueLabelFloor),
    ART(1, 140, ViolinStradivarius),
    ART(1, 146, CrownGaudy),
    ART(1, 147, CakeRed),
    VARIANT(1, 149, LanternOff, LanternOn, ITEM_WHEN_LIT),
    ART(1, 167, BrokenLanternFloor),
    ART(1, 179, BillsStack),
    ART(1, 181, NewspaperFolded),
    ART(1, 186, NewPropsWater),
    ART(1, ZORK2_OBSIDIAN_STAND, StandObsidian),
    VARIANT(1, 198, BookPurpleClosed, BookOpenPages, ITEM_WHEN_OPEN),
    ART(1, 213, FuseCoiled),
    ART(1, 214, RepellentCan),
    ART(1, ZORK2_SAPPHIRE_STAND, StandSapphire),
    ART(1, 220, LeafletFolded),
    ART(1, 223, SwordFloor),
    ART(1, 225, BrickClay),
    VARIANT(1, 232, BookWhiteClosed, BookOpenPages, ITEM_WHEN_OPEN),
    ART(1, 241, CageMangled),
    ART(1, 244, GreenNotePaper),
    ART(1, 246, CoinZorkmid),
    ART(1, 247, LetterOpenerSteel),
    ART(1, 215, NewPropsWater),

    ART(2, 18, TorchFlame),
    ART(2, 22, VialClear),
    ART(2, 13, StrangeKeyOld),
    ART(2, 59, CloakDiscarded),
    ART(2, 26, NewPropsValuables),
    VARIANT(2, 65, ChestClosed, ChestOpen, ITEM_WHEN_OPEN),
    ART(2, 77, BookWhiteClosed),
    ART(2, 83, BrokenStaff),
    ART(2, 90, Z3AmuletPendant),
    ART(2, 99, WaybreadPiece),
    ART(2, 111, Z3GoldenRingGold),
    ART(2, 129, Z3AmuletPendant),
    ART(2, ZORK3_TIME_MACHINE, MachineGold),
    VARIANT(2, 126, LanternOff, LanternOn, ITEM_WHEN_LIT),
    ART(2, 148, Z3HoodFloor),
    ART(2, 158, Z3BrokenTimber),
    ART(2, 145, BrokenLanternFloor),
    ART(2, 164, NewPropsWater),
    ART(2, 168, BurnedTorch),
    ART(2, 171, GreenNotePaper),
    ART(2, 174, KnifeJewelledFloor),
    ART(2, 180, SceptreFloor),
    ART(2, 185, OilLamp),
    VARIANT(2, ZORK3_SWORD_ROCK, SwordRockEmpty, SwordRockEmbedded, ITEM_WHEN_SWORD_EMBEDDED),
    ART(2, ZORK3_SWORD, SwordFloor),
    ART(2, 213, StaffFloor),
    ART(2, 195, RepellentCan),
};
#undef ART
#undef VARIANT

typedef struct {
    uint8_t chapter, room, object, parent;
    ScenePoint at;
    bool shadow;
    const SceneSprite *special_art;
} ItemPlacement;

// Coordinates are contact points in the room's visible crop. Parent keeps
// table, stand, or container placement separate from an item dropped nearby.
static const ItemPlacement item_placements[] = {
    {0, 64, 230, 64, {57, 80}, true, NULL},
    {0, 64, 76, 230, {54, 64}, false, &ArtLeafletInside},
    {0, 75, 24, 197, {181, 61}, false, NULL},
    {0, 27, 138, 169, {111, 65}, true, NULL},
    {0, 27, 99, 169, {140, 67}, true, NULL},
    {0, 5, 135, 5, {110, 78}, true, NULL},
    {0, 5, 105, 135, {110, 74}, false, NULL},
    {0, 5, 115, 135, {110, 74}, false, NULL},
    {0, 11, 58, 11, {113, 86}, false, &ArtRopeHanging},
    {0, 18, 12, 79, {123, 59}, false, NULL},
    {0, 30, 208, 30, {145, 85}, true, NULL},
    {0, 30, 132, 30, {185, 73}, true, NULL},
    {0, 37, 208, 37, {153, 86}, false, NULL},
    {0, 38, 83, 38, {86, 82}, true, NULL},
    {0, 38, 168, 38, {86, 82}, true, NULL},
    {0, 38, 26, 38, {105, 85}, true, NULL},
    {0, 38, 196, 38, {153, 82}, true, NULL},
    // The shared remains are small and distant through the open Hades gate,
    // then appear in the left corner once the player reaches the Land of the Dead.
    {0, 38, 144, 38, {93, 65}, false, &ArtPileBodiesDistant},
    {0, 13, 144, 13, {46, 76}, false, NULL},
    {0, 45, 208, 45, {123, 86}, false, NULL},
    {0, ZORK1_GAS_ROOM, ZORK1_BRACELET, ZORK1_GAS_ROOM, {146, 82}, true, NULL},
    {0, 53, 208, 53, {125, 86}, false, NULL},
    {0, 75, 146, 75, {169, 47}, true, NULL},
    {0, 75, 227, 75, {179, 30}, false, NULL},
    {0, 82, 242, 82, {120, 80}, true, NULL},
    // Place the painting over the pale portrait-shaped patch on the right side
    // of the Gallery's far wall.
    {0, 122, 92, 122, {193, 53}, false, NULL},
    {0, 95, 229, 95, {131, 82}, true, NULL},
    {0, 95, 208, 95, {167, 86}, false, NULL},
    {0, 111, 208, 111, {74, 85}, true, NULL},
    {0, 111, 68, 111, {74, 85}, true, NULL},
    {0, 111, 219, 111, {72, 83}, true, NULL},
    {0, 120, 215, 120, {83, 47}, false, NULL},
    {0, 120, 17, 120, {126, 47}, false, NULL},
    {0, 129, 26, 129, {93, 70}, false, NULL},
    {0, 129, 196, 35, {135, 70}, false, &ArtRitualBookOpen},
    {0, 156, 60, 156, {129, 84}, true, NULL},
    {0, 160, 157, 160, {120, 76}, true, NULL},
    {0, 216, 157, 216, {120, 84}, true, NULL},
    {0, 166, 208, 166, {163, 85}, true, NULL},
    {0, 174, 208, 174, {132, 87}, false, NULL},
    {0, 182, 208, 182, {147, 86}, false, NULL},
    {0, 182, 185, 182, {82, 72}, false, NULL},
    {0, 184, 208, 184, {168, 85}, true, NULL},
    {0, 187, 58, 187, {129, 73}, true, NULL},
    {0, 187, 165, 250, {171, 72}, true, NULL}, // knife lies on the attic table
    {0, 187, ZORK1_STAIRS, 48, {21, 86}, false, &ArtAtticRailingRail}, // stairs are global scenery
    {0, 191, 109, 191, {176, 84}, true, NULL},
    {0, 218, 206, 218, {96, 77}, true, NULL},
    {0, 218, 96, 218, {130, 77}, false, NULL},
    {0, 218, 97, 218, {143, 82}, true, NULL},
    {0, 218, 232, 218, {74, 83}, false, NULL},
    // The Studio's manual is loosely attached to the wall. Keep it fully on
    // the painted wall panel, clear of the doorway edge so it reads as affixed.
    {0, 220, 41, 220, {48, 50}, false, NULL},
    {0, 224, 236, 224, {115, 81}, false, NULL},
    {0, 224, 114, 224, {144, 83}, false, NULL},
    {0, 224, 195, 224, {163, 79}, false, NULL},
    {0, 224, 237, 224, {35, 75}, true, NULL},
    {0, 241, 179, 241, {120, 78}, true, NULL},
    {0, 245, 108, 245, {80, 73}, false, NULL},
    {0, 245, 12, 108, {84, 66}, false, NULL},
    {0, 245, 114, 108, {75, 63}, false, NULL},
    {0, 249, 108, 249, {119, 82}, true, NULL},
    {0, 249, 12, 108, {124, 74}, false, NULL},
    {0, 249, 114, 108, {113, 71}, false, NULL},
    {0, 178, 210, 178, {120, 70}, false, NULL},
    {0, 4, 23, 4, {57, 88}, true, NULL},
    {0, 125, 154, 125, {95, 82}, true, NULL},
    {0, 127, 36, 127, {97, 84}, true, NULL},

    {1, 51, 149, 51, {80, 82}, true, NULL},
    {1, 51, 223, 51, {146, 85}, true, NULL},
    {1, 59, 14, 108, {52, 49}, false, NULL},
    {1, 16, 127, 26, {158, 48}, false, NULL},
    {1, 64, 115, 133, {153, 82}, true, NULL},
    {1, 16, 100, 178, {63, 63}, false, &ArtPlaceMatTable},
    {1, 16, 247, 178, {55, 64}, false, &ArtLetterOpenerTable},
    {1, 16, 181, 178, {68, 61}, false, &ArtNewspaperTable},
    {1, 16, 23, 178, {38, 63}, false, &ArtMatchbookTable},
    {1, 16, 107, 178, {48, 61}, true, &ArtTeapotTable},
    {1, 28, 79, 28, {175, 59}, false, NULL},
    {1, 46, 100, 46, {160, 84}, false, NULL},
    {1, 46, 247, 130, {133, 55}, false, &ArtLetterOpenerInserted},
    {1, 49, 56, 49, {125, 59}, false, &ArtPortraitBanker},
    {1, 59, 33, 59, {143, 66}, false, NULL},
    {1, 136, 198, 136, {67, 82}, true, NULL},
    {1, 136, 232, 136, {95, 82}, true, NULL},
    {1, 136, 41, 136, {135, 84}, true, NULL},
    {1, 136, 61, 136, {192, 82}, true, NULL},
    {1, 182, 146, 132, {170, 59}, false, &ArtCrownSafe},
    {1, 182, 55, 132, {186, 59}, false, &ArtCardSafe},
    {1, 233, 17, 233, {105, 85}, true, NULL},
    {1, 54, 241, 54, {124, 84}, true, NULL},
    {1, 190, 83, 155, {81, 59}, true, NULL},
    {1, 190, 103, 155, {107, 61}, true, NULL},
    {1, 190, 35, 155, {134, 61}, true, NULL},
    {1, 190, 147, 155, {158, 59}, true, NULL},
    {1, ZORK2_WORKROOM, ZORK2_RUBY_STAND, 76, {78, 63}, true, NULL},
    {1, ZORK2_WORKROOM, ZORK2_SAPPHIRE_STAND, 76, {143, 63}, true, NULL},
    {1, ZORK2_WORKROOM, ZORK2_OBSIDIAN_STAND, 76, {116, 67}, true, NULL},
    {1, ZORK2_WORKROOM, ZORK2_DIAMOND_STAND, 76, {102, 77}, true, NULL},
    {1, 91, 24, 91, {148, 81}, true, NULL},
    {1, 201, 81, 201, {77, 82}, true, NULL},

    {2, 211, 126, 211, {138, 83}, true, NULL},
    {2, 218, 65, 218, {138, 82}, true, NULL},
    {2, 109, 65, 109, {101, 83}, true, NULL},
    {2, 109, 26, 139, {164, 83}, true, NULL},
    {2, 176, ZORK3_TIME_MACHINE, 176, {120, 86}, true, NULL},
    {2, 227, ZORK3_TIME_MACHINE, 227, {142, 84}, true, NULL},
    {2, 103, ZORK3_TIME_MACHINE, 103, {200, 86}, true, NULL},
    {2, ZORK3_JUNCTION, ZORK3_SWORD_ROCK, ZORK3_JUNCTION, {145, 86}, true, NULL},
    {2, 181, 22, 181, {174, 85}, true, NULL},
    {2, 109, 99, 109, {92, 78}, true, NULL},
    {2, 30, 59, 30, {111, 83}, true, NULL},
    {2, 12, 129, 12, {122, 82}, true, NULL},
    {2, 143, 171, 143, {214, 59}, false, NULL},
    {2, ZORK3_SCENIC_VISTA, 18, ZORK3_SCENIC_VISTA, {219, 61}, false, &ArtTorchFlame},
    {2, 103, 61, 103, {120, 72}, false, &ArtZ3Pedestal},
    {2, 109, 47, 109, {86, 78}, false, &ArtRopeHanging},
    {2, ZORK3_CLIFF_LEDGE, 137, 41, {169, 78}, false, &ArtRopeHanging},
    {2, 6, ZORK3_SWORD, 6, {120, 67}, false, NULL},
};

// One target box per frame. The first box drawn for the focused target wins:
// sprite and special-case boxes are drawn before the anchors[] table, so a
// table row never adds a second frame around something already framed.
// Redrawing the identical box (an item re-drawn on top) stays allowed.
static bool focus_box_drawn;
static int focus_box[4];
static unsigned current_focused;

// The focused sprite's actual drawn rect, reported independently of the gold
// box so the replay audit can prove the box surrounds the right thing.
static void report_focused(unsigned object, int x, int y, int width, int height) {
#ifdef VISUAL_QA
    extern void visual_qa_focused_visual(unsigned object, int kind, int x, int y, int width, int height);
    visual_qa_focused_visual(object, 0, x, y, width, height);
#else
    (void)object; (void)x; (void)y; (void)width; (void)height;
#endif
}

static void report_focused_feature(unsigned object, int x, int y, int width, int height) {
#ifdef VISUAL_QA
    extern void visual_qa_focused_visual(unsigned object, int kind, int x, int y, int width, int height);
    visual_qa_focused_visual(object, 1, x, y, width, height);
#else
    (void)object; (void)x; (void)y; (void)width; (void)height;
#endif
}

static void corners(unsigned object, int x, int y, int width, int height, int length) {
    if (object != current_focused) return;
    // A border around an edge-of-band sprite can spill above row 15 or below
    // row 90; keep the whole box inside the art band.
    if (y < 15) { height -= 15 - y; y = 15; }
    if (y + height > 91) height = 91 - y;
    if (x < 0) { width += x; x = 0; }
    if (x + width > SCREEN_W) width = SCREEN_W - x;
    if (width <= 0 || height <= 0) return;
#ifdef VISUAL_QA
    extern void visual_qa_marker(int x, int y, int width, int height);
    extern void visual_qa_focused_box(unsigned object, int x, int y, int width, int height);
    visual_qa_marker(x, y, width, height);
    if (!focus_box_drawn) visual_qa_focused_box(object, x, y, width, height);
#endif
    if (focus_box_drawn && (x != focus_box[0] || y != focus_box[1] ||
                            width != focus_box[2] || height != focus_box[3])) return;
    focus_box_drawn = true;
    focus_box[0] = x, focus_box[1] = y, focus_box[2] = width, focus_box[3] = height;
    fill(x, y, length, 1, GOLD);
    fill(x + width - length, y, length, 1, GOLD);
    fill(x, y + height - 1, length, 1, GOLD);
    fill(x + width - length, y + height - 1, length, 1, GOLD);
    fill(x, y, 1, length, GOLD);
    fill(x + width - 1, y, 1, length, GOLD);
    fill(x, y + height - length, 1, length, GOLD);
    fill(x + width - 1, y + height - length, 1, length, GOLD);
}

static void focus_feature_box(unsigned object, int x, int y, int width, int height, int length) {
    if (object != current_focused) return;
    report_focused_feature(object, x, y, width, height);
    corners(object, x, y, width, height, length);
}

void draw_target_focus_box(bool visible) {
    if (!visible) return;
    const int x = 2, y = 92, width = 236, height = 11, length = 4;
    // Scene-only concepts can be selectable without having a separate sprite
    // footprint. Frame the selected name row so every L/R target stays explicit.
    fill(x, y, length, 1, GOLD);
    fill(x + width - length, y, length, 1, GOLD);
    fill(x, y + height - 1, length, 1, GOLD);
    fill(x + width - length, y + height - 1, length, 1, GOLD);
    fill(x, y, 1, length, GOLD);
    fill(x + width - 1, y, 1, length, GOLD);
    fill(x, y + height - length, 1, length, GOLD);
    fill(x + width - 1, y + height - length, 1, length, GOLD);
}

static unsigned visual_frame, finale_age, finale_stage, animation_room;

bool room_visuals_animate(unsigned chapter, unsigned room, unsigned story_stage) {
    visual_frame++;
    bool changed = animation_room != room || finale_stage != story_stage;
    if (animation_room != room) { animation_room = room; finale_age = 0; finale_stage = story_stage; }
    if (finale_stage != story_stage) { finale_stage = story_stage; finale_age = 0; }
    bool finished_fade = room == 230 && story_stage == 1 && finale_age == 63;
    if (finale_age < 64) finale_age++;
    if (chapter != 1) return false;
    bool finale = room == 230 && (story_stage == 0 || (story_stage == 1 && finale_age < 64));
    // Room 106 is only entered when taking the sphere drops the cage. The
    // story starts the poison gas in that same turn; there is no gas-free
    // prompt inside the cage.
    bool cage_gas = room == 106;
    bool robot_winding = chapter == 1 && room == 87 && z_global(149) == 1;
    return changed || finished_fade ||
           (!(visual_frame & 3) && (finale || zork2_fuse_burning() || cage_gas || robot_winding));
}

static void draw_fuse_spark(int x, int y) {
    unsigned phase = (visual_frame >> 2) & 3;
    fill(x, y - 3, 2, 4, RGB(230, 124, 37));
    fill(x, y - 2, 1, 3, RGB(255, 225, 132));
    pixel(x + (phase & 1), y - 4 - (int)(phase >> 1), CREAM);
}

static bool in_scene(const Targets *targets, unsigned object) {
    for (unsigned i = 0; i < targets->count; i++) {
        if (targets->id[i] == object) return true;
    }
    return false;
}

extern const Color art_west[], art_north[], art_south[], art_rear[];
extern const Color art_kitchen[], art_living[], art_living_moved[], art_cellar[];
extern const Color art_barrow[], art_stairway[], art_cavern[], art_water[];
extern const Color art_hall[], art_forest[], art_troll_room[], art_riddle_room[];
extern const Color art_riddle_room_open[], art_mirror_room_v2[], art_canyon[];
extern const Color art_z2_riddle_room_238[], art_z2_riddle_room_238_open[];
extern const Color art_dam[], art_egyptian[], art_carousel[];
extern const Color art_tea[], art_lake_shore[], art_jewel_room[], art_treasure_room[];
extern const Color art_atlantis[], art_great_cavern[], art_workroom[];
extern const Color art_cliff_ledge_upper_shelf[];
extern const Color art_flathead_ocean[], art_cyclops_room[], art_aragain_falls[], art_end_of_rainbow[];
extern const Color art_pentagram_room[], art_menhir_room[], art_flathead_ship[], art_dragon_room[];
extern const Color art_robot_machine[], art_land_of_shadow[], art_dungeon_entrance[], art_aquarium[];
extern const Color art_aquarium_broken[];
extern const Color art_bank_vault[], art_cerberus_room[], art_hades[], art_engravings[], art_cliff_top[];
extern const Color art_bank_small_room[], art_bank_depository[], art_crypt_anteroom[], art_crypt_anteroom_open[];
extern const Color art_hades_inside[], art_cliff_base[];
extern const Color art_junction[], art_technology_museum[], art_museum_entrance[], art_museum_entrance_open[];
extern const Color art_z3_underwater[], art_z3_lake_surface[], art_z3_west_shore[], art_z3_south_shore[];
extern const Color art_z3_aqueduct[], art_z3_high_arch[], art_z3_water_slide[], art_z3_damp_passage[];
extern const Color art_z3_aqueduct_view[], art_z3_scenic_vista[];
extern const Color art_z3_technology_unfinished[], art_z3_jewel_unfinished[], art_z3_treasury[];
extern const Color art_z3_crystal_grotto[], art_z3_foggy_room[], art_z3_great_door[];
extern const Color art_z3_endless_stair[], art_z3_marble_hall[], art_z3_marble_hall_prison_open[];
extern const Color art_z3_prison_cell_open[];
extern const Color art_z3_prison_cell_closed[], art_z3_beam_room[], art_z3_button_room[];
extern const Color art_z3_timber_room[], art_z3_royal_puzzle_entrance[], art_z3_royal_hall[];
extern const Color art_z3_mirror_hall_closed[], art_z3_mirror_hall_open[];
extern const Color art_z3_narrow_mirror[], art_z3_parapet[];
extern const Color art_z3_narrow_mirror_guardians[], art_z3_narrow_mirror_guardians_north[];
extern const Color art_z3_ladder_top[], art_z3_ladder_bottom[], art_z3_mine_dead_end[], art_z3_side_room[];
extern const Color art_z3_narrow_mirror_west[], art_z3_narrow_mirror_guardians_west[];
extern const Color art_z3_narrow_mirror_guardians_north_west[];
extern const Color art_z3_barren_area[], art_z3_dead_end[], art_z3_east_corridor[];
extern const Color art_z3_guardian_hallway_v3[], art_z3_hairpin_loop[], art_z3_key_room[];
extern const Color art_z3_narrow_corridor[], art_z3_puzzle_room[], art_z3_south_corridor[];
extern const Color art_z3_tight_squeeze[], art_z3_west_corridor[];
extern const Color art_z3_creepy_crawl[], art_z3_prison_cell_bronze[], art_z3_shadow_cliff[];
extern const Color art_z3_land_of_shadow_69[];

typedef struct {
    const Color *background;
    ScenePoint left, right;
} RoomFloor;

#define FLOOR(name, lx, ly, rx, ry) {art_##name, {lx, ly}, {rx, ry}}
static const RoomFloor room_floors[] = {
#define ZORK2_SCENE(name, room, lx, ly, rx, ry) FLOOR(z2_##name, lx, ly, rx, ry),
#include "zork2_scenes.def"
#undef ZORK2_SCENE
    FLOOR(west, 63, 85, 194, 86), FLOOR(north, 85, 86, 164, 88),
    FLOOR(south, 104, 88, 184, 85), FLOOR(rear, 66, 88, 167, 85),
    FLOOR(kitchen, 44, 87, 190, 89), FLOOR(living, 88, 83, 148, 85),
    FLOOR(living_moved, 76, 83, 160, 85), FLOOR(cellar, 70, 84, 180, 86),
    FLOOR(barrow, 98, 84, 146, 85), FLOOR(stairway, 105, 86, 166, 84),
    FLOOR(cavern, 94, 86, 158, 84), FLOOR(water, 39, 85, 50, 86),
    FLOOR(hall, 82, 81, 155, 83), FLOOR(forest, 89, 86, 153, 86),
    FLOOR(troll_room, 73, 83, 168, 82), FLOOR(riddle_room, 74, 88, 159, 88),
    FLOOR(riddle_room_open, 74, 87, 159, 88),
    FLOOR(z2_riddle_room_238, 74, 88, 159, 88),
    FLOOR(z2_riddle_room_238_open, 74, 88, 159, 88),
    FLOOR(mirror_room_v2, 65, 84, 170, 84),
    FLOOR(canyon, 46, 86, 55, 86),
    FLOOR(dam, 56, 87, 180, 87), FLOOR(egyptian, 60, 87, 185, 88),
    FLOOR(carousel, 67, 83, 174, 84),
    FLOOR(tea, 49, 85, 194, 85), FLOOR(lake_shore, 144, 87, 191, 84),
    FLOOR(jewel_room, 55, 85, 176, 85), FLOOR(treasure_room, 98, 87, 157, 87),
    FLOOR(atlantis, 52, 87, 182, 89), FLOOR(great_cavern, 83, 86, 171, 81),
    FLOOR(workroom, 61, 87, 175, 87), FLOOR(cliff_ledge_upper_shelf, 78, 83, 158, 79),
    FLOOR(flathead_ocean, 167, 86, 185, 84), FLOOR(flathead_ship, 167, 86, 185, 84),
    FLOOR(cyclops_room, 91, 80, 157, 85), FLOOR(aragain_falls, 156, 86, 182, 87),
    FLOOR(end_of_rainbow, 62, 87, 103, 87), FLOOR(pentagram_room, 67, 84, 178, 82),
    FLOOR(menhir_room, 80, 84, 158, 83), FLOOR(dragon_room, 76, 85, 173, 84),
    FLOOR(robot_machine, 77, 84, 164, 81), FLOOR(land_of_shadow, 75, 84, 157, 85),
    FLOOR(dungeon_entrance, 76, 87, 150, 86), FLOOR(aquarium, 75, 86, 180, 81),
    FLOOR(aquarium_broken, 75, 86, 180, 81),
    FLOOR(bank_vault, 89, 83, 157, 85), FLOOR(cerberus_room, 98, 86, 158, 86),
    FLOOR(hades, 87, 83, 167, 84), FLOOR(engravings, 68, 81, 161, 83),
    FLOOR(cliff_top, 108, 84, 153, 85),
    FLOOR(bank_small_room, 89, 83, 158, 84), FLOOR(bank_depository, 65, 84, 176, 85),
    FLOOR(crypt_anteroom, 81, 83, 165, 84), FLOOR(crypt_anteroom_open, 81, 83, 165, 84),
    FLOOR(hades_inside, 85, 85, 159, 83), FLOOR(cliff_base, 79, 85, 154, 84),
    FLOOR(junction, 77, 85, 166, 86), FLOOR(technology_museum, 94, 84, 159, 86),
    FLOOR(museum_entrance, 84, 85, 155, 86), FLOOR(museum_entrance_open, 84, 85, 155, 86),
    FLOOR(z3_underwater, 53, 85, 185, 85), FLOOR(z3_lake_surface, 70, 85, 181, 85),
    FLOOR(z3_west_shore, 66, 85, 190, 84), FLOOR(z3_south_shore, 78, 86, 119, 86),
    FLOOR(z3_aqueduct, 62, 84, 180, 84), FLOOR(z3_high_arch, 73, 82, 169, 81),
    FLOOR(z3_water_slide, 82, 84, 164, 85),
    FLOOR(z3_damp_passage, 71, 84, 166, 84),
    FLOOR(z3_aqueduct_view, 78, 83, 168, 82), FLOOR(z3_scenic_vista, 83, 84, 172, 84),
    FLOOR(z3_technology_unfinished, 72, 84, 170, 84),
    FLOOR(z3_jewel_unfinished, 66, 85, 177, 85), FLOOR(z3_treasury, 66, 85, 178, 84),
    FLOOR(z3_crystal_grotto, 70, 85, 177, 84), FLOOR(z3_foggy_room, 70, 84, 178, 84),
    FLOOR(z3_great_door, 70, 84, 170, 84), FLOOR(z3_endless_stair, 82, 82, 157, 83),
    FLOOR(z3_marble_hall, 66, 84, 177, 84),
    FLOOR(z3_marble_hall_prison_open, 66, 84, 177, 84),
    FLOOR(z3_prison_cell_open, 70, 84, 174, 84),
    FLOOR(z3_prison_cell_closed, 72, 84, 172, 84), FLOOR(z3_beam_room, 78, 84, 162, 84),
    FLOOR(z3_button_room, 58, 84, 182, 84), FLOOR(z3_timber_room, 64, 84, 178, 84),
    FLOOR(z3_royal_puzzle_entrance, 60, 84, 180, 84), FLOOR(z3_royal_hall, 70, 84, 176, 84),
    FLOOR(z3_mirror_hall_open, 65, 84, 175, 84),
    FLOOR(z3_mirror_hall_closed, 65, 84, 175, 84),
    FLOOR(z3_narrow_mirror, 78, 84, 162, 84),
    FLOOR(z3_narrow_mirror_guardians, 78, 84, 162, 84),
    FLOOR(z3_narrow_mirror_guardians_north, 78, 84, 162, 84),
    FLOOR(z3_narrow_mirror_west, 78, 84, 162, 84),
    FLOOR(z3_narrow_mirror_guardians_west, 78, 84, 162, 84),
    FLOOR(z3_narrow_mirror_guardians_north_west, 78, 84, 162, 84),
    FLOOR(z3_ladder_top, 72, 84, 151, 85), FLOOR(z3_ladder_bottom, 69, 85, 173, 85),
    FLOOR(z3_mine_dead_end, 72, 85, 170, 85), FLOOR(z3_side_room, 58, 85, 188, 85),
    FLOOR(z3_parapet, 46, 84, 190, 84),
    FLOOR(z3_barren_area, 65, 85, 176, 85), FLOOR(z3_dead_end, 70, 85, 170, 85),
    FLOOR(z3_east_corridor, 72, 84, 168, 84), FLOOR(z3_west_corridor, 72, 84, 168, 84),
    FLOOR(z3_guardian_hallway_v3, 54, 84, 186, 84), FLOOR(z3_hairpin_loop, 70, 85, 170, 85),
    FLOOR(z3_key_room, 64, 84, 176, 84), FLOOR(z3_narrow_corridor, 75, 85, 165, 85),
    FLOOR(z3_puzzle_room, 72, 84, 168, 84), FLOOR(z3_south_corridor, 70, 85, 170, 85),
    FLOOR(z3_tight_squeeze, 72, 85, 168, 85),
    FLOOR(z3_creepy_crawl, 72, 86, 165, 86),
    FLOOR(z3_prison_cell_bronze, 72, 84, 172, 84), FLOOR(z3_shadow_cliff, 120, 85, 200, 85),
    FLOOR(z3_land_of_shadow_69, 68, 86, 173, 86),
    FLOOR(z1_rear_open, 66, 88, 167, 85), FLOOR(z1_kitchen_shut, 44, 87, 190, 88),
    FLOOR(z1_living_shortcut, 88, 83, 148, 85), FLOOR(z1_living_moved_shortcut, 76, 83, 160, 85),
    FLOOR(z1_hatch_open, 65, 84, 190, 84), FLOOR(z1_hatch_open_shortcut, 65, 84, 190, 84),
    FLOOR(z1_cellar_hatch, 69, 86, 173, 86), FLOOR(z1_cellar_hatch_open, 69, 86, 173, 86),
    FLOOR(z1_dam_open, 56, 87, 180, 87), FLOOR(z1_dam_low, 56, 87, 180, 87),
    FLOOR(z1_dam_refill, 56, 87, 180, 87), FLOOR(z1_dam_lobby, 50, 84, 180, 85),
    FLOOR(z1_maintenance, 84, 85, 153, 86), FLOOR(z1_dam_base, 60, 85, 95, 85),
    FLOOR(z1_reservoir_south, 89, 85, 172, 85), FLOOR(z1_reservoir_south_dry, 89, 85, 172, 85),
    FLOOR(z1_reservoir_north, 156, 85, 189, 85), FLOOR(z1_reservoir_drained, 79, 85, 166, 85),
    FLOOR(z1_sandy_beach, 111, 85, 179, 86), FLOOR(z1_white_cliffs, 154, 85, 195, 85),
    FLOOR(z1_shore, 153, 85, 207, 85), FLOOR(z1_stream_view, 157, 85, 207, 85),
    FLOOR(z1_stream, 183, 85, 214, 85),
    FLOOR(z1_damp_cave, 72, 85, 174, 85), FLOOR(z1_deep_canyon, 84, 87, 168, 87),
    FLOOR(z1_forest_path, 102, 85, 151, 85), FLOOR(z1_forest167, 70, 84, 170, 84),
    FLOOR(z1_studio, 75, 86, 168, 86),
    FLOOR(z1_tree_canopy, 68, 76, 159, 74), FLOOR(z1_attic, 59, 86, 188, 86),
    FLOOR(z1_maze_dead_end, 66, 87, 176, 87), FLOOR(z1_torch_room, 61, 85, 186, 85),
    FLOOR(z1_ladder_top, 53, 85, 102, 85), FLOOR(z1_ladder_bottom, 73, 87, 131, 86),
    FLOOR(z1_gas_room, 92, 85, 173, 86),
    FLOOR(z1_mirror_north, 72, 85, 175, 84), FLOOR(z1_mirror_south, 72, 85, 175, 84),
    FLOOR(z1_dome, 56, 88, 194, 88), FLOOR(z1_shaft, 140, 86, 199, 86),
    FLOOR(z1_shaft_bottom, 72, 86, 173, 84), FLOOR(z1_altar, 54, 86, 179, 85),
    FLOOR(z1_grate_clearing, 66, 84, 167, 85),
    FLOOR(z1_grate_clearing_open, 66, 84, 167, 85),
    FLOOR(z1_grate_clearing_leaves, 66, 84, 167, 85),
    FLOOR(z1_grate_cellar, 72, 85, 179, 85),
    FLOOR(z1_grate_cellar_open, 72, 85, 179, 85),
    FLOOR(z1_strange_passage, 84, 85, 162, 84), FLOOR(z1_barrow_outside, 78, 86, 180, 84),
    FLOOR(z1_temple, 64, 85, 158, 85), FLOOR(z1_round_room, 72, 85, 168, 85),
    FLOOR(z1_slide_room, 60, 85, 180, 85), FLOOR(z1_machine_room, 68, 84, 174, 84),
    FLOOR(z1_smelly_room, 66, 84, 174, 84), FLOOR(z1_gallery, 66, 84, 174, 84),
    FLOOR(z1_mine_dead_end, 72, 85, 170, 85), FLOOR(z1_squeaky_room, 72, 85, 168, 85),
    FLOOR(z1_rainbow_top, 68, 84, 172, 84), FLOOR(z1_loud_room, 68, 85, 172, 85),
    FLOOR(z1_bat_room, 67, 87, 157, 87), FLOOR(z1_canyon_bottom, 62, 87, 180, 87),
    FLOOR(z1_chasm_east, 153, 86, 202, 86), FLOOR(z1_chasm_south, 147, 86, 194, 86),
    FLOOR(z1_coal_mine, 72, 87, 168, 87), FLOOR(z1_cold_passage, 64, 87, 171, 87),
    FLOOR(z1_east_west_passage, 78, 86, 161, 86), FLOOR(z1_engraving_cave, 60, 87, 174, 87),
    FLOOR(z1_maze, 66, 87, 176, 87), FLOOR(z1_mine_entrance, 91, 87, 182, 87),
    FLOOR(z1_narrow_passage, 88, 87, 153, 87), FLOOR(z1_north_south_passage, 91, 87, 152, 87),
    FLOOR(z1_rocky_ledge, 49, 86, 99, 86), FLOOR(z1_sandy_cave, 56, 88, 188, 88),
    FLOOR(z1_tiny_cave, 63, 87, 164, 87),
};
#undef FLOOR

static const RoomFloor *room_floor(const Color *background) {
    for (unsigned i = 0; i < sizeof(room_floors) / sizeof(room_floors[0]); i++) {
        if (room_floors[i].background == background) return &room_floors[i];
    }
    return NULL;
}

static ScenePoint encounter_position(const Encounter *figure, unsigned room, const Color *background) {
    if (figure->chapter == 1 && figure->icon == PRINCESS && room == ZORK2_DRAGONS_LAIR && z_global(82))
        return (ScenePoint){180, 85};
    if (figure->chapter == 2 && figure->icon == DUNGEON_MASTER && room == ZORK3_PARAPET)
        return (ScenePoint){54, 89};
    if (figure->home_room == room) return figure->at;
    if (figure->chapter == 1 && room == 234) {
        if (figure->icon == UNICORN) return (ScenePoint){161, 55};
    }
    if (figure->chapter == 1 && room == 16) {
        if (figure->icon == PRINCESS) return (ScenePoint){145, 85};
    }
    const RoomFloor *floor = room_floor(background);
    if (floor && floor->left.y) return figure->at.x < SCREEN_W / 2 ? floor->left : floor->right;
    return figure->at;
}

static void draw_encounter(const Encounter *figure, unsigned room, const Color *background, unsigned focused) {
    const SceneSprite *pose = encounter_pose(figure, room);
    ScenePoint at = encounter_position(figure, room, background);
    int baseline = at.y;
    switch (figure->motion) {
        case ENCOUNTER_FLOATING:
            baseline -= figure->hover;
            break;
        case ENCOUNTER_HANGING:
            // The hanging pose's anchor is the ceiling attachment point.
            baseline += pose->height;
            break;
        case ENCOUNTER_GROUNDED:
        case ENCOUNTER_SWIMMING:
            break;
    }
    if (figure->motion == ENCOUNTER_FLOATING && figure->icon == WIZARD) {
        if (baseline - pose->height < 17) baseline = pose->height + 17;
    }
    int x = at.x - pose->width / 2, y = baseline - pose->height;
    // Keep the whole sprite inside the visible art band so its focus box is
    // always on screen (Zork II's garden unicorn otherwise frames above row 15).
    if (y < 15) y = 15;
    if (y + pose->height > 90) y = 90 - pose->height;
    int feet = y + pose->height; // follow the clamped sprite, not the raw anchor
    if (figure->floor_shadow) {
        ground_shadow(x + pose->width / 8, feet - 3, pose->width * 3 / 4);
        if ((figure->chapter == 2 && figure->icon == OLD_MAN && room == ZORK3_ENGRAVINGS_ROOM) ||
            (figure->chapter == 2 && figure->icon == DUNGEON_MASTER && room == ZORK3_PARAPET))
            ground_shadow(x + pose->width / 8, feet - 3, pose->width * 3 / 4);
    }
    bool mirrored = (figure->facing == FACE_LEFT && at.x < 111) ||
                    (figure->facing == FACE_RIGHT && at.x > 129);
    draw_masked_pixels(x, y, pose->width, pose->height, pose->pixels, mirrored);
    if (focused == figure->object) {
        report_focused(figure->object, x, y, pose->width, pose->height);
        corners(figure->object, x - 2, y - 2, pose->width + 4, pose->height + 4, 4);
    }
}

typedef struct {
    int x, y, width, height;
} ItemBounds;

static const SceneSprite *item_pose(unsigned chapter, unsigned object) {
    for (unsigned i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        const RoomItem *item = &items[i];
        if (item->chapter != chapter || item->object != object) continue;
        if (item->state == ITEM_WHEN_LIT && z_attr(object, ATTR_LIT)) return item->alternate;
        if (item->state == ITEM_WHEN_OPEN && z_attr(object, ATTR_OPEN)) return item->alternate;
        if (item->state == ITEM_WHEN_BOTTLE_FULL && z_parent(ZORK1_BOTTLE_WATER) == object) return item->alternate;
        if (item->state == ITEM_WHEN_SWORD_EMBEDDED && z_parent(ZORK3_SWORD) == object) return item->alternate;
        if (item->state == ITEM_WHEN_LIZARD_ASLEEP && z_global(142)) return item->alternate;
        return item->art;
    }
    return NULL;
}

static ItemBounds item_bounds(const SceneSprite *art, ScenePoint at) {
    return (ItemBounds){at.x - art->width / 2, at.y - art->height, art->width, art->height};
}

static void draw_room_item(unsigned focused_object, const SceneSprite *art,
                           ItemBounds bounds, bool shadow, bool selected) {
    int x = bounds.x, y = bounds.y, width = bounds.width, height = bounds.height;
    if (shadow && width >= 10) ground_shadow(x + width / 8, y + height - 3, width * 3 / 4);
    draw_masked_pixels(x, y, width, height, art->pixels, false);
    if (selected) {
        report_focused(focused_object, x, y, width, height);
        corners(focused_object, x - 1, y - 1, width + 2, height + 2, 3);
    }
}

static void draw_boat_cargo(unsigned focused_object, const SceneSprite *art, ScenePoint at, bool selected) {
    unsigned largest = art->width > art->height ? art->width : art->height;
    unsigned shift = largest > 28 ? 2 : largest > 14;
    unsigned step = 1u << shift;
    int width = (art->width + step - 1) >> shift, height = (art->height + step - 1) >> shift;
    int x = at.x - width / 2, y = at.y - height;
    for (int row = 0; row < height; row++) {
        const Color *source = art->pixels + row * step * art->width;
        for (int col = 0; col < width; col++) {
            Color sample = source[col * step];
            if (sample & 0x8000) pixel(x + col, y + row, sample & 0x7fff);
        }
    }
    if (selected) {
        report_focused(focused_object, x, y, width, height);
        corners(focused_object, x - 1, y - 1, width + 2, height + 2, 2);
    }
}

static void draw_contents_sprite(unsigned object, const SceneSprite *art, ScenePoint at,
                                unsigned max_width, unsigned max_height, bool selected);

static void draw_zork1_item_contents(unsigned object, ItemBounds bounds,
                                     const Targets *targets, unsigned focused) {
    int center = bounds.x + bounds.width / 2, baseline = bounds.y + bounds.height;
    if (object == 179 && z_parent(204) == object && in_scene(targets, 204)) {
        ScenePoint at = {center - 2, baseline - 9};
        draw_room_item(204, &ArtSceptreInside, item_bounds(&ArtSceptreInside, at), false, focused == 204);
    }
    if (object == 138 && z_parent(ZORK1_BOTTLE_WATER) == object && focused == ZORK1_BOTTLE_WATER)
        focus_feature_box(focused, bounds.x - 1, bounds.y - 1, bounds.width + 2, bounds.height + 2, 2);

    if (object == 99 || object == 105 || object == 115 || object == 185 || object == 195) {
        for (unsigned i = 0; i < targets->count; i++) {
            unsigned child = targets->id[i];
            if (z_parent(child) != object) continue;
            const SceneSprite *art = item_pose(0, child);
            if (!art) continue;
            if (object == 99 && child == 217 && z_attr(object, ATTR_OPEN)) {
                // Once opened, the clove is visible on the tabletop beside the sack.
                // Keep its native sprite size so the pre-pickup scene reads clearly.
                ScenePoint at = {center - 16, baseline};
                draw_room_item(child, art, item_bounds(art, at), false, child == focused);
                continue;
            }
            ScenePoint at = {center, baseline - 4};
            unsigned max_width = 9, max_height = 7;
            if (object == 99) {
                at.x += child == 217 ? -5 : 5;
                at.y -= 2;
                max_width = 8;
                max_height = 5;
            } else if (object == 105 || object == 115) {
                at.y -= 3;
                max_width = 9;
                max_height = 7;
            } else if (object == 185) {
                at.y -= 2;
                max_width = 8;
                max_height = 8;
            } else {
                at.y -= 1;
                max_width = 9;
                max_height = 5;
            }
            draw_contents_sprite(child, art, at, max_width, max_height, child == focused);
        }
    }
    if (object != 208) return;

    // These seats follow the hull, so cargo moves with the boat down the river.
    static const int8_t cargo_x[] = {-10, 0, 9, 3};
    static const int8_t cargo_y[] = {-13, -11, -9, -6};
    unsigned drawn = 0;
    for (unsigned i = 0; i < targets->count && drawn < sizeof(cargo_x); i++) {
        unsigned child = targets->id[i];
        if (z_parent(child) != object) continue;
        const SceneSprite *art = child == 180 ? &ArtBoatLabelTan : item_pose(0, child);
        if (!art) continue;
        ScenePoint at = {center + cargo_x[drawn], baseline + cargo_y[drawn]};
        draw_boat_cargo(child, art, at, child == focused);
        drawn++;
    }
}

static void draw_contents_sprite(unsigned object, const SceneSprite *art, ScenePoint at,
                                unsigned max_width, unsigned max_height, bool selected) {
    unsigned step = 1, shift = 0;
    while (((art->width + step - 1) >> shift) > max_width ||
           ((art->height + step - 1) >> shift) > max_height) { step <<= 1; shift++; }
    int width = (art->width + step - 1) >> shift, height = (art->height + step - 1) >> shift;
    int x = at.x - width / 2, y = at.y - height;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            Color sample = art->pixels[row * step * art->width + col * step];
            if (sample & 0x8000) pixel(x + col, y + row, sample & 0x7fff);
        }
    }
    if (selected) {
        report_focused(object, x, y, width, height);
        corners(object, x - 1, y - 1, width + 2, height + 2, 2);
    }
}

static ItemBounds contents_bounds(const SceneSprite *art, ScenePoint at,
                                  unsigned max_width, unsigned max_height) {
    unsigned step = 1, shift = 0;
    while (((art->width + step - 1) >> shift) > max_width ||
           ((art->height + step - 1) >> shift) > max_height) { step <<= 1; shift++; }
    int width = (art->width + step - 1) >> shift, height = (art->height + step - 1) >> shift;
    return (ItemBounds){at.x - width / 2, at.y - height, width, height};
}

static void draw_contents_marker(unsigned object, const SceneSprite *art, ScenePoint at,
                                 unsigned max_width, unsigned max_height) {
    ItemBounds bounds = contents_bounds(art, at, max_width, max_height);
    report_focused(object, bounds.x, bounds.y, bounds.width, bounds.height);
    corners(object, bounds.x - 1, bounds.y - 1, bounds.width + 2, bounds.height + 2, 2);
}

static void draw_zork2_item_contents(unsigned object, ItemBounds bounds,
                                    const Targets *targets, unsigned focused) {
    if (object == 225 && z_parent(213) == 225) {
        int x = bounds.x + bounds.width - 3, y = bounds.y + bounds.height / 2;
        fill(x - 3, y, 4, 1, SHADOW);
        if (zork2_fuse_burning()) draw_fuse_spark(x, y);
        if (focused == 213) focus_feature_box(213, x - 4, y - 6, 8, 10, 2);
        return;
    }
    if (object != 22 && object != 81 && object != 198) return;
    int center = bounds.x + bounds.width / 2, baseline = bounds.y + bounds.height;
    unsigned slot = 0, capacity = object == 198 ? 1 : 4;
    for (unsigned pass = 0; pass < 2; pass++) {
      for (unsigned i = 0; i < targets->count && slot < capacity; i++) {
        unsigned child = targets->id[i];
        if (((child == focused) != (pass == 0)) || z_parent(child) != object) continue;
        const SceneSprite *art = item_pose(1, child);
        if (!art) continue;
        ScenePoint at = {center, baseline - (object == 198 ? 3 : 7)};
        if (object != 198) {
            at.x += (slot & 1) ? 6 : -6;
            at.y -= (slot < 2) ? 5 : 0;
        }
        draw_contents_sprite(child, art, at, object == 198 ? 12 : 10, object == 198 ? 8 : 5,
                             child == focused);
        slot++;
      }
    }
}

static void draw_zork3_item_contents(unsigned object, ItemBounds bounds,
                                    const Targets *targets, unsigned focused) {
    // The seat, dial and button are drawn as part of the time machine sprite,
    // so frame them wherever the machine itself was placed.
    if (object == ZORK3_TIME_MACHINE && in_scene(targets, focused)) {
        if (focused == 25 || focused == 121)
            focus_feature_box(focused, bounds.x + 26, bounds.y, 26, 14, 3);
        else if (focused == 76)
            focus_feature_box(focused, bounds.x + 9, bounds.y + 4, 8, 8, 3);
        else if (focused == 120)
            focus_feature_box(focused, bounds.x + 4, bounds.y + 4, 7, 7, 3);
        return;
    }
    if (object != 61) return;
    int center = bounds.x + bounds.width / 2;
    int baseline = bounds.y + bounds.height;
    fill(center - 9, baseline - 4, 18, 4, RGB(76, 75, 69));
    fill(center - 7, baseline - 13, 14, 9, RGB(126, 126, 118));
    fill(center - 10, baseline - 15, 20, 2, RGB(182, 171, 142));
    fill(center - 8, baseline - 14, 16, 1, RGB(211, 195, 159));
}

static void draw_z3_jewel_display(unsigned room, const Color *background,
                                  const Targets *targets, unsigned focused) {
    if ((room != ZORK3_JEWEL_947 && room != ZORK3_JEWEL_PRESENT && room != ZORK3_JEWEL_PAST) ||
        !in_scene(targets, 61)) return;

    static const unsigned jewels[] = {111, 174, 180};
    static const int8_t shifts[] = {-10, 0, 10};
    int baseline = room == ZORK3_JEWEL_PRESENT ? 57 :
                   room == ZORK3_JEWEL_PAST ? 66 : 70;
    bool focused_drawn = false;
    for (unsigned i = 0; i < sizeof(jewels) / sizeof(jewels[0]); i++) {
        unsigned object = jewels[i];
        if (!in_scene(targets, object) || z_parent(object) != 61) continue;
        const SceneSprite *art = item_pose(2, object);
        if (!art) continue;
        draw_contents_sprite(object, art, (ScenePoint){120 + shifts[i], baseline}, 13, 9, false);
        if (object == focused) focused_drawn = true;
    }

    // Restore the three foreground cage bars over the jewels. The bars are part
    // of the background art; copying their original pixels keeps the props behind
    // the cage instead of making them appear to float in front of it.
    if ((room == ZORK3_JEWEL_947 || room == ZORK3_JEWEL_PRESENT) && background == art_jewel_room) {
        static const uint8_t bars[] = {112, 113, 114, 122, 123, 124, 132, 133, 134};
        for (int row = baseline - 10; row <= baseline; row++) {
            unsigned source_row = (unsigned)(row - 5);
            for (unsigned i = 0; i < sizeof(bars); i++) {
                unsigned x = bars[i];
                Color sample = background[source_row * SCREEN_W + x];
                if (sample & 0x8000) pixel(x, row, sample & 0x7fff);
            }
        }
    }

    // Selection corners sit above the cage bars so the focused jewel stays easy
    // to identify while preserving the physical depth of the scene.
    if (focused_drawn) {
        unsigned index = 0;
        while (index < sizeof(jewels) / sizeof(jewels[0]) && jewels[index] != focused) index++;
        if (index < sizeof(jewels) / sizeof(jewels[0])) {
            const SceneSprite *art = item_pose(2, focused);
            if (art) draw_contents_marker(focused, art, (ScenePoint){120 + shifts[index], baseline}, 13, 9);
        }
    }
}

static void draw_z3_beam(unsigned room, const Targets *targets) {
    if (room != ZORK3_BEAM_ROOM && room != ZORK3_BUTTON_ROOM) return;
    bool present = room == ZORK3_BUTTON_ROOM || in_scene(targets, 27);
    if (!present) return;
    bool interrupted = z_parent(ZORK3_SWORD) == ZORK3_BEAM_ROOM;
    int y = room == ZORK3_BEAM_ROOM ? 59 : 53;
    int left = room == ZORK3_BEAM_ROOM ? 103 : 109;
    int right = room == ZORK3_BEAM_ROOM ? 137 : 131;
    if (interrupted && room == ZORK3_BEAM_ROOM) {
        scene_segment((ScenePoint){left, y}, (ScenePoint){113, y}, RGB(255, 115, 83));
        scene_segment((ScenePoint){127, y}, (ScenePoint){right, y}, RGB(255, 115, 83));
        fill(left - 1, y - 1, 3, 3, RGB(239, 72, 62));
        fill(right - 1, y - 1, 3, 3, RGB(239, 72, 62));
    } else if (!interrupted) {
        scene_segment((ScenePoint){left, y}, (ScenePoint){right, y}, RGB(255, 88, 67));
        scene_segment((ScenePoint){left, y + 1}, (ScenePoint){right, y + 1}, RGB(217, 55, 48));
        fill(left - 1, y - 1, 3, 4, RGB(242, 89, 66));
        fill(right - 1, y - 1, 3, 4, RGB(242, 89, 66));
    }
}

static void draw_sundial_digit(unsigned digit, int x, int y) {
    static const uint8_t rows[8][5] = {
        {2, 6, 2, 2, 7}, {6, 1, 2, 4, 7}, {6, 1, 2, 1, 6}, {5, 5, 7, 1, 1},
        {7, 4, 6, 1, 6}, {3, 4, 6, 5, 2}, {7, 1, 2, 2, 2}, {2, 5, 2, 5, 2}
    };
    if (digit < 1 || digit > 8) return;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (!(rows[digit - 1][row] & (1u << (2 - col)))) continue;
            // The dark lower edge and warm upper glint make each mark read as cut into stone.
            pixel(x + col + 1, y + row + 1, RGB(75, 48, 27));
            pixel(x + col, y + row, row == 0 ? RGB(179, 129, 69) : RGB(93, 58, 31));
        }
    }
}

static void draw_z3_parapet(void) {
    static const uint8_t half_width[] = {
        54, 53, 53, 52, 51, 50, 49, 47, 46, 44, 42,
        40, 37, 34, 30, 26, 22, 17, 12, 6, 0
    };
    // Repaint the whole dial face as one pixel-art object so its numerals and grooves
    // share the same elliptical perspective, lighting, and stone palette.
    int cx = 120, cy = 68;
    for (int row = -20; row <= 20; row++) {
        unsigned depth = (unsigned)(row < 0 ? -row : row);
        int width = half_width[depth];
        int y = cy + row;
        Color face = row < -10 ? RGB(184, 128, 73) :
                     row < 0 ? RGB(177, 116, 61) :
                     row < 10 ? RGB(162, 99, 48) : RGB(145, 83, 40);
        fill(cx - width, y, width * 2 + 1, 1, RGB(75, 47, 27));
        if (width > 2) fill(cx - width + 2, y, (width - 2) * 2 + 1, 1, face);
        if (width > 7 && row < -13) fill(cx - width + 4, y, (width - 4) * 2 + 1, 1,
                                        RGB(199, 147, 88));
    }

    // Eight recessed dividers sit between the eight numbered sectors.
    static const int8_t edge_x[] = {20, 45, 45, 20, -20, -45, -45, -20};
    static const int8_t edge_y[] = {-16, -7, 7, 16, 16, 7, -7, -16};
    for (unsigned i = 0; i < 8; i++) {
        ScenePoint edge = {cx + edge_x[i], cy + edge_y[i]};
        scene_segment((ScenePoint){cx, cy + 1}, (ScenePoint){edge.x, edge.y + 1}, RGB(92, 57, 31));
        scene_segment((ScenePoint){cx - 1, cy}, (ScenePoint){edge.x - 1, edge.y}, RGB(194, 137, 76));
    }

    // Numerals sit at sector centers, not on top of the divider lines.
    static const int8_t dx[] = {0, 23, 32, 23, 0, -23, -32, -23};
    static const int8_t dy[] = {-13, -9, 0, 9, 13, 9, 0, -9};
    for (unsigned i = 0; i < 8; i++)
        draw_sundial_digit(i + 1, cx + dx[i] - 1, cy + dy[i] - 2);

    unsigned selected = visual_state_parapet_dial();
    if (selected < 1 || selected > 8) selected = 1;
    unsigned direction = selected - 1;
    ScenePoint start = {cx + dx[direction] / 8, cy + dy[direction] / 8};
    ScenePoint tip = {cx + dx[direction] * 3 / 4, cy + dy[direction] * 3 / 4};
    scene_segment((ScenePoint){start.x + 1, start.y + 1}, (ScenePoint){tip.x + 1, tip.y + 1},
                  RGB(77, 46, 24));
    scene_segment(start, tip, GOLD);
    scene_segment((ScenePoint){tip.x, tip.y}, (ScenePoint){tip.x - dx[direction] / 8 - dy[direction] / 8,
                  tip.y - dy[direction] / 8 + dx[direction] / 8}, RGB(250, 210, 123));
    scene_segment((ScenePoint){tip.x, tip.y}, (ScenePoint){tip.x - dx[direction] / 8 + dy[direction] / 8,
                  tip.y - dy[direction] / 8 - dx[direction] / 8}, RGB(250, 210, 123));

    // Raised central button covers the pointer's pivot and catches the firelight.
    fill(cx - 5, cy - 2, 11, 5, RGB(80, 49, 27));
    fill(cx - 4, cy - 2, 9, 3, RGB(164, 102, 44));
    fill(cx - 3, cy - 3, 7, 1, RGB(218, 161, 82));
    fill(cx - 2, cy - 2, 5, 1, RGB(239, 184, 98));
    fill(cx - 3, cy + 1, 7, 1, RGB(125, 73, 33));
}

static void draw_z3_mirror_cues(void) {
    static const int8_t dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int8_t dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    unsigned direction = visual_state_mirror_arrow();
    int cx = 120, cy = 77;
    ScenePoint tip = {cx + dx[direction] * 8, cy + dy[direction] * 8};
    scene_segment((ScenePoint){cx, cy}, tip, GOLD);
    scene_segment((ScenePoint){tip.x, tip.y}, (ScenePoint){tip.x - dx[direction] - dy[direction],
                  tip.y - dy[direction] + dx[direction]}, GOLD);
    scene_segment((ScenePoint){tip.x, tip.y}, (ScenePoint){tip.x - dx[direction] + dy[direction],
                  tip.y - dy[direction] - dx[direction]}, GOLD);

    // The short pole's hand grip rises above the rail when it is lifted.
    int pole_top = visual_state_mirror_pole_raised() ? 28 : 41;
    int pole_bottom = 55;
    fill(24, pole_top, 2, pole_bottom - pole_top, RGB(149, 98, 56));
    fill(23, pole_top, 4, 2, RGB(207, 161, 103));
    if (visual_state_mirror_pine_open()) {
        fill(178, 35, 18, 39, RGB(20, 25, 24));
        fill(176, 34, 2, 41, RGB(164, 116, 70));
        scene_segment((ScenePoint){179, 73}, (ScenePoint){194, 68}, RGB(101, 71, 48));
        scene_segment((ScenePoint){194, 68}, (ScenePoint){194, 38}, RGB(101, 71, 48));
    }
}

static void draw_z3_secret_door(void) {
    if (!z_attr(222, ATTR_OPEN)) return;
    fill(103, 36, 34, 44, RGB(17, 23, 24));
    box(103, 36, 34, 44, RGB(17, 23, 24), RGB(119, 104, 83));
    for (int step = 0; step < 6; step++) {
        int y = 72 - step * 5, inset = step * 2;
        fill(106 + inset, y, 28 - inset * 2, 1, RGB(125 + step * 8, 110 + step * 7, 87 + step * 5));
    }
}

static unsigned stand_socket_height(unsigned object) {
    if (object == ZORK2_OBSIDIAN_STAND) return 10;
    if (object == ZORK2_RUBY_STAND || object == ZORK2_DIAMOND_STAND ||
        object == ZORK2_SAPPHIRE_STAND) return 8;
    return 0;
}

static unsigned draw_stand_spheres(const ItemPlacement *stand, const Targets *targets, unsigned focused,
                                   ItemBounds *occupied, bool *drawn) {
    unsigned socket_height = stand_socket_height(stand->object);
    if (!socket_height) return 0;

    // Any crystal can be placed here. Additional occupants cluster around the base.
    static const int8_t shifts[] = {0, -14, 14, 28};
    unsigned rendered = 0;
    for (unsigned i = 0; i < targets->count; i++) {
        unsigned object = targets->id[i];
        if ((object != ZORK2_RUBY_SPHERE && object != ZORK2_DIAMOND_SPHERE &&
             object != ZORK2_SAPPHIRE_SPHERE && object != ZORK2_OBSIDIAN_SPHERE) ||
            z_parent(object) != stand->object) continue;
        const SceneSprite *art = item_pose(1, object);
        ScenePoint at = {stand->at.x + shifts[rendered], stand->at.y};
        if (!rendered) at.y -= socket_height;
        ItemBounds bounds = item_bounds(art, at);
        draw_room_item(object, art, bounds, rendered != 0, object == focused);
        occupied[rendered++] = bounds;
        drawn[object] = true;
    }
    return rendered;
}

static bool open_item_place(ItemBounds bound, const ItemBounds *occupied, unsigned count) {
    if (bound.x < 2 || bound.x + bound.width > SCREEN_W - 2 ||
        bound.y < 17 || bound.y + bound.height > 89) return false;
    for (unsigned i = 0; i < count; i++) {
        const ItemBounds *prev = &occupied[i];
        if (bound.x < prev->x + prev->width + 2 && prev->x < bound.x + bound.width + 2 &&
            bound.y < prev->y + prev->height + 2 && prev->y < bound.y + bound.height + 2)
            return false;
    }
    return true;
}

static bool choose_item_place(const SceneSprite *art, const RoomFloor *floor, const ItemBounds *occupied,
                              unsigned count, ItemBounds *out) {
    static const int8_t shifts[] = {0, -18, 18, -31, 31};
    for (unsigned j = 0; j < sizeof(shifts) / sizeof(shifts[0]); j++) {
        ScenePoint spots[] = {floor->right, floor->left};
        for (unsigned i = 0; i < sizeof(spots) / sizeof(spots[0]); i++) {
            int x = spots[i].x + shifts[j];
            if (x < 2 || x >= SCREEN_W - 2) continue;
            *out = item_bounds(art, (ScenePoint){x, spots[i].y});
            if (floor->background == art_z2_volcano_view &&
                (out->x < 168 || out->x + out->width > 239)) continue;
            if (open_item_place(*out, occupied, count)) return true;
        }
    }
    return false;
}

static void draw_scaled_scene_sprite(const SceneSprite *art, int x, int y, int width, int height);

// Story-local targets whose object-tree parent is another room or container, but
// whose description and art place them in this room (thief's loot, the shared
// coal pile and diamond, the attic knife, the nest egg).
static const struct { uint8_t chapter; uint16_t room, object; } loose_extras[] = {
    {0, 89, 192}, {0, 245, 192}, {0, 249, 192}
};

static bool loose_extra(unsigned chapter, unsigned room, unsigned object) {
    for (unsigned i = 0; i < sizeof(loose_extras) / sizeof(loose_extras[0]); i++)
        if (loose_extras[i].chapter == chapter && loose_extras[i].room == room &&
            loose_extras[i].object == object) return true;
    return false;
}

static void draw_room_items(unsigned chapter, unsigned room, const Color *background,
                            const Targets *targets, unsigned focused) {
    bool drawn[256] = {false};
    ItemBounds occupied[TARGET_LIMIT];
    unsigned count = 0;
    unsigned overflow = 0;
    const SceneSprite *focused_art = NULL;
    const SceneSprite *focused_compact = NULL;
    ScenePoint focused_compact_at = {0, 0};
    ItemBounds focused_bounds = {0, 0, 0, 0};
    if (chapter == 1) {
        for (unsigned i = 0; i < sizeof(encounters) / sizeof(*encounters); i++) {
            const Encounter *figure = &encounters[i];
            if (figure->chapter != chapter || figure->icon == WIZARD ||
                !in_scene(targets, figure->object) || z_parent(figure->object) != room) continue;
            const SceneSprite *pose = encounter_pose(figure, room);
            ScenePoint at = encounter_position(figure, room, background);
            occupied[count++] = (ItemBounds){at.x - pose->width / 2 - 2, at.y - 12, pose->width + 4, 14};
        }
    }
    for (unsigned i = 0; i < sizeof(item_placements) / sizeof(item_placements[0]); i++) {
        const ItemPlacement *place = &item_placements[i];
        unsigned object = place->object;
        bool shared_hades_bodies = chapter == 0 && object == 144 &&
                                   (room == 13 || room == 38);
        if (chapter != place->chapter || room != place->room || drawn[object] || !in_scene(targets, object) ||
            (!shared_hades_bodies && z_parent(object) != place->parent)) continue;
        if (chapter == 2 && room == 30 && object == 59 && in_scene(targets, 110)) {
            // The encounter sprite includes the cloak while the hooded figure
            // wears it. Don't also draw a loose cloak at its post-encounter spot.
            drawn[object] = true;
            continue;
        }
        // When the player is aboard, replace the shore-side boat sprite with
        // a foreground view from inside the boat (drawn after the room items).
        if (chapter == 0 && object == 208 && z_parent(chapters[0].player) == 208) {
            drawn[object] = true;
            continue;
        }
        if (chapter == 1) {
            if ((object == 56 || object == 33) && z_attr(object, 2)) continue;
            if (room == 46 && object == 100 && !z_attr(object, 14)) continue;
        }
        if (chapter == 2 && object == 18 && z_attr(object, ZORK1_TOUCHED)) continue;
        // These story props initially sit on scenery but share the room as a parent with dropped items.
        if (chapter == 0 && room == 75 && z_attr(object, ZORK1_TOUCHED)) continue;
        // In the Clearing's covered state, the leaf pile is part of the
        // background illustration. Keep it selectable without drawing the
        // small inventory sprite over the larger scene art.
        if (chapter == 0 && room == 160 && object == 157 &&
            z_parent(object) == room && z_attr(29, ATTR_INVISIBLE)) {
            occupied[count++] = (ItemBounds){94, 56, 52, 21};
            drawn[object] = true;
            continue;
        }
        if (chapter == 0 && object == 179) {
            // Opening the coffin sets TOUCHBIT too. The story clears its pickup score only when taken.
            const uint8_t *value;
            if (z_property(object, ZORK1_PICKUP_SCORE, &value) == 2 && !(value[0] || value[1])) continue;
        }
        const SceneSprite *art = place->special_art ? place->special_art : item_pose(chapter, object);
        if (!art) continue;
        ScenePoint at = place->at;
        if (chapter == 1 && object == 79 && z_attr(43, ATTR_OPEN)) {
            at = (ScenePoint){145, 54};
            art = z_global(142) ? &ArtLizardHeadAsleepOpen : &ArtLizardHeadAwakeOpen;
        }
        if (chapter == 0 && room == 160 && object == 157 && in_scene(targets, 29)) at.x += 48;
        ItemBounds bounds = item_bounds(art, at);
        bool represented_by_scene_anchor = chapter == 2 && room == ZORK3_CLIFF_LEDGE && object == 137;
        bool selected = object == focused ||
                        (chapter == 2 && object == ZORK3_SWORD_ROCK &&
                         focused == ZORK3_SWORD && z_parent(focused) == object);
        if (chapter == 0 && room == 122 && object == 92) {
            // The old large frame is centered above an arch. The story painting
            // belongs on the smaller, lighter wall patch to the right.
            bounds = (ItemBounds){at.x - 12, at.y - 22, 24, 22};
            draw_scaled_scene_sprite(art, bounds.x, bounds.y, bounds.width, bounds.height);
            if (selected) {
                unsigned focus_object = selected ? focused : object;
                report_focused(focus_object, bounds.x, bounds.y, bounds.width, bounds.height);
                corners(focus_object, bounds.x - 1, bounds.y - 1, bounds.width + 2, bounds.height + 2, 3);
            }
        } else {
            draw_room_item(selected ? focused : object, art, bounds, place->shadow,
                           selected && !represented_by_scene_anchor);
        }
        if (chapter == 0) draw_zork1_item_contents(object, bounds, targets, focused);
        if (chapter == 1) draw_zork2_item_contents(object, bounds, targets, focused);
        if (chapter == 2) draw_zork3_item_contents(object, bounds, targets, focused);
        occupied[count++] = bounds;
        drawn[object] = true;
        if (chapter == 1 && room == ZORK2_WORKROOM)
            count += draw_stand_spheres(place, targets, focused, occupied + count, drawn);
    }
    const RoomFloor *floor = room_floor(background);
    if (!floor || !floor->left.y) return;
    // A selected loose item gets a visible slot even after a crowded treasure drop.
    for (unsigned pass = 0; pass < 2; pass++) {
      for (unsigned i = 0; i < targets->count; i++) {
        unsigned object = targets->id[i];
        if ((object == focused) != (pass == 0)) continue;
        bool represented_by_scene_anchor = chapter == 2 && room == ZORK3_CLIFF_LEDGE && object == 137;
        bool extra = loose_extra(chapter, room, object);
        if (object >= sizeof(drawn) / sizeof(drawn[0]) || drawn[object] ||
            (!extra && (z_parent(object) != room ||
             (!z_attr(object, ATTR_PORTABLE) && !(chapter == 2 && object == ZORK3_TIME_MACHINE))))) continue;
        // The boat has no shore-side placement in several river rooms, so it
        // reaches this generic dropped-item pass while the player is aboard.
        // The interior POV below already depicts the boat around the player.
        if (chapter == 0 && object == 208 && z_parent(chapters[0].player) == 208) {
            drawn[object] = true;
            continue;
        }
        const SceneSprite *art = item_pose(chapter, object);
        ItemBounds bounds;
        if (!art) continue;
        if (!choose_item_place(art, floor, occupied, count, &bounds)) {
            // Densely piled items stay visible even when a room has no free floor slot.
            unsigned slot = overflow++ % 96;
            ScenePoint at = {floor->right.x + (int)(slot % 12) * 4 - 22,
                             floor->right.y - (int)(slot / 12) * 4};
            draw_contents_sprite(object, art, at, object == focused ? 20 : 5,
                                 object == focused ? 18 : 4,
                                 object == focused && !represented_by_scene_anchor);
            if (object == focused && !represented_by_scene_anchor) {
                ItemBounds compact = contents_bounds(art, at, 20, 18);
                report_focused(object, compact.x, compact.y, compact.width, compact.height);
                focused_compact = art; focused_compact_at = at;
            }
            continue;
        }
        draw_room_item(object, art, bounds, true, object == focused && !represented_by_scene_anchor);
        if (chapter == 0) draw_zork1_item_contents(object, bounds, targets, focused);
        if (chapter == 1) draw_zork2_item_contents(object, bounds, targets, focused);
        if (chapter == 2) draw_zork3_item_contents(object, bounds, targets, focused);
        occupied[count++] = bounds;
        if (object == focused && !represented_by_scene_anchor) {
            focused_art = art;
            focused_bounds = bounds;
        }
      }
    }
    if (focused_art) {
        // Keep the inspected object legible in front of compact overflow piles.
        draw_room_item(focused, focused_art, focused_bounds, false, true);
        if (chapter == 1) draw_zork2_item_contents(focused, focused_bounds, targets, focused);
        if (chapter == 2) draw_zork3_item_contents(focused, focused_bounds, targets, focused);
    }
    if (focused_compact) draw_contents_sprite(focused, focused_compact, focused_compact_at, 20, 18, true);
}

static void draw_zork1_trophy_case(const Targets *targets, unsigned focused) {
    if (!in_scene(targets, 197) || z_parent(197) != 75) return;
    unsigned slot = 0;
    for (unsigned i = 0; i < targets->count; i++) {
        unsigned object = targets->id[i];
        if (z_parent(object) != 197) continue;
        if (object == 24) continue; // The ancient map has its own full-size case placement.
        const SceneSprite *art = item_pose(0, object);
        if (!art) continue;
        unsigned column = slot % 5, row = slot / 5;
        draw_contents_sprite(object, art, (ScenePoint){187 + (int)column * 7, 53 - (int)row * 6},
                             5, 5, object == focused);
        slot++;
    }
}

static void draw_menhir(unsigned focused) {
    // MENHIR-POSITION in the bundled Zork II story tracks movement and levitation.
    unsigned position = z_global(45);
    if (position == 2 || z_parent(166) == 91) return;
    const SceneSprite *art = position == 1 ? &ArtMenhirFallen : &ArtMenhirUpright;
    ScenePoint at = position == 1 ? (ScenePoint){100, 83} : (ScenePoint){40, position == 3 ? 64 : 79};
    ItemBounds bounds = item_bounds(art, at);
    // In Release 63, waving the wand at the upright menhir sets G63 to -1
    // until the follow-up "float" command. That is the short-lived dim-glow state.
    bool enchanted = position == 0 && z_global(63) == 0xffff;
    if (enchanted) {
        Color aura = RGB(119, 168, 143);
        tint_rect(bounds.x - 4, bounds.y + 7, bounds.width + 8, bounds.height - 14, aura, 1);
        tint_rect(bounds.x - 2, bounds.y + 10, bounds.width + 4, bounds.height - 20,
                  RGB(147, 190, 153), 2);
        for (int y = bounds.y + 9; y < bounds.y + bounds.height - 8; y += 5) {
            pixel(bounds.x - 2, y, aura);
            pixel(bounds.x + bounds.width + 1, y + 2, aura);
        }
    }
    draw_room_item(focused, art, bounds, position != 3, focused == 84 || focused == 166);
    if (position != 1)
        label(at.x - 1, bounds.y + art->height / 3, "F", enchanted ? RGB(157, 207, 167) : EDGE);
}

static void draw_grating(unsigned room, const Targets *targets, unsigned focused) {
    if (room == 160) {
        if (focused == 29 && in_scene(targets, 29)) {
            report_focused_feature(29, 102, 56, 36, 19);
            corners(29, 102, 56, 36, 19, 3);
        }
        if (focused == 157 && in_scene(targets, 157) &&
            z_parent(157) == room && z_attr(29, ATTR_INVISIBLE)) {
            report_focused_feature(157, 94, 56, 52, 21);
            corners(157, 94, 56, 52, 21, 3);
        }
    } else if (room == 216 && focused == 29 && in_scene(targets, 29)) {
        report_focused_feature(29, 56, 15, 128, 25);
        corners(29, 56, 15, 128, 25, 3);
    }
}

static void draw_shaft_chain(unsigned room) {
    bool raised = z_parent(108) == 245;
    int x = room == 245 ? 80 : 119;
    int end = room == 245 ? (raised ? 56 : 81) : (raised ? 28 : 65);
    for (int y = 15; y < end; y += 4) {
        pixel(x - 1, y, EDGE);
        pixel(x, y + 1, STEEL);
        pixel(x, y + 2, STEEL);
        pixel(x - 1, y + 3, SHADOW);
    }
}

static void draw_dam_indicator(unsigned focused) {
    bool enabled = z_global(88) != 0;
    Color glass = enabled ? RGB(106, 213, 114) : RGB(63, 92, 58);
    fill(116, 56, 8, 5, SHADOW);
    fill(117, 56, 6, 1, STEEL);
    fill(117, 57, 6, 3, glass);
    pixel(118, 57, enabled ? RGB(223, 255, 191) : EDGE);
    if (focused == 200) focus_feature_box(200, 114, 54, 12, 9, 3);
}

static void draw_maintenance_buttons(unsigned focused) {
    static const struct { uint8_t id, x; Color color; } buttons[] = {
        {225, 99, RGB(92, 156, 203)}, {63, 119, RGB(211, 181, 78)},
        {151, 139, RGB(149, 103, 68)}, {212, 160, RGB(188, 77, 60)},
    };
    for (unsigned i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        int x = buttons[i].x;
        fill(x - 3, 35, 7, 4, buttons[i].color);
        fill(x - 2, 34, 5, 1, buttons[i].color);
        fill(x - 2, 39, 5, 1, SHADOW);
        pixel(x - 2, 35, RGB(235, 220, 177));
        if (focused == buttons[i].id) focus_feature_box(buttons[i].id, x - 6, 31, 13, 12, 3);
    }
}

static void draw_maintenance_water(void) {
    unsigned level = z_global(94);
    if (level == 0 || level > 14) return;
    int surface = 89 - 3 * level;
    tint_rect(0, surface, SCREEN_W, 91 - surface, RGB(49, 104, 112), 8);
    tint_rect(0, surface, SCREEN_W, 1, RGB(130, 172, 170), 9);
    for (int y = surface + 3; y < 90; y += 5) {
        tint_rect(17 + (y & 7) * 3, y, 80, 1, RGB(163, 193, 177), 5);
        tint_rect(140 - (y & 7) * 2, y + 1, 65, 1, RGB(163, 193, 177), 5);
    }
    if (surface > 53) tint_rect(187, 51, 3, surface - 51, RGB(125, 186, 188), 7);
}

static void draw_vehicle_contents(unsigned parent, const Targets *targets, unsigned focused,
                                  ScenePoint first, int spacing, unsigned capacity,
                                  unsigned width, unsigned height) {
    unsigned slot = 0;
    for (unsigned pass = 0; pass < 2; pass++) {
      for (unsigned i = 0; i < targets->count && slot < capacity; i++) {
        unsigned child = targets->id[i];
        if (child == 40 || (parent == 52 && child == 186) ||
            ((child == focused) != (pass == 0)) || z_parent(child) != parent) continue;
        const SceneSprite *art = item_pose(1, child);
        if (!art) continue;
        draw_contents_sprite(child, art, (ScenePoint){first.x + (int)slot * spacing, first.y},
                             width, height, focused == child);
        slot++;
      }
    }
}

static void draw_z2_bucket_bail(int center_x, int rim_y) {
    const Color iron_shadow = RGB(48, 48, 44);
    const Color iron = RGB(124, 126, 114);
    const Color iron_highlight = RGB(183, 166, 126);
    ScenePoint arch[] = {
        {center_x - 8, rim_y + 4}, {center_x - 8, rim_y + 1},
        {center_x - 5, rim_y - 3}, {center_x, rim_y - 5},
        {center_x + 5, rim_y - 3}, {center_x + 8, rim_y + 1},
        {center_x + 8, rim_y + 4}
    };
    for (unsigned i = 0; i + 1 < sizeof(arch) / sizeof(arch[0]); i++)
        scene_segment(arch[i], arch[i + 1], iron_shadow);
    for (unsigned i = 0; i + 1 < sizeof(arch) / sizeof(arch[0]); i++) {
        ScenePoint a = {arch[i].x + 1, arch[i].y};
        ScenePoint b = {arch[i + 1].x + 1, arch[i + 1].y};
        scene_segment(a, b, iron);
    }
    pixel(center_x, rim_y - 5, iron_highlight);
}

static void draw_z2_bucket_tether(int center_x, int end_y) {
    int length = end_y - 10 + 1;
    if (length <= 0) return;
    fill(center_x - 2, 10, 5, length, RGB(48, 37, 29));
    fill(center_x - 1, 10, 3, length, RGB(122, 82, 48));
    fill(center_x, 10, 1, length, RGB(194, 145, 81));
    for (int y = 12; y + 2 < end_y; y += 4) {
        pixel(center_x - 1, y, RGB(177, 126, 69));
        pixel(center_x + 1, y + 2, RGB(75, 52, 34));
    }
}

static void draw_zork2_bucket(unsigned room, const Targets *targets, unsigned focused) {
    if ((room != 119 && room != 8) || !in_scene(targets, 52) || z_parent(52) != room) return;
    if (z_parent(40) == 52) {
        const SceneSprite *rim = &ArtBucketPovRim;
        draw_room_item(52, rim, item_bounds(rim, (ScenePoint){120, 90}), false, focused == 52);
        draw_vehicle_contents(52, targets, focused, (ScenePoint){65, 81}, 22, 6, 18, 11);
        // The puddle is below the rider's view; focus locates it inside this bucket.
        if (focused == 186 && z_parent(186) == 52) focus_feature_box(186, 106, 78, 28, 8, 2);
        return;
    }
    const SceneSprite *art = &ArtBucketWooden;
    ScenePoint at = room == 8 ? (ScenePoint){82, 82} : (ScenePoint){141, 86};
    ItemBounds bounds = item_bounds(art, at);
    if (room == 119) {
        // From the foot chamber, the bucket hangs directly below the unseen
        // wellhead. The braided cable reaches the bail at the top of the scene.
        draw_z2_bucket_tether(at.x, bounds.y - 3);
    }
    draw_room_item(52, art, bounds, room == 119, focused == 52);
    // The handle is raised while suspended and gives the rope a visible anchor.
    draw_z2_bucket_bail(at.x, bounds.y + 2);
    if (z_parent(186) == 52) {
        // The story specifies a puddle, not a bucket filled to its rim.
        int y = bounds.y + 8;
        fill(at.x - 5, y, 10, 1, RGB(42, 83, 91));
        fill(at.x - 3, y + 1, 6, 1, RGB(79, 119, 128));
        if (focused == 186) focus_feature_box(186, at.x - 7, y - 1, 14, 4, 2);
    }
    draw_vehicle_contents(52, targets, focused, (ScenePoint){at.x - 6, bounds.y + 10}, 6, 3, 5, 5);
}

static void scene_segment(ScenePoint a, ScenePoint b, Color color) {
    int dx = b.x > a.x ? b.x - a.x : a.x - b.x;
    int dy = b.y > a.y ? a.y - b.y : b.y - a.y;
    int sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        pixel(a.x, a.y, color);
        if (a.x == b.x && a.y == b.y) break;
        int twice = error * 2;
        if (twice >= dy) { error += dy; a.x += sx; }
        if (twice <= dx) { error += dx; a.y += sy; }
    }
}

static void wire_segment(ScenePoint a, ScenePoint b) { scene_segment(a, b, STEEL); }

static void draw_scaled_scene_sprite(const SceneSprite *art, int x, int y, int width, int height) {
    int source_y = 0, y_error = 0;
    for (int row = 0; row < height; row++) {
        int source_x = 0, x_error = 0;
        for (int col = 0; col < width; col++) {
            Color color = art->pixels[source_y * art->width + source_x];
            if (color & 0x8000) pixel(x + col, y + row, color & 0x7fff);
            x_error += art->width;
            while (x_error >= width) {
                source_x++;
                x_error -= width;
            }
        }
        y_error += art->height;
        while (y_error >= height) {
            source_y++;
            y_error -= height;
        }
    }
}

static void draw_z3_guardians(unsigned room, unsigned focused) {
    int left_center, right_center, width, height, baseline;
    switch (room) {
        case ZORK3_GUARDIANS_HALL:
            // The player is passing between the pair, so they stand at full scale
            // on the near alcove floors.
            left_center = 44; right_center = 196; width = 38; height = 59; baseline = 82;
            break;
        case ZORK3_GUARDIANS_SOUTH:
            // From the mirror side, the same statues are farther up the corridor.
            left_center = 62; right_center = 178; width = 25; height = 39; baseline = 66;
            break;
        case ZORK3_GUARDIANS_NORTH:
            // From the north approach, they remain distant but slightly larger.
            left_center = 60; right_center = 180; width = 29; height = 44; baseline = 72;
            break;
        default:
            return;
    }
    int left_x = left_center - width / 2;
    int right_x = right_center - width / 2;
    int top = baseline - height;
    draw_scaled_scene_sprite(&ArtGuardiansLeft, left_x, top, width, height);
    draw_scaled_scene_sprite(&ArtGuardiansRight, right_x, top, width, height);
    if (focused == 152) {
        // The target is the identical pair; frame both statues as one object.
        int x = left_x < right_x ? left_x : right_x;
        int right = (left_x > right_x ? left_x : right_x) + width;
        report_focused(152, x, top, right - x, height);
        corners(152, x - 2, top - 2, right - x + 4, height + 4, 4);
    }
}

static void draw_z3_museum_plaque(unsigned room, const Targets *targets) {
    if (room != ZORK3_TECHNOLOGY_PAST || !in_scene(targets, 219) || z_parent(219) != room) return;
    // The unfinished museum's source art omits the faded plaque that the
    // story places beside the door. Add a small, wall-mounted brass plate.
    fill(197, 31, 18, 22, RGB(45, 43, 37));
    fill(198, 32, 16, 20, RGB(112, 86, 53));
    fill(199, 33, 14, 18, RGB(151, 116, 68));
    fill(200, 34, 12, 1, RGB(187, 151, 94));
    fill(201, 38, 10, 1, RGB(91, 72, 49));
    fill(201, 41, 9, 1, RGB(96, 74, 50));
    fill(202, 44, 7, 1, RGB(102, 79, 53));
    fill(200, 49, 12, 1, RGB(119, 91, 58));
}

static void draw_z3_prison_master(unsigned room, const Targets *targets, unsigned focused) {
    if (room != 45 || !z_attr(183, ATTR_OPEN) || !in_scene(targets, 11)) return;
    // He stays in the corridor when the player enters the cell. Through the
    // open doorway he is visible at a distance; the closed door occludes him.
    ground_shadow(222, 58, 12);
    draw_scaled_scene_sprite(&ArtMasterEncounter, 222, 34, 12, 25);
    if (focused == 11) report_focused(11, 222, 34, 12, 25);
}

static void draw_zork2_balloon(unsigned room, const Targets *targets, unsigned focused) {
    if (z_parent(169) != room || !in_scene(targets, 169)) return;
    bool aboard = z_parent(40) == 169;
    const SceneSprite *art = aboard ? &ArtBalloonBasketPov :
                            z_global(133) ? &ArtBalloonInflatedVehicle : &ArtBalloonDeflatedVehicle;
    ScenePoint at = aboard ? (ScenePoint){120, 90} :
                    (room == 13 || room == 143) ? (ScenePoint){109, 89} : (ScenePoint){165, 89};
    draw_room_item(focused, art, item_bounds(art, at), !aboard, focused == 169 || focused == 93);
    if (aboard) {
        draw_vehicle_contents(169, targets, focused, (ScenePoint){62, 76}, 20, 6, 17, 10);
    } else draw_vehicle_contents(169, targets, focused, (ScenePoint){at.x - 6, 82}, 5, 3, 4, 4);
    if (z_parent(181) == 126) {
        int x = aboard ? 173 : at.x, y = aboard ? 73 : 79;
        if (z_attr(181, ATTR_LIT)) {
            fill(x - 1, y, 3, 4, RGB(210, 114, 43));
            fill(x, y - 2, 1, 5, RGB(242, 190, 92));
            pixel(x, y, CREAM);
            if (focused == 181) focus_feature_box(181, x - 4, y - 4, 9, 9, 2);
        } else {
            const SceneSprite *fuel = item_pose(1, 181);
            if (fuel) draw_contents_sprite(181, fuel, (ScenePoint){x, y + 2}, 10, 6, focused == 181);
        }
    }
    if ((room == 13 || room == 143) && z_global(51)) {
        // The line terminates at the mounted hook, rather than in the shaft.
        ScenePoint hook = room == 13 ? (ScenePoint){90, 43} : (ScenePoint){146, 78};
        if (room == 143) {
            fill(144, 76, 4, 2, STEEL);
            pixel(147, 78, STEEL);
            pixel(146, 79, STEEL);
        }
        ScenePoint basket = aboard ? (ScenePoint){201, 81} : (ScenePoint){at.x + 9, 82};
        wire_segment(basket, hook);
    }
    if (focused == 126) focus_feature_box(126, 167, 71, 13, 13, 2);
    if (focused == 170) focus_feature_box(170, 198, 72, 21, 15, 2);
}

static void draw_zork2_cage(void) {
    tint_rect(0, 72, SCREEN_W, 18, RGB(144, 157, 134), 1);
    // The room is reached only after the cage drops. The transcript says the
    // gas starts immediately, so show it low behind the bars on every visit.
    tint_rect(0, 78, SCREEN_W, 10, RGB(123, 153, 91), 5);
    unsigned drift = (visual_frame >> 3) & 7;
    Color gas_edge = RGB(91, 128, 89), gas_light = RGB(144, 174, 105);
    for (int i = 0; i < 3; i++) {
        int y = 79 + i * 3;
        int offset = (int)((drift + i * 3) & 7);
        scene_segment((ScenePoint){3 + offset, y + 1}, (ScenePoint){22 + offset, y - 1}, gas_edge);
        scene_segment((ScenePoint){22 + offset, y - 1}, (ScenePoint){42 + offset, y + 1}, gas_light);
        scene_segment((ScenePoint){136 - offset, y + 1}, (ScenePoint){157 - offset, y - 1}, gas_edge);
        scene_segment((ScenePoint){157 - offset, y - 1}, (ScenePoint){179 - offset, y + 1}, gas_light);
    }
    for (int x = 23; x < SCREEN_W; x += 40) {
        fill(x, 17, 4, 73, RGB(48, 60, 63));
        fill(x + 1, 17, 1, 73, RGB(151, 167, 169));
        fill(x + 2, 17, 1, 73, RGB(88, 111, 117));
    }
    fill(0, 18, SCREEN_W, 3, RGB(73, 91, 95));
    fill(0, 87, SCREEN_W, 3, RGB(73, 91, 95));
}

static void draw_zork2_cues(unsigned room, const Targets *targets, unsigned focused) {
    draw_zork2_bucket(room, targets, focused);
    draw_zork2_balloon(room, targets, focused);
    if (focused == 43 && room == 45) focus_feature_box(43, 179, 16, 56, 65, 3);
    if (focused == 43 && room == 28) focus_feature_box(43, 137, 16, 66, 65, 3);
    if (room == 182 && z_parent(225) == 163) {
        // The charge occupies the chipped oblong hole in the safe's front door.
        draw_contents_sprite(225, &ArtBrickClay, (ScenePoint){181, 51}, 11, 6, focused == 225);
        if (z_parent(213) == 225) {
            wire_segment((ScenePoint){182, 48}, (ScenePoint){186, 52});
            if (zork2_fuse_burning()) draw_fuse_spark(186, 52);
            if (focused == 213) focus_feature_box(213, 180, 46, 9, 8, 2);
        }
    }
    if (room == 87 && z_global(149) == 1) {
        // The square-button order increases the whirring without changing the
        // robot's pose. Draw broad sound waves that remain legible in still
        // captures, while a four-frame pulse keeps the live scene animated.
        unsigned pulse = (visual_frame >> 2) & 3;
        Color glow = pulse & 1 ? RGB(255, 210, 120) : RGB(171, 222, 211);
        Color outer = pulse & 1 ? RGB(221, 159, 82) : RGB(124, 180, 172);
        for (int ring = 0; ring < 3; ring++) {
            int inset = 7 + ring * 5;
            int left = 96 - inset;
            int right = 144 + inset;
            int top = 47 - ring;
            int bottom = 71 + ring;
            Color wave = ring == 2 ? outer : glow;
            scene_segment((ScenePoint){left + 4, top}, (ScenePoint){left, top + 5}, wave);
            scene_segment((ScenePoint){left, top + 5}, (ScenePoint){left, bottom - 5}, wave);
            scene_segment((ScenePoint){left, bottom - 5}, (ScenePoint){left + 4, bottom}, wave);
            scene_segment((ScenePoint){right - 4, top}, (ScenePoint){right, top + 5}, wave);
            scene_segment((ScenePoint){right, top + 5}, (ScenePoint){right, bottom - 5}, wave);
            scene_segment((ScenePoint){right, bottom - 5}, (ScenePoint){right - 4, bottom}, wave);
        }
        int flicker = pulse & 1 ? 1 : 0;
        pixel(116, 43, CREAM);
        pixel(121 + flicker, 44, glow);
        pixel(126, 43 + flicker, CREAM);
    }
    if (room == 162 && z_parent(72) == 58) {
        const SceneSprite *collar = &ArtCollarWorn;
        // Only the front/lower arc is in front of the Cerberus from this view;
        // the upper half wraps behind the neck and is hidden by the body.
        int width = collar->width, height = collar->height;
        int x = 145 - width / 2, y = 65 - height;
        int front = height / 2;
        draw_masked_pixels(x, y + front, width, height - front,
                           collar->pixels + front * width, false);
        if (focused == 72) focus_feature_box(72, x, y, width, height, 3);
    }
    if ((room == 46 || room == 59) && !z_attr(137, ATTR_OPEN)) {
        unsigned lid = room == 46 ? 112 : 109;
        int x = room == 46 ? 132 : 80, y = room == 46 ? 55 : 55;
        if (z_attr(lid, ATTR_OPEN)) {
            fill(x - 1, y - 2, 3, 5, RGB(80, 83, 76));
            fill(x, y - 1, 1, 3, SHADOW);
            fill(x + 3, y - 4, 2, 3, STEEL);
        }
        if (room == 59 && focused == lid)
            focus_feature_box(lid, x - 3, y - 5, 10, 10, 2);
    }
    if (room == 202 || room == 121 || room == 242 || room == 75 ||
        room == 95 || room == 142 || room == 185 || room == 226) {
        // Story text: global 58 = 0..4 reads dark, flickering dimly, dimly glowing, glowing,
        // glowing brightly; global 13 (passage opened) reads "glowing serenely".
        unsigned glow = z_global(13) ? 5 : (z_global(58) > 4 ? 4 : z_global(58));
        Color color = RGB(25 + glow * 32, 28 + glow * 30, 21 + glow * 20);
        for (int y = 0; y < 5; y++) {
            int half = y < 2 ? y + 1 : 5 - y;
            fill(120 - half, 74 + y, half * 2, 1, color);
        }
        if (focused == 216) focus_feature_box(216, 113, 72, 14, 9, 2);
    }
    if (room == 106) draw_zork2_cage();
    if (room == 230 && (finale_stage == 0 || (finale_stage == 1 && finale_age < 64))) {
        // Follow the displayed narration: undulating lines, their flare, then disappearance.
        unsigned glow = finale_stage == 1 ? 64 - finale_age : 18 + ((visual_frame >> 2) & 7);
        Color color = RGB(24 + glow * 3, 80 + glow * 2, 38 + glow * 2);
        int drift = (visual_frame >> 3) & 3;
        for (int i = 0; i < 3; i++) {
            int y = 76 + i * 4;
            scene_segment((ScenePoint){8, y + drift}, (ScenePoint){34, y - 2}, color);
            scene_segment((ScenePoint){34, y - 2}, (ScenePoint){57, y + 2 - drift}, color);
            scene_segment((ScenePoint){57, y + 2 - drift}, (ScenePoint){83, y - 2}, color);
        }
    }
}

static void draw_zork1_boat_pov(unsigned room, const Targets *targets, unsigned focused) {
    if (z_parent(chapters[0].player) != 208 || z_parent(208) != room) return;

    // Use the same approved tube and dark single-slat floor in every river room.
    // Cargo is drawn afterward so it remains visible on top of the interior.
    draw_masked_pixels(0, 68, ArtBoatInteriorView.width, ArtBoatInteriorView.height,
                       ArtBoatInteriorView.pixels, false);

    // Keep visible cargo inside the raft instead of drawing it on the shore-side hull.
    unsigned slot = 0;
    for (unsigned i = 0; i < targets->count; i++) {
        unsigned child = targets->id[i];
        if (z_parent(child) != 208) continue;
        const SceneSprite *art = child == 180 ? &ArtBoatLabelTan : item_pose(0, child);
        if (!art) continue;
        ScenePoint at = {120 + (int)(slot % 3) * 10 - 10, 87 - (int)(slot / 3) * 4};
        draw_boat_cargo(child, art, at, child == focused);
        slot++;
    }
    if (focused == 208) focus_feature_box(208, 0, 68, 240, 23, 3);   // boat seen from inside: rows 68-90, the bottom of the art band
}

static void draw_z3_room8_temporal_exit(void) {
    unsigned phase = (visual_frame >> 2) & 7;
    tint_rect(203, 34, 21, 37, RGB(72, 98, 97), 2);
    for (int i = 0; i < 8; i++) {
        int y = 35 + i * 4;
        int dx = (int)((i + phase) % 5) - 2;
        Color pale = (i & 1) ? RGB(148, 167, 153) : RGB(119, 153, 158);
        Color cool = (i & 1) ? RGB(93, 137, 143) : RGB(148, 159, 134);
        scene_segment((ScenePoint){205 + dx, y}, (ScenePoint){210 + dx, y + 2}, pale);
        scene_segment((ScenePoint){215 - dx, y + 1}, (ScenePoint){218 - dx, y + 3}, cool);
    }
}

static void draw_manhole_disc(int cx, int cy) {
    fill(cx - 7, cy - 4, 14, 1, RGB(67, 73, 72));
    fill(cx - 10, cy - 3, 20, 1, RGB(103, 111, 109));
    fill(cx - 11, cy - 2, 22, 4, RGB(74, 82, 80));
    fill(cx - 9, cy + 2, 18, 2, RGB(52, 59, 58));
    fill(cx - 6, cy + 4, 12, 1, RGB(38, 45, 44));
    scene_segment((ScenePoint){cx - 10, cy - 2}, (ScenePoint){cx - 7, cy - 4}, RGB(147, 155, 149));
    scene_segment((ScenePoint){cx + 7, cy - 4}, (ScenePoint){cx + 10, cy - 2}, RGB(120, 129, 124));
    scene_segment((ScenePoint){cx - 6, cy}, (ScenePoint){cx + 6, cy}, RGB(54, 62, 61));
}

static void draw_z3_manhole(unsigned focused) {
    bool open = z_attr(135, ATTR_OPEN);
    if (open) {
        fill(178, 58, 20, 6, RGB(29, 34, 33));
        fill(180, 59, 16, 4, RGB(19, 23, 23));
        scene_segment((ScenePoint){178, 58}, (ScenePoint){181, 55}, RGB(120, 124, 114));
        scene_segment((ScenePoint){195, 58}, (ScenePoint){192, 55}, RGB(100, 107, 101));
        draw_manhole_disc(205, 65);
    } else {
        draw_manhole_disc(188, 60);
    }
    if (focused == 135) focus_feature_box(135, 175, 52, 35, 20, 3);
}

static void draw_room_target_anchor(unsigned chapter, unsigned room, const Targets *targets, unsigned focused) {
    if (chapter == 0 && room == 127 && focused == 36 && z_parent(36) == room)
        return; // The axe's floor sprite has its own focus box in this state.
    if (chapter == 0 && room == 75 && focused == 55 && in_scene(targets, 55)) {
        // The rug is spread across the floor until the trap door is moved; after
        // that the same carpet is the rolled bundle at the left of the frame.
        if (z_attr(ZORK1_TRAP_DOOR, ATTR_INVISIBLE))
            focus_feature_box(55, 52, 72, 186, 18, 4);
        else
            focus_feature_box(55, 28, 57, 40, 33, 4);
        return;
    }
    if (chapter == 2 && focused == 11 && in_scene(targets, 11) &&
        (room == 45 || room == 204 || room == 225)) {
        unsigned door = room == 45 ? 183 : (room == 204 ? 153 : 72);
        if (room == 45 && z_attr(door, ATTR_OPEN)) {
            // He stays in the corridor; through the open doorway he is visible
            // at a distance, so frame that sprite.
            focus_feature_box(11, 220, 32, 16, 29, 3);
        } else {
            // He is outside the shut cell door. The barred window is the only
            // view of him and the place the conversation happens.
            focus_feature_box(11, 203, 15, 32, 17, 3);
        }
        return;
    }
    if (chapter == 2 && room == 54 && in_scene(targets, focused) &&
        (focused == 29 || focused == 106)) {
        // The mirror/panel at the north end; room 54 always uses the closed view.
        focus_feature_box(focused, 82, 31, 76, 36, 4);
        return;
    }
    if (chapter == 2 && room == 169 && in_scene(targets, focused) &&
        (focused == 29 || focused == 80 || focused == 106)) {
        if (zork3_mirror_panel_open()) focus_feature_box(focused, 158, 20, 82, 70, 4);
        else focus_feature_box(focused, 82, 31, 76, 36, 4);
        return;
    }
    for (unsigned i = 0; i < sizeof(anchors) / sizeof(anchors[0]); i++) {
        const RoomAnchor *anchor = &anchors[i];
        if (anchor->chapter == chapter && anchor->room == room && anchor->object == focused &&
            in_scene(targets, focused)) {
            // During the encounter the cloak is worn by the hooded figure;
            // after the figure vanishes, the same object is a loose floor item.
            // Only use this figure-sized target while the wearer is present.
            if (chapter == 2 && room == 30 && focused == 59 && !in_scene(targets, 110))
                continue;
            report_focused_feature(focused, anchor->x, anchor->y, anchor->width, anchor->height);
            corners(focused, anchor->x - 2, anchor->y - 2, anchor->width + 4, anchor->height + 4, 4);
        }
    }
}

void draw_room_details(unsigned chapter, unsigned room, const Color *background,
                       const Targets *targets, unsigned focused) {
    focus_box_drawn = false;
    current_focused = focused;
    if (chapter == 1 && room == 125 &&
        (background == art_z2_crypt_dark || background == art_z2_crypt_secret_open)) {
        if (!z_attr(151, ATTR_OPEN)) label(119, 49, "F", RGB(92, 112, 101));
        if (focused == 151) focus_feature_box(151, 103, 24, 35, 43, 3);
        draw_room_target_anchor(chapter, room, targets, focused);
        return;
    }
    if (chapter == 2) {
        draw_z3_beam(room, targets);
        if (room == ZORK3_ROOM_8) draw_z3_room8_temporal_exit();
        if (room == 39) draw_z3_manhole(focused);
        if (room == ZORK3_PARAPET) draw_z3_parapet();
        if (room == 160) draw_z3_mirror_cues();
        if (room == ZORK3_ENGRAVINGS_ROOM) draw_z3_secret_door();
        if (room == ZORK3_SCENIC_VISTA) {
            static const char *const roman[] = {"?", "I", "II", "III", "IV"};
            uint8_t value = visual_state_vista();
            if (value <= VISTA_IV) label(17, 25, roman[value], GOLD);
        }
    }
    if (chapter == 1 && room == 159) draw_menhir(focused);
    if (chapter == 0) {
        if (room == 160 || room == 216) draw_grating(room, targets, focused);
        if (room == 245 || room == 249) draw_shaft_chain(room);
        if (room == 178) draw_dam_indicator(focused);
        if (room == 224) draw_maintenance_buttons(focused);
    }
    if (chapter == 2 && room == 181 && in_scene(targets, ZORK3_VIKING_SHIP)) {
        const SceneSprite *sailor = &ArtSailorEncounter;
        // The bow is the raised left stem. The sailor looks out to sea.
        ground_shadow(35 - sailor->width / 2 + sailor->width / 8, 53,
                      sailor->width * 3 / 4);
        draw_masked_pixels(35 - sailor->width / 2, 56 - sailor->height,
                           sailor->width, sailor->height, sailor->pixels, true);
    }
    for (unsigned i = 0; i < sizeof(encounters) / sizeof(encounters[0]); i++) {
        const Encounter *figure = &encounters[i];
        if (figure->chapter != chapter || !in_scene(targets, figure->object)) continue;
        if (chapter == 0 && figure->object == ZORK1_TROLL && visual_state_troll_disappeared()) continue;
        unsigned parent = z_parent(figure->object);
        bool through_glass = chapter == 1 && room == ZORK2_AQUARIUM_ROOM &&
                             figure->object == ZORK2_SERPENT && parent == ZORK2_AQUARIUM;
        if (parent == room || through_glass || (chapter == 2 && figure->object == ZORK3_CLIFF_MAN))
            draw_encounter(figure, room, background, focused);
    }
    if (chapter == 2) {
        draw_z3_museum_plaque(room, targets);
        draw_z3_prison_master(room, targets, focused);
        draw_z3_guardians(room, focused);
    }
    draw_room_items(chapter, room, background, targets, focused);
    if (chapter == 2 && room == ZORK3_CLIFF_LEDGE &&
        in_scene(targets, 137) && z_parent(137) == room) {
        // Continue the loose rope above its existing hanging tail so it visibly
        // descends from the cliff edge described in the room text.
        scene_segment((ScenePoint){168, 17}, (ScenePoint){168, 48}, RGB(70, 52, 38));
        scene_segment((ScenePoint){169, 17}, (ScenePoint){169, 48}, RGB(154, 110, 66));
        scene_segment((ScenePoint){170, 17}, (ScenePoint){170, 48}, RGB(208, 162, 102));
        scene_segment((ScenePoint){166, 17}, (ScenePoint){172, 17}, RGB(61, 48, 39));
        for (int y = 22; y < 48; y += 6) {
            pixel(168, y, RGB(202, 154, 95));
            pixel(170, y + 3, RGB(91, 65, 45));
        }
    }
    if (chapter == 0 && room == 75) draw_zork1_trophy_case(targets, focused);
    if (chapter == 1 && in_scene(targets, 18) && z_parent(18) == room && z_parent(24) == 18) {
        const SceneSprite *wand = item_pose(1, 24);
        if (wand) draw_room_item(24, wand, item_bounds(wand, (ScenePoint){184, 47}), false, focused == 24);
    }
    if (chapter == 2) draw_z3_jewel_display(room, background, targets, focused);
    if (chapter == 1) draw_zork2_cues(room, targets, focused);
    if (chapter == 0 && room == 224) draw_maintenance_water();
    if (chapter == 0) draw_zork1_boat_pov(room, targets, focused);
    draw_room_target_anchor(chapter, room, targets, focused);
}

void draw_z3_ending_cue(unsigned stage) {
    if (!stage) return;

    const SceneSprite *mage = &ArtMasterEncounter;
    if (stage == 1) {
        // The narration briefly shows the old Master and his identical successor.
        static const int8_t lefts[] = {84, 132};
        for (unsigned i = 0; i < sizeof(lefts); i++) {
            int x = lefts[i], y = 85 - mage->height;
            ground_shadow(x + 3, 82, mage->width - 6);
            draw_masked_pixels(x, y, mage->width, mage->height, mage->pixels, false);
        }
        return;
    }

    // Once the counterpart vanishes, the transformed player remains as Master.
    int x = 108, y = 85 - mage->height;
    ground_shadow(x + 3, 82, mage->width - 6);
    draw_masked_pixels(x, y, mage->width, mage->height, mage->pixels, false);
    Color mist = RGB(176, 194, 171), edge = RGB(118, 151, 132);
    scene_segment((ScenePoint){145, 75}, (ScenePoint){150, 70}, edge);
    scene_segment((ScenePoint){150, 70}, (ScenePoint){156, 74}, mist);
    scene_segment((ScenePoint){146, 79}, (ScenePoint){153, 76}, mist);
    pixel(153, 67, mist);
    pixel(158, 72, edge);
}

static const Color tile_bg[] = {
    RGB(43, 58, 53), RGB(117, 139, 141), RGB(167, 110, 57), RGB(21, 30, 29), RGB(45, 70, 62)
};
static const Color tile_edge[] = {
    RGB(65, 79, 67), RGB(183, 196, 178), RGB(227, 172, 101), RGB(52, 65, 58), RGB(121, 171, 139)
};

static void map_tile(int x, int y, int size, PuzzleTile tile) {
    box(x, y, size, size, tile_bg[tile], tile_edge[tile]);
    if (tile == PUZZLE_MARBLE || tile == PUZZLE_SANDSTONE) {
        fill(x + 2, y + 2, size - 4, 1, tile_edge[tile]);
        fill(x + 2, y + size / 2, size - 4, 1, tile_edge[tile]);
    } else if (tile == PUZZLE_PLAYER) {
        int cx = x + size / 2, cy = y + size / 2;
        for (int row = -3; row <= 3; row++) {
            int radius = 3 - (row < 0 ? -row : row);
            fill(cx - radius, cy + row, radius * 2 + 1, 1, GOLD);
        }
    }
}

static void draw_puzzle_floor_items(const Targets *targets, unsigned focused) {
    fill(0, 75, SCREEN_W, 16, RGB(32, 45, 41));
    fill(0, 75, SCREEN_W, 1, EDGE);
    label(7, 80, "Floor", CREAM);

    enum { FIRST_X = 44, SLOT_STEP = 17, MAX_VISIBLE = 10 };
    unsigned visible = 0;
    unsigned floor_items[TARGET_LIMIT];
    for (unsigned i = 0; i < targets->count; i++) {
        unsigned object = targets->id[i];
        if (object >= 256 || z_parent(object) != ROYAL_PUZZLE_ROOM ||
            !z_attr(object, ATTR_PORTABLE) || !item_pose(2, object)) continue;
        floor_items[visible++] = object;
    }

    unsigned first = 0;
    if (visible > MAX_VISIBLE && focused) {
        for (unsigned i = 0; i < visible; i++) {
            if (floor_items[i] != focused) continue;
            first = i > MAX_VISIBLE / 2 ? i - MAX_VISIBLE / 2 : 0;
            if (first > visible - MAX_VISIBLE) first = visible - MAX_VISIBLE;
            break;
        }
    }

    unsigned count = visible - first;
    if (count > MAX_VISIBLE) count = MAX_VISIBLE;
    for (unsigned i = 0; i < count; i++) {
        unsigned object = floor_items[first + i];
        const SceneSprite *art = item_pose(2, object);
        if (art) draw_contents_sprite(object, art, (ScenePoint){FIRST_X + (int)i * SLOT_STEP, 88}, 13, 9,
                                      object == focused);
    }
    if (visible > count) {
        unsigned extra = visible - count;
        char overflow[4] = {'+', '0', '0', 0};
        if (extra > 9) {
            overflow[1] = (char)('0' + extra / 10);
            overflow[2] = (char)('0' + extra % 10);
        } else {
            overflow[1] = (char)('0' + extra);
            overflow[2] = 0;
        }
        label(220, 80, overflow, TEAL);
    }
}

static void draw_puzzle_directions(unsigned focused) {
    static const unsigned ids[] = {TARGET_NORTH_WALL, TARGET_EAST_WALL,
                                   TARGET_SOUTH_WALL, TARGET_WEST_WALL};
    static const int x[] = {44, 63, 44, 25};
    static const int y[] = {21, 40, 59, 40};
    static const char *const labels[] = {"N", "E", "S", "W"};
    box(20, 19, 68, 54, RGB(32, 45, 41), EDGE);
    box(45, 41, 18, 12, RGB(51, 69, 60), RGB(103, 128, 107));
    label(51, 44, "?", CREAM);
    for (unsigned i = 0; i < 4; i++) {
        label(x[i], y[i], labels[i], focused == ids[i] ? GOLD : TEAL);
        if (focused == ids[i]) focus_feature_box(ids[i], x[i] - 3, y[i] - 3, 12, 12, 3);
    }
}

static void draw_puzzle_special_targets(const Targets *targets, unsigned focused) {
    static const struct { unsigned id; int x, y; const char *name; } features[] = {
        {85, 96, 62, "Ladder"}, {117, 139, 62, "Door"}
    };
    for (unsigned i = 0; i < sizeof(features) / sizeof(features[0]); i++) {
        bool available = false;
        for (unsigned j = 0; j < targets->count; j++)
            if (targets->id[j] == features[i].id) available = true;
        if (!available) continue;
        label(features[i].x, features[i].y, features[i].name, CREAM);
        if (focused == features[i].id)
            focus_feature_box(features[i].id, features[i].x - 3, features[i].y - 3,
                              i ? 40 : 36, 14, 3);
    }
}

void draw_puzzle_map(const PuzzleMap *map, const Targets *targets, unsigned focused) {
    focus_box_drawn = false;
    current_focused = focused;
    fill(0, 15, SCREEN_W, 76, RGB(32, 45, 41));
    if (!map->valid) {
        draw_puzzle_directions(focused);
        label(99, 35, "Press B to", CREAM);
        label(99, 45, "survey puzzle.", CREAM);
        draw_puzzle_special_targets(targets, focused);
        draw_puzzle_floor_items(targets, focused);
        return;
    }

    box(6, 17, 228, 57, INK, EDGE);
    const unsigned walls[] = {TARGET_NORTH_WALL, TARGET_EAST_WALL, TARGET_SOUTH_WALL, TARGET_WEST_WALL};
    const unsigned cells[] = {1, 5, 7, 3};
    unsigned selected = 9;
    for (unsigned i = 0; i < 4; i++) {
        if (focused == walls[i]) selected = cells[i];
    }
    label(76, 23, "N", TEAL);
    fill(78, 32, 1, 7, TEAL);
    fill(77, 33, 3, 1, TEAL);
    fill(76, 34, 5, 1, TEAL);
    label(104, 38, "Royal Puzzle", CREAM);
    for (unsigned i = 0; i < 9; i++) {
        int x = 14 + (i % 3) * 18, y = 19 + (i / 3) * 18;
        map_tile(x, y, 16, map->tiles[i]);
        if (i == selected) focus_feature_box(focused, x - 1, y - 1, 18, 18, 5);
    }
    const PuzzleTile legend[] = {PUZZLE_MARBLE, PUZZLE_SANDSTONE, PUZZLE_FLOOR, PUZZLE_UNKNOWN, PUZZLE_PLAYER};
    const char *const names[] = {"Marble", "Sandstone", "Open floor", "Unseen", "You"};
    for (unsigned i = 0; i < 5; i++) {
        int y = 21 + i * 10;
        map_tile(166, y, 9, legend[i]);
        label(180, y + 1, names[i], CREAM);
    }
    draw_puzzle_special_targets(targets, focused);
    draw_puzzle_floor_items(targets, focused);
}

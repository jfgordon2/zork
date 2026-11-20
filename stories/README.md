# Infocom story files

The GBA build embeds the following unmodified Z-machine version 3 programs from Microsoft’s MIT-licensed release. The original `.zip` files for II and III are raw story programs that use the historical ZIP filename convention.

| Story | Release / serial | Upstream file |
| --- | --- | --- |
| `zork1.z3` | 119 / 880429 | [Historical Source](https://github.com/historicalsource/zork1/blob/master/COMPILED/zork1.z3) |
| `zork2.z3` | 63 / 860811 | [Historical Source](https://github.com/historicalsource/zork2/blob/master/zork2.zip) |
| `zork3.z3` | 25 / 860811 | [Historical Source](https://github.com/historicalsource/zork3/blob/master/zork3.zip) |

The [upstream MIT notice](../LICENSE-ZORK.txt) applies to the stories. The GBA interpreter incorporates an altered version of [MojoZork](https://github.com/icculus/mojozork) by Ryan C. Gordon. Its [zlib license](../vendor/mojozork/LICENSE.txt) is included with the source. The upstream `mojozork.c` used for this adaptation had SHA-256 `fdaff7424d3e35c12711ab87dfe6ed6a1b6f7c00e2d69790d0c737d3dc0db2a1`.

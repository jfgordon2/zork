from pathlib import Path
import sys

# Boot ROM verification data and header fields, as documented by GBATEK.
LOGO = bytes.fromhex(
    "24ffae51699aa2213d84820a84e409ad11248b98c0817f21a352be199309ce20"
    "10464a4af82731ec58c7e83382e3cebf85f4df94ce4b09c194568ac01372a7fc"
    "9f844d73a3ca9a615897a327fc039876231dc7610304ae56bf38840040a70efd"
    "ff52fe036f9530f197fbc08560d68025a963be03014e38e2f9a234ffbb3e0344"
    "780090cb88113a9465c07c6387f03cafd625e48b380aac7221d4f807"
)


def fix(path, trilogy=False):
    rom = bytearray(path.read_bytes())
    if len(rom) < 0xC0 or rom[3] != 0xEA:
        raise ValueError("missing GBA header and ARM entry branch")
    if len(LOGO) != 156:
        raise ValueError("invalid boot logo length")
    rom[4:0xA0] = LOGO
    rom[0xA0:0xAC] = b"ZORK TRILOGY" if trilogy else b"ZORK I DEMO\0"
    rom[0xAC:0xB0] = b"ZKTE" if trilogy else b"ZK1E"
    rom[0xB0:0xB2] = b"00"
    rom[0xB2] = 0x96
    rom[0xBD] = (-sum(rom[0xA0:0xBD]) - 0x19) & 0xFF
    path.write_bytes(rom)


if __name__ == "__main__":
    fix(Path(sys.argv[1]), trilogy=sys.argv[2:] == ["trilogy"])

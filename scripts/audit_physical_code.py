#!/usr/bin/env python3
"""Audit a GoldenEye PHYSICAL_CODE ELF/ROM for stale virtual-code assumptions.

The physical build relocates:
  retail .code    0x7000.... -> KSEG0 0x8000....
  retail .inflate 0x7020.... -> KSEG0 0x8020....
  retail .game    0x7F00.... -> KSEG0 0x8060....

This audit works on the linked ELF so symbol-resolved function-pointer tables are
checked before csegment compression.  It intentionally avoids raw whole-ROM
pattern matching, which produces false positives from compressed assets, floats,
colors, and unrelated data.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHT_SYMTAB = 2
SHT_NOBITS = 8
STT_FUNC = 2
SHN_UNDEF = 0

RETAIL_CRC1 = 0xDCBC50D1
RETAIL_CRC2 = 0x09FD1AA3
NATIVE_ROM_SIZE = 0x00C00000

# Runtime regions deliberately used by the physical architecture.
KSEG0_RDRAM_START = 0x80000000
KSEG0_RDRAM_END = 0x80800000
PHYS_GAME_START = 0x80600000
PHYS_GAME_LIMIT = 0x80800000
PHYS_INFLATE_START = 0x80200000
RETAIL_GAME_ROM_START = 0x00034B30
RETAIL_GAME_ROM_END_BASELINE = 0x00117880
RETAIL_DATA_START = 0x80020D90

# Exact legacy virtual bases/ranges whose presence in live physical runtime data
# is suspicious.  The ranges are deliberately narrow enough to avoid values
# such as 0x7F7FFFFF (max finite float) and unrelated 0x7... constants.
LEGACY_EXACT_WORDS = {
    0x70000000: "retail resident-code virtual base",
    0x70100000: "retail resident-code debug range end",
    0x70200000: "retail inflate virtual base",
    0x7F000000: "retail game-code virtual base",
    0x7F100000: "retail game-code debug range end",
}

# Known retail executable address windows.  These are also scanned directly,
# independent of the relocated symbol map.  That matters if physical-only
# stubs shrink an object and therefore move later symbols: a stale literal
# still points at the *retail* address even though a symbol-derived reverse
# mapping would now have a slightly different offset.
LEGACY_EXEC_RANGES = (
    (0x70000450, 0x70020D90, "retail resident code"),
    (0x70200000, 0x702015A0, "retail inflate code"),
    (0x7F000000, 0x7F100000, "retail game code"),
)


@dataclass
class Section:
    index: int
    name: str
    sh_type: int
    flags: int
    addr: int
    offset: int
    size: int
    link: int
    info: int
    addralign: int
    entsize: int

    @property
    def alloc(self) -> bool:
        return bool(self.flags & SHF_ALLOC)

    @property
    def executable(self) -> bool:
        return bool(self.flags & SHF_EXECINSTR)

    @property
    def writable(self) -> bool:
        return bool(self.flags & SHF_WRITE)


@dataclass
class Symbol:
    name: str
    value: int
    size: int
    info: int
    other: int
    shndx: int

    @property
    def sym_type(self) -> int:
        return self.info & 0xF


class Elf32:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if len(self.data) < 52 or self.data[:4] != b"\x7fELF":
            raise ValueError(f"{path}: not an ELF file")
        if self.data[4] != 1:
            raise ValueError(f"{path}: expected ELF32, found class {self.data[4]}")
        if self.data[5] == 2:
            self.endian = ">"
        elif self.data[5] == 1:
            self.endian = "<"
        else:
            raise ValueError(f"{path}: unsupported ELF data encoding {self.data[5]}")

        hdr = struct.unpack_from(self.endian + "16sHHIIIIIHHHHHH", self.data, 0)
        (
            _ident,
            self.e_type,
            self.e_machine,
            self.e_version,
            self.e_entry,
            self.e_phoff,
            self.e_shoff,
            self.e_flags,
            self.e_ehsize,
            self.e_phentsize,
            self.e_phnum,
            self.e_shentsize,
            self.e_shnum,
            self.e_shstrndx,
        ) = hdr

        if self.e_shentsize < 40:
            raise ValueError(f"{path}: invalid ELF32 section-header size {self.e_shentsize}")
        if self.e_shoff + self.e_shnum * self.e_shentsize > len(self.data):
            raise ValueError(f"{path}: truncated section-header table")

        raw_sections = []
        for i in range(self.e_shnum):
            off = self.e_shoff + i * self.e_shentsize
            raw_sections.append(struct.unpack_from(self.endian + "IIIIIIIIII", self.data, off))

        if not (0 <= self.e_shstrndx < len(raw_sections)):
            raise ValueError(f"{path}: invalid shstrndx {self.e_shstrndx}")
        shstr = raw_sections[self.e_shstrndx]
        shstr_data = self._slice(shstr[4], shstr[5], "section-name string table")

        self.sections: List[Section] = []
        for i, raw in enumerate(raw_sections):
            name_off, sh_type, flags, addr, off, size, link, info, align, entsize = raw
            name = self._cstring(shstr_data, name_off)
            self.sections.append(Section(i, name, sh_type, flags, addr, off, size, link, info, align, entsize))

        self.symbols: List[Symbol] = []
        for sec in self.sections:
            if sec.sh_type != SHT_SYMTAB or sec.entsize == 0:
                continue
            if not (0 <= sec.link < len(self.sections)):
                continue
            strsec = self.sections[sec.link]
            strdata = self.section_bytes(strsec)
            symdata = self.section_bytes(sec)
            count = len(symdata) // sec.entsize
            for i in range(count):
                off = i * sec.entsize
                if off + 16 > len(symdata):
                    break
                st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(
                    self.endian + "IIIBBH", symdata, off
                )
                self.symbols.append(
                    Symbol(self._cstring(strdata, st_name), st_value, st_size, st_info, st_other, st_shndx)
                )

    def _slice(self, off: int, size: int, what: str) -> bytes:
        if off < 0 or size < 0 or off + size > len(self.data):
            raise ValueError(f"{self.path}: truncated {what} (off=0x{off:X} size=0x{size:X})")
        return self.data[off : off + size]

    @staticmethod
    def _cstring(blob: bytes, off: int) -> str:
        if off < 0 or off >= len(blob):
            return ""
        end = blob.find(b"\0", off)
        if end < 0:
            end = len(blob)
        return blob[off:end].decode("utf-8", errors="replace")

    def section_bytes(self, sec: Section) -> bytes:
        if sec.sh_type == SHT_NOBITS or sec.size == 0:
            return b""
        return self._slice(sec.offset, sec.size, f"section {sec.name}")

    def symbols_by_name(self) -> Dict[str, Symbol]:
        # Prefer defined symbols and preserve the first defined occurrence.
        result: Dict[str, Symbol] = {}
        for sym in self.symbols:
            if not sym.name:
                continue
            old = result.get(sym.name)
            if old is None or (old.shndx == SHN_UNDEF and sym.shndx != SHN_UNDEF):
                result[sym.name] = sym
        return result


class Audit:
    def __init__(self):
        self.errors: List[str] = []
        self.warnings: List[str] = []
        self.notes: List[str] = []

    def error(self, msg: str) -> None:
        self.errors.append(msg)

    def warn(self, msg: str) -> None:
        self.warnings.append(msg)

    def note(self, msg: str) -> None:
        self.notes.append(msg)


def in_range(value: int, lo: int, hi: int) -> bool:
    return lo <= value < hi


def section_for_symbol(elf: Elf32, sym: Symbol) -> Optional[Section]:
    if 0 <= sym.shndx < len(elf.sections):
        return elf.sections[sym.shndx]
    return None


def old_runtime_address(sec: Section, value: int) -> Optional[int]:
    """Return the retail virtual address corresponding to a relocated text symbol."""
    if sec.name == ".game" and in_range(value, PHYS_GAME_START, PHYS_GAME_LIMIT):
        return (value - 0x01600000) & 0xFFFFFFFF
    if sec.name in (".code", ".inflate") and in_range(value, 0x80000000, 0x80400000):
        return (value - 0x10000000) & 0xFFFFFFFF
    return None


def u32_words(blob: bytes, endian: str) -> Iterable[Tuple[int, int]]:
    fmt = endian + "I"
    for off in range(0, len(blob) - 3, 4):
        yield off, struct.unpack_from(fmt, blob, off)[0]


def sign16(x: int) -> int:
    return x - 0x10000 if x & 0x8000 else x


def legacy_exec_range(value: int) -> Optional[str]:
    for lo, hi, label in LEGACY_EXEC_RANGES:
        if lo <= value < hi:
            return label
    return None


def reg_written_by(word: int) -> Optional[int]:
    op = (word >> 26) & 0x3F
    rt = (word >> 16) & 0x1F
    rd = (word >> 11) & 0x1F
    if op == 0:
        funct = word & 0x3F
        # Common SPECIAL instructions that write rd; jr/jalr handled conservatively.
        if funct in {
            0x00, 0x02, 0x03, 0x04, 0x06, 0x07, 0x09, 0x10, 0x12, 0x20, 0x21,
            0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2A, 0x2B,
        }:
            return rd
        return None
    if op in {0x02, 0x03, 0x04, 0x05, 0x06, 0x07}:
        return None
    if op == 0x01:  # REGIMM branches normally do not write GPRs (link forms write ra).
        rt_field = rt
        if rt_field in {0x10, 0x11, 0x12, 0x13}:
            return 31
        return None
    # Loads/immediates/cop moves generally write rt. Stores do not.
    if op in {0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x30, 0x38, 0x3A, 0x3E}:
        return None
    if op in {0x10, 0x11, 0x12, 0x13}:
        # Coprocessor encoding is varied; avoid pretending all write rt.
        return None
    return rt



def reg_read_by(word: int, reg: int) -> bool:
    op = (word >> 26) & 0x3F
    rs = (word >> 21) & 0x1F
    rt = (word >> 16) & 0x1F
    if op == 0x0F:  # LUI
        return False
    if op == 0:  # SPECIAL
        funct = word & 0x3F
        if funct in {0x00, 0x02, 0x03}:  # fixed shifts read rt only
            return rt == reg
        if funct in {0x08, 0x09}:  # JR/JALR read rs
            return rs == reg
        return rs == reg or rt == reg
    if op in {0x02, 0x03}:  # J/JAL read no GPR operand
        return False
    if op == 0x01:  # REGIMM branch reads rs
        return rs == reg
    if op in {0x04, 0x05}:  # BEQ/BNE read both
        return rs == reg or rt == reg
    if op in {0x06, 0x07}:  # BLEZ/BGTZ read rs
        return rs == reg
    if op in {0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x30, 0x38, 0x3A, 0x3E}:  # stores/cache-like
        return rs == reg or rt == reg
    # Immediate ALU and loads read rs; rt is the destination.
    return rs == reg

def audit_symbols_and_layout(elf: Elf32, audit: Audit) -> Dict[str, Symbol]:
    syms = elf.symbols_by_name()

    required_exact = {
        "_gameSegmentStart": PHYS_GAME_START,
        "_inflateSegmentStart": PHYS_INFLATE_START,
        "__dataSegmentVaddrStart": RETAIL_DATA_START,
        "_gameSegmentRomStart": RETAIL_GAME_ROM_START,
    }
    for name, expected in required_exact.items():
        sym = syms.get(name)
        if sym is None or sym.shndx == SHN_UNDEF:
            audit.error(f"missing required linker symbol {name}")
        elif sym.value != expected:
            audit.error(f"{name}=0x{sym.value:08X}, expected 0x{expected:08X}")

    def getv(name: str) -> Optional[int]:
        sym = syms.get(name)
        return None if sym is None or sym.shndx == SHN_UNDEF else sym.value

    game_end = getv("_gameSegmentEnd")
    if game_end is None:
        audit.error("missing required linker symbol _gameSegmentEnd")
    elif not (PHYS_GAME_START < game_end <= PHYS_GAME_LIMIT):
        audit.error(f"_gameSegmentEnd=0x{game_end:08X} is outside Expansion Pak game-code reservation")
    else:
        audit.note(f"game text: 0x{PHYS_GAME_START:08X}..0x{game_end:08X} ({game_end-PHYS_GAME_START:#x} bytes)")

    code_start = getv("_codeSegmentStart")
    code_end = getv("_codeSegmentEnd")
    if code_start is None or code_end is None:
        audit.error("missing _codeSegmentStart/_codeSegmentEnd")
    else:
        if not in_range(code_start, 0x80000000, RETAIL_DATA_START):
            audit.error(f"_codeSegmentStart=0x{code_start:08X} is not in resident KSEG0")
        if code_end > RETAIL_DATA_START:
            audit.error(f"_codeSegmentEnd=0x{code_end:08X} overlaps retail data start 0x{RETAIL_DATA_START:08X}")
        audit.note(f"resident code: 0x{code_start:08X}..0x{code_end:08X}")

    inflate_end = getv("_inflateSegmentEnd")
    if inflate_end is None:
        audit.error("missing _inflateSegmentEnd")
    elif not (PHYS_INFLATE_START < inflate_end < 0x80300000):
        audit.error(f"_inflateSegmentEnd=0x{inflate_end:08X} is outside expected KSEG0 inflate area")
    else:
        audit.note(f"inflate: 0x{PHYS_INFLATE_START:08X}..0x{inflate_end:08X}")

    bss_end = getv("_bssSegmentEnd")
    if bss_end is not None and bss_end >= PHYS_GAME_START:
        audit.error(f"_bssSegmentEnd=0x{bss_end:08X} overlaps physical game text")
    cfb_end = getv("_cfbSegmentEnd")
    if cfb_end is not None and cfb_end > PHYS_GAME_START:
        audit.error(f"_cfbSegmentEnd=0x{cfb_end:08X} overlaps physical game text")

    game_rom_end = getv("_gameSegmentRomEnd")
    if game_rom_end is not None:
        if game_rom_end == RETAIL_GAME_ROM_END_BASELINE:
            audit.note(f"game ROM end still matches baseline: 0x{game_rom_end:08X}")
        else:
            audit.warn(
                f"_gameSegmentRomEnd=0x{game_rom_end:08X} differs from physical baseline "
                f"0x{RETAIL_GAME_ROM_END_BASELINE:08X}; verify downstream ROM offsets intentionally moved"
            )

    # No live function may remain in the old demand-paged/virtual execution ranges.
    for sym in elf.symbols:
        if sym.shndx == SHN_UNDEF or sym.sym_type != STT_FUNC or sym.value == 0:
            continue
        if in_range(sym.value, 0x7F000000, 0x80000000):
            audit.error(f"live function symbol remains in 0x7F virtual range: {sym.name}=0x{sym.value:08X}")
        if in_range(sym.value, 0x70000000, 0x70300000):
            audit.error(f"live function symbol remains in old 0x70/0x702 virtual range: {sym.name}=0x{sym.value:08X}")

    # Any alloc+executable output section itself in legacy virtual space is a hard failure.
    for sec in elf.sections:
        if not (sec.alloc and sec.executable and sec.size):
            continue
        end = sec.addr + sec.size
        if max(sec.addr, 0x7F000000) < min(end, 0x80000000):
            audit.error(f"executable section {sec.name} still occupies 0x7F virtual space: 0x{sec.addr:08X}..0x{end:08X}")
        if max(sec.addr, 0x70000000) < min(end, 0x70300000):
            audit.error(f"executable section {sec.name} still occupies old 0x70/0x702 virtual space: 0x{sec.addr:08X}..0x{end:08X}")

    return syms


def audit_stale_pointer_words(elf: Elf32, audit: Audit) -> None:
    syms_by_name = elf.symbols_by_name()
    non_cpu_ranges = []
    for start_name, end_name in (
        ("rspbootTextStart", "rspbootTextEnd"),
        ("gsp3DTextStart", "gsp3DTextEnd"),
        ("aspMainTextStart", "aspMainTextEnd"),
        ("gsp3DDataStart", "gsp3DDataEnd"),
        ("aspMainDataStart", "aspMainDataEnd"),
    ):
        a = syms_by_name.get(start_name)
        b = syms_by_name.get(end_name)
        if a is not None and b is not None and a.value < b.value:
            non_cpu_ranges.append((a.value, b.value, start_name))

    def is_non_cpu_runtime_word(addr: int) -> bool:
        return any(lo <= addr < hi for lo, hi, _ in non_cpu_ranges)

    # Map every symbol in relocated executable sections back to its retail virtual
    # address. Exact matches catch stale function pointers. Do not reject arbitrary
    # 0x7Fxxxxxx data words: GoldenEye's sine/audio tables and bytecode streams
    # naturally contain many such packed values. Hardcoded literals are covered by
    # audit_physical_source.py; this linked pass is for exact resolved addresses.
    old_to_symbols: Dict[int, List[str]] = {}
    for sym in elf.symbols:
        if sym.shndx == SHN_UNDEF or sym.value == 0:
            continue
        sec = section_for_symbol(elf, sym)
        if sec is None or not sec.executable:
            continue
        old = old_runtime_address(sec, sym.value)
        if old is not None:
            old_to_symbols.setdefault(old, []).append(sym.name or f"<sym@0x{sym.value:08X}>")

    hits = []
    exact_hits = []
    for sec in elf.sections:
        if not (sec.alloc and sec.size) or sec.sh_type == SHT_NOBITS:
            continue
        # .csegment is CPU runtime data, but the current linker script/LLD
        # representation marks the aggregate output WAX because it also carries
        # copied microcode blobs.  Treat it as data for pointer auditing.
        if sec.executable and sec.name != ".csegment":
            continue
        # Only inspect live CPU runtime data.  ROM-only payload/output sections
        # (cdata, assets, compressed blobs) can coincidentally contain these
        # byte patterns and are not dereferenced as pointers at their ELF VMA.
        if not in_range(sec.addr, KSEG0_RDRAM_START, KSEG0_RDRAM_END):
            continue
        blob = elf.section_bytes(sec)
        for off, word in u32_words(blob, elf.endian):
            runtime_addr = sec.addr + off
            if is_non_cpu_runtime_word(runtime_addr):
                continue
            if word in old_to_symbols:
                # libultra's static AL equal-power envelope table contains the
                # adjacent signed-16 coefficients 0x7F05,0x7ED0. When aligned
                # inside the aggregate .csegment these four bytes happen to
                # equal the retail address 0x7F057ED0 (getposstan). This is
                # numeric audio data, not a pointer. Require the neighboring
                # coefficients as a signature so we do not globally suppress
                # a genuine stale pointer with the same value.
                is_eqpower_coeff_pair = False
                if sec.name == ".csegment" and word == 0x7F057ED0 and off >= 4 and off + 8 <= len(blob):
                    is_eqpower_coeff_pair = (
                        blob[off - 4:off] == bytes.fromhex("7f5f7f34")
                        and blob[off + 4:off + 8] == bytes.fromhex("7e977e58")
                    )
                if not is_eqpower_coeff_pair:
                    names = ", ".join(sorted(set(old_to_symbols[word]))[:4])
                    hits.append((sec, off, word, names))
            if word in LEGACY_EXACT_WORDS:
                # A printable NUL-terminated UI string can coincidentally form
                # 0x70000000 when its final ASCII byte is 0x70 ('p') followed
                # by linker padding.  R7's literal "Co-Op" is one such case.
                # Do not classify that byte pattern as a live CPU pointer.
                is_ascii_string_tail = False
                if word == 0x70000000 and off >= 3:
                    prev = blob[off - 3:off + 1]
                    foll = blob[off + 1:off + 4]
                    if all(0x20 <= b <= 0x7e for b in prev) and foll == b"\x00\x00\x00":
                        is_ascii_string_tail = True
                if not is_ascii_string_tail:
                    exact_hits.append((sec, off, word, LEGACY_EXACT_WORDS[word]))

    seen = set()
    for sec, off, word, names in hits:
        key = (sec.index, off, word)
        if key in seen:
            continue
        seen.add(key)
        audit.error(
            f"stale relocated executable pointer 0x{word:08X} in {sec.name}+0x{off:X} "
            f"(matches retail address of {names})"
        )

    for sec, off, word, desc in exact_hits:
        key = (sec.index, off, word)
        if key in seen:
            continue
        seen.add(key)
        audit.error(f"legacy virtual-address word 0x{word:08X} ({desc}) in live {sec.name}+0x{off:X}")


def audit_executable_instructions(elf: Elf32, audit: Audit) -> None:
    # Decode only real CPU function ranges, not every byte in an output section.
    # Some aggregate linker sections (notably .csegment, and small tails of
    # .inflate) contain data/microcode while carrying SHF_EXECINSTR. Treating
    # arbitrary data words as MIPS instructions creates hundreds of fake J/JALs.
    cpu_text_sections = {".start", ".code", ".inflate", ".game"}

    stale_symbol_values = set()
    stale_symbol_names: Dict[int, List[str]] = {}
    for sym in elf.symbols:
        if sym.shndx == SHN_UNDEF or sym.value == 0:
            continue
        sec = section_for_symbol(elf, sym)
        if sec is None or sec.name not in cpu_text_sections:
            continue
        old = old_runtime_address(sec, sym.value)
        if old is not None:
            stale_symbol_values.add(old)
            stale_symbol_names.setdefault(old, []).append(sym.name)

    # Use sized STT_FUNC symbols as authoritative instruction ranges. Hand-written
    # assembly literals are independently covered by the source preflight.
    funcs: List[Tuple[Section, Symbol]] = []
    for sym in elf.symbols:
        if sym.sym_type != STT_FUNC or sym.shndx == SHN_UNDEF or sym.size <= 0:
            continue
        sec = section_for_symbol(elf, sym)
        if sec is None or sec.name not in cpu_text_sections:
            continue
        if not in_range(sym.value, KSEG0_RDRAM_START, KSEG0_RDRAM_END):
            continue
        funcs.append((sec, sym))

    seen_ranges = set()
    for sec, sym in funcs:
        key = (sec.index, sym.value, sym.size)
        if key in seen_ranges:
            continue
        seen_ranges.add(key)
        start_off = sym.value - sec.addr
        if start_off < 0 or start_off + sym.size > sec.size:
            audit.error(f"function {sym.name} range escapes section {sec.name}")
            continue
        blob = elf.section_bytes(sec)[start_off:start_off + sym.size]
        words = [word for _, word in u32_words(blob, elf.endian)]
        for i, word in enumerate(words):
            pc = sym.value + i * 4
            op = (word >> 26) & 0x3F
            if op in (0x02, 0x03):  # J / JAL
                target = ((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
                if not in_range(target, KSEG0_RDRAM_START, KSEG0_RDRAM_END):
                    audit.error(
                        f"direct {'JAL' if op == 3 else 'J'} in {sym.name}+0x{i*4:X} "
                        f"(PC=0x{pc:08X}) targets 0x{target:08X}, outside 8 MiB KSEG0 RDRAM"
                    )

            if op != 0x0F:  # LUI
                continue
            rt = (word >> 16) & 0x1F
            hi = word & 0xFFFF
            base_only = (hi << 16) & 0xFFFFFFFF
            for j in range(i + 1, min(i + 7, len(words))):
                w2 = words[j]
                op2 = (w2 >> 26) & 0x3F
                rs2 = (w2 >> 21) & 0x1F
                rt2 = (w2 >> 16) & 0x1F
                if op2 in (0x0D, 0x09) and rs2 == rt and rt2 == rt:
                    lo = w2 & 0xFFFF
                    value = ((hi << 16) | lo) & 0xFFFFFFFF if op2 == 0x0D else ((hi << 16) + sign16(lo)) & 0xFFFFFFFF
                    if value in stale_symbol_values:
                        names = ", ".join(sorted(set(stale_symbol_names[value]))[:4])
                        audit.error(
                            f"stale retail executable address 0x{value:08X} materialized in "
                            f"{sym.name}+0x{i*4:X}..0x{j*4:X} (matches {names})"
                        )
                    elif value in LEGACY_EXACT_WORDS:
                        audit.error(
                            f"legacy virtual address 0x{value:08X} materialized in "
                            f"{sym.name}+0x{i*4:X}..0x{j*4:X} ({LEGACY_EXACT_WORDS[value]})"
                        )
                    else:
                        legacy_range = legacy_exec_range(value)
                        if legacy_range is not None:
                            audit.error(
                                f"legacy executable address 0x{value:08X} materialized in "
                                f"{sym.name}+0x{i*4:X}..0x{j*4:X} ({legacy_range})"
                            )
                    break
                if base_only in LEGACY_EXACT_WORDS and reg_read_by(w2, rt):
                    audit.error(
                        f"legacy virtual base 0x{base_only:08X} materialized/used in "
                        f"{sym.name}+0x{i*4:X}..0x{j*4:X} ({LEGACY_EXACT_WORDS[base_only]})"
                    )
                    break
                if reg_written_by(w2) == rt:
                    break

    audit.note(f"decoded {len(seen_ranges)} sized CPU function range(s) for J/JAL/address checks")

def audit_rom(path: Path, audit: Audit) -> None:
    data = path.read_bytes()
    if len(data) != NATIVE_ROM_SIZE:
        audit.error(f"ROM size is 0x{len(data):X}; expected native 12 MiB (0x{NATIVE_ROM_SIZE:X})")
    else:
        audit.note("ROM size: 12 MiB (0xC00000)")

    if len(data) >= 0x18:
        crc1, crc2 = struct.unpack_from(">II", data, 0x10)
        audit.note(f"ROM CRC1/CRC2: {crc1:08X}-{crc2:08X}")
        if (crc1, crc2) == (RETAIL_CRC1, RETAIL_CRC2):
            audit.error("physical ROM still carries the retail GoldenEye CRC1/CRC2 identity")
        if crc1 == 0 and crc2 == 0:
            audit.error("ROM CRC1/CRC2 are both zero")

    audit.note(f"ROM SHA-1: {hashlib.sha1(data).hexdigest()}")
    audit.note(f"ROM SHA-256: {hashlib.sha256(data).hexdigest()}")


def print_report(audit: Audit) -> None:
    print("GoldenEye PHYSICAL_CODE audit")
    print("=" * 30)
    for msg in audit.notes:
        print(f"[ OK ] {msg}")
    for msg in audit.warnings:
        print(f"[WARN] {msg}")
    for msg in audit.errors:
        print(f"[FAIL] {msg}")
    if audit.errors:
        print(f"\nRESULT: FAIL ({len(audit.errors)} error(s), {len(audit.warnings)} warning(s))")
    else:
        print(f"\nRESULT: PASS ({len(audit.warnings)} warning(s))")


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True, type=Path, help="linked PHYSICAL_CODE ELF")
    ap.add_argument("--rom", type=Path, help="final checksummed .z64 (optional)")
    args = ap.parse_args(argv)

    audit = Audit()
    try:
        elf = Elf32(args.elf)
    except (OSError, ValueError) as exc:
        print(f"audit_physical_code.py: {exc}", file=sys.stderr)
        return 2

    if elf.endian != ">":
        audit.error("physical N64 ELF is not big-endian")
    audit.note(f"ELF: {args.elf}")
    audit.note(f"ELF entry: 0x{elf.e_entry:08X}")

    audit_symbols_and_layout(elf, audit)
    audit_stale_pointer_words(elf, audit)
    audit_executable_instructions(elf, audit)

    if args.rom is not None:
        if not args.rom.is_file():
            audit.error(f"ROM not found: {args.rom}")
        else:
            audit_rom(args.rom, audit)

    print_report(audit)
    return 1 if audit.errors else 0


if __name__ == "__main__":
    raise SystemExit(main())

# rom2msx
CLI-converter to convert a MSX-ROM to match one of the following mappers:
- **MegaSCC** (Konami SCC-mapper) – *default*
- **ESE-RC755**
- **Simple64K** (med `--addr` startblok som i originalen)

The output is a binary file ready to be burned on a SST39SF0x0 EEPROM for a flash cart such as Spider Flash

The project is **heavily inspired by the wrtsst.com tool by HRA!** but where wrtsst.com runs on the MSX hardware and writes directy to the flashcart, this program can be used on a linux/unix/mac/pc to prepare the chip before burning it.

## Build
```bash
	g++ -O2 -std=c++17 -o rom2msx rom2msx.cpp
	
	or use the include Makefile
	Make
```

## Usage
```bash
./rom2msx input.rom output.bin [--chip 64|128|256|512] [--type mega|rc755|s64k] [--addr 0..7] [--verify]
```
Defaults: `--chip 128` (SST39SF010), `--type mega`.
For use with Spider Flash remember to specify ""--type s64k"

### Examples
```bash
# MegaSCC (default), chip = 128 KiB
./rom2msx game.rom game_128k.bin

# MegaSCC to 512 KiB
./rom2msx game.rom game_512k.bin --chip 512

# ESE-RC755
./rom2msx game.rom rc755.bin --type rc755

# Simple64K, auto start bank (2 ved ≤32 KiB otherwise 0)
./rom2msx small.rom s64k.bin --type s64k

# Simple64K with start block 2 (0..7)
./rom2msx small.rom s64k.bin --type s64k --addr 2

# verify that the output matches the conversion and the has been padded with 0xFF
./rom2msx game.rom out.bin --verify
```

## Mapping rules (mirrors those set by `wrtsst`)
- **Bank size:** 8 KiB.
- **MegaSCC:** start bank = 0; banks are written in sequence (0,1,2,…).
- **RC755:** start bank = 0; linear 8 KiB placement.
- **Simple64K:**
  - Without `--addr`: start bank = 2 at ROM ≤32 KiB, otherwise 0.
  - With `--addr 0..7`: start bank = addr; Requires: `addr + no_of_banks ≤ 8`.
  - Only the first 64 KiB (8 banks) are used; the rest is 0xFF.
- **Padding:** Input is rounded to the nearest 8 KiB with `0xFF`. Output buffer is equal to the chosen chip size and is filled with `0xFF`; only the relevant banks are overwritten.

## Noter
- `--verify` opens the generated bin.file and checks:
  1) the the written 8 KiB banks matches input, and
  2) that everything outside the written area is 0xFF.
- The program performs **no** patching of the ROM data (on checksums or changes to the code).

# rom2msx

CLI-konverter til at lægge en MSX-ROM i det layout, som originale `wrtsst` bruger på flash for:
- **MegaSCC** (Konami SCC-mapper) – *default*
- **ESE-RC755**
- **Simple64K** (med `--addr` startblok som i originalen)

Output er en binær fil klar til at blive brændt på en flashchip (f.eks. SST39SF010).

## Build
```bash
g++ -O2 -std=c++17 -o rom2msx rom2msx.cpp
```

## Brug
```bash
./rom2msx input.rom output.bin [--chip 64|128|256|512] [--type mega|rc755|s64k] [--addr 0..7] [--verify]
```
Defaults: `--chip 128` (SST39SF010), `--type mega`.

### Eksempler
```bash
# MegaSCC (default), chip = 128 KiB
./rom2msx game.rom game_128k.bin

# MegaSCC til 512 KiB
./rom2msx game.rom game_512k.bin --chip 512

# ESE-RC755
./rom2msx game.rom rc755.bin --type rc755

# Simple64K, auto startbank (2 ved ≤32 KiB ellers 0)
./rom2msx small.rom s64k.bin --type s64k

# Simple64K med startblok 2 (0..7)
./rom2msx small.rom s64k.bin --type s64k --addr 2

# Verificér at output svarer til konverteringen og at resten er 0xFF
./rom2msx game.rom out.bin --verify
```

## Mappingregler (spejler `wrtsst`)
- **Bankstørrelse:** 8 KiB.
- **MegaSCC:** startbank = 0; bankene skrives sekventielt (0,1,2,…). Bankvinduer i `wrtsst` sættes til (bank0=N, bank2=1, bank3=6), men offline er effekten blot lineær 8 KiB placering.
- **RC755:** startbank = 0; lineær 8 KiB placering.
- **Simple64K:**
  - Uden `--addr`: startbank = 2 ved ROM ≤32 KiB, ellers 0.
  - Med `--addr 0..7`: startbank = addr; krav: `addr + antal_banke ≤ 8`.
  - Kun de første 64 KiB (8 banker) bruges; resten 0xFF.
- **Padding:** Input rundes op til nærmeste 8 KiB med `0xFF`. Output-bufferen svarer til valgt chipstørrelse og fyldes med `0xFF`; kun de relevante banker overskrives.

## Noter
- `--verify` åbner den genererede fil og tjekker:
  1) at de skrevne 8 KiB-banke matcher input, og
  2) at alt uden for det skrevne område er `0xFF`.
- Programmet foretager **ingen** patching af ROM-data (ingen checksums eller kodeændringer). Det afspejler `wrtsst`’s adfærd før selve flash-skrivningen.

// rom2msx.cpp
// Convert an MSX ROM to the on-flash layout used by:
//   - MegaSCC (Konami SCC mapper)  [default]
//   - ESE-RC755
//   - Simple64K (with optional start address block)
// Output is a binary image you can program directly to a flash chip.
// Default chip: SST39SF010 (128 KiB).
//
// Build: g++ -O2 -std=c++17 -o rom2msx rom2msx.cpp
//
// Usage:
//   rom2msx input.rom output.bin [--chip 64|128|256|512]
//                               [--type mega|rc755|s64k]
//                               [--addr 0..7]        (only for --type s64k)
//
// Mapping rules (mirrors wrtsst logic):
// - MegaSCC: start bank = 0, 8 KiB banks written sequentially.
// - RC755:   start bank = 0, 8 KiB banks written sequentially.
// - Simple64K:
//     * If --addr not given: start bank = 2 for <=32 KiB ROM; otherwise start bank = 0.
//     * If --addr is given (0..7): start bank = addr, but must fit within 8 banks
//       (i.e., addr + ceil(size/8 KiB) <= 8).
//     * Only the first 64 KiB window is used; rest of chip remains 0xFF.
//
// General rules:
// - Bank size = 8 KiB.
// - Input is padded up to next 8 KiB with 0xFF if needed.
// - Output is sized to the selected chip (64/128/256/512 KiB), filled with 0xFF,
//   then banks are placed per mapping.
//
// License: MIT (to align with upstream's permissive license intention).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

static void die(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    std::exit(1);
}

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) die("Cannot open input file: " + path);
    f.seekg(0, std::ios::end);
    std::streamoff len = f.tellg();
    if (len < 0) die("tellg failed for input file");
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<size_t>(len));
    if (len > 0) {
        f.read(reinterpret_cast<char*>(buf.data()), len);
        if (!f) die("Failed to read input file fully");
    }
    return buf;
}

static void write_file(const std::string& path, const std::vector<uint8_t>& buf) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) die("Cannot open output file: " + path);
    f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    if (!f) die("Failed to write output file");
}

enum class CartType { MegaSCC, RC755, Simple64K };

int main(int argc, char** argv) {
    bool verify = false;
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " input.rom output.bin [--chip 64|128|256|512] [--type mega|rc755|s64k] [--addr 0..7]\n";
        std::cerr << "Defaults: --chip 128 (SST39SF010), --type mega\n";
        return 1;
    }

    std::string in_path = argv[1];
    std::string out_path = argv[2];
    int chip_kib = 128; // default SST39SF010
    CartType type = CartType::MegaSCC;
    int s64k_addr = -1; // optional start block

    // Parse options
    for (int i = 3; i < argc; ++i) {
        std::string opt = argv[i];
        if (opt == "--chip") {
            if (i + 1 >= argc) die("--chip requires a value");
            chip_kib = std::atoi(argv[++i]);
            if (!(chip_kib == 64 || chip_kib == 128 || chip_kib == 256 || chip_kib == 512)) {
                die("Unsupported --chip value (use 64, 128, 256, or 512)");
            }
        } else if (opt == "--type") {
            if (i + 1 >= argc) die("--type requires a value");
            std::string v = argv[++i];
            if (v == "mega" || v == "scc" || v == "megascc") type = CartType::MegaSCC;
            else if (v == "rc755") type = CartType::RC755;
            else if (v == "s64k" || v == "simple64k") type = CartType::Simple64K;
            else die("Unknown --type value (use mega|rc755|s64k)");
        } else if (opt == "--addr") {
            if (i + 1 >= argc) die("--addr requires a value 0..7");
            s64k_addr = std::atoi(argv[++i]);
            if (s64k_addr < 0 || s64k_addr > 7) die("--addr must be 0..7");
        } else if (opt == "--verify") {
            verify = true;
        } else {
            die(std::string("Unknown option: ") + opt);
        }
    }

    constexpr size_t BANK_SIZE = 0x2000; // 8 KiB
    size_t chip_bytes = static_cast<size_t>(chip_kib) * 1024ULL;

    auto rom = read_file(in_path);

    // Pad input ROM up to multiple of 8 KiB with 0xFF.
    if (rom.size() % BANK_SIZE != 0) {
        size_t padded = ((rom.size() + BANK_SIZE - 1) / BANK_SIZE) * BANK_SIZE;
        rom.resize(padded, 0xFF);
    }
    size_t in_banks = rom.size() / BANK_SIZE;

    // Simple64K has a hard limit of 64 KiB window (8 banks).
    if (type == CartType::Simple64K) {
        if (in_banks > 8) {
            die("ROM too large for Simple64K (max 64 KiB)");
        }
    }

    if (rom.size() > chip_bytes) {
        die("Input ROM (after 8 KiB padding) is larger than selected chip size");
    }

    // Prepare output buffer filled with 0xFF (erased state)
    std::vector<uint8_t> out(chip_bytes, 0xFF);

    // Compute placement
    size_t start_bank = 0;

    switch (type) {
        case CartType::MegaSCC:
            // Start bank = 0 always.
            start_bank = 0;
            break;
        case CartType::RC755:
            // Start bank = 0.
            start_bank = 0;
            break;
        case CartType::Simple64K: {
            if (s64k_addr >= 0) {
                // Use requested block; must fit in 8 banks.
                if (static_cast<size_t>(s64k_addr) + in_banks > 8) {
                    die("Simple64K: --addr + bank_count exceeds 8 banks");
                }
                start_bank = static_cast<size_t>(s64k_addr);
            } else {
                // Auto selection: <=32 KiB -> start at block 2 (0x4000), else block 0 (0x0000).
                size_t size_kib = rom.size() / 1024;
                if (size_kib <= 32) start_bank = 2;
                else start_bank = 0;
                // Sanity: ensure it fits.
                if (start_bank + in_banks > 8) {
                    die("Simple64K: auto start doesn't fit; try a smaller ROM or pass --addr");
                }
            }
            break;
        }
    }

    // Place banks
    for (size_t bank = 0; bank < in_banks; ++bank) {
        size_t dst_bank = start_bank + bank;
        size_t dst_off = dst_bank * BANK_SIZE;
        size_t src_off = bank * BANK_SIZE;
        if (dst_off + BANK_SIZE > out.size()) {
            die("Output overflow: bank placement exceeds selected chip size");
        }
        std::memcpy(out.data() + dst_off, rom.data() + src_off, BANK_SIZE);
    }

    write_file(out_path, out);

    // Report
    std::cout << "Type: ";
    switch (type) {
        case CartType::MegaSCC:   std::cout << "MegaSCC"; break;
        case CartType::RC755:     std::cout << "RC755"; break;
        case CartType::Simple64K: std::cout << "Simple64K"; break;
    }
    std::cout << ", chip: " << chip_kib << " KiB, banks written: " << in_banks
              << ", start bank: " << start_bank << ", bank size: 8 KiB";
    if (verify) {
        // Validate: check that written banks match input and the rest is 0xFF
        std::ifstream vf(out_path, std::ios::binary);
        vf.seekg(0, std::ios::end);
        size_t vlen = static_cast<size_t>(vf.tellg());
        vf.seekg(0, std::ios::beg);
        std::vector<uint8_t> vb(vlen);
        vf.read(reinterpret_cast<char*>(vb.data()), vlen);
        if (!vf) die("--verify: failed to read output back");
        // Check banks
        for (size_t bank = 0; bank < in_banks; ++bank) {
            size_t dst_off = (start_bank + bank) * BANK_SIZE;
            for (size_t i = 0; i < BANK_SIZE; ++i) {
                if (vb[dst_off + i] != rom[bank * BANK_SIZE + i]) {
                    die("--verify: mismatch in bank " + std::to_string(bank));
                }
            }
        }
        // Check 0xFF elsewhere
        for (size_t i = 0; i < vb.size(); ++i) {
            bool within_written = false;
            size_t bank_idx = i / BANK_SIZE;
            if (bank_idx >= start_bank && bank_idx < start_bank + in_banks) within_written = true;
            if (!within_written && vb[i] != 0xFF) die("--verify: non-0xFF found outside written area");
        }
        std::cout << "; verify: OK";
    }
    std::cout << "\n";
    return 0;
}

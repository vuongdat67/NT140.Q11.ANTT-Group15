#include "filevault/cli/commands/list_cmd.hpp"
#include "filevault/utils/console.hpp"
#include <fmt/core.h>

namespace filevault {
namespace cli {

ListCommand::ListCommand(core::CryptoEngine& engine)
    : engine_(engine) {
}

void ListCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name(), description());
    cmd->callback([this]() { execute(); });
}

int ListCommand::execute() {
    utils::Console::header("FileVault - Available Algorithms");
    
    // Symmetric Encryption
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), 
              "Symmetric Encryption Algorithms (AEAD)\n");
    fmt::print("{}\n", std::string(80, '-'));
    
    fmt::print("  Algorithm          Key Size    Security    Speed    Notes\n");
    fmt::print("  ----------------   --------    --------    -----    ------------------\n");
    fmt::print("  AES-128-GCM        128-bit     Good        ****     Fast, NIST standard\n");
    fmt::print("  AES-192-GCM        192-bit     Strong      ***      Balanced\n");
    fmt::print("  AES-256-GCM        256-bit     Maximum     ***      Recommended\n");
    fmt::print("  ChaCha20-Poly1305  256-bit     Maximum     ****     SW-optimized\n");
    fmt::print("  Serpent-256-GCM    256-bit     Maximum     **       AES finalist\n");
    fmt::print("  Twofish-128-GCM    128-bit     Good        ***      AES finalist\n");
    fmt::print("  Twofish-256-GCM    256-bit     Maximum     ***      AES finalist\n\n");
    
    // International Standards
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::magenta), 
              "International Standard Ciphers\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  Algorithm          Key Size    Origin      Standard\n");
    fmt::print("  ----------------   --------    ---------   ----------------------\n");
    fmt::print("  Camellia-128-GCM   128-bit     Japan       ISO/IEC 18033-3\n");
    fmt::print("  Camellia-256-GCM   256-bit     Japan       CRYPTREC, NESSIE\n");
    fmt::print("  ARIA-128-GCM       128-bit     Korea       KS X 1213, RFC 5794\n");
    fmt::print("  ARIA-256-GCM       256-bit     Korea       ISO/IEC 18033-3\n");
    fmt::print("  SM4-GCM            128-bit     China       GB/T 32907-2016\n\n");

    // Classical Ciphers (Educational)
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::yellow), 
              "Classical Ciphers (Educational Only)\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  WARNING: These are INSECURE and for EDUCATIONAL purposes only!\n\n");
    fmt::print("  Cipher      Type              Attack Method    Note\n");
    fmt::print("  --------    ---------------   --------------   ---------------------\n");
    fmt::print("  Caesar      Shift             Brute-force      Only 26 possible keys\n");
    fmt::print("  Vigenere    Polyalphabetic    Kasiski exam     Repeated key weakness\n");
    fmt::print("  Playfair    Digraph           Frequency        600 digraphs\n");
    fmt::print("  Hill        Matrix            Known-plaintext  Linear algebra attack\n");
    fmt::print("  Substitution Monoalphabetic  Frequency        26! permutations\n\n");
    
    // KDF
    fmt::print("Key Derivation Functions\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  KDF              Type          Resistance      Speed    Note\n");
    fmt::print("  --------------   -----------   -------------   -----    ---------------\n");
    fmt::print("  Argon2id         Memory-hard   GPU/ASIC        Slow     Recommended\n");
    fmt::print("  Argon2i          Memory-hard   Side-channel    Slow     Cache-safe\n");
    fmt::print("  PBKDF2-SHA256    Standard      Basic           Fast     Legacy support\n");
    fmt::print("  PBKDF2-SHA512    Standard      Basic           Fast     Stronger\n");
    fmt::print("  scrypt           Memory-hard   GPU/ASIC        Slow     Legacy\n\n");
    
    // Hash Functions
    fmt::print("Hash Functions\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  Algorithm    Output     Security    Speed    Note\n");
    fmt::print("  ----------   --------   --------    -----    --------------\n");
    fmt::print("  SHA-256      256-bit    Strong      **       Standard\n");
    fmt::print("  SHA-512      512-bit    Maximum     *        Stronger\n");
    fmt::print("  BLAKE2b      512-bit    Maximum     ***      Modern, fastest\n\n");
    
    // Security Levels
    fmt::print("Security Levels\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  Level       Iterations    Memory    Time     Use Case\n");
    fmt::print("  ---------   ----------    ------    ----     -----------------\n");
    fmt::print("  weak        1             4MB       ~2ms     Testing only\n");
    fmt::print("  medium      2             16MB      ~10ms    Recommended\n");
    fmt::print("  strong      3             64MB      ~30ms    Sensitive data\n");
    fmt::print("  paranoid    4             128MB     ~60ms    Top secret\n\n");
    
    // Usage examples
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::green), "Usage Examples\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  Encrypt:  filevault encrypt input.txt output.fvlt -a aes-256-gcm -s medium\n");
    fmt::print("  Decrypt:  filevault decrypt output.fvlt decrypted.txt -p mypassword\n");
    fmt::print("  Caesar:   filevault encrypt msg.txt caesar.fvlt -a caesar -p \"key\"\n\n");
    
    return 0;
}

} // namespace cli
} // namespace filevault

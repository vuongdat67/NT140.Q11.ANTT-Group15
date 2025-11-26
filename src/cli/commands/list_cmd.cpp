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
    
    // Symmetric Encryption (AEAD)
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
    
    // Symmetric Encryption (Non-AEAD)
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::light_blue), 
              "Symmetric Encryption Algorithms (Non-AEAD)\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  WARNING: These modes do NOT provide authentication!\n\n");
    fmt::print("  Algorithm          Key Size    IV Size     Mode        Notes\n");
    fmt::print("  ----------------   --------    --------    ---------   ------------------\n");
    fmt::print("  AES-128-CBC        128-bit     16 bytes    Block       Requires HMAC\n");
    fmt::print("  AES-256-CBC        256-bit     16 bytes    Block       Requires HMAC\n");
    fmt::print("  AES-128-CTR        128-bit     16 bytes    Stream      Counter mode\n");
    fmt::print("  AES-256-CTR        256-bit     16 bytes    Stream      Counter mode\n");
    fmt::print("  AES-128-CFB        128-bit     16 bytes    Stream      Self-sync\n");
    fmt::print("  AES-256-CFB        256-bit     16 bytes    Stream      Self-sync\n");
    fmt::print("  AES-128-OFB        128-bit     16 bytes    Stream      Pre-computed\n");
    fmt::print("  AES-256-OFB        256-bit     16 bytes    Stream      Pre-computed\n");
    fmt::print("  AES-128-XTS        256-bit     16 bytes    Disk        Storage encryption\n");
    fmt::print("  AES-256-XTS        512-bit     16 bytes    Disk        Storage encryption\n");
    fmt::print("  3DES               168-bit     8 bytes     Block       Legacy only!\n\n");
    
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::red), 
              "  ⚠ AES-ECB modes (INSECURE - for testing only):\n");
    fmt::print("  AES-128-ECB        128-bit     None        Block       INSECURE!\n");
    fmt::print("  AES-256-ECB        256-bit     None        Block       INSECURE!\n\n");
    
    // Asymmetric Encryption
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), 
              "Asymmetric Encryption (Public-Key)\n");
    fmt::print("{}\n", std::string(80, '-'));
    fmt::print("  Algorithm    Key Size    Security    Speed    Use Case\n");
    fmt::print("  ----------   --------    --------    -----    ----------------------\n");
    fmt::print("  RSA-2048     2048-bit    Good        *        Key exchange, small data\n");
    fmt::print("  RSA-3072     3072-bit    Strong      *        Recommended minimum\n");
    fmt::print("  RSA-4096     4096-bit    Maximum     *        High security\n");
    fmt::print("  ECC-P256     256-bit     Strong      ***      ECDH + AES-GCM hybrid\n");
    fmt::print("  ECC-P384     384-bit     Strong      **       192-bit security\n");
    fmt::print("  ECC-P521     521-bit     Maximum     **       256-bit security\n\n");

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
    fmt::print("  # AEAD encryption (recommended)\n");
    fmt::print("  filevault encrypt input.txt output.fv -a aes-256-gcm -s medium\n\n");
    fmt::print("  # Non-AEAD encryption\n");
    fmt::print("  filevault encrypt input.txt output.fv -a aes-256-cbc\n");
    fmt::print("  filevault encrypt input.txt output.fv -a aes-256-cfb\n");
    fmt::print("  filevault encrypt input.txt output.fv -a aes-256-xts\n\n");
    fmt::print("  # RSA asymmetric encryption\n");
    fmt::print("  filevault encrypt small.txt output.fv -a rsa-2048\n\n");
    fmt::print("  # ECC hybrid encryption (ECDH + AES-GCM)\n");
    fmt::print("  filevault encrypt data.txt output.fv -a ecc-p256\n\n");
    fmt::print("  # Decrypt\n");
    fmt::print("  filevault decrypt output.fv decrypted.txt -p mypassword\n\n");
    fmt::print("  # Classical ciphers (educational)\n");
    fmt::print("  filevault encrypt msg.txt out.fv -a caesar -p \"key\"\n\n");
    
    return 0;
}

} // namespace cli
} // namespace filevault

#include "filevault/cli/commands/encrypt_cmd.hpp"
#include "filevault/format/file_header.hpp"
#include "filevault/core/file_format.hpp"
#include "filevault/core/modes.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/file_io.hpp"
#include "filevault/utils/crypto_utils.hpp"
#include "filevault/utils/password.hpp"
#include "filevault/utils/progress.hpp"
#include "filevault/compression/compressor.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <iostream>

namespace filevault {
namespace cli {

EncryptCommand::EncryptCommand(core::CryptoEngine& engine)
    : engine_(engine) {
}

void EncryptCommand::setup(CLI::App& app) {
    auto* encrypt_cmd = app.add_subcommand(name(), description());
    
    encrypt_cmd->add_option("input", input_file_, "Input file to encrypt")
        ->required()
        ->check(CLI::ExistingFile);
    
    encrypt_cmd->add_option("output", output_file_, "Output encrypted file");
    
    encrypt_cmd->add_option("-m,--mode", mode_, "Mode preset (overrides other options)")
        ->check(CLI::IsMember({"basic", "standard", "advanced"}));
    
    encrypt_cmd->add_option("-a,--algorithm", algorithm_, "Encryption algorithm")
        ->check(CLI::IsMember({
            // Modern AEAD algorithms
            "aes-128-gcm", "aes-192-gcm", "aes-256-gcm", 
            "chacha20-poly1305", "serpent-256-gcm",
            "twofish-128-gcm", "twofish-192-gcm", "twofish-256-gcm",
            // International standards
            "camellia-128-gcm", "camellia-192-gcm", "camellia-256-gcm",
            "aria-128-gcm", "aria-192-gcm", "aria-256-gcm",
            "sm4-gcm",
            // Non-AEAD modes (CBC)
            "aes-128-cbc", "aes-192-cbc", "aes-256-cbc",
            // Non-AEAD modes (CTR)
            "aes-128-ctr", "aes-192-ctr", "aes-256-ctr",
            // Non-AEAD modes (CFB)
            "aes-128-cfb", "aes-192-cfb", "aes-256-cfb",
            // Non-AEAD modes (OFB)
            "aes-128-ofb", "aes-192-ofb", "aes-256-ofb",
            // Non-AEAD modes (ECB - INSECURE)
            "aes-128-ecb", "aes-192-ecb", "aes-256-ecb",
            // Disk encryption mode (XTS)
            "aes-128-xts", "aes-256-xts",
            // Legacy
            "3des", "tripledes", "triple-des",
            // Asymmetric (RSA)
            "rsa-2048", "rsa-3072", "rsa-4096", "rsa",
            // Asymmetric (ECC)
            "ecc-p256", "ecc-p384", "ecc-p521", "ecc", "p256", "p384", "p521",
            // Classical (educational)
            "caesar", "vigenere", "playfair", "substitution", "hill"
        }));
    
    encrypt_cmd->add_option("-s,--security", security_level_, "Security level")
        ->check(CLI::IsMember({"weak", "medium", "strong", "paranoid"}));
    
    encrypt_cmd->add_option("-k,--kdf", kdf_, "Key derivation function")
        ->check(CLI::IsMember({
            "argon2id", "argon2i", "pbkdf2-sha256", "pbkdf2-sha512", "scrypt"
        }));
    
    encrypt_cmd->add_option("-p,--password", password_, "Encryption password (not recommended)");
    
    encrypt_cmd->add_option("--compression", compression_type_, "Compression algorithm")
        ->check(CLI::IsMember({"none", "zlib", "bzip2", "lzma"}));
    
    encrypt_cmd->add_option("--compression-level", compression_level_, "Compression level (1-9)")
        ->check(CLI::Range(1, 9));
    
    encrypt_cmd->add_flag("-v,--verbose", verbose_, "Verbose output");
    
    encrypt_cmd->add_flag("--no-progress", no_progress_, "Disable progress bars");
    
    encrypt_cmd->callback([this]() { execute(); });
}

int EncryptCommand::execute() {
    try {
        utils::Console::header("FileVault Encryption");
        
        // Apply mode preset if specified
        if (!mode_.empty()) {
            auto user_mode = core::ModePreset::parse_mode(mode_);
            auto preset = core::ModePreset::get_preset(user_mode);
            
            // Override settings with preset
            algorithm_ = engine_.algorithm_name(preset.algorithm);
            kdf_ = engine_.kdf_name(preset.kdf);
            security_level_ = engine_.security_level_name(preset.security_level);
            compression_type_ = compression::CompressionService::get_algorithm_name(preset.compression);
            compression_level_ = preset.compression_level;
            
            utils::Console::info(fmt::format("Using {} mode: {}", 
                               preset.name(), preset.description()));
        }
        
        // Get password securely if not provided
        if (password_.empty()) {
            // Try up to 3 times to get a valid password
            int attempts = 0;
            const int MAX_ATTEMPTS = 3;
            
            while (password_.empty() && attempts < MAX_ATTEMPTS) {
                password_ = utils::Password::read_secure("Enter encryption password: ", true);
                
                if (password_.empty()) {
                    attempts++;
                    if (attempts < MAX_ATTEMPTS) {
                        utils::Console::error("Password cannot be empty. Please try again.");
                    } else {
                        utils::Console::error("Too many failed attempts. Encryption cancelled.");
                        return 1;
                    }
                }
            }
            
            if (password_.empty()) {
                utils::Console::error("Password cannot be empty");
                return 1;
            }
        } else {
            utils::Console::warning("Using password from command line is insecure!");
        }
        
        // Check password strength (only once after we have a valid password)
        auto strength_analysis = utils::Password::analyze_strength(password_);
        if (strength_analysis.strength == core::PasswordStrength::VERY_WEAK || 
            strength_analysis.strength == core::PasswordStrength::WEAK) {
            fmt::print("\n");
            utils::Console::warning(fmt::format("Password strength: {} (score: {}/100)", 
                                   utils::Password::get_strength_label(strength_analysis.strength),
                                   static_cast<int>(strength_analysis.score)));
            
            if (!strength_analysis.warnings.empty()) {
                fmt::print("  ⚠️  Warnings:\n");
                for (const auto& warning : strength_analysis.warnings) {
                    fmt::print("      • {}\n", warning);
                }
            }
            
            if (!strength_analysis.suggestions.empty()) {
                fmt::print("  💡 Suggestions:\n");
                for (const auto& suggestion : strength_analysis.suggestions) {
                    fmt::print("      • {}\n", suggestion);
                }
            }
            
            fmt::print("\n");
            std::string response;
            fmt::print("Continue with weak password? (y/N): ");
            std::getline(std::cin, response);
            
            if (response != "y" && response != "Y") {
                utils::Console::info("Encryption cancelled");
                return 0;
            }
        } else if (verbose_ && (strength_analysis.strength == core::PasswordStrength::FAIR || 
                               strength_analysis.strength == core::PasswordStrength::STRONG ||
                               strength_analysis.strength == core::PasswordStrength::VERY_STRONG)) {
            utils::Console::success(fmt::format("Password strength: {} (score: {}/100)", 
                                   utils::Password::get_strength_label(strength_analysis.strength),
                                   static_cast<int>(strength_analysis.score)));
        }
        
        // Set output file if not specified
        if (output_file_.empty()) {
            output_file_ = input_file_ + ".fvlt";
        }
        
        // Show configuration
        utils::Console::info(fmt::format("Input:     {}", input_file_));
        utils::Console::info(fmt::format("Output:    {}", output_file_));
        utils::Console::info(fmt::format("Algorithm: {}", algorithm_));
        utils::Console::info(fmt::format("Security:  {}", security_level_));
        utils::Console::info(fmt::format("KDF:       {}", kdf_));
        utils::Console::separator();
        
        // Read input file
        auto file_result = utils::FileIO::read_file(input_file_);
        if (!file_result) {
            utils::Console::error(file_result.error_message);
            return 1;
        }
        
        auto plaintext = file_result.value;
        utils::Console::info(fmt::format("Read {} bytes", plaintext.size()));
        
        // Step 1: Compress if requested
        bool compressed = false;
        size_t original_size = plaintext.size();
        
        if (compression_type_ != "none") {
            utils::Console::info(fmt::format("Compressing with {}...", compression_type_));
            
            auto comp_type = compression::CompressionService::parse_algorithm(compression_type_);
            
            auto compressor = compression::CompressionService::create(comp_type);
            if (!compressor) {
                utils::Console::error("Failed to create compressor");
                return 1;
            }
            
            std::unique_ptr<utils::ProgressBar> compress_progress;
            if (!no_progress_) {
                compress_progress = std::make_unique<utils::ProgressBar>("Compressing", 100);
                compress_progress->set_progress(50);  // Show activity
            }
            
            auto compress_result = compressor->compress(plaintext, compression_level_);
            
            if (compress_progress) {
                compress_progress->mark_as_completed();
            }
            
            if (!compress_result.success) {
                utils::Console::error(compress_result.error_message);
                return 1;
            }
            
            plaintext = std::move(compress_result.data);
            compressed = true;
            
            utils::Console::info(fmt::format("Compressed: {} -> {} bytes ({:.1f}% ratio)",
                               original_size,
                               plaintext.size(),
                               compress_result.compression_ratio));
        }
        
        spdlog::debug("Parsing configuration...");
        // Parse configuration
        auto algo_type_opt = engine_.parse_algorithm(algorithm_);
        auto kdf_type_opt = engine_.parse_kdf(kdf_);
        auto sec_level_opt = engine_.parse_security_level(security_level_);
        
        spdlog::debug("algo_type_opt: {}, kdf_type_opt: {}, sec_level_opt: {}", 
                     algo_type_opt.has_value(), kdf_type_opt.has_value(), sec_level_opt.has_value());
        
        if (!algo_type_opt || !kdf_type_opt || !sec_level_opt) {
            utils::Console::error("Invalid configuration parameters");
            return 1;
        }
        
        auto algo_type = algo_type_opt.value();
        auto kdf_type = kdf_type_opt.value();
        auto sec_level = sec_level_opt.value();
        
        // Get algorithm
        auto* algorithm = engine_.get_algorithm(algo_type);
        if (!algorithm) {
            utils::Console::error(fmt::format("Algorithm '{}' not available", algorithm_));
            return 1;
        }
        
        // Setup encryption config
        core::EncryptionConfig config;
        config.algorithm = algo_type;
        config.kdf = kdf_type;
        config.level = sec_level;
        config.apply_security_level();
        
        // Step 2: Generate salt and derive key
        utils::Console::info("Deriving key...");
        std::unique_ptr<utils::ProgressBar> kdf_progress;
        if (!no_progress_) {
            kdf_progress = std::make_unique<utils::ProgressBar>("Deriving key", 100);
            kdf_progress->set_progress(50);  // Show activity
        }
        
        auto salt = engine_.generate_salt(32);
        auto key = engine_.derive_key(password_, salt, config);
        
        if (kdf_progress) {
            kdf_progress->mark_as_completed();
        }
        
        // Generate nonce and add to config
        auto nonce = engine_.generate_nonce(12); // GCM standard
        config.nonce = nonce;
        
        // Step 3: Encrypt
        utils::Console::info("Encrypting...");
        std::unique_ptr<utils::ProgressBar> encrypt_progress;
        if (!no_progress_) {
            encrypt_progress = std::make_unique<utils::ProgressBar>("Encrypting", 100);
            encrypt_progress->set_progress(50);  // Show activity
        }
        
        auto encrypt_result = algorithm->encrypt(plaintext, key, config);
        
        if (encrypt_progress) {
            encrypt_progress->mark_as_completed();
        }
        
        if (!encrypt_result.success) {
            utils::Console::error(encrypt_result.error_message);
            return 1;
        }
        
        utils::Console::info(fmt::format("Encrypted in {:.2f}ms", encrypt_result.processing_time_ms));
        
        // For non-AEAD algorithms (CBC, CTR), use the IV/nonce from encrypt result
        // The algorithm generates its own IV during encryption
        std::vector<uint8_t> nonce_to_store = nonce;
        if (encrypt_result.nonce.has_value() && !encrypt_result.nonce.value().empty()) {
            nonce_to_store = encrypt_result.nonce.value();
        }
        
        // Create enhanced file header
        config.compression = compressed ? core::CompressionType::ZLIB : core::CompressionType::NONE;
        auto header = core::FileFormatHandler::create_header(
            algo_type,
            kdf_type,
            config,
            salt,
            nonce_to_store,
            compressed
        );
        
        // Extract ciphertext and tag from CryptoResult
        // AES_GCM::encrypt() stores them separately in result.data and result.tag
        std::vector<uint8_t> ciphertext_only = encrypt_result.data;  // Already without tag
        std::vector<uint8_t> auth_tag;
        
        // Only AEAD algorithms (GCM, ChaCha20-Poly1305) have authentication tags
        bool is_aead = (algo_type == core::AlgorithmType::AES_128_GCM ||
                       algo_type == core::AlgorithmType::AES_192_GCM ||
                       algo_type == core::AlgorithmType::AES_256_GCM ||
                       algo_type == core::AlgorithmType::CHACHA20_POLY1305 ||
                       algo_type == core::AlgorithmType::SERPENT_256_GCM ||
                       algo_type == core::AlgorithmType::TWOFISH_128_GCM ||
                       algo_type == core::AlgorithmType::TWOFISH_192_GCM ||
                       algo_type == core::AlgorithmType::TWOFISH_256_GCM ||
                       algo_type == core::AlgorithmType::CAMELLIA_128_GCM ||
                       algo_type == core::AlgorithmType::CAMELLIA_192_GCM ||
                       algo_type == core::AlgorithmType::CAMELLIA_256_GCM ||
                       algo_type == core::AlgorithmType::ARIA_128_GCM ||
                       algo_type == core::AlgorithmType::ARIA_192_GCM ||
                       algo_type == core::AlgorithmType::ARIA_256_GCM ||
                       algo_type == core::AlgorithmType::SM4_GCM);
        
        if (is_aead) {
            if (encrypt_result.tag.has_value()) {
                auth_tag = encrypt_result.tag.value();
            } else {
                utils::Console::error("No authentication tag generated for AEAD algorithm");
                return 1;
            }
        }
        // Classical ciphers don't have tags - auth_tag remains empty
        
        // Write enhanced format file
        bool write_success = core::FileFormatHandler::write_file(
            output_file_,
            header,
            ciphertext_only,
            auth_tag
        );
        
        if (!write_success) {
            utils::Console::error("Failed to write output file");
            return 1;
        }
        
        // Get final file size
        std::ifstream check_file(output_file_, std::ios::binary | std::ios::ate);
        size_t final_size = check_file.tellg();
        check_file.close();
        
        utils::Console::separator();
        utils::Console::success("Encryption completed!");
        utils::Console::info(fmt::format("Output: {} ({})", 
                           output_file_, 
                           utils::CryptoUtils::format_bytes(final_size)));
        utils::Console::info(fmt::format("Compression: {:.1f}%", 
                           100.0 * final_size / plaintext.size()));
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Encryption failed: {}", e.what()));
        return 1;
    }
}

} // namespace cli
} // namespace filevault

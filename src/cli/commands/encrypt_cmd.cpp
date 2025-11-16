#include "filevault/cli/commands/encrypt_cmd.hpp"
#include "filevault/format/file_header.hpp"
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
    
    encrypt_cmd->add_option("-a,--algorithm", algorithm_, "Encryption algorithm")
        ->check(CLI::IsMember({
            "aes-128-gcm", "aes-192-gcm", "aes-256-gcm", 
            "chacha20-poly1305",
            "caesar", "vigenere", "playfair"
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
        
        // Get password securely if not provided
        if (password_.empty()) {
            password_ = utils::Password::read_secure("Enter encryption password: ", true);
            if (password_.empty()) {
                utils::Console::error("Password cannot be empty");
                return 1;
            }
        } else {
            utils::Console::warning("Using password from command line is insecure!");
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
        
        // Create file header
        format::FileHeader header;
        header.set_algorithm(algo_type);
        header.set_kdf(kdf_type);
        header.set_security_level(sec_level);
        header.set_salt(salt);
        header.set_nonce(nonce);
        if (encrypt_result.tag.has_value()) {
            header.set_tag(encrypt_result.tag.value());
        }
        header.set_original_size(original_size);  // Original uncompressed size
        header.set_encrypted_size(encrypt_result.data.size());
        header.set_timestamp(std::chrono::system_clock::now().time_since_epoch().count());
        header.set_compressed(compressed);
        
        if (!header.validate()) {
            utils::Console::error("Invalid header generated");
            return 1;
        }
        
        // Serialize header + ciphertext
        auto header_bytes = header.serialize();
        std::vector<uint8_t> output;
        output.reserve(header_bytes.size() + encrypt_result.data.size());
        output.insert(output.end(), header_bytes.begin(), header_bytes.end());
        output.insert(output.end(), encrypt_result.data.begin(), encrypt_result.data.end());
        
        // Write output file
        auto write_result = utils::FileIO::write_file(output_file_, output);
        if (!write_result) {
            utils::Console::error(write_result.error_message);
            return 1;
        }
        
        utils::Console::separator();
        utils::Console::success("Encryption completed!");
        utils::Console::info(fmt::format("Output: {} ({} bytes)", 
                           output_file_, 
                           utils::CryptoUtils::format_bytes(output.size())));
        utils::Console::info(fmt::format("Compression: {:.1f}%", 
                           100.0 * output.size() / plaintext.size()));
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Encryption failed: {}", e.what()));
        return 1;
    }
}

} // namespace cli
} // namespace filevault

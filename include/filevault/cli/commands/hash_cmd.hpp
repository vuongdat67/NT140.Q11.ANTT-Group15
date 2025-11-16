#ifndef FILEVAULT_CLI_COMMANDS_HASH_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_HASH_CMD_HPP

#include "filevault/cli/command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <string>
#include <unordered_map>

namespace filevault {
namespace cli {

/**
 * @brief Hash command - Calculate cryptographic hashes
 * 
 * Supports: MD5, SHA1, SHA2 family (224/256/384/512), 
 * SHA3 family (224/256/384/512), BLAKE2 (b/s variants)
 * 
 * Features:
 * - HMAC mode with key
 * - Hash verification
 * - Batch processing
 * - Performance benchmarking
 */
class HashCommand : public ICommand {
public:
    explicit HashCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return "hash"; }
    std::string description() const override { 
        return "Calculate cryptographic hash of files (MD5, SHA1-3, BLAKE2)"; 
    }
    
    void setup(CLI::App& app) override;
    int execute() override;

private:
    core::CryptoEngine& engine_;
    
    // Options
    std::string input_file_;
    std::string output_file_;
    std::string algorithm_ = "sha256";
    std::string verify_hash_;
    std::string hmac_key_;
    bool uppercase_ = false;
    bool no_filename_ = false;
    bool verbose_ = false;
    bool benchmark_ = false;
    
    // Helper methods
    std::string get_botan_algorithm_name(const std::string& algo);
    bool is_secure_algorithm(const std::string& algo);
    
    std::string calculate_file_hash(
        const std::string& filepath,
        const std::string& algorithm
    );
    
    std::string calculate_file_hmac(
        const std::string& filepath,
        const std::string& hash_algorithm,
        const std::string& key
    );
    
    int verify_mode(const std::string& calculated_hash);
};

} // namespace cli
} // namespace filevault

#endif // FILEVAULT_CLI_COMMANDS_HASH_CMD_HPP

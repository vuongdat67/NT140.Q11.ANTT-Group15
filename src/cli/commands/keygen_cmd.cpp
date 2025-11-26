/**
 * @file keygen_cmd.cpp
 * @brief Key generation command implementation
 */

#include "filevault/cli/commands/keygen_cmd.hpp"
#include "filevault/algorithms/asymmetric/rsa.hpp"
#include "filevault/algorithms/asymmetric/ecc.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/file_io.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>

namespace filevault {
namespace cli {

KeygenCommand::KeygenCommand(core::CryptoEngine& engine) 
    : engine_(engine) {
}

void KeygenCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name(), description());
    
    cmd->add_option("-a,--algorithm", algorithm_, "Algorithm for key generation")
        ->check(CLI::IsMember({
            "rsa-2048", "rsa-3072", "rsa-4096", "rsa",
            "ecc-p256", "ecc-p384", "ecc-p521", "ecc",
            "ecdsa-p256", "ecdsa-p384", "ecdsa-p521"
        }));
    
    cmd->add_option("-o,--output", output_prefix_, "Output file prefix (default: filevault_key)");
    
    cmd->add_flag("-f,--force", force_, "Overwrite existing key files");
    cmd->add_flag("-v,--verbose", verbose_, "Verbose output");
    
    cmd->callback([this]() { execute(); });
}

int KeygenCommand::execute() {
    utils::Console::header("FileVault Key Generation");
    
    std::string pub_file = output_prefix_ + ".pub";
    std::string priv_file = output_prefix_ + ".key";
    
    // Check if files exist
    if (!force_) {
        if (std::filesystem::exists(pub_file)) {
            utils::Console::error(fmt::format("Public key file '{}' already exists. Use -f to overwrite.", pub_file));
            return 1;
        }
        if (std::filesystem::exists(priv_file)) {
            utils::Console::error(fmt::format("Private key file '{}' already exists. Use -f to overwrite.", priv_file));
            return 1;
        }
    }
    
    utils::Console::info(fmt::format("Generating {} key pair...", algorithm_));
    
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> private_key;
    std::string algo_type;
    
    try {
        // Normalize algorithm name
        std::string algo = algorithm_;
        std::transform(algo.begin(), algo.end(), algo.begin(), ::tolower);
        
        if (algo == "rsa" || algo == "rsa-2048") {
            algorithms::asymmetric::RSA rsa(2048);
            auto keypair = rsa.generate_key_pair();
            public_key = keypair.public_key;
            private_key = keypair.private_key;
            algo_type = "RSA-2048";
        } else if (algo == "rsa-3072") {
            algorithms::asymmetric::RSA rsa(3072);
            auto keypair = rsa.generate_key_pair();
            public_key = keypair.public_key;
            private_key = keypair.private_key;
            algo_type = "RSA-3072";
        } else if (algo == "rsa-4096") {
            algorithms::asymmetric::RSA rsa(4096);
            auto keypair = rsa.generate_key_pair();
            public_key = keypair.public_key;
            private_key = keypair.private_key;
            algo_type = "RSA-4096";
        } else if (algo == "ecc" || algo == "ecc-p256" || algo == "ecdsa-p256") {
            algorithms::asymmetric::ECCHybrid ecc(algorithms::asymmetric::ECCurve::SECP256R1);
            auto keypair = ecc.generate_key_pair();
            public_key = keypair.public_key;
            private_key = keypair.private_key;
            algo_type = "ECC-P256";
        } else if (algo == "ecc-p384" || algo == "ecdsa-p384") {
            algorithms::asymmetric::ECCHybrid ecc(algorithms::asymmetric::ECCurve::SECP384R1);
            auto keypair = ecc.generate_key_pair();
            public_key = keypair.public_key;
            private_key = keypair.private_key;
            algo_type = "ECC-P384";
        } else if (algo == "ecc-p521" || algo == "ecdsa-p521") {
            algorithms::asymmetric::ECCHybrid ecc(algorithms::asymmetric::ECCurve::SECP521R1);
            auto keypair = ecc.generate_key_pair();
            public_key = keypair.public_key;
            private_key = keypair.private_key;
            algo_type = "ECC-P521";
        } else {
            utils::Console::error(fmt::format("Unknown algorithm: {}", algorithm_));
            return 1;
        }
        
        // Write public key
        {
            std::ofstream file(pub_file, std::ios::binary);
            if (!file) {
                utils::Console::error(fmt::format("Failed to create public key file: {}", pub_file));
                return 1;
            }
            file.write(reinterpret_cast<const char*>(public_key.data()), public_key.size());
        }
        
        // Write private key
        {
            std::ofstream file(priv_file, std::ios::binary);
            if (!file) {
                utils::Console::error(fmt::format("Failed to create private key file: {}", priv_file));
                return 1;
            }
            file.write(reinterpret_cast<const char*>(private_key.data()), private_key.size());
        }
        
        utils::Console::separator();
        utils::Console::success(fmt::format("{} key pair generated successfully!", algo_type));
        utils::Console::info(fmt::format("Public key:  {} ({} bytes)", pub_file, public_key.size()));
        utils::Console::info(fmt::format("Private key: {} ({} bytes)", priv_file, private_key.size()));
        
        fmt::print("\n");
        fmt::print("Usage:\n");
        fmt::print("  Encrypt with public key:\n");
        fmt::print("    filevault encrypt input.txt output.fv --public-key {}\n", pub_file);
        fmt::print("  Decrypt with private key:\n");
        fmt::print("    filevault decrypt output.fv decrypted.txt --private-key {}\n", priv_file);
        fmt::print("\n");
        utils::Console::warning("Keep your private key secure! Never share it.");
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Key generation failed: {}", e.what()));
        return 1;
    }
}

} // namespace cli
} // namespace filevault

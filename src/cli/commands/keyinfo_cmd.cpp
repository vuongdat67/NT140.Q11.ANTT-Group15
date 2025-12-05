#include "filevault/cli/commands/keyinfo_cmd.hpp"
#include "filevault/utils/console.hpp"
#include <botan/pk_keys.h>
#include <botan/pem.h>
#include <botan/data_src.h>
#include <botan/x509_key.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>
#include <fstream>
#include <sstream>

namespace filevault {
namespace cli {
namespace commands {

KeyInfoCommand::KeyInfoCommand(core::CryptoEngine& engine) 
    : engine_(engine) {}

void KeyInfoCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name_, description_);
    
    cmd->add_option("key", key_path_, "Key file to inspect (PEM format)")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_flag("--public", show_public_, "Extract and display public key from private key");
    
    cmd->add_option("--check-pair", pair_key_path_, "Check if this key forms a valid pair with the main key")
        ->check(CLI::ExistingFile);
    
    cmd->footer(
        "\nExamples:\n"
        "  Show key info:         filevault keyinfo private.pem\n"
        "  Extract public key:    filevault keyinfo private.pem --public\n"
        "  Check key pair:        filevault keyinfo private.pem --check-pair public.pem\n"
        "\n"
        "Supported formats: PEM (PKCS#8 for private, X.509 for public)\n"
        "Displays: algorithm, key size, fingerprint, validity\n"
    );
    
    cmd->callback([this]() { this->execute(); });
}

int KeyInfoCommand::execute() {
    try {
        // Read key file
        std::ifstream file(key_path_, std::ios::binary);
        if (!file) {
            utils::Console::error("Failed to open key file");
            return 1;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string key_pem = buffer.str();
        file.close();
        
        utils::Console::header("Key Information");
        utils::Console::info(fmt::format("File: {}", key_path_));
        
        // Try to load as private key first
        bool is_private = false;
        std::unique_ptr<Botan::Private_Key> priv_key;
        std::unique_ptr<Botan::Public_Key> pub_key;
        
        try {
            Botan::DataSource_Memory key_source(key_pem);
            priv_key = Botan::PKCS8::load_key(key_source);
            if (priv_key) {
                is_private = true;
                utils::Console::info("Type: Private Key");
            }
        } catch (...) {
            // Not a private key, try public
        }
        
        if (!is_private) {
            try {
                Botan::DataSource_Memory key_source(key_pem);
                pub_key = Botan::X509::load_key(key_source);
                if (pub_key) {
                    utils::Console::info("Type: Public Key");
                }
            } catch (const std::exception& e) {
                utils::Console::error(fmt::format("Failed to load key: {}", e.what()));
                return 1;
            }
        }
        
        // Get key information
        const Botan::Public_Key* key_to_inspect = nullptr;
        if (is_private && priv_key) {
            key_to_inspect = priv_key.get();
        } else if (pub_key) {
            key_to_inspect = pub_key.get();
        }
        
        if (!key_to_inspect) {
            utils::Console::error("Invalid key format");
            return 1;
        }
        
        // Display key properties
        utils::Console::info(fmt::format("Algorithm: {}", key_to_inspect->algo_name()));
        utils::Console::info(fmt::format("Key Size: {} bits", key_to_inspect->key_length()));
        
        // Get fingerprint
        auto public_bits = key_to_inspect->public_key_bits();
        auto fingerprint = Botan::hex_encode(public_bits.data(), 
                                             std::min(size_t(20), public_bits.size()));
        utils::Console::info(fmt::format("Fingerprint: {}", fingerprint));
        
        // Show public key if requested
        if (show_public_ && is_private && priv_key) {
            fmt::print("\n");
            utils::Console::header("Public Key (PEM)");
            std::string pub_pem = Botan::X509::PEM_encode(*priv_key);
            fmt::print("{}\n", pub_pem);
        }
        
        // Check key pair if requested
        if (!pair_key_path_.empty()) {
            fmt::print("\n");
            utils::Console::header("Key Pair Validation");
            
            std::ifstream pair_file(pair_key_path_, std::ios::binary);
            if (!pair_file) {
                utils::Console::error("Failed to open pair key file");
                return 1;
            }
            
            std::stringstream pair_buffer;
            pair_buffer << pair_file.rdbuf();
            std::string pair_pem = pair_buffer.str();
            pair_file.close();
            
            try {
                // Load the pair key
                std::unique_ptr<Botan::Public_Key> pair_pub;
                std::unique_ptr<Botan::Private_Key> pair_priv;
                
                try {
                    Botan::DataSource_Memory source(pair_pem);
                    pair_priv = Botan::PKCS8::load_key(source);
                    if (pair_priv) {
                        pair_pub.reset(dynamic_cast<Botan::Public_Key*>(pair_priv.get()));
                    }
                } catch (...) {
                    Botan::DataSource_Memory source(pair_pem);
                    pair_pub = Botan::X509::load_key(source);
                }
                
                if (!pair_pub) {
                    utils::Console::error("Failed to load pair key");
                    return 1;
                }
                
                // Compare public key bits
                auto bits1 = key_to_inspect->public_key_bits();
                auto bits2 = pair_pub->public_key_bits();
                
                if (bits1 == bits2) {
                    utils::Console::success("✓ Keys form a valid pair");
                } else {
                    utils::Console::error("✗ Keys do NOT form a valid pair");
                    return 1;
                }
                
            } catch (const std::exception& e) {
                utils::Console::error(fmt::format("Pair validation failed: {}", e.what()));
                return 1;
            }
        }
        
        fmt::print("\n");
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Failed to inspect key: {}", e.what()));
        return 1;
    }
}

} // namespace commands
} // namespace cli
} // namespace filevault

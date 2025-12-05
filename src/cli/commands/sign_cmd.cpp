#include "filevault/cli/commands/sign_cmd.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/file_io.hpp"
#include "filevault/algorithms/asymmetric/rsa.hpp"
#include <botan/hex.h>
#include <fstream>

namespace filevault {
namespace cli {
namespace commands {

SignCommand::SignCommand(core::CryptoEngine& engine) 
    : engine_(engine) {}

void SignCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name_, description_);
    
    cmd->add_option("file", file_path_, "File to sign")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_option("private-key", private_key_path_, "Private key file (PEM format)")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_option("-o,--output", output_path_, "Output signature file (.sig)")
        ->default_val("");
    
    cmd->add_option("-a,--algorithm", algorithm_, "Signature algorithm")
        ->default_val("rsa")
        ->check(CLI::IsMember({"rsa", "ecc", "ed25519"}));
    
    cmd->footer(
        "\nExamples:\n"
        "  Sign with RSA:     filevault sign document.txt private.pem -o document.sig\n"
        "  Sign with ECC:     filevault sign file.bin key.pem -a ecc\n"
        "\n"
        "Supported algorithms: rsa, ecc, ed25519\n"
        "Default output: <filename>.sig\n"
    );
    
    cmd->callback([this]() { this->execute(); });
}

int SignCommand::execute() {
    try {
        // Set default output path
        if (output_path_.empty()) {
            output_path_ = file_path_ + ".sig";
        }
        
        utils::Console::info(fmt::format("Signing file: {}", file_path_));
        utils::Console::info(fmt::format("Algorithm: {}", algorithm_));
        
        // Read file to sign
        std::ifstream file(file_path_, std::ios::binary);
        if (!file) {
            utils::Console::error("Failed to open file");
            return 1;
        }
        
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> data(file_size);
        file.read(reinterpret_cast<char*>(data.data()), file_size);
        file.close();
        
        // Read private key
        std::ifstream key_file(private_key_path_, std::ios::binary);
        if (!key_file) {
            utils::Console::error("Failed to open private key file");
            return 1;
        }
        
        key_file.seekg(0, std::ios::end);
        size_t key_size = key_file.tellg();
        key_file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> private_key(key_size);
        key_file.read(reinterpret_cast<char*>(private_key.data()), key_size);
        key_file.close();
        
        // Create signature based on algorithm
        std::vector<uint8_t> signature;
        
        if (algorithm_ == "rsa") {
            algorithms::asymmetric::RSA rsa;
            signature = rsa.sign(data, private_key);
        } else if (algorithm_ == "ecc") {
            utils::Console::error("ECC signing not yet implemented");
            return 1;
        } else if (algorithm_ == "ed25519") {
            utils::Console::error("Ed25519 signing not yet implemented");
            return 1;
        }
        
        // Write signature to file
        std::ofstream sig_file(output_path_, std::ios::binary);
        if (!sig_file) {
            utils::Console::error("Failed to create signature file");
            return 1;
        }
        
        sig_file.write(reinterpret_cast<const char*>(signature.data()), signature.size());
        sig_file.close();
        
        utils::Console::success(fmt::format("Signature created: {}", output_path_));
        utils::Console::info(fmt::format("Signature size: {} bytes", signature.size()));
        utils::Console::info(fmt::format("Signature (hex): {}", 
            Botan::hex_encode(signature.data(), std::min(size_t(32), signature.size()))));
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Signing failed: {}", e.what()));
        return 1;
    }
}

} // namespace commands
} // namespace cli
} // namespace filevault

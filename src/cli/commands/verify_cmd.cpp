#include "filevault/cli/commands/verify_cmd.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/file_io.hpp"
#include "filevault/algorithms/asymmetric/rsa.hpp"
#include <fstream>

namespace filevault {
namespace cli {
namespace commands {

VerifyCommand::VerifyCommand(core::CryptoEngine& engine) 
    : engine_(engine) {}

void VerifyCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name_, description_);
    
    cmd->add_option("file", file_path_, "File to verify")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_option("signature", signature_path_, "Signature file (.sig)")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_option("public-key", public_key_path_, "Public key file (PEM format)")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_option("-a,--algorithm", algorithm_, "Signature algorithm")
        ->default_val("rsa")
        ->check(CLI::IsMember({"rsa", "ecc", "ed25519"}));
    
    cmd->footer(
        "\nExamples:\n"
        "  Verify RSA signature:  filevault verify document.txt document.sig public.pem\n"
        "  Verify ECC signature:  filevault verify file.bin file.sig key.pem -a ecc\n"
        "\n"
        "Supported algorithms: rsa, ecc, ed25519\n"
        "Exit code: 0 = valid signature, 1 = invalid signature or error\n"
    );
    
    cmd->callback([this]() { this->execute(); });
}

int VerifyCommand::execute() {
    try {
        utils::Console::info(fmt::format("Verifying signature for: {}", file_path_));
        utils::Console::info(fmt::format("Algorithm: {}", algorithm_));
        
        // Read file data
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
        
        // Read signature
        std::ifstream sig_file(signature_path_, std::ios::binary);
        if (!sig_file) {
            utils::Console::error("Failed to open signature file");
            return 1;
        }
        
        sig_file.seekg(0, std::ios::end);
        size_t sig_size = sig_file.tellg();
        sig_file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> signature(sig_size);
        sig_file.read(reinterpret_cast<char*>(signature.data()), sig_size);
        sig_file.close();
        
        // Read public key
        std::ifstream key_file(public_key_path_, std::ios::binary);
        if (!key_file) {
            utils::Console::error("Failed to open public key file");
            return 1;
        }
        
        key_file.seekg(0, std::ios::end);
        size_t key_size = key_file.tellg();
        key_file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> public_key(key_size);
        key_file.read(reinterpret_cast<char*>(public_key.data()), key_size);
        key_file.close();
        
        // Verify signature based on algorithm
        bool valid = false;
        
        if (algorithm_ == "rsa") {
            algorithms::asymmetric::RSA rsa;
            valid = rsa.verify(data, signature, public_key);
        } else if (algorithm_ == "ecc") {
            utils::Console::error("ECC verification not yet implemented");
            return 1;
        } else if (algorithm_ == "ed25519") {
            utils::Console::error("Ed25519 verification not yet implemented");
            return 1;
        }
        
        if (valid) {
            utils::Console::success("✓ Signature is VALID");
            return 0;
        } else {
            utils::Console::error("✗ Signature is INVALID");
            return 1;
        }
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Verification failed: {}", e.what()));
        return 1;
    }
}

} // namespace commands
} // namespace cli
} // namespace filevault

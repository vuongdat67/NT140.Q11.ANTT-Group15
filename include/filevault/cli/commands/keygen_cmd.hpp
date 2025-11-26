/**
 * @file keygen_cmd.hpp
 * @brief Key generation command for asymmetric encryption
 */

#ifndef FILEVAULT_CLI_COMMANDS_KEYGEN_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_KEYGEN_CMD_HPP

#include "filevault/cli/command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <CLI/CLI.hpp>

namespace filevault {
namespace cli {

/**
 * @brief Command to generate key pairs for asymmetric encryption
 */
class KeygenCommand : public ICommand {
public:
    explicit KeygenCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return "keygen"; }
    std::string description() const override { 
        return "Generate key pair for asymmetric encryption (RSA/ECC)"; 
    }
    
    void setup(CLI::App& app) override;
    int execute() override;
    
private:
    core::CryptoEngine& engine_;
    std::string algorithm_ = "rsa-2048";
    std::string output_prefix_ = "filevault_key";
    bool force_ = false;
    bool verbose_ = false;
};

} // namespace cli
} // namespace filevault

#endif

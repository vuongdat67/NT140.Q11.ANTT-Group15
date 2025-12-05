#ifndef FILEVAULT_CLI_COMMANDS_SIGN_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_SIGN_CMD_HPP

#include "../command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <string>

namespace filevault {
namespace cli {
namespace commands {

/**
 * @brief Command to create digital signature for a file
 */
class SignCommand : public ICommand {
public:
    explicit SignCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return name_; }
    std::string description() const override { return description_; }
    
    void setup(CLI::App& app) override;
    int execute() override;
    
private:
    std::string name_ = "sign";
    std::string description_ = "Create digital signature for a file";
    
    core::CryptoEngine& engine_;
    std::string file_path_;
    std::string private_key_path_;
    std::string output_path_;
    std::string algorithm_ = "rsa";  // rsa, ecc, ed25519
};

} // namespace commands
} // namespace cli
} // namespace filevault

#endif // FILEVAULT_CLI_COMMANDS_SIGN_CMD_HPP

#ifndef FILEVAULT_CLI_COMMANDS_VERIFY_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_VERIFY_CMD_HPP

#include "../command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <string>

namespace filevault {
namespace cli {
namespace commands {

/**
 * @brief Command to verify digital signature of a file
 */
class VerifyCommand : public ICommand {
public:
    explicit VerifyCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return name_; }
    std::string description() const override { return description_; }
    
    void setup(CLI::App& app) override;
    int execute() override;
    
private:
    std::string name_ = "verify";
    std::string description_ = "Verify digital signature of a file";
    
    core::CryptoEngine& engine_;
    std::string file_path_;
    std::string signature_path_;
    std::string public_key_path_;
    std::string algorithm_ = "rsa";
};

} // namespace commands
} // namespace cli
} // namespace filevault

#endif // FILEVAULT_CLI_COMMANDS_VERIFY_CMD_HPP

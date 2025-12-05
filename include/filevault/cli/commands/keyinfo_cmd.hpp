#ifndef FILEVAULT_CLI_COMMANDS_KEYINFO_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_KEYINFO_CMD_HPP

#include "../command.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <string>

namespace filevault {
namespace cli {
namespace commands {

/**
 * @brief Command to display information about cryptographic keys
 */
class KeyInfoCommand : public ICommand {
public:
    explicit KeyInfoCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return name_; }
    std::string description() const override { return description_; }
    
    void setup(CLI::App& app) override;
    int execute() override;
    
private:
    std::string name_ = "keyinfo";
    std::string description_ = "Display information about cryptographic keys";
    
    core::CryptoEngine& engine_;
    std::string key_path_;
    bool show_public_ = false;
    bool check_pair_ = false;
    std::string pair_key_path_;
};

} // namespace commands
} // namespace cli
} // namespace filevault

#endif // FILEVAULT_CLI_COMMANDS_KEYINFO_CMD_HPP

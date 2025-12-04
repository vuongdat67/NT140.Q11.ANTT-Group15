#ifndef FILEVAULT_CLI_COMMANDS_DECOMPRESS_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_DECOMPRESS_CMD_HPP

#include "filevault/cli/command.hpp"

namespace filevault::cli {

/**
 * @brief Decompress command - alias for compress -d
 * 
 * Provides a direct `decompress` command for better UX
 */
class DecompressCommand : public ICommand {
public:
    std::string name() const override { return "decompress"; }
    std::string description() const override { return "Decompress a compressed file"; }
    
    void setup(CLI::App& app) override;
    int execute() override;

private:
    std::string detect_algorithm(const std::string& path);
    std::string generate_output_path(const std::string& input);
    
    CLI::App* subcommand_ = nullptr;
    std::string input_file_;
    std::string output_file_;
    std::string algorithm_;
    bool auto_detect_ = true;  // Auto-detect by default for decompress
    bool verbose_ = false;
    bool benchmark_ = false;
};

} // namespace filevault::cli

#endif

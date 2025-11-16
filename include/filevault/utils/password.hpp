#ifndef FILEVAULT_UTILS_PASSWORD_HPP
#define FILEVAULT_UTILS_PASSWORD_HPP

#include "filevault/core/types.hpp"
#include <string>
#include <vector>

namespace filevault {
namespace utils {

/**
 * @brief Password utilities for secure input and strength analysis
 */
class Password {
public:
    /**
     * @brief Read password from terminal with masking
     * @param prompt Prompt message to display
     * @param confirm If true, ask for confirmation
     * @return Password string
     */
    static std::string read_secure(const std::string& prompt = "Password: ", 
                                   bool confirm = false);
    
    /**
     * @brief Analyze password strength
     * @param password Password to analyze
     * @return Detailed analysis with score and recommendations
     */
    static core::PasswordAnalysis analyze_strength(const std::string& password);
    
    /**
     * @brief Get strength color for display
     */
    static std::string get_strength_color(core::PasswordStrength strength);
    
    /**
     * @brief Get strength label
     */
    static std::string get_strength_label(core::PasswordStrength strength);
    
    /**
     * @brief Display password strength meter
     */
    static void display_strength_meter(const core::PasswordAnalysis& analysis);

private:
    /**
     * @brief Platform-specific: Disable terminal echo
     */
    static void disable_echo();
    
    /**
     * @brief Platform-specific: Enable terminal echo
     */
    static void enable_echo();
    
    /**
     * @brief Check if password is in common password list
     */
    static bool is_common_password(const std::string& password);
    
    /**
     * @brief Calculate password entropy
     */
    static double calculate_entropy(const std::string& password);
    
    /**
     * @brief Estimate crack time
     */
    static std::pair<std::string, std::string> estimate_crack_time(
        double entropy, 
        const core::PasswordAnalysis& analysis
    );
    
    // Top 100 most common passwords
    static const std::vector<std::string> common_passwords_;
};

} // namespace utils
} // namespace filevault

#endif // FILEVAULT_UTILS_PASSWORD_HPP

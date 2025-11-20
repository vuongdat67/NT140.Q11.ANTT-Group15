#include "filevault/utils/password.hpp"
#include "filevault/utils/console.hpp"
#include <fmt/core.h>
#include <fmt/color.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <unordered_map>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

namespace filevault {
namespace utils {

// Top 100 common passwords
const std::vector<std::string> Password::common_passwords_ = {
    "password", "123456", "12345678", "qwerty", "abc123", "monkey", "1234567",
    "letmein", "trustno1", "dragon", "baseball", "111111", "iloveyou", "master",
    "sunshine", "ashley", "bailey", "passw0rd", "shadow", "123123", "654321",
    "superman", "qazwsx", "michael", "football", "welcome", "jesus", "ninja",
    "mustang", "password1", "123456789", "adobe123", "admin", "1234567890",
    "photoshop", "1234", "12345", "000000", "computer", "test", "qwerty123"
};

void Password::disable_echo() {
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
#else
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

void Password::enable_echo() {
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode |= ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
#else
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

std::string Password::read_secure(const std::string& prompt, bool confirm) {
    std::string password;
    
    while (true) {
        // First input
        fmt::print(fmt::emphasis::bold, "{}", prompt);
        std::cout.flush();
        
        disable_echo();
        if (!std::getline(std::cin, password)) {
            enable_echo();
            // EOF or error - cannot continue interactively
            return "";
        }
        enable_echo();
        
        fmt::print("\n");
        
        if (password.empty()) {
            Console::warning("Password cannot be empty!");
            continue;
        }
        
        // Analyze strength
        auto analysis = analyze_strength(password);
        
        if (analysis.strength == core::PasswordStrength::VERY_WEAK || 
            analysis.strength == core::PasswordStrength::WEAK) {
            display_strength_meter(analysis);
            Console::warning("Password is weak. Consider using a stronger password.");
            
            // Ask if user wants to continue with weak password
            fmt::print("\nUse this password anyway? [y/N]: ");
            std::string response;
            if (!std::getline(std::cin, response)) {
                // EOF or error - cannot continue interactively
                Console::error("Cannot read input. Aborting.");
                return "";
            }
            if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
                continue;
            }
        }
        
        // Confirmation
        if (confirm) {
            fmt::print(fmt::emphasis::bold, "Confirm password: ");
            std::cout.flush();
            
            std::string confirm_password;
            disable_echo();
            if (!std::getline(std::cin, confirm_password)) {
                enable_echo();
                // EOF or error - cannot continue interactively
                Console::error("Cannot read confirmation. Aborting.");
                return "";
            }
            enable_echo();
            
            fmt::print("\n");
            
            if (password != confirm_password) {
                Console::error("Passwords do not match!");
                continue;
            }
        }
        
        break;
    }
    
    return password;
}

core::PasswordAnalysis Password::analyze_strength(const std::string& password) {
    core::PasswordAnalysis analysis;
    analysis.length = password.length();
    
    // Character class detection
    analysis.has_lowercase = false;
    analysis.has_uppercase = false;
    analysis.has_digits = false;
    analysis.has_special = false;
    analysis.has_repeated_chars = false;
    
    std::unordered_map<char, int> char_count;
    
    for (char c : password) {
        if (std::islower(c)) analysis.has_lowercase = true;
        if (std::isupper(c)) analysis.has_uppercase = true;
        if (std::isdigit(c)) analysis.has_digits = true;
        if (!std::isalnum(c)) analysis.has_special = true;
        
        char_count[c]++;
        if (char_count[c] > 2) {
            analysis.has_repeated_chars = true;
        }
    }
    
    // Check common passwords
    std::string lower_password = password;
    std::transform(lower_password.begin(), lower_password.end(), 
                   lower_password.begin(), ::tolower);
    
    analysis.is_common_password = is_common_password(lower_password);
    
    // Calculate score (0-100)
    int score = 0;
    
    // Length bonus
    if (analysis.length >= 8) score += 20;
    if (analysis.length >= 12) score += 10;
    if (analysis.length >= 16) score += 10;
    if (analysis.length >= 20) score += 10;
    
    // Character variety
    if (analysis.has_lowercase) score += 10;
    if (analysis.has_uppercase) score += 10;
    if (analysis.has_digits) score += 10;
    if (analysis.has_special) score += 15;
    
    // Penalties
    if (analysis.length < 8) score -= 30;
    if (analysis.has_repeated_chars) score -= 10;
    if (analysis.is_common_password) score -= 50;
    if (!analysis.has_special && !analysis.has_digits) score -= 20;
    
    analysis.score = std::clamp(score, 0, 100);
    
    // Determine strength
    if (analysis.score < 20) {
        analysis.strength = core::PasswordStrength::VERY_WEAK;
    } else if (analysis.score < 40) {
        analysis.strength = core::PasswordStrength::WEAK;
    } else if (analysis.score < 60) {
        analysis.strength = core::PasswordStrength::FAIR;
    } else if (analysis.score < 80) {
        analysis.strength = core::PasswordStrength::STRONG;
    } else {
        analysis.strength = core::PasswordStrength::VERY_STRONG;
    }
    
    // Generate warnings and suggestions
    if (analysis.length < 8) {
        analysis.warnings.push_back("Too short (minimum 8 characters)");
        analysis.suggestions.push_back("Use at least 12 characters");
    }
    
    if (analysis.is_common_password) {
        analysis.warnings.push_back("Common password - easily guessed!");
        analysis.suggestions.push_back("Use a unique, unpredictable password");
    }
    
    if (!analysis.has_lowercase || !analysis.has_uppercase) {
        analysis.warnings.push_back("Missing mixed case");
        analysis.suggestions.push_back("Use both uppercase and lowercase letters");
    }
    
    if (!analysis.has_digits) {
        analysis.warnings.push_back("No numbers");
        analysis.suggestions.push_back("Include numbers (0-9)");
    }
    
    if (!analysis.has_special) {
        analysis.warnings.push_back("No special characters");
        analysis.suggestions.push_back("Include symbols (!@#$%^&*)");
    }
    
    if (analysis.has_repeated_chars) {
        analysis.warnings.push_back("Repeated characters detected");
        analysis.suggestions.push_back("Avoid repeated patterns");
    }
    
    // Calculate entropy and crack time
    double entropy = calculate_entropy(password);
    auto [online_time, offline_time] = estimate_crack_time(entropy, analysis);
    analysis.crack_time_online = online_time;
    analysis.crack_time_offline = offline_time;
    
    return analysis;
}

bool Password::is_common_password(const std::string& password) {
    return std::find(common_passwords_.begin(), common_passwords_.end(), password) 
           != common_passwords_.end();
}

double Password::calculate_entropy(const std::string& password) {
    int charset_size = 0;
    
    bool has_lower = false, has_upper = false;
    bool has_digit = false, has_special = false;
    
    for (char c : password) {
        if (std::islower(c)) has_lower = true;
        if (std::isupper(c)) has_upper = true;
        if (std::isdigit(c)) has_digit = true;
        if (!std::isalnum(c)) has_special = true;
    }
    
    if (has_lower) charset_size += 26;
    if (has_upper) charset_size += 26;
    if (has_digit) charset_size += 10;
    if (has_special) charset_size += 32;  // Common special chars
    
    if (charset_size == 0) charset_size = 1;  // Avoid log(0)
    
    return password.length() * std::log2(charset_size);
}

std::pair<std::string, std::string> Password::estimate_crack_time(
    double entropy,
    const core::PasswordAnalysis& analysis
) {
    // Online attack: 1000 guesses/second (rate-limited)
    // Offline attack: 10 billion guesses/second (GPU cluster)
    
    double guesses = std::pow(2.0, entropy);
    
    // Common password → instant crack
    if (analysis.is_common_password) {
        return {"< 1 second", "< 1 second"};
    }
    
    // Online attack
    double online_seconds = guesses / 1000.0;
    std::string online_time;
    
    if (online_seconds < 1) online_time = "< 1 second";
    else if (online_seconds < 60) online_time = fmt::format("{:.0f} seconds", online_seconds);
    else if (online_seconds < 3600) online_time = fmt::format("{:.0f} minutes", online_seconds / 60);
    else if (online_seconds < 86400) online_time = fmt::format("{:.0f} hours", online_seconds / 3600);
    else if (online_seconds < 31536000) online_time = fmt::format("{:.0f} days", online_seconds / 86400);
    else if (online_seconds < 3153600000) online_time = fmt::format("{:.0f} years", online_seconds / 31536000);
    else online_time = "centuries";
    
    // Offline attack
    double offline_seconds = guesses / 10000000000.0;  // 10 billion/sec
    std::string offline_time;
    
    if (offline_seconds < 1) offline_time = "< 1 second";
    else if (offline_seconds < 60) offline_time = fmt::format("{:.0f} seconds", offline_seconds);
    else if (offline_seconds < 3600) offline_time = fmt::format("{:.0f} minutes", offline_seconds / 60);
    else if (offline_seconds < 86400) offline_time = fmt::format("{:.0f} hours", offline_seconds / 3600);
    else if (offline_seconds < 31536000) offline_time = fmt::format("{:.0f} days", offline_seconds / 86400);
    else if (offline_seconds < 3153600000) offline_time = fmt::format("{:.0f} years", offline_seconds / 31536000);
    else offline_time = "centuries";
    
    return {online_time, offline_time};
}

std::string Password::get_strength_color(core::PasswordStrength strength) {
    switch (strength) {
        case core::PasswordStrength::VERY_WEAK: return "red";
        case core::PasswordStrength::WEAK: return "orange";
        case core::PasswordStrength::FAIR: return "yellow";
        case core::PasswordStrength::STRONG: return "light_green";
        case core::PasswordStrength::VERY_STRONG: return "green";
        default: return "white";
    }
}

std::string Password::get_strength_label(core::PasswordStrength strength) {
    switch (strength) {
        case core::PasswordStrength::VERY_WEAK: return "VERY WEAK";
        case core::PasswordStrength::WEAK: return "WEAK";
        case core::PasswordStrength::FAIR: return "FAIR";
        case core::PasswordStrength::STRONG: return "STRONG";
        case core::PasswordStrength::VERY_STRONG: return "VERY STRONG";
        default: return "UNKNOWN";
    }
}

void Password::display_strength_meter(const core::PasswordAnalysis& analysis) {
    fmt::print("\n");
    fmt::print(fmt::emphasis::bold, "Password Strength: ");
    
    // Color-coded strength
    auto color_name = get_strength_color(analysis.strength);
    fmt::color color;
    
    if (color_name == "red") color = fmt::color::red;
    else if (color_name == "orange") color = fmt::color::orange;
    else if (color_name == "yellow") color = fmt::color::yellow;
    else if (color_name == "light_green") color = fmt::color::light_green;
    else color = fmt::color::green;
    
    fmt::print(fmt::fg(color) | fmt::emphasis::bold, 
              "{} (Score: {}/100)\n", 
              get_strength_label(analysis.strength), 
              analysis.score);
    
    // Progress bar
    int bars = analysis.score / 5;  // 20 bars for 100
    fmt::print("[");
    for (int i = 0; i < 20; ++i) {
        if (i < bars) {
            fmt::print(fmt::fg(color), "█");
        } else {
            fmt::print("░");
        }
    }
    fmt::print("]\n");
    
    // Warnings
    if (!analysis.warnings.empty()) {
        fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "\nWarnings:\n");
        for (const auto& warning : analysis.warnings) {
            fmt::print("  • {}\n", warning);
        }
    }
    
    // Suggestions
    if (!analysis.suggestions.empty()) {
        fmt::print(fmt::fg(fmt::color::cyan), "\nSuggestions:\n");
        for (const auto& suggestion : analysis.suggestions) {
            fmt::print("  • {}\n", suggestion);
        }
    }
    
    // Crack time estimates
    fmt::print("\nEstimated crack time:\n");
    fmt::print("  Online attack:  {}\n", analysis.crack_time_online);
    fmt::print("  Offline attack: {}\n", analysis.crack_time_offline);
    fmt::print("\n");
}

} // namespace utils
} // namespace filevault

#pragma once

#include <exception>
#include <string>

namespace filevault {

/**
 * Base exception class for all FileVault errors
 */
class FileVaultException : public std::exception {
protected:
    std::string message_;
    std::string context_;

public:
    explicit FileVaultException(std::string message, std::string context = "")
        : message_(std::move(message)), context_(std::move(context)) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    const std::string& context() const noexcept {
        return context_;
    }
};

/**
 * Cryptographic operation errors
 */
class CryptoException : public FileVaultException {
    using FileVaultException::FileVaultException;
};

/**
 * Authentication/integrity check failed
 */
class AuthenticationFailedException : public CryptoException {
public:
    AuthenticationFailedException()
        : CryptoException("Authentication failed - wrong password or corrupted file") {}
};

/**
 * File not found errors
 */
class FileNotFoundException : public FileVaultException {
public:
    explicit FileNotFoundException(const std::string& path)
        : FileVaultException("File not found", path) {}
};

/**
 * File I/O errors
 */
class FileIOException : public FileVaultException {
    using FileVaultException::FileVaultException;
};

/**
 * Invalid file format
 */
class InvalidFormatException : public FileVaultException {
public:
    explicit InvalidFormatException(const std::string& reason)
        : FileVaultException("Invalid file format: " + reason) {}
};

/**
 * Unsupported version
 */
class UnsupportedVersionException : public FileVaultException {
public:
    explicit UnsupportedVersionException(uint8_t major, uint8_t minor)
        : FileVaultException("Unsupported version: " + std::to_string(major) + "." + std::to_string(minor)) {}
};

/**
 * Compression/decompression errors
 */
class CompressionException : public FileVaultException {
    using FileVaultException::FileVaultException;
};

/**
 * Invalid argument errors
 */
class InvalidArgumentException : public FileVaultException {
    using FileVaultException::FileVaultException;
};

} // namespace filevault

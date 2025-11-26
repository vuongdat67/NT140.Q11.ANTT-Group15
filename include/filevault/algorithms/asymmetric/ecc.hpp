/**
 * @file ecc.hpp
 * @brief Elliptic Curve Cryptography (ECC) implementation
 * 
 * Provides ECDH key exchange and ECDSA digital signatures
 * using standard NIST curves.
 */

#ifndef FILEVAULT_ALGORITHMS_ASYMMETRIC_ECC_HPP
#define FILEVAULT_ALGORITHMS_ASYMMETRIC_ECC_HPP

#include "filevault/core/crypto_algorithm.hpp"
#include <botan/ecdh.h>
#include <botan/ecdsa.h>
#include <botan/ec_group.h>
#include <string>
#include <vector>

namespace filevault {
namespace algorithms {
namespace asymmetric {

/**
 * @brief Supported elliptic curves
 */
enum class ECCurve {
    SECP256R1,  // P-256, 128-bit security
    SECP384R1,  // P-384, 192-bit security
    SECP521R1,  // P-521, 256-bit security
    X25519      // Curve25519, 128-bit security (for ECDH only)
};

/**
 * @brief ECC key pair (ECDH or ECDSA)
 */
struct ECCKeyPair {
    std::vector<uint8_t> public_key;   // Raw or DER encoded
    std::vector<uint8_t> private_key;  // Raw or DER encoded
    ECCurve curve;
    std::string curve_name;
};

/**
 * @brief ECDH Key Exchange result
 */
struct ECDHResult {
    bool success;
    std::vector<uint8_t> shared_secret;  // Derived shared secret
    std::string error_message;
};

/**
 * @brief ECDSA signature result
 */
struct ECDSASignResult {
    bool success;
    std::vector<uint8_t> signature;
    std::string error_message;
};

/**
 * @brief Elliptic Curve Diffie-Hellman (ECDH) key exchange
 * 
 * Used for secure key agreement between two parties.
 * Each party generates a key pair, exchanges public keys,
 * and derives the same shared secret.
 */
class ECDH {
public:
    /**
     * @brief Construct ECDH with specified curve
     * @param curve Elliptic curve to use
     */
    explicit ECDH(ECCurve curve = ECCurve::SECP256R1);
    
    ~ECDH() = default;
    
    /**
     * @brief Generate a new ECDH key pair
     */
    ECCKeyPair generate_key_pair();
    
    /**
     * @brief Derive shared secret from own private key and peer's public key
     * @param own_private_key Our private key
     * @param peer_public_key Peer's public key
     * @return Shared secret that both parties can derive
     */
    ECDHResult derive_shared_secret(
        std::span<const uint8_t> own_private_key,
        std::span<const uint8_t> peer_public_key
    );
    
    std::string name() const;
    std::string curve_name() const;
    size_t key_size() const;  // In bytes
    
private:
    ECCurve curve_;
    std::string botan_curve_name_;
};

/**
 * @brief Elliptic Curve Digital Signature Algorithm (ECDSA)
 * 
 * Used for digital signatures - proving authenticity and integrity.
 */
class ECDSA {
public:
    /**
     * @brief Construct ECDSA with specified curve
     * @param curve Elliptic curve to use
     */
    explicit ECDSA(ECCurve curve = ECCurve::SECP256R1);
    
    ~ECDSA() = default;
    
    /**
     * @brief Generate a new ECDSA key pair
     */
    ECCKeyPair generate_key_pair();
    
    /**
     * @brief Sign data with private key
     * @param data Data to sign
     * @param private_key ECDSA private key
     * @return Signature
     */
    ECDSASignResult sign(
        std::span<const uint8_t> data,
        std::span<const uint8_t> private_key
    );
    
    /**
     * @brief Verify signature with public key
     * @param data Original data
     * @param signature Signature to verify
     * @param public_key ECDSA public key
     * @return true if signature is valid
     */
    bool verify(
        std::span<const uint8_t> data,
        std::span<const uint8_t> signature,
        std::span<const uint8_t> public_key
    );
    
    std::string name() const;
    std::string curve_name() const;
    size_t key_size() const;
    size_t signature_size() const;
    
private:
    ECCurve curve_;
    std::string botan_curve_name_;
};

/**
 * @brief Hybrid encryption using ECDH + AES-GCM
 * 
 * Combines the security of ECC key exchange with the efficiency
 * of symmetric encryption for encrypting arbitrary-sized data.
 * 
 * Encryption flow:
 * 1. Generate ephemeral ECDH key pair
 * 2. Derive shared secret with recipient's public key
 * 3. Use KDF to derive AES key from shared secret
 * 4. Encrypt data with AES-GCM
 * 5. Output: ephemeral public key + nonce + ciphertext + tag
 */
class ECCHybrid : public core::ICryptoAlgorithm {
public:
    /**
     * @brief Construct ECC hybrid encryption
     * @param curve Elliptic curve to use
     */
    explicit ECCHybrid(ECCurve curve = ECCurve::SECP256R1);
    
    ~ECCHybrid() override = default;
    
    std::string name() const override;
    core::AlgorithmType type() const override;
    
    /**
     * @brief Encrypt with recipient's public key
     * @param plaintext Data to encrypt
     * @param key Recipient's public key
     * @param config Encryption configuration
     */
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    /**
     * @brief Decrypt with own private key
     * @param ciphertext Encrypted data (includes ephemeral public key)
     * @param key Own private key
     * @param config Encryption configuration
     */
    core::CryptoResult decrypt(
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    /**
     * @brief Generate a new ECC key pair for this hybrid scheme
     */
    ECCKeyPair generate_key_pair();
    
    size_t key_size() const override;
    bool is_suitable_for(core::SecurityLevel level) const override;
    
private:
    ECCurve curve_;
    std::string botan_curve_name_;
    core::AlgorithmType type_;
    ECDH ecdh_;
};

} // namespace asymmetric
} // namespace algorithms
} // namespace filevault

#endif

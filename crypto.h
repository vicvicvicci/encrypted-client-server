// header file to store and organise openssl cryptographic functions

#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/evp.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// Hashes raw data with SHA-512 and returns a readable hexadecimal string
inline std::string calculate_sha512_hex(const unsigned char* data, size_t len) {
    std::vector<unsigned char> hash(64);
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    
    EVP_DigestInit_ex(ctx, EVP_sha512(), nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, hash.data(), nullptr);
    EVP_MD_CTX_free(ctx);

    // Convert binary digest to a 128-character hex string
    std::stringstream ss;
    for (unsigned char b : hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

#endif // CRYPTO_H


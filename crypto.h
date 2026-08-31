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

// aes encryption

inline std::vector<unsigned char> encrypt_aes256 (const std::string& plaintext, const unsigned char* key, const unsigned char* iv ){
    // helper function returns raw std::vector<unsigned char>

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new(); // cipher context objected instantiated on the heap -> internal state engine
    std::vector<unsigned char> ciphertext(plaintext.length()+16); // ciphertext length is at most plaintext length + 16 bytes (for padding)

    int len = 0, ciphertext_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv); // initialises ctx context for encryption and uses aes256cbc algorithm
    // nullptr default openssl engine
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (const unsigned char*) plaintext.data(), plaintext.length());
    // encrypts as many full 16 byte data from plaintext as possible, storing them in ciphertext.data and keeping count of bytes written via len
    ciphertext_len = ciphertext_len + len;

    EVP_EncryptFinal(ctx, ciphertext.data() + len, &len); // handles final block, ciphertext.data() does what
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx); // free memory allocated for ctx - prevent memory leaks
    ciphertext.resize(ciphertext_len); // resize vector to actual length of ciphertext
    return ciphertext;
} 

// aes decryption

inline std::string decrypt_aes256 (const std::vector<unsigned char>& ciphertext, const unsigned char* key, const unsigned char* iv){

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    // prepare a buffer for the decrypted text, which will be at most the size of the ciphertext
    std::vector<unsigned char> plaintext(ciphertext.size());

    int len = 0, plaintext_len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()); // uses .size() instead of .length() because ciphertext is a vector, not a string
    plaintext_len += len;

    EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintext_len); // resize vector to actual length of plaintext
    return std::string((char*)plaintext.data(), plaintext_len); // convert vector to string
}

#endif // CRYPTO_H
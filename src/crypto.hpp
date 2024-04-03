#include <string>
#include <string_view>
#include <vector>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>

using namespace CryptoPP;

std::string sha256(std::string_view str)
{
    CryptoPP::SHA256 hash;
    std::string digest;
    CryptoPP::StringSource s(str.data(), true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
    return digest;
}

bool verify(const std::string &message, const std::string &signature, const std::string &address)
{
    ECDSA<ECP, SHA256>::PublicKey publicKey;
    std::string publicKeyStr;
    StringSource ss(address, true, new HexDecoder(new StringSink(publicKeyStr)));
    StringSource ss2(publicKeyStr, true);
    publicKey.Load(ss2);

    ECDSA<ECP, SHA256>::Verifier verifier(publicKey);
    std::string hash = sha256(message);
    CryptoPP::byte *hashBytes = reinterpret_cast<CryptoPP::byte *>(hash.data());

    std::vector<CryptoPP::byte> signatureBytes;
    CryptoPP::StringSource ss3(signature, true, new CryptoPP::HexDecoder(new CryptoPP::VectorSink(signatureBytes)));

    return verifier.VerifyMessage(hashBytes, hash.size(), signatureBytes.data(), signatureBytes.size());
}
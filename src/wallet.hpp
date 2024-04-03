/**
 * todo: 不管是has256还是密钥生成，它们的十六进制前面的几位都一样，需要改进，选择其他的加密算法
 *
 *
 */
#include <iostream>
#include <fstream>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>
#include <cryptopp/asn.h>
#include <cryptopp/ecp.h>
#include <cryptopp/integer.h>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

#include "crypto.hpp"

using namespace CryptoPP;
using namespace std;

class Wallet
{
public:
    Wallet() {}
    Wallet(const string &filename) { loadFromFile(filename); }

    ~Wallet() {}

    void init()
    {
        AutoSeededRandomPool rng;
        privateKey.Initialize(rng, CryptoPP::ASN1::secp256r1());
        privateKey.MakePublicKey(publicKey);
    }

    string sign(const string &message)
    {
        AutoSeededRandomPool rng;
        ECDSA<ECP, SHA256>::Signer signer(privateKey);

        string hash = sha256(message);
        CryptoPP::byte *hashBytes = reinterpret_cast<CryptoPP::byte *>(hash.data());

        size_t signatureSize = signer.MaxSignatureLength();
        // cout << "signatureSize_before: " << signatureSize << endl;
        CryptoPP::byte signature[signatureSize];
        signatureSize = signer.SignMessage(rng, hashBytes, hash.size(), signature);
        // cout << "signatureSize_after: " << signatureSize << endl;
        string hex;
        StringSource(signature, signatureSize, true, new HexEncoder(new StringSink(hex)));
        return hex;
    }

    void saveToFile(const string &filename)
    {
        string privateKeyStr;
        privateKey.Save(StringSink(privateKeyStr).Ref());
        string hex;
        StringSource(privateKeyStr, true, new HexEncoder(new StringSink(hex)));

        // 使用std库的流保存到文件
        std::ofstream file{filename};
        if (file.is_open())
        {
            cout << "saveToFile: " << hex << endl;
            file << hex;
            file.close();
        }
        else
        {
            cout << filename << endl;
            throw std::runtime_error("Cannot open file to write");
        }
    }

    void loadFromFile(const string &filename)
    {
        std::ifstream file{filename};
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file to read");
        }
        string hex;
        file >> hex;
        file.close();
        cout << "loadFromFile: " << hex << endl;

        string privateKeyStr;
        StringSource ss1(hex, true, new HexDecoder(new StringSink(privateKeyStr)));
        StringSource ss2(privateKeyStr, true);
        privateKey.Load(ss2);
        privateKey.MakePublicKey(publicKey);
        cout << "publicKey: " << getAddress() << endl;
    }
    /**互相引用的麻烦*/
    // auto transfer(string_view to, const int &amount)
    // {
    //     /* todo 排查是否有内存泄漏的风险*/
    //     auto tx = make_unique<Transaction>(getAddress(), to, amount);
    //     tx->sign(*this);
    //     return tx;
    // }

    // todo 破坏封装性，需要修改
    const auto &getPublicKey() const { return publicKey; }

    const string getAddress() const
    {
        string publicKeyStr;
        publicKey.Save(StringSink(publicKeyStr).Ref());
        string hex;
        StringSource(publicKeyStr, true, new HexEncoder(new StringSink(hex)));
        return hex;
    }

private:
    ECDSA<ECP, SHA256>::PrivateKey privateKey;
    ECDSA<ECP, SHA256>::PublicKey publicKey;
};

// int main()
// {
//     // Wallet alice{};
//     // alice.saveToFile("alice.key");

//     Wallet alice{"alice.key"};

//     // 待签名内容（发送的文本内容）
//     string message = "I am MKing Hello Everyone";
//     string message2 = "I am MKing Hello Everyone ";

//     // 签名
//     string signature = alice.sign(message);
//     string signature2 = alice.sign(message2);
//     cout << "signature: " << signature << endl;
//     cout << "signature2: " << signature2 << endl;

//     cout << verify(message, signature, alice.getAddress()) << endl;
//     cout << verify(message2, signature2, alice.getAddress()) << endl;

//     return 0;
// }
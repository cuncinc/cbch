#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <ostream>
#include <thread>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include "../lib/json.hpp"
#include "wallet.hpp"

using namespace std;
using json = nlohmann::json;

class Transaction
{
public:
    Transaction(string_view from, string_view to, const int &amount)
        : from{from}, to{to}, amount{amount} {}
    ~Transaction() {}
    // void setSig(string_view sig) { this->sig = sig; }

    bool isValid()
    {
        if (to.empty() || amount < 0)
        {
            return false;
        }
        return verifySig();
    }

    bool operator==(const Transaction &t)
    {
        return from == t.from && to == t.to && amount == t.amount && sig == t.sig;
    }

    string toJson()
    {
        json j = {
            {"from", from},
            {"to", to},
            {"amount", amount},
            {"sig", sig}};
        return j.dump();
    }

    bool verifySig()
    {
        if (from.empty()) // 区块奖励
        {
            return true;
        }
        return verify(getMessge(), sig, from);
    }

    void sign(Wallet &w)
    {
        sig = w.sign(getMessge());
    }

    friend ostream &operator<<(ostream &os, Transaction &t)
    {
        os << t.toJson() << endl;
        return os;
    }

private:
    string from;
    string to;
    int amount;
    string sig;
    string getMessge()
    {
        return from + to + to_string(amount);
    }
};

auto now = []
{ return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count(); };

auto timeUtil(int64_t start, int64_t end)
{
    static long count = 0;
    static long average = 0;

    auto duration = end - start;
    average = (average * count + duration) / (count + 1);
    count++;
    return average;
}

class Block
{
public:
    Block(Block &preBlock, vector<Transaction> &trans) // normal block
        : trans{trans}, nonce{0}, timestamp{now()}, height{preBlock.height + 1}, prevHash{preBlock.hash}
    {
        merkle = culcMerkleRoot();
    }
    Block(Transaction t) // genesis block
        : trans{t}, nonce{0}, timestamp{now()}, height{0}, prevHash{""}
    {
        merkle = culcMerkleRoot();
        hash = calcHash();
    }
    ~Block() {}
    string getHash() { return hash; }

    vector<Transaction> &getTrans()
    {
        return trans;
    }

    string culcMerkleRoot()
    {
        // todo: 计算merkle root未完成
        string data = transJson().dump();
        return sha256(data);
    }

    string calcHash()
    {
        if (merkle.empty())
        {
            merkle = culcMerkleRoot();
        }
        string data = to_string(timestamp) + prevHash + merkle + to_string(nonce);
        string hash = sha256(data);
        return hash;
    }

    string toJson()
    {
        json j = {
            {"trans", transJson()},
            {"merkle", merkle},
            {"hash", hash},
            {"height", height},
            {"prevHash", prevHash},
            {"nonce", nonce},
            {"timestamp", timestamp}};
        return j.dump();
    }

    string mineBlock(const int &difficulty)
    {
        string target(difficulty, '0');
        auto t1 = now(); // 记录挖矿开始时间
        while (hash.substr(0, difficulty) != target)
        {
            nonce++;
            hash = calcHash();
        }
        auto t2 = now(); // 记录挖矿结束时间
        auto average = timeUtil(t1, t2);
        auto duration = t2 - t1;
        cout << "Block mined: " << height << " | " << nonce << " | this: " << duration << "ms | avg: " << average << "ms" << endl;

        return hash;
    }

    bool isValid(Block &preBlock) // 如何验证创世区块
    {
        // check hash
        if (hash != calcHash())
        {
            cout << "Block hash is invalid: " << height << " " << hash << " != " << calcHash() << endl;
            return false;
        }
        else if (prevHash != preBlock.getHash())
        {
            cout << "Block prevHash is invalid: " << height << " " << prevHash << " != " << preBlock.getHash() << endl;
            return false;
        }

        for (auto &t : trans)
        {
            if (!t.isValid())
            {
                cout << "Transaction is invalid: " << height << " " << t << endl;
                return false;
            }
        }
        return true;
    }

    friend ostream &operator<<(ostream &os, Block &b)
    {
        os << b.toJson() << endl;
        return os;
    }

private:
    vector<Transaction> trans;
    string hash;
    int height;
    string prevHash;
    string merkle;
    int nonce;
    time_t timestamp;

    json transJson()
    {
        json transJson = json::array();
        for (auto &t : trans)
        {
            transJson.push_back(json::parse(t.toJson()));
        }
        // cout << "transJson: " << transJson << endl;
        return transJson;
    }
};

class Chain
{
public:
    Chain() : difficulty{5}
    {
        Transaction t{"", "me", 10000};
        Block genesisBlock{t};
        cout << "genesis block created: " << genesisBlock << endl;
        blocks.push_back(genesisBlock);
    }

    ~Chain() {}

    void addBlock(Block &b)
    {
        if (b.isValid(getLastBlock()))
        {
            cout << "new_block: " << b << endl;
            blocks.push_back(b);
        }
    }

    // clear elements in peddingTrans that are in v
    void removePeddingTrans(vector<Transaction> &v)
    {
        for (auto &t : v)
        {
            auto it = find(peddingTrans.begin(), peddingTrans.end(), t);
            if (it != peddingTrans.end())
            {
                peddingTrans.erase(it);
            }
        }
    }

    Block &getLastBlock()
    {
        return blocks.back();
    }

    int getMinerReward() const
    {
        return minerReward;
    }

    void setMinerReward(int reward)
    {
        minerReward = reward;
    }

    void addTrans(Transaction &t)
    {
        peddingTrans.push_back(t);
    }

    void mine(string_view minerAddr)
    {
        if (peddingTrans.empty())
        {
            this_thread::sleep_for(chrono::milliseconds(500));
            return;
        }
        cout << "Mining..." << endl;
        // 验证交易的签名
        for (auto &t : peddingTrans)
        {
            if (!t.isValid())
            {
                cout << "Transaction is invalid: " << t << endl;
                return;
            }
        }
        // 打包交易
        vector packedTrans{peddingTrans};
        // 添加挖矿奖励
        Transaction reward{"", minerAddr, getMinerReward()};
        packedTrans.push_back(reward);
        // 创建新区块
        Block b{getLastBlock(), packedTrans};
        // 挖矿
        b.mineBlock(difficulty);

        addBlock(b);

        // 清除peddingTrans中的packedTrans
        removePeddingTrans(packedTrans);
    }

    bool isValid()
    {
        auto genesis = blocks.begin(); // genesis是创世区块
        auto first = genesis + 1;      // first是除了创世区块外的第一个区块
        /*todo: 没有验证创世区块 */
        for (auto it = first; it != blocks.end(); ++it)
        {
            auto &preBlock = *(it - 1);
            if (!it->isValid(preBlock))
            {
                return false;
            }
        }

        return true;
    }

private:
    int difficulty;
    int minerReward = 50;
    vector<Block> blocks;
    vector<Transaction> peddingTrans;
};

void genTxs(Chain &chain, Wallet &from, string_view to)
{
    for (;;)
    {
        int ramdom = rand() % 10000;
        Transaction t{from.getAddress(), to, ramdom};
        t.sign(from);
        cout << "new transaction: " << ramdom << endl;
        chain.addTrans(t);
        // 等待一段时间
        // int ramdomTime = 6000 + rand() % 5000;
        int ramdomTime = rand() % 5000;
        this_thread::sleep_for(chrono::milliseconds(ramdomTime));
    }
}

int main(int argc, char *argv[])
{
    Wallet alice{};
    Wallet bob{};
    Wallet miner{};

    // 命令行参数有-r，则重新生成密钥对，否则读取密钥对
    if (argc == 2)
    {
        if (string(argv[1]) == "-r")
        {
            alice.init();
            bob.init();
            miner.init();

            alice.saveToFile("run/alice.key");
            bob.saveToFile("run/bob.key");
            miner.saveToFile("run/miner.key");
        }
        else
        {
            cout << "Usage: " << argv[0] << " [-r]" << endl;
            return 1;
        }
    }
    else
    {
        alice.loadFromFile("run/alice.key");
        bob.loadFromFile("run/bob.key");
        miner.loadFromFile("run/miner.key");
    }

    string aliceAddr = alice.getAddress();
    string bobAddr = bob.getAddress();
    string minerAddr = miner.getAddress();

    Chain blockchain;

    // thread to generate transactions
    thread genTxThread{genTxs, ref(blockchain), ref(alice), bobAddr};
    genTxThread.detach();

    // main thread to mine
    for (;;)
    {
        blockchain.mine(minerAddr);
    }

    cout << "isValid: " << (blockchain.isValid() ? "ture" : "false") << endl;
}
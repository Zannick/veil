// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2019-2022 The Veil developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PIVX_STAKEINPUT_H
#define PIVX_STAKEINPUT_H

#include "veil/ringct/transactionrecord.h"
#include "veil/ringct/transactionsigcontext.h"
#include "veil/zerocoin/accumulatormap.h"
#include "chain.h"
#include "streams.h"

#include "libzerocoin/CoinSpend.h"

class AnonWallet;
class CKeyStore;
class CWallet;
class CWalletTx;
class COutputR;

class CStakeInput
{
protected:
    CBlockIndex* pindexFrom;
    libzerocoin::CoinDenomination denom = libzerocoin::CoinDenomination::ZQ_ERROR;

public:
    virtual ~CStakeInput(){};
    virtual CBlockIndex* GetIndexFrom() = 0;
    virtual bool GetTxFrom(CTransaction& tx) = 0;
    virtual CAmount GetValue() = 0;
    virtual CAmount GetWeight() = 0;
    virtual bool GetModifier(uint64_t& nStakeModifier, const CBlockIndex* pindexChainPrev) = 0;
    virtual bool IsZerocoins() = 0;
    virtual CDataStream GetUniqueness() = 0;
    libzerocoin::CoinDenomination GetDenomination() {return denom;};

    virtual bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = uint256()) = 0;
    virtual bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOutBaseRef>& vpout, CAmount nTotal) = 0;
    virtual bool CompleteTx(CWallet* pwallet, CMutableTransaction& txNew) = 0;
};


class RingCTStake : public CStakeInput
{
private:
    static const int RING_SIZE = 32;

    const COutputR& coin;  // Contains: depth (coin.i), output record (coin.rtx->second)
    veil_ringct::TransactionInputsSigContext tx_inCtx;
    veil_ringct::TransactionOutputsSigContext tx_outCtx;
    CTransactionRecord rtx;

    CAmount GetBracketMinValue();

public:
    explicit RingCTStake(const COutputR& coin_) : coin(coin_), tx_inCtx(RING_SIZE, 2) { }

    CBlockIndex* GetIndexFrom() override;
    bool GetTxFrom(CTransaction& tx) override;
    CAmount GetValue() override;
    CAmount GetWeight() override;
    bool GetModifier(uint64_t& nStakeModifier, const CBlockIndex* pindexChainPrev) override;
    CDataStream GetUniqueness() override;

    bool IsZerocoins() override { return false; }

    bool MarkSpent(AnonWallet* panonwallet, CMutableTransaction& txNew);

    bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = uint256()) override;
    bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOutBaseRef>& vpout, CAmount nTotal) override;
    bool CompleteTx(CWallet* pwallet, CMutableTransaction& txNew) override;
};


// ZerocoinStake can take two forms
// 1) the stake candidate, which is a zcmint that is attempted to be staked
// 2) a staked zerocoin, which is a zcspend that has successfully staked
class ZerocoinStake : public CStakeInput
{
private:
    uint256 nChecksum;
    bool fMint;
    uint256 hashSerial;

public:
    explicit ZerocoinStake(libzerocoin::CoinDenomination denom, const uint256& hashSerial)
    {
        this->denom = denom;
        this->hashSerial = hashSerial;
        this->pindexFrom = nullptr;
        fMint = true;
    }

    explicit ZerocoinStake(const libzerocoin::CoinSpend& spend);

    CBlockIndex* GetIndexFrom() override;
    bool GetTxFrom(CTransaction& tx) override;
    CAmount GetValue() override;
    CAmount GetWeight() override;
    bool GetModifier(uint64_t& nStakeModifier, const CBlockIndex* pindexChainPrev) override;
    CDataStream GetUniqueness() override;
    bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = uint256()) override;
    bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOutBaseRef>& vpout, CAmount nTotal) override;
    bool CompleteTx(CWallet* pwallet, CMutableTransaction& txNew) override;
    bool IsZerocoins() override { return true; }

    bool MarkSpent(CWallet* pwallet, const uint256& txid);
    int GetChecksumHeightFromMint();
    int GetChecksumHeightFromSpend();
    uint256 GetChecksum();

    static int HeightToModifierHeight(int nHeight);
};

#endif //PIVX_STAKEINPUT_H

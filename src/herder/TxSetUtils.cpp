// Copyright 2022 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/asio.h"
#include "TxSetUtils.h"
#include "crypto/Hex.h"
#include "crypto/Random.h"
#include "crypto/SHA.h"
#include "database/Database.h"
#include "ledger/LedgerManager.h"
#include "ledger/LedgerTxn.h"
#include "ledger/LedgerTxnEntry.h"
#include "ledger/LedgerTxnHeader.h"
#include "main/Application.h"
#include "main/Config.h"
#include "transactions/MutableTransactionResult.h"
#include "transactions/TransactionUtils.h"
#include "util/GlobalChecks.h"
#include "util/Logging.h"
#include "util/ProtocolVersion.h"
#include "util/UnorderedSet.h"
#include "util/XDRCereal.h"
#include "util/XDROperators.h"
#include "xdrpp/marshal.h"

#include <Tracy.hpp>
#include <algorithm>
#include <list>
#include <numeric>

namespace stellar
{
namespace
{
// Target use case is to remove a subset of invalid transactions from a TxSet.
// I.e. txSet.size() >= txsToRemove.size()
TxSetTransactions
removeTxs(TxSetTransactions const& txs, TxSetTransactions const& txsToRemove)
{
    UnorderedSet<Hash> txsToRemoveSet;
    txsToRemoveSet.reserve(txsToRemove.size());
    std::transform(
        txsToRemove.cbegin(), txsToRemove.cend(),
        std::inserter(txsToRemoveSet, txsToRemoveSet.end()),
        [](TransactionFrameBasePtr const& tx) { return tx->getFullHash(); });

    TxSetTransactions newTxs;
    newTxs.reserve(txs.size() - txsToRemove.size());
    for (auto const& tx : txs)
    {
        if (txsToRemoveSet.find(tx->getFullHash()) == txsToRemoveSet.end())
        {
            newTxs.emplace_back(tx);
        }
    }

    return newTxs;
}
} // namespace

AccountTransactionQueue::AccountTransactionQueue(
    std::vector<TransactionFrameBasePtr> const& accountTxs)
    : mTxs(accountTxs.begin(), accountTxs.end())
{
    releaseAssert(!mTxs.empty());
    std::sort(mTxs.begin(), mTxs.end(),
              [](TransactionFrameBasePtr const& tx1,
                 TransactionFrameBasePtr const& tx2) {
                  return tx1->getSeqNum() < tx2->getSeqNum();
              });
    for (auto const& tx : accountTxs)
    {
        mNumOperations += tx->getNumOperations();
    }
}

TransactionFrameBasePtr
AccountTransactionQueue::getTopTx() const
{
    releaseAssert(!mTxs.empty());
    return mTxs.front();
}

bool
AccountTransactionQueue::empty() const
{
    return mTxs.empty();
}

void
AccountTransactionQueue::popTopTx()
{
    releaseAssert(!mTxs.empty());
    mNumOperations -= mTxs.front()->getNumOperations();
    mTxs.pop_front();
}

bool
TxSetUtils::hashTxSorter(TransactionFrameBasePtr const& tx1,
                         TransactionFrameBasePtr const& tx2)
{
    // need to use the hash of whole tx here since multiple txs could have
    // the same Contents
    return tx1->getFullHash() < tx2->getFullHash();
}

TxSetTransactions
TxSetUtils::sortTxsInHashOrder(TxSetTransactions const& transactions)
{
    ZoneScoped;
    TxSetTransactions sortedTxs(transactions);
    std::sort(sortedTxs.begin(), sortedTxs.end(), TxSetUtils::hashTxSorter);
    return sortedTxs;
}

std::vector<std::shared_ptr<AccountTransactionQueue>>
TxSetUtils::buildAccountTxQueues(TxSetTransactions const& txs)
{
    ZoneScoped;
    UnorderedMap<AccountID, std::vector<TransactionFrameBasePtr>> actTxMap;

    for (auto const& tx : txs)
    {
        auto id = tx->getSourceID();
        auto it =
            actTxMap.emplace(id, std::vector<TransactionFrameBasePtr>()).first;
        it->second.emplace_back(tx);
    }

    std::vector<std::shared_ptr<AccountTransactionQueue>> queues;
    for (auto const& [_, actTxs] : actTxMap)
    {
        queues.emplace_back(std::make_shared<AccountTransactionQueue>(actTxs));
    }
    return queues;
}

TxSetTransactions
TxSetUtils::getInvalidTxList(TxSetTransactions const& txs, Application& app,
                             uint64_t lowerBoundCloseTimeOffset,
                             uint64_t upperBoundCloseTimeOffset)
{
    ZoneScoped;
    LedgerTxn ltx(app.getLedgerTxnRoot(), /* shouldUpdateLastModified */ true,
                  TransactionMode::READ_ONLY_WITHOUT_SQL_TXN);
    // This is done so minSeqLedgerGap is validated against the next
    // ledgerSeq, which is what will be used at apply time
    ltx.loadHeader().current().ledgerSeq =
        app.getLedgerManager().getLastClosedLedgerNum() + 1;

    TxSetTransactions invalidTxs;

    for (auto const& tx : txs)
    {
        auto txResult = tx->checkValid(app, ltx, 0, lowerBoundCloseTimeOffset,
                                       upperBoundCloseTimeOffset);
        if (!txResult->isSuccess())
        {
            invalidTxs.emplace_back(tx);
        }
    }

    return invalidTxs;
}

TxSetTransactions
TxSetUtils::trimInvalid(TxSetTransactions const& txs, Application& app,
                        uint64_t lowerBoundCloseTimeOffset,
                        uint64_t upperBoundCloseTimeOffset,
                        TxSetTransactions& invalidTxs)
{
    invalidTxs = getInvalidTxList(txs, app, lowerBoundCloseTimeOffset,
                                  upperBoundCloseTimeOffset);
    return removeTxs(txs, invalidTxs);
}

} // namespace stellar

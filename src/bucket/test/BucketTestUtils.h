#pragma once

// Copyright 2022 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "test/TestUtils.h"

namespace stellar
{
namespace BucketTestUtils
{

uint32_t getAppLedgerVersion(Application& app);

uint32_t getAppLedgerVersion(std::shared_ptr<Application> app);

void for_versions_with_differing_bucket_logic(
    Config const& cfg, std::function<void(Config const&)> const& f);

struct EntryCounts
{
    size_t nMeta{0};
    size_t nInit{0};
    size_t nLive{0};
    size_t nDead{0};
    size_t
    sum() const
    {
        return nLive + nInit + nDead;
    }
    size_t
    sumIncludingMeta() const
    {
        return nLive + nInit + nDead + nMeta;
    }

    EntryCounts(std::shared_ptr<stellar::Bucket> bucket);
};

size_t countEntries(std::shared_ptr<stellar::Bucket> bucket);

Hash closeLedger(Application& app, std::optional<SecretKey> skToSignValue,
                 xdr::xvector<UpgradeType, 6> upgrades = emptyUpgradeSteps);

Hash closeLedger(Application& app);

class LedgerManagerForBucketTests : public LedgerManagerImpl
{
    bool mUseTestEntries{false};
    std::vector<LedgerEntry> mTestInitEntries;
    std::vector<LedgerEntry> mTestLiveEntries;
    std::vector<LedgerKey> mTestDeadEntries;

  protected:
    void transferLedgerEntriesToBucketList(
        AbstractLedgerTxn& ltx,
        std::unique_ptr<LedgerCloseMetaFrame> const& ledgerCloseMeta,
        uint32_t ledgerSeq, uint32_t currLedgerVers,
        uint32_t initialLedgerVers) override;

  public:
    void
    setNextLedgerEntryBatchForBucketTesting(
        std::vector<LedgerEntry> const& initEntries,
        std::vector<LedgerEntry> const& liveEntries,
        std::vector<LedgerKey> const& deadEntries)
    {
        mUseTestEntries = true;
        mTestInitEntries = initEntries;
        mTestLiveEntries = liveEntries;
        mTestDeadEntries = deadEntries;
    }

    LedgerManagerForBucketTests(Application& app) : LedgerManagerImpl(app)
    {
    }
};

class BucketTestApplication : public TestApplication
{
  public:
    BucketTestApplication(VirtualClock& clock, Config const& cfg)
        : TestApplication(clock, cfg)
    {
    }

    virtual LedgerManagerForBucketTests& getLedgerManager() override;

  private:
    virtual std::unique_ptr<LedgerManager>
    createLedgerManager() override
    {
        return std::make_unique<LedgerManagerForBucketTests>(*this);
    }
};
}
}
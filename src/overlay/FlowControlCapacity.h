#pragma once

// Copyright 2023 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "overlay/StellarXDR.h"

namespace stellar
{

class Application;

class FlowControlCapacity
{
  protected:
    Application& mApp;

    struct ReadingCapacity
    {
        uint64_t mFloodCapacity;
        uint64_t mTotalCapacity;
    };

    // Capacity of local node configured by the operator
    ReadingCapacity mCapacity;

    // Capacity of a connected peer
    uint64_t mOutboundCapacity{0};
    NodeID const& mNodeID;

  public:
    virtual uint64_t getMsgResourceCount(StellarMessage const& msg) const = 0;
    virtual ReadingCapacity getCapacityLimits() const = 0;
    virtual void releaseOutboundCapacity(StellarMessage const& msg) = 0;

    void lockOutboundCapacity(StellarMessage const& msg);
    bool lockLocalCapacity(StellarMessage const& msg);
    uint64_t releaseLocalCapacity(StellarMessage const& msg);

    bool hasOutboundCapacity(StellarMessage const& msg) const;
    void checkCapacityInvariants() const;
    ReadingCapacity
    getCapacity() const
    {
        return mCapacity;
    }

    uint64_t
    getOutboundCapacity() const
    {
        return mOutboundCapacity;
    }

#ifdef BUILD_TESTS
    void
    setOutboundCapacity(uint64_t newCapacity)
    {
        mOutboundCapacity = newCapacity;
    }
#endif

    FlowControlCapacity(Application& app, NodeID const& nodeID);
};

class FlowControlMessageCapacity : public FlowControlCapacity
{
  public:
    FlowControlMessageCapacity(Application& app, NodeID const& nodeID);
    virtual ~FlowControlMessageCapacity() = default;
    virtual uint64_t
    getMsgResourceCount(StellarMessage const& msg) const override;
    virtual ReadingCapacity getCapacityLimits() const override;
    void releaseOutboundCapacity(StellarMessage const& msg) override;
};
}
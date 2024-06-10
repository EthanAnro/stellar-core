// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/CreatePassiveSellOfferOpFrame.h"

namespace stellar
{

// change from CreatePassiveOfferOp to ManageOfferOp
ManageSellOfferOpHolder::ManageSellOfferOpHolder(Operation const& op)
{
    mCreateOp.body.type(MANAGE_SELL_OFFER);
    auto& manageOffer = mCreateOp.body.manageSellOfferOp();
    auto const& createPassiveOp = op.body.createPassiveSellOfferOp();
    manageOffer.amount = createPassiveOp.amount;
    manageOffer.buying = createPassiveOp.buying;
    manageOffer.selling = createPassiveOp.selling;
    manageOffer.offerID = 0;
    manageOffer.price = createPassiveOp.price;
    mCreateOp.sourceAccount = op.sourceAccount;
}

CreatePassiveSellOfferOpFrame::CreatePassiveSellOfferOpFrame(
    Operation const& op, TransactionFrame const& parentTx)
    : ManageSellOfferOpHolder(op)
    , ManageSellOfferOpFrame(mCreateOp, parentTx, true)
{
}
}

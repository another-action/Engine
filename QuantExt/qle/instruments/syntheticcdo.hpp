/*
 Copyright (C) 2017 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

/*! \file syntheticcdo.hpp
    \brief Synthetic Collateralized Debt Obligation and pricing engines
*/

#ifndef quantext_synthetic_cdo_hpp
#define quantext_synthetic_cdo_hpp

#include <ql/qldefines.hpp>

#ifndef QL_PATCH_SOLARIS

#include <ql/default.hpp>
#include <ql/instrument.hpp>
#include <ql/time/schedule.hpp>

//#include <ql/experimental/credit/basket.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/instruments/creditdefaultswap.hpp>
#include <qle/models/basket.hpp>

namespace QuantExt {
using namespace QuantLib;

// class YieldTermStructure;

//! Synthetic Collateralized Debt Obligation
/*!
  The instrument prices a mezzanine CDO tranche with loss given default
  between attachment point \f$ D_1\f$ and detachment point
  \f$ D_2 > D_1 \f$.

  For purchased protection, the instrument value is given by the difference
  of the protection value \f$ V_1 \f$ and premium value \f$ V_2 \f$,

  \f[ V = V_1 - V_2. \f]

  The protection leg is priced as follows:

  - Build the probability distribution for volume of defaults \f$ L \f$
  (before recovery) or Loss Given Default \f$ LGD = (1-r)\,L \f$ at
  times/dates \f$ t_i, i=1, ..., N\f$ (premium schedule times with
  intermediate steps)

  - Determine the expected value
  \f$ E_i = E_{t_i}\,\left[Pay(LGD)\right] \f$
  of the protection payoff \f$ Pay(LGD) \f$  at each time \f$ t_i\f$ where
  \f[
  Pay(L) = min (D_1, LGD) - min (D_2, LGD) = \left\{
  \begin{array}{lcl}
  \displaystyle 0 &;& LGD < D_1 \\
  \displaystyle LGD - D_1 &;& D_1 \leq LGD \leq D_2 \\
  \displaystyle D_2 - D_1 &;& LGD > D_2
  \end{array}
  \right.
  \f]

  - The protection value is then calculated as
  \f[ V_1 \:=\: \sum_{i=1}^N (E_i - E_{i-1}) \cdot  d_i \f]
  where \f$ d_i\f$ is the discount factor at time/date \f$ t_i \f$

  The premium is paid on the protected notional amount, initially
  \f$ D_2 - D_1. \f$ This notional amount is reduced by the expected
  protection
  payments \f$ E_i \f$ at times \f$ t_i, \f$ so that the premium value is
  calculated as

  \f[
  V_2 =m \, \cdot \sum_{i=1}^N \,(D_2 - D_1 - E_i) \cdot \Delta_{i-1,i}\,d_i
  \f]

  where \f$ m \f$ is the premium rate, \f$ \Delta_{i-1, i}\f$ is the day
  count fraction between date/time \f$ t_{i-1}\f$ and \f$ t_i.\f$

  The construction of the portfolio loss distribution \f$ E_i \f$ is
  based on the probability bucketing algorithm described in

  <strong>
  John Hull and Alan White, "Valuation of a CDO and nth to default CDS
  without Monte Carlo simulation", Journal of Derivatives 12, 2, 2004
  </strong>

  The pricing algorithm allows for varying notional amounts and
  default termstructures of the underlyings.

  \ingroup credit

  \todo Investigate and fix cases \f$ E_{i+1} < E_i. \f$
*/
class SyntheticCDO : public Instrument {
public:
    class arguments;
    class results;
    class engine;

    // Review: No accrual settlement flag. No separate upfront payment date.
    // Review: Forward start case.
    /*! If the notional exceeds the basket inception tranche
        notional, the cdo is leveraged by that factor.

        \todo: allow for extra payment flags, arbitrary upfront
               payment date...
    */
    // RL: QuantExt version
    SyntheticCDO(const QuantLib::ext::shared_ptr<QuantExt::Basket>& basket, Protection::Side side, const Schedule& schedule,
                 Rate upfrontRate, Rate runningRate, const DayCounter& dayCounter,
                 BusinessDayConvention paymentConvention, bool settlesAccrual = true,
                 const QuantLib::CreditDefaultSwap::ProtectionPaymentTime protectionPaymentTime =
                     QuantLib::CreditDefaultSwap::ProtectionPaymentTime::atDefault,
                 Date protectionStart = Date(), Date upfrontDate = Date(),
                 boost::optional<Real> notional = boost::none,
                 Real recoveryRate = Null<Real>(),
                 const DayCounter& lastPeriodDayCounter = DayCounter());

    // RL: QuantExt version
    const QuantLib::ext::shared_ptr<QuantExt::Basket>& basket() const { return basket_; }

    bool isExpired() const override;
    Rate fairPremium() const;
    Rate fairUpfrontPremium() const;
    Rate premiumValue() const;
    Rate protectionValue() const;
    Real premiumLegNPV() const;
    Real protectionLegNPV() const;
    Real accrualRebateNPV() const;
    Real accrualRebateNPVCurrent() const;
    Real upfrontPremiumValue() const;
    Real cleanNPV() const;

    //! Returns the recovery rate for fixed recovery CDO, otherwise returns Null<Real>()
    Real recoveryRate() const { return recoveryRate_; }
    /*!
      Total outstanding tranche notional, not wiped out
    */
    Real remainingNotional() const;
    /*! The number of times the contract contains the portfolio tranched
            notional.
    */
    Real leverageFactor() const { return leverageFactor_; }
    //! Last protection date.
    const Date& maturity() const {
        return QuantLib::ext::dynamic_pointer_cast<FixedRateCoupon>(normalizedLeg_.back())->accrualEndDate();
    }
    /*! The Gaussian Copula LHP implied correlation that makes the
        contract zero value. This is for a flat correlation along
        time and portfolio loss level.
    */
    Real implicitCorrelation(const std::vector<Real>& recoveries, const Handle<YieldTermStructure>& discountCurve,
                             Real targetNPV = 0., Real accuracy = 1.0e-3) const;

    /*!
      Expected tranche loss for all payment dates
     */
    std::vector<Real> expectedTrancheLoss() const;
    Size error() const;

    void setupArguments(PricingEngine::arguments*) const override;
    void fetchResults(const PricingEngine::results*) const override;

private:
    void setupExpired() const override;

    // RL: QuantExt version
    QuantLib::ext::shared_ptr<QuantExt::Basket> basket_;
    Protection::Side side_;
    Leg normalizedLeg_;

    Rate upfrontRate_;
    Rate runningRate_;
    const Real leverageFactor_;
    DayCounter dayCounter_;
    BusinessDayConvention paymentConvention_;
    bool settlesAccrual_;
    QuantLib::CreditDefaultSwap::ProtectionPaymentTime protectionPaymentTime_;
    Date protectionStart_;
    Date upfrontDate_;
    QuantLib::ext::shared_ptr<CashFlow> upfrontPayment_, accrualRebate_, accrualRebateCurrent_;
    Real recoveryRate_;
    
    mutable Real premiumValue_;
    mutable Real protectionValue_;
    mutable Real upfrontPremiumValue_;
    mutable Real remainingNotional_;
    mutable Size error_;
    mutable std::vector<Real> expectedTrancheLoss_;
    mutable Real accrualRebateNPV_;
    mutable Real accrualRebateNPVCurrent_;
    mutable Real cleanNPV_;
};

class SyntheticCDO::arguments : public virtual PricingEngine::arguments {
public:
    arguments() : side(Protection::Side(-1)), upfrontRate(Null<Real>()), runningRate(Null<Real>()) {}
    void validate() const override;

    // RL: QuantExt version
    QuantLib::ext::shared_ptr<QuantExt::Basket> basket;
    Protection::Side side;
    Leg normalizedLeg;

    Rate upfrontRate;
    Rate runningRate;
    Real leverageFactor;
    DayCounter dayCounter;
    BusinessDayConvention paymentConvention;
    QuantLib::ext::shared_ptr<CashFlow> upfrontPayment, accrualRebate, accrualRebateCurrent;
    bool settlesAccrual;
    QuantLib::CreditDefaultSwap::ProtectionPaymentTime protectionPaymentTime;
    Date protectionStart;
    Date maturity;
    Real recoveryRate;
};

class SyntheticCDO::results : public Instrument::results {
public:
    void reset() override;
    Real premiumValue;
    Real protectionValue;
    Real upfrontPremiumValue;
    Real accrualRebateValue;
    Real accrualRebateCurrentValue;
    Real accrualRebateNPV_;
    Real accrualRebateNPVCurrent_;
    Real cleanNPV;
    Real remainingNotional;
    Real xMin, xMax;
    Size error;
    /* Expected tranche losses affecting this tranche coupons. Notice this
    number might be below the actual basket losses, since the cdo protection
    might start after basket inception (forward start CDO)*/
    std::vector<Real> expectedTrancheLoss;
};

//! CDO base engine
class SyntheticCDO::engine : public GenericEngine<SyntheticCDO::arguments, SyntheticCDO::results> {};

} // namespace QuantExt

#endif

#endif

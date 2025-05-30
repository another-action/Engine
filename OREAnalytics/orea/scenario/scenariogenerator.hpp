/*
 Copyright (C) 2016 Quaternion Risk Management Ltd
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

/*! \file scenario/scenariogenerator.hpp
    \brief Scenario generator base classes
    \ingroup scenario
*/

#pragma once

#include <vector>

#include <ql/shared_ptr.hpp>
#include <orea/scenario/scenario.hpp>
#include <orea/scenario/scenarioloader.hpp>
#include <ql/time/date.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/timegrid.hpp>

namespace ore {
namespace analytics {
using QuantLib::TimeGrid;
using std::vector;

//! Scenario generator base class
/*! \ingroup scenario
 */
class ScenarioGenerator {
public:
    //! Default destructor
    virtual ~ScenarioGenerator() {}

    //! Return the next scenario for the given date.
    virtual QuantLib::ext::shared_ptr<Scenario> next(const Date& d) = 0;

    //! Reset the generator so calls to next() return the first scenario.
    /*! This allows re-generation of scenarios if required. */
    virtual void reset() = 0;
};

//! Scenario generator that generates an entire path
/*! \ingroup scenario
 */
class ScenarioPathGenerator : public ScenarioGenerator {
public:
    // TODO: Why dates AND timegrid, why not DateGrid???
    //! Constructor
    ScenarioPathGenerator( //! Today's date
        Date today,
        //! Future evaluation dates
        const vector<Date>& dates,
        //! Associated time grid
        TimeGrid timeGrid)                                    // DayCounter dayCounter = ActualActual())
        : today_(today), dates_(dates), timeGrid_(timeGrid) { // dayCounter_(dayCounter) {
        QL_REQUIRE(dates.size() > 0, "empty date vector passed");
        QL_REQUIRE(dates.front() > today, "date grid must start in the future");
    }

    virtual QuantLib::ext::shared_ptr<Scenario> next(const Date& d) override;

protected:
    virtual std::vector<QuantLib::ext::shared_ptr<Scenario>> nextPath() = 0;

    Date today_;
    vector<Date> dates_;
    Size pathStep_;
    // DayCounter dayCounter_;
    TimeGrid timeGrid_;
    std::vector<QuantLib::ext::shared_ptr<Scenario>> path_;
};

// A simple scenario generator that contains a single scenario
class StaticScenarioGenerator : public ScenarioGenerator {
public:
    StaticScenarioGenerator() {}

    void reset() override {}
    QuantLib::ext::shared_ptr<Scenario> next(const Date&) override { return s_; }

    void setScenario(const QuantLib::ext::shared_ptr<Scenario>& s) { s_ = s; }

private:
    QuantLib::ext::shared_ptr<Scenario> s_;
};

// A simple scenario generator that returns scenarios from a scenario loader
class ScenarioLoaderGenerator : public ScenarioPathGenerator {
public:
    ScenarioLoaderGenerator(const QuantLib::ext::shared_ptr<SimpleScenarioLoader>& scenarioLoader, Date today,
                            const vector<Date>& dates, TimeGrid timeGrid)
        : ScenarioPathGenerator(today, dates, timeGrid), scenarioLoader_(scenarioLoader), i_(0) {}

    void reset() override { i_ = 0; }

    const QuantLib::ext::shared_ptr<SimpleScenarioLoader>& scenarioLoader() const { return scenarioLoader_; }
    std::vector<QuantLib::ext::shared_ptr<Scenario>> nextPath() override;

private:
    QuantLib::ext::shared_ptr<SimpleScenarioLoader> scenarioLoader_;
    Size i_ = 0;
};

} // namespace analytics
} // namespace ore

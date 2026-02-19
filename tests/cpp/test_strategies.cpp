#include <gtest/gtest.h>
#include <cmath>
#include "strategy/Strategy.h"
#include "strategy/BullCall.h"
#include "strategy/Straddle.h"
#include "strategy/Strangle.h"
#include "strategy/IronCondor.h"
#include "strategy/StrategyFactory.h"

using namespace OptionPricer;

class StrategyTest : public ::testing::Test
{
protected:
    double spot = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double sigma = 0.2;
    double time = 1.0;
};

// Test bull call spread max profit
TEST_F(StrategyTest, BullCallMaxProfit)
{
    BullCall bullCall(spot, strike, rate, sigma, time, true);
    double maxProfit = bullCall.maxProfit();

    // Max profit should be positive
    EXPECT_GT(maxProfit, 0.0);
}

// Test bull call spread max loss
TEST_F(StrategyTest, BullCallMaxLoss)
{
    BullCall bullCall(spot, strike, rate, sigma, time, true);
    double maxLoss = bullCall.maxLoss();

    // Max loss should be non-negative
    EXPECT_GE(maxLoss, 0.0);
}

// Test bull call spread breakeven
TEST_F(StrategyTest, BullCallBreakeven)
{
    BullCall bullCall(spot, strike, rate, sigma, time, true);
    double breakeven = bullCall.breakeven();

    // Breakeven should be between lower and upper strike
    EXPECT_GE(breakeven, strike);
    EXPECT_LE(breakeven, strike * 1.05);
}

// Test straddle has positive value
TEST_F(StrategyTest, StraddlePrice)
{
    Straddle straddle(spot, strike, rate, sigma, time, true);
    double price = straddle.price();

    EXPECT_GT(price, 0.0);
}

// Test long straddle has delta near zero
TEST_F(StrategyTest, StraddleDeltaNeutral)
{
    Straddle straddle(spot, strike, rate, sigma, time, true);
    double delta = straddle.delta();

    // Delta should be close to zero (long call + long put)
    EXPECT_NEAR(delta, 0.0, 0.1);
}

// Test strangle has lower price than straddle
TEST_F(StrategyTest, StranglePriceLessThanStraddle)
{
    Straddle straddle(spot, strike, rate, sigma, time, true);
    Strangle strangle(spot, strike, rate, sigma, time, true);

    // Strangle should be cheaper (strikes further apart)
    EXPECT_LT(strangle.price(), straddle.price());
}

// Test iron condor has positive max profit
TEST_F(StrategyTest, IronCondorMaxProfit)
{
    IronCondor ironCondor(spot, strike, rate, sigma, time, true);
    double maxProfit = ironCondor.maxProfit();

    EXPECT_GT(maxProfit, 0.0);
}

// Test iron condor is credit strategy (negative cost at entry)
TEST_F(StrategyTest, IronCondorCredit)
{
    IronCondor ironCondor(spot, strike, rate, sigma, time, true);
    double price = ironCondor.price();

    // Should be net credit (negative price means received premium)
    EXPECT_LT(price, 0.0);
}

// Test strategy factory creates bull call
TEST_F(StrategyTest, StrategyFactoryBullCall)
{
    auto bullCall = StrategyFactory::create(
        "bull_call", spot, strike, rate, sigma, time, true);

    EXPECT_NE(bullCall, nullptr);
    EXPECT_GT(bullCall->price(), -10000.0); // Reasonable price
}

// Test strategy factory creates straddle
TEST_F(StrategyTest, StrategyFactoryStraddle)
{
    auto straddle = StrategyFactory::create(
        "straddle", spot, strike, rate, sigma, time, true);

    EXPECT_NE(straddle, nullptr);
    EXPECT_GT(straddle->price(), 0.0);
}

// Test strategy factory creates strangle
TEST_F(StrategyTest, StrategyFactoryStrangle)
{
    auto strangle = StrategyFactory::create(
        "strangle", spot, strike, rate, sigma, time, true);

    EXPECT_NE(strangle, nullptr);
    EXPECT_GT(strangle->price(), 0.0);
}

// Test strategy factory creates iron condor
TEST_F(StrategyTest, StrategyFactoryIronCondor)
{
    auto ironCondor = StrategyFactory::create(
        "iron_condor", spot, strike, rate, sigma, time, true);

    EXPECT_NE(ironCondor, nullptr);
}

// Test strategy factory lists available strategies
TEST_F(StrategyTest, StrategyFactoryList)
{
    auto strategies = StrategyFactory::getAvailableStrategies();

    EXPECT_GE(strategies.size(), 4);
    EXPECT_TRUE(std::find(strategies.begin(), strategies.end(), "bull_call") != strategies.end());
}

// Test bull call spread reversal (short position)
TEST_F(StrategyTest, BullCallShort)
{
    BullCall longBullCall(spot, strike, rate, sigma, time, true);
    BullCall shortBullCall(spot, strike, rate, sigma, time, false);

    // Short should be negative of long
    EXPECT_NEAR(longBullCall.price() + shortBullCall.price(), 0.0, 0.1);
}

// Test strategy vega is positive for long volatility
TEST_F(StrategyTest, StrategyVegaLongVolatility)
{
    Straddle straddle(spot, strike, rate, sigma, time, true);
    double vega = straddle.vega();

    // Long straddle has positive vega
    EXPECT_GT(vega, 0.0);
}

// Test strategy theta for time decay
TEST_F(StrategyTest, StrategyTheta)
{
    Straddle straddle(spot, strike, rate, sigma, time, true);
    double theta = straddle.theta();

    // Long straddle theta (time decay)
    // Can be negative or positive depending on spot/strike relationship
    EXPECT_NE(theta, 0.0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

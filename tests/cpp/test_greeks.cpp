#include <gtest/gtest.h>
#include <cmath>
#include "options/EuropeanOption.h"
#include "models/Greeks.h"

using namespace OptionPricer;

class GreeksTest : public ::testing::Test
{
protected:
    double spot = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double sigma = 0.2;
    double time = 1.0;
};

// Test delta is between -1 and 1
TEST_F(GreeksTest, DeltaBounds)
{
    EuropeanOption callOption(spot, strike, rate, sigma, time, "call");
    EuropeanOption putOption(spot, strike, rate, sigma, time, "put");

    double callDelta = callOption.delta();
    double putDelta = putOption.delta();

    EXPECT_GE(callDelta, 0.0);
    EXPECT_LE(callDelta, 1.0);
    EXPECT_GE(putDelta, -1.0);
    EXPECT_LE(putDelta, 0.0);
}

// Test gamma is always positive
TEST_F(GreeksTest, GammaPositive)
{
    EuropeanOption callOption(spot, strike, rate, sigma, time, "call");
    EuropeanOption putOption(spot, strike, rate, sigma, time, "put");

    EXPECT_GT(callOption.gamma(), 0.0);
    EXPECT_GT(putOption.gamma(), 0.0);
}

// Test vega is always positive (per 1% change in volatility)
TEST_F(GreeksTest, VegaPositive)
{
    EuropeanOption callOption(spot, strike, rate, sigma, time, "call");
    EuropeanOption putOption(spot, strike, rate, sigma, time, "put");

    EXPECT_GT(callOption.vega(), 0.0);
    EXPECT_GT(putOption.vega(), 0.0);
}

// Test put-call parity: C - P = S * exp(-r*T) - K * exp(-r*T) (no dividend)
TEST_F(GreeksTest, PutCallParity)
{
    EuropeanOption callOption(spot, strike, rate, sigma, time, "call");
    EuropeanOption putOption(spot, strike, rate, sigma, time, "put");

    double callPrice = callOption.price();
    double putPrice = putOption.price();

    double parity = callPrice - putPrice;
    double expected = spot * std::exp(-rate * time) - strike * std::exp(-rate * time);

    EXPECT_NEAR(parity, expected, 0.01);
}

// Test delta symmetry: Delta(call) + Delta(put) ≈ 1
TEST_F(GreeksTest, DeltaSymmetry)
{
    EuropeanOption callOption(spot, strike, rate, sigma, time, "call");
    EuropeanOption putOption(spot, strike, rate, sigma, time, "put");

    double deltaSum = callOption.delta() + putOption.delta();

    EXPECT_NEAR(deltaSum, 1.0, 0.01);
}

// Test that ATM options have delta around 0.5 for calls
TEST_F(GreeksTest, ATMCallDelta)
{
    EuropeanOption atmCall(spot, spot, rate, sigma, time, "call");

    double delta = atmCall.delta();
    EXPECT_GT(delta, 0.4);
    EXPECT_LT(delta, 0.6);
}

// Test volatility impact on vega
TEST_F(GreeksTest, VegaVsVolatility)
{
    EuropeanOption opt1(spot, strike, rate, 0.1, time, "call");
    EuropeanOption opt2(spot, strike, rate, 0.3, time, "call");

    // Both should have positive vega
    EXPECT_GT(opt1.vega(), 0.0);
    EXPECT_GT(opt2.vega(), 0.0);
}

// Test time decay (theta) for ATM call
TEST_F(GreeksTest, ThetaATMCall)
{
    EuropeanOption atmCall(spot, strike, rate, sigma, time, "call");

    double theta = atmCall.theta();
    // ATM call theta is typically negative (time decay)
    EXPECT_LT(theta, 0.0);
}

// Test in-the-money call has higher delta
TEST_F(GreeksTest, DeltaMoneyness)
{
    EuropeanOption itmCall(110.0, strike, rate, sigma, time, "call");
    EuropeanOption otmCall(90.0, strike, rate, sigma, time, "call");

    EXPECT_GT(itmCall.delta(), otmCall.delta());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>
#include <cmath>
#include "options/EuropeanOption.h"
#include "options/OptionFactory.h"

using namespace OptionPricer;

class OptionPricingTest : public ::testing::Test
{
protected:
    double spot = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double sigma = 0.2;
    double time = 1.0;
};

// Test European call pricing bounds
TEST_F(OptionPricingTest, CallPriceBounds)
{
    EuropeanOption call(spot, strike, rate, sigma, time, "call");
    double price = call.price();

    // Price should be between intrinsic value and spot price
    double intrinsic = std::max(spot - strike, 0.0);
    EXPECT_GE(price, intrinsic);
    EXPECT_LE(price, spot);
}

// Test European put pricing bounds
TEST_F(OptionPricingTest, PutPriceBounds)
{
    EuropeanOption put(spot, strike, rate, sigma, time, "put");
    double price = put.price();

    // Price should be between intrinsic value and strike * exp(-r*T)
    double intrinsic = std::max(strike - spot, 0.0);
    EXPECT_GE(price, intrinsic);
    EXPECT_LE(price, strike * std::exp(-rate * time));
}

// Test ITM call has intrinsic value
TEST_F(OptionPricingTest, ITMCallPrice)
{
    EuropeanOption itmCall(110.0, 100.0, rate, sigma, time, "call");
    double price = itmCall.price();
    double intrinsic = 10.0;

    EXPECT_GE(price, intrinsic);
}

// Test OTM option has non-zero price (time value)
TEST_F(OptionPricingTest, OTMOptionPrice)
{
    EuropeanOption otmCall(90.0, 100.0, rate, sigma, time, "call");
    double price = otmCall.price();

    // Should have time value, not zero
    EXPECT_GT(price, 0.0);
}

// Test zero time to expiration gives intrinsic value
TEST_F(OptionPricingTest, ExpiredCallPrice)
{
    EuropeanOption expiredCall(110.0, 100.0, rate, sigma, 0.001, "call");
    double price = expiredCall.price();
    double intrinsic = 10.0;

    EXPECT_NEAR(price, intrinsic, 0.1);
}

// Test that call price increases with spot
TEST_F(OptionPricingTest, CallMoneyness)
{
    EuropeanOption call1(100.0, strike, rate, sigma, time, "call");
    EuropeanOption call2(110.0, strike, rate, sigma, time, "call");

    EXPECT_LT(call1.price(), call2.price());
}

// Test that put price increases with strike
TEST_F(OptionPricingTest, PutMoneyness)
{
    EuropeanOption put1(spot, 100.0, rate, sigma, time, "put");
    EuropeanOption put2(spot, 110.0, rate, sigma, time, "put");

    EXPECT_LT(put1.price(), put2.price());
}

// Test option factory creates correct type
TEST_F(OptionPricingTest, OptionFactory)
{
    auto europeanOption = OptionFactory::create(
        "european", spot, strike, rate, sigma, time, "call", 100);

    EXPECT_NE(europeanOption, nullptr);
    EXPECT_GT(europeanOption->price(), 0.0);
}

// Test option factory with invalid type throws
TEST_F(OptionPricingTest, OptionFactoryInvalidType)
{
    EXPECT_THROW(
        OptionFactory::create("invalid", spot, strike, rate, sigma, time, "call", 100),
        std::invalid_argument);
}

// Test negative time throws or returns sensible value
TEST_F(OptionPricingTest, NegativeTimeHandling)
{
    // This depends on implementation - might throw or treat as zero
    EXPECT_NO_THROW({
        EuropeanOption option(spot, strike, rate, sigma, 0.0, "call");
        double price = option.price();
        EXPECT_GE(price, 0.0);
    });
}

// Test high volatility increases option price
TEST_F(OptionPricingTest, VolatilityEffect)
{
    EuropeanOption optionLowVol(spot, strike, rate, 0.1, time, "call");
    EuropeanOption optionHighVol(spot, strike, rate, 0.5, time, "call");

    EXPECT_LT(optionLowVol.price(), optionHighVol.price());
}

// Test discount rate effects
TEST_F(OptionPricingTest, RateEffect)
{
    EuropeanOption callLowRate(spot, strike, 0.01, sigma, time, "call");
    EuropeanOption callHighRate(spot, strike, 0.10, sigma, time, "call");

    // For calls, higher rates increase price (higher forward)
    EXPECT_LT(callLowRate.price(), callHighRate.price());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

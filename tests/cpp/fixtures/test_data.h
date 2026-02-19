#ifndef OPTION_PRICER_TEST_DATA_H
#define OPTION_PRICER_TEST_DATA_H

#include <vector>
#include <map>
#include <string>

namespace OptionPricer
{
    namespace Tests
    {

        // Known test cases from published sources
        struct TestCase
        {
            double spot;
            double strike;
            double rate;
            double volatility;
            double time;
            double expectedCallPrice;
            double expectedPutPrice;
            std::string description;
        };

        // Haug's Handbook test cases (commonly used in quant finance)
        static const std::vector<TestCase> EUROPEAN_OPTION_TEST_CASES = {
            {100.0, // spot
             100.0, // strike
             0.05,  // rate
             0.2,   // volatility
             1.0,   // time
             10.45, // expected call price
             5.57,  // expected put price
             "ATM option with standard parameters"},
            {110.0, // spot
             100.0, // strike
             0.05,  // rate
             0.2,   // time
             1.0,   // time
             14.97, // expected call price
             1.10,  // expected put price
             "ITM call, OTM put"},
            {90.0,  // spot
             100.0, // strike
             0.05,  // rate
             0.2,   // volatility
             1.0,   // time
             5.87,  // expected call price
             9.99,  // expected put price
             "OTM call, ITM put"},
            {100.0, // spot
             100.0, // strike
             0.0,   // rate (risk-free rate = 0)
             0.2,   // volatility
             1.0,   // time
             7.97,  // expected call price
             7.97,  // expected put price
             "ATM with zero interest rate"},
            {100.0, // spot
             100.0, // strike
             0.05,  // rate
             0.1,   // volatility (low volatility)
             1.0,   // time
             5.66,  // expected call price
             1.73,  // expected put price
             "ATM with low volatility"}};

        // Strategy test cases
        struct StrategyTestCase
        {
            std::string strategyName;
            double spot;
            double strike;
            double rate;
            double volatility;
            double time;
            double expectedMaxProfit;
            double expectedMaxLoss;
            std::string description;
        };

        static const std::vector<StrategyTestCase> STRATEGY_TEST_CASES = {
            {"bull_call",
             100.0,
             100.0,
             0.05,
             0.2,
             1.0,
             5.0,   // Spread width
             10.45, // Cost of call
             "Bull call spread at ATM"},
            {"straddle",
             100.0,
             100.0,
             0.05,
             0.2,
             1.0,
             100.0, // Unlimited on upside
             20.9,  // Cost (2x call price)
             "Long straddle at ATM"},
            {"iron_condor",
             100.0,
             100.0,
             0.05,
             0.2,
             1.0,
             2.0, // Credit received - profit range
             3.0, // Remaining risk to expiration
             "Iron condor with 5% width"}};

        // Greeks test data - known values for standard scenarios
        struct GreeksTestData
        {
            double spot;
            double strike;
            double rate;
            double volatility;
            double time;
            double expectedDelta;
            double expectedGamma;
            double expectedVega;
            std::string optionType;
        };

        static const std::vector<GreeksTestData> GREEKS_TEST_DATA = {
            {100.0,  // spot
             100.0,  // strike
             0.05,   // rate
             0.2,    // volatility
             1.0,    // time
             0.54,   // expected delta for ATM call
             0.0198, // expected gamma
             39.45,  // expected vega (per 1% change)
             "call"},
            {100.0,  // spot
             100.0,  // strike
             0.05,   // rate
             0.2,    // volatility
             1.0,    // time
             -0.46,  // expected delta for ATM put
             0.0198, // expected gamma
             39.45,  // expected vega
             "put"},
            {110.0,  // spot
             100.0,  // strike
             0.05,   // rate
             0.2,    // volatility
             1.0,    // time
             0.75,   // expected delta for ITM call
             0.0122, // expected gamma
             28.79,  // expected vega
             "call"}};

        // Boundary test cases
        struct BoundaryTestCase
        {
            double spot;
            double strike;
            double rate;
            double volatility;
            double time;
            std::string description;
            bool shouldPass;
        };

        static const std::vector<BoundaryTestCase> BOUNDARY_TEST_CASES = {
            {0.01, 100.0, 0.05, 0.2, 1.0, "Very low spot price", true},
            {1000.0, 100.0, 0.05, 0.2, 1.0, "Very high spot price", true},
            {100.0, 100.0, 0.05, 0.01, 1.0, "Very low volatility", true},
            {100.0, 100.0, 0.05, 2.0, 1.0, "Very high volatility", true},
            {100.0, 100.0, 0.05, 0.2, 0.01, "Very short time to expiration", true},
            {100.0, 100.0, 0.05, 0.2, 10.0, "Very long time to expiration", true},
            {-100.0, 100.0, 0.05, 0.2, 1.0, "Negative spot price", false},
            {100.0, -100.0, 0.05, 0.2, 1.0, "Negative strike price", false},
            {100.0, 100.0, 0.05, -0.2, 1.0, "Negative volatility", false},
            {100.0, 100.0, 0.05, 0.2, -1.0, "Negative time", false}};

        // Market scenarios for stress testing
        struct MarketScenario
        {
            std::string name;
            double volatilityMultiplier;
            double rateShift;
            double spotMove;
        };

        static const std::vector<MarketScenario> MARKET_SCENARIOS = {
            {"Normal Market", 1.0, 0.0, 0.0},
            {"High Volatility", 2.0, 0.0, 0.0},
            {"Low Volatility", 0.5, 0.0, 0.0},
            {"Rate Increase", 1.0, 0.01, 0.0},
            {"Rate Decrease", 1.0, -0.01, 0.0},
            {"Spot Up 10%", 1.0, 0.0, 10.0},
            {"Spot Down 10%", 1.0, 0.0, -10.0},
            {"Crisis Scenario", 3.0, 0.02, -15.0}};

    } // namespace Tests
} // namespace OptionPricer

#endif // OPTION_PRICER_TEST_DATA_H

#include "strategy/StrategyFactory.h"
#include <stdexcept>

namespace OptionPricer
{

    std::shared_ptr<Strategy> StrategyFactory::create(
        const std::string &strategyName,
        double S, double K, double r, double sigma, double T,
        bool isLong)
    {

        if (strategyName == "straddle")
        {
            return std::make_shared<Straddle>(S, K, r, sigma, T, isLong);
        }
        else if (strategyName == "strangle")
        {
            // Default: 5% OTM strikes
            double K_call = K * 1.05;
            double K_put = K * 0.95;
            return std::make_shared<Strangle>(S, K_call, K_put, r, sigma, T, isLong);
        }
        else if (strategyName == "bull_call" || strategyName == "bull_call_spread")
        {
            // Default: 5% spread
            double K_long = K;
            double K_short = K * 1.05;
            return createBullCall(S, K_long, K_short, r, sigma, T);
        }
        else if (strategyName == "iron_condor")
        {
            // Default: symmetric iron condor
            double K1 = K * 0.95; // Long put
            double K2 = K * 0.98; // Short put
            double K3 = K * 1.02; // Short call
            double K4 = K * 1.05; // Long call
            return createIronCondor(S, K1, K2, K3, K4, r, sigma, T, true);
        }
        else
        {
            throw std::invalid_argument("Unknown strategy: " + strategyName);
        }
    }

    std::shared_ptr<Strategy> StrategyFactory::createBullCall(
        double S, double K1, double K2, double r, double sigma, double T)
    {
        return std::make_shared<BullCall>(S, K1, K2, r, sigma, T);
    }

    std::shared_ptr<Strategy> StrategyFactory::createIronCondor(
        double S, double K1, double K2, double K3, double K4,
        double r, double sigma, double T, bool isShort)
    {
        return std::make_shared<IronCondor>(S, K1, K2, K3, K4, r, sigma, T);
    }

    std::vector<std::string> StrategyFactory::getAvailableStrategies()
    {
        return {
            "straddle",
            "strangle",
            "bull_call",
            "bull_call_spread",
            "iron_condor"};
    }

} // namespace OptionPricer

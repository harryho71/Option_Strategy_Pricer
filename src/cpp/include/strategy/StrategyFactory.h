#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Strategy.h"
#include "Straddle.h"
#include "Strangle.h"
#include "BullCall.h"
#include "IronCondor.h"

namespace OptionPricer
{

    /**
     * @class StrategyFactory
     * @brief Factory for creating Strategy objects
     *
     * Encapsulates strategy creation logic. Provides named strategies
     * for common option combinations.
     */
    class StrategyFactory
    {
    public:
        /**
         * Create a predefined strategy
         * @param strategyName Name of strategy: "straddle", "strangle", "bull_call", "iron_condor"
         * @param S Spot price
         * @param K Strike price(s)
         * @param r Risk-free rate
         * @param sigma Volatility
         * @param T Time to expiration
         * @param isLong Whether to take long position (buy) vs short (sell)
         * @return Shared pointer to created Strategy
         */
        static std::shared_ptr<Strategy> create(
            const std::string &strategyName,
            double S, double K, double r, double sigma, double T,
            bool isLong = true);

        /**
         * Create custom spread strategies with multiple strikes
         */
        static std::shared_ptr<Strategy> createBullCall(
            double S, double K1, double K2, double r, double sigma, double T);

        static std::shared_ptr<Strategy> createIronCondor(
            double S, double K1, double K2, double K3, double K4,
            double r, double sigma, double T, bool isShort = true);

        /**
         * Get list of available strategy names
         */
        static std::vector<std::string> getAvailableStrategies();
    };

} // namespace OptionPricer

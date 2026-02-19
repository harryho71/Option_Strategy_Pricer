#pragma once

#include <vector>
#include <map>
#include <memory>
#include "../options/Option.h"

namespace OptionPricer
{

    /**
     * @namespace RiskMeasures
     * @brief Portfolio risk measurement functions
     *
     * Implements Value-at-Risk (VaR), Expected Shortfall (ES),
     * and other risk metrics for option portfolios.
     */
    namespace RiskMeasures
    {

        /**
         * Value-at-Risk (VaR)
         * Maximum loss at given confidence level
         *
         * @param portfolio Vector of option positions
         * @param confidence Confidence level (0.95 = 95%)
         * @param horizon Time horizon in years
         * @param spotPrices Simulated spot prices
         * @return VaR amount
         */
        double valueAtRisk(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                           double confidence, double horizon,
                           const std::vector<double> &spotPrices);

        /**
         * Expected Shortfall (ES)
         * Average loss beyond VaR threshold
         *
         * @param portfolio Vector of option positions
         * @param confidence Confidence level (0.95 = 95%)
         * @param horizon Time horizon in years
         * @param spotPrices Simulated spot prices
         * @return ES amount
         */
        double expectedShortfall(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                                 double confidence, double horizon,
                                 const std::vector<double> &spotPrices);

        /**
         * Maximum loss
         * Worst-case loss in portfolio
         */
        double maxLoss(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                       const std::vector<double> &spotPrices);

        /**
         * Probability of profit
         * Percentage of outcomes with positive P&L
         */
        double probabilityOfProfit(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                                   const std::vector<double> &spotPrices);

        /**
         * Greeks aggregation for portfolio
         */
        struct PortfolioRisk
        {
            double delta;
            double gamma;
            double vega;
            double theta;
            double rho;
            double var; // Value-at-Risk
            double es;  // Expected Shortfall
            double maxLoss;
            double pop; // Probability of profit
        };

        PortfolioRisk calculatePortfolioRisk(
            const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
            double confidence = 0.95,
            double horizon = 1.0 / 252.0); // 1 day

    } // namespace RiskMeasures
} // namespace OptionPricer

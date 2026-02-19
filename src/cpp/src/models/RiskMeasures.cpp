#include "models/RiskMeasures.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace OptionPricer
{
    namespace RiskMeasures
    {

        double valueAtRisk(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                           double confidence, double horizon,
                           const std::vector<double> &spotPrices)
        {

            std::vector<double> losses;
            for (double spot : spotPrices)
            {
                double pnl = 0.0;
                for (const auto &leg : portfolio)
                {
                    auto option = leg.first;
                    int qty = leg.second;
                    option->setSpot(spot);
                    pnl += qty * option->price();
                }
                losses.push_back(-pnl); // Negative for losses
            }

            // Sort losses
            std::sort(losses.begin(), losses.end());

            // Get VaR at confidence level
            int index = static_cast<int>(std::ceil((1.0 - confidence) * losses.size())) - 1;
            return (index >= 0) ? losses[index] : 0.0;
        }

        double expectedShortfall(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                                 double confidence, double horizon,
                                 const std::vector<double> &spotPrices)
        {

            std::vector<double> losses;
            for (double spot : spotPrices)
            {
                double pnl = 0.0;
                for (const auto &leg : portfolio)
                {
                    auto option = leg.first;
                    int qty = leg.second;
                    option->setSpot(spot);
                    pnl += qty * option->price();
                }
                losses.push_back(-pnl);
            }

            std::sort(losses.begin(), losses.end());

            // Average of worst losses beyond VaR
            int index = static_cast<int>(std::ceil((1.0 - confidence) * losses.size())) - 1;
            if (index < 0)
                index = 0;

            double sum = 0.0;
            for (int i = 0; i <= index; ++i)
            {
                sum += losses[i];
            }

            return (index >= 0) ? sum / (index + 1) : 0.0;
        }

        double maxLoss(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                       const std::vector<double> &spotPrices)
        {

            double worst = 0.0;
            for (double spot : spotPrices)
            {
                double pnl = 0.0;
                for (const auto &leg : portfolio)
                {
                    auto option = leg.first;
                    int qty = leg.second;
                    option->setSpot(spot);
                    pnl += qty * option->price();
                }
                worst = std::max(worst, -pnl);
            }

            return worst;
        }

        double probabilityOfProfit(const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
                                   const std::vector<double> &spotPrices)
        {

            int profitCount = 0;
            for (double spot : spotPrices)
            {
                double pnl = 0.0;
                for (const auto &leg : portfolio)
                {
                    auto option = leg.first;
                    int qty = leg.second;
                    option->setSpot(spot);
                    pnl += qty * option->price();
                }
                if (pnl > 0.0)
                {
                    profitCount++;
                }
            }

            return static_cast<double>(profitCount) / spotPrices.size();
        }

        PortfolioRisk calculatePortfolioRisk(
            const std::vector<std::pair<std::shared_ptr<Option>, int>> &portfolio,
            double confidence,
            double horizon)
        {

            PortfolioRisk risk;

            // Calculate Greeks
            risk.delta = 0.0;
            risk.gamma = 0.0;
            risk.vega = 0.0;
            risk.theta = 0.0;
            risk.rho = 0.0;

            for (const auto &leg : portfolio)
            {
                auto option = leg.first;
                int qty = leg.second;

                risk.delta += qty * option->delta();
                risk.gamma += qty * option->gamma();
                risk.vega += qty * option->vega();
                risk.theta += qty * option->theta();
                risk.rho += qty * option->rho();
            }

            // Generate spot price simulations
            // (Simplified: linear range, should be lognormal distribution)
            double spot = (portfolio[0].first)->getSpot();
            std::vector<double> spots;
            for (int i = 0; i <= 100; ++i)
            {
                spots.push_back(spot * 0.8 + (spot * 0.4) * i / 100.0);
            }

            // Calculate risk measures
            risk.var = valueAtRisk(portfolio, confidence, horizon, spots);
            risk.es = expectedShortfall(portfolio, confidence, horizon, spots);
            risk.maxLoss = maxLoss(portfolio, spots);
            risk.pop = probabilityOfProfit(portfolio, spots);

            return risk;
        }

    } // namespace RiskMeasures
} // namespace OptionPricer

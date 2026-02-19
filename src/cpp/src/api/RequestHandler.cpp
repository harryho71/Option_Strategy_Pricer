#include "api/RequestHandler.h"
#include "api/JsonSerializer.h"
#include "options/OptionFactory.h"
#include "strategy/StrategyFactory.h"
#include "models/Greeks.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace OptionPricer
{
    namespace API
    {

        bool RequestHandler::validatePriceRequest(const OptionParams &params)
        {
            if (params.spot <= 0.0)
                return false;
            if (params.strike <= 0.0)
                return false;
            if (params.rate < 0.0)
                return false;
            if (params.volatility <= 0.0)
                return false;
            if (params.time <= 0.0)
                return false;
            if (params.type != "european" && params.type != "american")
                return false;
            if (params.optionType != "call" && params.optionType != "put")
                return false;
            return true;
        }

        bool RequestHandler::validateStrategyRequest(const StrategyParams &params)
        {
            if (params.spot <= 0.0)
                return false;
            if (params.strike <= 0.0)
                return false;
            if (params.rate < 0.0)
                return false;
            if (params.volatility <= 0.0)
                return false;
            if (params.time <= 0.0)
                return false;
            return true;
        }

        std::string RequestHandler::handlePrice(const std::string &jsonRequest)
        {
            try
            {
                OptionParams params = JsonSerializer::deserializeOptionParams(jsonRequest);

                if (!validatePriceRequest(params))
                {
                    return JsonSerializer::serializeError("Invalid parameters", 400);
                }

                // Create option using factory
                auto option = OptionFactory::create(
                    params.type,
                    params.spot,
                    params.strike,
                    params.rate,
                    params.volatility,
                    params.time,
                    params.optionType,
                    params.steps);

                return JsonSerializer::serializeOptionResult(option, params.type);
            }
            catch (const std::exception &e)
            {
                return JsonSerializer::serializeError(e.what(), 400);
            }
        }

        std::string RequestHandler::handleStrategy(const std::string &jsonRequest)
        {
            try
            {
                StrategyParams params = JsonSerializer::deserializeStrategyParams(jsonRequest);

                if (!validateStrategyRequest(params))
                {
                    return JsonSerializer::serializeError("Invalid parameters", 400);
                }

                // Create strategy using factory
                auto strategy = StrategyFactory::create(
                    params.strategyName,
                    params.spot,
                    params.strike,
                    params.rate,
                    params.volatility,
                    params.time,
                    params.isLong);

                return JsonSerializer::serializeStrategyResult(strategy, params.strategyName);
            }
            catch (const std::exception &e)
            {
                return JsonSerializer::serializeError(e.what(), 400);
            }
        }

        std::string RequestHandler::handleGreeksSurface(
            double spot, double strike, double rate, double volatility,
            double time, const std::string &greekType, int gridSize)
        {

            try
            {
                if (spot <= 0.0 || strike <= 0.0 || volatility <= 0.0 || time <= 0.0)
                {
                    return JsonSerializer::serializeError("Invalid parameters", 400);
                }

                // Generate spot and volatility ranges
                std::vector<double> spots, vols;
                for (int i = 0; i <= gridSize; ++i)
                {
                    spots.push_back(spot * (0.8 + 0.4 * i / gridSize));
                    vols.push_back(volatility * (0.5 + 1.5 * i / gridSize));
                }

                std::vector<std::vector<double>> surface;

                for (double s : spots)
                {
                    std::vector<double> row;
                    for (double v : vols)
                    {
                        double greek = 0.0;

                        auto option = OptionFactory::create("european", s, strike, rate, v, time, "call", 100);

                        if (greekType == "delta")
                        {
                            greek = option->delta();
                        }
                        else if (greekType == "gamma")
                        {
                            greek = option->gamma();
                        }
                        else if (greekType == "vega")
                        {
                            greek = option->vega();
                        }
                        else if (greekType == "theta")
                        {
                            greek = option->theta();
                        }
                        else if (greekType == "rho")
                        {
                            greek = option->rho();
                        }

                        row.push_back(greek);
                    }
                    surface.push_back(row);
                }

                return JsonSerializer::serializeGreeksSurface(surface, greekType, spots, vols);
            }
            catch (const std::exception &e)
            {
                return JsonSerializer::serializeError(e.what(), 400);
            }
        }

        std::string RequestHandler::handleImpliedVol(
            double spot, double strike, double rate, double time,
            double marketPrice, const std::string &optionType)
        {

            try
            {
                // Simple Newton-Raphson iteration for implied volatility
                double sigma = 0.2; // Initial guess
                const int maxIter = 50;
                const double tol = 1e-6;

                for (int i = 0; i < maxIter; ++i)
                {
                    auto option = OptionFactory::create("european", spot, strike, rate, sigma, time, optionType, 100);
                    double price = option->price();
                    double vega = option->vega();

                    if (std::abs(vega) < 1e-8)
                        break;

                    double diff = price - marketPrice;
                    if (std::abs(diff) < tol)
                        break;

                    sigma = sigma - diff / vega;
                    if (sigma < 0.001)
                        sigma = 0.001;
                    if (sigma > 5.0)
                        sigma = 5.0;
                }

                json result;
                result["impliedVolatility"] = sigma;
                result["spot"] = spot;
                result["strike"] = strike;
                result["rate"] = rate;
                result["time"] = time;
                result["marketPrice"] = marketPrice;

                return result.dump(4);
            }
            catch (const std::exception &e)
            {
                return JsonSerializer::serializeError(e.what(), 400);
            }
        }

        std::string RequestHandler::handleHealth()
        {
            json response;
            response["status"] = "healthy";
            response["version"] = "1.0.0";
            response["timestamp"] = "2024-01-01T00:00:00Z"; // Should use actual timestamp

            return response.dump(4);
        }

        std::string RequestHandler::handleStrategyList()
        {
            try
            {
                json response;
                response["strategies"] = StrategyFactory::getAvailableStrategies();

                return response.dump(4);
            }
            catch (const std::exception &e)
            {
                return JsonSerializer::serializeError(e.what(), 400);
            }
        }

    } // namespace API
} // namespace OptionPricer

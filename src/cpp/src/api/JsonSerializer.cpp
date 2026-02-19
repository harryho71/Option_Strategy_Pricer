#include "api/JsonSerializer.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace OptionPricer
{
    namespace API
    {

        std::string JsonSerializer::serializeOptionResult(
            const std::shared_ptr<Option> &option,
            const std::string &optionType)
        {

            json result;
            result["type"] = optionType;
            result["spot"] = option->getSpot();
            result["strike"] = option->getStrike();
            result["rate"] = option->getRate();
            result["volatility"] = option->getVolatility();
            result["time"] = option->getTime();
            result["price"] = option->price();

            json greeks;
            greeks["delta"] = option->delta();
            greeks["gamma"] = option->gamma();
            greeks["vega"] = option->vega();
            greeks["theta"] = option->theta();
            greeks["rho"] = option->rho();

            result["greeks"] = greeks;

            return result.dump(4);
        }

        std::string JsonSerializer::serializeStrategyResult(
            const std::shared_ptr<Strategy> &strategy,
            const std::string &strategyName)
        {

            json result;
            result["strategy"] = strategyName;
            result["price"] = strategy->price();

            json greeks;
            greeks["delta"] = strategy->delta();
            greeks["gamma"] = strategy->gamma();
            greeks["vega"] = strategy->vega();
            greeks["theta"] = strategy->theta();
            greeks["rho"] = strategy->rho();

            result["greeks"] = greeks;
            result["maxProfit"] = strategy->maxProfit();
            result["maxLoss"] = strategy->maxLoss();
            result["breakeven"] = strategy->breakeven();

            return result.dump(4);
        }

        std::string JsonSerializer::serializeGreeksSurface(
            const std::vector<std::vector<double>> &surface,
            const std::string &greekType,
            const std::vector<double> &spots,
            const std::vector<double> &volatilities)
        {

            json result;
            result["greek"] = greekType;
            result["data"] = surface;
            result["spots"] = spots;
            result["volatilities"] = volatilities;

            return result.dump(4);
        }

        std::string JsonSerializer::serializeError(
            const std::string &message,
            int errorCode)
        {

            json error;
            error["error"] = message;
            error["code"] = errorCode;

            return error.dump(4);
        }

        OptionParams JsonSerializer::deserializeOptionParams(const std::string &jsonStr)
        {
            try
            {
                json data = json::parse(jsonStr);

                OptionParams params;
                params.spot = data["spot"].get<double>();
                params.strike = data["strike"].get<double>();
                params.rate = data["rate"].get<double>();
                params.volatility = data["volatility"].get<double>();
                params.time = data["time"].get<double>();
                params.type = data["type"].get<std::string>();
                params.optionType = data.value("optionType", std::string("call"));
                params.steps = data.value("steps", 100);

                return params;
            }
            catch (const std::exception &e)
            {
                throw std::invalid_argument(std::string("JSON parse error: ") + e.what());
            }
        }

        StrategyParams JsonSerializer::deserializeStrategyParams(const std::string &jsonStr)
        {
            try
            {
                json data = json::parse(jsonStr);

                StrategyParams params;
                params.spot = data["spot"].get<double>();
                params.strike = data["strike"].get<double>();
                params.rate = data["rate"].get<double>();
                params.volatility = data["volatility"].get<double>();
                params.time = data["time"].get<double>();
                params.strategyName = data["strategy"].get<std::string>();
                params.isLong = data.value("isLong", true);

                return params;
            }
            catch (const std::exception &e)
            {
                throw std::invalid_argument(std::string("JSON parse error: ") + e.what());
            }
        }

    } // namespace API
} // namespace OptionPricer

#include "api/PricingEndpoint.h"
#include "options/EuropeanOption.h"
#include "options/AmericanOption.h"
#include "strategy/Straddle.h"
#include "strategy/Strangle.h"
#include <stdexcept>
#include <cmath>

namespace OptionPricer
{
    namespace API
    {

        std::shared_ptr<Option> PricingEndpoint::createOptionFromJson(const json &params)
        {
            if (!params.contains("type") || !params.contains("spot") ||
                !params.contains("strike") || !params.contains("rate") ||
                !params.contains("volatility") || !params.contains("time"))
            {
                throw std::invalid_argument("Missing required pricing parameters");
            }

            std::string type = params["type"].get<std::string>();
            double spot = params["spot"].get<double>();
            double strike = params["strike"].get<double>();
            double rate = params["rate"].get<double>();
            double volatility = params["volatility"].get<double>();
            double time = params["time"].get<double>();

            if (spot <= 0 || strike <= 0 || volatility <= 0 || time <= 0)
            {
                throw std::invalid_argument("Parameters must be positive");
            }

            // "model" field selects the pricing model; defaults to European Black-Scholes
            std::string model = params.value("model", "european");
            if (model == "american")
            {
                int steps = params.value("steps", 100);
                return std::make_shared<OptionPricer::AmericanOption>(spot, strike, rate, volatility, time, type, steps);
            }
            return std::make_shared<EuropeanOption>(spot, strike, rate, volatility, time, type);
        }

        json PricingEndpoint::buildGreeksResponse(const std::shared_ptr<Option> &option,
                                                  const std::string &model)
        {
            json response;
            response["price"] = option->price();
            response["delta"] = option->delta();
            response["gamma"] = option->gamma();
            response["vega"] = option->vega();
            response["theta"] = option->theta();
            response["rho"] = option->rho();
            response["spot"] = option->getSpot();
            response["strike"] = option->getStrike();
            response["type"] = option->type();
            response["model"] = model;
            return response;
        }

        json PricingEndpoint::handlePriceRequest(const json &request)
        {
            try
            {
                auto option = createOptionFromJson(request);
                std::string model = request.value("model", "european");
                return buildGreeksResponse(option, model);
            }
            catch (const std::exception &e)
            {
                json errorResponse;
                errorResponse["error"] = e.what();
                errorResponse["status"] = "error";
                return errorResponse;
            }
        }

        json PricingEndpoint::handleStrategyRequest(const json &request)
        {
            try
            {
                if (!request.contains("strategy"))
                {
                    throw std::invalid_argument("Missing 'strategy' parameter");
                }

                std::string strategy = request["strategy"].get<std::string>();
                double spot = request["spot"].get<double>();
                double rate = request["rate"].get<double>();
                double volatility = request["volatility"].get<double>();
                double time = request["time"].get<double>();
                bool isLong = request.value("is_long", true);

                std::shared_ptr<Strategy> strat = nullptr;

                if (strategy == "straddle")
                {
                    double strike = request["strike"].get<double>();
                    strat = std::make_shared<Straddle>(spot, strike, rate, volatility, time, isLong);
                }
                else if (strategy == "strangle")
                {
                    double callStrike = request.value("strike_call", request["strike"].get<double>() + 5.0);
                    double putStrike = request.value("strike_put", request["strike"].get<double>() - 5.0);
                    strat = std::make_shared<Strangle>(spot, callStrike, putStrike, rate, volatility, time, isLong);
                }
                else
                {
                    throw std::invalid_argument("Unknown strategy: " + strategy);
                }

                json response;
                response["strategy"] = strategy;
                response["is_long"] = isLong;
                response["price"] = strat->totalPrice();
                response["delta"] = strat->totalDelta();
                response["gamma"] = strat->totalGamma();
                response["vega"] = strat->totalVega();
                response["theta"] = strat->totalTheta();
                response["rho"] = strat->totalRho();
                response["num_legs"] = static_cast<int>(strat->getLegs().size());
                response["status"] = "success";

                return response;
            }
            catch (const std::exception &e)
            {
                json errorResponse;
                errorResponse["error"] = e.what();
                errorResponse["status"] = "error";
                return errorResponse;
            }
        }

        json PricingEndpoint::handleGreeksSurface(const json &request)
        {
            try
            {
                auto baseOption = createOptionFromJson(request);

                auto spotRange = request.value("spot_range", json::array({90.0, 110.0}));
                auto timeRange = request.value("time_range", json::array({0.1, 2.0}));
                int steps = request.value("steps", 10);

                double spotMin = spotRange[0].get<double>();
                double spotMax = spotRange[1].get<double>();
                double timeMin = timeRange[0].get<double>();
                double timeMax = timeRange[1].get<double>();

                json surface = json::array();

                for (int i = 0; i <= steps; ++i)
                {
                    double spot = spotMin + (spotMax - spotMin) * i / steps;
                    json timeSlice = json::array();

                    for (int j = 0; j <= steps; ++j)
                    {
                        double time = timeMin + (timeMax - timeMin) * j / steps;

                        // Create option with varied spot and time
                        auto variedOption = std::make_shared<EuropeanOption>(
                            spot, baseOption->getStrike(),
                            0.05, 0.2, time, request["type"].get<std::string>());

                        json point;
                        point["spot"] = spot;
                        point["time"] = time;
                        point["delta"] = variedOption->delta();
                        point["gamma"] = variedOption->gamma();
                        point["vega"] = variedOption->vega();

                        timeSlice.push_back(point);
                    }

                    surface.push_back(timeSlice);
                }

                json response;
                response["surface"] = surface;
                response["spot_range"] = spotRange;
                response["time_range"] = timeRange;
                response["status"] = "success";

                return response;
            }
            catch (const std::exception &e)
            {
                json errorResponse;
                errorResponse["error"] = e.what();
                errorResponse["status"] = "error";
                return errorResponse;
            }
        }

        json PricingEndpoint::handlePortfolioRequest(const json &request)
        {
            try
            {
                if (!request.contains("spot") || !request.contains("rate") || !request.contains("legs"))
                {
                    throw std::invalid_argument("Missing required parameters: spot, rate, legs");
                }

                double spot = request["spot"].get<double>();
                double rate = request["rate"].get<double>();
                auto legsArray = request["legs"];

                if (!legsArray.is_array() || legsArray.empty())
                {
                    throw std::invalid_argument("legs must be a non-empty array");
                }

                // Create strategy with all legs
                auto portfolio = std::make_shared<Strategy>();
                json legsResponse = json::array();
                double totalPrice = 0.0;
                double totalDelta = 0.0;
                double totalGamma = 0.0;
                double totalVega = 0.0;
                double totalTheta = 0.0;
                double totalRho = 0.0;

                for (const auto &legJson : legsArray)
                {
                    if (!legJson.contains("strike") || !legJson.contains("volatility") || !legJson.contains("time"))
                    {
                        throw std::invalid_argument("Each leg must have: strike, volatility, time");
                    }

                    // Extract option direction (call/put) and pricing model (european/american)
                    std::string optionDirection = legJson.value("optionType", "call");
                    std::string modelType = legJson.value("type", "european");

                    // Build full parameter set for this leg
                    json legParams;
                    legParams["spot"] = spot;
                    legParams["strike"] = legJson["strike"];
                    legParams["rate"] = rate;
                    legParams["volatility"] = legJson["volatility"];
                    legParams["time"] = legJson["time"];
                    legParams["type"] = optionDirection; // call/put direction
                    legParams["model"] = modelType;      // european/american model

                    auto option = createOptionFromJson(legParams);
                    int quantity = legJson.value("quantity", 1);

                    portfolio->addLeg(option, quantity);

                    // Accumulate greeks
                    double legPrice = option->price() * quantity;
                    totalPrice += legPrice;
                    totalDelta += option->delta() * quantity;
                    totalGamma += option->gamma() * quantity;
                    totalVega += option->vega() * quantity;
                    totalTheta += option->theta() * quantity;
                    totalRho += option->rho() * quantity;

                    // Build leg response
                    json legResponse;
                    legResponse["optionType"] = legJson.value("optionType", "call");
                    legResponse["model"] = modelType;
                    legResponse["strike"] = option->getStrike();
                    legResponse["price"] = option->price();
                    legResponse["quantity"] = quantity;
                    legResponse["delta"] = option->delta();
                    legResponse["gamma"] = option->gamma();
                    legResponse["vega"] = option->vega();
                    legResponse["theta"] = option->theta();
                    legResponse["rho"] = option->rho();
                    legsResponse.push_back(legResponse);
                }

                // Generate payoff diagram
                int payoffSteps = request.value("payoff_steps", 100);
                double spotMin = spot * 0.7; // 30% below current spot
                double spotMax = spot * 1.3; // 30% above current spot

                json payoffData = json::object();
                payoffData["spot_prices"] = json::array();
                payoffData["payoffs"] = json::array();

                for (int i = 0; i <= payoffSteps; ++i)
                {
                    double testSpot = spotMin + (spotMax - spotMin) * i / payoffSteps;
                    double totalPayoff = portfolio->payoff(testSpot);

                    payoffData["spot_prices"].push_back(testSpot);
                    payoffData["payoffs"].push_back(totalPayoff);
                }

                // Build response
                json response;
                response["portfolio"] = json::object();
                response["portfolio"]["spot"] = spot;
                response["portfolio"]["totalPrice"] = totalPrice;
                response["portfolio"]["greeks"] = json::object();
                response["portfolio"]["greeks"]["delta"] = totalDelta;
                response["portfolio"]["greeks"]["gamma"] = totalGamma;
                response["portfolio"]["greeks"]["vega"] = totalVega;
                response["portfolio"]["greeks"]["theta"] = totalTheta;
                response["portfolio"]["greeks"]["rho"] = totalRho;
                response["portfolio"]["legs"] = legsResponse;
                response["portfolio"]["payoff"] = payoffData;
                response["status"] = "success";

                return response;
            }
            catch (const std::exception &e)
            {
                json errorResponse;
                errorResponse["error"] = e.what();
                errorResponse["status"] = "error";
                return errorResponse;
            }
        }

    } // namespace API
} // namespace OptionPricer

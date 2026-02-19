#pragma once

#include <nlohmann/json.hpp>
#include "options/Option.h"
#include "strategy/Strategy.h"

namespace OptionPricer
{
    namespace API
    {

        using json = nlohmann::json;

        /**
         * @class PricingEndpoint
         * @brief Handles pricing requests and converts to/from JSON
         */
        class PricingEndpoint
        {
        public:
            /**
             * Handle single option pricing request
             *
             * Request JSON format:
             * {
             *   "type": "call" | "put",
             *   "model": "european" | "american",  // optional, defaults to european
             *   "spot": 100.0,
             *   "strike": 100.0,
             *   "rate": 0.05,
             *   "volatility": 0.2,
             *   "time": 1.0
             * }
             *
             * Response JSON format:
             * {
             *   "price": 10.45,
             *   "delta": 0.64,
             *   "gamma": 0.02,
             *   "vega": 39.45,
             *   "theta": -6.41,
             *   "rho": 53.23,
             *   "model": "european" | "american"
             * }
             */
            static json handlePriceRequest(const json &request);

            /**
             * Handle strategy pricing request
             *
             * Request JSON format:
             * {
             *   "strategy": "straddle" | "strangle",
             *   "spot": 100.0,
             *   "strike": 100.0,
             *   "strike_call": 105.0,  // for strangle
             *   "strike_put": 95.0,    // for strangle
             *   "is_long": true,
             *   "rate": 0.05,
             *   "volatility": 0.2,
             *   "time": 1.0
             * }
             *
             * Response: Same structure as price request, but aggregated
             */
            static json handleStrategyRequest(const json &request);

            /**
             * Handle Greeks surface request for plotting
             *
             * Request JSON format:
             * {
             *   "type": "call" | "put",
             *   "strike": 100.0,
             *   "rate": 0.05,
             *   "volatility": 0.2,
             *   "spot_range": [90, 110],
             *   "time_range": [0.1, 2.0],
             *   "steps": 10
             * }
             *
             * Response: 3D surface data for visualization
             */
            static json handleGreeksSurface(const json &request);

            /**
             * Handle multi-leg portfolio pricing request
             *
             * Request JSON format:
             * {
             *   "spot": 100.0,
             *   "rate": 0.05,
             *   "legs": [
             *     {
             *       "type": "european" | "american",
             *       "optionType": "call" | "put",
             *       "strike": 100.0,
             *       "volatility": 0.2,
             *       "time": 1.0,
             *       "quantity": 1
             *     },
             *     ...
             *   ]
             * }
             *
             * Response:
             * {
             *   "portfolio": {
             *     "totalPrice": 10.45,
             *     "greeks": {
             *       "delta": 0.64,
             *       "gamma": 0.02,
             *       "vega": 39.45,
             *       "theta": -6.41,
             *       "rho": 53.23
             *     },
             *     "legs": [...],
             *     "payoff": {
             *       "spot_prices": [...],
             *       "payoffs": [...]
             *     }
             *   }
             * }
             */
            static json handlePortfolioRequest(const json &request);

        private:
            /**
             * Create Option from JSON parameters
             */
            static std::shared_ptr<Option> createOptionFromJson(const json &params);

            /**
             * Build Greeks response object
             * @param model "european" or "american" — echoed back in the response
             */
            static json buildGreeksResponse(const std::shared_ptr<Option> &option,
                                            const std::string &model = "european");
        };

    } // namespace API
} // namespace OptionPricer

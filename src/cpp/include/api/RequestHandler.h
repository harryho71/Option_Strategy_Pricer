#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace OptionPricer
{
    namespace API
    {

        /**
         * @class RequestHandler
         * @brief HTTP request handler for REST API
         *
         * Parses incoming requests, routes to appropriate handlers,
         * and returns JSON responses.
         */
        class RequestHandler
        {
        public:
            /**
             * Handle single option pricing request
             * POST /price
             */
            static std::string handlePrice(const json &request);

            /**
             * Handle strategy pricing request
             * POST /strategy/price
             */
            static std::string handleStrategy(const json &request);

            /**
             * Handle Greeks surface request
             * GET /greeks/surface?greek=delta&spot_min=80&spot_max=120&steps=41
             */
            static std::string handleGreeksSurface(const json &params);

            /**
             * Handle implied volatility calculation
             * POST /implied_vol
             */
            static std::string handleImpliedVol(const json &request);

            /**
             * Handle health check request
             * GET /health
             */
            static std::string handleHealth();

            /**
             * Handle strategy list request
             * GET /strategies
             */
            static std::string handleStrategyList();

            /**
             * Validate request parameters
             */
            static bool validatePriceRequest(const json &request);
            static bool validateStrategyRequest(const json &request);
        };

    } // namespace API
} // namespace OptionPricer

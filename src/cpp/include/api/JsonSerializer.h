#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace OptionPricer
{
    namespace API
    {

        /**
         * @class JsonSerializer
         * @brief JSON serialization/deserialization for API communication
         *
         * Converts between C++ objects and JSON for REST API requests/responses.
         */
        class JsonSerializer
        {
        public:
            /**
             * Serialize option pricing result to JSON
             */
            static json serializeOptionResult(
                double price, double delta, double gamma,
                double vega, double theta, double rho);

            /**
             * Serialize strategy pricing result
             */
            static json serializeStrategyResult(
                double totalPrice, double totalDelta, double totalGamma,
                double totalVega, double totalTheta, double totalRho,
                int numLegs);

            /**
             * Serialize Greeks surface data
             */
            static json serializeGreeksSurface(
                const std::vector<std::vector<double>> &surface,
                const std::vector<double> &spots,
                const std::vector<double> &times,
                const std::string &greek);

            /**
             * Serialize error response
             */
            static json serializeError(const std::string &message, const std::string &code = "error");

            /**
             * Deserialize option parameters from JSON request
             */
            static void deserializeOptionParams(const json &request,
                                                double &S, double &K, double &r,
                                                double &sigma, double &T, std::string &type);

            /**
             * Deserialize strategy parameters from JSON request
             */
            static void deserializeStrategyParams(const json &request,
                                                  std::string &strategyName,
                                                  double &S, double &K1, double &K2,
                                                  double &r, double &sigma, double &T,
                                                  bool &isLong);
        };

    } // namespace API
} // namespace OptionPricer

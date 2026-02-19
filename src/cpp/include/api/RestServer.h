#pragma once

#include <string>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace OptionPricer
{
    namespace API
    {

        using json = nlohmann::json;

        // Request/Response handler types
        using RequestHandler = std::function<json(const json &)>;

        /**
         * @class RestServer
         * @brief Lightweight REST API server for option pricing
         *
         * Provides HTTP endpoints for:
         * - Single option pricing: POST /api/price
         * - Strategy pricing: POST /api/strategy/price
         * - Greeks calculation: GET /api/greeks
         * - Implied volatility: POST /api/iv
         *
         * Note: This is a simplified REST interface. In production, use cpp-httplib or Crow framework.
         */
        class RestServer
        {
        public:
            RestServer(int port = 8080);
            ~RestServer();

            /**
             * Register a request handler for a specific endpoint
             * @param endpoint HTTP endpoint path (e.g., "/api/price")
             * @param method HTTP method ("GET", "POST")
             * @param handler Function that processes JSON request and returns JSON response
             */
            void registerEndpoint(const std::string &endpoint,
                                  const std::string &method,
                                  RequestHandler handler);

            /**
             * Start the server (blocking call)
             * Server will listen on http://localhost:<port>
             */
            void start();

            /**
             * Stop the server gracefully
             */
            void stop();

            /**
             * Get server status
             * @return true if server is running
             */
            bool isRunning() const;

        private:
            int port_;
            bool running_;
            std::map<std::string, std::map<std::string, RequestHandler>> routes_;
        };

    } // namespace API
} // namespace OptionPricer

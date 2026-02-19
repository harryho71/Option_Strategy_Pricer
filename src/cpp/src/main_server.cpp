/**
 * @file main_server.cpp
 * @brief Example HTTP server implementation using cpp-httplib
 *
 * To build this server, first install cpp-httplib:
 *   - Windows (vcpkg): vcpkg install cpp-httplib:x64-windows
 *   - macOS: brew install cpp-httplib
 *   - Linux: sudo apt-get install libcpp-httplib-dev
 *
 * Then uncomment the pricing_server target in CMakeLists.txt and rebuild.
 *
 * Usage:
 *   ./pricing_server
 *   curl -X POST http://localhost:8080/api/price \
 *     -H "Content-Type: application/json" \
 *     -d '{"type":"call","spot":100,"strike":100,"rate":0.05,"volatility":0.2,"time":1.0}'
 */

#ifdef ENABLE_HTTP_SERVER

// Windows SDK version for CreateFile2 compatibility
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00 // Windows 10+
#endif

#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include "api/PricingEndpoint.h"

using json = nlohmann::json;

// CORS middleware
void setCorsHeaders(httplib::Response &res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main()
{
    httplib::Server svr;

    // Handle CORS preflight requests
    svr.Options(R"(/api/.*)", [](const httplib::Request & /*req*/, httplib::Response &res)
                {
        setCorsHeaders(res);
        res.status = 200; });

    // ============================================================================
    // POST /api/price - Single option pricing
    // ============================================================================
    svr.Post("/api/price", [](const httplib::Request &req, httplib::Response &res)
             {
        setCorsHeaders(res);
        try {
            auto reqJson = json::parse(req.body);
            auto respJson = OptionPricer::API::PricingEndpoint::handlePriceRequest(reqJson);
            res.set_content(respJson.dump(2), "application/json");
            res.status = respJson.contains("error") ? 400 : 200;
        } catch (const std::exception& e) {
            json errorRes;
            errorRes["error"] = e.what();
            errorRes["status"] = "error";
            res.set_content(errorRes.dump(2), "application/json");
            res.status = 400;
        } });

    // ============================================================================
    // POST /api/strategy/price - Multi-leg strategy pricing
    // ============================================================================
    svr.Post("/api/strategy/price", [](const httplib::Request &req, httplib::Response &res)
             {
        setCorsHeaders(res);
        try {
            auto reqJson = json::parse(req.body);
            auto respJson = OptionPricer::API::PricingEndpoint::handleStrategyRequest(reqJson);
            res.set_content(respJson.dump(2), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json errorRes;
            errorRes["error"] = e.what();
            errorRes["status"] = "error";
            res.set_content(errorRes.dump(2), "application/json");
            res.status = 400;
        } });

    // ============================================================================
    // POST /api/portfolio/price - Multi-leg portfolio pricing
    // ============================================================================
    svr.Post("/api/portfolio/price", [](const httplib::Request &req, httplib::Response &res)
             {
        setCorsHeaders(res);
        try {
            auto reqJson = json::parse(req.body);
            auto respJson = OptionPricer::API::PricingEndpoint::handlePortfolioRequest(reqJson);
            res.set_content(respJson.dump(2), "application/json");
            res.status = respJson.contains("error") ? 400 : 200;
        } catch (const std::exception& e) {
            json errorRes;
            errorRes["error"] = e.what();
            errorRes["status"] = "error";
            res.set_content(errorRes.dump(2), "application/json");
            res.status = 400;
        } });

    // ============================================================================
    // GET /api/greeks/surface - Greeks surface for visualization
    // ============================================================================
    svr.Get("/api/greeks/surface", [](const httplib::Request &req, httplib::Response &res)
            {
        setCorsHeaders(res);
        try {
            // Parse query parameters into JSON
            json params;
            if (req.has_param("type")) params["type"] = req.get_param_value("type");
            if (req.has_param("strike")) params["strike"] = std::stod(req.get_param_value("strike"));
            if (req.has_param("rate")) params["rate"] = std::stod(req.get_param_value("rate"));
            if (req.has_param("volatility")) params["volatility"] = std::stod(req.get_param_value("volatility"));
            if (req.has_param("spot_range")) {
                // Parse as JSON array: "spot_range=[90,110]"
                std::string rangeStr = req.get_param_value("spot_range");
                params["spot_range"] = json::parse(rangeStr);
            }
            if (req.has_param("time_range")) {
                std::string rangeStr = req.get_param_value("time_range");
                params["time_range"] = json::parse(rangeStr);
            }
            if (req.has_param("steps")) params["steps"] = std::stoi(req.get_param_value("steps"));

            auto respJson = OptionPricer::API::PricingEndpoint::handleGreeksSurface(params);
            res.set_content(respJson.dump(2), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json errorRes;
            errorRes["error"] = e.what();
            errorRes["status"] = "error";
            res.set_content(errorRes.dump(2), "application/json");
            res.status = 400;
        } });

    // ============================================================================
    // GET /health - Health check endpoint
    // ============================================================================
    svr.Get("/health", [](const httplib::Request & /*req*/, httplib::Response &res)
            {
        setCorsHeaders(res);
        json healthRes;
        healthRes["status"] = "healthy";
        healthRes["version"] = "1.0.0";
        res.set_content(healthRes.dump(), "application/json");
        res.status = 200; });

    // ============================================================================
    // GET /api/strategies - List available strategies
    // ============================================================================
    svr.Get("/api/strategies", [](const httplib::Request & /*req*/, httplib::Response &res)
            {
        setCorsHeaders(res);
        json strategies;
        strategies["strategies"] = json::array({
            {{"name", "straddle"}, {"description", "Long/short straddle (call + put at same strike)"}},
            {{"name", "strangle"}, {"description", "Long/short strangle (OTM call + OTM put)"}},
            {{"name", "bull_call"}, {"description", "Bull call spread (long lower call + short higher call)"}},
            {{"name", "iron_condor"}, {"description", "Iron condor (short strangle + long wider strangle)"}}
        });
        res.set_content(strategies.dump(2), "application/json");
        res.status = 200; });

    std::cout << "Option Strategy Pricer Server" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Starting server on http://localhost:8080" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl
              << std::endl;

    std::cout << "Available Endpoints:" << std::endl;
    std::cout << "  POST   /api/price              - Price single option" << std::endl;
    std::cout << "  POST   /api/strategy/price     - Price strategy" << std::endl;
    std::cout << "  POST   /api/portfolio/price    - Price multi-leg portfolio" << std::endl;
    std::cout << "  GET    /api/greeks/surface     - Get Greeks surface" << std::endl;
    std::cout << "  GET    /api/strategies         - List strategies" << std::endl;
    std::cout << "  GET    /health                 - Health check" << std::endl
              << std::endl;

    svr.listen("0.0.0.0", 8080);

    return 0;
}

#else

#include <iostream>

int main()
{
    std::cerr << "Error: HTTP server support not enabled." << std::endl;
    std::cerr << "Please install cpp-httplib and enable ENABLE_HTTP_SERVER in CMakeLists.txt" << std::endl;
    return 1;
}

#endif

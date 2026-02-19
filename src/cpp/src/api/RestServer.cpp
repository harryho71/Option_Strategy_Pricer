#include "api/RestServer.h"

namespace OptionPricer
{
    namespace API
    {

        RestServer::RestServer(int port)
            : port_(port), running_(false)
        {
        }

        RestServer::~RestServer()
        {
            if (running_)
            {
                stop();
            }
        }

        void RestServer::registerEndpoint(const std::string &endpoint,
                                          const std::string &method,
                                          RequestHandler handler)
        {
            routes_[endpoint][method] = handler;
        }

        void RestServer::start()
        {
            running_ = true;
            // TODO: Implement HTTP server using cpp-httplib or Crow framework
            // Example structure:
            // 1. Set up httplib::Server or crow::SimpleApp
            // 2. For each registered route, bind handler
            // 3. Listen on port_
            // 4. Call listen() in blocking mode
        }

        void RestServer::stop()
        {
            running_ = false;
        }

        bool RestServer::isRunning() const
        {
            return running_;
        }

    } // namespace API
} // namespace OptionPricer

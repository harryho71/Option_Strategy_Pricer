#include "strategy/IronCondor.h"
#include "options/EuropeanOption.h"
#include <stdexcept>

namespace OptionPricer
{

    IronCondor::IronCondor(double S,
                           double K_short_put, double K_short_call,
                           double K_long_put, double K_long_call,
                           double r, double sigma, double T)
    {

        // Validate strike ordering: K_long_put < K_short_put < K_short_call < K_long_call
        if (!(K_long_put < K_short_put && K_short_put < K_short_call && K_short_call < K_long_call))
        {
            throw std::invalid_argument(
                "IronCondor: Invalid strike ordering. Required: "
                "K_long_put < K_short_put < K_short_call < K_long_call");
        }

        // Short put at K_short_put
        auto shortPut = std::make_shared<EuropeanOption>(S, K_short_put, r, sigma, T, "put");
        legs_.push_back({shortPut, -1});

        // Long put at K_long_put (protective)
        auto longPut = std::make_shared<EuropeanOption>(S, K_long_put, r, sigma, T, "put");
        legs_.push_back({longPut, 1});

        // Short call at K_short_call
        auto shortCall = std::make_shared<EuropeanOption>(S, K_short_call, r, sigma, T, "call");
        legs_.push_back({shortCall, -1});

        // Long call at K_long_call (protective)
        auto longCall = std::make_shared<EuropeanOption>(S, K_long_call, r, sigma, T, "call");
        legs_.push_back({longCall, 1});
    }

} // namespace OptionPricer

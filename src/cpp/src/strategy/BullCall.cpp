#include "strategy/BullCall.h"
#include "options/EuropeanOption.h"
#include <stdexcept>

namespace OptionPricer
{

    BullCall::BullCall(double S, double K1, double K2, double r, double sigma, double T)
    {
        if (K1 >= K2)
        {
            throw std::invalid_argument("BullCall: K1 must be less than K2");
        }

        // Long call at lower strike K1
        auto longCall = std::make_shared<EuropeanOption>(S, K1, r, sigma, T, "call");
        legs_.push_back({longCall, 1});

        // Short call at higher strike K2
        auto shortCall = std::make_shared<EuropeanOption>(S, K2, r, sigma, T, "call");
        legs_.push_back({shortCall, -1});
    }

} // namespace OptionPricer

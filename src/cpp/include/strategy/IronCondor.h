#pragma once

#include "Strategy.h"

namespace OptionPricer
{

    /**
     * @class IronCondor
     * @brief Iron Condor Strategy
     *
     * Composition:
     * - Short strangle: short OTM call at K_short_call + short OTM put at K_short_put
     * - Long strangle: long OTM call at K_long_call + long OTM put at K_long_put
     * Where: K_short_put < K_short_call < K_long_call and K_short_put < K_short_call < K_long_put
     *
     * Profit: Limited to net credit received
     * Loss: Limited to max spread minus net credit
     * Best used when: Neutral view, want premium income, defined risk
     */
    class IronCondor : public Strategy
    {
    public:
        /**
         * @param S Spot price
         * @param K_short_put Short put strike (lowest)
         * @param K_short_call Short call strike
         * @param K_long_put Long put strike (below short put)
         * @param K_long_call Long call strike (above short call)
         * @param r Risk-free rate
         * @param sigma Volatility
         * @param T Time to expiration
         */
        IronCondor(double S,
                   double K_short_put, double K_short_call,
                   double K_long_put, double K_long_call,
                   double r, double sigma, double T);

        ~IronCondor() = default;
    };

} // namespace OptionPricer

#pragma once

#include "Strategy.h"

namespace OptionPricer
{

    /**
     * @class BullCall
     * @brief Bull Call Spread Strategy
     *
     * Composition:
     * - Long call at lower strike K1
     * - Short call at higher strike K2 (K2 > K1)
     *
     * Profit: Limited to K2 - K1 minus net premium paid
     * Loss: Limited to net premium paid
     * Best used when: Mildly bullish, want limited risk
     */
    class BullCall : public Strategy
    {
    public:
        /**
         * @param S Spot price
         * @param K1 Lower strike (long call)
         * @param K2 Higher strike (short call), must be > K1
         * @param r Risk-free rate
         * @param sigma Volatility
         * @param T Time to expiration
         */
        BullCall(double S, double K1, double K2, double r, double sigma, double T);

        ~BullCall() = default;
    };

} // namespace OptionPricer

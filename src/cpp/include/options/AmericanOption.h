#pragma once

#include <memory>
#include <string>
#include "Option.h"

namespace OptionPricer
{

    /**
     * @class AmericanOption
     * @brief American option pricing using binomial tree
     *
     * American options can be exercised at any time before expiration,
     * requiring numerical methods like binomial trees or Monte Carlo.
     */
    class AmericanOption : public Option
    {
    private:
        int steps_; // Number of steps in binomial tree

    public:
        /**
         * @param S Spot price
         * @param K Strike price
         * @param r Risk-free rate
         * @param sigma Volatility
         * @param T Time to expiration
         * @param type "call" or "put"
         * @param steps Number of binomial tree steps (default 100)
         */
        AmericanOption(double S, double K, double r, double sigma, double T,
                       const std::string &type, int steps = 100);

        // Pricing and Greeks
        double price() const override;
        double delta() const override;
        double gamma() const override;
        double vega() const override;
        double theta() const override;
        double rho() const override;

    private:
        // Binomial tree implementation
        double binomialPrice() const;
        double binomialDelta() const;
    };

} // namespace OptionPricer

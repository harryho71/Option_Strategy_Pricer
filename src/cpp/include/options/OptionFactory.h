#pragma once

#include <memory>
#include <string>
#include "Option.h"
#include "EuropeanOption.h"
#include "AmericanOption.h"

namespace OptionPricer
{

    /**
     * @class OptionFactory
     * @brief Factory for creating Option objects
     *
     * Uses factory pattern to encapsulate option creation logic.
     */
    class OptionFactory
    {
    public:
        /**
         * Create an option of the specified type
         * @param optionType "european" or "american"
         * @param S Spot price
         * @param K Strike price
         * @param r Risk-free rate
         * @param sigma Volatility
         * @param T Time to expiration
         * @param type "call" or "put"
         * @param steps Binomial tree steps (for American options)
         * @return Shared pointer to created Option
         */
        static std::shared_ptr<Option> create(
            const std::string &optionType,
            double S, double K, double r, double sigma, double T,
            const std::string &type, int steps = 100);
    };

} // namespace OptionPricer

#pragma once

#include <cmath>
#include <string>

namespace OptionPricer
{
    namespace Greeks
    {

        /**
         * @namespace Greeks
         * @brief Greeks calculation functions
         *
         * Provides individual Greek calculations and sensitivities.
         * All Greeks are implemented for both European and American options.
         */

        /**
         * Delta - First-order sensitivity to spot price changes
         * Represents the hedge ratio needed to delta-hedge a position
         * @param S Spot price
         * @param K Strike price
         * @param r Risk-free rate
         * @param sigma Volatility
         * @param T Time to expiration
         * @param type "call" or "put"
         * @return Delta value (typically -1 to 1)
         */
        double delta(double S, double K, double r, double sigma, double T,
                     const std::string &type);

        /**
         * Gamma - Second-order sensitivity (rate of delta change)
         * Measures convexity/curvature of the option price
         * Important for hedging large moves
         * @return Gamma value (always positive for long options)
         */
        double gamma(double S, double K, double r, double sigma, double T);

        /**
         * Vega - Sensitivity to volatility changes
         * Measures impact of 1% change in volatility
         * @return Vega value (typically per 1% volatility move)
         */
        double vega(double S, double K, double r, double sigma, double T);

        /**
         * Theta - Time decay
         * Measures daily P&L due to passage of time
         * Long options typically have negative theta (lose value)
         * @return Theta value (typically annualized, divide by 365 for daily)
         */
        double theta(double S, double K, double r, double sigma, double T,
                     const std::string &type);

        /**
         * Rho - Interest rate sensitivity
         * Measures impact of 1% change in risk-free rate
         * More important for long-dated options
         * @return Rho value (per 1% rate move)
         */
        double rho(double S, double K, double r, double sigma, double T,
                   const std::string &type);

        /**
         * Vanna - Mixed Greek (Δ sensitivity to volatility)
         * Measures how delta changes with volatility
         */
        double vanna(double S, double K, double r, double sigma, double T);

        /**
         * Volga - Vega sensitivity to volatility changes
         * Second-order volatility sensitivity
         */
        double volga(double S, double K, double r, double sigma, double T);

        /**
         * Charm - Delta decay (theta of delta)
         * Measures how delta changes with time
         */
        double charm(double S, double K, double r, double sigma, double T,
                     const std::string &type);

    } // namespace Greeks
} // namespace OptionPricer

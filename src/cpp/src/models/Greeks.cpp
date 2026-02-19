#include "models/Greeks.h"
#include "models/BlackScholes.h"
#include <cmath>

namespace OptionPricer
{
    namespace Greeks
    {

        double delta(double S, double K, double r, double sigma, double T,
                     const std::string &type)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double N_d1 = 0.5 * (1.0 + std::erf(d1 / std::sqrt(2.0)));

            if (type == "call")
            {
                return N_d1;
            }
            else
            {
                return N_d1 - 1.0;
            }
        }

        double gamma(double S, double K, double r, double sigma, double T)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);
            return phi_d1 / (S * sigma * std::sqrt(T));
        }

        double vega(double S, double K, double r, double sigma, double T)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);
            return S * phi_d1 * std::sqrt(T) / 100.0; // Per 1% volatility
        }

        double theta(double S, double K, double r, double sigma, double T,
                     const std::string &type)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double d2 = BlackScholes::d2(S, K, r, sigma, T);
            double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);
            double N_d2 = 0.5 * (1.0 + std::erf(d2 / std::sqrt(2.0)));

            double sqrtT = std::sqrt(T);
            double decay = -S * phi_d1 * sigma / (2.0 * sqrtT);

            if (type == "call")
            {
                double rf_term = -r * K * std::exp(-r * T) * N_d2;
                return (decay + rf_term) / 365.0; // Per day
            }
            else
            {
                double N_minus_d2 = 1.0 - N_d2;
                double rf_term = r * K * std::exp(-r * T) * N_minus_d2;
                return (decay + rf_term) / 365.0;
            }
        }

        double rho(double S, double K, double r, double sigma, double T,
                   const std::string &type)
        {
            double d2 = BlackScholes::d2(S, K, r, sigma, T);
            double N_d2 = 0.5 * (1.0 + std::erf(d2 / std::sqrt(2.0)));

            if (type == "call")
            {
                return K * T * std::exp(-r * T) * N_d2 / 100.0; // Per 1% rate
            }
            else
            {
                double N_minus_d2 = 1.0 - N_d2;
                return -K * T * std::exp(-r * T) * N_minus_d2 / 100.0;
            }
        }

        double vanna(double S, double K, double r, double sigma, double T)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double d2 = BlackScholes::d2(S, K, r, sigma, T);
            double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);

            return -phi_d1 * d2 / sigma;
        }

        double volga(double S, double K, double r, double sigma, double T)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double d2 = BlackScholes::d2(S, K, r, sigma, T);
            double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);

            return S * phi_d1 * std::sqrt(T) * d1 * d2 / sigma;
        }

        double charm(double S, double K, double r, double sigma, double T,
                     const std::string &type)
        {
            double d1 = BlackScholes::d1(S, K, r, sigma, T);
            double d2 = BlackScholes::d2(S, K, r, sigma, T);
            double phi_d1 = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI);

            double sqrtT = std::sqrt(T);
            double common = -r * phi_d1 / (sigma * sqrtT);

            if (type == "call")
            {
                return common * d1 - r * std::exp(-r * T) * d2;
            }
            else
            {
                return -common * d1 + r * std::exp(-r * T) * d2;
            }
        }

    } // namespace Greeks
} // namespace OptionPricer

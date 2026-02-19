#include "../../cpp/include/models/BlackScholes.h"
#include <cmath>

namespace BlackScholes
{
    static const double PI = 3.14159265358979323846;

    double standardNormal(double x)
    {
        return std::exp(-0.5 * x * x) / std::sqrt(2.0 * PI);
    }

    double cumulativeNormal(double x)
    {
        return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
    }

    double d1(double S, double K, double r, double sigma, double T)
    {
        return (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    }

    double d2(double S, double K, double r, double sigma, double T)
    {
        return d1(S, K, r, sigma, T) - sigma * std::sqrt(T);
    }

    double callPrice(double S, double K, double r, double sigma, double T)
    {
        if (T <= 0)
            return std::max(0.0, S - K);
        double D1 = d1(S, K, r, sigma, T);
        double D2 = d2(S, K, r, sigma, T);
        return S * cumulativeNormal(D1) - K * std::exp(-r * T) * cumulativeNormal(D2);
    }

    double putPrice(double S, double K, double r, double sigma, double T)
    {
        if (T <= 0)
            return std::max(0.0, K - S);
        double D1 = d1(S, K, r, sigma, T);
        double D2 = d2(S, K, r, sigma, T);
        return K * std::exp(-r * T) * cumulativeNormal(-D2) - S * cumulativeNormal(-D1);
    }

    double delta(double S, double K, double r, double sigma, double T, const std::string &type)
    {
        double D1 = d1(S, K, r, sigma, T);
        if (type == "call")
            return cumulativeNormal(D1);
        return cumulativeNormal(D1) - 1.0;
    }

    double gamma(double S, double K, double r, double sigma, double T)
    {
        double D1 = d1(S, K, r, sigma, T);
        return standardNormal(D1) / (S * sigma * std::sqrt(T));
    }

    double vega(double S, double K, double r, double sigma, double T)
    {
        double D1 = d1(S, K, r, sigma, T);
        return S * standardNormal(D1) * std::sqrt(T);
    }

    double theta(double S, double K, double r, double sigma, double T, const std::string &type)
    {
        // Approximate daily theta (per year) for simplicity
        double D1 = d1(S, K, r, sigma, T);
        double D2 = d2(S, K, r, sigma, T);
        double first = -(S * standardNormal(D1) * sigma) / (2.0 * std::sqrt(T));
        if (type == "call")
        {
            double second = -r * K * std::exp(-r * T) * cumulativeNormal(D2);
            return first + second;
        }
        else
        {
            double second = r * K * std::exp(-r * T) * cumulativeNormal(-D2);
            return first + second;
        }
    }

    double rho(double S, double K, double r, double sigma, double T, const std::string &type)
    {
        double D2 = d2(S, K, r, sigma, T);
        if (type == "call")
            return K * T * std::exp(-r * T) * cumulativeNormal(D2);
        return -K * T * std::exp(-r * T) * cumulativeNormal(-D2);
    }
}

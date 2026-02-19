#pragma once
#include <string>

namespace BlackScholes
{
    double standardNormal(double x);
    double cumulativeNormal(double x);
    double d1(double S, double K, double r, double sigma, double T);
    double d2(double S, double K, double r, double sigma, double T);

    double callPrice(double S, double K, double r, double sigma, double T);
    double putPrice(double S, double K, double r, double sigma, double T);

    double delta(double S, double K, double r, double sigma, double T, const std::string &type);
    double gamma(double S, double K, double r, double sigma, double T);
    double vega(double S, double K, double r, double sigma, double T);
    double theta(double S, double K, double r, double sigma, double T, const std::string &type);
    double rho(double S, double K, double r, double sigma, double T, const std::string &type);
}

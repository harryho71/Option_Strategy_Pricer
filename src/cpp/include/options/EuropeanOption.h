#pragma once
#include "options/Option.h"
#include "models/BlackScholes.h"
#include <string>

class EuropeanOption : public Option
{
public:
    EuropeanOption(double S, double K, double r, double sigma, double T, const std::string &type)
        : Option(S, K, r, sigma, T, type) {}

    double price() const override;
    double delta() const override;
    double gamma() const override;
    double vega() const override;
    double theta() const override;
    double rho() const override;
};

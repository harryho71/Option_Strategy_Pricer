#pragma once
#include "strategy/Strategy.h"
#include "options/EuropeanOption.h"

class Strangle : public Strategy
{
public:
    Strangle(double S, double K_call, double K_put, double r, double sigma, double T, bool isLong)
    {
        auto call = std::make_shared<EuropeanOption>(S, K_call, r, sigma, T, "call");
        auto put = std::make_shared<EuropeanOption>(S, K_put, r, sigma, T, "put");
        addLeg(call, isLong ? 1 : -1);
        addLeg(put, isLong ? 1 : -1);
    }
};

#pragma once
#include "strategy/Strategy.h"
#include "options/EuropeanOption.h"

class Straddle : public Strategy
{
public:
    Straddle(double S, double K, double r, double sigma, double T, bool isLong)
    {
        auto call = std::make_shared<EuropeanOption>(S, K, r, sigma, T, "call");
        auto put = std::make_shared<EuropeanOption>(S, K, r, sigma, T, "put");
        addLeg(call, isLong ? 1 : -1);
        addLeg(put, isLong ? 1 : -1);
    }
};

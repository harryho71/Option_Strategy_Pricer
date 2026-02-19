#include "strategy/Strategy.h"
#include <algorithm>

double Strategy::payoff(double spotPrice) const
{
    double pnl = 0.0;
    for (const auto &leg : legs_)
    {
        auto option = leg.option;
        int qty = leg.quantity;

        // Calculate intrinsic value at expiration (time=0)
        double strike = option->getStrike();
        double intrinsic = 0.0;

        if (option->type() == "call")
        {
            intrinsic = std::max(0.0, spotPrice - strike);
        }
        else
        {
            intrinsic = std::max(0.0, strike - spotPrice);
        }

        // Use stored initial premium (from when leg was added)
        double initial_premium = leg.initialPremium;

        // P&L = (intrinsic value - initial premium) * quantity
        pnl += qty * (intrinsic - initial_premium);
    }
    return pnl;
}

double Strategy::totalPrice() const
{
    double total = 0.0;
    for (const auto &leg : legs_)
    {
        total += leg.quantity * leg.initialPremium;
    }
    return total;
}

double Strategy::totalDelta() const
{
    double delta = 0.0;
    for (const auto &leg : legs_)
    {
        delta += leg.quantity * leg.option->delta();
    }
    return delta;
}

double Strategy::totalGamma() const
{
    double gamma = 0.0;
    for (const auto &leg : legs_)
    {
        gamma += leg.quantity * leg.option->gamma();
    }
    return gamma;
}

double Strategy::totalVega() const
{
    double vega = 0.0;
    for (const auto &leg : legs_)
    {
        vega += leg.quantity * leg.option->vega();
    }
    return vega;
}

double Strategy::totalTheta() const
{
    double theta = 0.0;
    for (const auto &leg : legs_)
    {
        theta += leg.quantity * leg.option->theta();
    }
    return theta;
}

double Strategy::totalRho() const
{
    double rho = 0.0;
    for (const auto &leg : legs_)
    {
        rho += leg.quantity * leg.option->rho();
    }
    return rho;
}

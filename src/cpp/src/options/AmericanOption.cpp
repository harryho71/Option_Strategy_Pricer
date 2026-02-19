#include "options/AmericanOption.h"
#include "models/BlackScholes.h"
#include <algorithm>
#include <cmath>

namespace OptionPricer
{

    AmericanOption::AmericanOption(double S, double K, double r, double sigma, double T,
                                   const std::string &type, int steps)
        : Option(S, K, r, sigma, T, type), steps_(steps)
    {
    }

    double AmericanOption::price() const
    {
        return binomialPrice();
    }

    double AmericanOption::delta() const
    {
        return binomialDelta();
    }

    double AmericanOption::gamma() const
    {
        // Numerical approximation: (delta_up - delta_down) / (2 * dS)
        double h = spot_ * 0.01; // 1% bump
        AmericanOption optionUp(spot_ + h, strike_, rate_, sigma_, time_, type_, steps_);
        AmericanOption optionDown(spot_ - h, strike_, rate_, sigma_, time_, type_, steps_);

        double deltaUp = optionUp.delta();
        double deltaDown = optionDown.delta();

        return (deltaUp - deltaDown) / (2.0 * h);
    }

    double AmericanOption::vega() const
    {
        // Numerical approximation
        double h = 0.01; // 1% change
        AmericanOption optionUp(spot_, strike_, rate_, sigma_ + h, time_, type_, steps_);
        AmericanOption optionDown(spot_, strike_, rate_, sigma_ - h, time_, type_, steps_);

        return (optionUp.price() - optionDown.price()) / (2.0 * h);
    }

    double AmericanOption::theta() const
    {
        // Numerical approximation
        double dt = 1.0 / 252.0; // 1 day
        AmericanOption optionTomorrow(spot_, strike_, rate_, sigma_, std::max(0.0, time_ - dt), type_, steps_);

        return (optionTomorrow.price() - price()) / dt;
    }

    double AmericanOption::rho() const
    {
        // Numerical approximation
        double h = 0.01; // 1% change
        AmericanOption optionUp(spot_, strike_, rate_ + h, sigma_, time_, type_, steps_);
        AmericanOption optionDown(spot_, strike_, rate_ - h, sigma_, time_, type_, steps_);

        return (optionUp.price() - optionDown.price()) / (2.0 * h);
    }

    double AmericanOption::binomialPrice() const
    {
        // Build binomial tree
        double dt = time_ / steps_;
        double u = std::exp(sigma_ * std::sqrt(dt));
        double d = 1.0 / u;
        double p = (std::exp(rate_ * dt) - d) / (u - d);

        // Initialize price tree
        std::vector<std::vector<double>> priceTree(steps_ + 1, std::vector<double>(steps_ + 1, 0.0));

        // Fill terminal nodes
        for (int j = 0; j <= steps_; ++j)
        {
            double S_T = spot_ * std::pow(u, steps_ - 2.0 * j);
            double payoff = (type_ == "call") ? std::max(0.0, S_T - strike_) : std::max(0.0, strike_ - S_T);
            priceTree[steps_][j] = payoff;
        }

        // Work backwards through tree
        for (int i = steps_ - 1; i >= 0; --i)
        {
            for (int j = 0; j <= i; ++j)
            {
                double S_t = spot_ * std::pow(u, i - 2.0 * j);
                double discountedValue = std::exp(-rate_ * dt) *
                                         (p * priceTree[i + 1][j] + (1.0 - p) * priceTree[i + 1][j + 1]);

                // American option: max of holding and exercising
                double payoff = (type_ == "call") ? std::max(0.0, S_t - strike_) : std::max(0.0, strike_ - S_t);

                priceTree[i][j] = std::max(discountedValue, payoff);
            }
        }

        return priceTree[0][0];
    }

    double AmericanOption::binomialDelta() const
    {
        double dt = time_ / steps_;
        double u = std::exp(sigma_ * std::sqrt(dt));
        double d = 1.0 / u;
        double p = (std::exp(rate_ * dt) - d) / (u - d);

        // Calculate delta as (price up - price down) / (2 * S * (u - d))
        double S_up = spot_ * u;
        double S_down = spot_ * d;

        AmericanOption optionUp(S_up, strike_, rate_, sigma_, time_, type_, steps_);
        AmericanOption optionDown(S_down, strike_, rate_, sigma_, time_, type_, steps_);

        return (optionUp.price() - optionDown.price()) / (spot_ * (u - d));
    }

} // namespace OptionPricer

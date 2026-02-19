#include "../../cpp/include/options/EuropeanOption.h"

double EuropeanOption::price() const
{
    if (type_ == "call")
        return BlackScholes::callPrice(spot_, strike_, rate_, sigma_, time_);
    return BlackScholes::putPrice(spot_, strike_, rate_, sigma_, time_);
}

double EuropeanOption::delta() const
{
    return BlackScholes::delta(spot_, strike_, rate_, sigma_, time_, type_);
}

double EuropeanOption::gamma() const
{
    return BlackScholes::gamma(spot_, strike_, rate_, sigma_, time_);
}

double EuropeanOption::vega() const
{
    return BlackScholes::vega(spot_, strike_, rate_, sigma_, time_);
}

double EuropeanOption::theta() const
{
    return BlackScholes::theta(spot_, strike_, rate_, sigma_, time_, type_);
}

double EuropeanOption::rho() const
{
    return BlackScholes::rho(spot_, strike_, rate_, sigma_, time_, type_);
}

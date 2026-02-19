#include "options/OptionFactory.h"
#include <stdexcept>

namespace OptionPricer
{

    std::shared_ptr<Option> OptionFactory::create(
        const std::string &optionType,
        double S, double K, double r, double sigma, double T,
        const std::string &type, int steps)
    {

        if (optionType == "european" || optionType == "european_option")
        {
            return std::make_shared<EuropeanOption>(S, K, r, sigma, T, type);
        }
        else if (optionType == "american" || optionType == "american_option")
        {
            return std::make_shared<AmericanOption>(S, K, r, sigma, T, type, steps);
        }
        else
        {
            throw std::invalid_argument("Unknown option type: " + optionType);
        }
    }

} // namespace OptionPricer

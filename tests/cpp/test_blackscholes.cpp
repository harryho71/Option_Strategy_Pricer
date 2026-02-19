#include <iostream>
#include <cmath>
#include "models/BlackScholes.h"

int main()
{
    double S = 100.0, K = 100.0, r = 0.05, sigma = 0.2, T = 1.0;
    double price = BlackScholes::callPrice(S, K, r, sigma, T);
    double delta = BlackScholes::delta(S, K, r, sigma, T, "call");

    std::cout << "Call price: " << price << std::endl;
    std::cout << "Delta: " << delta << std::endl;

    if (std::abs(price - 10.45) > 0.05)
    {
        std::cerr << "Price deviates from reference: " << price << std::endl;
        return 2;
    }
    if (std::abs(delta - 0.64) > 0.05)
    {
        std::cerr << "Delta deviates from reference: " << delta << std::endl;
        return 3;
    }

    std::cout << "Black-Scholes smoke test passed" << std::endl;
    return 0;
}

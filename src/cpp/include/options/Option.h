#pragma once
#include <string>

class Option
{
protected:
    double spot_;      // Current price
    double strike_;    // Strike price
    double rate_;      // Risk-free rate
    double sigma_;     // Volatility
    double time_;      // Time to expiry (years)
    std::string type_; // "call" or "put"

public:
    Option(double S, double K, double r, double sigma, double T,
           const std::string &type)
        : spot_(S), strike_(K), rate_(r), sigma_(sigma), time_(T), type_(type) {}

    virtual ~Option() = default;

    virtual double price() const = 0;
    virtual double delta() const = 0;
    virtual double gamma() const = 0;
    virtual double vega() const = 0;
    virtual double theta() const = 0;
    virtual double rho() const = 0;

    double getSpot() const { return spot_; }
    double getStrike() const { return strike_; }
    void setSpot(double S) { spot_ = S; }
    void setSigma(double sigma) { sigma_ = sigma; }
    const std::string &type() const { return type_; }
};

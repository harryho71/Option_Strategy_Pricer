#pragma once
#include <vector>
#include <memory>
#include "options/Option.h"

class Strategy
{
protected:
    struct Leg
    {
        std::shared_ptr<Option> option;
        int quantity;
        double initialPremium; // Store initial premium for payoff calculation
    };
    std::vector<Leg> legs_;

public:
    virtual ~Strategy() = default;

    void addLeg(std::shared_ptr<Option> option, int quantity)
    {
        legs_.push_back({option, quantity, option->price()});
    }

    const std::vector<Leg> &getLegs() const
    {
        return legs_;
    }

    double payoff(double spotPrice) const;
    double totalPrice() const;
    double totalDelta() const;
    double totalGamma() const;
    double totalVega() const;
    double totalTheta() const;
    double totalRho() const;
};

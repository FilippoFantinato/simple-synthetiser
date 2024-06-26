#ifndef SIMPLE_SAFETY_SOLVER_H
#define SIMPLE_SAFETY_SOLVER_H

#include <iostream>

#include "./BDD2Aiger.h"
#include "./GameSolver.h"

class SimpleSafetySolver : public GameSolver
{
private:
    std::vector<BDD> get_strategies(const BDD& winning_region) override;

public:
    SimpleSafetySolver(const SafetyArena& arena, const Cudd& manager);

    BDD solve() override;
    aiger* synthesize(const BDD& winning_region) override;
};

#endif

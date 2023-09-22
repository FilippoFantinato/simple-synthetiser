#include "./SimpleSafetySolver.h"

SimpleSafetySolver::SimpleSafetySolver(const SafetyArena& arena, const Cudd& manager) 
    : SafetySolver(arena, manager)
{
}

BDD SimpleSafetySolver::solve()
{
    BDD fixpoint  = _manager.bddZero();
    BDD attractor = ~_arena.safety_condition();

    unsigned round = 0;
    while(fixpoint != attractor)
    {
        std::cout << "Round: " << ++round << std::endl;

        fixpoint = attractor;
        
        BDD controlled_predecessor = attractor.VectorCompose(_arena.compose())
            .AndAbstract(_arena.safety_condition(), _controllable_cube)
            .UnivAbstract(_uncontrollable_cube);
        
        attractor = attractor | controlled_predecessor;
        
        std::cout << fixpoint << std::endl;
        std::cout << attractor << std::endl;
    }

    std::cout << "END" << std::endl;
    std::cout << ~attractor << std::endl;

    return attractor;
}

aiger* SimpleSafetySolver::synthesize(const BDD& winning_region)
{
    std::vector<BDD> strategies = get_strategies(winning_region);

    BDD2Aiger encoder(_manager);

    const auto& uncontrollables       = _arena.uncontrollables();
    const auto& uncontrollables_names = _arena.uncontrollables_names();
    for(size_t i = 0; i < uncontrollables.size(); ++i)
    {
        encoder.add_input(uncontrollables[i], uncontrollables_names[i]);
    }

    const auto& latches       = _arena.latches();
    const auto& latches_names = _arena.latches_names();
    for(size_t i = 0; i < _arena.latches().size(); ++i)
    {
        encoder.add_latch(latches[i], latches_names[i]);
    }

    const auto& controllables_names = _arena.controllables_names();
    for(size_t i = 0; i < controllables_names.size(); ++i)
    {
        encoder.add_output(strategies[i], controllables_names[i]);
    }

    return encoder.get_encoding();
}

std::vector<BDD> SimpleSafetySolver::get_strategies(const BDD& winning_region)
{
    BDD care_set = winning_region;
    BDD nondeterministic_strategy = winning_region.VectorCompose(_arena.compose()) & _arena.safety_condition();
    const std::vector<BDD>& controllables = _arena.controllables();

    std::vector<BDD> strategies;
    for(auto c: controllables)
    {
        BDD winning_controllables = nondeterministic_strategy;

        std::vector<BDD> other_controllables;
        for(auto o_c: controllables) if(o_c != c) other_controllables.push_back(o_c);

        if(other_controllables.size() > 0)
        {
            winning_controllables = winning_controllables.ExistAbstract(
                std::accumulate(other_controllables.begin(), other_controllables.end(), _manager.bddOne(), [](const BDD& acc, const BDD& el){return acc&el;})
            );
        }

        BDD canBeTrue   = winning_controllables.Cofactor(c);
        BDD canBeFalse  = winning_controllables.Cofactor(~c);
        BDD mustBeTrue  = (~canBeFalse) & canBeTrue;
        BDD mustBeFalse = (~canBeTrue) & canBeFalse;

        BDD local_care_set = care_set & (mustBeTrue | mustBeFalse);

        BDD model_true = mustBeTrue.Restrict(local_care_set);
        BDD model_false = (~mustBeFalse).Restrict(local_care_set);

        BDD model = model_true.nodeCount() < model_false.nodeCount() ? model_true : model_false;

        strategies.push_back(model);
        
        nondeterministic_strategy &= c.Xnor(model);
    }

    return strategies;
}


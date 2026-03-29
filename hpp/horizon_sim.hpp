// #ifndef HORIZON_SIM_HPP
// #define HORIZON_SIM_HPP

// #include "../mcts/include/mcts.hpp"
// #include "defs.hpp"
// #include "sequencer.hpp"
// #include "bind_map.hpp"
// #include "goal_store.hpp"
// #include "candidate_store.hpp"
// #include "weight_store.hpp"
// #include "mcts_decider.hpp"
// #include "cdcl.hpp"

// struct horizon_sim {
//     horizon_sim(
//         size_t,
//         const database&,
//         const goals&,
//         trail&,
//         sequencer&,
//         expr_pool&,
//         bind_map&,
//         lineage_pool&,
//         cdcl c,
//         monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
//     );
//     bool operator()();
//     const resolutions& get_resolutions() const;
//     const decisions& get_decisions() const;
// #ifndef DEBUG
// private:
// #endif
//     void resolve(const resolution_lineage*);

//     size_t max_resolutions;

//     const database& db;
//     trail& t;
//     lineage_pool& lp;
    
//     resolutions rs;
//     decisions ds;
    
//     copier cp;

//     goal_store gs;
//     candidate_store cs;
//     weight_store ws;
    
//     mcts_decider dec;
    
//     cdcl c;
// };

// #endif

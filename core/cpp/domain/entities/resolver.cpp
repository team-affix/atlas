#include "../../../hpp/domain/entities/resolver.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

resolver::resolver(size_t initial_goal_count) :
    db(locator::locate<i_database>()),
    lp(locator::locate<i_lineage_pool>()),
    frontier(locator::locate<i_frontier>()),
    goal_factory(locator::locate<i_factory<goal>>()),
    candidate_factory(locator::locate<i_factory<candidate>>()),
    goal_initializer(locator::locate<i_goal_initializer>()),
    candidate_initializer(locator::locate<i_candidate_initializer>()),
    resolving_producer(locator::locate<i_event_producer<resolving_event>>()),
    resolved_producer(locator::locate<i_event_producer<resolved_event>>()),
    goal_activating_producer(locator::locate<i_event_producer<goal_activating_event>>()),
    goal_activated_producer(locator::locate<i_event_producer<goal_activated_event>>()),
    goal_deactivating_producer(locator::locate<i_event_producer<goal_deactivating_event>>()),
    goal_deactivated_producer(locator::locate<i_event_producer<goal_deactivated_event>>()),
    resolve_yielded_producer(locator::locate<i_event_producer<resolve_yielded_event>>()),
    candidate_activating_producer(locator::locate<i_event_producer<candidate_activating_event>>()),
    candidate_activated_producer(locator::locate<i_event_producer<candidate_activated_event>>()),
    candidate_deactivating_producer(locator::locate<i_event_producer<candidate_deactivating_event>>()),
    candidate_deactivated_producer(locator::locate<i_event_producer<candidate_deactivated_event>>()),
    initial_goal_count(initial_goal_count),
    resolve_state_machine(std::nullopt) {
}

void resolver::init_resolve(const resolution_lineage* rl) {
    size_t body_size = rl ? db.at(rl->idx).body.size() : initial_goal_count;
    
    // start the state machine
    resolve_state_machine = resolve(rl, body_size);
    
    // yield to start the state machine
    resolve_yielded_producer.produce({});
}

void resolver::resume() {
    // resume the state machine
    resolve_state_machine->resume();

    // if the state machine is not done, yield
    if (!resolve_state_machine->done())
        resolve_yielded_producer.produce({});
}

state_machine resolver::resolve(const resolution_lineage* rl, size_t body_size) {
    // set up the goal initializer
    goal_initializer.seed_expansion(rl);
    
    // emit resolving event
    resolving_producer.produce({rl});
    co_await std::suspend_always{};

    // activate goals
    for (size_t i = 0; i < body_size; ++i) {
        // get the goal lineage
        const goal_lineage* gl = lp.goal(rl, i);

        // make the goal
        auto g = goal_factory.make();

        // get the raw
        auto raw_g = g.get();

        // insert the goal into the frontier
        frontier.insert(gl, std::move(g));

        // initialize the goal
        goal_initializer.initialize(gl);
        
        // emit goal activating event
        goal_activating_producer.produce({gl});
        co_await std::suspend_always{};

        // set up the candidate initializer
        candidate_initializer.seed_expansion(gl);

        // activate candidates
        for (size_t j = 0; j < db.size(); ++j) {
            // get the candidate lineage
            const resolution_lineage* candidate_rl = lp.resolution(gl, j);

            // make the candidate
            auto c = candidate_factory.make();

            // insert the candidate into the frontier
            g->candidates.insert({j, std::move(c)});

            // initialize the candidate
            candidate_initializer.initialize(candidate_rl);
            
            // emit candidate activating event
            candidate_activating_producer.produce({candidate_rl});
            co_await std::suspend_always{};

            // emit candidate activated event
            candidate_activated_producer.produce({candidate_rl});
            co_await std::suspend_always{};
        }
        
        // emit goal activated event
        goal_activated_producer.produce({gl});
        co_await std::suspend_always{};
    }

    // if rl is nullptr, emit resolved event
    if (!rl) {
        resolved_producer.produce({rl});
        co_return;
    }

    // get parent goal
    const goal_lineage* parent_gl = rl->parent;

    // get parent goal
    auto& parent_goal = frontier.at(parent_gl);
    
    // emit goal_deactivating_event
    goal_deactivating_producer.produce({parent_gl});
    co_await std::suspend_always{};

    // deactivate parent candidates
    for (auto it = parent_goal->candidates.begin(); it != parent_goal->candidates.end();) {
        // get the index
        size_t idx = it->first;

        // pre-advance the iterator before erasure
        auto curr = it++;

        // erase the candidate
        parent_goal->candidates.erase(curr);

        // get the candidate lineage
        const resolution_lineage* candidate_rl = lp.resolution(parent_gl, idx);

        // emit candidate deactivating event
        candidate_deactivating_producer.produce({candidate_rl});
        co_await std::suspend_always{};

        // emit candidate deactivated event
        candidate_deactivated_producer.produce({candidate_rl});
        co_await std::suspend_always{};
    }

    // erase the parent goal
    frontier.erase(parent_gl);

    // emit goal_deactivated_event
    goal_deactivated_producer.produce({parent_gl});
    co_await std::suspend_always{};

    // emit resolved event
    resolved_producer.produce({rl});
}

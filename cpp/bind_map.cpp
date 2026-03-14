#include "../hpp/bind_map.hpp"

bind_map::bind_map(trail& trail_ref) : trail_ref(trail_ref) {

}

const expr* bind_map::whnf(const expr* key) {
    // If the key is not a variable, it is already in WHNF
    if (!std::holds_alternative<expr::var>(key->content))
        return key;

    // Get the variable out of the key
    const expr::var& var = std::get<expr::var>(key->content);

    // Check if the variable is bound
    auto it = bindings.find(var.index);
    
    // If the variable is not bound, return the key
    if (it == bindings.end())
        return key;

    // Get the bound value
    const expr* bound_value = it->second;
        
    // WHNF the bound value
    const expr* whnf_bound_value = whnf(bound_value);

    // Collapse the binding
    bind(var.index, whnf_bound_value);

    return whnf_bound_value;
}

bool bind_map::unify(const expr* lhs, const expr* rhs) {
    // WHNF the lhs and rhs
    lhs = whnf(lhs);
    rhs = whnf(rhs);

    // get the lhs and rhs var handles if they are variables
    const expr::var* lv = std::get_if<expr::var>(&lhs->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs->content);

    // if they are the same variable, unification succeeds trivially
    if (lv && rv && lv->index == rv->index)
        return true;
    
    // If the lhs is a variable, add a binding to the whnf of the rhs
    if (lv) {
        if (occurs_check(lv->index, rhs))
            return false;
        bind(lv->index, rhs);
        return true;
    }

    // If the rhs is a variable, add a binding to the whnf of the lhs
    if (rv) {
        if (occurs_check(rv->index, lhs))
            return false;
        bind(rv->index, lhs);
        return true;
    }

    // If they are not the same type, unification fails
    if (lhs->content.index() != rhs->content.index())
        return false;

    // If they are both atoms, unify the values
    if (std::holds_alternative<expr::atom>(lhs->content)) {
        const expr::atom& lAtom = std::get<expr::atom>(lhs->content);
        const expr::atom& rAtom = std::get<expr::atom>(rhs->content);
        return lAtom.value == rAtom.value;
    }

    // If they are both cons cells, unify the children
    if (std::holds_alternative<expr::cons>(lhs->content)) {
        const expr::cons& lCons = std::get<expr::cons>(lhs->content);
        const expr::cons& rCons = std::get<expr::cons>(rhs->content);
        return unify(lCons.lhs, rCons.lhs) && unify(lCons.rhs, rCons.rhs);
    }

    return false;

}

bool bind_map::occurs_check(uint32_t index, const expr* key) {
    key = whnf(key);

    if (const expr::var* var = std::get_if<expr::var>(&key->content))
        return var->index == index;

    if (const expr::cons* cons = std::get_if<expr::cons>(&key->content)) {
        return occurs_check(index, cons->lhs) || occurs_check(index, cons->rhs);
    }

    return false;
}

void bind_map::bind(uint32_t index, const expr* value) {
    // look up the entry for the index
    auto it = bindings.find(index);

    if (it == bindings.end()) {
        // if the value is not found, insert it
        trail_ref.log([this, index]{bindings.erase(index);});
        it = bindings.insert({index, value}).first;
    }
    else {
        // Get the old value
        const expr* old_value = it->second;
        
        // If the new value is the same as the old value, do nothing
        if (old_value == value)
            return;

        // If the new value is different from the old value, insert it
        trail_ref.log([it, old_value]{it->second = old_value;});

        // Update the value
        it->second = value;
    }
    
    return;
}




// #include <queue>
// #include "../hpp/unify.hpp"

// unification_graph::unification_graph(trail& trail_ref) : trail_ref(trail_ref) {

// }

// causal_set unification_graph::unify(const expr* lhs, const expr* rhs, const causal_set& cause) {
//     // 1. Find the CIN of the lhs, and path to it (lPath)
//     auto [lCinFound, lPath] = cin_dijkstra(lhs);

//     // 2. Find the CIN of the rhs, and path to it (rPath)
//     auto [rCinFound, rPath] = cin_dijkstra(rhs);

//     // 3. Merge the lhs and rhs with a new edge
//     edges[lhs].insert(edge{rhs, cause});
//     edges[rhs].insert(edge{lhs, cause});

//     // 4. If either CIN is not found, unification succeeds
//     if (!lCinFound || !rCinFound)
//         return {};

//     // 5. Construct the full causal set for why the CINs were unified
//     causal_set cinUnifyCause = cause + lPath + rPath;

//     // 6. If the CINs heads differ, unification fails.
    
//     // 6.1 Check if the CINs have different types
//     if (lhs->content.index() != rhs->content.index())
//         return cinUnifyCause;

//     // 6.2 Both are atoms
//     if (std::holds_alternative<expr::atom>(lhs->content) && std::holds_alternative<expr::atom>(rhs->content)) {
//         const expr::atom& lAtom = std::get<expr::atom>(lhs->content);
//         const expr::atom& rAtom = std::get<expr::atom>(rhs->content);
//         if (lAtom.value != rAtom.value)
//             return cinUnifyCause;
//         return {};
//     }

//     // 6.3 Both are cons cells
//     const expr::cons& lList = std::get<expr::cons>(lhs->content);
//     const expr::cons& rList = std::get<expr::cons>(rhs->content);
    
//     // 7. Decompose based on the CINs, unifying children
//     causal_set result;

//     // 7.1 Unify the car of the CINs
//     result = unify(lList.lhs, rList.lhs, cinUnifyCause);

//     if (!result.empty())
//         return result;

//     result = unify(lList.rhs, rList.rhs, cinUnifyCause);

//     return result;
    
// }

// std::pair<bool, causal_set> unification_graph::cin_dijkstra(const expr* src) const {
//     // 1. Create the priority queue for Dijkstra's algorithm
//     std::priority_queue<
//         std::pair<causal_set, const expr*>,
//         std::vector<std::pair<causal_set, const expr*>>,
//         std::greater<std::pair<causal_set, const expr*>> // min-heap
//     > pq;

//     // 2. Create the visited set to track nodes we have already visited
//     std::set<const expr*> visited;

//     // 3. Initialize the priority queue with the source
//     pq.push({{}, src});

//     // 4. Create cin node
//     const expr* cinNode = nullptr;
//     causal_set  cinCause;

//     // 5. Run Dijkstra's algorithm
//     while (!pq.empty()) {
//         // Get the node with the smallest distance from the source
//         auto [causalSet, node] = pq.top();
//         pq.pop();

//         // If we have already visited this node, skip it
//         if (visited.count(node))
//             continue;

//         // Mark the node as visited
//         visited.insert(node);
        
//         // If the node is a nonvar, we are done.
//         if (!std::holds_alternative<expr::var>(node->content)) {
//             cinNode = node;
//             cinCause = causalSet;
//             break;
//         }

//         // Add the children of the node to the priority queue
//         for (const auto& e : edges.at(node))
//             pq.push({causalSet + e.cause, e.dst});
//     }

//     // 6. If we did not find a CIN, return false
//     if (cinNode == nullptr)
//         return {false, {}};
    
//     return {true, cinCause};

// }

#ifndef PUD_QUERY_INTRODUCTION_RECORDER_HPP
#define PUD_QUERY_INTRODUCTION_RECORDER_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_introduced_query.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// Forwards each query introduction and withdrawal, and logs it until a reader takes the log. Lets a reader keep
// state per live query, updated as queries come and go instead of rescanning the leaves that hold them.
template<typename IIntroduceQuery, typename IWithdrawIntroduction>
struct pud_query_introduction_recorder {
    pud_query_introduction_recorder(IIntroduceQuery& introduce_query, IWithdrawIntroduction& withdraw_introduction);
    void introduce(pud_mhq_head_id head_id, pud_global_node_id node, pud_body_goal_id goal);
    void withdraw(pud_mhq_head_id head_id);
    std::vector<pud_introduced_query> take_introduced();
    std::vector<pud_mhq_head_id> take_withdrawn();
private:
    IIntroduceQuery& introduce_query_;
    IWithdrawIntroduction& withdraw_introduction_;
    std::vector<pud_introduced_query> introduced_;
    std::vector<pud_mhq_head_id> withdrawn_;
};

#define PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE template<typename IIntroduceQuery, typename IWithdrawIntroduction>
#define PUD_QUERY_INTRODUCTION_RECORDER          pud_query_introduction_recorder<IIntroduceQuery, IWithdrawIntroduction>

PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE
PUD_QUERY_INTRODUCTION_RECORDER::pud_query_introduction_recorder(
    IIntroduceQuery& introduce_query, IWithdrawIntroduction& withdraw_introduction)
    : introduce_query_(introduce_query)
    , withdraw_introduction_(withdraw_introduction)
    , introduced_()
    , withdrawn_() {}

PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE
void PUD_QUERY_INTRODUCTION_RECORDER::introduce(
    pud_mhq_head_id head_id, pud_global_node_id node, pud_body_goal_id goal) {
    introduce_query_.introduce(head_id, node, goal);
    introduced_.push_back(pud_introduced_query{.head = head_id, .node = node, .goal = goal});
}

PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE
void PUD_QUERY_INTRODUCTION_RECORDER::withdraw(pud_mhq_head_id head_id) {
    withdraw_introduction_.withdraw(head_id);
    withdrawn_.push_back(head_id);
}

PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE
std::vector<pud_introduced_query> PUD_QUERY_INTRODUCTION_RECORDER::take_introduced() {
    return std::exchange(introduced_, {});
}

PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE
std::vector<pud_mhq_head_id> PUD_QUERY_INTRODUCTION_RECORDER::take_withdrawn() {
    return std::exchange(withdrawn_, {});
}

#undef PUD_QUERY_INTRODUCTION_RECORDER
#undef PUD_QUERY_INTRODUCTION_RECORDER_TEMPLATE

#endif

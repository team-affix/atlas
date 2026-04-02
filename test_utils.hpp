#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <assert.h>
#include <iostream>
#include <utility>
#include <vector>

#define LOG(x)                                             \
    if(ENABLE_DEBUG_LOGS)                                  \
        std::cout << x;

#define assert_throws(expr, type)                          \
    {                                                      \
        bool l_caught = false;                             \
        try                                                \
        {                                                  \
            (expr);                                        \
        }                                                  \
        catch(type)                                        \
        {                                                  \
            l_caught = true;                               \
        }                                                  \
        assert(l_caught);                                  \
    }

#define TEST(void_fn)                                      \
    LOG(">>>> TEST STARTING: " << #void_fn << std::endl);  \
    void_fn();
// LOG("    <<<< TEST FINISHED: " << #void_fn << std::endl);

template <typename Key, typename Value>
using data_points = std::vector<std::pair<Key, Value>>;

#endif

#include "../hpp/expr.hpp"
#include "../hpp/bind_map.hpp"
#include "../hpp/lineage.hpp"
#include "../hpp/sequencer.hpp"
#include "../hpp/copier.hpp"
#include "../hpp/normalizer.hpp"
#include "../hpp/rule.hpp"
#include "../hpp/defs.hpp"
#include "../hpp/sim.hpp"
#include "../hpp/mcts_decider.hpp"
#include "../hpp/ridge_sim.hpp"
#include "../hpp/horizon_sim.hpp"
#include "../hpp/ridge.hpp"
#include "../hpp/horizon.hpp"
#include "../hpp/expr_printer.hpp"
#include "../hpp/cdcl.hpp"
#include "../hpp/lemma.hpp"
#include "../hpp/weight_store.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include "../../test_utils.hpp"

using resolution_store = resolutions;
using decision_store = decisions;

void test_trail_constructor() {
    // Basic construction - should not crash
    trail t1;
    assert(t1.depth() == 0);
    assert(t1.undo_stack.size() == 0);
    assert(t1.frame_boundary_stack.size() == 0);
    
    // Multiple trails can be constructed
    trail t2;
    trail t3;
    assert(t2.depth() == 0);
    assert(t3.depth() == 0);
    assert(t2.undo_stack.size() == 0);
    assert(t2.frame_boundary_stack.size() == 0);
    assert(t3.undo_stack.size() == 0);
    assert(t3.frame_boundary_stack.size() == 0);
    
    // Trail should be usable immediately after construction
    t1.push();
    assert(t1.depth() == 1);
    assert(t1.frame_boundary_stack.size() == 1);
    assert(t1.frame_boundary_stack.top() == 0);  // Boundary at position 0
    assert(t1.undo_stack.size() == 0);  // No operations logged yet
    t1.pop();
    assert(t1.depth() == 0);
    assert(t1.undo_stack.size() == 0);
    assert(t1.frame_boundary_stack.size() == 0);
}

void test_trail_push_pop() {
    trail t;
    assert(t.depth() == 0);
    assert(t.undo_stack.size() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    // Single push/pop with no logged operations
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    assert(t.undo_stack.size() == 0);
    t.pop();
    assert(t.depth() == 0);
    assert(t.undo_stack.size() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    // Multiple push/pop pairs with no logged operations
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    // Nested push/pop
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.push();
    assert(t.depth() == 2);
    assert(t.frame_boundary_stack.size() == 2);
    assert(t.frame_boundary_stack.top() == 0);  // Second frame also at position 0
    t.pop();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    // Deeper nesting
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.push();
    assert(t.depth() == 2);
    assert(t.frame_boundary_stack.size() == 2);
    assert(t.frame_boundary_stack.top() == 0);
    t.push();
    assert(t.depth() == 3);
    assert(t.frame_boundary_stack.size() == 3);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 2);
    assert(t.frame_boundary_stack.size() == 2);
    t.pop();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    t.pop();
    assert(t.depth() == 0);
    assert(t.frame_boundary_stack.size() == 0);
    
    // Mixed nesting
    t.push();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.push();
    assert(t.depth() == 2);
    assert(t.frame_boundary_stack.size() == 2);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    assert(t.frame_boundary_stack.top() == 0);
    t.push();
    assert(t.depth() == 2);
    assert(t.frame_boundary_stack.size() == 2);
    assert(t.frame_boundary_stack.top() == 0);
    t.pop();
    assert(t.depth() == 1);
    assert(t.frame_boundary_stack.size() == 1);
    t.pop();
    assert(t.depth() == 0);
    assert(t.frame_boundary_stack.size() == 0);
}

void test_trail_log() {
    // Test 1: Single log operation
    {
        trail t;
        int x = 5;
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
        
        t.push();
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        assert(t.undo_stack.size() == 0);
        
        x = 10;
        t.log([&x]() { x = 5; });
        assert(x == 10);
        assert(t.undo_stack.size() == 1);
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.pop();
        assert(x == 5);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 2: Multiple log operations in one frame
    {
        trail t;
        int a = 1, b = 2, c = 3;
        t.push();
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        assert(t.undo_stack.size() == 0);
        
        a = 10;
        t.log([&a]() { a = 1; });
        assert(t.undo_stack.size() == 1);
        
        b = 20;
        t.log([&b]() { b = 2; });
        assert(t.undo_stack.size() == 2);
        
        c = 30;
        t.log([&c]() { c = 3; });
        assert(t.undo_stack.size() == 3);
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);  // Boundary still at 0
        
        assert(a == 10 && b == 20 && c == 30);
        t.pop();
        assert(a == 1 && b == 2 && c == 3);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 3: Nested frames with logs
    {
        trail t;

        int x = 0;
        
        t.push();  // Frame 1
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        x = 1;
        t.log([&x]() { x = 0; });
        assert(t.undo_stack.size() == 1);
        
        t.push();  // Frame 2
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);  // Boundary at position 1
        assert(t.undo_stack.size() == 1);
        
        x = 2;
        t.log([&x]() { x = 1; });
        assert(t.undo_stack.size() == 2);
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);
        
        assert(x == 2);
        t.pop();  // Pop frame 2
        assert(x == 1);
        assert(t.undo_stack.size() == 1);  // One undo from frame 1 remains
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.pop();  // Pop frame 1
        assert(x == 0);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 4: Multiple operations per frame with nesting
    {
        trail t;

        int a = 100, b = 200, c = 300;
        
        t.push();  // Frame 1
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        a = 111;
        t.log([&a]() { a = 100; });
        assert(t.undo_stack.size() == 1);
        
        b = 222;
        t.log([&b]() { b = 200; });
        assert(t.undo_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.push();  // Frame 2
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 2);  // Boundary at position 2
        assert(t.undo_stack.size() == 2);
        
        b = 333;
        t.log([&b]() { b = 222; });
        assert(t.undo_stack.size() == 3);
        
        c = 444;
        t.log([&c]() { c = 300; });
        assert(t.undo_stack.size() == 4);
        assert(t.frame_boundary_stack.top() == 2);
        
        t.push();  // Frame 3
        assert(t.frame_boundary_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 4);  // Boundary at position 4
        assert(t.undo_stack.size() == 4);
        
        a = 555;
        t.log([&a]() { a = 111; });
        assert(t.undo_stack.size() == 5);
        assert(t.frame_boundary_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 4);
        
        assert(a == 555 && b == 333 && c == 444);
        
        t.pop();  // Pop frame 3
        assert(a == 111 && b == 333 && c == 444);
        assert(t.undo_stack.size() == 4);  // Frame 3's undo removed
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 2);
        
        t.pop();  // Pop frame 2
        assert(a == 111 && b == 222 && c == 300);
        assert(t.undo_stack.size() == 2);  // Frame 2's undos removed
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.pop();  // Pop frame 1
        assert(a == 100 && b == 200 && c == 300);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 5: Empty frame (push/pop with no logs)
    {
        trail t;

        int x = 42;
        t.push();
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        assert(t.undo_stack.size() == 0);
        
        // No logs
        x = 99;
        assert(x == 99);
        assert(t.undo_stack.size() == 0);  // Still no undos
        
        t.pop();
        assert(x == 99);  // Should remain unchanged since no undo was logged
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 6: Complex nested scenario with partial pops
    {
        trail t;

        int val = 0;
        
        t.push();  // Frame A
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        val = 1;
        t.log([&val]() { val = 0; });
        assert(t.undo_stack.size() == 1);
        
        t.push();  // Frame B
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);  // Boundary at position 1
        
        val = 2;
        t.log([&val]() { val = 1; });
        assert(t.undo_stack.size() == 2);
        
        t.push();  // Frame C
        assert(t.frame_boundary_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 2);  // Boundary at position 2
        
        val = 3;
        t.log([&val]() { val = 2; });
        assert(t.undo_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 2);
        
        assert(val == 3);
        t.pop();  // Pop C
        assert(val == 2);
        assert(t.undo_stack.size() == 2);  // Frame C's undo removed
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);
        
        // Add more to frame B
        val = 4;
        t.log([&val]() { val = 2; });
        assert(t.undo_stack.size() == 3);  // New undo added to frame B
        assert(t.frame_boundary_stack.top() == 1);  // Boundary unchanged
        
        assert(val == 4);
        t.pop();  // Pop B (should undo both operations in B)
        assert(val == 1);
        assert(t.undo_stack.size() == 1);  // Frame B's undos removed, frame A's remains
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.pop();  // Pop A
        assert(val == 0);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 7: Multiple variables with complex state changes
    {
        trail t;

        int x = 10, y = 20, z = 30;
        
        t.push();  // Level 1
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        x += 5;
        t.log([&x]() { x -= 5; });
        y *= 2;
        t.log([&y]() { y /= 2; });
        assert(t.undo_stack.size() == 2);
        
        assert(x == 15 && y == 40 && z == 30);
        
        t.push();  // Level 2
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 2);
        
        z = x + y;  // z = 55
        t.log([&z]() { z = 30; });
        x = 0;
        t.log([&x]() { x = 15; });
        assert(t.undo_stack.size() == 4);
        
        assert(x == 0 && y == 40 && z == 55);
        
        t.push();  // Level 3
        assert(t.frame_boundary_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 4);
        
        y = 100;
        t.log([&y]() { y = 40; });
        assert(t.undo_stack.size() == 5);
        
        assert(x == 0 && y == 100 && z == 55);
        
        t.pop();  // Pop level 3
        assert(x == 0 && y == 40 && z == 55);
        assert(t.undo_stack.size() == 4);
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 2);
        
        t.pop();  // Pop level 2
        assert(x == 15 && y == 40 && z == 30);
        assert(t.undo_stack.size() == 2);
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.pop();  // Pop level 1
        assert(x == 10 && y == 20 && z == 30);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 8: Deeply nested frames (5 levels)
    {
        trail t;

        int depth = 0;
        
        t.push();
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        depth = 1;
        t.log([&depth]() { depth = 0; });
        assert(t.undo_stack.size() == 1);
        
        t.push();
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);
        depth = 2;
        t.log([&depth]() { depth = 1; });
        assert(t.undo_stack.size() == 2);
        
        t.push();
        assert(t.frame_boundary_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 2);
        depth = 3;
        t.log([&depth]() { depth = 2; });
        assert(t.undo_stack.size() == 3);
        
        t.push();
        assert(t.frame_boundary_stack.size() == 4);
        assert(t.frame_boundary_stack.top() == 3);
        depth = 4;
        t.log([&depth]() { depth = 3; });
        assert(t.undo_stack.size() == 4);
        
        t.push();
        assert(t.frame_boundary_stack.size() == 5);
        assert(t.frame_boundary_stack.top() == 4);
        depth = 5;
        t.log([&depth]() { depth = 4; });
        assert(t.undo_stack.size() == 5);
        
        assert(depth == 5);
        t.pop();
        assert(depth == 4);
        assert(t.undo_stack.size() == 4);
        assert(t.frame_boundary_stack.size() == 4);
        t.pop();
        assert(depth == 3);
        assert(t.undo_stack.size() == 3);
        assert(t.frame_boundary_stack.size() == 3);
        t.pop();
        assert(depth == 2);
        assert(t.undo_stack.size() == 2);
        assert(t.frame_boundary_stack.size() == 2);
        t.pop();
        assert(depth == 1);
        assert(t.undo_stack.size() == 1);
        assert(t.frame_boundary_stack.size() == 1);
        t.pop();
        assert(depth == 0);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 9: Many operations in a single frame
    {
        trail t;

        std::vector<int> values(10, 0);
        
        t.push();
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        for (int i = 0; i < 10; i++) {
            values[i] = i + 1;
            t.log([&values, i]() { values[i] = 0; });
        }
        assert(t.undo_stack.size() == 10);  // All 10 operations logged
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);  // Boundary unchanged
        
        for (int i = 0; i < 10; i++) {
            assert(values[i] == i + 1);
        }
        
        t.pop();
        assert(t.undo_stack.size() == 0);  // All undos executed
        assert(t.frame_boundary_stack.size() == 0);
        
        for (int i = 0; i < 10; i++) {
            assert(values[i] == 0);
        }
    }
    
    // Test 10: Interleaved push/pop/log operations
    {
        trail t;

        int state = 0;
        
        t.push();  // Frame 1
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        state = 1;
        t.log([&state]() { state = 0; });
        assert(t.undo_stack.size() == 1);
        
        t.push();  // Frame 2
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);
        state = 2;
        t.log([&state]() { state = 1; });
        assert(t.undo_stack.size() == 2);
        
        t.pop();  // Pop frame 2
        assert(state == 1);
        assert(t.undo_stack.size() == 1);
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        
        t.push();  // New frame 2
        assert(t.frame_boundary_stack.size() == 2);
        assert(t.frame_boundary_stack.top() == 1);
        state = 3;
        t.log([&state]() { state = 1; });
        assert(t.undo_stack.size() == 2);
        
        t.push();  // Frame 3
        assert(t.frame_boundary_stack.size() == 3);
        assert(t.frame_boundary_stack.top() == 2);
        state = 4;
        t.log([&state]() { state = 3; });
        assert(t.undo_stack.size() == 3);
        
        assert(state == 4);
        t.pop();  // Pop frame 3
        assert(state == 3);
        assert(t.undo_stack.size() == 2);
        assert(t.frame_boundary_stack.size() == 2);
        t.pop();  // Pop frame 2
        assert(state == 1);
        assert(t.undo_stack.size() == 1);
        assert(t.frame_boundary_stack.size() == 1);
        t.pop();  // Pop frame 1
        assert(state == 0);
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 11: String modifications
    {
        trail t;
        
        std::string str = "original";
        
        t.push();
        assert(t.frame_boundary_stack.size() == 1);
        assert(t.frame_boundary_stack.top() == 0);
        str = "modified";
        t.log([&str]() { str = "original"; });
        assert(t.undo_stack.size() == 1);
        
        assert(str == "modified");
        t.pop();
        assert(str == "original");
        assert(t.undo_stack.size() == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 12: Multiple independent trails
    {
        trail t1, t2;
        int x = 1, y = 2;
        
        t1.push();
        assert(t1.frame_boundary_stack.size() == 1);
        assert(t1.frame_boundary_stack.top() == 0);
        assert(t2.frame_boundary_stack.size() == 0);  // t2 unaffected
        x = 10;
        t1.log([&x]() { x = 1; });
        assert(t1.undo_stack.size() == 1);
        
        t2.push();
        assert(t2.frame_boundary_stack.size() == 1);
        assert(t2.frame_boundary_stack.top() == 0);
        assert(t1.frame_boundary_stack.size() == 1);  // t1 unaffected
        y = 20;
        t2.log([&y]() { y = 2; });
        assert(t2.undo_stack.size() == 1);
        
        assert(x == 10 && y == 20);
        
        t1.pop();
        assert(x == 1 && y == 20);
        assert(t1.undo_stack.size() == 0);
        assert(t1.frame_boundary_stack.size() == 0);
        assert(t2.undo_stack.size() == 1);  // t2 unaffected
        
        t2.pop();
        assert(x == 1 && y == 2);
        assert(t2.undo_stack.size() == 0);
        assert(t2.frame_boundary_stack.size() == 0);
    }
    
    // Test 13: Complex multiple independent trails with nested frames
    {
        trail t1, t2, t3;
        
        // Each trail manages its own independent data
        int data1 = 100;
        int data2 = 200;
        int data3 = 300;
        
        assert(t1.depth() == 0);
        assert(t2.depth() == 0);
        assert(t3.depth() == 0);
        
        // === Trail 1: Nested frames with data1 ===
        t1.push();  // Frame 1.1
        assert(t1.depth() == 1);
        assert(t1.frame_boundary_stack.size() == 1);
        assert(t1.frame_boundary_stack.top() == 0);
        data1 += 10;  // 110
        t1.log([&data1]() { data1 -= 10; });
        assert(t1.undo_stack.size() == 1);
        
        t1.push();  // Frame 1.2
        assert(t1.depth() == 2);
        assert(t1.frame_boundary_stack.size() == 2);
        assert(t1.frame_boundary_stack.top() == 1);
        data1 *= 2;  // 220
        t1.log([&data1]() { data1 /= 2; });
        assert(t1.undo_stack.size() == 2);
        
        t1.push();  // Frame 1.3
        assert(t1.depth() == 3);
        assert(t1.frame_boundary_stack.size() == 3);
        assert(t1.frame_boundary_stack.top() == 2);
        data1 += 80;  // 300
        t1.log([&data1]() { data1 -= 80; });
        assert(t1.undo_stack.size() == 3);
        
        assert(data1 == 300);
        
        // === Trail 2: Nested frames with data2 ===
        t2.push();  // Frame 2.1
        assert(t2.depth() == 1);
        assert(t2.frame_boundary_stack.size() == 1);
        assert(t2.frame_boundary_stack.top() == 0);
        data2 -= 50;  // 150
        t2.log([&data2]() { data2 += 50; });
        assert(t2.undo_stack.size() == 1);
        
        t2.push();  // Frame 2.2
        assert(t2.depth() == 2);
        assert(t2.frame_boundary_stack.size() == 2);
        assert(t2.frame_boundary_stack.top() == 1);
        data2 *= 3;  // 450
        t2.log([&data2]() { data2 /= 3; });
        assert(t2.undo_stack.size() == 2);
        
        assert(data2 == 450);
        
        // === Trail 3: Single frame with multiple operations on data3 ===
        t3.push();  // Frame 3.1
        assert(t3.depth() == 1);
        assert(t3.frame_boundary_stack.size() == 1);
        assert(t3.frame_boundary_stack.top() == 0);
        data3 /= 3;  // 100
        t3.log([&data3]() { data3 *= 3; });
        assert(t3.undo_stack.size() == 1);
        data3 += 50;  // 150
        t3.log([&data3]() { data3 -= 50; });
        assert(t3.undo_stack.size() == 2);
        data3 *= 4;  // 600
        t3.log([&data3]() { data3 /= 4; });
        assert(t3.undo_stack.size() == 3);
        
        assert(data3 == 600);
        
        // Verify all data is at expected state
        assert(data1 == 300 && data2 == 450 && data3 == 600);
        
        // === Pop trail 1 innermost frame ===
        t1.pop();  // Pop frame 1.3
        assert(t1.depth() == 2);
        assert(t1.undo_stack.size() == 2);
        assert(t1.frame_boundary_stack.size() == 2);
        assert(data1 == 220);  // Restored from frame 1.3
        assert(data2 == 450);  // Unchanged
        assert(data3 == 600);  // Unchanged
        
        // === Add more to trail 2 ===
        t2.push();  // Frame 2.3
        assert(t2.depth() == 3);
        assert(t2.frame_boundary_stack.size() == 3);
        assert(t2.frame_boundary_stack.top() == 2);
        data2 += 50;  // 500
        t2.log([&data2]() { data2 -= 50; });
        assert(t2.undo_stack.size() == 3);
        
        assert(data1 == 220 && data2 == 500 && data3 == 600);
        
        // === Pop trail 3 completely ===
        t3.pop();  // Pop frame 3.1
        assert(t3.depth() == 0);
        assert(t3.undo_stack.size() == 0);
        assert(t3.frame_boundary_stack.size() == 0);
        assert(data1 == 220);  // Unchanged
        assert(data2 == 500);  // Unchanged
        assert(data3 == 300);  // Restored to original
        
        // === Add new frame to trail 3 ===
        t3.push();  // New frame 3.1
        assert(t3.depth() == 1);
        assert(t3.frame_boundary_stack.size() == 1);
        assert(t3.frame_boundary_stack.top() == 0);
        data3 -= 100;  // 200
        t3.log([&data3]() { data3 += 100; });
        assert(t3.undo_stack.size() == 1);
        data3 *= 5;  // 1000
        t3.log([&data3]() { data3 /= 5; });
        assert(t3.undo_stack.size() == 2);
        
        assert(data1 == 220 && data2 == 500 && data3 == 1000);
        
        // === Pop trail 2 innermost frame ===
        t2.pop();  // Pop frame 2.3
        assert(t2.depth() == 2);
        assert(t2.undo_stack.size() == 2);
        assert(t2.frame_boundary_stack.size() == 2);
        assert(data1 == 220);  // Unchanged
        assert(data2 == 450);  // Restored from frame 2.3
        assert(data3 == 1000);  // Unchanged
        
        // === Pop trail 1 middle frame ===
        t1.pop();  // Pop frame 1.2
        assert(t1.depth() == 1);
        assert(data1 == 110);  // Restored from frame 1.2
        assert(data2 == 450);  // Unchanged
        assert(data3 == 1000);  // Unchanged
        
        // === Pop trail 2 all remaining frames ===
        t2.pop();  // Pop frame 2.2
        assert(t2.depth() == 1);
        assert(t2.undo_stack.size() == 1);
        assert(t2.frame_boundary_stack.size() == 1);
        assert(data2 == 150);  // Restored from frame 2.2
        
        t2.pop();  // Pop frame 2.1
        assert(t2.depth() == 0);
        assert(t2.undo_stack.size() == 0);
        assert(t2.frame_boundary_stack.size() == 0);
        assert(data2 == 200);  // Restored to original
        
        assert(data1 == 110 && data2 == 200 && data3 == 1000);
        
        // === Pop trail 3 ===
        t3.pop();  // Pop frame 3.1
        assert(t3.depth() == 0);
        assert(t3.undo_stack.size() == 0);
        assert(t3.frame_boundary_stack.size() == 0);
        assert(data3 == 300);  // Restored to original
        
        assert(data1 == 110 && data2 == 200 && data3 == 300);
        
        // === Pop trail 1 last frame ===
        t1.pop();  // Pop frame 1.1
        assert(t1.depth() == 0);
        assert(t1.undo_stack.size() == 0);
        assert(t1.frame_boundary_stack.size() == 0);
        assert(data1 == 100);  // Restored to original
        
        // All data restored to original values
        assert(data1 == 100 && data2 == 200 && data3 == 300);
        assert(t1.depth() == 0 && t2.depth() == 0 && t3.depth() == 0);
    }

    // Test 14: COMPREHENSIVE SEQUENCE REVERSAL TEST WITH CHECKPOINTS
    {
        trail t;
        
        int val = 100;  // Starting value
        
        // === FRAME 1 ===
        t.push();
        assert(t.depth() == 1);
        
        // Step 1: Add 5 -> 105
        val += 5;
        t.log([&val]() { val -= 5; });
        assert(val == 105);
        
        // Step 2: Multiply by 2 -> 210
        val *= 2;
        t.log([&val]() { val /= 2; });
        assert(val == 210);
        
        // Step 3: Subtract 10 -> 200
        val -= 10;
        t.log([&val]() { val += 10; });
        assert(val == 200);
        
        // Step 4: Add 50 -> 250
        val += 50;
        t.log([&val]() { val -= 50; });
        assert(val == 250);
        
        // CHECKPOINT 1: val should be 250
        int checkpoint1 = val;
        assert(checkpoint1 == 250);
        
        // === FRAME 2 ===
        t.push();
        assert(t.depth() == 2);
        
        // Step 5: Divide by 5 -> 50
        val /= 5;
        t.log([&val]() { val *= 5; });
        assert(val == 50);
        
        // Step 6: Add 150 -> 200
        val += 150;
        t.log([&val]() { val -= 150; });
        assert(val == 200);
        
        // Step 7: Multiply by 3 -> 600
        val *= 3;
        t.log([&val]() { val /= 3; });
        assert(val == 600);
        
        // Step 8: Subtract 100 -> 500
        val -= 100;
        t.log([&val]() { val += 100; });
        assert(val == 500);
        
        // Step 9: Add 25 -> 525
        val += 25;
        t.log([&val]() { val -= 25; });
        assert(val == 525);
        
        // CHECKPOINT 2: val should be 525
        int checkpoint2 = val;
        assert(checkpoint2 == 525);
        
        // === FRAME 3 ===
        t.push();
        assert(t.depth() == 3);
        
        // Step 10: Subtract 25 -> 500
        val -= 25;
        t.log([&val]() { val += 25; });
        assert(val == 500);
        
        // Step 11: Divide by 4 -> 125
        val /= 4;
        t.log([&val]() { val *= 4; });
        assert(val == 125);
        
        // Step 12: Add 75 -> 200
        val += 75;
        t.log([&val]() { val -= 75; });
        assert(val == 200);
        
        // Step 13: Multiply by 2 -> 400
        val *= 2;
        t.log([&val]() { val /= 2; });
        assert(val == 400);
        
        // Step 14: Subtract 50 -> 350
        val -= 50;
        t.log([&val]() { val += 50; });
        assert(val == 350);
        
        // Step 15: Add 150 -> 500
        val += 150;
        t.log([&val]() { val -= 150; });
        assert(val == 500);
        
        // CHECKPOINT 3: val should be 500
        int checkpoint3 = val;
        assert(checkpoint3 == 500);
        
        // === FRAME 4 ===
        t.push();
        assert(t.depth() == 4);
        
        // Step 16: Divide by 10 -> 50
        val /= 10;
        t.log([&val]() { val *= 10; });
        assert(val == 50);
        
        // Step 17: Add 450 -> 500
        val += 450;
        t.log([&val]() { val -= 450; });
        assert(val == 500);
        
        // Step 18: Multiply by 2 -> 1000
        val *= 2;
        t.log([&val]() { val /= 2; });
        assert(val == 1000);
        
        // Step 19: Subtract 200 -> 800
        val -= 200;
        t.log([&val]() { val += 200; });
        assert(val == 800);
        
        // Step 20: Add 100 -> 900
        val += 100;
        t.log([&val]() { val -= 100; });
        assert(val == 900);
        
        // CHECKPOINT 4: val should be 900
        int checkpoint4 = val;
        assert(checkpoint4 == 900);
        
        // === NOW UNDO IN REVERSE ORDER ===
        
        // Pop frame 4 - should restore to checkpoint 3 (500)
        t.pop();
        assert(t.depth() == 3);
        assert(val == checkpoint3);
        assert(val == 500);
        
        // Pop frame 3 - should restore to checkpoint 2 (525)
        t.pop();
        assert(t.depth() == 2);
        assert(val == checkpoint2);
        assert(val == 525);
        
        // Pop frame 2 - should restore to checkpoint 1 (250)
        t.pop();
        assert(t.depth() == 1);
        assert(val == checkpoint1);
        assert(val == 250);
        
        // Pop frame 1 - should restore to original (100)
        t.pop();
        assert(t.depth() == 0);
        assert(val == 100);
    }
}

void test_functor_constructor() {
    // Basic string
    expr::functor a1{"hello"};
    assert(a1.name == "hello");
    
    // Empty string
    expr::functor a2{""};
    assert(a2.name == "");
    
    // Single character
    expr::functor a3{"x"};
    assert(a3.name == "x");
    
    // Numeric strings
    expr::functor a4{"0"};
    assert(a4.name == "0");
    expr::functor a5{"12345"};
    assert(a5.name == "12345");
    expr::functor a6{"-42"};
    assert(a6.name == "-42");
    
    // Special characters
    expr::functor a7{"test_123"};
    assert(a7.name == "test_123");
    expr::functor a8{"hello world"};
    assert(a8.name == "hello world");
    expr::functor a9{"!@#$%^&*()"};
    assert(a9.name == "!@#$%^&*()");
    
    // Whitespace variations
    expr::functor a10{" "};
    assert(a10.name == " ");
    expr::functor a11{"\t"};
    assert(a11.name == "\t");
    expr::functor a12{"\n"};
    assert(a12.name == "\n");
    
    // Long string
    std::string long_str(1000, 'a');
    expr::functor a13{long_str};
    assert(a13.name == long_str);
    
    // Unicode/special characters
    expr::functor a14{"αβγδ"};
    assert(a14.name == "αβγδ");
    expr::functor a15{"日本語"};
    assert(a15.name == "日本語");
    
    // Escape sequences
    expr::functor a16{"line1\nline2"};
    assert(a16.name == "line1\nline2");
    
    // Multiple atoms with same value
    expr::functor a17{"duplicate"};
    expr::functor a18{"duplicate"};
    assert(a17.name == a18.name);
    assert((a17 <=> a18) == 0);
    
    // Test spaceship operator with different values
    expr::functor a19{"aaa"};
    expr::functor a20{"bbb"};
    assert((a19 <=> a20) < 0);
    assert((a20 <=> a19) > 0);
}

void test_var_constructor() {
    // Zero index
    expr::var v1{0};
    assert(v1.index == 0);
    
    // Small indices
    expr::var v2{1};
    assert(v2.index == 1);
    expr::var v3{10};
    assert(v3.index == 10);
    
    // Medium indices
    expr::var v4{100};
    assert(v4.index == 100);
    expr::var v5{999};
    assert(v5.index == 999);
    
    // Large indices
    expr::var v6{1000000};
    assert(v6.index == 1000000);
    expr::var v7{2147483647};
    assert(v7.index == 2147483647);
    
    // Maximum value
    expr::var v8{UINT32_MAX};
    assert(v8.index == UINT32_MAX);
    
    // Multiple vars with same index
    expr::var v9{42};
    expr::var v10{42};
    assert(v9.index == v10.index);
    assert((v9 <=> v10) == 0);
    
    // Test spaceship operator with different indices
    expr::var v11{10};
    expr::var v12{20};
    assert((v11 <=> v12) < 0);
    assert((v12 <=> v11) > 0);
    
    // Sequential indices
    for (uint32_t i = 0; i < 100; i++) {
        expr::var v{i};
        assert(v.index == i);
    }
}

void test_functor_cons_constructor() {
    // Basic cons with raw pointers (testing the struct itself)
    expr e1{expr::functor{"left", {}}};
    expr e2{expr::functor{"right", {}}};
    expr::functor c1{"cons", {&e1, &e2}};
    
    assert(c1.args[0] == &e1);
    assert(c1.args[1] == &e2);
    assert(std::get<expr::functor>(c1.args[0]->content).name == "left");
    assert(std::get<expr::functor>(c1.args[1]->content).name == "right");
    
    // Cons with variables
    expr e3{expr::var{0}};
    expr e4{expr::var{1}};
    expr::functor c2{"cons", {&e3, &e4}};
    
    assert(std::get<expr::var>(c2.args[0]->content).index == 0);
    assert(std::get<expr::var>(c2.args[1]->content).index == 1);
    
    // Cons with mixed types
    expr e5{expr::functor{"atom", {}}};
    expr e6{expr::var{42}};
    expr::functor c3{"cons", {&e5, &e6}};
    
    assert(std::get<expr::functor>(c3.args[0]->content).name == "atom");
    assert(std::get<expr::var>(c3.args[1]->content).index == 42);
    
    // Cons with same expr on both sides
    expr e7{expr::functor{"same", {}}};
    expr::functor c4{"cons", {&e7, &e7}};
    
    assert(c4.args[0] == c4.args[1]);
    assert(c4.args[0] == &e7);
    
    // Test spaceship operator
    expr e8{expr::functor{"a", {}}};
    expr e9{expr::functor{"b", {}}};
    expr::functor c5{"cons", {&e8, &e9}};
    expr::functor c6{"cons", {&e8, &e9}};
    
    assert((c5 <=> c6) == 0);
    
    // Different cons
    expr e10{expr::functor{"c", {}}};
    expr::functor c7{"cons", {&e8, &e10}};
    
    assert((c5 <=> c7) != 0);
}

void test_expr_constructor() {
    // Expr with atom
    expr e1{expr::functor{"test", {}}};
    assert(std::holds_alternative<expr::functor>(e1.content));
    assert(std::get<expr::functor>(e1.content).name == "test");
    
    // Expr with empty atom
    expr e2{expr::functor{"", {}}};
    assert(std::holds_alternative<expr::functor>(e2.content));
    assert(std::get<expr::functor>(e2.content).name == "");
    
    // Expr with var
    expr e3{expr::var{42}};
    assert(std::holds_alternative<expr::var>(e3.content));
    assert(std::get<expr::var>(e3.content).index == 42);
    
    // Expr with var - edge cases
    expr e4{expr::var{0}};
    assert(std::get<expr::var>(e4.content).index == 0);
    
    expr e5{expr::var{UINT32_MAX}};
    assert(std::get<expr::var>(e5.content).index == UINT32_MAX);
    
    // Expr with cons
    expr left{expr::functor{"left", {}}};
    expr right{expr::functor{"right", {}}};
    expr e6{expr::functor{"cons", {&left, &right}}};
    
    assert(std::holds_alternative<expr::functor>(e6.content));
    const expr::functor& c1 = std::get<expr::functor>(e6.content);
    assert(std::get<expr::functor>(c1.args[0]->content).name == "left");
    assert(std::get<expr::functor>(c1.args[1]->content).name == "right");
    
    // Test spaceship operator
    expr e7{expr::functor{"aaa", {}}};
    expr e8{expr::functor{"aaa", {}}};
    assert((e7 <=> e8) == 0);
    
    expr e9{expr::functor{"bbb", {}}};
    assert((e7 <=> e9) < 0);
    assert((e9 <=> e7) > 0);
    
    // Different variant types
    expr e10{expr::var{0}};
    // Comparison between different variant types is well-defined by spaceship
    assert((e7 <=> e10) != 0);
}

void test_expr_pool_functor_constructor() {
    trail t;
    
    // Basic construction with trail reference
    expr_pool pool1(t);
    assert(pool1.size() == 0);
    assert(pool1.exprs.size() == 0);
    assert(pool1.exprs.empty());
    
    // Multiple pools can be constructed with same trail
    expr_pool pool2(t);
    assert(pool2.size() == 0);
    assert(pool2.exprs.size() == 0);
    assert(pool2.exprs.empty());
    
    // Multiple pools with different trails
    trail t2;
    expr_pool pool3(t2);
    assert(pool3.size() == 0);
    assert(pool3.exprs.size() == 0);
    assert(pool3.exprs.empty());
    
    // Pool should be usable immediately after construction
    t.push();
    const expr* e1 = pool1.functor("test", {});
    assert(e1 != nullptr);
    assert(pool1.size() == 1);
    assert(pool1.exprs.size() == 1);
    assert(pool1.exprs.count(*e1) == 1);
    
    // Other pools are independent
    assert(pool2.size() == 0);
    assert(pool2.exprs.size() == 0);
    assert(pool3.size() == 0);
    assert(pool3.exprs.size() == 0);
    
    // Add to pool2 with same trail
    const expr* e2 = pool2.functor("test2", {});
    assert(pool2.size() == 1);
    assert(pool2.exprs.size() == 1);
    assert(pool2.exprs.count(*e2) == 1);
    assert(pool1.size() == 1);  // pool1 unchanged
    assert(pool1.exprs.size() == 1);
    
    // Pop should affect both pool1 and pool2 since they share trail t
    t.pop();
    assert(pool1.size() == 0);
    assert(pool1.exprs.size() == 0);
    assert(pool2.size() == 0);
    assert(pool2.exprs.size() == 0);
    
    // pool3 with different trail is unaffected
    assert(pool3.size() == 0);
    assert(pool3.exprs.size() == 0);
}

void test_expr_pool_functor() {
    trail t;
    expr_pool pool(t);
    
    // Push initial frame before any operations
    t.push();
    
    // Basic atom creation
    const expr* e1 = pool.functor("test", {});
    assert(e1 != nullptr);
    assert(std::holds_alternative<expr::functor>(e1->content));
    assert(std::get<expr::functor>(e1->content).name == "test");
    assert(pool.size() == 1);
    assert(pool.exprs.size() == 1);
    assert(pool.exprs.count(*e1) == 1);
    
    // Empty string
    const expr* e2 = pool.functor("", {});
    assert(std::get<expr::functor>(e2->content).name == "");
    assert(pool.size() == 2);
    assert(pool.exprs.size() == 2);
    assert(pool.exprs.count(*e2) == 1);
    assert(e1 != e2);
    
    // Interning - same string should return same pointer
    const expr* e3 = pool.functor("test", {});
    assert(e1 == e3);
    assert(pool.size() == 2);  // No new entry added
    assert(pool.exprs.size() == 2);
    
    // Different strings should return different pointers
    const expr* e4 = pool.functor("different", {});
    assert(e1 != e4);
    assert(pool.size() == 3);
    assert(pool.exprs.size() == 3);
    assert(pool.exprs.count(*e4) == 1);
    
    // Multiple calls with same string
    const expr* e5 = pool.functor("shared", {});
    assert(pool.exprs.size() == 4);
    const expr* e6 = pool.functor("shared", {});
    const expr* e7 = pool.functor("shared", {});
    assert(e5 == e6);
    assert(e6 == e7);
    assert(pool.size() == 4);  // Only one "shared" added
    assert(pool.exprs.size() == 4);
    assert(pool.exprs.count(*e5) == 1);
    
    // Special characters
    const expr* e8 = pool.functor("!@#$", {});
    assert(pool.exprs.size() == 5);
    const expr* e9 = pool.functor("!@#$", {});
    assert(e8 == e9);
    assert(pool.size() == 5);
    assert(pool.exprs.size() == 5);
    assert(pool.exprs.count(*e8) == 1);
    
    // Long strings
    std::string long_str(1000, 'x');
    const expr* e10 = pool.functor(long_str, {});
    assert(pool.exprs.size() == 6);
    const expr* e11 = pool.functor(long_str, {});
    assert(e10 == e11);
    assert(pool.size() == 6);
    assert(pool.exprs.size() == 6);
    assert(pool.exprs.count(*e10) == 1);
    
    // Test backtracking: push frame, add content, pop frame
    size_t size_before = pool.size();
    assert(pool.exprs.size() == size_before);
    t.push();
    const expr* temp1 = pool.functor("temporary1", {});
    const expr* temp2 = pool.functor("temporary2", {});
    assert(pool.size() == size_before + 2);
    assert(pool.exprs.size() == size_before + 2);
    assert(pool.exprs.count(*temp1) == 1);
    assert(pool.exprs.count(*temp2) == 1);
    t.pop();
    assert(pool.size() == size_before);  // Should be back to original size
    assert(pool.exprs.size() == size_before);
    
    // Test corner case: intern same content in nested frames
    t.push();  // Frame 1
    const expr* content_c = pool.functor("content_c", {});
    size_t checkpoint1 = pool.size();
    assert(content_c != nullptr);
    assert(pool.exprs.size() == checkpoint1);
    assert(pool.exprs.count(*content_c) == 1);
    
    t.push();  // Frame 2
    const expr* content_c_again = pool.functor("content_c", {});  // Should return same pointer, no log
    assert(content_c == content_c_again);
    assert(pool.size() == checkpoint1);  // Size unchanged
    assert(pool.exprs.size() == checkpoint1);
    
    t.pop();  // Pop frame 2
    assert(pool.size() == checkpoint1);  // Size still unchanged
    assert(pool.exprs.size() == checkpoint1);
    
    // Verify content_c is still there
    const expr* content_c_verify = pool.functor("content_c", {});
    assert(content_c == content_c_verify);
    assert(pool.size() == checkpoint1);
    assert(pool.exprs.size() == checkpoint1);
    assert(pool.exprs.count(*content_c) == 1);
    
    t.pop();  // Pop frame 1
    // Now content_c should be removed

    assert(pool.size() == size_before);
    assert(pool.exprs.size() == size_before);
    
    // Test nested pushes with checkpoints
    size_t checkpoint_start = pool.size();
    assert(pool.exprs.size() == checkpoint_start);
    
    t.push();  // Level 1
    pool.functor("level1_a", {});
    pool.functor("level1_b", {});
    size_t checkpoint_level1 = pool.size();
    assert(checkpoint_level1 == checkpoint_start + 2);
    assert(pool.exprs.size() == checkpoint_level1);
    
    t.push();  // Level 2
    pool.functor("level2_a", {});
    pool.functor("level2_b", {});
    pool.functor("level2_c", {});
    size_t checkpoint_level2 = pool.size();
    assert(checkpoint_level2 == checkpoint_level1 + 3);
    assert(pool.exprs.size() == checkpoint_level2);
    
    t.push();  // Level 3
    pool.functor("level3_a", {});
    size_t checkpoint_level3 = pool.size();
    assert(checkpoint_level3 == checkpoint_level2 + 1);
    assert(pool.exprs.size() == checkpoint_level3);
    
    // Pop level 3
    t.pop();
    assert(pool.size() == checkpoint_level2);
    assert(pool.exprs.size() == checkpoint_level2);
    
    // Pop level 2
    t.pop();
    assert(pool.size() == checkpoint_level1);
    assert(pool.exprs.size() == checkpoint_level1);
    
    // Pop level 1
    t.pop();
    assert(pool.size() == checkpoint_start);
    assert(pool.exprs.size() == checkpoint_start);
    
    // Test corner case: content added in earlier frame should not be removed by later frame pop
    t.push();  // Frame A
    assert(pool.exprs.size() == checkpoint_start);
    
    const expr* early_content_1 = pool.functor("early_1", {});
    assert(pool.exprs.size() == checkpoint_start + 1);
    assert(pool.exprs.count(*early_content_1) == 1);
    
    const expr* early_content_2 = pool.functor("early_2", {});
    assert(pool.exprs.size() == checkpoint_start + 2);
    assert(pool.exprs.count(*early_content_2) == 1);
    
    size_t checkpoint_a = pool.size();
    assert(checkpoint_a == checkpoint_start + 2);
    assert(pool.exprs.size() == checkpoint_a);
    
    t.push();  // Frame B
    assert(pool.exprs.size() == checkpoint_a);
    
    const expr* mid_content = pool.functor("mid_content", {});
    assert(pool.exprs.size() == checkpoint_a + 1);
    assert(pool.exprs.count(*mid_content) == 1);
    
    size_t checkpoint_b = pool.size();
    assert(checkpoint_b == checkpoint_a + 1);
    assert(pool.exprs.size() == checkpoint_b);
    
    // Re-intern early content in Frame B - should not log since already exists
    const expr* early_content_1_again = pool.functor("early_1", {});
    assert(early_content_1 == early_content_1_again);
    assert(pool.size() == checkpoint_b);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b);  // Set size also unchanged
    assert(pool.exprs.count(*early_content_1) == 1);  // Still in set
    
    // Add more new content in Frame B
    const expr* late_content_1 = pool.functor("late_1", {});
    assert(pool.exprs.size() == checkpoint_b + 1);
    assert(pool.exprs.count(*late_content_1) == 1);
    
    const expr* late_content_2 = pool.functor("late_2", {});
    assert(pool.exprs.size() == checkpoint_b + 2);
    assert(pool.exprs.count(*late_content_2) == 1);
    
    size_t checkpoint_b_final = pool.size();
    assert(checkpoint_b_final == checkpoint_b + 2);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    t.push();  // Frame C
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Re-intern content from both Frame A and Frame B
    const expr* early_content_2_again = pool.functor("early_2", {});
    assert(early_content_2 == early_content_2_again);
    assert(pool.exprs.size() == checkpoint_b_final);  // No change
    assert(pool.exprs.count(*early_content_2) == 1);
    
    const expr* mid_content_again = pool.functor("mid_content", {});
    assert(mid_content == mid_content_again);
    assert(pool.size() == checkpoint_b_final);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b_final);  // Set size unchanged
    assert(pool.exprs.count(*mid_content) == 1);
    
    // Add new content in Frame C
    const expr* frame_c_content = pool.functor("frame_c", {});
    assert(pool.exprs.size() == checkpoint_b_final + 1);
    assert(pool.exprs.count(*frame_c_content) == 1);
    
    size_t checkpoint_c = pool.size();
    assert(checkpoint_c == checkpoint_b_final + 1);
    assert(pool.exprs.size() == checkpoint_c);
    
    // Verify all content from all frames is present
    assert(pool.exprs.count(*early_content_1) == 1);
    assert(pool.exprs.count(*early_content_2) == 1);
    assert(pool.exprs.count(*mid_content) == 1);
    assert(pool.exprs.count(*late_content_1) == 1);
    assert(pool.exprs.count(*late_content_2) == 1);
    assert(pool.exprs.count(*frame_c_content) == 1);
    
    // Pop Frame C - only frame_c_content should be removed
    t.pop();
    assert(pool.size() == checkpoint_b_final);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // frame_c_content should no longer be in the set
    // Note: We can't safely dereference frame_c_content after it's removed from the set
    // because the pointer may be invalidated. We can only check what remains.
    
    // Verify early and mid content still exist
    const expr* verify_early_1 = pool.functor("early_1", {});
    assert(verify_early_1 == early_content_1);
    assert(pool.exprs.count(*verify_early_1) == 1);
    
    const expr* verify_mid = pool.functor("mid_content", {});
    assert(verify_mid == mid_content);
    assert(pool.exprs.count(*verify_mid) == 1);
    
    const expr* verify_late_1 = pool.functor("late_1", {});
    assert(verify_late_1 == late_content_1);
    assert(pool.exprs.count(*verify_late_1) == 1);
    
    assert(pool.size() == checkpoint_b_final);  // Still unchanged
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Verify all Frame A and Frame B content is still present
    assert(pool.exprs.count(*early_content_1) == 1);
    assert(pool.exprs.count(*early_content_2) == 1);
    assert(pool.exprs.count(*mid_content) == 1);
    assert(pool.exprs.count(*late_content_1) == 1);
    assert(pool.exprs.count(*late_content_2) == 1);
    
    // Pop Frame B - should remove mid_content, late_1, late_2 but NOT early_1, early_2
    t.pop();
    assert(pool.size() == checkpoint_a);
    assert(pool.exprs.size() == checkpoint_a);
    
    // Verify early content still exists
    const expr* verify_early_1_after_b = pool.functor("early_1", {});
    assert(verify_early_1_after_b == early_content_1);
    assert(pool.exprs.count(*verify_early_1_after_b) == 1);
    
    const expr* verify_early_2_after_b = pool.functor("early_2", {});
    assert(verify_early_2_after_b == early_content_2);
    assert(pool.exprs.count(*verify_early_2_after_b) == 1);
    
    assert(pool.size() == checkpoint_a);  // Still unchanged
    assert(pool.exprs.size() == checkpoint_a);
    
    // Verify only Frame A content remains
    assert(pool.exprs.count(*early_content_1) == 1);
    assert(pool.exprs.count(*early_content_2) == 1);
    
    // Pop Frame A - should remove early_1 and early_2
    t.pop();
    assert(pool.size() == checkpoint_start);
    assert(pool.exprs.size() == checkpoint_start);
    
    // Pop initial frame
    t.pop();
    assert(pool.size() == 0);
    assert(pool.exprs.size() == 0);
    assert(pool.exprs.empty());
}

void test_expr_pool_var() {
    trail t;
    expr_pool pool(t);
    
    // Push initial frame before any operations
    t.push();
    
    // Basic var creation
    const expr* e1 = pool.var(0);
    assert(e1 != nullptr);
    assert(std::holds_alternative<expr::var>(e1->content));
    assert(std::get<expr::var>(e1->content).index == 0);
    assert(pool.size() == 1);
    assert(pool.exprs.size() == 1);
    assert(pool.exprs.count(*e1) == 1);
    
    // Interning - same index should return same pointer
    const expr* e2 = pool.var(0);
    assert(e1 == e2);
    assert(pool.size() == 1);  // No new entry added
    assert(pool.exprs.size() == 1);
    
    // Different indices should return different pointers
    const expr* e3 = pool.var(1);
    assert(e1 != e3);
    assert(pool.size() == 2);
    assert(pool.exprs.size() == 2);
    assert(pool.exprs.count(*e3) == 1);
    
    // Multiple calls with same index
    const expr* e4 = pool.var(42);
    assert(pool.exprs.size() == 3);
    const expr* e5 = pool.var(42);
    const expr* e6 = pool.var(42);
    assert(e4 == e5);
    assert(e5 == e6);
    assert(pool.size() == 3);  // Only one var(42) added
    assert(pool.exprs.size() == 3);
    assert(pool.exprs.count(*e4) == 1);
    
    // Edge cases
    const expr* e7 = pool.var(UINT32_MAX);
    assert(pool.exprs.size() == 4);
    const expr* e8 = pool.var(UINT32_MAX);
    assert(e7 == e8);
    assert(pool.size() == 4);
    assert(pool.exprs.size() == 4);
    assert(pool.exprs.count(*e7) == 1);
    
    // Sequential indices
    for (uint32_t i = 0; i < 100; i++) {
        const expr* v1 = pool.var(i);
        const expr* v2 = pool.var(i);
        assert(v1 == v2);
        assert(std::get<expr::var>(v1->content).index == i);
    }
    size_t size_after_sequential = pool.size();
    
    // Test backtracking: push frame, add content, pop frame
    size_t size_before = pool.size();
    assert(pool.exprs.size() == size_before);
    t.push();
    const expr* temp1 = pool.var(9999);
    const expr* temp2 = pool.var(8888);
    assert(pool.size() == size_before + 2);
    assert(pool.exprs.size() == size_before + 2);
    assert(pool.exprs.count(*temp1) == 1);
    assert(pool.exprs.count(*temp2) == 1);
    t.pop();
    assert(pool.size() == size_before);  // Should be back to original size
    assert(pool.exprs.size() == size_before);
    
    // Test corner case: intern same content in nested frames
    t.push();  // Frame 1
    const expr* var_100 = pool.var(100);
    size_t checkpoint1 = pool.size();
    assert(var_100 != nullptr);
    assert(pool.exprs.size() == checkpoint1);
    assert(pool.exprs.count(*var_100) == 1);
    
    t.push();  // Frame 2
    const expr* var_100_again = pool.var(100);  // Should return same pointer, no log
    assert(var_100 == var_100_again);
    assert(pool.size() == checkpoint1);  // Size unchanged
    assert(pool.exprs.size() == checkpoint1);
    
    t.pop();  // Pop frame 2
    assert(pool.size() == checkpoint1);  // Size still unchanged
    assert(pool.exprs.size() == checkpoint1);
    
    // Verify var_100 is still there
    const expr* var_100_verify = pool.var(100);
    assert(var_100 == var_100_verify);
    assert(pool.size() == checkpoint1);
    assert(pool.exprs.size() == checkpoint1);
    assert(pool.exprs.count(*var_100) == 1);
    
    t.pop();  // Pop frame 1
    // Now var_100 should be removed
    
    // Test nested pushes with checkpoints
    size_t checkpoint_start = pool.size();
    
    t.push();  // Level 1
    pool.var(200);
    pool.var(201);
    size_t checkpoint_level1 = pool.size();
    assert(checkpoint_level1 == checkpoint_start + 2);
    
    t.push();  // Level 2
    pool.var(300);
    pool.var(301);
    pool.var(302);
    size_t checkpoint_level2 = pool.size();
    assert(checkpoint_level2 == checkpoint_level1 + 3);
    
    t.push();  // Level 3
    pool.var(400);
    size_t checkpoint_level3 = pool.size();
    assert(checkpoint_level3 == checkpoint_level2 + 1);
    
    // Pop level 3
    t.pop();
    assert(pool.size() == checkpoint_level2);
    
    // Pop level 2
    t.pop();
    assert(pool.size() == checkpoint_level1);
    
    // Pop level 1
    t.pop();
    assert(pool.size() == checkpoint_start);
    
    // Test corner case: content added in earlier frame should not be removed by later frame pop
    t.push();  // Frame A
    assert(pool.exprs.size() == checkpoint_start);
    
    const expr* early_var_1 = pool.var(500);
    assert(pool.exprs.size() == checkpoint_start + 1);
    assert(pool.exprs.count(*early_var_1) == 1);
    
    const expr* early_var_2 = pool.var(501);
    assert(pool.exprs.size() == checkpoint_start + 2);
    assert(pool.exprs.count(*early_var_2) == 1);
    
    size_t checkpoint_a = pool.size();
    assert(checkpoint_a == checkpoint_start + 2);
    assert(pool.exprs.size() == checkpoint_a);
    
    t.push();  // Frame B
    assert(pool.exprs.size() == checkpoint_a);
    
    const expr* mid_var = pool.var(600);
    assert(pool.exprs.size() == checkpoint_a + 1);
    assert(pool.exprs.count(*mid_var) == 1);
    
    size_t checkpoint_b = pool.size();
    assert(checkpoint_b == checkpoint_a + 1);
    assert(pool.exprs.size() == checkpoint_b);
    
    // Re-intern early content in Frame B - should not log since already exists
    const expr* early_var_1_again = pool.var(500);
    assert(early_var_1 == early_var_1_again);
    assert(pool.size() == checkpoint_b);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b);  // Set size also unchanged
    assert(pool.exprs.count(*early_var_1) == 1);  // Still in set
    
    // Add more new content in Frame B
    const expr* late_var_1 = pool.var(700);
    assert(pool.exprs.size() == checkpoint_b + 1);
    assert(pool.exprs.count(*late_var_1) == 1);
    
    const expr* late_var_2 = pool.var(701);
    assert(pool.exprs.size() == checkpoint_b + 2);
    assert(pool.exprs.count(*late_var_2) == 1);
    
    size_t checkpoint_b_final = pool.size();
    assert(checkpoint_b_final == checkpoint_b + 2);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    t.push();  // Frame C
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Re-intern content from both Frame A and Frame B
    const expr* early_var_2_again = pool.var(501);
    assert(early_var_2 == early_var_2_again);
    assert(pool.exprs.size() == checkpoint_b_final);  // No change
    assert(pool.exprs.count(*early_var_2) == 1);
    
    const expr* mid_var_again = pool.var(600);
    assert(mid_var == mid_var_again);
    assert(pool.size() == checkpoint_b_final);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b_final);  // Set size unchanged
    assert(pool.exprs.count(*mid_var) == 1);
    
    // Add new content in Frame C
    const expr* frame_c_var = pool.var(800);
    assert(pool.exprs.size() == checkpoint_b_final + 1);
    assert(pool.exprs.count(*frame_c_var) == 1);
    
    size_t checkpoint_c = pool.size();
    assert(checkpoint_c == checkpoint_b_final + 1);
    assert(pool.exprs.size() == checkpoint_c);
    
    // Verify all content from all frames is present
    assert(pool.exprs.count(*early_var_1) == 1);
    assert(pool.exprs.count(*early_var_2) == 1);
    assert(pool.exprs.count(*mid_var) == 1);
    assert(pool.exprs.count(*late_var_1) == 1);
    assert(pool.exprs.count(*late_var_2) == 1);
    assert(pool.exprs.count(*frame_c_var) == 1);
    
    // Pop Frame C - only frame_c_var should be removed
    t.pop();
    assert(pool.size() == checkpoint_b_final);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Verify early and mid content still exist
    const expr* verify_early_1 = pool.var(500);
    assert(verify_early_1 == early_var_1);
    assert(pool.exprs.count(*verify_early_1) == 1);
    
    const expr* verify_mid = pool.var(600);
    assert(verify_mid == mid_var);
    assert(pool.exprs.count(*verify_mid) == 1);
    
    const expr* verify_late_1 = pool.var(700);
    assert(verify_late_1 == late_var_1);
    assert(pool.exprs.count(*verify_late_1) == 1);
    
    assert(pool.size() == checkpoint_b_final);  // Still unchanged
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Verify all Frame A and Frame B content is still present
    assert(pool.exprs.count(*early_var_1) == 1);
    assert(pool.exprs.count(*early_var_2) == 1);
    assert(pool.exprs.count(*mid_var) == 1);
    assert(pool.exprs.count(*late_var_1) == 1);
    assert(pool.exprs.count(*late_var_2) == 1);
    
    // Pop Frame B - should remove mid_var, late_var_1, late_var_2 but NOT early vars
    t.pop();
    assert(pool.size() == checkpoint_a);
    assert(pool.exprs.size() == checkpoint_a);
    
    // Verify early content still exists
    const expr* verify_early_1_after_b = pool.var(500);
    assert(verify_early_1_after_b == early_var_1);
    assert(pool.exprs.count(*verify_early_1_after_b) == 1);
    
    const expr* verify_early_2_after_b = pool.var(501);
    assert(verify_early_2_after_b == early_var_2);
    assert(pool.exprs.count(*verify_early_2_after_b) == 1);
    
    assert(pool.size() == checkpoint_a);  // Still unchanged
    assert(pool.exprs.size() == checkpoint_a);
    
    // Verify only Frame A content remains
    assert(pool.exprs.count(*early_var_1) == 1);
    assert(pool.exprs.count(*early_var_2) == 1);
    
    // Pop Frame A - should remove early vars
    t.pop();
    assert(pool.size() == checkpoint_start);
    assert(pool.exprs.size() == checkpoint_start);
    
    // Pop initial frame
    t.pop();
    assert(pool.size() == 0);
    assert(pool.exprs.size() == 0);
    assert(pool.exprs.empty());
}

void test_expr_pool_functor_cons() {
    trail t;
    expr_pool pool(t);
    
    // Push initial frame before any operations
    t.push();
    
    // Basic cons creation
    const expr* left = pool.functor("left", {});
    const expr* right = pool.functor("right", {});
    assert(pool.exprs.size() == 2);
    const expr* c1 = pool.functor("cons", {left, right});
    
    assert(c1 != nullptr);
    assert(std::holds_alternative<expr::functor>(c1->content));
    const expr::functor& cons1 = std::get<expr::functor>(c1->content);
    assert(cons1.args[0] == left);
    assert(cons1.args[1] == right);
    assert(pool.size() == 3);  // left, right, cons
    assert(pool.exprs.size() == 3);
    assert(pool.exprs.count(*c1) == 1);
    
    // Interning - same cons should return same pointer
    const expr* c2 = pool.functor("cons", {left, right});
    assert(c1 == c2);
    assert(pool.size() == 3);  // No new entry added
    assert(pool.exprs.size() == 3);
    
    // Different cons should return different pointers
    const expr* c3 = pool.functor("cons", {right, left});  // Swapped
    assert(c1 != c3);
    assert(pool.size() == 4);
    assert(pool.exprs.size() == 4);
    assert(pool.exprs.count(*c3) == 1);
    
    // Cons with variables
    const expr* v1 = pool.var(10);
    const expr* v2 = pool.var(20);
    assert(pool.exprs.size() == 6);
    const expr* c4 = pool.functor("cons", {v1, v2});
    assert(pool.exprs.size() == 7);
    const expr* c5 = pool.functor("cons", {v1, v2});
    assert(c4 == c5);
    assert(pool.size() == 7);  // v1, v2, cons(v1,v2)
    assert(pool.exprs.size() == 7);
    assert(pool.exprs.count(*c4) == 1);
    
    // Nested cons
    const expr* inner = pool.functor("cons", {pool.functor("a", {}), pool.functor("b", {})});
    assert(pool.exprs.count(*inner) == 1);
    const expr* outer = pool.functor("cons", {inner, pool.functor("c", {})});
    assert(pool.exprs.count(*outer) == 1);
    const expr* outer2 = pool.functor("cons", {inner, pool.functor("c", {})});
    assert(outer == outer2);
    size_t size_after_nested = pool.size();
    assert(pool.exprs.size() == size_after_nested);
    
    // Same expr on both sides
    const expr* same = pool.functor("same", {});
    const expr* c6 = pool.functor("cons", {same, same});
    assert(pool.exprs.count(*c6) == 1);
    const expr* c7 = pool.functor("cons", {same, same});
    assert(c6 == c7);
    assert(pool.size() == size_after_nested + 2);  // same, cons(same,same)
    assert(pool.exprs.size() == size_after_nested + 2);
    
    // Deep nesting with interning
    const expr* d1 = pool.functor("cons", {pool.functor("x", {}), pool.functor("y", {})});
    assert(pool.exprs.count(*d1) == 1);
    const expr* d2 = pool.functor("cons", {d1, d1});
    assert(pool.exprs.count(*d2) == 1);
    const expr* d3 = pool.functor("cons", {d2, d2});
    assert(pool.exprs.count(*d3) == 1);
    
    const expr* d1_dup = pool.functor("cons", {pool.functor("x", {}), pool.functor("y", {})});
    const expr* d2_dup = pool.functor("cons", {d1_dup, d1_dup});
    const expr* d3_dup = pool.functor("cons", {d2_dup, d2_dup});
    
    assert(d1 == d1_dup);
    assert(d2 == d2_dup);
    assert(d3 == d3_dup);
    size_t size_after_deep = pool.size();
    
    // Test backtracking: push frame, add content, pop frame
    size_t size_before = pool.size();
    t.push();
    const expr* temp_left = pool.functor("temp_left", {});
    const expr* temp_right = pool.functor("temp_right", {});
    const expr* temp_cons = pool.functor("cons", {temp_left, temp_right});
    assert(pool.size() == size_before + 3);
    t.pop();
    assert(pool.size() == size_before);  // Should be back to original size
    
    // Test corner case: intern same cons in nested frames
    t.push();  // Frame 1
    const expr* cons_a = pool.functor("cons_a", {});
    const expr* cons_b = pool.functor("cons_b", {});
    const expr* cons_ab = pool.functor("cons", {cons_a, cons_b});
    size_t checkpoint1 = pool.size();
    assert(cons_ab != nullptr);
    
    t.push();  // Frame 2
    const expr* cons_ab_again = pool.functor("cons", {cons_a, cons_b});  // Should return same pointer, no log
    assert(cons_ab == cons_ab_again);
    assert(pool.size() == checkpoint1);  // Size unchanged
    
    t.pop();  // Pop frame 2
    assert(pool.size() == checkpoint1);  // Size still unchanged
    
    // Verify cons_ab is still there
    const expr* cons_ab_verify = pool.functor("cons", {cons_a, cons_b});
    assert(cons_ab == cons_ab_verify);
    assert(pool.size() == checkpoint1);
    
    t.pop();  // Pop frame 1
    // Now cons_ab, cons_a, cons_b should be removed
    
    // Test nested pushes with checkpoints
    size_t checkpoint_start = pool.size();
    
    t.push();  // Level 1
    const expr* l1_a = pool.functor("l1_a", {});
    const expr* l1_b = pool.functor("l1_b", {});
    pool.functor("cons", {l1_a, l1_b});
    size_t checkpoint_level1 = pool.size();
    assert(checkpoint_level1 == checkpoint_start + 3);
    
    t.push();  // Level 2
    const expr* l2_a = pool.var(100);
    const expr* l2_b = pool.var(200);
    pool.functor("cons", {l2_a, l2_b});
    pool.functor("cons", {l1_a, l2_a});  // Mix from level 1 and level 2
    size_t checkpoint_level2 = pool.size();
    assert(checkpoint_level2 == checkpoint_level1 + 4);  // l2_a, l2_b, cons(l2_a,l2_b), cons(l1_a,l2_a)
    
    t.push();  // Level 3
    pool.functor("cons", {l1_a, l1_b});  // Re-intern from level 1, no new entry
    const expr* l3_cons = pool.functor("cons", {l2_a, l1_a});
    size_t checkpoint_level3 = pool.size();
    assert(checkpoint_level3 == checkpoint_level2 + 1);
    
    // Pop level 3
    t.pop();
    assert(pool.size() == checkpoint_level2);
    
    // Pop level 2
    t.pop();
    assert(pool.size() == checkpoint_level1);
    
    // Pop level 1
    t.pop();
    assert(pool.size() == checkpoint_start);
    
    // Test corner case: content added in earlier frame should not be removed by later frame pop
    t.push();  // Frame A
    assert(pool.exprs.size() == checkpoint_start);
    
    const expr* early_atom_1 = pool.functor("early_atom_1", {});
    assert(pool.exprs.size() == checkpoint_start + 1);
    assert(pool.exprs.count(*early_atom_1) == 1);
    
    const expr* early_atom_2 = pool.functor("early_atom_2", {});
    assert(pool.exprs.size() == checkpoint_start + 2);
    assert(pool.exprs.count(*early_atom_2) == 1);
    
    const expr* early_cons = pool.functor("cons", {early_atom_1, early_atom_2});
    assert(pool.exprs.size() == checkpoint_start + 3);
    assert(pool.exprs.count(*early_cons) == 1);
    
    size_t checkpoint_a = pool.size();
    assert(checkpoint_a == checkpoint_start + 3);
    assert(pool.exprs.size() == checkpoint_a);
    
    t.push();  // Frame B
    assert(pool.exprs.size() == checkpoint_a);
    
    const expr* mid_atom = pool.functor("mid_atom", {});
    assert(pool.exprs.size() == checkpoint_a + 1);
    assert(pool.exprs.count(*mid_atom) == 1);
    
    const expr* mid_cons = pool.functor("cons", {early_atom_1, mid_atom});  // Uses early_atom_1
    assert(pool.exprs.size() == checkpoint_a + 2);
    assert(pool.exprs.count(*mid_cons) == 1);
    
    size_t checkpoint_b = pool.size();
    assert(checkpoint_b == checkpoint_a + 2);  // mid_atom, mid_cons
    assert(pool.exprs.size() == checkpoint_b);
    
    // Re-intern early cons in Frame B - should not log since already exists
    const expr* early_cons_again = pool.functor("cons", {early_atom_1, early_atom_2});
    assert(early_cons == early_cons_again);
    assert(pool.size() == checkpoint_b);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b);  // Set size also unchanged
    assert(pool.exprs.count(*early_cons) == 1);  // Still in set
    
    // Add more new content in Frame B
    const expr* late_atom = pool.functor("late_atom", {});
    assert(pool.exprs.size() == checkpoint_b + 1);
    assert(pool.exprs.count(*late_atom) == 1);
    
    const expr* late_cons = pool.functor("cons", {late_atom, mid_atom});
    assert(pool.exprs.size() == checkpoint_b + 2);
    assert(pool.exprs.count(*late_cons) == 1);
    
    size_t checkpoint_b_final = pool.size();
    assert(checkpoint_b_final == checkpoint_b + 2);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    t.push();  // Frame C
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Re-intern content from both Frame A and Frame B
    const expr* early_cons_again2 = pool.functor("cons", {early_atom_1, early_atom_2});
    assert(early_cons == early_cons_again2);
    assert(pool.exprs.size() == checkpoint_b_final);  // No change
    assert(pool.exprs.count(*early_cons) == 1);
    
    const expr* mid_cons_again = pool.functor("cons", {early_atom_1, mid_atom});
    assert(mid_cons == mid_cons_again);
    assert(pool.size() == checkpoint_b_final);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b_final);  // Set size unchanged
    assert(pool.exprs.count(*mid_cons) == 1);
    
    // Add new content in Frame C
    const expr* frame_c_atom = pool.functor("frame_c_atom", {});
    assert(pool.exprs.size() == checkpoint_b_final + 1);
    assert(pool.exprs.count(*frame_c_atom) == 1);
    
    const expr* frame_c_cons = pool.functor("cons", {frame_c_atom, early_atom_1});
    assert(pool.exprs.size() == checkpoint_b_final + 2);
    assert(pool.exprs.count(*frame_c_cons) == 1);
    
    size_t checkpoint_c = pool.size();
    assert(checkpoint_c == checkpoint_b_final + 2);
    assert(pool.exprs.size() == checkpoint_c);
    
    // Verify all content from all frames is present
    assert(pool.exprs.count(*early_atom_1) == 1);
    assert(pool.exprs.count(*early_atom_2) == 1);
    assert(pool.exprs.count(*early_cons) == 1);
    assert(pool.exprs.count(*mid_atom) == 1);
    assert(pool.exprs.count(*mid_cons) == 1);
    assert(pool.exprs.count(*late_atom) == 1);
    assert(pool.exprs.count(*late_cons) == 1);
    assert(pool.exprs.count(*frame_c_atom) == 1);
    assert(pool.exprs.count(*frame_c_cons) == 1);
    
    // Pop Frame C - only frame_c_atom and frame_c_cons should be removed
    t.pop();
    assert(pool.size() == checkpoint_b_final);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Verify early and mid content still exist
    const expr* verify_early_cons = pool.functor("cons", {early_atom_1, early_atom_2});
    assert(verify_early_cons == early_cons);
    assert(pool.exprs.count(*verify_early_cons) == 1);
    
    const expr* verify_mid_cons = pool.functor("cons", {early_atom_1, mid_atom});
    assert(verify_mid_cons == mid_cons);
    assert(pool.exprs.count(*verify_mid_cons) == 1);
    
    assert(pool.size() == checkpoint_b_final);  // Still unchanged
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Verify all Frame A and Frame B content is still present
    assert(pool.exprs.count(*early_atom_1) == 1);
    assert(pool.exprs.count(*early_atom_2) == 1);
    assert(pool.exprs.count(*early_cons) == 1);
    assert(pool.exprs.count(*mid_atom) == 1);
    assert(pool.exprs.count(*mid_cons) == 1);
    assert(pool.exprs.count(*late_atom) == 1);
    assert(pool.exprs.count(*late_cons) == 1);
    
    // Pop Frame B - should remove mid_atom, mid_cons, late_atom, late_cons but NOT early content
    t.pop();
    assert(pool.size() == checkpoint_a);
    assert(pool.exprs.size() == checkpoint_a);
    
    // Verify early content still exists
    const expr* verify_early_cons_after_b = pool.functor("cons", {early_atom_1, early_atom_2});
    assert(verify_early_cons_after_b == early_cons);
    assert(pool.exprs.count(*verify_early_cons_after_b) == 1);
    
    assert(pool.size() == checkpoint_a);  // Still unchanged
    assert(pool.exprs.size() == checkpoint_a);
    
    // Verify only Frame A content remains
    assert(pool.exprs.count(*early_atom_1) == 1);
    assert(pool.exprs.count(*early_atom_2) == 1);
    assert(pool.exprs.count(*early_cons) == 1);
    
    // Pop Frame A - should remove early atoms and cons
    t.pop();
    assert(pool.size() == checkpoint_start);
    assert(pool.exprs.size() == checkpoint_start);
    
    // Pop initial frame
    t.pop();
    assert(pool.size() == 0);
    assert(pool.exprs.size() == 0);
    assert(pool.exprs.empty());
}

void test_expr_pool_import() {

    // Test 1: Import an atom from the stack - result must be a pool pointer
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr stack_atom{expr::functor{"hello", {}}};
        const expr* imported = pool.import(&stack_atom);
        assert(imported != nullptr);
        assert(imported != &stack_atom);
        assert(std::holds_alternative<expr::functor>(imported->content));
        assert(std::get<expr::functor>(imported->content).name == "hello");
        assert(pool.exprs.count(*imported) == 1);
        assert(pool.size() == 1);
        t.pop();
    }

    // Test 2: Import a var from the stack - result must be a pool pointer
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr stack_var{expr::var{42}};
        const expr* imported = pool.import(&stack_var);
        assert(imported != nullptr);
        assert(imported != &stack_var);
        assert(std::holds_alternative<expr::var>(imported->content));
        assert(std::get<expr::var>(imported->content).index == 42);
        assert(pool.exprs.count(*imported) == 1);
        assert(pool.size() == 1);
        t.pop();
    }

    // Test 3: Import a var with edge-case indices (0 and UINT32_MAX)
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr stack_var0{expr::var{0}};
        const expr* imported0 = pool.import(&stack_var0);
        assert(imported0 != &stack_var0);
        assert(std::get<expr::var>(imported0->content).index == 0);
        assert(pool.exprs.count(*imported0) == 1);
        assert(pool.size() == 1);

        expr stack_var_max{expr::var{UINT32_MAX}};
        const expr* imported_max = pool.import(&stack_var_max);
        assert(imported_max != &stack_var_max);
        assert(std::get<expr::var>(imported_max->content).index == UINT32_MAX);
        assert(pool.exprs.count(*imported_max) == 1);
        assert(pool.size() == 2);
        t.pop();
    }

    // Test 4: Import a simple cons (all three nodes on stack) - children must land in pool
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr l{expr::functor{"left", {}}};
        expr r{expr::functor{"right", {}}};
        expr c{expr::functor{"cons", {&l, &r}}};
        const expr* imported = pool.import(&c);
        assert(imported != nullptr);
        assert(imported != &c);
        assert(std::holds_alternative<expr::functor>(imported->content));
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(ic.args[0] != &l);   // pool copy, not the stack object
        assert(ic.args[1] != &r);
        assert(pool.exprs.count(*ic.args[0]) == 1);
        assert(pool.exprs.count(*ic.args[1]) == 1);
        assert(pool.exprs.count(*imported) == 1);
        assert(std::get<expr::functor>(ic.args[0]->content).name == "left");
        assert(std::get<expr::functor>(ic.args[1]->content).name == "right");
        assert(pool.size() == 3);  // l, r, cons
        t.pop();
    }

    // Test 5: Import nested structure entirely on stack: cons(cons(atom, var), atom)
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr a{expr::functor{"a", {}}};
        expr v{expr::var{1}};
        expr inner{expr::functor{"cons", {&a, &v}}};
        expr b{expr::functor{"b", {}}};
        expr root{expr::functor{"cons", {&inner, &b}}};
        const expr* imported = pool.import(&root);
        assert(imported != nullptr);
        assert(pool.size() == 5);  // a, v, inner, b, root
        const expr::functor& rc = std::get<expr::functor>(imported->content);
        const expr::functor& ic = std::get<expr::functor>(rc.args[0]->content);
        assert(std::get<expr::functor>(ic.args[0]->content).name == "a");
        assert(std::get<expr::var>(ic.args[1]->content).index == 1);
        assert(std::get<expr::functor>(rc.args[1]->content).name == "b");
        assert(pool.exprs.count(*rc.args[0]) == 1);
        assert(pool.exprs.count(*rc.args[1]) == 1);
        assert(pool.exprs.count(*imported) == 1);
        t.pop();
    }

    // Test 6: cons with LHS already in pool, RHS on stack - pool pointer must be preserved
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_x = pool.functor("x", {});
        expr stack_r{expr::functor{"r", {}}};
        expr stack_c{expr::functor{"cons", {pool_x, &stack_r}}};
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(ic.args[0] == pool_x);       // pool LHS pointer is preserved exactly
        assert(ic.args[1] != &stack_r);     // stack RHS got a fresh pool pointer
        assert(pool.exprs.count(*ic.args[1]) == 1);
        assert(std::get<expr::functor>(ic.args[1]->content).name == "r");
        assert(pool.size() == 3);  // pool_x + stack_r atom + cons
        t.pop();
    }

    // Test 7: cons with LHS on stack, RHS already in pool - pool pointer must be preserved
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_y = pool.functor("y", {});
        expr stack_l{expr::functor{"l", {}}};
        expr stack_c{expr::functor{"cons", {&stack_l, pool_y}}};
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(ic.args[0] != &stack_l);     // stack LHS got a fresh pool pointer
        assert(ic.args[1] == pool_y);       // pool RHS pointer is preserved exactly
        assert(pool.exprs.count(*ic.args[0]) == 1);
        assert(pool.size() == 3);  // pool_y + stack_l atom + cons
        t.pop();
    }

    // Test 8: nested cons where the inner cons is already in pool but outer is on stack
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_p = pool.functor("p", {});
        const expr* pool_q = pool.functor("q", {});
        const expr* pool_inner = pool.functor("cons", {pool_p, pool_q});
        expr stack_r{expr::functor{"r", {}}};
        expr stack_outer{expr::functor{"cons", {pool_inner, &stack_r}}};
        const expr* imported = pool.import(&stack_outer);
        assert(imported != nullptr);
        const expr::functor& oc = std::get<expr::functor>(imported->content);
        assert(oc.args[0] == pool_inner);   // inner cons pointer is preserved exactly
        assert(oc.args[1] != &stack_r);
        assert(pool.exprs.count(*oc.args[1]) == 1);
        assert(pool.size() == 5);  // p, q, inner, stack_r atom, outer cons
        t.pop();
    }

    // Test 9: Import atom already entirely in pool - returns same pointer, pool unchanged
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_atom = pool.functor("already", {});
        const expr* imported = pool.import(pool_atom);
        assert(imported == pool_atom);
        assert(pool.size() == 1);
        t.pop();
    }

    // Test 10: Import var already entirely in pool - returns same pointer, pool unchanged
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_var = pool.var(99);
        const expr* imported = pool.import(pool_var);
        assert(imported == pool_var);
        assert(pool.size() == 1);
        t.pop();
    }

    // Test 11: Import cons already entirely in pool - returns same pointer, pool unchanged
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pl = pool.functor("pl", {});
        const expr* pr = pool.functor("pr", {});
        const expr* pc = pool.functor("cons", {pl, pr});
        const expr* imported = pool.import(pc);
        assert(imported == pc);
        assert(pool.size() == 3);
        t.pop();
    }

    // Test 12: Import stack atom with same value as existing pool atom - must deduplicate
    // (i.e. return the pool pointer, not the stack pointer)
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_dup = pool.functor("dup", {});
        expr stack_dup{expr::functor{"dup", {}}};
        const expr* imported = pool.import(&stack_dup);
        assert(imported == pool_dup);   // must return pool pointer, not &stack_dup
        assert(pool.size() == 1);       // pool must not grow
        t.pop();
    }

    // Test 13: Import stack cons structurally identical to an existing pool cons - must deduplicate
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pa = pool.functor("a", {});
        const expr* pb = pool.functor("b", {});
        const expr* pc = pool.functor("cons", {pa, pb});
        expr stack_a{expr::functor{"a", {}}};
        expr stack_b{expr::functor{"b", {}}};
        expr stack_c{expr::functor{"cons", {&stack_a, &stack_b}}};
        const expr* imported = pool.import(&stack_c);
        assert(imported == pc);         // must deduplicate to the existing pool pointer
        assert(pool.size() == 3);
        t.pop();
    }

    // Test 14: Deeply nested all-stack structure - 9 distinct nodes
    // Shape: cons(cons(cons(atom,var), cons(var,atom)), atom)
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr d0{expr::functor{"d0", {}}};
        expr x0{expr::var{0}};
        expr ll{expr::functor{"cons", {&d0, &x0}}};
        expr x1{expr::var{1}};
        expr d1{expr::functor{"d1", {}}};
        expr lr{expr::functor{"cons", {&x1, &d1}}};
        expr l{expr::functor{"cons", {&ll, &lr}}};
        expr d2{expr::functor{"d2", {}}};
        expr root{expr::functor{"cons", {&l, &d2}}};
        const expr* imported = pool.import(&root);
        assert(imported != nullptr);
        assert(pool.size() == 9);
        const expr::functor& rc = std::get<expr::functor>(imported->content);
        assert(std::get<expr::functor>(rc.args[1]->content).name == "d2");
        assert(pool.exprs.count(*rc.args[1]) == 1);
        const expr::functor& lc = std::get<expr::functor>(rc.args[0]->content);
        assert(pool.exprs.count(*rc.args[0]) == 1);
        const expr::functor& llc = std::get<expr::functor>(lc.args[0]->content);
        assert(pool.exprs.count(*lc.args[0]) == 1);
        assert(std::get<expr::functor>(llc.args[0]->content).name == "d0");
        assert(std::get<expr::var>(llc.args[1]->content).index == 0);
        const expr::functor& lrc = std::get<expr::functor>(lc.args[1]->content);
        assert(pool.exprs.count(*lc.args[1]) == 1);
        assert(std::get<expr::var>(lrc.args[0]->content).index == 1);
        assert(std::get<expr::functor>(lrc.args[1]->content).name == "d1");
        assert(pool.exprs.count(*imported) == 1);
        t.pop();
    }

    // Test 15: cons with the same pool pointer on both sides
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pool_s = pool.functor("s", {});
        expr stack_c{expr::functor{"cons", {pool_s, pool_s}}};
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(ic.args[0] == pool_s);
        assert(ic.args[1] == pool_s);
        assert(pool.size() == 2);  // pool_s + cons
        t.pop();
    }

    // Test 16: Trail integration - import in a nested frame; pop removes imported expressions
    {
        trail t;
        expr_pool pool(t);
        t.push();
        t.push();
        expr stack_atom{expr::functor{"trail_atom", {}}};
        const expr* imported = pool.import(&stack_atom);
        assert(imported != nullptr);
        assert(pool.size() == 1);
        t.pop();
        assert(pool.size() == 0);
        t.pop();
    }

    // Test 17: Trail - import cons in inner frame where LHS comes from outer frame
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* outer_atom = pool.functor("outer", {});
        assert(pool.size() == 1);
        t.push();
        expr stack_inner{expr::functor{"inner", {}}};
        expr stack_c{expr::functor{"cons", {outer_atom, &stack_inner}}};
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(ic.args[0] == outer_atom);   // outer-frame pool pointer is preserved
        assert(pool.size() == 3);       // outer_atom + inner atom + cons
        t.pop();
        // inner atom and cons were rolled back; outer_atom must survive
        assert(pool.size() == 1);
        assert(pool.exprs.count(*outer_atom) == 1);
        t.pop();
    }

    // Test 18: Importing an expression already in pool must not alter pool size,
    // even when called multiple times in a row
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* pa = pool.functor("a", {});
        const expr* pb = pool.var(0);
        const expr* pc = pool.functor("cons", {pa, pb});
        assert(pool.size() == 3);
        assert(pool.import(pa) == pa);
        assert(pool.import(pb) == pb);
        assert(pool.import(pc) == pc);
        assert(pool.import(pa) == pa);  // repeat
        assert(pool.import(pc) == pc);  // repeat
        assert(pool.size() == 3);       // absolutely no growth
        t.pop();
    }

    // Test 19: cons created via pool.cons() with raw stack pointers as children,
    // then imported. The cons itself is already in the pool's set, but its children
    // are not - import must still recurse into them and return a fully-interned cons.
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr stack_e1{expr::functor{"e1", {}}};
        expr stack_e2{expr::functor{"e2", {}}};

        // Deliberately construct a cons in the pool whose children are stack pointers
        const expr* pool_cons_bad = pool.functor("cons", {&stack_e1, &stack_e2});
        assert(pool.exprs.count(*pool_cons_bad) == 1);  // cons is in pool
        assert(pool.exprs.count(stack_e1) == 0);        // but children are NOT
        assert(pool.exprs.count(stack_e2) == 0);
        assert(pool.size() == 1);

        const expr* imported = pool.import(pool_cons_bad);

        // import must recurse through the children even though the cons itself
        // was already in the set; result must be a fully-interned cons
        assert(imported != nullptr);
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(pool.exprs.count(*ic.args[0]) == 1);  // e1 now in pool
        assert(pool.exprs.count(*ic.args[1]) == 1);  // e2 now in pool
        assert(ic.args[0] != &stack_e1);              // pool pointer, not stack
        assert(ic.args[1] != &stack_e2);
        assert(std::get<expr::functor>(ic.args[0]->content).name == "e1");
        assert(std::get<expr::functor>(ic.args[1]->content).name == "e2");
        // pool has: pool_cons_bad (with stack children) + e1 + e2 + proper cons
        assert(pool.size() == 4);
        t.pop();
    }

    // Test 20: Same stack atom referenced multiple times throughout the tree -
    // all references must collapse to a single pool entry
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr a{expr::functor{"a", {}}};
        // &a appears as both children of inner, and again as the rhs of root
        expr inner{expr::functor{"cons", {&a, &a}}};
        expr root{expr::functor{"cons", {&inner, &a}}};
        const expr* imported = pool.import(&root);
        assert(imported != nullptr);
        // Only 3 pool entries: atom "a", cons(a,a), root cons
        assert(pool.size() == 3);
        const expr::functor& rc = std::get<expr::functor>(imported->content);
        const expr::functor& ic = std::get<expr::functor>(rc.args[0]->content);
        // All four leaf references resolve to the exact same pool pointer
        assert(ic.args[0] == ic.args[1]);
        assert(ic.args[0] == rc.args[1]);
        assert(pool.exprs.count(*ic.args[0]) == 1);
        t.pop();
    }

    // Test 21: Same stack atom as both direct children of a single cons -
    // pool must contain exactly one copy of the atom
    {
        trail t;
        expr_pool pool(t);
        t.push();
        expr a{expr::functor{"a", {}}};
        expr c{expr::functor{"cons", {&a, &a}}}; 
        const expr* imported = pool.import(&c);
        assert(imported != nullptr);
        assert(pool.size() == 2);  // atom "a" + cons, not 3
        const expr::functor& ic = std::get<expr::functor>(imported->content);
        assert(ic.args[0] == ic.args[1]);  // both sides are the same pool pointer
        assert(pool.exprs.count(*ic.args[0]) == 1);
        t.pop();
    }

    // Test 22: Import from a different pool - pool1 pointers are foreign to pool2
    // so the entire tree must be re-interned into pool2 with fresh pool2 pointers
    {
        trail t1;
        expr_pool pool1(t1);
        t1.push();
        const expr* p1_atom = pool1.functor("x", {});
        const expr* p1_var  = pool1.var(5);
        const expr* p1_cons = pool1.functor("cons", {p1_atom, p1_var});
        assert(pool1.size() == 3);

        trail t2;
        expr_pool pool2(t2);
        t2.push();
        const expr* p2_cons = pool2.import(p1_cons);
        assert(p2_cons != p1_cons);         // different pool, different pointer
        assert(pool2.size() == 3);          // x, var(5), cons re-interned into pool2
        const expr::functor& c = std::get<expr::functor>(p2_cons->content);
        assert(c.args[0] != p1_atom);           // pool2 pointer, not pool1's
        assert(c.args[1] != p1_var);
        assert(pool2.exprs.count(*c.args[0]) == 1);
        assert(pool2.exprs.count(*c.args[1]) == 1);
        assert(pool2.exprs.count(*p2_cons) == 1);
        assert(std::get<expr::functor>(c.args[0]->content).name == "x");
        assert(std::get<expr::var>(c.args[1]->content).index == 5);
        // pool1 is unaffected
        assert(pool1.size() == 3);
        t1.pop();
        t2.pop();
    }
}

void test_bind_map_bind() {
    // bind() is the fundamental function for managing bindings with trail support
    // It tracks all changes to the bindings map and logs rollback operations
    // Tests verify: new bindings, updates, no-op optimization, trail integration
    
    // ========== BASIC BIND OPERATIONS ==========
    
    // Test 1: Bind new entry - should insert and log erase
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"test", {}}};
        bm.bind(0, &a1);
        
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(0) == 1);
        assert(bm.bindings.at(0) == &a1);
        
        // Pop should erase the entry
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.bindings.count(0) == 0);
    }
    
    // Test 2: Bind multiple new entries
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"first", {}}};
        expr a2{expr::functor{"second", {}}};
        expr a3{expr::functor{"third", {}}};
        
        bm.bind(0, &a1);
        bm.bind(1, &a2);
        bm.bind(2, &a3);
        
        assert(bm.bindings.size() == 3);
        assert(bm.bindings.at(0) == &a1);
        assert(bm.bindings.at(1) == &a2);
        assert(bm.bindings.at(2) == &a3);
        
        // Pop should erase all entries
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 3: Update existing entry - should log old value
    {
        trail t;
        bind_map bm(t);
        
        // Frame 1: Create initial binding
        t.push();
        expr a1{expr::functor{"old", {}}};
        bm.bind(5, &a1);
        assert(bm.bindings.at(5) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: Update to new value
        t.push();
        expr a2{expr::functor{"new", {}}};
        bm.bind(5, &a2);
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(5) == &a2);
        
        // Pop Frame 2: should restore old value
        t.pop();
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(5) == &a1);  // Restored to old value
        
        // Pop Frame 1: should remove entry
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 4: No-op optimization - binding same value twice
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"same", {}}};
        
        bm.bind(10, &a1);
        assert(bm.bindings.at(10) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Check undo stack size after first bind
        size_t undo_stack_size = t.undo_stack.size();
        assert(undo_stack_size == 1);  // One undo for the insert
        
        // Bind same value again - should be no-op
        bm.bind(10, &a1);
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(10) == &a1);
        
        // Verify undo stack did NOT grow (no-op optimization)
        assert(t.undo_stack.size() == undo_stack_size);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 5: Binding different values sequentially
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"first", {}}};
        expr a2{expr::functor{"second", {}}};
        expr a3{expr::functor{"third", {}}};
        
        // Frame 1: Initial binding
        t.push();
        bm.bind(15, &a1);
        assert(bm.bindings.at(15) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: Update to second value
        t.push();
        bm.bind(15, &a2);
        assert(bm.bindings.at(15) == &a2);
        assert(bm.bindings.size() == 1);
        
        // Frame 3: Update to third value
        t.push();
        bm.bind(15, &a3);
        assert(bm.bindings.at(15) == &a3);
        assert(bm.bindings.size() == 1);
        
        // Pop Frame 3: restore to a2
        t.pop();
        assert(bm.bindings.at(15) == &a2);
        assert(bm.bindings.size() == 1);
        
        // Pop Frame 2: restore to a1
        t.pop();
        assert(bm.bindings.at(15) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Pop Frame 1: remove entry
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // ========== MULTIPLE TRAIL FRAMES ==========
    
    // Test 6: Multiple frames with bindings at different levels
    {
        trail t;
        bind_map bm(t);
        
        // Frame 1
        t.push();
        expr a1{expr::functor{"frame1", {}}};
        bm.bind(20, &a1);
        assert(bm.bindings.size() == 1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2
        t.push();
        expr a2{expr::functor{"frame2", {}}};
        bm.bind(21, &a2);
        assert(bm.bindings.size() == 2);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3
        t.push();
        expr a3{expr::functor{"frame3", {}}};
        bm.bind(22, &a3);
        assert(bm.bindings.size() == 3);
        
        // Pop frame 3
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.size() == 2);
        
        // Pop frame 2
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.size() == 1);
        
        // Pop frame 1
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 7: Nested frames with updates to same index
    {
        trail t;
        bind_map bm(t);
        
        // Frame 1: bind index 30 to a1
        t.push();
        expr a1{expr::functor{"v1", {}}};
        bm.bind(30, &a1);
        assert(bm.bindings.at(30) == &a1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: update index 30 to a2
        t.push();
        expr a2{expr::functor{"v2", {}}};
        bm.bind(30, &a2);
        assert(bm.bindings.at(30) == &a2);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3: update index 30 to a3
        t.push();
        expr a3{expr::functor{"v3", {}}};
        bm.bind(30, &a3);
        assert(bm.bindings.at(30) == &a3);
        
        // Pop frame 3: should restore to a2
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.at(30) == &a2);
        
        // Pop frame 2: should restore to a1
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.at(30) == &a1);
        
        // Pop frame 1: should erase entry
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 8: Multiple indices across multiple frames
    {
        trail t;
        bind_map bm(t);
        
        t.push();
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        bm.bind(40, &a1);
        bm.bind(41, &a2);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        t.push();
        expr a3{expr::functor{"c", {}}};
        expr a4{expr::functor{"d", {}}};
        bm.bind(42, &a3);
        bm.bind(43, &a4);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        t.push();
        expr a5{expr::functor{"e", {}}};
        bm.bind(40, &a5);  // Update existing from frame 1
        assert(bm.bindings.size() == 4);
        assert(bm.bindings.at(40) == &a5);
        
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.at(40) == &a1);  // Restored
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // ========== COMPLEX SCENARIOS ==========
    
    // Test 9: Binding vars to atoms, cons, and other vars across frames
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"atom", {}}};
        expr v1{expr::var{50}};
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a2, &a3}}};
        
        // Frame 1: var -> atom
        t.push();
        bm.bind(50, &a1);
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(50) == &a1);
        
        // Frame 2: var -> var
        t.push();
        bm.bind(51, &v1);
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.at(51) == &v1);
        
        // Frame 3: var -> cons
        t.push();
        bm.bind(52, &c1);
        assert(bm.bindings.size() == 3);
        assert(bm.bindings.at(52) == &c1);
        
        // Pop frames
        t.pop();
        assert(bm.bindings.size() == 2);
        
        t.pop();
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 10: Chain of bindings built incrementally across frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{60}};
        expr v2{expr::var{61}};
        expr v3{expr::var{62}};
        expr a1{expr::functor{"end", {}}};
        
        // Frame 1: Start chain 60 -> v1
        t.push();
        bm.bind(60, &v1);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: Extend 61 -> v2
        t.push();
        bm.bind(61, &v2);
        assert(bm.bindings.size() == 2);
        
        // Frame 3: Extend 62 -> v3
        t.push();
        bm.bind(62, &v3);
        assert(bm.bindings.size() == 3);
        
        // Frame 4: Terminate 63 -> a1
        t.push();
        bm.bind(63, &a1);
        assert(bm.bindings.size() == 4);
        
        // Pop frames incrementally
        t.pop();
        assert(bm.bindings.size() == 3);
        
        t.pop();
        assert(bm.bindings.size() == 2);
        
        t.pop();
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 11: Deeply nested cons with multiple var bindings across frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{70}};
        expr v2{expr::var{71}};
        expr a1{expr::functor{"a", {}}};
        
        // Build nested: cons(cons(v1, a1), v2)
        expr inner{expr::functor{"cons", {&v1, &a1}}};
        expr outer{expr::functor{"cons", {&inner, &v2}}};
        
        expr a2{expr::functor{"bound1", {}}};
        expr a3{expr::functor{"bound2", {}}};
        
        // Frame 1: Bind first var
        t.push();
        bm.bind(70, &a2);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: Bind second var
        t.push();
        bm.bind(71, &a3);
        assert(bm.bindings.size() == 2);
        
        // Frame 3: Bind to nested cons structure
        t.push();
        bm.bind(72, &outer);
        assert(bm.bindings.size() == 3);
        assert(bm.bindings.at(70) == &a2);
        assert(bm.bindings.at(71) == &a3);
        assert(bm.bindings.at(72) == &outer);
        
        // Pop frames
        t.pop();
        assert(bm.bindings.size() == 2);
        
        t.pop();
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 12: Complex scenario - multiple frames with mixed operations
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        expr a4{expr::functor{"d", {}}};
        expr a5{expr::functor{"e", {}}};
        
        // Frame 1: Create initial bindings
        t.push();
        bm.bind(80, &a1);
        bm.bind(81, &a2);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: Add new and update existing
        t.push();
        bm.bind(82, &a3);
        bm.bind(80, &a4);  // Update 80
        assert(bm.bindings.size() == 3);
        assert(bm.bindings.at(80) == &a4);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3: More updates
        t.push();
        bm.bind(81, &a5);  // Update 81
        bm.bind(83, &a1);  // New
        assert(bm.bindings.size() == 4);
        
        // Pop frame 3
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.at(81) == &a2);  // Restored
        assert(bm.bindings.count(83) == 0);  // Erased
        
        // Pop frame 2
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.at(80) == &a1);  // Restored
        assert(bm.bindings.count(82) == 0);  // Erased
        
        // Pop frame 1
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 13: Binding same value multiple times across frames (no-op optimization)
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"same", {}}};
        
        // Frame 1: Initial binding
        t.push();
        bm.bind(90, &a1);
        assert(bm.bindings.size() == 1);
        size_t undo_after_frame1 = t.undo_stack.size();
        
        // Frame 2: No-op binding (same value)
        t.push();
        bm.bind(90, &a1);  // Should be no-op
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(90) == &a1);
        // Verify undo stack didn't grow (no-op optimization)
        assert(t.undo_stack.size() == undo_after_frame1);  // No new undo logged
        
        // Frame 3: Another no-op
        t.push();
        bm.bind(90, &a1);  // Should be no-op
        bm.bind(90, &a1);  // Should be no-op
        assert(bm.bindings.at(90) == &a1);
        // Verify undo stack didn't grow
        assert(t.undo_stack.size() == undo_after_frame1);  // Still no new undos
        
        // Pop all frames
        t.pop();
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 14: Alternating between two values
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"val1", {}}};
        expr a2{expr::functor{"val2", {}}};
        
        t.push();
        bm.bind(95, &a1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        t.push();
        bm.bind(95, &a2);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        t.push();
        bm.bind(95, &a1);  // Back to a1
        std::map<uint32_t, const expr*> checkpoint3 = bm.bindings;
        
        t.push();
        bm.bind(95, &a2);  // Back to a2
        assert(bm.bindings.at(95) == &a2);
        
        t.pop();
        assert(bm.bindings == checkpoint3);
        assert(bm.bindings.at(95) == &a1);
        
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.at(95) == &a2);
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.at(95) == &a1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // ========== COMPLEX STRUCTURES ==========
    
    // Test 15: Building chains across multiple frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{100}};
        expr v2{expr::var{101}};
        expr v3{expr::var{102}};
        expr a1{expr::functor{"end", {}}};
        
        // Frame 1: Start of chain
        t.push();
        bm.bind(100, &v1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: Extend chain
        t.push();
        bm.bind(101, &v2);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3: Extend more
        t.push();
        bm.bind(102, &v3);
        std::map<uint32_t, const expr*> checkpoint3 = bm.bindings;
        
        // Frame 4: Terminate chain
        t.push();
        bm.bind(103, &a1);
        assert(bm.bindings.size() == 4);
        
        // Pop frames one by one
        t.pop();
        assert(bm.bindings == checkpoint3);
        assert(bm.bindings.size() == 3);
        
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.size() == 2);
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 16: Binding to cons structures across frames
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"z", {}}};
        expr c2{expr::functor{"cons", {&c1, &a3}}};
        
        t.push();
        bm.bind(110, &c1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        t.push();
        bm.bind(111, &c2);
        assert(bm.bindings.size() == 2);
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 17: Very complex - multiple indices, multiple frames, updates
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"1", {}}};
        expr a2{expr::functor{"2", {}}};
        expr a3{expr::functor{"3", {}}};
        expr a4{expr::functor{"4", {}}};
        expr a5{expr::functor{"5", {}}};
        expr a6{expr::functor{"6", {}}};
        
        // Frame 1
        t.push();
        bm.bind(120, &a1);
        bm.bind(121, &a2);
        bm.bind(122, &a3);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2
        t.push();
        bm.bind(120, &a4);  // Update
        bm.bind(123, &a5);  // New
        assert(bm.bindings.size() == 4);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3
        t.push();
        bm.bind(121, &a6);  // Update
        bm.bind(124, &a1);  // New
        bm.bind(122, &a2);  // Update
        assert(bm.bindings.size() == 5);
        
        t.pop();
        assert(bm.bindings == checkpoint2);
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 18: Deeply nested cons with vars bound across frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{130}};
        expr v2{expr::var{131}};
        expr v3{expr::var{132}};
        expr a1{expr::functor{"a", {}}};
        
        // Build: cons(cons(v1, v2), cons(v3, a1))
        expr left{expr::functor{"cons", {&v1, &v2}}};
        expr right{expr::functor{"cons", {&v3, &a1}}};
        expr outer{expr::functor{"cons", {&left, &right}}};
        
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr a4{expr::functor{"z", {}}};
        
        t.push();
        bm.bind(130, &a2);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        t.push();
        bm.bind(131, &a3);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        t.push();
        bm.bind(132, &a4);
        bm.bind(133, &outer);
        assert(bm.bindings.size() == 4);
        
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.bindings.size() == 2);
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 19: Stress test - many bindings, many frames, many updates
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        // Frame 1: 10 bindings
        t.push();
        for (uint32_t i = 140; i < 150; i++) {
            bm.bind(i, &a1);
        }
        assert(bm.bindings.size() == 10);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: Update half, add 5 new
        t.push();
        for (uint32_t i = 140; i < 145; i++) {
            bm.bind(i, &a2);  // Update
        }
        for (uint32_t i = 150; i < 155; i++) {
            bm.bind(i, &a3);  // New
        }
        assert(bm.bindings.size() == 15);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3: More updates
        t.push();
        for (uint32_t i = 145; i < 150; i++) {
            bm.bind(i, &a3);  // Update
        }
        assert(bm.bindings.size() == 15);
        
        t.pop();
        assert(bm.bindings == checkpoint2);
        
        t.pop();
        assert(bm.bindings == checkpoint1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 20: No-op in nested frames with undo stack verification
    {
        trail t;
        bind_map bm(t);
        
        expr a1{expr::functor{"same", {}}};
        
        t.push();
        bm.bind(160, &a1);
        size_t undo_after_bind = t.undo_stack.size();
        
        t.push();
        bm.bind(160, &a1);  // No-op
        // Verify no undo was logged
        assert(t.undo_stack.size() == undo_after_bind);
        
        t.push();
        bm.bind(160, &a1);  // No-op
        // Verify no undo was logged
        assert(t.undo_stack.size() == undo_after_bind);
        
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(160) == &a1);
        
        t.pop();
        assert(bm.bindings.at(160) == &a1);
        
        t.pop();
        assert(bm.bindings.at(160) == &a1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
    }
}

void test_bind_map_whnf() {
    // Test 1: whnf of atom returns itself
    {
        trail t;
        bind_map bm(t);
        expr a1{expr::functor{"test", {}}};
        const expr* result = bm.whnf(&a1);
        assert(result == &a1);
        assert(bm.bindings.size() == 0);  // No bindings created for non-vars
    }
    
    // Test 2: whnf of cons returns itself
    {
        trail t;
        bind_map bm(t);
        expr a1{expr::functor{"left", {}}};
        expr a2{expr::functor{"right", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 3: whnf of unbound var returns itself (no entry created)
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{0}};
        const expr* result = bm.whnf(&v1);
        assert(result == &v1);
        assert(bm.bindings.size() == 0);  // No default initialization
    }
    
    // Test 4: whnf of bound var returns the binding
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{1}};
        expr a1{expr::functor{"bound", {}}};
        bm.bindings[1] = &a1;
        
        const expr* result = bm.whnf(&v1);
        assert(result == &a1);
        assert(bm.bindings.size() == 1);
    }
    
    // Test 5: whnf with chain of var bindings (path compression)
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v1{expr::var{10}};
        expr v2{expr::var{11}};
        expr v3{expr::var{12}};
        expr a1{expr::functor{"end", {}}};
        
        // Create chain: v1 -> v2 -> v3 -> a1
        bm.bindings[10] = &v2;
        bm.bindings[11] = &v3;
        bm.bindings[12] = &a1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &a1);
        
        // Verify path compression: v1 should now point directly to a1
        assert(bm.bindings.at(10) == &a1);
        // v2 should also be compressed to a1
        assert(bm.bindings.at(11) == &a1);
        assert(bm.bindings.size() == 3);
        
        t.pop();
    }
    
    // Test 6: whnf with long chain
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v1{expr::var{20}};
        expr v2{expr::var{21}};
        expr v3{expr::var{22}};
        expr v4{expr::var{23}};
        expr v5{expr::var{24}};
        expr a1{expr::functor{"final", {}}};
        
        // Create chain: v1 -> v2 -> v3 -> v4 -> v5 -> a1
        bm.bindings[20] = &v2;
        bm.bindings[21] = &v3;
        bm.bindings[22] = &v4;
        bm.bindings[23] = &v5;
        bm.bindings[24] = &a1;
        assert(bm.bindings.size() == 5);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &a1);
        
        // Verify all intermediate vars are compressed to a1
        assert(bm.bindings.at(20) == &a1);
        assert(bm.bindings.at(21) == &a1);
        assert(bm.bindings.at(22) == &a1);
        assert(bm.bindings.at(23) == &a1);
        assert(bm.bindings.size() == 5);
        
        t.pop();
    }
    
    // Test 7: whnf with var bound to cons
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{30}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        bm.bindings[30] = &c1;
        assert(bm.bindings.size() == 1);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &c1);
        assert(std::holds_alternative<expr::functor>(result->content));
        assert(bm.bindings.size() == 1);
    }
    
    // Test 8: whnf with var bound to another var bound to cons
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v1{expr::var{40}};
        expr v2{expr::var{41}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        bm.bindings[40] = &v2;
        bm.bindings[41] = &c1;
        assert(bm.bindings.size() == 2);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &c1);
        // Verify path compression
        assert(bm.bindings.at(40) == &c1);
        assert(bm.bindings.size() == 2);
        
        t.pop();
    }
    
    // Test 9: Multiple unbound vars remain unbound (no entries created)
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{50}};
        expr v2{expr::var{51}};
        expr v3{expr::var{52}};
        
        assert(bm.whnf(&v1) == &v1);
        assert(bm.bindings.size() == 0);
        
        assert(bm.whnf(&v2) == &v2);
        assert(bm.bindings.size() == 0);
        
        assert(bm.whnf(&v3) == &v3);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 10: whnf called multiple times on same var with binding
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{60}};
        expr a1{expr::functor{"repeated", {}}};
        bm.bindings[60] = &a1;
        assert(bm.bindings.size() == 1);
        
        const expr* r1 = bm.whnf(&v1);
        const expr* r2 = bm.whnf(&v1);
        const expr* r3 = bm.whnf(&v1);
        
        assert(r1 == &a1);
        assert(r2 == &a1);
        assert(r3 == &a1);
        assert(bm.bindings.size() == 1);
    }
    
    // Test 11: whnf with var bound to var bound to var (partial chain)
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v1{expr::var{70}};
        expr v2{expr::var{71}};
        expr v3{expr::var{72}};
        
        // Chain: v1 -> v2 -> v3 (v3 is unbound - no entry for it)
        bm.bindings[70] = &v2;
        bm.bindings[71] = &v3;
        assert(bm.bindings.size() == 2);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &v3);  // Should return v3, the unbound var
        
        // Verify path compression to v3
        assert(bm.bindings.at(70) == &v3);
        assert(bm.bindings.at(71) == &v3);
        assert(bm.bindings.size() == 2);
        
        t.pop();
    }
    
    // Test 12: whnf with nested cons structures
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{80}};
        expr a1{expr::functor{"inner", {}}};
        expr a2{expr::functor{"outer", {}}};
        expr inner_cons{expr::functor{"cons", {&a1, &a2}}};
        expr a3{expr::functor{"wrap", {}}};
        expr outer_cons{expr::functor{"cons", {&inner_cons, &a3}}};
        
        bm.bindings[80] = &outer_cons;
        assert(bm.bindings.size() == 1);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &outer_cons);
        assert(std::holds_alternative<expr::functor>(result->content));
        assert(bm.bindings.size() == 1);
    }
    
    // Test 13: whnf with different var indices
    {
        trail t;
        bind_map bm(t);
        expr v_small{expr::var{0}};
        expr v_large{expr::var{UINT32_MAX}};
        expr a1{expr::functor{"small", {}}};
        expr a2{expr::functor{"large", {}}};
        
        bm.bindings[0] = &a1;
        bm.bindings[UINT32_MAX] = &a2;
        assert(bm.bindings.size() == 2);
        
        assert(bm.whnf(&v_small) == &a1);
        assert(bm.whnf(&v_large) == &a2);
        assert(bm.bindings.size() == 2);
    }
    
    // Test 14: whnf preserves atom values
    {
        trail t;
        bind_map bm(t);
        expr a1{expr::functor{"", {}}};
        expr a2{expr::functor{"test123", {}}};
        expr a3{expr::functor{"!@#$%", {}}};
        
        assert(bm.whnf(&a1) == &a1);
        assert(bm.whnf(&a2) == &a2);
        assert(bm.whnf(&a3) == &a3);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 15: whnf with complex chain ending in cons
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v1{expr::var{90}};
        expr v2{expr::var{91}};
        expr v3{expr::var{92}};
        expr a1{expr::functor{"left", {}}};
        expr a2{expr::functor{"right", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        // Chain: v1 -> v2 -> v3 -> c1
        bm.bindings[90] = &v2;
        bm.bindings[91] = &v3;
        bm.bindings[92] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &c1);
        
        // Verify all vars compressed to c1
        assert(bm.bindings.at(90) == &c1);
        assert(bm.bindings.at(91) == &c1);
        assert(bm.bindings.size() == 3);
        
        t.pop();
    }
    
    // Test 16: WHNF does NOT reduce vars inside cons (WEAK head, not strong)
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{100}};
        expr v2{expr::var{101}};
        expr a1{expr::functor{"bound_atom", {}}};
        
        // Bind v2 to an atom
        bm.bindings[101] = &a1;
        
        // Create cons with v1 and v2 as children
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // whnf of the cons should return the cons itself, NOT reduce children
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        assert(std::holds_alternative<expr::functor>(result->content));
        
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        // Children should still be the original var pointers, NOT reduced
        assert(cons_ref.args[0] == &v1);
        assert(cons_ref.args[1] == &v2);
        assert(bm.bindings.size() == 1);
    }
    
    // Test 17: Var bound to cons containing bound vars - children not reduced
    {
        trail t;
        bind_map bm(t);
        expr v_outer{expr::var{110}};
        expr v_left{expr::var{111}};
        expr v_right{expr::var{112}};
        expr a1{expr::functor{"left_val", {}}};
        expr a2{expr::functor{"right_val", {}}};
        
        // Bind the inner vars
        bm.bindings[111] = &a1;
        bm.bindings[112] = &a2;
        
        // Create cons with bound vars as children
        expr c1{expr::functor{"cons", {&v_left, &v_right}}};
        bm.bindings[110] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // The cons children should still point to vars, not atoms
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v_left);
        assert(cons_ref.args[1] == &v_right);
        assert(bm.bindings.size() == 3);
    }
    
    // Test 18: Chain ending in cons with bound vars inside
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v_chain1{expr::var{120}};
        expr v_chain2{expr::var{121}};
        expr v_inner1{expr::var{122}};
        expr v_inner2{expr::var{123}};
        expr a1{expr::functor{"inner_bound", {}}};
        
        bm.bindings[122] = &a1;
        
        expr c1{expr::functor{"cons", {&v_inner1, &v_inner2}}};
        bm.bindings[120] = &v_chain2;
        bm.bindings[121] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_chain1);
        assert(result == &c1);
        
        // Cons children should be unchanged
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v_inner1);
        assert(cons_ref.args[1] == &v_inner2);
        
        // Path compression on outer chain
        assert(bm.bindings.at(120) == &c1);
        assert(bm.bindings.size() == 3);
        
        t.pop();
    }
    
    // Test 19: Deeply nested cons with vars at all levels
    {
        trail t;
        bind_map bm(t);
        expr v_outer{expr::var{130}};
        expr v1{expr::var{131}};
        expr v2{expr::var{132}};
        expr v3{expr::var{133}};
        expr v4{expr::var{134}};
        
        // Bind some inner vars
        expr a1{expr::functor{"bound1", {}}};
        expr a2{expr::functor{"bound2", {}}};
        bm.bindings[131] = &a1;
        bm.bindings[133] = &a2;
        
        // Create nested structure: cons(cons(v1, v2), cons(v3, v4))
        expr inner_left{expr::functor{"cons", {&v1, &v2}}};
        expr inner_right{expr::functor{"cons", {&v3, &v4}}};
        expr outer{expr::functor{"cons", {&inner_left, &inner_right}}};
        
        bm.bindings[130] = &outer;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &outer);
        
        // Verify structure is unchanged - vars inside cons are not reduced
        const expr::functor& outer_cons = std::get<expr::functor>(result->content);
        assert(outer_cons.args[0] == &inner_left);
        assert(outer_cons.args[1] == &inner_right);
        
        const expr::functor& left_cons = std::get<expr::functor>(outer_cons.args[0]->content);
        assert(left_cons.args[0] == &v1);  // Still points to var, not a1
        assert(left_cons.args[1] == &v2);
        
        const expr::functor& right_cons = std::get<expr::functor>(outer_cons.args[1]->content);
        assert(right_cons.args[0] == &v3);  // Still points to var, not a2
        assert(right_cons.args[1] == &v4);
        
        assert(bm.bindings.size() == 3);
    }
    
    // Test 20: Var bound to cons of vars, one of which chains to atom
    {
        trail t;
        bind_map bm(t);
        expr v_outer{expr::var{140}};
        expr v_left{expr::var{141}};
        expr v_right{expr::var{142}};
        expr v_chain{expr::var{143}};
        expr a1{expr::functor{"chained", {}}};
        
        // v_left chains to atom
        bm.bindings[141] = &v_chain;
        bm.bindings[143] = &a1;
        
        expr c1{expr::functor{"cons", {&v_left, &v_right}}};
        bm.bindings[140] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Cons children should be unchanged - v_left is NOT reduced to a1
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v_left);
        assert(cons_ref.args[1] == &v_right);
        assert(bm.bindings.size() == 3);
    }
    
    // Test 21: Multiple vars bound to same atom
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{150}};
        expr v2{expr::var{151}};
        expr v3{expr::var{152}};
        expr a1{expr::functor{"shared", {}}};
        
        bm.bindings[150] = &a1;
        bm.bindings[151] = &a1;
        bm.bindings[152] = &a1;
        assert(bm.bindings.size() == 3);
        
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);
        assert(bm.whnf(&v3) == &a1);
        assert(bm.bindings.size() == 3);
    }
    
    // Test 22: Multiple vars bound to same cons
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{160}};
        expr v2{expr::var{161}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        bm.bindings[160] = &c1;
        bm.bindings[161] = &c1;
        assert(bm.bindings.size() == 2);
        
        assert(bm.whnf(&v1) == &c1);
        assert(bm.whnf(&v2) == &c1);
        assert(bm.bindings.size() == 2);
    }
    
    // Test 23: Cons of unbound vars
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{170}};
        expr v2{expr::var{171}};
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        
        // Cons children remain as unbound vars
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v1);
        assert(cons_ref.args[1] == &v2);
        assert(bm.bindings.size() == 0);  // No bindings created
    }
    
    // Test 24: Cons of mix (atom and var)
    {
        trail t;
        bind_map bm(t);
        expr a1{expr::functor{"atom", {}}};
        expr v1{expr::var{180}};
        expr c1{expr::functor{"cons", {&a1, &v1}}};
        
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &a1);
        assert(cons_ref.args[1] == &v1);  // Var not reduced
        assert(bm.bindings.size() == 0);
    }
    
    // Test 25: Cons of mix (var and cons)
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{190}};
        expr a1{expr::functor{"inner", {}}};
        expr a2{expr::functor{"inner2", {}}};
        expr inner_cons{expr::functor{"cons", {&a1, &a2}}};
        expr outer_cons{expr::functor{"cons", {&v1, &inner_cons}}};
        
        const expr* result = bm.whnf(&outer_cons);
        assert(result == &outer_cons);
        
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v1);  // Var not reduced
        assert(cons_ref.args[1] == &inner_cons);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 26: Var bound to cons of bound vars - only outer reduced
    {
        trail t;
        bind_map bm(t);
        expr v_outer{expr::var{200}};
        expr v_inner1{expr::var{201}};
        expr v_inner2{expr::var{202}};
        expr a1{expr::functor{"val1", {}}};
        expr a2{expr::functor{"val2", {}}};
        
        // Bind inner vars
        bm.bindings[201] = &a1;
        bm.bindings[202] = &a2;
        
        // Create cons with bound vars
        expr c1{expr::functor{"cons", {&v_inner1, &v_inner2}}};
        bm.bindings[200] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Children should STILL be vars, not reduced to atoms
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v_inner1);
        assert(cons_ref.args[1] == &v_inner2);
        assert(std::holds_alternative<expr::var>(cons_ref.args[0]->content));
        assert(std::holds_alternative<expr::var>(cons_ref.args[1]->content));
        assert(bm.bindings.size() == 3);
    }
    
    // Test 27: Chain to cons of chained vars
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v_outer{expr::var{210}};
        expr v_mid{expr::var{211}};
        expr v_left{expr::var{212}};
        expr v_right{expr::var{213}};
        expr v_left_chain{expr::var{214}};
        expr a1{expr::functor{"left_end", {}}};
        
        // v_left chains to atom
        bm.bindings[212] = &v_left_chain;
        bm.bindings[214] = &a1;
        
        // Cons contains chained var and unbound var
        expr c1{expr::functor{"cons", {&v_left, &v_right}}};
        bm.bindings[210] = &v_mid;
        bm.bindings[211] = &c1;
        assert(bm.bindings.size() == 4);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Cons children should be unchanged - not reduced
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v_left);  // Still v_left, not a1
        assert(cons_ref.args[1] == &v_right);
        
        // Outer chain should be compressed
        assert(bm.bindings.at(210) == &c1);
        assert(bm.bindings.size() == 4);
        
        t.pop();
    }
    
    // Test 28: Deeply nested cons with vars everywhere
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{220}};
        expr v2{expr::var{221}};
        expr v3{expr::var{222}};
        expr v4{expr::var{223}};
        expr a1{expr::functor{"deep", {}}};
        
        // Bind v4
        bm.bindings[223] = &a1;
        
        // Create: cons(cons(v1, v2), cons(v3, v4))
        expr inner_left{expr::functor{"cons", {&v1, &v2}}};
        expr inner_right{expr::functor{"cons", {&v3, &v4}}};
        expr outer{expr::functor{"cons", {&inner_left, &inner_right}}};
        
        const expr* result = bm.whnf(&outer);
        assert(result == &outer);
        
        // All structure should be preserved, vars not reduced
        const expr::functor& outer_cons = std::get<expr::functor>(result->content);
        assert(outer_cons.args[0] == &inner_left);
        assert(outer_cons.args[1] == &inner_right);
        
        const expr::functor& left_cons = std::get<expr::functor>(outer_cons.args[0]->content);
        assert(left_cons.args[0] == &v1);
        assert(left_cons.args[1] == &v2);
        
        const expr::functor& right_cons = std::get<expr::functor>(outer_cons.args[1]->content);
        assert(right_cons.args[0] == &v3);
        assert(right_cons.args[1] == &v4);  // Still v4, not a1
        
        assert(bm.bindings.size() == 1);
    }
    
    // Test 29: Var chain to cons of var chains
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v_outer{expr::var{230}};
        expr v_outer_chain{expr::var{231}};
        expr v_left{expr::var{232}};
        expr v_left_chain{expr::var{233}};
        expr v_right{expr::var{234}};
        expr a_left{expr::functor{"left", {}}};
        expr a_right{expr::functor{"right", {}}};
        
        // Setup chains
        bm.bindings[232] = &v_left_chain;
        bm.bindings[233] = &a_left;
        bm.bindings[234] = &a_right;
        
        expr c1{expr::functor{"cons", {&v_left, &v_right}}};
        bm.bindings[230] = &v_outer_chain;
        bm.bindings[231] = &c1;
        assert(bm.bindings.size() == 5);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Cons children are still original vars, not reduced
        const expr::functor& cons_ref = std::get<expr::functor>(result->content);
        assert(cons_ref.args[0] == &v_left);
        assert(cons_ref.args[1] == &v_right);
        
        // Outer chain compressed
        assert(bm.bindings.at(230) == &c1);
        assert(bm.bindings.size() == 5);
        
        t.pop();
    }
    
    // Test 30: Empty bindings map remains empty for unbound vars
    {
        trail t;
        bind_map bm(t);
        assert(bm.bindings.size() == 0);
        
        expr v1{expr::var{240}};
        bm.whnf(&v1);
        assert(bm.bindings.size() == 0);
        
        expr v2{expr::var{241}};
        bm.whnf(&v2);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 31: Same unbound var called multiple times creates no entries
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{250}};
        
        bm.whnf(&v1);
        assert(bm.bindings.size() == 0);
        
        bm.whnf(&v1);
        assert(bm.bindings.size() == 0);
        
        bm.whnf(&v1);
        assert(bm.bindings.size() == 0);
    }
    
    // ========== TRAIL INTEGRATION TESTS ==========
    
    // Test 32: whnf with trail - bind in frame, pop frame, verify rollback
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{300}};
        expr a1{expr::functor{"frame1", {}}};
        
        t.push();
        bm.bind(300, &a1);
        assert(bm.bindings.size() == 1);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &a1);
        assert(bm.bindings.size() == 1);
        
        t.pop();
        assert(bm.bindings.size() == 0);
        
        // After pop, v1 is unbound again
        result = bm.whnf(&v1);
        assert(result == &v1);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 33: whnf with nested frames - multiple bindings
    {
        trail t;
        bind_map bm(t);

        expr v1{expr::var{310}};
        expr v2{expr::var{311}};
        expr a1{expr::functor{"outer", {}}};
        expr a2{expr::functor{"inner", {}}};
        
        // Frame 1: bind v1 to a1
        t.push();
        bm.bind(310, &a1);
        assert(bm.whnf(&v1) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: bind v2 to a2
        t.push();
        bm.bind(311, &a2);
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a2);
        assert(bm.bindings.size() == 2);
        
        // Pop frame 2
        t.pop();
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &v2);  // v2 unbound again
        assert(bm.bindings.size() == 1);
        
        // Pop frame 1
        t.pop();
        assert(bm.whnf(&v1) == &v1);  // v1 unbound again
        assert(bm.bindings.size() == 0);
    }
    
    // Test 34: whnf with path compression across frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{320}};
        expr v2{expr::var{321}};
        expr v3{expr::var{322}};
        expr a1{expr::functor{"end", {}}};
        
        // Frame 1: v1 -> v2
        t.push();
        bm.bind(320, &v2);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: v2 -> v3
        t.push();
        bm.bind(321, &v3);
        assert(bm.bindings.size() == 2);
        
        // Frame 3: v3 -> a1
        t.push();
        bm.bind(322, &a1);
        assert(bm.bindings.size() == 3);
        
        // whnf should follow chain and compress
        const expr* result = bm.whnf(&v1);
        assert(result == &a1);
        assert(bm.bindings.size() == 3);  // Path compression updates values, not count
        
        // After compression, v1 should point directly to a1
        assert(bm.bindings.at(320) == &a1);
        
        // Pop frame 3
        t.pop();
        assert(bm.bindings.size() == 2);
        // v1 should be restored to point to v2
        assert(bm.bindings.at(320) == &v2);
        
        // Pop frame 2
        t.pop();
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.at(320) == &v2);  // Still points to v2
        
        // Pop frame 1
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 35: whnf with trail - update binding in nested frame
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{330}};
        expr a1{expr::functor{"first", {}}};
        expr a2{expr::functor{"second", {}}};
        
        // Frame 1: bind v1 to a1
        t.push();
        bm.bind(330, &a1);
        assert(bm.whnf(&v1) == &a1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: update v1 to a2
        t.push();
        bm.bind(330, &a2);
        assert(bm.whnf(&v1) == &a2);
        assert(bm.bindings.size() == 1);
        
        // Pop frame 2 - should restore to a1
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.whnf(&v1) == &a1);
        
        // Pop frame 1 - should be unbound
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
    }
    
    // Test 36: Complex - chain built across frames, then popped
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{340}};
        expr v2{expr::var{341}};
        expr v3{expr::var{342}};
        expr v4{expr::var{343}};
        expr a1{expr::functor{"final", {}}};
        
        // Frame 1: v1 -> v2
        t.push();
        bm.bind(340, &v2);
        
        // Frame 2: v2 -> v3
        t.push();
        bm.bind(341, &v3);
        
        // Frame 3: v3 -> v4
        t.push();
        bm.bind(342, &v4);
        
        // Frame 4: v4 -> a1
        t.push();
        bm.bind(343, &a1);
        
        // Full chain: v1 -> v2 -> v3 -> v4 -> a1
        assert(bm.whnf(&v1) == &a1);
        assert(bm.bindings.size() == 4);
        
        // Pop frame 4: v4 becomes unbound
        t.pop();
        assert(bm.bindings.size() == 3);
        assert(bm.whnf(&v1) == &v4);  // Chain ends at v4 now
        
        // Pop frame 3: v3 becomes unbound
        t.pop();
        assert(bm.bindings.size() == 2);
        assert(bm.whnf(&v1) == &v3);  // Chain ends at v3 now
        
        // Pop frame 2: v2 becomes unbound
        t.pop();
        assert(bm.bindings.size() == 1);
        assert(bm.whnf(&v1) == &v2);  // Chain ends at v2 now
        
        // Pop frame 1: v1 becomes unbound
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);  // v1 is unbound
    }
    
    // Test 37: whnf with cons containing vars bound in frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{350}};
        expr v2{expr::var{351}};
        expr a1{expr::functor{"left", {}}};
        expr a2{expr::functor{"right", {}}};
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Frame 1: bind v1 to a1
        t.push();
        bm.bind(350, &a1);
        
        // whnf of cons returns cons itself (weak head normal form)
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        
        // But whnf of v1 returns a1
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &v2);  // v2 still unbound
        
        // Frame 2: bind v2 to a2
        t.push();
        bm.bind(351, &a2);
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a2);
        assert(bm.whnf(&c1) == &c1);  // cons still returns itself
        
        // Pop frame 2
        t.pop();
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &v2);
        assert(bm.bindings.size() == 1);
        
        // Pop frame 1
        t.pop();
        assert(bm.whnf(&v1) == &v1);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 38: Multiple independent chains across frames
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{360}};
        expr v2{expr::var{361}};
        expr v3{expr::var{362}};
        expr v4{expr::var{363}};
        expr a1{expr::functor{"chain1", {}}};
        expr a2{expr::functor{"chain2", {}}};
        
        // Frame 1: Start two chains
        t.push();
        bm.bind(360, &v2);  // Chain 1: v1 -> v2
        bm.bind(362, &v4);  // Chain 2: v3 -> v4
        assert(bm.bindings.size() == 2);
        
        // Frame 2: Complete both chains
        t.push();
        bm.bind(361, &a1);  // v2 -> a1
        bm.bind(363, &a2);  // v4 -> a2
        assert(bm.bindings.size() == 4);
        
        // Both chains resolve
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v3) == &a2);
        
        // Pop frame 2: chains end at v2 and v4
        t.pop();
        assert(bm.bindings.size() == 2);
        assert(bm.whnf(&v1) == &v2);
        assert(bm.whnf(&v3) == &v4);
        
        // Pop frame 1: all unbound
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
        assert(bm.whnf(&v3) == &v3);
    }
    
    // Test 39: Very complex - multiple frames with overlapping updates and chains
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{370}};
        expr v2{expr::var{371}};
        expr v3{expr::var{372}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        // Frame 1: v1 -> a1
        t.push();
        bm.bind(370, &a1);
        assert(bm.whnf(&v1) == &a1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: v1 -> a2 (update v1's binding)
        t.push();
        bm.bind(370, &a2);
        assert(bm.whnf(&v1) == &a2);
        assert(bm.bindings.size() == 1);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3: v1 -> a3 (update again), v2 -> a1
        t.push();
        bm.bind(370, &a3);
        bm.bind(371, &a1);
        assert(bm.whnf(&v1) == &a3);
        assert(bm.whnf(&v2) == &a1);
        assert(bm.bindings.size() == 2);
        
        // Pop frame 3: back to v1 -> a2
        t.pop();
        assert(bm.bindings == checkpoint2);
        assert(bm.whnf(&v1) == &a2);
        assert(bm.bindings.size() == 1);
        
        // Pop frame 2: back to v1 -> a1
        t.pop();
        assert(bm.bindings == checkpoint1);
        assert(bm.whnf(&v1) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Pop frame 1: all unbound
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
    }
    
    // Test 40: Path compression with trail - verify compressed paths are restored
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{380}};
        expr v2{expr::var{381}};
        expr v3{expr::var{382}};
        expr a1{expr::functor{"target", {}}};
        
        // Build chain in single frame: v1 -> v2 -> v3 -> a1
        t.push();
        bm.bind(380, &v2);
        bm.bind(381, &v3);
        bm.bind(382, &a1);
        assert(bm.bindings.size() == 3);
        
        // Before whnf, v1 points to v2
        assert(bm.bindings.at(380) == &v2);
        
        // whnf performs path compression
        assert(bm.whnf(&v1) == &a1);
        
        // After whnf, v1 should point directly to a1 (path compression)
        assert(bm.bindings.at(380) == &a1);
        assert(bm.bindings.size() == 3);  // Count unchanged
        
        // Pop frame - should restore original v1 -> v2 binding
        t.pop();
        assert(bm.bindings.size() == 0);
    }
    
    // Test 41: Deeply nested frames with cons structures
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{390}};
        expr v2{expr::var{391}};
        expr v3{expr::var{392}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&v2, &a1}}};
        expr c2{expr::functor{"cons", {&v3, &a2}}};
        
        // Frame 1: v1 -> c1
        t.push();
        bm.bind(390, &c1);
        assert(bm.whnf(&v1) == &c1);
        
        // Frame 2: v2 -> a1 (bind var inside cons)
        t.push();
        bm.bind(391, &a1);
        assert(bm.whnf(&v1) == &c1);  // Still returns c1 (WHNF)
        assert(bm.whnf(&v2) == &a1);  // But v2 resolves to a1
        
        // Frame 3: v1 -> c2 (rebind v1)
        t.push();
        bm.bind(390, &c2);
        assert(bm.whnf(&v1) == &c2);
        assert(bm.whnf(&v3) == &v3);  // v3 unbound
        
        // Frame 4: v3 -> a2
        t.push();
        bm.bind(392, &a2);
        assert(bm.whnf(&v1) == &c2);
        assert(bm.whnf(&v3) == &a2);
        
        // Pop frame 4
        t.pop();
        assert(bm.whnf(&v1) == &c2);
        assert(bm.whnf(&v3) == &v3);
        
        // Pop frame 3
        t.pop();
        assert(bm.whnf(&v1) == &c1);
        assert(bm.whnf(&v2) == &a1);
        
        // Pop frame 2
        t.pop();
        assert(bm.whnf(&v1) == &c1);
        assert(bm.whnf(&v2) == &v2);
        
        // Pop frame 1
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
    }
    
    // Test 42: Stress test - path compression with trail rollback
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{400}};
        expr v2{expr::var{401}};
        expr v3{expr::var{402}};
        expr v4{expr::var{403}};
        expr v5{expr::var{404}};
        expr a1{expr::functor{"1", {}}};
        expr a2{expr::functor{"2", {}}};
        expr a3{expr::functor{"3", {}}};
        
        // Frame 1: Build chain v1 -> v2 -> a1
        t.push();
        bm.bind(400, &v2);  // v1 -> v2
        bm.bind(401, &a1);  // v2 -> a1
        
        // Before whnf: bindings[400] = &v2, bindings[401] = &a1
        assert(bm.bindings.at(400) == &v2);
        assert(bm.bindings.at(401) == &a1);
        
        // Call whnf to trigger path compression
        assert(bm.whnf(&v1) == &a1);
        
        // After whnf with path compression: bindings[400] should be compressed to &a1
        // The compression happens via bind(400, &a1) which logs to Frame 1
        assert(bm.bindings.at(400) == &a1);  // Path compressed!
        assert(bm.bindings.size() == 2);
        
        // Frame 2: Build another chain v3 -> v4 -> a2
        t.push();
        bm.bind(402, &v4);  // v3 -> v4
        bm.bind(403, &a2);  // v4 -> a2
        
        // v1 still resolves to a1 (already compressed)
        assert(bm.whnf(&v1) == &a1);
        
        // Trigger path compression for v3
        assert(bm.whnf(&v3) == &a2);
        assert(bm.bindings.at(402) == &a2);  // Path compressed!
        assert(bm.bindings.size() == 4);
        
        // Frame 3: Update v2's binding (but v1 is already compressed, so won't affect v1)
        t.push();
        bm.bind(401, &v5);  // Update: v2 -> v5 (was v2 -> a1)
        bm.bind(404, &a3);  // v5 -> a3
        
        // v1 still points to a1 because it was compressed in Frame 1
        // bindings[400] = &a1 directly, doesn't go through v2 anymore
        assert(bm.whnf(&v1) == &a1);
        
        // Check v2's chain before calling whnf (before compression)
        assert(bm.bindings.at(401) == &v5);
        assert(bm.bindings.at(404) == &a3);
        
        // v2 resolves to a3, and this triggers path compression
        assert(bm.whnf(&v2) == &a3);
        
        // After whnf, v2 is compressed to point directly to a3
        assert(bm.bindings.at(401) == &a3);  // Path compressed in Frame 3!
        assert(bm.bindings.size() == 5);
        
        // Frame 4: Update v5 to point to v3
        t.push();
        bm.bind(404, &v3);  // Update: v5 -> v3 (was v5 -> a3)
        
        // v2 still points to a3 because it was compressed in Frame 3
        // bindings[401] = &a3 directly, doesn't go through v5 anymore
        assert(bm.whnf(&v2) == &a3);
        
        // But v5 now resolves to a2 (v5 -> v3 -> a2)
        assert(bm.whnf(&v5) == &a2);
        assert(bm.bindings.at(404) == &a2);  // Path compressed in Frame 4!
        assert(bm.bindings.size() == 5);
        
        // Pop Frame 4: restore v5 -> a3, undo v5's compression
        t.pop();
        assert(bm.bindings.at(404) == &a3);  // Restored
        assert(bm.bindings.at(401) == &a3);  // v2 still compressed (from Frame 3)
        assert(bm.whnf(&v2) == &a3);
        assert(bm.whnf(&v5) == &a3);
        assert(bm.bindings.size() == 5);
        
        // Pop Frame 3: restore v2 -> a1, remove v5 binding, undo v2's compression
        t.pop();
        assert(bm.bindings.at(401) == &a1);  // Restored to original Frame 1 value
        assert(bm.bindings.count(404) == 0);  // v5 binding removed
        
        // v1 is still compressed from Frame 1
        assert(bm.bindings.at(400) == &a1);
        assert(bm.whnf(&v1) == &a1);
        
        // v2 now points directly to a1 again
        assert(bm.whnf(&v2) == &a1);
        
        // v3 is still compressed from Frame 2
        assert(bm.bindings.at(402) == &a2);
        assert(bm.whnf(&v3) == &a2);
        
        assert(bm.bindings.size() == 4);
        
        // Pop Frame 2: remove v3, v4 bindings, undo v3's compression
        t.pop();
        assert(bm.bindings.count(402) == 0);  // v3 binding removed
        assert(bm.bindings.count(403) == 0);  // v4 binding removed
        
        // v1 is STILL compressed from Frame 1 (compression persists across frame pops)
        assert(bm.bindings.at(400) == &a1);
        assert(bm.whnf(&v1) == &a1);
        
        // v2 still points to a1 from Frame 1
        assert(bm.bindings.at(401) == &a1);
        assert(bm.whnf(&v2) == &a1);
        
        // v3 is now unbound
        assert(bm.whnf(&v3) == &v3);
        
        assert(bm.bindings.size() == 2);
        
        // Pop Frame 1: undo v1's compression, remove all bindings
        t.pop();
        assert(bm.bindings.size() == 0);
        
        // All vars are now unbound
        assert(bm.whnf(&v1) == &v1);
        assert(bm.whnf(&v2) == &v2);
        assert(bm.whnf(&v3) == &v3);
        assert(bm.whnf(&v4) == &v4);
        assert(bm.whnf(&v5) == &v5);
    }
    
    // Test 43: Path compression with overlapping chains and multiple compressions per frame
    {
        trail t;
        bind_map bm(t);
        
        expr v1{expr::var{500}};
        expr v2{expr::var{501}};
        expr v3{expr::var{502}};
        expr v4{expr::var{503}};
        expr v5{expr::var{504}};
        expr v6{expr::var{505}};
        expr a1{expr::functor{"end", {}}};
        
        // Frame 1: Create a long chain v1 -> v2 -> v3 -> v4 -> a1
        t.push();
        bm.bind(500, &v2);
        bm.bind(501, &v3);
        bm.bind(502, &v4);
        bm.bind(503, &a1);
        
        // Compress the entire chain
        assert(bm.whnf(&v1) == &a1);
        
        // All intermediate nodes should be compressed to a1
        assert(bm.bindings.at(500) == &a1);  // v1 compressed
        assert(bm.bindings.at(501) == &a1);  // v2 compressed
        assert(bm.bindings.at(502) == &a1);  // v3 compressed
        assert(bm.bindings.at(503) == &a1);  // v4 already pointed to a1
        
        // Frame 2: Create a branch - update v3 to point to v5 -> v6 -> a1
        t.push();
        bm.bind(502, &v5);  // Update: v3 -> v5 (was v3 -> a1 from compression)
        bm.bind(504, &v6);  // v5 -> v6
        bm.bind(505, &a1);  // v6 -> a1
        
        // v1 and v2 still point to a1 (compressed in Frame 1)
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);
        
        // v3 now goes through the new branch
        assert(bm.whnf(&v3) == &a1);
        
        // After whnf(&v3), path compression should update v3, v5, v6
        assert(bm.bindings.at(502) == &a1);  // v3 compressed in Frame 2
        assert(bm.bindings.at(504) == &a1);  // v5 compressed in Frame 2
        assert(bm.bindings.at(505) == &a1);  // v6 compressed in Frame 2
        
        // Frame 3: Update v2 to point to v3 (creating a new connection)
        t.push();
        bm.bind(501, &v3);  // Update: v2 -> v3 (was v2 -> a1 from Frame 1)
        
        // v2 now goes through v3, which is compressed to a1
        assert(bm.whnf(&v2) == &a1);
        
        // v2 should be compressed again in Frame 3
        assert(bm.bindings.at(501) == &a1);  // v2 re-compressed in Frame 3
        
        // Pop Frame 3: restore v2 -> a1 from Frame 1
        t.pop();
        assert(bm.bindings.at(501) == &a1);  // Back to Frame 1's compression
        assert(bm.whnf(&v2) == &a1);
        
        // Pop Frame 2: restore v3 -> a1 from Frame 1, remove v5, v6
        t.pop();
        assert(bm.bindings.at(502) == &a1);  // Back to Frame 1's compression
        assert(bm.bindings.count(504) == 0);  // v5 removed
        assert(bm.bindings.count(505) == 0);  // v6 removed
        assert(bm.whnf(&v3) == &a1);
        
        // Pop Frame 1: remove all bindings, undo all compressions
        t.pop();
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
        assert(bm.whnf(&v2) == &v2);
        assert(bm.whnf(&v3) == &v3);
    }
}

void test_bind_map_occurs_check() {
    // occurs_check determines if a variable (by index) occurs anywhere in an expression,
    // even through indirection via bindings. It uses whnf to follow chains.
    // Tests cover: atoms, unbound vars, bound vars, chains, cons structures, nested cons,
    // and various combinations. Circular bindings (undefined behavior) are not tested.
    
    // Test 1: occurs_check on atom - should return false
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"test", {}}};
        assert(!bm.occurs_check(0, &a1));
        assert(!bm.occurs_check(100, &a1));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 2: occurs_check on unbound var with same index - should return true
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{5}};
        assert(bm.occurs_check(5, &v1));
        assert(bm.bindings.size() == 0);  // whnf no longer creates entries
        
        t.pop();
    }
    
    // Test 3: occurs_check on unbound var with different index - should return false
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{10}};
        assert(!bm.occurs_check(5, &v1));
        assert(!bm.occurs_check(11, &v1));
        assert(bm.bindings.size() == 0);  // No entries created
        
        t.pop();
    }
    
    // Test 4: occurs_check on var bound to atom - should return false
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{15}};
        expr a1{expr::functor{"bound", {}}};
        bm.bindings[15] = &a1;
        
        assert(!bm.occurs_check(15, &v1));  // v1 reduces to atom
        assert(!bm.occurs_check(20, &v1));
        assert(bm.bindings.size() == 1);
        
        t.pop();
    }
    
    // Test 5: occurs_check on var bound to same var - creates infinite loop, SKIP
    // This is undefined behavior - circular binding
    
    // Test 6: occurs_check through chain ending in unbound var
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{30}};
        expr v2{expr::var{31}};
        expr v3{expr::var{32}};
        
        // Chain: v1 -> v2 -> v3 (v3 is unbound, don't set it explicitly)
        bm.bindings[30] = &v2;
        bm.bindings[31] = &v3;
        // Don't set bindings[32] - let whnf handle it
        
        // v1 eventually points to v3 (var 32)
        assert(bm.occurs_check(32, &v1));
        assert(bm.bindings.size() == 2);  // Only the 2 explicit bindings
        
        t.pop();
    }
    
    // Test 7: occurs_check through chain ending in atom
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{40}};
        expr v2{expr::var{41}};
        expr a1{expr::functor{"end", {}}};
        
        // Chain: v1 -> v2 -> atom
        bm.bindings[40] = &v2;
        bm.bindings[41] = &a1;
        
        assert(!bm.occurs_check(40, &v1));
        assert(!bm.occurs_check(41, &v1));
        assert(!bm.occurs_check(99, &v1));
        assert(bm.bindings.size() == 2);
        
        t.pop();
    }
    
    // Test 8: occurs_check on cons with no vars
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"left", {}}};
        expr a2{expr::functor{"right", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        assert(!bm.occurs_check(0, &c1));
        assert(!bm.occurs_check(50, &c1));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 9: occurs_check on cons with matching var in lhs
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{55}};
        expr a1{expr::functor{"right", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        assert(bm.occurs_check(55, &c1));
        assert(!bm.occurs_check(56, &c1));
        assert(bm.bindings.size() == 0);  // No entries created
        
        t.pop();
    }
    
    // Test 10: occurs_check on cons with matching var in rhs
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"left", {}}};
        expr v1{expr::var{60}};
        expr c1{expr::functor{"cons", {&a1, &v1}}};
        
        assert(bm.occurs_check(60, &c1));
        assert(!bm.occurs_check(61, &c1));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 11: occurs_check on cons with matching var in both children
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{65}};
        expr v2{expr::var{65}};  // Same index
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        assert(bm.occurs_check(65, &c1));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 12: occurs_check on cons with different vars
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{70}};
        expr v2{expr::var{71}};
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        assert(bm.occurs_check(70, &c1));
        assert(bm.occurs_check(71, &c1));
        assert(!bm.occurs_check(72, &c1));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 13: occurs_check on nested cons
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{75}};
        expr a1{expr::functor{"inner", {}}};
        expr inner_cons{expr::functor{"cons", {&v1, &a1}}};
        expr a2{expr::functor{"outer", {}}};
        expr outer_cons{expr::functor{"cons", {&inner_cons, &a2}}};
        
        assert(bm.occurs_check(75, &outer_cons));  // v1 is in nested cons
        assert(!bm.occurs_check(76, &outer_cons));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 14: occurs_check on deeply nested cons
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{80}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        // Build: cons(cons(cons(v1, a1), a2), a3)
        expr inner1{expr::functor{"cons", {&v1, &a1}}};
        expr inner2{expr::functor{"cons", {&inner1, &a2}}};
        expr outer{expr::functor{"cons", {&inner2, &a3}}};
        
        assert(bm.occurs_check(80, &outer));
        assert(!bm.occurs_check(81, &outer));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 15: occurs_check with var bound to cons containing same var (indirect cycle)
    // This is undefined behavior - circular binding
    // {
    //     bind_map bm(t);
    //     expr v1{expr::var{85}};
    //     expr v2{expr::var{85}};  // Same index
    //     expr a1{expr::functor{"test", {}}};
    //     expr c1{expr::functor{"cons", {&v2, &a1}}};
    //     
    //     bm.bindings[85] = &c1;
    //     
    //     // v1 is bound to cons containing v2 (same index), so occurs_check should find it
    //     assert(bm.occurs_check(85, &v1));
    //     assert(bm.bindings.size() == 1);
    // }
    
    // Test 16: occurs_check with var bound to cons not containing that var
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{90}};
        expr v2{expr::var{91}};
        expr a1{expr::functor{"test", {}}};
        expr c1{expr::functor{"cons", {&v2, &a1}}};
        
        bm.bindings[90] = &c1;
        
        assert(!bm.occurs_check(90, &v1));  // v1 -> c1, but c1 doesn't contain v1
        assert(bm.occurs_check(91, &v1));   // v1 -> c1 which contains v2
        assert(bm.bindings.size() == 1);  // Only the explicit binding
        
        t.pop();
    }
    
    // Test 17: occurs_check through chain to cons
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{95}};
        expr v2{expr::var{96}};
        expr v3{expr::var{97}};
        expr a1{expr::functor{"test", {}}};
        expr c1{expr::functor{"cons", {&v3, &a1}}};
        
        // Chain: v1 -> v2 -> c1 (which contains v3, v3 unbound)
        bm.bindings[95] = &v2;
        bm.bindings[96] = &c1;
        
        assert(!bm.occurs_check(95, &v1));  // After whnf, v1 -> c1, doesn't contain v1
        assert(!bm.occurs_check(96, &v1));
        assert(bm.occurs_check(97, &v1));   // c1 contains v3
        assert(bm.bindings.size() == 2);  // Only the 2 explicit bindings
        
        t.pop();
    }
    
    // Test 18: occurs_check on cons with bound vars
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{100}};
        expr v2{expr::var{101}};
        expr a1{expr::functor{"bound", {}}};
        
        // Bind v2 to atom
        bm.bindings[101] = &a1;
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        assert(bm.occurs_check(100, &c1));  // v1 is in cons
        assert(!bm.occurs_check(101, &c1)); // v2 reduces to atom
        assert(bm.bindings.size() == 1);  // Only v2's binding
        
        t.pop();
    }
    
    // Test 19: occurs_check with multiple levels of indirection
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{105}};
        expr v2{expr::var{106}};
        expr v3{expr::var{107}};
        expr v4{expr::var{108}};
        
        // Chain: v1 -> v2 -> v3 -> v4 (v4 unbound)
        bm.bindings[105] = &v2;
        bm.bindings[106] = &v3;
        bm.bindings[107] = &v4;
        
        assert(bm.occurs_check(108, &v1));  // Eventually points to v4
        assert(!bm.occurs_check(105, &v1)); // After path compression
        assert(bm.bindings.size() == 3);  // Only the 3 explicit bindings
        
        t.pop();
    }
    
    // Test 20: occurs_check on cons where both children are bound vars
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{110}};
        expr v2{expr::var{111}};
        expr a1{expr::functor{"left_bound", {}}};
        expr a2{expr::functor{"right_bound", {}}};
        
        bm.bindings[110] = &a1;
        bm.bindings[111] = &a2;
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        assert(!bm.occurs_check(110, &c1));  // Both reduce to atoms
        assert(!bm.occurs_check(111, &c1));
        assert(!bm.occurs_check(112, &c1));
        assert(bm.bindings.size() == 2);  // The 2 explicit bindings
        
        t.pop();
    }
    
    // Test 21: occurs_check on cons with chain in lhs
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{115}};
        expr v2{expr::var{116}};
        expr v3{expr::var{117}};
        expr a1{expr::functor{"rhs", {}}};
        
        // v1 -> v2 -> v3 (v3 unbound)
        bm.bindings[115] = &v2;
        bm.bindings[116] = &v3;
        
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        assert(bm.occurs_check(117, &c1));  // v1 chains to v3
        assert(!bm.occurs_check(115, &c1)); // After compression
        assert(!bm.occurs_check(116, &c1));
        assert(bm.bindings.size() == 2);  // Only the 2 explicit bindings
        
        t.pop();
    }
    
    // Test 22: occurs_check with cons of cons, target var in nested structure
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{120}};
        expr v2{expr::var{121}};
        expr a1{expr::functor{"a", {}}};
        
        expr inner{expr::functor{"cons", {&v1, &a1}}};
        expr outer{expr::functor{"cons", {&inner, &v2}}};
        
        assert(bm.occurs_check(120, &outer));
        assert(bm.occurs_check(121, &outer));
        assert(!bm.occurs_check(122, &outer));
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 23: occurs_check on var bound to deeply nested structure
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_outer{expr::var{125}};
        expr v_inner{expr::var{126}};
        expr a1{expr::functor{"deep", {}}};
        expr a2{expr::functor{"deeper", {}}};
        
        // Build nested: cons(cons(v_inner, a1), a2)
        expr inner{expr::functor{"cons", {&v_inner, &a1}}};
        expr outer{expr::functor{"cons", {&inner, &a2}}};
        
        bm.bindings[125] = &outer;
        
        assert(!bm.occurs_check(125, &v_outer));  // v_outer -> outer, doesn't contain itself
        assert(bm.occurs_check(126, &v_outer));   // outer contains v_inner
        assert(bm.bindings.size() == 1);  // Only v_outer's binding
        
        t.pop();
    }
    
    // Test 24: occurs_check with symmetric cons (same var on both sides)
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{130}};
        expr v2{expr::var{130}};  // Same var
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        assert(bm.occurs_check(130, &c1));
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 25: occurs_check on empty bindings map
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{135}};
        
        assert(bm.bindings.size() == 0);
        assert(bm.occurs_check(135, &v1));
        assert(bm.bindings.size() == 0);  // whnf no longer creates entries
        
        t.pop();
    }
    
    // Test 26: occurs_check with var bound through multiple cons layers
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{140}};
        expr v2{expr::var{141}};
        expr a1{expr::functor{"base", {}}};
        
        // Build: cons(cons(a1, v2), a1) where v2 is unbound
        expr inner{expr::functor{"cons", {&a1, &v2}}};
        expr outer{expr::functor{"cons", {&inner, &a1}}};
        
        bm.bindings[140] = &outer;
        
        assert(!bm.occurs_check(140, &v1));  // v1 -> outer, doesn't contain itself
        assert(bm.occurs_check(141, &v1));   // outer contains v2
        assert(bm.bindings.size() == 1);  // Only v1's binding
        
        t.pop();
    }
    
    // Test 27: occurs_check with all atoms (no vars anywhere)
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        expr c2{expr::functor{"cons", {&c1, &a3}}};
        
        assert(!bm.occurs_check(0, &c2));
        assert(!bm.occurs_check(999, &c2));
        assert(bm.bindings.size() == 0);
        
        t.pop();
    }
    
    // Test 28: occurs_check with var bound to var bound to cons containing target
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{145}};
        expr v2{expr::var{146}};
        expr v3{expr::var{147}};
        expr a1{expr::functor{"test", {}}};
        
        expr c1{expr::functor{"cons", {&v3, &a1}}};  // v3 unbound
        bm.bindings[145] = &v2;
        bm.bindings[146] = &c1;
        
        assert(!bm.occurs_check(145, &v1));  // After whnf, v1 -> c1
        assert(!bm.occurs_check(146, &v1));
        assert(bm.occurs_check(147, &v1));   // c1 contains v3
        assert(bm.bindings.size() == 2);  // Only the 2 explicit bindings
        
        t.pop();
    }
    
    // Test 29: Very long chain (10 levels) ending in unbound var
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v0{expr::var{200}};
        expr v1{expr::var{201}};
        expr v2{expr::var{202}};
        expr v3{expr::var{203}};
        expr v4{expr::var{204}};
        expr v5{expr::var{205}};
        expr v6{expr::var{206}};
        expr v7{expr::var{207}};
        expr v8{expr::var{208}};
        expr v9{expr::var{209}};
        
        // Chain: v0 -> v1 -> v2 -> ... -> v9 (unbound)
        bm.bindings[200] = &v1;
        bm.bindings[201] = &v2;
        bm.bindings[202] = &v3;
        bm.bindings[203] = &v4;
        bm.bindings[204] = &v5;
        bm.bindings[205] = &v6;
        bm.bindings[206] = &v7;
        bm.bindings[207] = &v8;
        bm.bindings[208] = &v9;
        
        assert(bm.occurs_check(209, &v0));  // v0 eventually points to v9
        assert(!bm.occurs_check(200, &v0)); // After path compression
        assert(!bm.occurs_check(205, &v0)); // Middle of chain
        assert(bm.bindings.size() == 9);  // Only the 9 explicit bindings
        
        t.pop();
    }
    
    // Test 30: Very long chain ending in atom
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v0{expr::var{210}};
        expr v1{expr::var{211}};
        expr v2{expr::var{212}};
        expr v3{expr::var{213}};
        expr v4{expr::var{214}};
        expr v5{expr::var{215}};
        expr v6{expr::var{216}};
        expr v7{expr::var{217}};
        expr a1{expr::functor{"end", {}}};
        
        bm.bindings[210] = &v1;
        bm.bindings[211] = &v2;
        bm.bindings[212] = &v3;
        bm.bindings[213] = &v4;
        bm.bindings[214] = &v5;
        bm.bindings[215] = &v6;
        bm.bindings[216] = &v7;
        bm.bindings[217] = &a1;
        
        assert(!bm.occurs_check(210, &v0));
        assert(!bm.occurs_check(217, &v0));
        assert(!bm.occurs_check(999, &v0));
        assert(bm.bindings.size() == 8);
        
        t.pop();
    }
    
    // Test 31: Deeply nested cons (5 levels) with var at bottom
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{220}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        expr a4{expr::functor{"d", {}}};
        
        // Build: cons(cons(cons(cons(cons(v1, a1), a2), a3), a4), a1)
        expr level1{expr::functor{"cons", {&v1, &a1}}};
        expr level2{expr::functor{"cons", {&level1, &a2}}};
        expr level3{expr::functor{"cons", {&level2, &a3}}};
        expr level4{expr::functor{"cons", {&level3, &a4}}};
        expr level5{expr::functor{"cons", {&level4, &a1}}};
        
        assert(bm.occurs_check(220, &level5));
        assert(!bm.occurs_check(221, &level5));
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 32: Deeply nested cons with var at different positions
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{225}};
        expr v2{expr::var{226}};
        expr v3{expr::var{227}};
        expr a1{expr::functor{"x", {}}};
        
        // Build: cons(cons(v1, v2), cons(a1, v3))
        expr left{expr::functor{"cons", {&v1, &v2}}};
        expr right{expr::functor{"cons", {&a1, &v3}}};
        expr outer{expr::functor{"cons", {&left, &right}}};
        
        assert(bm.occurs_check(225, &outer));  // v1 in left subtree
        assert(bm.occurs_check(226, &outer));  // v2 in left subtree
        assert(bm.occurs_check(227, &outer));  // v3 in right subtree
        assert(!bm.occurs_check(228, &outer));
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 33: Var bound to deeply nested cons containing bound vars
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_outer{expr::var{230}};
        expr v1{expr::var{231}};
        expr v2{expr::var{232}};
        expr v3{expr::var{233}};
        expr a1{expr::functor{"bound1", {}}};
        expr a2{expr::functor{"bound2", {}}};
        
        // Bind some vars to atoms
        bm.bindings[231] = &a1;
        bm.bindings[232] = &a2;
        
        // Build: cons(cons(v1, v2), cons(v3, a1))
        expr left{expr::functor{"cons", {&v1, &v2}}};
        expr right{expr::functor{"cons", {&v3, &a1}}};
        expr outer{expr::functor{"cons", {&left, &right}}};
        
        bm.bindings[230] = &outer;
        
        // v_outer -> outer, check what's in outer
        assert(!bm.occurs_check(230, &v_outer));  // v_outer doesn't contain itself
        assert(!bm.occurs_check(231, &v_outer));  // v1 reduces to atom
        assert(!bm.occurs_check(232, &v_outer));  // v2 reduces to atom
        assert(bm.occurs_check(233, &v_outer));   // v3 is unbound in outer
        assert(bm.bindings.size() == 3);  // Only the 3 explicit bindings
        
        t.pop();
    }
    
    // Test 34: Long chain to deeply nested cons with target var deep inside
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_chain1{expr::var{240}};
        expr v_chain2{expr::var{241}};
        expr v_chain3{expr::var{242}};
        expr v_target{expr::var{243}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Build nested cons: cons(cons(cons(v_target, a1), a2), a1)
        expr inner1{expr::functor{"cons", {&v_target, &a1}}};
        expr inner2{expr::functor{"cons", {&inner1, &a2}}};
        expr outer{expr::functor{"cons", {&inner2, &a1}}};
        
        // Chain: v_chain1 -> v_chain2 -> v_chain3 -> outer
        bm.bindings[240] = &v_chain2;
        bm.bindings[241] = &v_chain3;
        bm.bindings[242] = &outer;
        
        assert(bm.occurs_check(243, &v_chain1));  // Target is deep in the cons
        assert(!bm.occurs_check(240, &v_chain1)); // After path compression
        assert(!bm.occurs_check(241, &v_chain1));
        assert(!bm.occurs_check(242, &v_chain1));
        assert(bm.bindings.size() == 3);  // Only the 3 explicit bindings
        
        t.pop();
    }
    
    // Test 35: Cons with chains in both children
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_left1{expr::var{250}};
        expr v_left2{expr::var{251}};
        expr v_left3{expr::var{252}};
        expr v_right1{expr::var{253}};
        expr v_right2{expr::var{254}};
        expr v_right3{expr::var{255}};
        
        // Left chain: v_left1 -> v_left2 -> v_left3 (unbound)
        bm.bindings[250] = &v_left2;
        bm.bindings[251] = &v_left3;
        
        // Right chain: v_right1 -> v_right2 -> v_right3 (unbound)
        bm.bindings[253] = &v_right2;
        bm.bindings[254] = &v_right3;
        
        expr c1{expr::functor{"cons", {&v_left1, &v_right1}}};
        
        assert(bm.occurs_check(252, &c1));  // v_left3 via left child
        assert(bm.occurs_check(255, &c1));  // v_right3 via right child
        assert(!bm.occurs_check(250, &c1)); // After path compression
        assert(!bm.occurs_check(253, &c1)); // After path compression
        assert(bm.bindings.size() == 4);  // Only the 4 explicit bindings
        
        t.pop();
    }
    
    // Test 36: Very complex nested structure with multiple vars at different depths
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{260}};
        expr v2{expr::var{261}};
        expr v3{expr::var{262}};
        expr v4{expr::var{263}};
        expr v5{expr::var{264}};
        expr a1{expr::functor{"atom", {}}};
        
        // Build: cons(cons(v1, cons(v2, v3)), cons(cons(v4, v5), a1))
        expr inner_left_right{expr::functor{"cons", {&v2, &v3}}};
        expr inner_left{expr::functor{"cons", {&v1, &inner_left_right}}};
        expr inner_right_left{expr::functor{"cons", {&v4, &v5}}};
        expr inner_right{expr::functor{"cons", {&inner_right_left, &a1}}};
        expr outer{expr::functor{"cons", {&inner_left, &inner_right}}};
        
        assert(bm.occurs_check(260, &outer));  // v1 in left subtree
        assert(bm.occurs_check(261, &outer));  // v2 in left subtree, nested
        assert(bm.occurs_check(262, &outer));  // v3 in left subtree, nested
        assert(bm.occurs_check(263, &outer));  // v4 in right subtree, nested
        assert(bm.occurs_check(264, &outer));  // v5 in right subtree, nested
        assert(!bm.occurs_check(265, &outer));
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 37: Chain to cons, where cons children are also chains
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_outer{expr::var{270}};
        expr v_mid{expr::var{271}};
        
        expr v_left1{expr::var{272}};
        expr v_left2{expr::var{273}};
        expr a_left{expr::functor{"left_end", {}}};
        
        expr v_right1{expr::var{274}};
        expr v_right2{expr::var{275}};
        expr a_right{expr::functor{"right_end", {}}};
        
        // Left chain: v_left1 -> v_left2 -> a_left
        bm.bindings[272] = &v_left2;
        bm.bindings[273] = &a_left;
        
        // Right chain: v_right1 -> v_right2 -> a_right
        bm.bindings[274] = &v_right2;
        bm.bindings[275] = &a_right;
        
        expr c1{expr::functor{"cons", {&v_left1, &v_right1}}};
        
        // Outer chain: v_outer -> v_mid -> c1
        bm.bindings[270] = &v_mid;
        bm.bindings[271] = &c1;
        
        // After whnf, v_outer -> c1
        // c1's children will be reduced when occurs_check recurses
        assert(!bm.occurs_check(270, &v_outer));
        assert(!bm.occurs_check(271, &v_outer));
        assert(!bm.occurs_check(272, &v_outer));  // v_left1 reduces to atom
        assert(!bm.occurs_check(273, &v_outer));  // v_left2 reduces to atom
        assert(!bm.occurs_check(274, &v_outer));  // v_right1 reduces to atom
        assert(!bm.occurs_check(275, &v_outer));  // v_right2 reduces to atom
        assert(bm.bindings.size() == 6);  // All 6 explicit bindings
        
        t.pop();
    }
    
    // Test 38: Cons with one child being a long chain ending in target var
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{280}};
        expr v2{expr::var{281}};
        expr v3{expr::var{282}};
        expr v4{expr::var{283}};
        expr v5{expr::var{284}};
        expr a1{expr::functor{"other", {}}};
        
        // Chain: v1 -> v2 -> v3 -> v4 -> v5 (unbound)
        bm.bindings[280] = &v2;
        bm.bindings[281] = &v3;
        bm.bindings[282] = &v4;
        bm.bindings[283] = &v5;
        
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        assert(bm.occurs_check(284, &c1));  // v5 is at end of chain in lhs
        assert(!bm.occurs_check(280, &c1)); // After path compression
        assert(bm.bindings.size() == 4);  // Only the 4 explicit bindings
        
        t.pop();
    }
    
    // Test 39: Multiple nested cons with same var appearing in different positions
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{290}};
        expr v2{expr::var{290}};  // Same index as v1
        expr v3{expr::var{290}};  // Same index as v1
        expr a1{expr::functor{"x", {}}};
        
        // Build: cons(cons(v1, a1), cons(v2, v3))
        expr left{expr::functor{"cons", {&v1, &a1}}};
        expr right{expr::functor{"cons", {&v2, &v3}}};
        expr outer{expr::functor{"cons", {&left, &right}}};
        
        assert(bm.occurs_check(290, &outer));  // Var 290 appears 3 times
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 40: Stress test - very deep nesting (10 levels) with var at bottom
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{300}};
        expr a1{expr::functor{"a", {}}};
        
        // Build 10 levels of nesting
        expr* current = &v1;
        expr level1{expr::functor{"cons", {current, &a1}}};
        expr level2{expr::functor{"cons", {&level1, &a1}}};
        expr level3{expr::functor{"cons", {&level2, &a1}}};
        expr level4{expr::functor{"cons", {&level3, &a1}}};
        expr level5{expr::functor{"cons", {&level4, &a1}}};
        expr level6{expr::functor{"cons", {&level5, &a1}}};
        expr level7{expr::functor{"cons", {&level6, &a1}}};
        expr level8{expr::functor{"cons", {&level7, &a1}}};
        expr level9{expr::functor{"cons", {&level8, &a1}}};
        expr level10{expr::functor{"cons", {&level9, &a1}}};
        
        assert(bm.occurs_check(300, &level10));  // Should find v1 at the bottom
        assert(!bm.occurs_check(301, &level10));
        assert(bm.bindings.size() == 0);  // No explicit bindings
        
        t.pop();
    }
    
    // Test 41: ULTIMATE STRESS TEST - Long chain to deeply nested cons with chains inside
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        // Outer chain (5 levels): v0 -> v1 -> v2 -> v3 -> v4 -> nested_cons
        expr v0{expr::var{400}};
        expr v1{expr::var{401}};
        expr v2{expr::var{402}};
        expr v3{expr::var{403}};
        expr v4{expr::var{404}};
        
        // Left inner chain (3 levels): v_left1 -> v_left2 -> v_left3 (target)
        expr v_left1{expr::var{405}};
        expr v_left2{expr::var{406}};
        expr v_left3{expr::var{407}};
        bm.bindings[405] = &v_left2;
        bm.bindings[406] = &v_left3;
        
        // Right inner chain (3 levels): v_right1 -> v_right2 -> v_right3
        expr v_right1{expr::var{408}};
        expr v_right2{expr::var{409}};
        expr v_right3{expr::var{410}};
        bm.bindings[408] = &v_right2;
        bm.bindings[409] = &v_right3;
        
        // Build deeply nested cons (4 levels) with chains at the leaves
        // Structure: cons(cons(cons(cons(v_left1, v_right1), atom), atom), atom)
        expr a1{expr::functor{"a", {}}};
        expr level1{expr::functor{"cons", {&v_left1, &v_right1}}};  // Chains in both children
        expr level2{expr::functor{"cons", {&level1, &a1}}};
        expr level3{expr::functor{"cons", {&level2, &a1}}};
        expr level4{expr::functor{"cons", {&level3, &a1}}};
        
        // Setup outer chain
        bm.bindings[400] = &v1;
        bm.bindings[401] = &v2;
        bm.bindings[402] = &v3;
        bm.bindings[403] = &v4;
        bm.bindings[404] = &level4;
        
        // Test: v0 chains to level4, which contains level1, which has v_left1 in lhs
        // v_left1 chains to v_left3 (target)
        assert(bm.occurs_check(407, &v0));  // Should find v_left3 through all the indirection
        assert(bm.occurs_check(410, &v0));  // Should find v_right3 through all the indirection
        assert(!bm.occurs_check(400, &v0)); // After path compression
        assert(!bm.occurs_check(405, &v0)); // v_left1 chains to v_left3
        assert(!bm.occurs_check(408, &v0)); // v_right1 chains to v_right3
        assert(bm.bindings.size() == 9);  // Only the 9 explicit bindings
        
        t.pop();
    }
    
    // Test 42: Nested cons where EVERY node has a chain
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        // Build: cons(cons(chain_a, chain_b), cons(chain_c, chain_d))
        // where each chain is 2 levels deep
        
        // Chain A: va1 -> va2 (unbound)
        expr va1{expr::var{500}};
        expr va2{expr::var{501}};
        bm.bindings[500] = &va2;
        
        // Chain B: vb1 -> vb2 (unbound)
        expr vb1{expr::var{502}};
        expr vb2{expr::var{503}};
        bm.bindings[502] = &vb2;
        
        // Chain C: vc1 -> vc2 (unbound)
        expr vc1{expr::var{504}};
        expr vc2{expr::var{505}};
        bm.bindings[504] = &vc2;
        
        // Chain D: vd1 -> vd2 (unbound)
        expr vd1{expr::var{506}};
        expr vd2{expr::var{507}};
        bm.bindings[506] = &vd2;
        
        expr left{expr::functor{"cons", {&va1, &vb1}}};
        expr right{expr::functor{"cons", {&vc1, &vd1}}};
        expr outer{expr::functor{"cons", {&left, &right}}};
        
        // All four target vars should be found
        assert(bm.occurs_check(501, &outer));  // va2
        assert(bm.occurs_check(503, &outer));  // vb2
        assert(bm.occurs_check(505, &outer));  // vc2
        assert(bm.occurs_check(507, &outer));  // vd2
        
        // None of the chain heads should be found (after compression)
        assert(!bm.occurs_check(500, &outer));
        assert(!bm.occurs_check(502, &outer));
        assert(!bm.occurs_check(504, &outer));
        assert(!bm.occurs_check(506, &outer));
        
        // Path compression updates values but doesn't change count
        assert(bm.bindings.size() == 4);  // Still 4 entries
        
        t.pop();
    }
    
    // Test 43: Chain to nested cons, where nested cons contains chains to more nested cons
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        // Outer chain: v0 -> v1 -> outer_cons
        expr v0{expr::var{600}};
        expr v1{expr::var{601}};
        
        // Inner left chain: v_left -> inner_left_cons
        expr v_left{expr::var{602}};
        
        // Inner right chain: v_right -> inner_right_cons
        expr v_right{expr::var{603}};
        
        // Target var deep inside
        expr v_target{expr::var{604}};
        expr a1{expr::functor{"a", {}}};
        
        // Build: outer_cons = cons(v_left, v_right)
        //        v_left -> inner_left_cons = cons(v_target, a1)
        //        v_right -> inner_right_cons = cons(a1, a1)
        expr inner_left_cons{expr::functor{"cons", {&v_target, &a1}}};
        expr inner_right_cons{expr::functor{"cons", {&a1, &a1}}};
        
        bm.bindings[602] = &inner_left_cons;
        bm.bindings[603] = &inner_right_cons;
        
        expr outer_cons{expr::functor{"cons", {&v_left, &v_right}}};
        
        bm.bindings[600] = &v1;
        bm.bindings[601] = &outer_cons;
        
        // v0 -> v1 -> outer_cons
        // outer_cons.args[0] = v_left -> inner_left_cons
        // inner_left_cons.args[0] = v_target
        assert(bm.occurs_check(604, &v0));  // Should find v_target through all the indirection
        assert(!bm.occurs_check(600, &v0)); // After path compression
        assert(!bm.occurs_check(602, &v0)); // v_left chains to cons (no var 602 in result)
        assert(!bm.occurs_check(603, &v0)); // v_right chains to cons (no var 603 in result)
        assert(bm.bindings.size() == 4);  // Path compression doesn't change count
        
        t.pop();
    }
    
    // Test 44: Cons where both children are bound to the SAME atom
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{700}};
        expr v2{expr::var{701}};
        expr a1{expr::functor{"shared", {}}};
        
        // Both vars bound to the same atom
        bm.bindings[700] = &a1;
        bm.bindings[701] = &a1;
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Both children reduce to the same atom, so no vars should be found
        assert(!bm.occurs_check(700, &c1));  // v1 reduces to atom
        assert(!bm.occurs_check(701, &c1));  // v2 reduces to atom
        assert(!bm.occurs_check(702, &c1));  // Unrelated var
        assert(bm.bindings.size() == 2);
        
        t.pop();
    }
    
    // Test 45: Cons where both children are bound to the SAME var
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{710}};
        expr v2{expr::var{711}};
        expr v_target{expr::var{712}};
        
        // Both vars bound to the same target var (unbound)
        bm.bindings[710] = &v_target;
        bm.bindings[711] = &v_target;
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Both children reduce to v_target, so searching for v_target should find it
        assert(bm.occurs_check(712, &c1));   // v_target found in both children
        assert(!bm.occurs_check(710, &c1));  // v1 reduces to v_target
        assert(!bm.occurs_check(711, &c1));  // v2 reduces to v_target
        assert(bm.bindings.size() == 2);
        
        t.pop();
    }
    
    // Test 46: Cons where both children are bound to the SAME cons
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v1{expr::var{720}};
        expr v2{expr::var{721}};
        expr v_inner{expr::var{722}};
        expr a1{expr::functor{"x", {}}};
        expr shared_cons{expr::functor{"cons", {&v_inner, &a1}}};
        
        // Both vars bound to the same cons
        bm.bindings[720] = &shared_cons;
        bm.bindings[721] = &shared_cons;
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Both children reduce to shared_cons which contains v_inner
        assert(bm.occurs_check(722, &c1));   // v_inner found in shared_cons
        assert(!bm.occurs_check(720, &c1));  // v1 reduces to cons
        assert(!bm.occurs_check(721, &c1));  // v2 reduces to cons
        assert(bm.bindings.size() == 2);
        
        t.pop();
    }
    
    // Test 47: Cons where both children are chains that converge to the SAME var
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_left1{expr::var{730}};
        expr v_left2{expr::var{731}};
        expr v_right1{expr::var{732}};
        expr v_right2{expr::var{733}};
        expr v_target{expr::var{734}};
        
        // Left chain: v_left1 -> v_left2 -> v_target
        bm.bindings[730] = &v_left2;
        bm.bindings[731] = &v_target;
        
        // Right chain: v_right1 -> v_right2 -> v_target (same target!)
        bm.bindings[732] = &v_right2;
        bm.bindings[733] = &v_target;
        
        expr c1{expr::functor{"cons", {&v_left1, &v_right1}}};
        
        // Both chains converge to v_target
        assert(bm.occurs_check(734, &c1));    // v_target found via both chains
        assert(!bm.occurs_check(730, &c1));   // v_left1 compressed away
        assert(!bm.occurs_check(731, &c1));   // v_left2 compressed away
        assert(!bm.occurs_check(732, &c1));   // v_right1 compressed away
        assert(!bm.occurs_check(733, &c1));   // v_right2 compressed away
        assert(bm.bindings.size() == 4);
        
        t.pop();
    }
    
    // Test 48: Cons where both children are chains that converge to the SAME atom
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_left1{expr::var{740}};
        expr v_left2{expr::var{741}};
        expr v_right1{expr::var{742}};
        expr v_right2{expr::var{743}};
        expr a_target{expr::functor{"convergence", {}}};
        
        // Left chain: v_left1 -> v_left2 -> a_target
        bm.bindings[740] = &v_left2;
        bm.bindings[741] = &a_target;
        
        // Right chain: v_right1 -> v_right2 -> a_target (same target!)
        bm.bindings[742] = &v_right2;
        bm.bindings[743] = &a_target;
        
        expr c1{expr::functor{"cons", {&v_left1, &v_right1}}};
        
        // Both chains converge to atom, so no vars should be found
        assert(!bm.occurs_check(740, &c1));   // v_left1 compressed away
        assert(!bm.occurs_check(741, &c1));   // v_left2 compressed away
        assert(!bm.occurs_check(742, &c1));   // v_right1 compressed away
        assert(!bm.occurs_check(743, &c1));   // v_right2 compressed away
        assert(!bm.occurs_check(999, &c1));   // No vars in result
        assert(bm.bindings.size() == 4);
        
        t.pop();
    }
    
    // Test 49: Cons where both children are chains that converge to the SAME cons
    {
        trail t;
        bind_map bm(t);
        t.push();
        
        expr v_left1{expr::var{750}};
        expr v_left2{expr::var{751}};
        expr v_right1{expr::var{752}};
        expr v_right2{expr::var{753}};
        expr v_inner{expr::var{754}};
        expr a1{expr::functor{"y", {}}};
        expr target_cons{expr::functor{"cons", {&v_inner, &a1}}};
        
        // Left chain: v_left1 -> v_left2 -> target_cons
        bm.bindings[750] = &v_left2;
        bm.bindings[751] = &target_cons;
        
        // Right chain: v_right1 -> v_right2 -> target_cons (same target!)
        bm.bindings[752] = &v_right2;
        bm.bindings[753] = &target_cons;
        
        expr c1{expr::functor{"cons", {&v_left1, &v_right1}}};
        
        // Both chains converge to target_cons which contains v_inner
        assert(bm.occurs_check(754, &c1));    // v_inner found in target_cons
        assert(!bm.occurs_check(750, &c1));   // v_left1 compressed away
        assert(!bm.occurs_check(751, &c1));   // v_left2 compressed away
        assert(!bm.occurs_check(752, &c1));   // v_right1 compressed away
        assert(!bm.occurs_check(753, &c1));   // v_right2 compressed away
        assert(bm.bindings.size() == 4);
        
        t.pop();
    }
}

void test_bind_map_unify() {
    // unify() performs Prolog-like unification with occurs check
    // It's a symmetric operation: unify(a, b) should behave like unify(b, a)
    // Tests verify symmetry by commuting arguments between consecutive test cases
    
    // ========== BASIC CASES ==========
    
    // Test 1: Identical expressions (same pointer) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"test", {}}};
        assert(bm.unify(&a1, &a1));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 2: Two different atoms - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"foo", {}}};
        expr a2{expr::functor{"bar", {}}};
        assert(!bm.unify(&a1, &a2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 3: Two different atoms (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"foo", {}}};
        expr a2{expr::functor{"bar", {}}};
        assert(!bm.unify(&a2, &a1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 4: Two identical atoms (different objects) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"same", {}}};
        expr a2{expr::functor{"same", {}}};
        assert(bm.unify(&a1, &a2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 5: Two identical atoms (commuted) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"same", {}}};
        expr a2{expr::functor{"same", {}}};
        assert(bm.unify(&a2, &a1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 6: Unbound var with atom - should succeed and create binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{0}};
        expr a1{expr::functor{"bound", {}}};
        assert(bm.unify(&v1, &a1));
        assert(bm.bindings.size() == 1);  // Just v1 -> a1
        assert(bm.bindings.count(0) == 1);
        assert(bm.whnf(&v1) == &a1);

        t.pop();
    }
    
    // Test 7: Atom with unbound var (commuted) - should succeed and create binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{1}};
        expr a1{expr::functor{"bound", {}}};
        assert(bm.unify(&a1, &v1));  // Commuted
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(1) == 1);
        assert(bm.whnf(&v1) == &a1);

        t.pop();
    }
    
    // Test 8: Unbound var with cons - should succeed and create binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{2}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        assert(bm.unify(&v1, &c1));
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(2) == 1);  // v1 is bound
        assert(bm.whnf(&v1) == &c1);

        t.pop();
    }
    
    // Test 9: Cons with unbound var (commuted) - should succeed and create binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{3}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        assert(bm.unify(&c1, &v1));  // Commuted
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(3) == 1);  // v1 is bound
        assert(bm.whnf(&v1) == &c1);

        t.pop();
    }
    
    // Test 10: Two unbound vars - should succeed and create binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{4}};
        expr v2{expr::var{5}};
        assert(bm.unify(&v1, &v2));
        assert(bm.bindings.size() == 1);  // One var bound to the other
        // Verify one of them is bound (either 4->v2 or 5->v1)
        assert(bm.bindings.count(4) == 1 || bm.bindings.count(5) == 1);
        // Both should reduce to the same thing
        const expr* result1 = bm.whnf(&v1);
        const expr* result2 = bm.whnf(&v2);
        assert(result1 == result2);
        assert(result1 == &v1 || result1 == &v2);  // Result is one of the vars
        t.pop();
    }
    
    // Test 11: Two unbound vars (commuted) - should succeed and create binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{6}};
        expr v2{expr::var{7}};
        assert(bm.unify(&v2, &v1));  // Commuted
        assert(bm.bindings.size() == 1);
        // Verify one of them is bound (either 6->v2 or 7->v1)
        assert(bm.bindings.count(6) == 1 || bm.bindings.count(7) == 1);
        // Both should reduce to the same thing
        const expr* result1 = bm.whnf(&v1);
        const expr* result2 = bm.whnf(&v2);
        assert(result1 == result2);
        assert(result1 == &v1 || result1 == &v2);  // Result is one of the vars

        t.pop();
    }
    
    // ========== OCCURS CHECK CASES ==========
    
    // Test 12: Var with cons containing that var - should fail (occurs check)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{10}};
        expr v2{expr::var{10}};  // Same index
        expr a1{expr::functor{"test", {}}};
        expr c1{expr::functor{"cons", {&v2, &a1}}};
        assert(!bm.unify(&v1, &c1));  // Should fail occurs check
        assert(bm.bindings.size() == 0);  // No binding created (occurs check before bind)

        t.pop();
    }
    
    // Test 13: Cons containing var with that var (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{11}};
        expr v2{expr::var{11}};
        expr a1{expr::functor{"test", {}}};
        expr c1{expr::functor{"cons", {&v2, &a1}}};
        assert(!bm.unify(&c1, &v1));  // Commuted
        assert(bm.bindings.size() == 0);  // No binding created (occurs check before bind)

        t.pop();
    }
    
    // Test 14: Var with cons containing chain to that var - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{12}};
        expr v2{expr::var{13}};
        expr v3{expr::var{12}};  // Same as v1
        expr a1{expr::functor{"test", {}}};
        
        // v2 -> v3 (which is same index as v1)
        bm.bindings[13] = &v3;
        
        expr c1{expr::functor{"cons", {&v2, &a1}}};
        assert(!bm.unify(&v1, &c1));  // Should fail: v1 with cons containing chain to v1
        assert(bm.bindings.size() == 1);  // Only original binding (v2 -> v3), no new binding

        t.pop();
    }
    
    // Test 15: Cons with chain to var, unified with that var (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{14}};
        expr v2{expr::var{15}};
        expr v3{expr::var{14}};
        expr a1{expr::functor{"test", {}}};
        
        bm.bindings[15] = &v3;
        
        expr c1{expr::functor{"cons", {&v2, &a1}}};
        assert(!bm.unify(&c1, &v1));  // Commuted
        assert(bm.bindings.size() == 1);  // Only original binding (v2 -> v3), no new binding

        t.pop();
    }
    
    // Test 16: Var with deeply nested cons containing that var - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{16}};
        expr v2{expr::var{16}};
        expr a1{expr::functor{"a", {}}};
        
        // Build: cons(cons(cons(v2, a1), a1), a1)
        expr inner1{expr::functor{"cons", {&v2, &a1}}};
        expr inner2{expr::functor{"cons", {&inner1, &a1}}};
        expr outer{expr::functor{"cons", {&inner2, &a1}}};
        
        assert(!bm.unify(&v1, &outer));  // Should fail
        assert(bm.bindings.size() == 0);  // No binding created (occurs check before bind)

        t.pop();
    }
    
    // Test 17: Deeply nested cons with var, unified with that var (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{17}};
        expr v2{expr::var{17}};
        expr a1{expr::functor{"a", {}}};
        
        expr inner1{expr::functor{"cons", {&v2, &a1}}};
        expr inner2{expr::functor{"cons", {&inner1, &a1}}};
        expr outer{expr::functor{"cons", {&inner2, &a1}}};
        
        assert(!bm.unify(&outer, &v1));  // Commuted
        assert(bm.bindings.size() == 0);  // No binding created (occurs check before bind)

        t.pop();
    }
    
    // ========== CHAIN CASES ==========
    
    // Test 18: Var bound to another var, unify with atom - should follow chain
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{20}};
        expr v2{expr::var{21}};
        expr a1{expr::functor{"target", {}}};
        
        // v1 -> v2 (unbound)
        bm.bindings[20] = &v2;
        
        assert(bm.unify(&v1, &a1));  // Should bind v2 to a1
        assert(bm.bindings.size() == 2);  // Original chain + new binding (v2 -> a1)
        assert(bm.bindings.count(21) == 1);  // v2 is now bound
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);

        t.pop();
    }
    
    // Test 19: Atom with var bound to another var (commuted) - should follow chain
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{22}};
        expr v2{expr::var{23}};
        expr a1{expr::functor{"target", {}}};
        
        bm.bindings[22] = &v2;
        
        assert(bm.unify(&a1, &v1));  // Commuted
        assert(bm.bindings.size() == 2);  // Original chain + new binding (v2 -> a1)
        assert(bm.bindings.count(23) == 1);  // v2 is now bound
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);

        t.pop();
    }
    
    // Test 20: Two vars in a chain, unify them - should succeed (already unified)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{24}};
        expr v2{expr::var{25}};
        
        // v1 -> v2
        bm.bindings[24] = &v2;
        
        assert(bm.unify(&v1, &v2));  // Both reduce to v2, already unified
        assert(bm.bindings.size() == 1);  // No new binding created
        assert(bm.whnf(&v1) == &v2);
        assert(bm.whnf(&v2) == &v2);

        t.pop();
    }
    
    // Test 21: Two vars in a chain, unify them (commuted) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{26}};
        expr v2{expr::var{27}};
        
        bm.bindings[26] = &v2;
        
        assert(bm.unify(&v2, &v1));  // Commuted
        assert(bm.bindings.size() == 1);  // No new binding created
        assert(bm.whnf(&v1) == &v2);
        assert(bm.whnf(&v2) == &v2);

        t.pop();
    }
    
    // Test 22: Long chain of vars, unify head with atom
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{28}};
        expr v2{expr::var{29}};
        expr v3{expr::var{30}};
        expr v4{expr::var{31}};
        expr a1{expr::functor{"end", {}}};
        
        // Chain: v1 -> v2 -> v3 -> v4 (unbound)
        bm.bindings[28] = &v2;
        bm.bindings[29] = &v3;
        bm.bindings[30] = &v4;
        
        assert(bm.unify(&v1, &a1));  // Should bind v4 to a1
        assert(bm.bindings.size() == 4);  // 3 original chain + 1 new binding (v4 -> a1)
        assert(bm.bindings.count(31) == 1);  // v4 is now bound
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v4) == &a1);

        t.pop();
    }
    
    // Test 23: Atom with long chain of vars (commuted)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{32}};
        expr v2{expr::var{33}};
        expr v3{expr::var{34}};
        expr v4{expr::var{35}};
        expr a1{expr::functor{"end", {}}};
        
        bm.bindings[32] = &v2;
        bm.bindings[33] = &v3;
        bm.bindings[34] = &v4;
        
        assert(bm.unify(&a1, &v1));  // Commuted
        assert(bm.bindings.size() == 4);  // 3 original chain + 1 new binding (v4 -> a1)
        assert(bm.bindings.count(35) == 1);  // v4 is now bound
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v4) == &a1);

        t.pop();
    }
    
    // ========== CONS UNIFICATION CASES ==========
    
    // Test 24: Two cons with same atoms - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"x", {}}};
        expr a4{expr::functor{"y", {}}};
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 25: Two cons with same atoms (commuted) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"x", {}}};
        expr a4{expr::functor{"y", {}}};
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        assert(bm.unify(&c2, &c1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 26: Two cons with different atoms in lhs - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"z", {}}};  // Different
        expr a4{expr::functor{"y", {}}};
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // No bindings created (all atoms)

        t.pop();
    }
    
    // Test 27: Two cons with different atoms in rhs - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"x", {}}};
        expr a4{expr::functor{"z", {}}};  // Different
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // No bindings created (all atoms)

        t.pop();
    }
    
    // Test 28: Cons with vars that can be unified - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{40}};
        expr a1{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr c2{expr::functor{"cons", {&a2, &a3}}};
        
        assert(bm.unify(&c1, &c2));  // Should bind v1 to a2
        assert(bm.bindings.size() == 1);  // One binding created
        assert(bm.bindings.count(40) == 1);  // v1 is bound
        assert(bm.whnf(&v1) == &a2);

        t.pop();
    }
    
    // Test 29: Cons with vars that can be unified (commuted) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{41}};
        expr a1{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr c2{expr::functor{"cons", {&a2, &a3}}};
        
        assert(bm.unify(&c2, &c1));  // Commuted
        assert(bm.bindings.size() == 1);  // One binding created
        assert(bm.bindings.count(41) == 1);  // v1 is bound
        assert(bm.whnf(&v1) == &a2);

        t.pop();
    }
    
    // Test 30: Cons with vars that cannot be unified (conflicting) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{42}};
        expr a1{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"z", {}}};  // Conflicts with a1
        expr c2{expr::functor{"cons", {&a2, &a3}}};
        
        assert(!bm.unify(&c1, &c2));  // rhs children don't match
        // Partial binding left: v1 was bound to a2 before rhs failed
        assert(bm.bindings.size() == 1);
        assert(bm.whnf(&v1) == &a2);

        t.pop();
    }
    
    // Test 31: Cons with vars that cannot be unified (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{43}};
        expr a1{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"z", {}}};
        expr c2{expr::functor{"cons", {&a2, &a3}}};
        
        assert(!bm.unify(&c2, &c1));  // Commuted
        // Partial binding left: v1 was bound to a2 before rhs failed
        assert(bm.bindings.size() == 1);
        assert(bm.whnf(&v1) == &a2);

        t.pop();
    }
    
    // Test 32: Nested cons structures - should recursively unify
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr inner1{expr::functor{"cons", {&a1, &a2}}};
        expr a3{expr::functor{"c", {}}};
        expr outer1{expr::functor{"cons", {&inner1, &a3}}};
        
        expr a4{expr::functor{"a", {}}};
        expr a5{expr::functor{"b", {}}};
        expr inner2{expr::functor{"cons", {&a4, &a5}}};
        expr a6{expr::functor{"c", {}}};
        expr outer2{expr::functor{"cons", {&inner2, &a6}}};
        
        assert(bm.unify(&outer1, &outer2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 33: Nested cons structures (commuted) - should recursively unify
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr inner1{expr::functor{"cons", {&a1, &a2}}};
        expr a3{expr::functor{"c", {}}};
        expr outer1{expr::functor{"cons", {&inner1, &a3}}};
        
        expr a4{expr::functor{"a", {}}};
        expr a5{expr::functor{"b", {}}};
        expr inner2{expr::functor{"cons", {&a4, &a5}}};
        expr a6{expr::functor{"c", {}}};
        expr outer2{expr::functor{"cons", {&inner2, &a6}}};
        
        assert(bm.unify(&outer2, &outer1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // ========== TYPE MISMATCH CASES ==========
    
    // Test 34: Atom with cons - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"test", {}}};
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a2, &a3}}};
        
        assert(!bm.unify(&a1, &c1));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 35: Cons with atom (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"test", {}}};
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a2, &a3}}};
        
        assert(!bm.unify(&c1, &a1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // ========== COMPLEX CASES ==========
    
    // Test 36: Unify two cons where children need transitive unification
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{50}};
        expr a1{expr::functor{"a", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr v2{expr::var{51}};
        expr a2{expr::functor{"a", {}}};
        expr c2{expr::functor{"cons", {&v2, &a2}}};
        
        assert(bm.unify(&c1, &c2));  // Should bind v1 to v2
        assert(bm.bindings.size() == 1);  // Verify binding was created
        assert(bm.bindings.count(50) == 1 || bm.bindings.count(51) == 1);
        assert(bm.whnf(&v1) == bm.whnf(&v2));
        // Both should reduce to one of the vars
        const expr* result = bm.whnf(&v1);
        assert(result == &v1 || result == &v2);

        t.pop();
    }
    
    // Test 37: Transitive unification (commuted)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{52}};
        expr a1{expr::functor{"a", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr v2{expr::var{53}};
        expr a2{expr::functor{"a", {}}};
        expr c2{expr::functor{"cons", {&v2, &a2}}};
        
        assert(bm.unify(&c2, &c1));  // Commuted
        assert(bm.bindings.size() == 1);  // Verify binding was created
        assert(bm.bindings.count(52) == 1 || bm.bindings.count(53) == 1);
        assert(bm.whnf(&v1) == bm.whnf(&v2));
        // Both should reduce to one of the vars
        const expr* result = bm.whnf(&v1);
        assert(result == &v1 || result == &v2);

        t.pop();
    }
    
    // Test 38: Unify cons with nested vars - cons(v1, cons(v2, a)) with cons(a, cons(b, a))
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{54}};
        expr v2{expr::var{55}};
        expr a1{expr::functor{"a", {}}};
        expr inner1{expr::functor{"cons", {&v2, &a1}}};
        expr c1{expr::functor{"cons", {&v1, &inner1}}};
        
        expr a2{expr::functor{"a", {}}};
        expr a3{expr::functor{"b", {}}};
        expr a4{expr::functor{"a", {}}};
        expr inner2{expr::functor{"cons", {&a3, &a4}}};
        expr c2{expr::functor{"cons", {&a2, &inner2}}};
        
        assert(bm.unify(&c1, &c2));  // v1 -> a2, v2 -> a3
        assert(bm.bindings.size() == 2);  // Two bindings created
        assert(bm.bindings.count(54) == 1);  // v1 is bound
        assert(bm.bindings.count(55) == 1);  // v2 is bound
        assert(bm.whnf(&v1) == &a2);
        assert(bm.whnf(&v2) == &a3);

        t.pop();
    }
    
    // Test 39: Nested vars (commuted)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{56}};
        expr v2{expr::var{57}};
        expr a1{expr::functor{"a", {}}};
        expr inner1{expr::functor{"cons", {&v2, &a1}}};
        expr c1{expr::functor{"cons", {&v1, &inner1}}};
        
        expr a2{expr::functor{"a", {}}};
        expr a3{expr::functor{"b", {}}};
        expr a4{expr::functor{"a", {}}};
        expr inner2{expr::functor{"cons", {&a3, &a4}}};
        expr c2{expr::functor{"cons", {&a2, &inner2}}};
        
        assert(bm.unify(&c2, &c1));  // Commuted
        assert(bm.bindings.size() == 2);  // Two bindings created
        assert(bm.bindings.count(56) == 1);  // v1 is bound
        assert(bm.bindings.count(57) == 1);  // v2 is bound
        assert(bm.whnf(&v1) == &a2);
        assert(bm.whnf(&v2) == &a3);

        t.pop();
    }
    
    // Test 40: Multiple unifications building up bindings
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{60}};
        expr v2{expr::var{61}};
        expr v3{expr::var{62}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr a3{expr::functor{"z", {}}};
        
        // First unification: v1 with a1
        assert(bm.unify(&v1, &a1));
        assert(bm.bindings.size() == 1);
        assert(bm.whnf(&v1) == &a1);
        
        // Second unification: v2 with a2
        assert(bm.unify(&v2, &a2));
        assert(bm.bindings.size() == 2);
        assert(bm.whnf(&v2) == &a2);
        
        // Third unification: v3 with a3
        assert(bm.unify(&v3, &a3));
        assert(bm.bindings.size() == 3);
        assert(bm.whnf(&v3) == &a3);
        
        // All bindings should persist
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a2);
        assert(bm.whnf(&v3) == &a3);

        t.pop();
    }
    
    // Test 41: Unify after previous unification - bindings persist
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{63}};
        expr v2{expr::var{64}};
        expr a1{expr::functor{"first", {}}};
        
        // First: unify v1 with v2
        assert(bm.unify(&v1, &v2));
        assert(bm.bindings.size() == 1);  // One binding created
        
        // Second: unify v1 with a1 (should bind v2 to a1 via v1)
        assert(bm.unify(&v1, &a1));
        assert(bm.bindings.size() == 2);  // Two bindings total
        
        // Both should now point to a1
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);

        t.pop();
    }
    
    // Test 42: Self-unification of var - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{65}};
        assert(bm.unify(&v1, &v1));  // Same pointer
        assert(bm.bindings.size() == 0);  // No binding created

        t.pop();
    }
    
    // Test 43: Unify bound var with its binding - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{66}};
        expr a1{expr::functor{"bound", {}}};
        
        bm.bindings[66] = &a1;
        
        assert(bm.unify(&v1, &a1));  // v1 reduces to a1, unifying a1 with a1
        assert(bm.bindings.size() == 1);  // No new binding, still just the original

        t.pop();
    }
    
    // Test 44: Unify two bound vars bound to same thing - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{67}};
        expr v2{expr::var{68}};
        expr a1{expr::functor{"same", {}}};
        
        bm.bindings[67] = &a1;
        bm.bindings[68] = &a1;
        
        assert(bm.unify(&v1, &v2));  // Both reduce to a1
        assert(bm.bindings.size() == 2);  // No new binding, still just the two original

        t.pop();
    }
    
    // Test 45: Unify two bound vars bound to different atoms - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{69}};
        expr v2{expr::var{70}};
        expr a1{expr::functor{"first", {}}};
        expr a2{expr::functor{"second", {}}};
        
        bm.bindings[69] = &a1;
        bm.bindings[70] = &a2;
        
        assert(!bm.unify(&v1, &v2));  // Reduce to different atoms
        assert(bm.bindings.size() == 2);  // No new binding, still just the two original

        t.pop();
    }
    
    // Test 46: Deeply nested cons unification with vars at various levels
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{71}};
        expr v2{expr::var{72}};
        expr v3{expr::var{73}};
        expr a1{expr::functor{"a", {}}};
        
        // Build: cons(cons(v1, v2), cons(v3, a1))
        expr left1{expr::functor{"cons", {&v1, &v2}}};
        expr right1{expr::functor{"cons", {&v3, &a1}}};
        expr outer1{expr::functor{"cons", {&left1, &right1}}};
        
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr a4{expr::functor{"z", {}}};
        expr a5{expr::functor{"a", {}}};
        
        // Build: cons(cons(a2, a3), cons(a4, a5))
        expr left2{expr::functor{"cons", {&a2, &a3}}};
        expr right2{expr::functor{"cons", {&a4, &a5}}};
        expr outer2{expr::functor{"cons", {&left2, &right2}}};
        
        assert(bm.unify(&outer1, &outer2));
        assert(bm.bindings.size() == 3);  // Three bindings: v1->a2, v2->a3, v3->a4
        assert(bm.bindings.count(71) == 1);  // v1 is bound
        assert(bm.bindings.count(72) == 1);  // v2 is bound
        assert(bm.bindings.count(73) == 1);  // v3 is bound
        assert(bm.whnf(&v1) == &a2);
        assert(bm.whnf(&v2) == &a3);
        assert(bm.whnf(&v3) == &a4);

        t.pop();
    }
    
    // Test 47: Unify with chains on both sides
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{74}};
        expr v2{expr::var{75}};
        expr v3{expr::var{76}};
        expr v4{expr::var{77}};
        
        // Left chain: v1 -> v2 (unbound)
        bm.bindings[74] = &v2;
        
        // Right chain: v3 -> v4 (unbound)
        bm.bindings[76] = &v4;
        
        assert(bm.unify(&v1, &v3));  // Should unify v2 with v4
        assert(bm.bindings.size() == 3);  // Original 2 chains + 1 new binding (v2 <-> v4)
        assert(bm.whnf(&v1) == bm.whnf(&v3));
        assert(bm.whnf(&v2) == bm.whnf(&v4));
        // All four vars should reduce to the same final target
        const expr* result = bm.whnf(&v1);
        assert(result == &v2 || result == &v4);  // Should be one of the tail vars

        t.pop();
    }
    
    // Test 48: Cons unification where lhs succeeds but rhs fails
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"x", {}}};
        expr a4{expr::functor{"z", {}}};  // Different from a2
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        assert(!bm.unify(&c1, &c2));  // lhs matches, rhs doesn't
        assert(bm.bindings.size() == 0);  // No bindings created (all atoms)

        t.pop();
    }
    
    // Test 49: Cons unification where first child fails (short circuit)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&a1, &a2}}};
        
        expr a3{expr::functor{"z", {}}};  // Different from a1
        expr a4{expr::functor{"y", {}}};
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        assert(!bm.unify(&c1, &c2));  // Should fail on lhs
        assert(bm.bindings.size() == 0);  // No bindings created (all atoms)

        t.pop();
    }
    
    // Test 50: Very complex nested unification
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{80}};
        expr v2{expr::var{81}};
        
        // Build complex structure with vars
        expr a1{expr::functor{"a", {}}};
        expr inner1{expr::functor{"cons", {&v1, &a1}}};
        expr inner2{expr::functor{"cons", {&inner1, &v2}}};
        expr outer1{expr::functor{"cons", {&inner2, &a1}}};
        
        // Build matching structure with atoms
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"a", {}}};
        expr inner3{expr::functor{"cons", {&a2, &a3}}};
        expr a4{expr::functor{"y", {}}};
        expr inner4{expr::functor{"cons", {&inner3, &a4}}};
        expr a5{expr::functor{"a", {}}};
        expr outer2{expr::functor{"cons", {&inner4, &a5}}};
        
        assert(bm.unify(&outer1, &outer2));
        assert(bm.bindings.size() == 2);  // Two bindings: v1->a2, v2->a4
        assert(bm.bindings.count(80) == 1);  // v1 is bound
        assert(bm.bindings.count(81) == 1);  // v2 is bound
        assert(bm.whnf(&v1) == &a2);
        assert(bm.whnf(&v2) == &a4);

        t.pop();
    }
    
    // Test 51: Unification failure with trail - partial bindings should be undone
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{82}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        expr a3{expr::functor{"z", {}}};
        expr a4{expr::functor{"w", {}}};  // Different from a1
        expr c2{expr::functor{"cons", {&a3, &a4}}};
        
        // Push frame before unification
        t.push();
        
        // This should fail (rhs mismatch), leaving partial binding v1->a3
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Partial binding left
        assert(bm.bindings.count(82) == 1);
        assert(bm.whnf(&v1) == &a3);
        
        // Pop frame - partial bindings should be undone
        t.pop();
        assert(bm.bindings.size() == 0);  // All bindings undone
        assert(bm.bindings.count(82) == 0);  // v1 is unbound again
        assert(bm.whnf(&v1) == &v1);  // v1 reduces to itself
    }
    
    // Test 52: Multiple unifications with shared vars - transitive constraints
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{83}};
        expr v2{expr::var{84}};
        expr v3{expr::var{85}};
        
        // First structure: cons(V1, V1) - both children are same var
        expr c1{expr::functor{"cons", {&v1, &v1}}};
        
        // Second structure: cons(a, V2)
        expr a1{expr::functor{"a", {}}};
        expr c2{expr::functor{"cons", {&a1, &v2}}};
        
        // Unify cons(V1, V1) with cons(a, V2)
        // This should bind V1 to 'a' and V2 to 'a'
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // V1->a and V2->a (or V2->V1)
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);
        
        // Now try to unify cons(V1, V1) with cons(V3, k)
        expr a2{expr::functor{"k", {}}};
        expr c3{expr::functor{"cons", {&v3, &a2}}};
        
        // This should fail because V1 reduces to 'a', so we're trying to unify
        // cons(a, a) with cons(V3, k), which would bind V3 to 'a', but then
        // the rhs would try to unify 'a' with 'k', which fails
        assert(!bm.unify(&c1, &c3));
        // Partial binding: V3 was bound to 'a' before rhs failed
        assert(bm.bindings.size() == 3);  // Original 2 + V3->a
        assert(bm.bindings.count(85) == 1);
        assert(bm.whnf(&v3) == &a1);
        
        t.pop();
    }
    
    // Test 53: Chained structural unifications - transitive binding propagation
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{86}};
        expr v2{expr::var{87}};
        expr v3{expr::var{88}};
        
        // First structure: cons(V1, V1)
        expr c1{expr::functor{"cons", {&v1, &v1}}};
        
        // Second structure: cons(V2, V3)
        expr c2{expr::functor{"cons", {&v2, &v3}}};
        
        // Unify cons(V1, V1) with cons(V2, V3)
        // This unifies V1 with V2 (lhs), then V1 with V3 (rhs)
        // Result: all three vars in same equivalence class
        assert(bm.unify(&c1, &c2));
        size_t bindings_after_first = bm.bindings.size();
        assert(bindings_after_first == 2);  // 2 bindings (2 vars point to the third)
        
        // V1, V2, V3 should all reduce to the same thing
        const expr* result1 = bm.whnf(&v1);
        const expr* result2 = bm.whnf(&v2);
        const expr* result3 = bm.whnf(&v3);
        assert(result1 == result2);
        assert(result2 == result3);
        // Result should be one of the three vars
        assert(result1 == &v1 || result1 == &v2 || result1 == &v3);
        
        // Now unify V3 with atom 'a'
        expr a1{expr::functor{"a", {}}};
        assert(bm.unify(&v3, &a1));
        assert(bm.bindings.size() == bindings_after_first + 1);  // One more binding
        
        // All three vars should now reduce to 'a'
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);
        assert(bm.whnf(&v3) == &a1);
        
        t.pop();
    }
    
    // ========== STRUCTURAL OCCURS CHECK CASES ==========
    
    // Test 54: Structural occurs check - cons(V1, a) with cons(cons(V1, b), a)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{89}};
        expr v2{expr::var{89}};  // Same index
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        // Build cons(cons(V1, b), a) - V1 appears nested inside
        expr inner{expr::functor{"cons", {&v2, &a2}}};
        expr c2{expr::functor{"cons", {&inner, &a1}}};
        
        // Should fail: trying to bind V1 to cons(cons(V1, b), a) which contains V1
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 55: Structural occurs check - cons(V1, V2) with cons(cons(V1, a), V2)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{90}};
        expr v2{expr::var{91}};
        expr v3{expr::var{90}};  // Same as V1
        expr v4{expr::var{91}};  // Same as V2
        expr a1{expr::functor{"a", {}}};
        
        // Build cons(V1, V2)
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Build cons(cons(V1, a), V2)
        expr inner{expr::functor{"cons", {&v3, &a1}}};
        expr c2{expr::functor{"cons", {&inner, &v4}}};
        
        // Unifying cons(V1, V2) with cons(cons(V1, a), V2)
        // lhs: V1 with cons(V1, a) - should fail occurs check (V1 in structure)
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 56: Structural occurs check - cons(V1, cons(V2, V1)) with cons(cons(V1, a), b)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{92}};
        expr v2{expr::var{93}};
        expr v3{expr::var{92}};  // Same as V1
        expr v4{expr::var{92}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Build cons(V1, cons(V2, V1))
        expr inner1{expr::functor{"cons", {&v2, &v3}}};
        expr c1{expr::functor{"cons", {&v1, &inner1}}};
        
        // Build cons(cons(V1, a), b)
        expr inner2{expr::functor{"cons", {&v4, &a1}}};
        expr c2{expr::functor{"cons", {&inner2, &a2}}};
        
        // Unifying cons(V1, cons(V2, V1)) with cons(cons(V1, a), b)
        // lhs: V1 with cons(V1, a) - should fail occurs check (V1 in structure)
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 57: Structural occurs check through chain - cons(V1, V2) with cons(a, cons(V3, b)) where V1->V3
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{93}};
        expr v2{expr::var{94}};
        expr v3{expr::var{93}};  // Same as V1
        expr v4{expr::var{95}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Pre-existing chain: V4 -> V1
        bm.bindings[95] = &v1;
        
        expr c1{expr::functor{"cons", {&v4, &v2}}};
        
        // Build cons(a, cons(V1, b))
        expr inner{expr::functor{"cons", {&v3, &a2}}};
        expr c2{expr::functor{"cons", {&a1, &inner}}};
        
        // Unifying cons(V4, V2) with cons(a, cons(V1, b))
        // V4 reduces to V1 via chain
        // lhs: V1 with a - succeeds, binds V1 to a
        // rhs: V2 with cons(V1, b) - should fail occurs check through chain (V4->V1 in structure)
        // Wait, V1 is already bound to 'a' at this point, so cons(V1, b) reduces to cons(a, b)
        // Actually this should succeed
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 3);  // Original chain + V1->a + V2->cons(a,b)
        
        t.pop();
    }
    
    // Test 58: Structural occurs check - V1 appears multiple times, one occurrence creates cycle
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{96}};
        expr v2{expr::var{96}};  // Same as V1
        expr v3{expr::var{96}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        
        // Build cons(V1, cons(a, V1)) - V1 appears twice
        expr inner{expr::functor{"cons", {&a1, &v2}}};
        expr c1{expr::functor{"cons", {&v1, &inner}}};
        
        // Build cons(cons(V1, a), cons(a, b))
        expr a2{expr::functor{"b", {}}};
        expr inner2{expr::functor{"cons", {&v3, &a1}}};
        expr inner3{expr::functor{"cons", {&a1, &a2}}};
        expr c2{expr::functor{"cons", {&inner2, &inner3}}};
        
        // Unifying cons(V1, cons(a, V1)) with cons(cons(V1, a), cons(a, b))
        // lhs: V1 with cons(V1, a) - should fail occurs check immediately
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // ========== MULTIPLE OCCURRENCES OF SAME VAR ==========
    
    // Test 59: cons(V1, cons(V1, V1)) unified with cons(a, cons(a, a))
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{97}};
        expr v2{expr::var{97}};  // Same as V1
        expr v3{expr::var{97}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"a", {}}};
        expr a3{expr::functor{"a", {}}};
        
        // Build cons(V1, cons(V1, V1))
        expr inner1{expr::functor{"cons", {&v2, &v3}}};
        expr c1{expr::functor{"cons", {&v1, &inner1}}};
        
        // Build cons(a, cons(a, a))
        expr inner2{expr::functor{"cons", {&a2, &a3}}};
        expr c2{expr::functor{"cons", {&a1, &inner2}}};
        
        // All three occurrences of V1 must unify to 'a'
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Just V1->a
        assert(bm.bindings.count(97) == 1);
        assert(bm.whnf(&v1) == &a1);
        
        t.pop();
    }
    
    // Test 60: cons(cons(V1, V1), V1) - var in nested lhs and top-level rhs
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{98}};
        expr v2{expr::var{98}};  // Same as V1
        expr v3{expr::var{98}};  // Same as V1
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"x", {}}};
        
        // Build cons(cons(V1, V1), V1)
        expr inner1{expr::functor{"cons", {&v1, &v2}}};
        expr c1{expr::functor{"cons", {&inner1, &v3}}};
        
        // Build cons(cons(x, x), x)
        expr inner2{expr::functor{"cons", {&a1, &a2}}};
        expr c2{expr::functor{"cons", {&inner2, &a3}}};
        
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Just V1->x
        assert(bm.bindings.count(98) == 1);
        assert(bm.whnf(&v1) == &a1);
        
        t.pop();
    }
    
    // Test 61: cons(V1, cons(V1, V1)) fails occurs check with cons(cons(V1, a), b)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{99}};
        expr v2{expr::var{99}};  // Same as V1
        expr v3{expr::var{99}};  // Same as V1
        expr v4{expr::var{99}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Build cons(V1, cons(V1, V1))
        expr inner1{expr::functor{"cons", {&v2, &v3}}};
        expr c1{expr::functor{"cons", {&v1, &inner1}}};
        
        // Build cons(cons(V1, a), b)
        expr inner2{expr::functor{"cons", {&v4, &a1}}};
        expr c2{expr::functor{"cons", {&inner2, &a2}}};
        
        // Unifying cons(V1, cons(V1, V1)) with cons(cons(V1, a), b)
        // lhs: V1 with cons(V1, a) - should fail occurs check
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // ========== ADDITIONAL STRUCTURAL TESTS ==========
    
    // Test 62: Var appears in both lhs and rhs of cons - consistent binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{100}};
        expr v2{expr::var{101}};
        expr a1{expr::functor{"a", {}}};
        
        // Pre-bind V2 to V1
        bm.bindings[101] = &v1;
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        expr c2{expr::functor{"cons", {&a1, &v1}}};
        
        // Unifying cons(V1, V2) with cons(a, V1)
        // V2 reduces to V1, so we're unifying cons(V1, V1) with cons(a, V1)
        // lhs: V1 with a - binds V1 to a
        // rhs: V1 with V1 - same thing, succeeds
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // Original V2->V1 + V1->a
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);  // V2 transitively reduces to a
        
        t.pop();
    }
    
    // Test 63: cons(V1, cons(V2, V1)) with cons(V2, cons(V1, V2)) - complex symmetry
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{102}};
        expr v2{expr::var{103}};
        
        // Build cons(V1, cons(V2, V1))
        expr inner1{expr::functor{"cons", {&v2, &v1}}};
        expr c1{expr::functor{"cons", {&v1, &inner1}}};
        
        // Build cons(V2, cons(V1, V2))
        expr inner2{expr::functor{"cons", {&v1, &v2}}};
        expr c2{expr::functor{"cons", {&v2, &inner2}}};
        
        // Unifying cons(V1, cons(V2, V1)) with cons(V2, cons(V1, V2))
        // lhs: V1 with V2 - creates binding (say 102->103)
        // rhs: cons(V2, V1) with cons(V1, V2)
        //   Both V1 and V2 now reduce to 103, so:
        //   - lhs of rhs: V2 with V1 - both reduce to 103, succeeds
        //   - rhs of rhs: V1 with V2 - both reduce to 103, succeeds
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Just V1 and V2 unified
        assert(bm.bindings.count(102) == 1 || bm.bindings.count(103) == 1);
        assert(bm.whnf(&v1) == bm.whnf(&v2));
        const expr* result = bm.whnf(&v1);
        assert(result == &v1 || result == &v2);
        
        t.pop();
    }
    
    // ========== CHAIN EXTENSION THROUGH UNIFICATION ==========
    
    // Test 64: Pre-existing chain extended by structural unification
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{104}};
        expr v2{expr::var{105}};
        expr v3{expr::var{106}};
        expr a1{expr::functor{"a", {}}};
        
        // Pre-existing chain: V1 -> V2
        bm.bindings[104] = &v2;
        
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        expr c2{expr::functor{"cons", {&v3, &a1}}};
        
        // Unify cons(V1, a) with cons(V3, a)
        // V1 reduces to V2, so we unify V2 with V3
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // Original chain + new binding
        // Either V2->V3 or V3->V2
        assert(bm.bindings.count(105) == 1 || bm.bindings.count(106) == 1);
        assert(bm.whnf(&v1) == bm.whnf(&v3));
        assert(bm.whnf(&v2) == bm.whnf(&v3));
        
        t.pop();
    }
    
    // Test 65: Multiple chains merge through structural unification
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{107}};
        expr v2{expr::var{108}};
        expr v3{expr::var{109}};
        expr v4{expr::var{110}};
        expr v5{expr::var{111}};
        
        // Pre-existing chains: V1->V2, V3->V4
        bm.bindings[107] = &v2;
        bm.bindings[109] = &v4;
        
        // Unify cons(V2, V4) with cons(V5, V5)
        expr c1{expr::functor{"cons", {&v2, &v4}}};
        expr c2{expr::functor{"cons", {&v5, &v5}}};
        
        // lhs: V2 with V5 - binds one to the other
        // rhs: V4 with V5 - both now in same equivalence class
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 3 || bm.bindings.size() == 4);  // Original 2 chains + 1-2 new bindings
        
        // All five vars should reduce to the same thing
        const expr* result = bm.whnf(&v1);
        assert(bm.whnf(&v2) == result);
        assert(bm.whnf(&v3) == result);
        assert(bm.whnf(&v4) == result);
        assert(bm.whnf(&v5) == result);
        
        t.pop();
    }
    
    // ========== NESTED CONS WITH SHARED INNER STRUCTURE ==========
    
    // Test 66: Shared inner cons cell
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{112}};
        expr v2{expr::var{113}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Create shared inner: cons(V1, a)
        expr inner{expr::functor{"cons", {&v1, &a1}}};
        
        // Build cons(inner, V2) and cons(inner, b)
        expr c1{expr::functor{"cons", {&inner, &v2}}};
        expr c2{expr::functor{"cons", {&inner, &a2}}};
        
        // Unify cons(inner, V2) with cons(inner, b)
        // lhs: inner with inner - same pointer, succeeds
        // rhs: V2 with b - binds V2 to b
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Just V2->b, inner unchanged
        assert(bm.bindings.count(113) == 1);
        assert(bm.whnf(&v2) == &a2);
        assert(bm.whnf(&v1) == &v1);  // V1 still unbound
        
        t.pop();
    }
    
    // ========== MULTIPLE FAILURES WITH TRAIL ISOLATION ==========
    
    // Test 67: Multiple failures in sequence - trail isolation
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{114}};
        expr v2{expr::var{115}};
        expr a1{expr::functor{"x", {}}};
        expr a2{expr::functor{"y", {}}};
        expr a3{expr::functor{"z", {}}};
        
        // First failure
        t.push();
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        expr c2{expr::functor{"cons", {&a2, &a3}}};  // Mismatched rhs
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Partial: V1->a2
        assert(bm.whnf(&v1) == &a2);
        t.pop();
        
        // After pop, bindings should be undone
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
        assert(bm.whnf(&v2) == &v2);
        
        // Second failure - different partial bindings
        t.push();
        expr a4{expr::functor{"w", {}}};
        expr a5{expr::functor{"q", {}}};
        expr c3{expr::functor{"cons", {&v2, &a4}}};
        expr c4{expr::functor{"cons", {&a5, &a1}}};  // Mismatched rhs
        assert(!bm.unify(&c3, &c4));
        assert(bm.bindings.size() == 1);  // Partial: V2->a5
        assert(bm.whnf(&v2) == &a5);
        assert(bm.whnf(&v1) == &v1);  // V1 still unbound (isolated from first failure)
        t.pop();
        
        // After second pop, all bindings undone
        assert(bm.bindings.size() == 0);
        assert(bm.whnf(&v1) == &v1);
        assert(bm.whnf(&v2) == &v2);
    }
    
    // ========== DEEPLY NESTED IDENTICAL SUBSTRUCTURES ==========
    
    // Test 68: cons(cons(V1, V1), cons(V1, V1)) - var appears 4 times
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{116}};
        expr v2{expr::var{116}};
        expr v3{expr::var{116}};
        expr v4{expr::var{116}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"a", {}}};
        expr a3{expr::functor{"a", {}}};
        expr a4{expr::functor{"a", {}}};
        
        // Build cons(cons(V1, V1), cons(V1, V1))
        expr left1{expr::functor{"cons", {&v1, &v2}}};
        expr right1{expr::functor{"cons", {&v3, &v4}}};
        expr c1{expr::functor{"cons", {&left1, &right1}}};
        
        // Build cons(cons(a, a), cons(a, a))
        expr left2{expr::functor{"cons", {&a1, &a2}}};
        expr right2{expr::functor{"cons", {&a3, &a4}}};
        expr c2{expr::functor{"cons", {&left2, &right2}}};
        
        // All four occurrences must bind consistently
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Just V1->a
        assert(bm.bindings.count(116) == 1);
        assert(bm.whnf(&v1) == &a1);
        
        t.pop();
    }
    
    // ========== CONS CHILDREN BOUND TO CONS CELLS ==========
    
    // Test 69: Vars bound to cons cells, then unified
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{117}};
        expr v2{expr::var{118}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Create cons(a, b)
        expr c_ab{expr::functor{"cons", {&a1, &a2}}};
        
        // Bind both V1 and V2 to the same cons cell
        bm.bindings[117] = &c_ab;
        bm.bindings[118] = &c_ab;
        
        // Unify V1 with V2 - both reduce to same cons, should succeed
        assert(bm.unify(&v1, &v2));
        assert(bm.bindings.size() == 2);  // No new binding
        assert(bm.whnf(&v1) == &c_ab);
        assert(bm.whnf(&v2) == &c_ab);
        
        t.pop();
    }
    
    // Test 70: Var bound to cons, unified with cons containing vars
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{119}};
        expr v2{expr::var{120}};
        expr v3{expr::var{121}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Create cons(a, b) and bind V1 to it
        expr c_ab{expr::functor{"cons", {&a1, &a2}}};
        bm.bindings[119] = &c_ab;
        
        // Unify V1 with cons(V2, V3)
        expr c2{expr::functor{"cons", {&v2, &v3}}};
        
        // V1 reduces to cons(a, b), so we unify cons(a, b) with cons(V2, V3)
        // This should bind V2 to a and V3 to b
        assert(bm.unify(&v1, &c2));
        assert(bm.bindings.size() == 3);  // Original V1->cons + V2->a + V3->b
        assert(bm.bindings.count(120) == 1);
        assert(bm.bindings.count(121) == 1);
        assert(bm.whnf(&v2) == &a1);
        assert(bm.whnf(&v3) == &a2);
        
        t.pop();
    }
    
    // ========== UNIFICATION ORDERING MATTERS ==========
    
    // Test 71: Pre-bound var causes failure in structural unification
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{122}};
        expr v2{expr::var{123}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        // Pre-bind V1 to 'c'
        bm.bindings[122] = &a3;
        
        // Unify cons(V1, a) with cons(b, V2)
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        expr c2{expr::functor{"cons", {&a2, &v2}}};
        
        // lhs: V1 (reduces to c) with b - should fail
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Only original binding, no partial
        assert(bm.whnf(&v1) == &a3);
        assert(bm.whnf(&v2) == &v2);  // V2 unbound
        
        t.pop();
    }
    
    // Test 72: Pre-bound var in rhs causes partial binding
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{124}};
        expr v2{expr::var{125}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        // Pre-bind V2 to 'c'
        bm.bindings[125] = &a3;
        
        // Unify cons(V1, a) with cons(b, V2)
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        expr c2{expr::functor{"cons", {&a2, &v2}}};
        
        // lhs: V1 with b - succeeds, binds V1 to b
        // rhs: a with V2 (reduces to c) - should fail
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // Original V2->c + partial V1->b
        assert(bm.bindings.count(124) == 1);
        assert(bm.whnf(&v1) == &a2);
        assert(bm.whnf(&v2) == &a3);
        
        t.pop();
    }
    
    // ========== TRIPLE-NESTED CONS ==========
    
    // Test 73: Triple-nested cons with vars at each level
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{126}};
        expr v2{expr::var{127}};
        expr v3{expr::var{128}};
        expr v4{expr::var{129}};
        expr a1{expr::functor{"w", {}}};
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr a4{expr::functor{"z", {}}};
        
        // Build cons(cons(cons(V1, V2), V3), V4)
        expr inner1{expr::functor{"cons", {&v1, &v2}}};
        expr inner2{expr::functor{"cons", {&inner1, &v3}}};
        expr c1{expr::functor{"cons", {&inner2, &v4}}};
        
        // Build cons(cons(cons(w, x), y), z)
        expr inner3{expr::functor{"cons", {&a1, &a2}}};
        expr inner4{expr::functor{"cons", {&inner3, &a3}}};
        expr c2{expr::functor{"cons", {&inner4, &a4}}};
        
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 4);  // Four bindings created
        assert(bm.bindings.count(126) == 1);
        assert(bm.bindings.count(127) == 1);
        assert(bm.bindings.count(128) == 1);
        assert(bm.bindings.count(129) == 1);
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a2);
        assert(bm.whnf(&v3) == &a3);
        assert(bm.whnf(&v4) == &a4);
        
        t.pop();
    }
    
    // Test 74: Triple-nested with some pre-existing bindings
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{130}};
        expr v2{expr::var{131}};
        expr v3{expr::var{132}};
        expr v4{expr::var{133}};
        expr a1{expr::functor{"w", {}}};
        expr a2{expr::functor{"x", {}}};
        expr a3{expr::functor{"y", {}}};
        expr a4{expr::functor{"z", {}}};
        
        // Pre-bind V2 to x
        bm.bindings[131] = &a2;
        
        // Build cons(cons(cons(V1, V2), V3), V4)
        expr inner1{expr::functor{"cons", {&v1, &v2}}};
        expr inner2{expr::functor{"cons", {&inner1, &v3}}};
        expr c1{expr::functor{"cons", {&inner2, &v4}}};
        
        // Build cons(cons(cons(w, x), y), z)
        expr inner3{expr::functor{"cons", {&a1, &a2}}};
        expr inner4{expr::functor{"cons", {&inner3, &a3}}};
        expr c2{expr::functor{"cons", {&inner4, &a4}}};
        
        // V2 is already bound to x, so unification should succeed
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 4);  // Original 1 + 3 new bindings
        assert(bm.bindings.count(130) == 1);
        assert(bm.bindings.count(132) == 1);
        assert(bm.bindings.count(133) == 1);
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a2);
        assert(bm.whnf(&v3) == &a3);
        assert(bm.whnf(&v4) == &a4);
        
        t.pop();
    }
    
    // ========== COMPLEX CHAIN NETWORKS ==========
    
    // Test 75: Unification creates long chains through structure
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{134}};
        expr v2{expr::var{135}};
        expr v3{expr::var{136}};
        expr v4{expr::var{137}};
        expr v5{expr::var{138}};
        expr v6{expr::var{139}};
        
        // First unification: cons(V1, V2) with cons(V3, V4)
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        expr c2{expr::functor{"cons", {&v3, &v4}}};
        assert(bm.unify(&c1, &c2));
        size_t bindings_after_first = bm.bindings.size();
        assert(bindings_after_first == 2);  // V1 and V2 each bound
        
        // Second unification: cons(V3, V5) with cons(V6, V1)
        expr c3{expr::functor{"cons", {&v3, &v5}}};
        expr c4{expr::functor{"cons", {&v6, &v1}}};
        assert(bm.unify(&c3, &c4));
        
        // Now we have a complex network:
        // From first: V1 and V3 unified, V2 and V4 unified
        // From second: V3 and V6 unified, V5 and V1 unified
        // Result: V1, V3, V5, V6 all in one equivalence class
        //         V2, V4 in another equivalence class
        assert(bm.whnf(&v1) == bm.whnf(&v3));
        assert(bm.whnf(&v1) == bm.whnf(&v5));
        assert(bm.whnf(&v1) == bm.whnf(&v6));
        assert(bm.whnf(&v2) == bm.whnf(&v4));
        
        t.pop();
    }
    
    // ========== STRUCTURAL OCCURS CHECK WITH MULTIPLE VAR OCCURRENCES ==========
    
    // Test 76: Occurs check with var appearing multiple times in target structure
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{140}};
        expr v2{expr::var{140}};  // Same as V1
        expr v3{expr::var{140}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        
        // Build cons(cons(V1, V1), a) - V1 appears twice in nested structure
        expr inner{expr::functor{"cons", {&v2, &v3}}};
        expr c1{expr::functor{"cons", {&inner, &a1}}};
        
        // Try to unify V1 with this structure containing V1
        assert(!bm.unify(&v1, &c1));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 77: Var unified with deeply nested cons containing another var
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{141}};
        expr v2{expr::var{142}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"x", {}}};
        
        // Build deeply nested structure: cons(cons(cons(V1, a), a), a)
        expr inner1{expr::functor{"cons", {&v1, &a1}}};
        expr inner2{expr::functor{"cons", {&inner1, &a1}}};
        expr inner3{expr::functor{"cons", {&inner2, &a1}}};
        
        // Unify V2 with this nested structure
        assert(bm.unify(&v2, &inner3));
        assert(bm.bindings.size() == 1);  // V2 bound to nested structure
        assert(bm.bindings.count(142) == 1);
        assert(bm.whnf(&v2) == &inner3);
        assert(bm.whnf(&v1) == &v1);  // V1 still unbound
        
        // Now unify V1 with atom
        assert(bm.unify(&v1, &a2));
        assert(bm.bindings.size() == 2);  // V2->nested + V1->x
        assert(bm.bindings.count(141) == 1);
        assert(bm.whnf(&v1) == &a2);
        
        t.pop();
    }
    
    // Test 78: Occurs check during nested cons unification
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{143}};
        expr v2{expr::var{144}};
        expr v3{expr::var{143}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        
        // Build cons(V1, V2)
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Build cons(a, cons(V1, a)) - V1 appears in rhs
        expr inner{expr::functor{"cons", {&v3, &a1}}};
        expr c2{expr::functor{"cons", {&a1, &inner}}};
        
        // Unifying cons(V1, V2) with cons(a, cons(V1, a))
        // lhs: V1 with a - succeeds, binds V1 to a
        // rhs: V2 with cons(V1, a) - V1 now reduces to a, so cons(a, a), should succeed
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // V1->a, V2->cons(a,a)
        assert(bm.whnf(&v1) == &a1);
        // V2 should be bound to a cons cell
        const expr* v2_result = bm.whnf(&v2);
        assert(std::holds_alternative<expr::functor>(v2_result->content));
        
        t.pop();
    }
    
    // ========== ADDITIONAL PURE OCCURS CHECK TESTS ==========
    
    // Test 79: Occurs check at rhs child level - cons(a, V1) with cons(a, cons(b, V1))
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{145}};
        expr v2{expr::var{145}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        expr c1{expr::functor{"cons", {&a1, &v1}}};
        
        // Build cons(a, cons(b, V1))
        expr inner{expr::functor{"cons", {&a2, &v2}}};
        expr c2{expr::functor{"cons", {&a1, &inner}}};
        
        // Unifying cons(a, V1) with cons(a, cons(b, V1))
        // lhs: a with a - succeeds
        // rhs: V1 with cons(b, V1) - should fail occurs check (V1 in structure)
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 80: Occurs check in nested cons child - cons(cons(V1, a), b) with cons(cons(cons(V1, c), a), b)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{146}};
        expr v2{expr::var{146}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        // Build cons(cons(V1, a), b)
        expr inner1{expr::functor{"cons", {&v1, &a1}}};
        expr c1{expr::functor{"cons", {&inner1, &a2}}};
        
        // Build cons(cons(cons(V1, c), a), b)
        expr inner2{expr::functor{"cons", {&v2, &a3}}};
        expr inner3{expr::functor{"cons", {&inner2, &a1}}};
        expr c2{expr::functor{"cons", {&inner3, &a2}}};
        
        // Unifying cons(cons(V1, a), b) with cons(cons(cons(V1, c), a), b)
        // lhs: cons(V1, a) with cons(cons(V1, c), a)
        //   - lhs of lhs: V1 with cons(V1, c) - should fail occurs check
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 81: Occurs check with var in both children - cons(V1, V2) with cons(cons(V1, a), cons(b, V1))
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{147}};
        expr v2{expr::var{148}};
        expr v3{expr::var{147}};  // Same as V1
        expr v4{expr::var{147}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Build cons(cons(V1, a), cons(b, V1))
        expr inner1{expr::functor{"cons", {&v3, &a1}}};
        expr inner2{expr::functor{"cons", {&a2, &v4}}};
        expr c2{expr::functor{"cons", {&inner1, &inner2}}};
        
        // Unifying cons(V1, V2) with cons(cons(V1, a), cons(b, V1))
        // lhs: V1 with cons(V1, a) - should fail occurs check immediately
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 82: Occurs check through multiple levels - cons(V1, a) with cons(cons(cons(V1, b), c), a)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{149}};
        expr v2{expr::var{149}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        expr a3{expr::functor{"c", {}}};
        
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        // Build cons(cons(cons(V1, b), c), a) - V1 deeply nested in lhs
        expr inner1{expr::functor{"cons", {&v2, &a2}}};
        expr inner2{expr::functor{"cons", {&inner1, &a3}}};
        expr c2{expr::functor{"cons", {&inner2, &a1}}};
        
        // Unifying cons(V1, a) with cons(cons(cons(V1, b), c), a)
        // lhs: V1 with cons(cons(V1, b), c) - should fail occurs check (V1 deeply nested)
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 83: Occurs check with pre-existing chain - V2->V1, then cons(V1, a) with cons(cons(V2, b), a)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{150}};
        expr v2{expr::var{151}};
        expr v3{expr::var{151}};  // Same as V2
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Pre-existing chain: V2 -> V1
        bm.bindings[151] = &v1;
        
        expr c1{expr::functor{"cons", {&v1, &a1}}};
        
        // Build cons(cons(V2, b), a) - V2 chains to V1
        expr inner{expr::functor{"cons", {&v3, &a2}}};
        expr c2{expr::functor{"cons", {&inner, &a1}}};
        
        // Unifying cons(V1, a) with cons(cons(V2, b), a)
        // lhs: V1 with cons(V2, b)
        //   V2 reduces to V1 via chain, so cons(V2, b) contains V1 indirectly
        //   Should fail occurs check through chain
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 1);  // Only original chain, occurs check prevents new binding
        
        t.pop();
    }
    
    // Test 84: Occurs check with var appearing in multiple nested positions
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{152}};
        expr v2{expr::var{152}};  // Same as V1
        expr v3{expr::var{152}};  // Same as V1
        expr v4{expr::var{153}};
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Build cons(cons(V1, cons(V1, a)), b) - V1 appears twice in nested structure
        expr inner1{expr::functor{"cons", {&v2, &a1}}};
        expr inner2{expr::functor{"cons", {&v3, &inner1}}};
        expr c1{expr::functor{"cons", {&inner2, &a2}}};
        
        // Try to unify V1 with this structure
        assert(!bm.unify(&v1, &c1));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 85: Occurs check - cons(cons(V1, V2), a) with cons(cons(cons(V1, b), V2), a)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{154}};
        expr v2{expr::var{155}};
        expr v3{expr::var{154}};  // Same as V1
        expr v4{expr::var{155}};  // Same as V2
        expr a1{expr::functor{"a", {}}};
        expr a2{expr::functor{"b", {}}};
        
        // Build cons(cons(V1, V2), a)
        expr inner1{expr::functor{"cons", {&v1, &v2}}};
        expr c1{expr::functor{"cons", {&inner1, &a1}}};
        
        // Build cons(cons(cons(V1, b), V2), a)
        expr inner2{expr::functor{"cons", {&v3, &a2}}};
        expr inner3{expr::functor{"cons", {&inner2, &v4}}};
        expr c2{expr::functor{"cons", {&inner3, &a1}}};
        
        // Unifying cons(cons(V1, V2), a) with cons(cons(cons(V1, b), V2), a)
        // lhs: cons(V1, V2) with cons(cons(V1, b), V2)
        //   - lhs of lhs: V1 with cons(V1, b) - should fail occurs check
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // Occurs check prevents binding
        
        t.pop();
    }
    
    // Test 86: Occurs check through equivalence class - cons(V1, V2) with cons(V2, cons(V1, a))
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr v1{expr::var{156}};
        expr v2{expr::var{157}};
        expr v3{expr::var{157}};  // Same as V2
        expr v4{expr::var{156}};  // Same as V1
        expr a1{expr::functor{"a", {}}};
        
        expr c1{expr::functor{"cons", {&v1, &v2}}};
        
        // Build cons(V2, cons(V1, a))
        expr inner{expr::functor{"cons", {&v4, &a1}}};
        expr c2{expr::functor{"cons", {&v3, &inner}}};
        
        // Unifying cons(V1, V2) with cons(V2, cons(V1, a))
        // lhs: V1 with V2 - creates binding (either 156->157 or 157->156)
        // rhs: V2 with cons(V1, a)
        //   After lhs binding, V1 and V2 are in same equivalence class
        //   occurs_check(V2_index, cons(V1, a)) follows V1's binding via whnf()
        //   and correctly detects that V1 reduces to V2, thus V2 occurs in structure
        assert(!bm.unify(&c1, &c2));
        // Partial binding: one binding from lhs before rhs occurs check
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(156) == 1 || bm.bindings.count(157) == 1);
        
        t.pop();
    }

    // ========== STRUCTURAL IDENTITY: DIFFERENT ALLOCATIONS, SAME INDEX ==========
    // The following tests specifically target the new behavior where pointer equality
    // is not used. Two expressions with the same structural content (same variable
    // index, same atom value, same cons shape) must unify correctly regardless of
    // which expr_pool they came from or whether they live on the stack.

    // Test 87: Same-index var, two different stack allocations → succeed, no binding.
    // The old pointer-equality shortcut would miss this; the new same-index guard catches it.
    {
        trail t;
        bind_map bm(t);
        t.push();

        expr v1{expr::var{200}};
        expr v2{expr::var{200}};  // Same index, different stack address
        assert(&v1 != &v2);

        assert(bm.unify(&v1, &v2));
        assert(bm.bindings.size() == 0);  // Same logical variable — no binding needed
        assert(bm.whnf(&v1) == &v1);      // Still unbound (index 200 has no entry)
        assert(bm.whnf(&v2) == &v2);

        t.pop();
    }

    // Test 88: Same-index var — stack allocation vs expr_pool allocation → succeed, no binding.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep(t);

        expr      v_stack{expr::var{201}};
        const expr* v_pool = ep.var(201);  // Same index, allocated inside the pool's set
        assert(&v_stack != v_pool);

        assert(bm.unify(&v_stack, v_pool));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 89: Same-index var from two different expr_pools → succeed, no binding.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        const expr* v1 = ep1.var(202);
        const expr* v2 = ep2.var(202);  // Same index; different pool, different pointer
        assert(v1 != v2);

        assert(bm.unify(v1, v2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 90: Same-value atoms from two different expr_pools → succeed, no binding (regression).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        const expr* a1 = ep1.functor("hello", {});
        const expr* a2 = ep2.functor("hello", {});  // Same value, different pool
        assert(a1 != a2);

        assert(bm.unify(a1, a2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 91: Structurally equal cons cells from two different expr_pools → succeed, no binding (regression).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        const expr* c1 = ep1.functor("cons", {ep1.functor("x", {}), ep1.functor("y", {})});
        const expr* c2 = ep2.functor("cons", {ep2.functor("x", {}), ep2.functor("y", {})});
        assert(c1 != c2);

        assert(bm.unify(c1, c2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 92: Cross-pool var unified with atom from a different pool → correct binding.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        const expr* v    = ep1.var(203);
        const expr* atom = ep2.functor("target", {});

        assert(bm.unify(v, atom));
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(203) == 1);
        assert(bm.whnf(v) == atom);

        t.pop();
    }

    // Test 93: Same-index vars from different pools unified with each other, then with an atom.
    // Demonstrates that no binding is created for the same-index unification, and a later
    // binding of that index applies uniformly to all allocations of that variable.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        const expr* v1   = ep1.var(204);
        const expr* v2   = ep2.var(204);  // Same index, different pool
        const expr* atom = ep1.functor("value", {});

        // Step 1: unify same-index vars → no binding
        assert(bm.unify(v1, v2));
        assert(bm.bindings.size() == 0);

        // Step 2: unify either with atom → binding 204 → atom
        assert(bm.unify(v1, atom));
        assert(bm.bindings.size() == 1);

        // The binding applies to both pointers (they share the same index)
        assert(bm.whnf(v1) == atom);
        assert(bm.whnf(v2) == atom);

        t.pop();
    }

    // Test 94: Cross-pool cons with a var inside that gets bound to an atom from the other pool.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        // ep1: cons(var(205), "fixed")
        const expr* c1 = ep1.functor("cons", {ep1.var(205), ep1.functor("fixed", {})});

        // ep2: cons("bound_val", "fixed")
        const expr* bound_val = ep2.functor("bound_val", {});
        const expr* c2        = ep2.functor("cons", {bound_val, ep2.functor("fixed", {})});

        assert(bm.unify(c1, c2));
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(205) == 1);
        assert(bm.whnf(ep1.var(205)) == bound_val);

        t.pop();
    }

    // Test 95: Stack cons with a var inside, pool cons with atom → correct cross-allocation binding.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep(t);

        // Stack: cons(var(206), "hello")
        expr v_stack{expr::var{206}};
        expr a_hello{expr::functor{"hello", {}}};
        expr c_stack{expr::functor{"cons", {&v_stack, &a_hello}}};

        // Pool: cons("world", "hello")
        const expr* c_pool = ep.functor("cons", {ep.functor("world", {}), ep.functor("hello", {})});

        assert(bm.unify(&c_stack, c_pool));
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(206) == 1);

        const expr* world = ep.functor("world", {});
        assert(bm.whnf(&v_stack) == world);

        t.pop();
    }

    // Test 96: Two pool-allocated cons cells with same-index vars in every position →
    // succeed, no bindings (structurally identical in every dimension).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        // ep1: cons(var(207), var(208))
        const expr* c1 = ep1.functor("cons", {ep1.var(207), ep1.var(208)});
        // ep2: cons(var(207), var(208)) — same indices, different pool
        const expr* c2 = ep2.functor("cons", {ep2.var(207), ep2.var(208)});
        assert(c1 != c2);

        // Both children unify via same-index guard — no binding created at any position
        assert(bm.unify(c1, c2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 97: Three pools — var allocated in pool1 gets bound through unification of
    // a cons from pool2 against a cons from pool3. Binding applies to pool1 var too.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);
        expr_pool ep3(t);

        const expr* v_ep1 = ep1.var(209);

        // ep2: cons(var(209), "a")
        const expr* c2 = ep2.functor("cons", {ep2.var(209), ep2.functor("a", {})});

        // ep3: cons("bound", "a")
        const expr* bound = ep3.functor("bound", {});
        const expr* c3    = ep3.functor("cons", {bound, ep3.functor("a", {})});

        // Unify ep2's cons with ep3's cons → var(209) gets bound to "bound"
        assert(bm.unify(c2, c3));
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(209) == 1);

        // The binding propagates to v_ep1 from ep1, which shares index 209
        assert(bm.whnf(v_ep1) == bound);

        t.pop();
    }

    // Test 98: Occurs check works correctly across allocation boundaries.
    // Stack var(210) vs pool cons containing pool var(210) → same index → cycle detected.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep(t);

        expr v_stack{expr::var{210}};

        // Pool: cons(var(210), "a") — var index matches stack var
        const expr* c_pool = ep.functor("cons", {ep.var(210), ep.functor("a", {})});

        // Attempting to bind var(210) to a structure that contains var(210) must fail
        assert(!bm.unify(&v_stack, c_pool));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 99: Occurs check across two pools — pool1 var(211) unified with pool2 cons
    // containing pool2 var(211). The indices match even though the allocations differ.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        const expr* v1     = ep1.var(211);
        const expr* c_pool = ep2.functor("cons", {ep2.var(211), ep2.functor("b", {})});  // contains index 211

        assert(!bm.unify(v1, c_pool));
        assert(bm.bindings.size() == 0);

        t.pop();
    }

    // Test 100: Complex cross-pool nested cons with multiple vars, all resolved correctly.
    // Validates that an entire Prolog-style term from one pool unifies against a ground
    // term from another pool with no spurious failures from pointer inequality.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t);
        expr_pool ep2(t);

        // ep1: cons(cons(var(212), var(213)), cons(var(214), "leaf"))
        const expr* c1 = ep1.functor("cons", {
            ep1.functor("cons", {ep1.var(212), ep1.var(213)}), 
            ep1.functor("cons", {ep1.var(214), ep1.functor("leaf", {})})
        });

        // ep2: cons(cons("a", "b"), cons("c", "leaf"))
        const expr* a   = ep2.functor("a", {});
        const expr* b   = ep2.functor("b", {});
        const expr* c   = ep2.functor("c", {});
        const expr* c2  = ep2.functor("cons", {
            ep2.functor("cons", {a, b}), 
            ep2.functor("cons", {c, ep2.functor("leaf", {})})
        });

        assert(bm.unify(c1, c2));
        assert(bm.bindings.size() == 3);
        assert(bm.whnf(ep1.var(212)) == a);
        assert(bm.whnf(ep1.var(213)) == b);
        assert(bm.whnf(ep1.var(214)) == c);

        t.pop();
    }

    // ========== MIXED PRE-BOUND AND FRESH VARS — ALL DIFFERENT ALLOCATIONS ==========

    // Test 101: Pre-bound var from ep1 resolves during cons unification across ep1/ep2/ep3.
    // var(300) is pre-bound to ep2.functor("x", {}). Structure A from ep1 holds var(300) and
    // an unbound var(301). Structure B is entirely from ep3. Unification succeeds:
    // the pre-bound lhs position matches "x" structurally; the unbound rhs gets a new binding.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t);

        bm.bindings[300] = ep2.functor("x", {});  // pre-bind via raw map — no trail entry needed here

        const expr* A = ep1.functor("cons", {ep1.var(300), ep1.var(301)});
        const expr* B = ep3.functor("cons", {ep3.functor("x", {}), ep3.functor("y", {})});

        assert(bm.unify(A, B));
        // lhs: var(300) → ep2.functor("x", {}); ep3.functor("x", {}) → "x"=="x" → no new binding
        // rhs: var(301) unbound → bind 301 → ep3.functor("y", {})
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.count(301) == 1);
        assert(bm.whnf(ep1.var(300)) == ep2.functor("x", {}));
        assert(bm.whnf(ep1.var(301)) == ep3.functor("y", {}));

        t.pop();
    }

    // Test 102: Pre-bound chain spanning three pools — ep1.var(302)→ep2.var(303)→ep3.functor("end", {}).
    // Unifying ep4.var(302) with ep5.functor("end", {}) succeeds by traversing the chain:
    // ep4's allocation is irrelevant; only the index matters. No new binding is created.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t), ep5(t);

        bm.bindings[302] = ep2.var(303);
        bm.bindings[303] = ep3.functor("end", {});

        // ep4.var(302) and ep5.functor("end", {}) are wholly new allocations
        assert(bm.unify(ep4.var(302), ep5.functor("end", {})));
        assert(bm.bindings.size() == 2);  // no new binding; chain resolved to matching atom

        // All allocations of the same index follow the same chain
        assert(bm.whnf(ep4.var(302)) == ep3.functor("end", {}));
        assert(bm.whnf(ep5.var(303)) == ep3.functor("end", {}));

        t.pop();
    }

    // Test 103: Two cons from different pools, both sides with pre-bound vars —
    // the pre-bound values are mutually consistent so no new binding is needed.
    // var(304) pre-bound to ep2.functor("q", {}), var(305) pre-bound to ep4.functor("p", {}).
    // ep1.functor("cons", {var(304), "p"}) unified with ep3.functor("cons", {"q", var(305)}).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t);

        bm.bindings[304] = ep2.functor("q", {});
        bm.bindings[305] = ep4.functor("p", {});

        const expr* A = ep1.functor("cons", {ep1.var(304), ep1.functor("p", {})});
        const expr* B = ep3.functor("cons", {ep3.functor("q", {}), ep3.var(305)});

        assert(bm.unify(A, B));
        assert(bm.bindings.size() == 2);  // pre-bindings only — no new binding created
        assert(bm.whnf(ep1.var(304)) == ep2.functor("q", {}));
        assert(bm.whnf(ep3.var(305)) == ep4.functor("p", {}));

        t.pop();
    }

    // Test 104: Nested cons across three pools; one var pre-bound, two freshly bound,
    // one var left unbound inside the structure that gets captured as a binding target.
    // Structure A (ep1): cons(cons(var(306), var(307)), var(308))
    //   var(306) → ep2.functor("alpha", {}),  var(307) and var(308) unbound.
    // Structure B (ep3): cons(cons("alpha","beta"), cons(var(309), "gamma"))
    //   var(309) unbound (it lives inside the cons that var(308) gets bound to).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t);

        bm.bindings[306] = ep2.functor("alpha", {});

        const expr* A = ep1.functor("cons", {
            ep1.functor("cons", {ep1.var(306), ep1.var(307)}), 
            ep1.var(308)
        });
        const expr* B = ep3.functor("cons", {
            ep3.functor("cons", {ep3.functor("alpha", {}), ep3.functor("beta", {})}), 
            ep3.functor("cons", {ep3.var(309), ep3.functor("gamma", {})})
        });

        assert(bm.unify(A, B));
        // outer.args[0]: var(306)→"alpha" == "alpha" ✓; var(307) → ep3.functor("beta", {})
        // outer.args[1]: var(308) → ep3.functor("cons", {var(309), "gamma"})  (var(309) stays unbound inside)
        assert(bm.bindings.size() == 3);
        assert(bm.whnf(ep1.var(306)) == ep2.functor("alpha", {}));
        assert(bm.whnf(ep1.var(307)) == ep3.functor("beta", {}));

        const expr* rhs308 = bm.whnf(ep1.var(308));
        assert(std::holds_alternative<expr::functor>(rhs308->content));

        // var(309) was not unified away — it remains unbound
        assert(bm.whnf(ep3.var(309)) == ep3.var(309));

        t.pop();
    }

    // Test 105: Cross-pool occurs check traverses a pre-bound chain correctly.
    // var(310) → ep2.var(311). Attempting to bind var(311) to a cons that contains
    // ep4.var(310) must fail: occurs_check walks through the chain (310→311) and
    // discovers that 311 appears inside the target structure.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep2(t), ep3(t), ep4(t);

        bm.bindings[310] = ep2.var(311);  // pre-bound chain

        const expr* target = ep4.functor("cons", {ep4.var(310), ep4.functor("x", {})});
        assert(!bm.unify(ep3.var(311), target));
        assert(bm.bindings.size() == 1);  // only the pre-binding; occurs check stopped the bind

        t.pop();
    }

    // Test 106: Pre-bound var points to a cons from ep2; a fresh cons from ep5 contains
    // the same structure under a different allocation. Structural unification of the pre-bound
    // cons with the fresh cons succeeds without creating bindings for the atoms inside.
    // An unbound var(313) in the other position gets freshly bound.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep2(t), ep4(t), ep5(t);

        bm.bindings[312] = ep2.functor("cons", {ep2.functor("a", {}), ep2.functor("b", {})});

        // ep4: cons(var(312), var(313))
        const expr* A = ep4.functor("cons", {ep4.var(312), ep4.var(313)});

        // ep5: cons(cons("a","b"), "result")  — structurally matches the pre-binding
        const expr* B = ep5.functor("cons", {ep5.functor("cons", {ep5.functor("a", {}), ep5.functor("b", {})}), ep5.functor("result", {})});

        assert(bm.unify(A, B));
        // lhs: var(312) → ep2.functor("cons", {"a", "b"}); ep5.functor("cons", {"a", "b"}) — different alloc, same structure → ✓
        // rhs: var(313) unbound → bind 313 → ep5.functor("result", {})
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.count(313) == 1);
        assert(bm.whnf(ep4.var(313)) == ep5.functor("result", {}));

        t.pop();
    }

    // Test 107: Pre-bound chain ends in an unbound var; structural unification extends
    // the chain by binding the tail var to an atom from a fourth pool.
    // var(314) → ep2.var(315),  var(315) unbound.
    // Unify ep3.var(315) with ep4.functor("merged", {}) — binds 315; now var(314) transitively
    // resolves to ep4.functor("merged", {}) through the chain.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t);

        bm.bindings[314] = ep2.var(315);

        assert(bm.unify(ep3.var(315), ep4.functor("merged", {})));
        assert(bm.bindings.size() == 2);

        // var(314) from ep1 now resolves through the full chain
        assert(bm.whnf(ep1.var(314)) == ep4.functor("merged", {}));
        assert(bm.whnf(ep3.var(315)) == ep4.functor("merged", {}));

        t.pop();
    }

    // Test 108: Rich three-pool scenario — nested cons with a mix of pre-bound vars
    // (one direct, one chaining through another var) and two freshly unbound vars.
    // Every atom target comes from a different pool than the var it is matched against.
    // Structure A (ep1): cons(cons(var(316), var(317)), cons(var(318), var(320)))
    //   var(316) → ep1.functor("x", {})   (direct pre-binding)
    //   var(318) → ep1.var(319)    (chain; var(319) itself unbound)
    //   var(317), var(320) unbound
    // Structure B: cons(cons("x","y"), cons("z","w")) — atoms from ep2 and ep3
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t);

        bm.bindings[316] = ep1.functor("x", {});
        bm.bindings[318] = ep1.var(319);

        const expr* A = ep1.functor("cons", {
            ep1.functor("cons", {ep1.var(316), ep1.var(317)}), 
            ep1.functor("cons", {ep1.var(318), ep1.var(320)})
        });
        const expr* B = ep2.functor("cons", {
            ep2.functor("cons", {ep2.functor("x", {}), ep2.functor("y", {})}), 
            ep2.functor("cons", {ep3.functor("z", {}), ep2.functor("w", {})})   // ep3 atom for extra cross-pool coverage
        });

        assert(bm.unify(A, B));
        // outer.args[0]: var(316)→"x" == "x" ✓;  var(317) → ep2.functor("y", {})
        // outer.args[1]: var(318)→var(319)→unbound → bind 319 → ep3.functor("z", {});  var(320) → ep2.functor("w", {})
        assert(bm.bindings.size() == 5);  // 316(pre) + 318(pre) + 317 + 319 + 320

        assert(bm.whnf(ep1.var(316)) == ep1.functor("x", {}));
        assert(bm.whnf(ep1.var(317)) == ep2.functor("y", {}));
        assert(bm.whnf(ep1.var(318)) == ep3.functor("z", {}));  // transitive: 318→319→ep3.functor("z", {})
        assert(bm.whnf(ep1.var(319)) == ep3.functor("z", {}));
        assert(bm.whnf(ep1.var(320)) == ep2.functor("w", {}));

        t.pop();
    }

    // Test 109: Pre-bound chain where the tail var is also used as a structural position;
    // unification extends the chain via a new binding from yet another pool allocation.
    // var(321) → ep2.var(322),  var(322) unbound.
    // ep3.functor("cons", {var(321), "x"}) unified with ep4.functor("cons", {"a", "x"}):
    //   lhs: var(321)→var(322)→unbound → bind 322 → ep4.functor("a", {})
    //   rhs: "x" == "x" ✓
    // var(321) from ep1 (a different allocation) now also resolves to ep4.functor("a", {}).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t);

        bm.bindings[321] = ep2.var(322);

        const expr* A = ep3.functor("cons", {ep3.var(321), ep3.functor("x", {})});
        const expr* B = ep4.functor("cons", {ep4.functor("a", {}), ep4.functor("x", {})});

        assert(bm.unify(A, B));
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.count(322) == 1);

        assert(bm.whnf(ep1.var(321)) == ep4.functor("a", {}));  // ep1 allocation, same chain
        assert(bm.whnf(ep3.var(322)) == ep4.functor("a", {}));

        t.pop();
    }

    // Test 110: Maximum cross-pool complexity — every node in both structures from a
    // different pool. Pre-bound var points to a cons from ep2; that cons is structurally
    // matched against an equal cons from ep4 (atoms from ep4, different pointers).
    // A second unbound var in structure A gets bound to an atom from ep5.
    // The spine of structure A lives in ep6; structure B's spine lives in ep5.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t), ep5(t), ep6(t);

        // var(325) pre-bound to a cons from ep2
        bm.bindings[325] = ep2.functor("cons", {ep2.functor("L", {}), ep2.functor("R", {})});

        // Structure A: spine from ep6; var(325) ref from ep1, var(326) ref from ep3
        const expr* A = ep6.functor("cons", {ep1.var(325), ep3.var(326)});

        // Structure B: outer spine from ep5; inner cons from ep4; atoms from ep4
        const expr* B = ep5.functor("cons", {
            ep4.functor("cons", {ep4.functor("L", {}), ep4.functor("R", {})}), // same structure as the pre-binding
            ep5.functor("result", {})
        });

        assert(bm.unify(A, B));
        // lhs: var(325) → ep2.functor("cons", {"L", "R"}); ep4.functor("cons", {"L", "R"}) — different alloc, same shape
        //   "L"=="L" ✓  "R"=="R" ✓  (no new bindings for atoms)
        // rhs: var(326) unbound → bind 326 → ep5.functor("result", {})
        assert(bm.bindings.size() == 2);  // pre-binding 325 + new 326
        assert(bm.bindings.count(326) == 1);
        assert(bm.whnf(ep1.var(325)) == ep2.functor("cons", {ep2.functor("L", {}), ep2.functor("R", {})}));
        assert(bm.whnf(ep3.var(326)) == ep5.functor("result", {}));

        t.pop();
    }
}

void test_lineage_pool_constructor() {
    // Basic construction
    lineage_pool pool1;
    assert(pool1.goal_lineages.size() == 0);
    assert(pool1.resolution_lineages.size() == 0);
    assert(pool1.goal_lineages.empty());
    assert(pool1.resolution_lineages.empty());
}

void test_lineage_pool_intern_goal() {
    // Test 1: Intern a goal_lineage with nullptr parent (root)
    {
        lineage_pool pool;
        assert(pool.goal_lineages.size() == 0);
        
        goal_lineage g1{nullptr, 1};
        const goal_lineage* ptr1 = pool.intern(std::move(g1));
        
        assert(ptr1 != nullptr);
        assert(ptr1->parent == nullptr);
        assert(ptr1->idx == 1);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.goal_lineages.count(*ptr1) == 1);
        assert(pool.goal_lineages.at(*ptr1) == false);  // Not pinned by default
    }
    
    // Test 2: Intern duplicate goal_lineage - should return same pointer
    {
        lineage_pool pool;
        
        goal_lineage g1{nullptr, 1};
        const goal_lineage* ptr1 = pool.intern(std::move(g1));
        
        goal_lineage g2{nullptr, 1};  // Same as g1
        const goal_lineage* ptr2 = pool.intern(std::move(g2));
        
        assert(ptr2 == ptr1);
        assert(pool.goal_lineages.size() == 1);  // No new entry added
    }
    
    // Test 3: Intern goal_lineage with resolution_lineage parent
    {
        lineage_pool pool;
        
        // Create resolution parent
        resolution_lineage r_parent{nullptr, 10};
        const resolution_lineage* r_p = pool.intern(std::move(r_parent));
        assert(pool.resolution_lineages.size() == 1);
        
        goal_lineage g3{r_p, 5};
        const goal_lineage* ptr3 = pool.intern(std::move(g3));
        
        assert(ptr3 != nullptr);
        assert(ptr3->parent == r_p);
        assert(ptr3->idx == 5);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.goal_lineages.at(*ptr3) == false);
    }
    
    // Test 4: Intern multiple goal_lineages with same resolution parent
    {
        lineage_pool pool;
        
        // Create resolution parent
        resolution_lineage r_parent{nullptr, 20};
        const resolution_lineage* parent = pool.intern(std::move(r_parent));
        
        goal_lineage g1{parent, 10};
        const goal_lineage* ptr1 = pool.intern(std::move(g1));
        assert(pool.goal_lineages.size() == 1);
        assert(ptr1->parent == parent);
        assert(ptr1->idx == 10);
        
        goal_lineage g2{parent, 11};
        const goal_lineage* ptr2 = pool.intern(std::move(g2));
        assert(pool.goal_lineages.size() == 2);
        assert(ptr2->parent == parent);
        assert(ptr2->idx == 11);
        assert(ptr1 != ptr2);  // Different idx means different lineage
        
        // Same parent, same idx - duplicate
        goal_lineage g3{parent, 10};
        const goal_lineage* ptr3 = pool.intern(std::move(g3));
        assert(ptr3 == ptr1);  // Should be same pointer
        assert(pool.goal_lineages.size() == 2);  // No new entry
    }
    
    // Test 5: Different resolution parents, same idx
    {
        lineage_pool pool;
        
        // Create two different resolution parents
        resolution_lineage r1{nullptr, 1};
        const resolution_lineage* parent1 = pool.intern(std::move(r1));
        
        resolution_lineage r2{nullptr, 2};
        const resolution_lineage* parent2 = pool.intern(std::move(r2));
        assert(parent1 != parent2);
        
        goal_lineage g1{parent1, 99};
        const goal_lineage* ptr1 = pool.intern(std::move(g1));
        assert(pool.goal_lineages.size() == 1);
        assert(ptr1->parent == parent1);
        
        goal_lineage g2{parent2, 99};
        const goal_lineage* ptr2 = pool.intern(std::move(g2));
        assert(ptr2 != ptr1);  // Different parents, so different lineages
        assert(pool.goal_lineages.size() == 2);
        assert(ptr2->parent == parent2);
        assert(ptr2->idx == 99);
    }
    
    // Test 6: Integration with alternating chain
    {
        lineage_pool pool;
        
        // Root: goal_lineage
        goal_lineage g1{nullptr, 1};
        const goal_lineage* p1 = pool.intern(std::move(g1));
        assert(pool.goal_lineages.size() == 1);
        
        // Level 1: resolution_lineage
        resolution_lineage r1{p1, 2};
        const resolution_lineage* p2 = pool.intern(std::move(r1));
        
        // Level 2: goal_lineage with resolution parent
        goal_lineage g2{p2, 3};
        const goal_lineage* p3 = pool.intern(std::move(g2));
        assert(pool.goal_lineages.size() == 2);
        assert(p3->idx == 3);
        assert(p3->parent == p2);
        
        // Verify chain
        assert(p3->parent->parent == p1);
        assert(p1->parent == nullptr);
    }
    
    // Test 7: Edge case - idx = 0
    {
        lineage_pool pool;
        
        goal_lineage g_zero{nullptr, 0};
        const goal_lineage* ptr_zero = pool.intern(std::move(g_zero));
        assert(ptr_zero->idx == 0);
        assert(ptr_zero->parent == nullptr);
        assert(pool.goal_lineages.size() == 1);
    }
    
    // Test 8: Edge case - idx = SIZE_MAX
    {
        lineage_pool pool;
        
        goal_lineage g_max{nullptr, SIZE_MAX};
        const goal_lineage* ptr_max = pool.intern(std::move(g_max));
        assert(ptr_max->idx == SIZE_MAX);
        assert(ptr_max->parent == nullptr);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.goal_lineages.at(*ptr_max) == false);
    }
}

void test_lineage_pool_intern_resolution() {
    // Test 1: Intern a resolution_lineage with nullptr parent (root)
    {
        lineage_pool pool;
        assert(pool.resolution_lineages.size() == 0);
        
        resolution_lineage r1{nullptr, 2};
        const resolution_lineage* ptr1 = pool.intern(std::move(r1));
        
        assert(ptr1 != nullptr);
        assert(ptr1->parent == nullptr);
        assert(ptr1->idx == 2);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.resolution_lineages.at(*ptr1) == false);  // Not pinned by default
    }
    
    // Test 2: Intern duplicate resolution_lineage - should return same pointer
    {
        lineage_pool pool;
        
        resolution_lineage r1{nullptr, 5};
        const resolution_lineage* ptr1 = pool.intern(std::move(r1));
        
        resolution_lineage r2{nullptr, 5};  // Same as r1
        const resolution_lineage* ptr2 = pool.intern(std::move(r2));
        
        assert(ptr2 == ptr1);
        assert(pool.resolution_lineages.size() == 1);  // No new entry added
    }
    
    // Test 3: Intern resolution_lineage with goal_lineage parent
    {
        lineage_pool pool;
        
        // Create goal parent
        goal_lineage g_parent{nullptr, 20};
        const goal_lineage* g_p = pool.intern(std::move(g_parent));
        assert(pool.goal_lineages.size() == 1);
        
        resolution_lineage r2{g_p, 7};
        const resolution_lineage* ptr2 = pool.intern(std::move(r2));
        
        assert(ptr2 != nullptr);
        assert(ptr2->parent == g_p);
        assert(ptr2->idx == 7);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.resolution_lineages.at(*ptr2) == false);
    }
    
    // Test 4: Intern multiple resolution_lineages with same goal parent
    {
        lineage_pool pool;
        
        // Create goal parent
        goal_lineage g_parent{nullptr, 2};
        const goal_lineage* parent = pool.intern(std::move(g_parent));
        
        resolution_lineage r1{parent, 10};
        const resolution_lineage* ptr1 = pool.intern(std::move(r1));
        assert(pool.resolution_lineages.size() == 1);
        assert(ptr1->parent == parent);
        assert(ptr1->idx == 10);
        
        resolution_lineage r2{parent, 11};
        const resolution_lineage* ptr2 = pool.intern(std::move(r2));
        assert(pool.resolution_lineages.size() == 2);
        assert(ptr2->parent == parent);
        assert(ptr2->idx == 11);
        assert(ptr1 != ptr2);  // Different idx means different lineage
        
        // Same parent, same idx - duplicate
        resolution_lineage r3{parent, 10};
        const resolution_lineage* ptr3 = pool.intern(std::move(r3));
        assert(ptr3 == ptr1);  // Should be same pointer
        assert(pool.resolution_lineages.size() == 2);  // No new entry
    }
    
    // Test 5: Different goal parents, same idx
    {
        lineage_pool pool;
        
        // Create two different goal parents
        goal_lineage g1{nullptr, 1};
        const goal_lineage* parent1 = pool.intern(std::move(g1));
        
        goal_lineage g2{nullptr, 2};
        const goal_lineage* parent2 = pool.intern(std::move(g2));
        assert(parent1 != parent2);
        
        resolution_lineage r1{parent1, 99};
        const resolution_lineage* ptr1 = pool.intern(std::move(r1));
        assert(pool.resolution_lineages.size() == 1);
        assert(ptr1->parent == parent1);
        
        resolution_lineage r2{parent2, 99};
        const resolution_lineage* ptr2 = pool.intern(std::move(r2));
        assert(ptr2 != ptr1);  // Different parents, so different lineages
        assert(pool.resolution_lineages.size() == 2);
        assert(ptr2->parent == parent2);
        assert(ptr2->idx == 99);
    }
    
    // Test 6: Integration with alternating chain
    {
        lineage_pool pool;
        
        // Root: goal_lineage
        goal_lineage g1{nullptr, 1};
        const goal_lineage* p1 = pool.intern(std::move(g1));
        
        // Level 1: resolution_lineage with goal parent
        resolution_lineage r1{p1, 2};
        const resolution_lineage* p2 = pool.intern(std::move(r1));
        assert(pool.resolution_lineages.size() == 1);
        assert(p2->idx == 2);
        assert(p2->parent == p1);
        
        // Level 2: goal_lineage
        goal_lineage g2{p2, 3};
        const goal_lineage* p3 = pool.intern(std::move(g2));
        
        // Level 3: resolution_lineage with goal parent
        resolution_lineage r2{p3, 4};
        const resolution_lineage* p4 = pool.intern(std::move(r2));
        assert(pool.resolution_lineages.size() == 2);
        assert(p4->idx == 4);
        assert(p4->parent == p3);
        
        // Verify chain
        assert(p4->parent->parent == p2);
        assert(p2->parent == p1);
    }
    
    // Test 7: Edge case - idx = 0
    {
        lineage_pool pool;
        
        resolution_lineage r_zero{nullptr, 0};
        const resolution_lineage* ptr_zero = pool.intern(std::move(r_zero));
        assert(ptr_zero->idx == 0);
        assert(ptr_zero->parent == nullptr);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.resolution_lineages.at(*ptr_zero) == false);
    }
    
    // Test 8: Edge case - idx = SIZE_MAX
    {
        lineage_pool pool;
        
        resolution_lineage r_max{nullptr, SIZE_MAX};
        const resolution_lineage* ptr_max = pool.intern(std::move(r_max));
        assert(ptr_max->idx == SIZE_MAX);
        assert(ptr_max->parent == nullptr);
        assert(pool.resolution_lineages.size() == 1);
    }
}

void test_lineage_pool_goal() {
    // Test 1: Create root goal_lineage (nullptr parent)
    {
        lineage_pool pool;
        assert(pool.goal_lineages.size() == 0);
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        
        assert(g1 != nullptr);
        assert(g1->parent == nullptr);
        assert(g1->idx == 1);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.goal_lineages.count(*g1) == 1);
        assert(pool.goal_lineages.at(*g1) == false);
    }
    
    // Test 2: Create duplicate root - should return same pointer
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const goal_lineage* g2 = pool.goal(nullptr, 1);
        
        assert(g2 == g1);  // Should be interned to same pointer
        assert(pool.goal_lineages.size() == 1);  // No new entry
    }
    
    // Test 3: Create goal_lineage with resolution parent
    {
        lineage_pool pool;
        
        const resolution_lineage* parent = pool.resolution(nullptr, 30);
        assert(pool.resolution_lineages.size() == 1);
        
        const goal_lineage* child = pool.goal(parent, 31);
        
        assert(child != nullptr);
        assert(child->parent == parent);
        assert(child->idx == 31);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.goal_lineages.count(*child) == 1);
    }
    
    // Test 4: Create multiple goal children from same resolution parent
    {
        lineage_pool pool;
        
        const resolution_lineage* parent = pool.resolution(nullptr, 30);
        assert(pool.resolution_lineages.size() == 1);
        
        const goal_lineage* child1 = pool.goal(parent, 31);
        assert(pool.goal_lineages.size() == 1);
        assert(child1->parent == parent);
        assert(child1->idx == 31);
        
        const goal_lineage* child2 = pool.goal(parent, 32);
        assert(pool.goal_lineages.size() == 2);
        assert(child2->parent == parent);
        assert(child2->idx == 32);
        
        const goal_lineage* child3 = pool.goal(parent, 33);
        assert(pool.goal_lineages.size() == 3);
        assert(child3->parent == parent);
        assert(child3->idx == 33);
        
        // All children should be distinct
        assert(child1 != child2);
        assert(child2 != child3);
        assert(child1 != child3);
    }
    
    // Test 5: Create duplicate goal child - should return same pointer
    {
        lineage_pool pool;
        
        const resolution_lineage* parent = pool.resolution(nullptr, 20);
        const goal_lineage* original = pool.goal(parent, 21);
        size_t size_before = pool.goal_lineages.size();
        
        const goal_lineage* duplicate = pool.goal(parent, 21);
        
        assert(duplicate == original);  // Should be same pointer
        assert(pool.goal_lineages.size() == size_before);  // No new entry
    }
    
    // Test 6: Different resolution parents, same idx
    {
        lineage_pool pool;
        
        const resolution_lineage* parent1 = pool.resolution(nullptr, 100);
        const resolution_lineage* parent2 = pool.resolution(nullptr, 101);
        assert(parent1 != parent2);
        
        const goal_lineage* child1 = pool.goal(parent1, 50);
        assert(pool.goal_lineages.size() == 1);
        
        const goal_lineage* child2 = pool.goal(parent2, 50);
        assert(pool.goal_lineages.size() == 2);
        
        // Different parents means different lineages
        assert(child1 != child2);
        assert(child1->parent == parent1);
        assert(child2->parent == parent2);
        assert(child1->idx == child2->idx);
    }
    
    // Test 7: Branching structure - multiple goals from different resolution parents
    {
        lineage_pool pool;
        
        // Create resolution branches
        const resolution_lineage* r1 = pool.resolution(nullptr, 201);
        const resolution_lineage* r2 = pool.resolution(nullptr, 202);
        const resolution_lineage* r3 = pool.resolution(nullptr, 203);
        
        // Create goal children from each branch
        const goal_lineage* g1 = pool.goal(r1, 311);
        const goal_lineage* g2 = pool.goal(r2, 312);
        const goal_lineage* g3 = pool.goal(r3, 313);
        
        assert(pool.goal_lineages.size() == 3);
        assert(g1->parent == r1);
        assert(g2->parent == r2);
        assert(g3->parent == r3);
        assert(g1 != g2);
        assert(g2 != g3);
        assert(g1 != g3);
    }
    
    // Test 8: Edge case - idx = 0
    {
        lineage_pool pool;
        
        const goal_lineage* g0 = pool.goal(nullptr, 0);
        assert(g0->idx == 0);
        assert(g0->parent == nullptr);
        assert(pool.goal_lineages.size() == 1);
    }
    
    // Test 9: Edge case - idx = SIZE_MAX
    {
        lineage_pool pool;
        
        const resolution_lineage* r_parent = pool.resolution(nullptr, 1);
        const goal_lineage* g_max = pool.goal(r_parent, SIZE_MAX);
        assert(g_max->idx == SIZE_MAX);
        assert(g_max->parent == r_parent);
        assert(pool.goal_lineages.size() == 1);
    }
    
    // Test 10: Integration - alternating chain with goal at multiple levels
    {
        lineage_pool pool;
        
        // Level 0: goal root
        const goal_lineage* level0 = pool.goal(nullptr, 100);
        assert(level0->idx == 100);
        assert(pool.goal_lineages.size() == 1);
        
        // Level 1: resolution
        const resolution_lineage* level1 = pool.resolution(level0, 101);
        
        // Level 2: goal with resolution parent
        const goal_lineage* level2 = pool.goal(level1, 102);
        assert(pool.goal_lineages.size() == 2);
        assert(level2->parent == level1);
        assert(level2->idx == 102);
        
        // Level 3: resolution
        const resolution_lineage* level3 = pool.resolution(level2, 103);
        
        // Level 4: goal with resolution parent
        const goal_lineage* level4 = pool.goal(level3, 104);
        assert(pool.goal_lineages.size() == 3);
        assert(level4->parent == level3);
        assert(level4->idx == 104);
        
        // Verify goal lineages in the chain
        assert(level4->parent->parent == level2);
        assert(level2->parent->parent == level0);
        assert(level0->parent == nullptr);
    }
}

void test_lineage_pool_resolution() {
    // Test 1: Create root resolution_lineage (nullptr parent)
    {
        lineage_pool pool;
        assert(pool.resolution_lineages.size() == 0);
        
        const resolution_lineage* r1 = pool.resolution(nullptr, 5);
        
        assert(r1 != nullptr);
        assert(r1->parent == nullptr);
        assert(r1->idx == 5);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.resolution_lineages.count(*r1) == 1);
        assert(pool.resolution_lineages.at(*r1) == false);
    }
    
    // Test 2: Create duplicate root - should return same pointer
    {
        lineage_pool pool;
        
        const resolution_lineage* r1 = pool.resolution(nullptr, 5);
        const resolution_lineage* r2 = pool.resolution(nullptr, 5);
        
        assert(r2 == r1);  // Should be interned to same pointer
        assert(pool.resolution_lineages.size() == 1);  // No new entry
    }
    
    // Test 3: Create resolution_lineage with goal_lineage parent
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 10);
        assert(pool.goal_lineages.size() == 1);
        
        const resolution_lineage* child = pool.resolution(parent, 11);
        
        assert(child != nullptr);
        assert(child->parent == parent);
        assert(child->idx == 11);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.resolution_lineages.count(*child) == 1);
    }
    
    // Test 4: Create multiple resolution children from same goal parent
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 20);
        assert(pool.goal_lineages.size() == 1);
        
        const resolution_lineage* child1 = pool.resolution(parent, 21);
        assert(pool.resolution_lineages.size() == 1);
        assert(child1->parent == parent);
        assert(child1->idx == 21);
        
        const resolution_lineage* child2 = pool.resolution(parent, 22);
        assert(pool.resolution_lineages.size() == 2);
        assert(child2->parent == parent);
        assert(child2->idx == 22);
        
        const resolution_lineage* child3 = pool.resolution(parent, 23);
        assert(pool.resolution_lineages.size() == 3);
        assert(child3->parent == parent);
        assert(child3->idx == 23);
        
        // All children should be distinct
        assert(child1 != child2);
        assert(child2 != child3);
        assert(child1 != child3);
    }
    
    // Test 5: Create duplicate resolution child - should return same pointer
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 20);
        const resolution_lineage* original = pool.resolution(parent, 21);
        size_t size_before = pool.resolution_lineages.size();
        
        const resolution_lineage* duplicate = pool.resolution(parent, 21);
        
        assert(duplicate == original);  // Should be same pointer
        assert(pool.resolution_lineages.size() == size_before);  // No new entry
    }
    
    // Test 6: Different goal parents, same idx
    {
        lineage_pool pool;
        
        const goal_lineage* parent1 = pool.goal(nullptr, 200);
        const goal_lineage* parent2 = pool.goal(nullptr, 201);
        assert(parent1 != parent2);
        
        const resolution_lineage* child1 = pool.resolution(parent1, 50);
        assert(pool.resolution_lineages.size() == 1);
        
        const resolution_lineage* child2 = pool.resolution(parent2, 50);
        assert(pool.resolution_lineages.size() == 2);
        
        // Different parents means different lineages
        assert(child1 != child2);
        assert(child1->parent == parent1);
        assert(child2->parent == parent2);
        assert(child1->idx == child2->idx);
    }
    
    // Test 7: Branching structure - multiple resolutions from same goal parent
    {
        lineage_pool pool;
        
        // Goal root
        const goal_lineage* root = pool.goal(nullptr, 300);
        
        // Create multiple resolution branches from goal root
        const resolution_lineage* branch1 = pool.resolution(root, 301);
        const resolution_lineage* branch2 = pool.resolution(root, 302);
        const resolution_lineage* branch3 = pool.resolution(root, 303);
        
        assert(pool.resolution_lineages.size() == 3);
        assert(branch1->parent == root);
        assert(branch2->parent == root);
        assert(branch3->parent == root);
        assert(branch1 != branch2);
        assert(branch2 != branch3);
        assert(branch1 != branch3);
    }
    
    // Test 8: Edge case - idx = 0
    {
        lineage_pool pool;
        
        const resolution_lineage* r0 = pool.resolution(nullptr, 0);
        assert(r0->idx == 0);
        assert(r0->parent == nullptr);
        assert(pool.resolution_lineages.size() == 1);
    }
    
    // Test 9: Edge case - idx = SIZE_MAX
    {
        lineage_pool pool;
        
        const goal_lineage* g_parent = pool.goal(nullptr, 1);
        const resolution_lineage* r_max = pool.resolution(g_parent, SIZE_MAX);
        assert(r_max->idx == SIZE_MAX);
        assert(r_max->parent == g_parent);
        assert(pool.resolution_lineages.size() == 1);
    }
    
    // Test 10: Integration - alternating chain with resolution at multiple levels
    {
        lineage_pool pool;
        
        // Level 0: goal root
        const goal_lineage* level0 = pool.goal(nullptr, 100);
        
        // Level 1: resolution with goal parent
        const resolution_lineage* level1 = pool.resolution(level0, 101);
        assert(pool.resolution_lineages.size() == 1);
        assert(level1->parent == level0);
        assert(level1->idx == 101);
        
        // Level 2: goal
        const goal_lineage* level2 = pool.goal(level1, 102);
        
        // Level 3: resolution with goal parent
        const resolution_lineage* level3 = pool.resolution(level2, 103);
        assert(pool.resolution_lineages.size() == 2);
        assert(level3->parent == level2);
        assert(level3->idx == 103);
        
        // Level 4: goal
        const goal_lineage* level4 = pool.goal(level3, 104);
        
        // Verify resolution lineages in the chain
        assert(level3->parent->parent == level1);
        assert(level1->parent == level0);
        assert(level4->parent == level3);
    }
}

void test_lineage_pool_pin_goal() {
    // Test 1: Pin nullptr - should not crash
    {
        lineage_pool pool;
        
        const goal_lineage* g_null = nullptr;
        pool.pin(g_null);  // Should return immediately
        assert(pool.goal_lineages.size() == 0);
        assert(pool.resolution_lineages.size() == 0);
    }
    
    // Test 2: Pin a single root goal_lineage
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        assert(pool.goal_lineages.at(*g1) == false);
        
        pool.pin(g1);
        assert(pool.goal_lineages.at(*g1) == true);  // Now pinned
        assert(pool.goal_lineages.size() == 1);
    }
    
    // Test 3: Pin the same goal_lineage twice - should be idempotent
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        assert(pool.goal_lineages.at(*g1) == false);
        
        pool.pin(g1);
        assert(pool.goal_lineages.at(*g1) == true);
        
        pool.pin(g1);  // Pin again
        assert(pool.goal_lineages.at(*g1) == true);  // Still pinned
        assert(pool.goal_lineages.size() == 1);
    }
    
    // Test 4: Pin goal child - should pin resolution parent chain to root
    {
        lineage_pool pool;
        
        const resolution_lineage* parent = pool.resolution(nullptr, 1);
        const goal_lineage* child = pool.goal(parent, 2);
        
        assert(pool.resolution_lineages.at(*parent) == false);
        assert(pool.goal_lineages.at(*child) == false);
        
        pool.pin(child);
        
        // Both child and parent should be pinned
        assert(pool.goal_lineages.at(*child) == true);
        assert(pool.resolution_lineages.at(*parent) == true);
    }
    
    // Test 5: Pin deep goal in alternating chain - all ancestors should be pinned
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const resolution_lineage* r1 = pool.resolution(g1, 2);
        const goal_lineage* g2 = pool.goal(r1, 3);
        const resolution_lineage* r2 = pool.resolution(g2, 4);
        const goal_lineage* g3 = pool.goal(r2, 5);
        
        // All should be unpinned initially
        assert(pool.goal_lineages.at(*g1) == false);
        assert(pool.resolution_lineages.at(*r1) == false);
        assert(pool.goal_lineages.at(*g2) == false);
        assert(pool.resolution_lineages.at(*r2) == false);
        assert(pool.goal_lineages.at(*g3) == false);
        
        pool.pin(g3);
        
        // All should be pinned now
        assert(pool.goal_lineages.at(*g1) == true);
        assert(pool.resolution_lineages.at(*r1) == true);
        assert(pool.goal_lineages.at(*g2) == true);
        assert(pool.resolution_lineages.at(*r2) == true);
        assert(pool.goal_lineages.at(*g3) == true);
    }
    
    // Test 6: Pin multiple goal branches - shared ancestors pinned once
    {
        lineage_pool pool;
        
        const goal_lineage* root = pool.goal(nullptr, 1);
        const resolution_lineage* branch1 = pool.resolution(root, 2);
        const resolution_lineage* branch2 = pool.resolution(root, 3);
        const goal_lineage* leaf1 = pool.goal(branch1, 4);
        const goal_lineage* leaf2 = pool.goal(branch2, 5);
        
        pool.pin(leaf1);
        
        // leaf1, branch1, and root should be pinned
        assert(pool.goal_lineages.at(*root) == true);
        assert(pool.resolution_lineages.at(*branch1) == true);
        assert(pool.goal_lineages.at(*leaf1) == true);
        // branch2 and leaf2 should still be unpinned
        assert(pool.resolution_lineages.at(*branch2) == false);
        assert(pool.goal_lineages.at(*leaf2) == false);
        
        pool.pin(leaf2);
        
        // Now everything should be pinned
        assert(pool.goal_lineages.at(*root) == true);
        assert(pool.resolution_lineages.at(*branch1) == true);
        assert(pool.resolution_lineages.at(*branch2) == true);
        assert(pool.goal_lineages.at(*leaf1) == true);
        assert(pool.goal_lineages.at(*leaf2) == true);
    }
    
    // Test 7: Pin goal parent after resolution child already pinned it
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 1);
        const resolution_lineage* child = pool.resolution(parent, 2);
        
        pool.pin(child);
        assert(pool.goal_lineages.at(*parent) == true);
        assert(pool.resolution_lineages.at(*child) == true);
        
        // Pin parent again - should be no-op since already pinned
        pool.pin(parent);
        assert(pool.goal_lineages.at(*parent) == true);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 1);
    }
    
    // Test 8: Complex AND-OR tree - pin deep goal
    {
        lineage_pool pool;
        
        const goal_lineage* root = pool.goal(nullptr, 0);
        const resolution_lineage* l1_a = pool.resolution(root, 1);
        const resolution_lineage* l1_b = pool.resolution(root, 2);
        const goal_lineage* l2_a = pool.goal(l1_a, 3);
        const goal_lineage* l2_b = pool.goal(l1_a, 4);
        const goal_lineage* l2_c = pool.goal(l1_b, 5);
        const resolution_lineage* l3_a = pool.resolution(l2_a, 6);
        const goal_lineage* l4_a = pool.goal(l3_a, 7);
        
        // Pin l4_a (goal) - should pin l4_a, l3_a, l2_a, l1_a, root
        pool.pin(l4_a);
        assert(pool.goal_lineages.at(*root) == true);
        assert(pool.resolution_lineages.at(*l1_a) == true);
        assert(pool.resolution_lineages.at(*l1_b) == false);
        assert(pool.goal_lineages.at(*l2_a) == true);
        assert(pool.goal_lineages.at(*l2_b) == false);
        assert(pool.goal_lineages.at(*l2_c) == false);
        assert(pool.resolution_lineages.at(*l3_a) == true);
        assert(pool.goal_lineages.at(*l4_a) == true);
    }
    
    // Test 9: Pin goal parent does NOT pin resolution children
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 1);
        const resolution_lineage* child1 = pool.resolution(parent, 2);
        const resolution_lineage* child2 = pool.resolution(parent, 3);
        const goal_lineage* grandchild = pool.goal(child1, 4);
        
        // All should be unpinned initially
        assert(pool.goal_lineages.at(*parent) == false);
        assert(pool.resolution_lineages.at(*child1) == false);
        assert(pool.resolution_lineages.at(*child2) == false);
        assert(pool.goal_lineages.at(*grandchild) == false);
        
        // Pin only the parent
        pool.pin(parent);
        
        // Only parent should be pinned, children should remain unpinned
        assert(pool.goal_lineages.at(*parent) == true);
        assert(pool.resolution_lineages.at(*child1) == false);
        assert(pool.resolution_lineages.at(*child2) == false);
        assert(pool.goal_lineages.at(*grandchild) == false);
    }
}

void test_lineage_pool_pin_resolution() {
    // Test 1: Pin nullptr - should not crash
    {
        lineage_pool pool;
        
        const resolution_lineage* r_null = nullptr;
        pool.pin(r_null);  // Should return immediately
        assert(pool.goal_lineages.size() == 0);
        assert(pool.resolution_lineages.size() == 0);
    }
    
    // Test 2: Pin a single root resolution_lineage
    {
        lineage_pool pool;
        
        const resolution_lineage* r1 = pool.resolution(nullptr, 1);
        assert(pool.resolution_lineages.at(*r1) == false);
        
        pool.pin(r1);
        assert(pool.resolution_lineages.at(*r1) == true);  // Now pinned
        assert(pool.resolution_lineages.size() == 1);
    }
    
    // Test 3: Pin the same resolution_lineage twice - should be idempotent
    {
        lineage_pool pool;
        
        const resolution_lineage* r1 = pool.resolution(nullptr, 5);
        assert(pool.resolution_lineages.at(*r1) == false);
        
        pool.pin(r1);
        assert(pool.resolution_lineages.at(*r1) == true);
        
        pool.pin(r1);  // Pin again
        assert(pool.resolution_lineages.at(*r1) == true);  // Still pinned
        assert(pool.resolution_lineages.size() == 1);
    }
    
    // Test 4: Pin resolution child - should pin goal parent chain to root
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 1);
        const resolution_lineage* child = pool.resolution(parent, 2);
        
        assert(pool.goal_lineages.at(*parent) == false);
        assert(pool.resolution_lineages.at(*child) == false);
        
        pool.pin(child);
        
        // Both child and parent should be pinned
        assert(pool.resolution_lineages.at(*child) == true);
        assert(pool.goal_lineages.at(*parent) == true);
    }
    
    // Test 5: Pin deep resolution in alternating chain - all ancestors should be pinned
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const resolution_lineage* r1 = pool.resolution(g1, 2);
        const goal_lineage* g2 = pool.goal(r1, 3);
        const resolution_lineage* r2 = pool.resolution(g2, 4);
        const goal_lineage* g3 = pool.goal(r2, 5);
        const resolution_lineage* r3 = pool.resolution(g3, 6);
        
        // All should be unpinned initially
        assert(pool.goal_lineages.at(*g1) == false);
        assert(pool.resolution_lineages.at(*r1) == false);
        assert(pool.goal_lineages.at(*g2) == false);
        assert(pool.resolution_lineages.at(*r2) == false);
        assert(pool.goal_lineages.at(*g3) == false);
        assert(pool.resolution_lineages.at(*r3) == false);
        
        pool.pin(r3);
        
        // All should be pinned now
        assert(pool.goal_lineages.at(*g1) == true);
        assert(pool.resolution_lineages.at(*r1) == true);
        assert(pool.goal_lineages.at(*g2) == true);
        assert(pool.resolution_lineages.at(*r2) == true);
        assert(pool.goal_lineages.at(*g3) == true);
        assert(pool.resolution_lineages.at(*r3) == true);
    }
    
    // Test 6: Pin sibling resolution nodes independently
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 1);
        const resolution_lineage* child1 = pool.resolution(parent, 2);
        const resolution_lineage* child2 = pool.resolution(parent, 3);
        const resolution_lineage* child3 = pool.resolution(parent, 4);
        
        // Pin only child1
        pool.pin(child1);
        assert(pool.goal_lineages.at(*parent) == true);
        assert(pool.resolution_lineages.at(*child1) == true);
        assert(pool.resolution_lineages.at(*child2) == false);
        assert(pool.resolution_lineages.at(*child3) == false);
        
        // Pin child3
        pool.pin(child3);
        assert(pool.goal_lineages.at(*parent) == true);
        assert(pool.resolution_lineages.at(*child1) == true);
        assert(pool.resolution_lineages.at(*child2) == false);  // Still unpinned
        assert(pool.resolution_lineages.at(*child3) == true);
    }
    
    // Test 7: Pin resolution parent after goal child already pinned it
    {
        lineage_pool pool;
        
        const resolution_lineage* parent = pool.resolution(nullptr, 1);
        const goal_lineage* child = pool.goal(parent, 2);
        
        pool.pin(child);
        assert(pool.resolution_lineages.at(*parent) == true);
        assert(pool.goal_lineages.at(*child) == true);
        
        // Pin parent again - should be no-op since already pinned
        pool.pin(parent);
        assert(pool.resolution_lineages.at(*parent) == true);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.goal_lineages.size() == 1);
    }
    
    // Test 8: Complex AND-OR tree - pin deep resolution
    {
        lineage_pool pool;
        
        const goal_lineage* root = pool.goal(nullptr, 0);
        const resolution_lineage* l1_a = pool.resolution(root, 1);
        const resolution_lineage* l1_b = pool.resolution(root, 2);
        const goal_lineage* l2_a = pool.goal(l1_a, 3);
        const goal_lineage* l2_b = pool.goal(l1_a, 4);
        const goal_lineage* l2_c = pool.goal(l1_b, 5);
        const resolution_lineage* l3_a = pool.resolution(l2_a, 6);
        
        // Pin l3_a (resolution) - should pin l3_a, l2_a, l1_a, root
        pool.pin(l3_a);
        assert(pool.goal_lineages.at(*root) == true);
        assert(pool.resolution_lineages.at(*l1_a) == true);
        assert(pool.resolution_lineages.at(*l1_b) == false);
        assert(pool.goal_lineages.at(*l2_a) == true);
        assert(pool.goal_lineages.at(*l2_b) == false);
        assert(pool.goal_lineages.at(*l2_c) == false);
        assert(pool.resolution_lineages.at(*l3_a) == true);
    }
    
    // Test 9: Pin resolution parent does NOT pin goal children
    {
        lineage_pool pool;
        
        const resolution_lineage* parent = pool.resolution(nullptr, 1);
        const goal_lineage* child1 = pool.goal(parent, 2);
        const goal_lineage* child2 = pool.goal(parent, 3);
        const resolution_lineage* grandchild = pool.resolution(child1, 4);
        
        // All should be unpinned initially
        assert(pool.resolution_lineages.at(*parent) == false);
        assert(pool.goal_lineages.at(*child1) == false);
        assert(pool.goal_lineages.at(*child2) == false);
        assert(pool.resolution_lineages.at(*grandchild) == false);
        
        // Pin only the parent
        pool.pin(parent);
        
        // Only parent should be pinned, children should remain unpinned
        assert(pool.resolution_lineages.at(*parent) == true);
        assert(pool.goal_lineages.at(*child1) == false);
        assert(pool.goal_lineages.at(*child2) == false);
        assert(pool.resolution_lineages.at(*grandchild) == false);
    }
}

void test_lineage_pool_trim() {
    // Test 1: Trim empty pool - should not crash
    {
        lineage_pool pool;
        
        pool.trim();
        assert(pool.goal_lineages.size() == 0);
        assert(pool.resolution_lineages.size() == 0);
    }
    
    // Test 2: Trim pool with all unpinned entries - should remove all
    {
        lineage_pool pool;
        
        pool.goal(nullptr, 1);
        pool.resolution(nullptr, 2);
        pool.goal(nullptr, 3);
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        
        pool.trim();
        assert(pool.goal_lineages.size() == 0);
        assert(pool.resolution_lineages.size() == 0);
        assert(pool.goal_lineages.empty());
        assert(pool.resolution_lineages.empty());
    }
    
    // Test 3: Trim pool with all pinned entries - should remove none
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const resolution_lineage* r1 = pool.resolution(nullptr, 2);
        const goal_lineage* g2 = pool.goal(nullptr, 3);
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        
        pool.pin(g1);
        pool.pin(r1);
        pool.pin(g2);
        
        pool.trim();
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.goal_lineages.count(*g1) == 1);
        assert(pool.resolution_lineages.count(*r1) == 1);
        assert(pool.goal_lineages.count(*g2) == 1);
    }
    
    // Test 4: Trim pool with mixed pinned/unpinned - remove only unpinned
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const resolution_lineage* r1 = pool.resolution(nullptr, 2);
        const goal_lineage* g2 = pool.goal(nullptr, 3);
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        
        pool.pin(r1);  // Pin only r1
        
        pool.trim();
        assert(pool.goal_lineages.size() == 0);  // Both goals removed
        assert(pool.resolution_lineages.size() == 1);  // Only r1 remains
        assert(pool.resolution_lineages.count(*r1) == 1);
        assert(pool.resolution_lineages.at(*r1) == true);
    }
    
    // Test 5: Trim preserves pinned chain
    {
        lineage_pool pool;
        
        const goal_lineage* parent = pool.goal(nullptr, 1);
        const resolution_lineage* child = pool.resolution(parent, 2);
        const goal_lineage* unrelated = pool.goal(nullptr, 3);
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        
        pool.pin(child);  // This pins both child and parent
        
        pool.trim();
        assert(pool.goal_lineages.size() == 1);  // parent remains, unrelated removed
        assert(pool.resolution_lineages.size() == 1);  // child remains
        assert(pool.goal_lineages.count(*parent) == 1);
        assert(pool.resolution_lineages.count(*child) == 1);
        assert(pool.goal_lineages.at(*parent) == true);
        assert(pool.resolution_lineages.at(*child) == true);
    }
    
    // Test 6: Trim with branching structure
    {
        lineage_pool pool;
        
        const goal_lineage* root = pool.goal(nullptr, 1);
        const resolution_lineage* branch1 = pool.resolution(root, 2);
        const resolution_lineage* branch2 = pool.resolution(root, 3);
        const goal_lineage* leaf1 = pool.goal(branch1, 4);
        const goal_lineage* leaf2 = pool.goal(branch2, 5);
        assert(pool.goal_lineages.size() == 3);
        assert(pool.resolution_lineages.size() == 2);
        
        pool.pin(leaf1);  // Pins leaf1, branch1, root
        
        pool.trim();
        assert(pool.goal_lineages.size() == 2);  // root, leaf1 remain
        assert(pool.resolution_lineages.size() == 1);  // branch1 remains
        assert(pool.goal_lineages.count(*root) == 1);
        assert(pool.resolution_lineages.count(*branch1) == 1);
        assert(pool.goal_lineages.count(*leaf1) == 1);
    }
    
    // Test 7: Multiple trims in sequence
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const resolution_lineage* r1 = pool.resolution(nullptr, 2);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 1);
        
        pool.pin(g1);
        pool.trim();
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 0);
        
        // Add more entries
        const goal_lineage* g2 = pool.goal(nullptr, 3);
        const resolution_lineage* r2 = pool.resolution(nullptr, 4);
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        
        pool.trim();  // g2 and r2 should be removed
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 0);
        assert(pool.goal_lineages.count(*g1) == 1);
    }
    
    // Test 8: Complex AND-OR tree with multiple pin operations and trim
    {
        lineage_pool pool;
        
        // Create a tree structure
        const goal_lineage* root = pool.goal(nullptr, 0);
        const resolution_lineage* l1_a = pool.resolution(root, 1);
        const resolution_lineage* l1_b = pool.resolution(root, 2);
        const goal_lineage* l2_a = pool.goal(l1_a, 3);
        const goal_lineage* l2_b = pool.goal(l1_a, 4);
        const goal_lineage* l2_c = pool.goal(l1_b, 5);
        const resolution_lineage* l3_a = pool.resolution(l2_a, 6);
        assert(pool.goal_lineages.size() == 4);
        assert(pool.resolution_lineages.size() == 3);
        
        // Pin only l3_a
        pool.pin(l3_a);
        
        pool.trim();
        // Should keep: root, l1_a, l2_a, l3_a (the chain)
        // Should remove: l1_b, l2_b, l2_c
        assert(pool.goal_lineages.size() == 2);  // root, l2_a
        assert(pool.resolution_lineages.size() == 2);  // l1_a, l3_a
        assert(pool.goal_lineages.count(*root) == 1);
        assert(pool.resolution_lineages.count(*l1_a) == 1);
        assert(pool.goal_lineages.count(*l2_a) == 1);
        assert(pool.resolution_lineages.count(*l3_a) == 1);
    }
    
    // Test 9: Trim with multiple pinned branches
    {
        lineage_pool pool;
        
        const goal_lineage* root = pool.goal(nullptr, 0);
        const resolution_lineage* b1 = pool.resolution(root, 1);
        const resolution_lineage* b2 = pool.resolution(root, 2);
        const resolution_lineage* b3 = pool.resolution(root, 3);
        const goal_lineage* b1_child = pool.goal(b1, 4);
        const goal_lineage* b3_child = pool.goal(b3, 5);
        assert(pool.goal_lineages.size() == 3);
        assert(pool.resolution_lineages.size() == 3);
        
        pool.pin(b1_child);  // Pins b1_child, b1, root
        pool.pin(b3_child);  // Pins b3_child, b3, root (root already pinned)
        
        pool.trim();
        // Should keep: root, b1, b3, b1_child, b3_child
        // Should remove: b2
        assert(pool.goal_lineages.size() == 3);  // root, b1_child, b3_child
        assert(pool.resolution_lineages.size() == 2);  // b1, b3
        assert(pool.goal_lineages.count(*root) == 1);
        assert(pool.resolution_lineages.count(*b1) == 1);
        assert(pool.resolution_lineages.count(*b3) == 1);
        assert(pool.goal_lineages.count(*b1_child) == 1);
        assert(pool.goal_lineages.count(*b3_child) == 1);
    }
    
    // Test 10: Verify pointers remain valid after trim
    {
        lineage_pool pool;
        
        const goal_lineage* g1 = pool.goal(nullptr, 1);
        const resolution_lineage* r1 = pool.resolution(g1, 2);
        
        // Store values before trim
        const resolution_lineage* g1_parent = g1->parent;
        size_t g1_idx = g1->idx;
        const goal_lineage* r1_parent = r1->parent;
        size_t r1_idx = r1->idx;
        
        pool.pin(r1);
        pool.trim();
        
        // Pointers should still be valid with same values
        assert(g1->parent == g1_parent);
        assert(g1->idx == g1_idx);
        assert(r1->parent == r1_parent);
        assert(r1->idx == r1_idx);
        assert(r1->parent == g1);
    }
}

void test_lineage_pool_import() {

    // Test 1: Import nullptr goal_lineage - must return nullptr without touching pool
    {
        lineage_pool pool;
        const goal_lineage* imported = pool.import(static_cast<const goal_lineage*>(nullptr));
        assert(imported == nullptr);
        assert(pool.goal_lineages.empty());
        assert(pool.resolution_lineages.empty());
    }

    // Test 2: Import nullptr resolution_lineage - must return nullptr without touching pool
    {
        lineage_pool pool;
        const resolution_lineage* imported = pool.import(static_cast<const resolution_lineage*>(nullptr));
        assert(imported == nullptr);
        assert(pool.goal_lineages.empty());
        assert(pool.resolution_lineages.empty());
    }

    // Test 3: Import stack root goal (nullptr parent) - interned in pool
    {
        lineage_pool pool;
        goal_lineage g{nullptr, 7};
        const goal_lineage* imported = pool.import(&g);
        assert(imported != nullptr);
        assert(imported != &g);
        assert(imported->parent == nullptr);
        assert(imported->idx == 7);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 0);
        assert(pool.goal_lineages.count(*imported) == 1);
    }

    // Test 4: Import stack root resolution (nullptr parent) - interned in pool
    {
        lineage_pool pool;
        resolution_lineage r{nullptr, 3};
        const resolution_lineage* imported = pool.import(&r);
        assert(imported != nullptr);
        assert(imported != &r);
        assert(imported->parent == nullptr);
        assert(imported->idx == 3);
        assert(pool.goal_lineages.size() == 0);
        assert(pool.resolution_lineages.size() == 1);
        assert(pool.resolution_lineages.count(*imported) == 1);
    }

    // Test 5: Import a two-level stack chain: goal(resolution(nullptr))
    {
        lineage_pool pool;
        resolution_lineage r{nullptr, 1};
        goal_lineage g{&r, 2};
        const goal_lineage* imported = pool.import(&g);
        assert(imported != nullptr);
        assert(imported != &g);
        assert(imported->idx == 2);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 1);
        const resolution_lineage* pool_r = imported->parent;
        assert(pool_r != nullptr);
        assert(pool_r != &r);
        assert(pool_r->parent == nullptr);
        assert(pool_r->idx == 1);
        assert(pool.resolution_lineages.count(*pool_r) == 1);
    }

    // Test 6: Import a three-level stack chain: goal2(resolution1(goal0(nullptr)))
    {
        lineage_pool pool;
        goal_lineage g0{nullptr, 0};
        resolution_lineage r1{&g0, 1};
        goal_lineage g2{&r1, 2};
        const goal_lineage* imported = pool.import(&g2);
        assert(imported != nullptr);
        assert(imported->idx == 2);
        assert(pool.goal_lineages.size() == 2);
        assert(pool.resolution_lineages.size() == 1);
        const resolution_lineage* pool_r1 = imported->parent;
        assert(pool_r1 != nullptr);
        assert(pool_r1 != &r1);
        assert(pool_r1->idx == 1);
        const goal_lineage* pool_g0 = pool_r1->parent;
        assert(pool_g0 != nullptr);
        assert(pool_g0 != &g0);
        assert(pool_g0->parent == nullptr);
        assert(pool_g0->idx == 0);
    }

    // Test 7: Import goal already entirely in pool - returns same pointer, pool unchanged
    {
        lineage_pool pool;
        const goal_lineage* pool_g = pool.goal(nullptr, 5);
        const goal_lineage* imported = pool.import(pool_g);
        assert(imported == pool_g);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 0);
    }

    // Test 8: Import resolution already entirely in pool - returns same pointer, pool unchanged
    {
        lineage_pool pool;
        const resolution_lineage* pool_r = pool.resolution(nullptr, 5);
        const resolution_lineage* imported = pool.import(pool_r);
        assert(imported == pool_r);
        assert(pool.goal_lineages.size() == 0);
        assert(pool.resolution_lineages.size() == 1);
    }

    // Test 9: Import stack goal structurally identical to existing pool goal - must deduplicate
    {
        lineage_pool pool;
        const goal_lineage* pool_g = pool.goal(nullptr, 9);
        goal_lineage stack_g{nullptr, 9};
        const goal_lineage* imported = pool.import(&stack_g);
        assert(imported == pool_g);
        assert(pool.goal_lineages.size() == 1);
    }

    // Test 10: Import stack resolution structurally identical to existing pool resolution - must deduplicate
    {
        lineage_pool pool;
        const resolution_lineage* pool_r = pool.resolution(nullptr, 4);
        resolution_lineage stack_r{nullptr, 4};
        const resolution_lineage* imported = pool.import(&stack_r);
        assert(imported == pool_r);
        assert(pool.resolution_lineages.size() == 1);
    }

    // Test 11: Stack goal whose resolution parent is value-equal to an existing pool entry -
    // parent deduplicates to existing pool pointer; only the goal is newly added
    {
        lineage_pool pool;
        const resolution_lineage* pool_r = pool.resolution(nullptr, 1);
        resolution_lineage stack_r{nullptr, 1};   // same value, different address
        goal_lineage stack_g{&stack_r, 2};
        const goal_lineage* imported = pool.import(&stack_g);
        assert(imported != nullptr);
        assert(imported->parent == pool_r);        // deduplicates to the existing pool resolution
        assert(imported->idx == 2);
        assert(pool.goal_lineages.size() == 1);
        assert(pool.resolution_lineages.size() == 1);  // no new resolution was added
    }

    // Test 12: Cross-pool import - chain interned in pool1 is re-interned into pool2
    // with completely fresh pool2 pointers; pool1 must be unaffected
    {
        lineage_pool pool1;
        const goal_lineage*        p1_g0 = pool1.goal(nullptr, 0);
        const resolution_lineage*  p1_r1 = pool1.resolution(p1_g0, 1);
        const goal_lineage*        p1_g2 = pool1.goal(p1_r1, 2);
        assert(pool1.goal_lineages.size() == 2);
        assert(pool1.resolution_lineages.size() == 1);

        lineage_pool pool2;
        const goal_lineage* p2_g2 = pool2.import(p1_g2);
        assert(p2_g2 != p1_g2);
        assert(p2_g2->idx == 2);
        assert(pool2.goal_lineages.size() == 2);
        assert(pool2.resolution_lineages.size() == 1);
        const resolution_lineage* p2_r1 = p2_g2->parent;
        assert(p2_r1 != p1_r1);
        assert(p2_r1->idx == 1);
        const goal_lineage* p2_g0 = p2_r1->parent;
        assert(p2_g0 != p1_g0);
        assert(p2_g0->parent == nullptr);
        assert(p2_g0->idx == 0);
        // pool1 must be completely unaffected
        assert(pool1.goal_lineages.size() == 2);
        assert(pool1.resolution_lineages.size() == 1);
    }
}

void test_sequencer_constructor() {
    trail t;
    
    // Basic construction with trail reference
    sequencer vars1(t);
    assert(vars1.index == 0);
    
    // Multiple sequencers can be constructed with same trail
    sequencer vars2(t);
    assert(vars2.index == 0);
    
    // Multiple sequencers with different trails
    trail t2;
    sequencer vars3(t2);
    assert(vars3.index == 0);
    
    // All sequencers should be independent
    t.push();
    uint32_t v1 = vars1();
    assert(v1 == 0);
    assert(vars1.index == 1);
    assert(vars2.index == 0);  // vars2 unchanged
    assert(vars3.index == 0);  // vars3 unchanged
    
    uint32_t v2 = vars2();
    assert(v2 == 0);
    assert(vars2.index == 1);
    assert(vars1.index == 1);  // vars1 unchanged
    
    // Pop should affect both vars1 and vars2 since they share trail t
    t.pop();
    assert(vars1.index == 0);
    assert(vars2.index == 0);
    
    // vars3 with different trail is unaffected
    assert(vars3.index == 0);
}

void test_sequencer() {
    // Test 1: Basic next() calls - verify sequential indices
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        assert(vars.index == 0);
        assert(t.undo_stack.size() == 0);
        
        uint32_t v0 = vars();
        assert(v0 == 0);
        assert(vars.index == 1);
        assert(t.undo_stack.size() == 1);  // One undo logged
        
        uint32_t v1 = vars();
        assert(v1 == 1);
        assert(vars.index == 2);
        assert(t.undo_stack.size() == 2);
        
        uint32_t v2 = vars();
        assert(v2 == 2);
        assert(vars.index == 3);
        assert(t.undo_stack.size() == 3);
        
        uint32_t v3 = vars();
        assert(v3 == 3);
        assert(vars.index == 4);
        assert(t.undo_stack.size() == 4);
        
        t.pop();
        assert(vars.index == 0);
        assert(t.undo_stack.size() == 0);
    }
    
    // Test 2: Many sequential calls
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        
        for (uint32_t i = 0; i < 100; ++i) {
            uint32_t v = vars();
            assert(v == i);
            assert(vars.index == i + 1);
        }
        assert(vars.index == 100);
        assert(t.undo_stack.size() == 100);
        
        t.pop();
        assert(vars.index == 0);
    }
    
    // Test 3: Backtracking - single frame
    {
        trail t;
        sequencer vars(t);
        
        size_t count_before = vars.index;
        assert(count_before == 0);
        
        t.push();
        uint32_t v1 = vars();
        uint32_t v2 = vars();
        uint32_t v3 = vars();
        assert(v1 == 0);
        assert(v2 == 1);
        assert(v3 == 2);
        assert(vars.index == 3);
        
        t.pop();
        assert(vars.index == count_before);
        assert(vars.index == 0);
    }
    
    // Test 4: Backtracking - nested frames
    {
        trail t;
        sequencer vars(t);
        
        t.push();  // Frame 1
        assert(vars.index == 0);
        assert(t.frame_boundary_stack.size() == 1);
        
        uint32_t v0 = vars();
        uint32_t v1 = vars();
        assert(v0 == 0);
        assert(v1 == 1);
        assert(vars.index == 2);
        size_t checkpoint1 = vars.index;
        
        t.push();  // Frame 2
        assert(t.frame_boundary_stack.size() == 2);
        uint32_t v2 = vars();
        uint32_t v3 = vars();
        uint32_t v4 = vars();
        assert(v2 == 2);
        assert(v3 == 3);
        assert(v4 == 4);
        assert(vars.index == 5);
        size_t checkpoint2 = vars.index;
        
        t.push();  // Frame 3
        assert(t.frame_boundary_stack.size() == 3);
        uint32_t v5 = vars();
        assert(v5 == 5);
        assert(vars.index == 6);
        
        t.pop();  // Pop frame 3
        assert(vars.index == checkpoint2);
        assert(vars.index == 5);
        
        t.pop();  // Pop frame 2
        assert(vars.index == checkpoint1);
        assert(vars.index == 2);
        
        t.pop();  // Pop frame 1
        assert(vars.index == 0);
    }
    
    // Test 5: Re-interning after backtrack - should reuse same indices
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        uint32_t first_v0 = vars();
        uint32_t first_v1 = vars();
        assert(first_v0 == 0);
        assert(first_v1 == 1);
        assert(vars.index == 2);
        
        t.pop();
        assert(vars.index == 0);
        
        // Call next() again - should get same indices
        t.push();
        uint32_t second_v0 = vars();
        uint32_t second_v1 = vars();
        assert(second_v0 == 0);
        assert(second_v1 == 1);
        assert(second_v0 == first_v0);
        assert(second_v1 == first_v1);
        assert(vars.index == 2);
        
        t.pop();
    }
    
    // Test 6: Complex nested scenario with checkpoints
    {
        trail t;
        sequencer vars(t);
        
        size_t checkpoint_start = vars.index;
        assert(checkpoint_start == 0);
        
        t.push();  // Frame A
        vars();  // 0
        vars();  // 1
        size_t checkpoint_a = vars.index;
        assert(checkpoint_a == 2);
        
        t.push();  // Frame B
        vars();  // 2
        vars();  // 3
        vars();  // 4
        size_t checkpoint_b = vars.index;
        assert(checkpoint_b == 5);
        
        t.push();  // Frame C
        vars();  // 5
        size_t checkpoint_c = vars.index;
        assert(checkpoint_c == 6);
        
        // Pop Frame C
        t.pop();
        assert(vars.index == checkpoint_b);
        assert(vars.index == 5);
        
        // Pop Frame B
        t.pop();
        assert(vars.index == checkpoint_a);
        assert(vars.index == 2);
        
        // Pop Frame A
        t.pop();
        assert(vars.index == checkpoint_start);
        assert(vars.index == 0);
    }
    
    // Test 7: Empty frames - push/pop with no next() calls
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        vars();
        vars();
        assert(vars.index == 2);
        
        t.push();  // Empty frame
        // No next() calls
        assert(vars.index == 2);
        
        t.pop();  // Pop empty frame
        assert(vars.index == 2);  // Should remain unchanged
        
        t.pop();
        assert(vars.index == 0);
    }
    
    // Test 8: Multiple sequencers with shared trail
    {
        trail t;
        sequencer vars1(t);
        sequencer vars2(t);
        
        t.push();
        
        uint32_t v1 = vars1();
        assert(v1 == 0);
        assert(vars1.index == 1);
        assert(vars2.index == 0);  // Independent
        
        uint32_t v2 = vars2();
        assert(v2 == 0);
        assert(vars2.index == 1);
        assert(vars1.index == 1);  // Unchanged
        
        uint32_t v3 = vars1();
        assert(v3 == 1);
        assert(vars1.index == 2);
        assert(vars2.index == 1);  // Still unchanged
        
        // Pop should restore both sequencers
        t.pop();
        assert(vars1.index == 0);
        assert(vars2.index == 0);
    }
    
    // Test 9: Edge case - many variables
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        
        for (uint32_t i = 0; i < 1000; ++i) {
            uint32_t v = vars();
            assert(v == i);
            assert(vars.index == i + 1);
        }
        assert(vars.index == 1000);
        assert(t.undo_stack.size() == 1000);
        
        t.pop();
        assert(vars.index == 0);
        assert(t.undo_stack.size() == 0);
    }
    
    // Test 10: Post-backtrack continuation
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        vars();  // 0
        vars();  // 1
        vars();  // 2
        assert(vars.index == 3);
        
        t.push();
        vars();  // 3
        vars();  // 4
        assert(vars.index == 5);
        
        t.pop();  // Back to 3
        assert(vars.index == 3);
        
        // Continue from 3
        uint32_t v = vars();
        assert(v == 3);
        assert(vars.index == 4);
        
        v = vars();
        assert(v == 4);
        assert(vars.index == 5);
        
        t.pop();
        assert(vars.index == 0);
    }
    
    // Test 11: Deeply nested frames with precise tracking
    {
        trail t;
        sequencer vars(t);
        
        t.push();  // Level 1
        assert(vars.index == 0);
        assert(t.undo_stack.size() == 0);
        
        uint32_t v0 = vars();
        assert(v0 == 0);
        assert(vars.index == 1);
        assert(t.undo_stack.size() == 1);
        
        t.push();  // Level 2
        assert(t.frame_boundary_stack.size() == 2);
        uint32_t v1 = vars();
        assert(v1 == 1);
        assert(vars.index == 2);
        assert(t.undo_stack.size() == 2);
        
        t.push();  // Level 3
        assert(t.frame_boundary_stack.size() == 3);
        uint32_t v2 = vars();
        assert(v2 == 2);
        assert(vars.index == 3);
        assert(t.undo_stack.size() == 3);
        
        t.push();  // Level 4
        assert(t.frame_boundary_stack.size() == 4);
        uint32_t v3 = vars();
        assert(v3 == 3);
        assert(vars.index == 4);
        assert(t.undo_stack.size() == 4);
        
        t.push();  // Level 5
        assert(t.frame_boundary_stack.size() == 5);
        uint32_t v4 = vars();
        assert(v4 == 4);
        assert(vars.index == 5);
        assert(t.undo_stack.size() == 5);
        
        // Pop all levels
        t.pop();
        assert(vars.index == 4);
        assert(t.undo_stack.size() == 4);
        
        t.pop();
        assert(vars.index == 3);
        assert(t.undo_stack.size() == 3);
        
        t.pop();
        assert(vars.index == 2);
        assert(t.undo_stack.size() == 2);
        
        t.pop();
        assert(vars.index == 1);
        assert(t.undo_stack.size() == 1);
        
        t.pop();
        assert(vars.index == 0);
        assert(t.undo_stack.size() == 0);
    }
    
    // Test 12: Interleaved push/pop/next operations
    {
        trail t;
        sequencer vars(t);
        
        t.push();  // Frame 1
        uint32_t v0 = vars();
        assert(v0 == 0);
        assert(vars.index == 1);
        
        t.push();  // Frame 2
        uint32_t v1 = vars();
        assert(v1 == 1);
        assert(vars.index == 2);
        
        t.pop();  // Pop frame 2
        assert(vars.index == 1);
        
        t.push();  // New frame 2
        uint32_t v1_again = vars();
        assert(v1_again == 1);  // Same index as before
        assert(vars.index == 2);
        
        t.push();  // Frame 3
        uint32_t v2 = vars();
        assert(v2 == 2);
        assert(vars.index == 3);
        
        t.pop();  // Pop frame 3
        assert(vars.index == 2);
        
        t.pop();  // Pop frame 2
        assert(vars.index == 1);
        
        t.pop();  // Pop frame 1
        assert(vars.index == 0);
    }
    
    // Test 13: Complex nested scenario - Frame A, B, C with partial pops
    {
        trail t;
        sequencer vars(t);
        
        size_t checkpoint_start = vars.index;
        assert(checkpoint_start == 0);
        
        t.push();  // Frame A
        assert(vars.index == 0);
        assert(t.undo_stack.size() == 0);
        
        vars();  // 0
        assert(vars.index == 1);
        assert(t.undo_stack.size() == 1);
        
        vars();  // 1
        assert(vars.index == 2);
        assert(t.undo_stack.size() == 2);
        
        size_t checkpoint_a = vars.index;
        assert(checkpoint_a == 2);
        
        t.push();  // Frame B
        assert(t.frame_boundary_stack.size() == 2);
        assert(vars.index == 2);
        
        vars();  // 2
        assert(vars.index == 3);
        
        size_t checkpoint_b = vars.index;
        assert(checkpoint_b == 3);
        
        // Add more to Frame B
        vars();  // 3
        vars();  // 4
        assert(vars.index == 5);
        size_t checkpoint_b_final = vars.index;
        
        t.push();  // Frame C
        assert(t.frame_boundary_stack.size() == 3);
        assert(vars.index == 5);
        
        vars();  // 5
        assert(vars.index == 6);
        size_t checkpoint_c = vars.index;
        
        // Pop Frame C
        t.pop();
        assert(vars.index == checkpoint_b_final);
        assert(vars.index == 5);
        assert(t.frame_boundary_stack.size() == 2);
        
        // Pop Frame B
        t.pop();
        assert(vars.index == checkpoint_a);
        assert(vars.index == 2);
        assert(t.frame_boundary_stack.size() == 1);
        
        // Pop Frame A
        t.pop();
        assert(vars.index == checkpoint_start);
        assert(vars.index == 0);
        assert(t.frame_boundary_stack.size() == 0);
    }
    
    // Test 14: Multiple operations per frame with precise undo_stack tracking
    {
        trail t;
        sequencer vars(t);
        
        t.push();  // Level 1
        assert(t.frame_boundary_stack.top() == 0);
        
        vars();
        assert(t.undo_stack.size() == 1);
        vars();
        assert(t.undo_stack.size() == 2);
        assert(vars.index == 2);
        
        t.push();  // Level 2
        assert(t.frame_boundary_stack.top() == 2);
        
        vars();
        assert(t.undo_stack.size() == 3);
        vars();
        assert(t.undo_stack.size() == 4);
        vars();
        assert(t.undo_stack.size() == 5);
        assert(vars.index == 5);
        
        t.push();  // Level 3
        assert(t.frame_boundary_stack.top() == 5);
        
        vars();
        assert(t.undo_stack.size() == 6);
        assert(vars.index == 6);
        
        t.pop();  // Pop level 3
        assert(t.undo_stack.size() == 5);
        assert(vars.index == 5);
        
        t.pop();  // Pop level 2
        assert(t.undo_stack.size() == 2);
        assert(vars.index == 2);
        
        t.pop();  // Pop level 1
        assert(t.undo_stack.size() == 0);
        assert(vars.index == 0);
    }
    
    // Test 15: Verify no duplicate indices within a sequence
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        
        std::set<uint32_t> seen_indices;
        for (int i = 0; i < 50; ++i) {
            uint32_t v = vars();
            assert(seen_indices.count(v) == 0);  // Should be unique
            seen_indices.insert(v);
            assert(v == static_cast<uint32_t>(i));
        }
        assert(seen_indices.size() == 50);
        assert(vars.index == 50);
        
        t.pop();
    }
    
    // Test 16: Mixed nesting with multiple next() calls
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        assert(vars.index == 0);
        
        vars();
        vars();
        assert(vars.index == 2);
        
        t.push();
        vars();
        assert(vars.index == 3);
        
        t.pop();
        assert(vars.index == 2);
        
        t.push();
        vars();
        vars();
        assert(vars.index == 4);
        
        t.pop();
        assert(vars.index == 2);
        
        t.pop();
        assert(vars.index == 0);
    }
    
    // Test 17: Verify exact undo_stack behavior
    {
        trail t;
        sequencer vars(t);
        
        t.push();
        size_t undo_before = t.undo_stack.size();
        assert(undo_before == 0);
        
        for (int i = 0; i < 10; ++i) {
            vars();
            assert(t.undo_stack.size() == undo_before + i + 1);
        }
        
        t.pop();
        assert(t.undo_stack.size() == 0);
    }
}

void test_copier_constructor() {
    trail t;
    sequencer vars(t);
    expr_pool pool(t);
    
    // Basic construction
    copier copy1(vars, pool);
    assert(&copy1.sequencer_ref == &vars);
    assert(&copy1.expr_pool_ref == &pool);
    
    // Multiple copiers can share same dependencies
    copier copy2(vars, pool);
    assert(&copy2.sequencer_ref == &vars);
    assert(&copy2.expr_pool_ref == &pool);
    
    // Different dependencies
    trail t2;
    sequencer vars2(t2);
    expr_pool pool2(t2);
    copier copy3(vars2, pool2);
    assert(&copy3.sequencer_ref == &vars2);
    assert(&copy3.expr_pool_ref == &pool2);
}

void test_copier() {
    // Test 1: Copy atom - should return same pointer (atoms are immutable)
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* original = pool.functor("test", {});
        assert(pool.size() == 1);
        assert(pool.exprs.size() == 1);
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied == original);  // Same atom
        assert(var_map.empty());  // No variables mapped
        assert(vars.index == 0);  // No fresh vars created
        assert(pool.size() == 1);  // No new entries
        assert(pool.exprs.size() == 1);
        
        t.pop();
    }
    
    // Test 2: Copy single variable - creates fresh variable
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* original = pool.var(5);
        assert(pool.size() == 1);
        assert(pool.exprs.size() == 1);
        assert(pool.exprs.count(*original) == 1);
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied != original);  // Different variable
        assert(std::holds_alternative<expr::var>(copied->content));
        assert(std::get<expr::var>(copied->content).index == 0);  // Fresh var starts at 0
        assert(var_map.size() == 1);
        assert(var_map.at(5) == 0);  // Original index 5 mapped to 0
        assert(vars.index == 1);
        assert(pool.size() == 2);  // Original var(5) and new var(0)
        assert(pool.exprs.size() == 2);
        assert(pool.exprs.count(*original) == 1);
        assert(pool.exprs.count(*copied) == 1);
        
        t.pop();
    }
    
    // Test 3: Copy same variable twice - should use same mapping
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* original = pool.var(10);
        assert(pool.size() == 1);
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied1 = copy(original, var_map);
        assert(std::get<expr::var>(copied1->content).index == 0);
        assert(var_map.size() == 1);
        assert(var_map.at(10) == 0);
        assert(vars.index == 1);
        assert(pool.size() == 2);
        assert(pool.exprs.size() == 2);
        
        const expr* copied2 = copy(original, var_map);
        assert(copied2 == copied1);  // Should be same pointer (interned)
        assert(var_map.size() == 1);  // No new mapping
        assert(vars.index == 1);  // No new variable
        assert(pool.size() == 2);  // Still 2
        assert(pool.exprs.size() == 2);
        
        t.pop();
    }
    
    // Test 4: Copy cons with atoms
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* a = pool.functor("a", {});
        const expr* b = pool.functor("b", {});
        const expr* original = pool.functor("cons", {a, b});
        assert(pool.size() == 3);
        assert(pool.exprs.size() == 3);
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied == original);  // Same cons since atoms unchanged
        assert(var_map.empty());  // No variables
        assert(vars.index == 0);
        assert(pool.size() == 3);  // No new entries
        assert(pool.exprs.size() == 3);
        
        t.pop();
    }
    
    // Test 5: Copy cons with variables
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v1 = pool.var(10);
        const expr* v2 = pool.var(20);
        const expr* original = pool.functor("cons", {v1, v2});
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied != original);  // Different cons (different vars)
        assert(std::holds_alternative<expr::functor>(copied->content));
        
        const expr::functor& copied_cons = std::get<expr::functor>(copied->content);
        assert(std::get<expr::var>(copied_cons.args[0]->content).index == 0);
        assert(std::get<expr::var>(copied_cons.args[1]->content).index == 1);
        
        assert(var_map.size() == 2);
        assert(var_map.at(10) == 0);
        assert(var_map.at(20) == 1);
        assert(vars.index == 2);
        
        // Pool should contain: var(10), var(20), original cons, var(0), var(1), new cons
        assert(pool.size() == 6);
        assert(pool.exprs.size() == 6);
        assert(pool.exprs.count(*v1) == 1);
        assert(pool.exprs.count(*v2) == 1);
        assert(pool.exprs.count(*original) == 1);
        assert(pool.exprs.count(*copied_cons.args[0]) == 1);
        assert(pool.exprs.count(*copied_cons.args[1]) == 1);
        assert(pool.exprs.count(*copied) == 1);
        
        t.pop();
    }
    
    // Test 6: Copy cons with same variable in both positions
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v = pool.var(5);
        const expr* original = pool.functor("cons", {v, v});
        assert(pool.size() == 2);  // var(5) and cons
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        const expr::functor& copied_cons = std::get<expr::functor>(copied->content);
        assert(std::get<expr::var>(copied_cons.args[0]->content).index == 0);
        assert(std::get<expr::var>(copied_cons.args[1]->content).index == 0);
        assert(copied_cons.args[0] == copied_cons.args[1]);  // Same variable
        
        assert(var_map.size() == 1);
        assert(var_map.at(5) == 0);
        assert(vars.index == 1);  // Only one fresh variable created
        
        // Pool: var(5), original cons, var(0), new cons
        assert(pool.size() == 4);
        assert(pool.exprs.size() == 4);
        assert(pool.exprs.count(*v) == 1);
        assert(pool.exprs.count(*original) == 1);
        assert(pool.exprs.count(*copied_cons.args[0]) == 1);
        assert(pool.exprs.count(*copied) == 1);
        
        t.pop();
    }
    
    // Test 7: Copy nested cons with mixed content
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* a = pool.functor("atom", {});
        const expr* v = pool.var(7);
        const expr* inner = pool.functor("cons", {a, v});
        const expr* original = pool.functor("cons", {inner, a});
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied != original);
        const expr::functor& outer_cons = std::get<expr::functor>(copied->content);
        assert(outer_cons.args[1] == a);  // Atom unchanged
        
        const expr::functor& inner_cons = std::get<expr::functor>(outer_cons.args[0]->content);
        assert(inner_cons.args[0] == a);  // Atom unchanged
        assert(std::get<expr::var>(inner_cons.args[1]->content).index == 0);
        
        assert(var_map.size() == 1);
        assert(var_map.at(7) == 0);
        assert(vars.index == 1);
        
        t.pop();
    }
    
    // Test 8: Copy deeply nested cons
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v1 = pool.var(10);
        const expr* v2 = pool.var(20);
        const expr* v3 = pool.var(30);
        const expr* c1 = pool.functor("cons", {v1, v2});
        const expr* c2 = pool.functor("cons", {c1, v3});
        const expr* original = pool.functor("cons", {c2, v1});  // v1 appears twice
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 3);
        assert(var_map.at(10) == 0);
        assert(var_map.at(20) == 1);
        assert(var_map.at(30) == 2);
        assert(vars.index == 3);
        
        // Verify structure
        const expr::functor& top = std::get<expr::functor>(copied->content);
        assert(std::get<expr::var>(top.args[1]->content).index == 0);  // v1 mapped to 0
        
        const expr::functor& middle = std::get<expr::functor>(top.args[0]->content);
        assert(std::get<expr::var>(middle.args[1]->content).index == 2);  // v3 mapped to 2
        
        const expr::functor& bottom = std::get<expr::functor>(middle.args[0]->content);
        assert(std::get<expr::var>(bottom.args[0]->content).index == 0);  // v1 mapped to 0
        assert(std::get<expr::var>(bottom.args[1]->content).index == 1);  // v2 mapped to 1
        
        t.pop();
    }
    
    // Test 9: Copy with pre-populated variable map
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v5 = pool.var(5);
        const expr* v10 = pool.var(10);
        const expr* original = pool.functor("cons", {v5, v10});
        
        // Pre-populate map
        std::map<uint32_t, uint32_t> var_map;
        var_map[5] = 100;  // Map 5 to 100
        
        const expr* copied = copy(original, var_map);
        
        // v5 should use existing mapping, v10 should get fresh
        assert(var_map.size() == 2);
        assert(var_map.at(5) == 100);  // Unchanged
        assert(var_map.at(10) == 0);  // Fresh variable
        
        const expr::functor& copied_cons = std::get<expr::functor>(copied->content);
        assert(std::get<expr::var>(copied_cons.args[0]->content).index == 100);
        assert(std::get<expr::var>(copied_cons.args[1]->content).index == 0);
        
        assert(vars.index == 1);  // Only one fresh var created
        
        t.pop();
    }
    
    // Test 10: Copy multiple expressions with shared variable map
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* expr1 = pool.functor("cons", {v1, v2});
        const expr* expr2 = pool.functor("cons", {v2, v1});  // Swapped
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied1 = copy(expr1, var_map);
        assert(var_map.size() == 2);
        assert(var_map.at(1) == 0);
        assert(var_map.at(2) == 1);
        assert(vars.index == 2);
        
        const expr* copied2 = copy(expr2, var_map);
        assert(var_map.size() == 2);  // No new mappings
        assert(vars.index == 2);  // No new variables
        
        // Verify both use same variable mapping
        const expr::functor& cons1 = std::get<expr::functor>(copied1->content);
        const expr::functor& cons2 = std::get<expr::functor>(copied2->content);
        assert(std::get<expr::var>(cons1.args[0]->content).index == 0);
        assert(std::get<expr::var>(cons1.args[1]->content).index == 1);
        assert(std::get<expr::var>(cons2.args[0]->content).index == 1);
        assert(std::get<expr::var>(cons2.args[1]->content).index == 0);
        
        t.pop();
    }
    
    // Test 11: Copy empty variable map vs populated
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v = pool.var(42);
        
        std::map<uint32_t, uint32_t> map1;
        const expr* copy1 = copy(v, map1);
        assert(std::get<expr::var>(copy1->content).index == 0);
        assert(map1.at(42) == 0);
        
        std::map<uint32_t, uint32_t> map2;
        const expr* copy2 = copy(v, map2);
        assert(std::get<expr::var>(copy2->content).index == 1);
        assert(map2.at(42) == 1);
        
        // Different maps, different fresh variables
        assert(copy1 != copy2);
        assert(vars.index == 2);
        
        t.pop();
    }
    
    // Test 12: Copy complex nested structure
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        // Build: cons(cons(var(1), atom("a")), cons(var(2), var(1)))
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* a = pool.functor("a", {});
        const expr* left = pool.functor("cons", {v1, a});
        const expr* right = pool.functor("cons", {v2, v1});
        const expr* original = pool.functor("cons", {left, right});
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 2);
        assert(var_map.at(1) == 0);
        assert(var_map.at(2) == 1);
        assert(vars.index == 2);
        
        // Verify structure: cons(cons(var(0), atom("a")), cons(var(1), var(0)))
        const expr::functor& top = std::get<expr::functor>(copied->content);
        
        const expr::functor& left_cons = std::get<expr::functor>(top.args[0]->content);
        assert(std::get<expr::var>(left_cons.args[0]->content).index == 0);
        assert(left_cons.args[1] == a);
        
        const expr::functor& right_cons = std::get<expr::functor>(top.args[1]->content);
        assert(std::get<expr::var>(right_cons.args[0]->content).index == 1);
        assert(std::get<expr::var>(right_cons.args[1]->content).index == 0);
        
        t.pop();
    }
    
    // Test 13: Copy with backtracking
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v = pool.var(5);
        const expr* original = pool.functor("cons", {v, pool.functor("x", {})});
        
        t.push();
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        assert(vars.index == 1);
        assert(pool.size() == 5);  // v(5), atom("x"), original cons, v(0), new cons
        
        t.pop();
        assert(vars.index == 0);
        assert(pool.size() == 3);  // Back to v(5), atom("x"), original cons
        
        t.pop();
    }
    
    // Test 14: Multiple variables in sequence
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        std::map<uint32_t, uint32_t> var_map;
        
        // Copy variables in order
        const expr* v10 = pool.var(10);
        const expr* copy10 = copy(v10, var_map);
        assert(std::get<expr::var>(copy10->content).index == 0);
        assert(vars.index == 1);
        
        const expr* v20 = pool.var(20);
        const expr* copy20 = copy(v20, var_map);
        assert(std::get<expr::var>(copy20->content).index == 1);
        assert(vars.index == 2);
        
        const expr* v30 = pool.var(30);
        const expr* copy30 = copy(v30, var_map);
        assert(std::get<expr::var>(copy30->content).index == 2);
        assert(vars.index == 3);
        
        assert(var_map.size() == 3);
        
        t.pop();
    }
    
    // Test 15: Copy expression with all three types
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        // Build: cons(atom("a"), cons(var(5), atom("b")))
        const expr* a = pool.functor("a", {});
        const expr* b = pool.functor("b", {});
        const expr* v5 = pool.var(5);
        const expr* inner = pool.functor("cons", {v5, b});
        const expr* original = pool.functor("cons", {a, inner});
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 1);
        assert(var_map.at(5) == 0);
        assert(vars.index == 1);
        
        // Verify structure
        const expr::functor& top = std::get<expr::functor>(copied->content);
        assert(top.args[0] == a);  // Atom unchanged
        
        const expr::functor& inner_cons = std::get<expr::functor>(top.args[1]->content);
        assert(std::get<expr::var>(inner_cons.args[0]->content).index == 0);
        assert(inner_cons.args[1] == b);  // Atom unchanged
        
        t.pop();
    }
    
    // Test 16: Copy with variable appearing multiple times in deep structure
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        // Build: cons(cons(v1, v2), cons(v2, v1))
        const expr* left = pool.functor("cons", {v1, v2});
        const expr* right = pool.functor("cons", {v2, v1});
        const expr* original = pool.functor("cons", {left, right});
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 2);
        assert(var_map.at(1) == 0);
        assert(var_map.at(2) == 1);
        assert(vars.index == 2);
        
        // Verify all occurrences use consistent mapping
        const expr::functor& top = std::get<expr::functor>(copied->content);
        const expr::functor& left_cons = std::get<expr::functor>(top.args[0]->content);
        const expr::functor& right_cons = std::get<expr::functor>(top.args[1]->content);
        
        assert(std::get<expr::var>(left_cons.args[0]->content).index == 0);  // v1
        assert(std::get<expr::var>(left_cons.args[1]->content).index == 1);  // v2
        assert(std::get<expr::var>(right_cons.args[0]->content).index == 1);  // v2
        assert(std::get<expr::var>(right_cons.args[1]->content).index == 0);  // v1
        
        t.pop();
    }
    
    // Test 17: Copy atom-only structure
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* a1 = pool.functor("a1", {});
        const expr* a2 = pool.functor("a2", {});
        const expr* a3 = pool.functor("a3", {});
        const expr* c1 = pool.functor("cons", {a1, a2});
        const expr* original = pool.functor("cons", {c1, a3});
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(copied == original);  // No variables, so same structure
        assert(var_map.empty());
        assert(vars.index == 0);
        
        t.pop();
    }
    
    // Test 18: Copy with interleaved fresh() calls
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v10 = pool.var(10);
        std::map<uint32_t, uint32_t> var_map;
        
        copy(v10, var_map);
        assert(vars.index == 1);
        assert(var_map.at(10) == 0);
        
        // Manually create a fresh variable using vars()
        const expr* fresh_var = pool.var(vars());
        assert(std::get<expr::var>(fresh_var->content).index == 1);
        assert(vars.index == 2);
        
        const expr* v20 = pool.var(20);
        copy(v20, var_map);
        assert(vars.index == 3);
        assert(var_map.at(20) == 2);
        
        t.pop();
    }
    
    // Test 19: Verify pool interning during copy
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* a = pool.functor("a", {});
        const expr* original = pool.functor("cons", {v1, a});
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied1 = copy(original, var_map);
        size_t pool_size_after_first = pool.size();
        
        // Copy again with cleared map - creates fresh var(1) which already exists!
        var_map.clear();
        const expr* copied2 = copy(original, var_map);
        
        // Fresh variable is var(1), which already exists, so gets interned
        // cons(var(1), atom("a")) also already exists, so also interned
        assert(vars.index == 2);
        assert(pool.exprs.size() == pool_size_after_first);  // No new entries due to interning
        assert(copied2 == original);  // Should be same as original due to interning!
        
        t.pop();
    }
    
    // Test 20: Copy with zero-indexed variables
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* v0 = pool.var(0);
        const expr* v1 = pool.var(1);
        const expr* original = pool.functor("cons", {v0, v1});
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.at(0) == 0);  // 0 maps to fresh 0
        assert(var_map.at(1) == 1);  // 1 maps to fresh 1
        assert(vars.index == 2);
        
        t.pop();
    }
}

void test_normalizer_constructor() {
    trail t;
    expr_pool pool(t);
    bind_map bm(t);
    
    // Basic construction
    normalizer norm1(pool, bm);
    assert(&norm1.expr_pool_ref == &pool);
    assert(&norm1.bind_map_ref == &bm);
    
    // Multiple normalizers can share same dependencies
    normalizer norm2(pool, bm);
    assert(&norm2.expr_pool_ref == &pool);
    assert(&norm2.bind_map_ref == &bm);
    
    // Different dependencies
    trail t2;
    expr_pool pool2(t2);
    bind_map bm2(t2);
    normalizer norm3(pool2, bm2);
    assert(&norm3.expr_pool_ref == &pool2);
    assert(&norm3.bind_map_ref == &bm2);
}

void test_normalizer() {
    // Test 1: Normalize atom - returns unchanged
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* a = pool.functor("test", {});
        const expr* result = norm(a);
        
        assert(result == a);
        assert(std::holds_alternative<expr::functor>(result->content));
        
        t.pop();
    }
    
    // Test 2: Normalize unbound variable - returns unchanged
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v = pool.var(5);
        const expr* result = norm(v);
        
        assert(result == v);
        assert(std::holds_alternative<expr::var>(result->content));
        assert(std::get<expr::var>(result->content).index == 5);
        
        t.pop();
    }
    
    // Test 3: Normalize variable bound to atom
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v = pool.var(1);
        const expr* a = pool.functor("hello", {});
        bm.bind(1, a);
        
        const expr* result = norm(v);
        
        assert(result == a);
        assert(std::holds_alternative<expr::functor>(result->content));
        assert(std::get<expr::functor>(result->content).name == "hello");
        
        t.pop();
    }
    
    // Test 4: Normalize variable with chain of bindings
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* v3 = pool.var(3);
        const expr* a = pool.functor("end", {});
        
        bm.bind(1, v2);
        bm.bind(2, v3);
        bm.bind(3, a);
        
        const expr* result = norm(v1);
        
        assert(result == a);
        assert(std::get<expr::functor>(result->content).name == "end");
        
        t.pop();
    }
    
    // Test 5: Normalize cons with atoms
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* a1 = pool.functor("left", {});
        const expr* a2 = pool.functor("right", {});
        const expr* c = pool.functor("cons", {a1, a2});
        
        const expr* result = norm(c);
        
        assert(result == c);
        const expr::functor& result_cons = std::get<expr::functor>(result->content);
        assert(result_cons.args[0] == a1);
        assert(result_cons.args[1] == a2);
        
        t.pop();
    }
    
    // Test 6: Normalize cons with unbound variables
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* c = pool.functor("cons", {v1, v2});
        
        const expr* result = norm(c);
        
        assert(result == c);
        const expr::functor& result_cons = std::get<expr::functor>(result->content);
        assert(result_cons.args[0] == v1);
        assert(result_cons.args[1] == v2);
        
        t.pop();
    }
    
    // Test 7: Normalize cons with bound variables
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* a1 = pool.functor("a", {});
        const expr* a2 = pool.functor("b", {});
        const expr* c = pool.functor("cons", {v1, v2});
        
        bm.bind(1, a1);
        bm.bind(2, a2);
        
        const expr* result = norm(c);
        
        assert(result != c);
        const expr::functor& result_cons = std::get<expr::functor>(result->content);
        assert(result_cons.args[0] == a1);
        assert(result_cons.args[1] == a2);
        
        t.pop();
    }
    
    // Test 8: Normalize cons with one bound, one unbound variable
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* a = pool.functor("bound", {});
        const expr* c = pool.functor("cons", {v1, v2});
        
        bm.bind(1, a);
        
        const expr* result = norm(c);
        
        const expr::functor& result_cons = std::get<expr::functor>(result->content);
        assert(result_cons.args[0] == a);
        assert(result_cons.args[1] == v2);
        
        t.pop();
    }
    
    // Test 9: Normalize nested cons with all atoms
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* a1 = pool.functor("a", {});
        const expr* a2 = pool.functor("b", {});
        const expr* a3 = pool.functor("c", {});
        const expr* inner = pool.functor("cons", {a1, a2});
        const expr* outer = pool.functor("cons", {inner, a3});
        
        const expr* result = norm(outer);
        
        assert(result == outer);
        
        t.pop();
    }
    
    // Test 10: Normalize nested cons with bound variables
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* a1 = pool.functor("x", {});
        const expr* a2 = pool.functor("y", {});
        const expr* inner = pool.functor("cons", {v1, v2});
        const expr* outer = pool.functor("cons", {inner, a1});
        
        bm.bind(1, a1);
        bm.bind(2, a2);
        
        const expr* result = norm(outer);
        
        const expr::functor& outer_cons = std::get<expr::functor>(result->content);
        assert(outer_cons.args[1] == a1);
        
        const expr::functor& inner_cons = std::get<expr::functor>(outer_cons.args[0]->content);
        assert(inner_cons.args[0] == a1);
        assert(inner_cons.args[1] == a2);
        
        t.pop();
    }
    
    // Test 11: Normalize variable bound to cons
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v = pool.var(1);
        const expr* a1 = pool.functor("left", {});
        const expr* a2 = pool.functor("right", {});
        const expr* c = pool.functor("cons", {a1, a2});
        
        bm.bind(1, c);
        
        const expr* result = norm(v);
        
        assert(result == c);
        
        t.pop();
    }
    
    // Test 12: Normalize variable bound to cons with variables
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* v3 = pool.var(3);
        const expr* a1 = pool.functor("a", {});
        const expr* a2 = pool.functor("b", {});
        const expr* c = pool.functor("cons", {v2, v3});
        
        bm.bind(1, c);
        bm.bind(2, a1);
        bm.bind(3, a2);
        
        const expr* result = norm(v1);
        
        const expr::functor& result_cons = std::get<expr::functor>(result->content);
        assert(result_cons.args[0] == a1);
        assert(result_cons.args[1] == a2);
        
        t.pop();
    }
    
    // Test 13: Normalize deeply nested structure
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* v3 = pool.var(3);
        const expr* a = pool.functor("atom", {});
        
        const expr* c1 = pool.functor("cons", {v1, v2});
        const expr* c2 = pool.functor("cons", {c1, v3});
        
        bm.bind(1, a);
        bm.bind(2, a);
        bm.bind(3, a);
        
        const expr* result = norm(c2);
        
        const expr::functor& top = std::get<expr::functor>(result->content);
        assert(top.args[1] == a);
        
        const expr::functor& bottom = std::get<expr::functor>(top.args[0]->content);
        assert(bottom.args[0] == a);
        assert(bottom.args[1] == a);
        
        t.pop();
    }
    
    // Test 14: Normalize cons where same variable appears multiple times
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v = pool.var(1);
        const expr* a = pool.functor("shared", {});
        const expr* c = pool.functor("cons", {v, v});
        
        bm.bind(1, a);
        
        const expr* result = norm(c);
        
        const expr::functor& result_cons = std::get<expr::functor>(result->content);
        assert(result_cons.args[0] == a);
        assert(result_cons.args[1] == a);
        assert(result_cons.args[0] == result_cons.args[1]);
        
        t.pop();
    }
    
    // Test 15: Normalize with backtracking
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v = pool.var(1);
        const expr* a1 = pool.functor("first", {});
        const expr* a2 = pool.functor("second", {});
        
        t.push();
        bm.bind(1, a1);
        const expr* result1 = norm(v);
        assert(result1 == a1);
        t.pop();
        
        // After pop, v should be unbound again
        const expr* result2 = norm(v);
        assert(result2 == v);
        
        t.push();
        bm.bind(1, a2);
        const expr* result3 = norm(v);
        assert(result3 == a2);
        t.pop();
        
        t.pop();
    }
    
    // Test 16: Normalize complex structure with mixed bindings
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* v3 = pool.var(3);
        const expr* v4 = pool.var(4);
        const expr* a = pool.functor("atom", {});
        
        const expr* c1 = pool.functor("cons", {v1, a});
        const expr* c2 = pool.functor("cons", {v2, v3});
        const expr* c3 = pool.functor("cons", {c1, c2});
        
        bm.bind(1, a);
        bm.bind(2, v4);
        bm.bind(4, a);
        // v3 remains unbound
        
        const expr* result = norm(c3);
        
        const expr::functor& top = std::get<expr::functor>(result->content);
        
        const expr::functor& left = std::get<expr::functor>(top.args[0]->content);
        assert(left.args[0] == a);
        assert(left.args[1] == a);
        
        const expr::functor& right = std::get<expr::functor>(top.args[1]->content);
        assert(right.args[0] == a);
        assert(right.args[1] == v3);
        
        t.pop();
    }
    
    // Test 17: Normalize after unification
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* a = pool.functor("unified", {});
        const expr* c1 = pool.functor("cons", {v1, a});
        const expr* c2 = pool.functor("cons", {a, v2});
        
        bool unified = bm.unify(c1, c2);
        assert(unified);
        
        const expr* result1 = norm(c1);
        const expr* result2 = norm(c2);
        
        const expr::functor& r1 = std::get<expr::functor>(result1->content);
        const expr::functor& r2 = std::get<expr::functor>(result2->content);
        
        assert(r1.args[0] == a);
        assert(r1.args[1] == a);
        assert(r2.args[0] == a);
        assert(r2.args[1] == a);
        
        t.pop();
    }
    
    // Test 18: Normalize variable bound through multiple indirections
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* v3 = pool.var(3);
        const expr* v4 = pool.var(4);
        const expr* v5 = pool.var(5);
        const expr* a = pool.functor("final", {});
        
        bm.bind(1, v2);
        bm.bind(2, v3);
        bm.bind(3, v4);
        bm.bind(4, v5);
        bm.bind(5, a);
        
        const expr* result = norm(v1);
        
        assert(result == a);
        
        t.pop();
    }
    
    // Test 19: Normalize cons of cons with various binding patterns
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* v1 = pool.var(1);
        const expr* v2 = pool.var(2);
        const expr* v3 = pool.var(3);
        const expr* a1 = pool.functor("a1", {});
        const expr* a2 = pool.functor("a2", {});
        
        const expr* inner1 = pool.functor("cons", {v1, a1});
        const expr* inner2 = pool.functor("cons", {v2, v3});
        const expr* outer = pool.functor("cons", {inner1, inner2});
        
        bm.bind(1, a2);
        bm.bind(3, a1);
        // v2 remains unbound
        
        const expr* result = norm(outer);
        
        const expr::functor& top = std::get<expr::functor>(result->content);
        
        const expr::functor& left = std::get<expr::functor>(top.args[0]->content);
        assert(left.args[0] == a2);
        assert(left.args[1] == a1);
        
        const expr::functor& right = std::get<expr::functor>(top.args[1]->content);
        assert(right.args[0] == v2);
        assert(right.args[1] == a1);
        
        t.pop();
    }
}

// Concrete frontier for use in frontier tests.
// Each active goal maps to an int; expand returns (parent_val + 1) for each
// body literal so that child values are deterministically derived from the
// parent and the rule arity.
struct int_frontier : frontier<int> {
    int_frontier(const database& db, lineage_pool& lp)
        : frontier<int>(db, lp) {}
    std::vector<int> expand(const int& val, const rule& r) override {
        std::vector<int> result;
        for (size_t i = 0; i < r.body.size(); i++)
            result.push_back(val + 1);
        return result;
    }
};

void test_expr_printer_constructor() {
    const std::map<uint32_t, std::string> no_names;
    // Test 1: Construct with std::cout - reference is stored correctly
    {
        expr_printer ep(std::cout, no_names);
        assert(&ep.os == &std::cout);
    }

    // Test 2: Construct with std::cerr - different stream, different reference
    {
        expr_printer ep(std::cerr, no_names);
        assert(&ep.os == &std::cerr);
    }

    // Test 3: Construct with a stringstream - reference is to the local stream
    {
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        assert(&ep.os == &oss);
    }

    // Test 4: Two printers bound to the same stream share the same reference
    {
        std::ostringstream oss;
        expr_printer ep1(oss, no_names);
        expr_printer ep2(oss, no_names);
        assert(&ep1.os == &ep2.os);
    }

    // Test 5: var_names map is stored by reference in the member
    {
        std::ostringstream oss;
        std::map<uint32_t, std::string> var_names = {{0, "X"}, {1, "Y"}};
        expr_printer ep(oss, var_names);
        assert(&ep.var_names == &var_names);
    }

    // Test 6: default argument gives an empty var_names map
    {
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        assert(ep.var_names.empty());
    }
}

void test_expr_printer() {
    const std::map<uint32_t, std::string> no_names;
    // Test 1: Print an atom
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.functor("hello", {});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "hello");
        t.pop();
    }

    // Test 2: Print an empty-string atom
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.functor("", {});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "");
        t.pop();
    }

    // Test 3: Print a var
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.var(0);
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "?0");
        t.pop();
    }

    // Test 4: Print a var with a larger index
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.var(42);
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "?42");
        t.pop();
    }

    // Test 5: Print a flat cons cell
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.functor("cons", {pool.functor("a", {}), pool.functor("b", {})});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "[a|b]");
        t.pop();
    }

    // Test 6: Print a nested cons — left-heavy
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* inner = pool.functor("cons", {pool.functor("f", {}), pool.functor("x", {})});
        const expr* outer = pool.functor("cons", {inner, pool.functor("y", {})});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(outer);
        assert(oss.str() == "[[f|x]|y]");
        t.pop();
    }

    // Test 7: Print a nested cons — right-heavy
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* inner = pool.functor("cons", {pool.functor("x", {}), pool.functor("y", {})});
        const expr* outer = pool.functor("cons", {pool.functor("f", {}), inner});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(outer);
        assert(oss.str() == "[f, x|y]");
        t.pop();
    }

    // Test 8: Print a cons containing a var
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.functor("cons", {pool.functor("f", {}), pool.var(3)});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "[f|?3]");
        t.pop();
    }

    // Test 9: Print the same expression twice into the same stream — output concatenates
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.functor("x", {});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        ep(e);
        assert(oss.str() == "xx");
        t.pop();
    }

    // Test 10: Two separate printers on separate streams produce independent output
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* a = pool.functor("a", {});
        const expr* b = pool.functor("b", {});
        std::ostringstream oss1, oss2;
        expr_printer ep1(oss1, no_names);
        expr_printer ep2(oss2, no_names);
        ep1(a);
        ep2(b);
        assert(oss1.str() == "a");
        assert(oss2.str() == "b");
        t.pop();
    }

    // Test 11: Deep nesting — Peano numeral suc(suc(suc(zero)))
    // Encoded as cons(atom("suc"), cons(atom("suc"), cons(atom("suc"), atom("zero"))))
    // Expected: (suc . (suc . (suc . zero)))
    {
        trail t;
        expr_pool pool(t);
        t.push();
        const expr* e = pool.functor("cons", {pool.functor("suc", {}), 
                        pool.functor("cons", {pool.functor("suc", {}), 
                        pool.functor("cons", {pool.functor("suc", {}), pool.functor("zero", {})})})});
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(e);
        assert(oss.str() == "[suc, suc, suc|zero]");
        t.pop();
    }

    // Test 12: Named var is printed by name, not by index
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "X"}};
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.var(0));
        assert(oss.str() == "X");
        t.pop();
    }

    // Test 13: Two distinct named vars each print their own name
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "X"}, {1, "Y"}};
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.var(0));
        oss << ",";
        ep(pool.var(1));
        assert(oss.str() == "X,Y");
        t.pop();
    }

    // Test 14: Var whose index is not in the map falls back to ?<index>
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "X"}};
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.var(5));
        assert(oss.str() == "?5");
        t.pop();
    }

    // Test 15: Empty var_names map — all vars fall back to ?<index>
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::ostringstream oss;
        expr_printer ep(oss, no_names);
        ep(pool.var(7));
        assert(oss.str() == "?7");
        t.pop();
    }

    // Test 16: Named var in cons — lhs named, rhs unnamed
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "Head"}};
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.functor("cons", {pool.var(0), pool.var(99)}));
        assert(oss.str() == "[Head|?99]");
        t.pop();
    }

    // Test 17: Named vars mixed with atoms in nested structure: (f . (X . Y))
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "X"}, {1, "Y"}};
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.functor("cons", {pool.functor("f", {}), pool.functor("cons", {pool.var(0), pool.var(1)})}));
        assert(oss.str() == "[f, X|Y]");
        t.pop();
    }

    // Test 18: Same var appearing multiple times — (X . X)
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "X"}};
        const expr* xv = pool.var(0);
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.functor("cons", {xv, xv}));
        assert(oss.str() == "[X|X]");
        t.pop();
    }

    // Test 19: Deeply nested named vars — (X . (Y . (Z . nil)))
    {
        trail t;
        expr_pool pool(t);
        t.push();
        std::map<uint32_t, std::string> var_names = {{0, "X"}, {1, "Y"}, {2, "Z"}};
        std::ostringstream oss;
        expr_printer ep(oss, var_names);
        ep(pool.functor("cons", {pool.var(0), pool.functor("cons", {pool.var(1), pool.functor("cons", {pool.var(2), pool.functor("nil", {})})})}));
        assert(oss.str() == "[X, Y, Z]");
        t.pop();
    }
}
void test_frontier_constructor() {
    // Test 1: empty database and fresh pool - db and lp refs stored, members empty
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(&f.db == &db);
        assert(&f.lp == &lp);
        assert(f.members.empty());
    }

    // Test 2: non-empty database - members still starts empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* a = ep.functor("head", {});
        database db;
        db.push_back({a, {}});
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(&f.db == &db);
        assert(f.members.empty());
        assert(f.members.size() == 0);
        t.pop();
    }

    // Test 3: two frontiers sharing the same db and lp reference the same objects
    {
        database db;
        lineage_pool lp;
        int_frontier f1(db, lp);
        int_frontier f2(db, lp);
        assert(&f1.db == &f2.db);
        assert(&f1.lp == &f2.lp);
        assert(f1.members.empty());
        assert(f2.members.empty());
    }

    // Test 4: large database - members still starts empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        for (int i = 0; i < 10; i++)
            db.push_back({h, {b}});
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(f.members.empty());
        assert(f.members.size() == 0);
        t.pop();
    }
}

// insert() is tested before empty()/size()/at() so those functions may
// freely use insert() as a known-good setup primitive.
void test_frontier_insert() {
    // Test 1: insert single goal - stored in members with correct value
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 42);
        assert(f.members.size() == 1);
        assert(!f.members.empty());
        assert(f.members.at(gl) == 42);
    }

    // Test 2: insert multiple distinct goals all stored independently
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const goal_lineage* gl2 = lp.goal(nullptr, 2);
        f.insert(gl0, 10);
        f.insert(gl1, 20);
        f.insert(gl2, 30);
        assert(f.members.size() == 3);
        assert(f.members.at(gl0) == 10);
        assert(f.members.at(gl1) == 20);
        assert(f.members.at(gl2) == 30);
    }

    // Test 3: insert goals that are children of a resolution lineage
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* root = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(root, 0);
        const goal_lineage* child0 = lp.goal(rl, 0);
        const goal_lineage* child1 = lp.goal(rl, 1);
        f.insert(child0, 100);
        f.insert(child1, 200);
        assert(f.members.size() == 2);
        assert(f.members.at(child0) == 100);
        assert(f.members.at(child1) == 200);
    }

    // Test 4: insert value 0 (zero boundary)
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        assert(f.members.size() == 1);
        assert(f.members.at(gl) == 0);
    }

    // Test 5: insert negative value
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, -99);
        assert(f.members.size() == 1);
        assert(f.members.at(gl) == -99);
    }

    // Test 6: goals with same body index but different resolution parents are distinct keys
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* root0 = lp.goal(nullptr, 0);
        const goal_lineage* root1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl0 = lp.resolution(root0, 0);
        const resolution_lineage* rl1 = lp.resolution(root1, 0);
        const goal_lineage* child_a = lp.goal(rl0, 0);
        const goal_lineage* child_b = lp.goal(rl1, 0);
        f.insert(child_a, 1);
        f.insert(child_b, 2);
        assert(f.members.size() == 2);
        assert(f.members.at(child_a) == 1);
        assert(f.members.at(child_b) == 2);
    }
}

void test_frontier_empty() {
    // Test 1: empty() returns true for a freshly constructed frontier
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(f.empty());
    }

    // Test 2: empty() returns false after inserting one goal
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        assert(!f.empty());
    }

    // Test 3: empty() returns false with multiple goals inserted
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        f.insert(gl0, 10);
        f.insert(gl1, 20);
        assert(!f.empty());
    }

    // Test 4: empty() returns true after clearing all members directly
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 5);
        assert(!f.empty());
        f.members.clear();
        assert(f.empty());
    }

    // Test 5: empty() is independent between two separate frontier instances
    {
        database db;
        lineage_pool lp;
        int_frontier f1(db, lp);
        int_frontier f2(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f1.insert(gl, 1);
        assert(!f1.empty());
        assert(f2.empty());
    }
}

void test_frontier_size() {
    // Test 1: size() is 0 for a freshly constructed frontier
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(f.size() == 0);
    }

    // Test 2: size() increments with each insert
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const goal_lineage* gl2 = lp.goal(nullptr, 2);
        f.insert(gl0, 1);
        assert(f.size() == 1);
        f.insert(gl1, 2);
        assert(f.size() == 2);
        f.insert(gl2, 3);
        assert(f.size() == 3);
    }

    // Test 3: size() matches empty() - size 0 iff empty
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(f.size() == 0);
        assert(f.empty());
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        assert(f.size() == 1);
        assert(!f.empty());
    }

    // Test 4: size() tracked correctly across five sequential inserts
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        for (size_t i = 0; i < 5; i++) {
            const goal_lineage* gl = lp.goal(nullptr, i);
            f.insert(gl, (int)i);
            assert(f.size() == i + 1);
        }
        assert(f.size() == 5);
    }

    // Test 5: size() is independent between two separate frontier instances
    {
        database db;
        lineage_pool lp;
        int_frontier f1(db, lp);
        int_frontier f2(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        f1.insert(gl0, 10);
        f1.insert(gl1, 20);
        assert(f1.size() == 2);
        assert(f2.size() == 0);
    }
}

void test_frontier_at() {
    // Test 1: at() retrieves the inserted value
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 77);
        assert(f.at(gl) == 77);
    }

    // Test 2: at() returns independent values for distinct goals
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        f.insert(gl0, 5);
        f.insert(gl1, 15);
        assert(f.at(gl0) == 5);
        assert(f.at(gl1) == 15);
    }

    // Test 3: const at() returns the same value as mutable at()
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 55);
        const int_frontier& cf = f;
        assert(cf.at(gl) == 55);
        assert(cf.at(gl) == f.at(gl));
    }

    // Test 4: mutable at() allows value modification in-place
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 1);
        f.at(gl) = 999;
        assert(f.at(gl) == 999);
    }

    // Test 5: at() throws std::out_of_range for a goal not in the frontier
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        assert_throws(f.at(gl), std::out_of_range);
    }

    // Test 6: const at() throws std::out_of_range for a missing goal
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const int_frontier& cf = f;
        assert_throws(cf.at(gl), std::out_of_range);
    }
}

void test_frontier_begin_end() {
    // Test 1: begin() == end() for an empty frontier
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        assert(f.begin() == f.end());
    }

    // Test 2: const begin() == const end() for an empty frontier
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const int_frontier& cf = f;
        assert(cf.begin() == cf.end());
    }

    // Test 3: range-for over a single element yields the correct key-value pair
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 42);
        int count = 0;
        for (auto& [k, v] : f) {
            assert(k == gl);
            assert(v == 42);
            count++;
        }
        assert(count == 1);
    }

    // Test 4: range-for over multiple elements visits every entry exactly once
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const goal_lineage* gl2 = lp.goal(nullptr, 2);
        f.insert(gl0, 10);
        f.insert(gl1, 20);
        f.insert(gl2, 30);
        int sum = 0;
        int count = 0;
        for (auto& [k, v] : f) {
            sum += v;
            count++;
        }
        assert(count == 3);
        assert(sum == 60);
    }

    // Test 5: const range-for works correctly
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 7);
        const int_frontier& cf = f;
        int count = 0;
        for (const auto& [k, v] : cf) {
            assert(k == gl);
            assert(v == 7);
            count++;
        }
        assert(count == 1);
    }

    // Test 6: iterating with explicit begin()/end() spans exactly size() elements
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const goal_lineage* gl2 = lp.goal(nullptr, 2);
        f.insert(gl0, 1);
        f.insert(gl1, 2);
        f.insert(gl2, 3);
        size_t count = 0;
        for (auto it = f.begin(); it != f.end(); ++it)
            count++;
        assert(count == f.size());
    }

    // Test 7: mutable iterator allows value modification during iteration
    {
        database db;
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        f.insert(gl0, 1);
        f.insert(gl1, 2);
        for (auto& [k, v] : f)
            v *= 10;
        assert(f.at(gl0) == 10);
        assert(f.at(gl1) == 20);
    }
}

void test_frontier_resolve() {
    // Test 1: resolve with 0-body rule erases the goal and adds no children
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        database db;
        db.push_back({h, {}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 10);
        assert(f.size() == 1);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        f.resolve(rl);
        assert(f.size() == 0);
        assert(f.empty());
        assert_throws(f.at(gl), std::out_of_range);
        t.pop();
    }

    // Test 2: resolve with 1-body rule replaces goal with exactly one child
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({h, {b}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 5);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        f.resolve(rl);
        assert(f.size() == 1);
        const goal_lineage* child = lp.goal(rl, 0);
        assert(f.at(child) == 6);
        assert_throws(f.at(gl), std::out_of_range);
        t.pop();
    }

    // Test 3: resolve with 2-body rule replaces goal with two children
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        database db;
        db.push_back({h, {b1, b2}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 3);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        f.resolve(rl);
        assert(f.size() == 2);
        const goal_lineage* c0 = lp.goal(rl, 0);
        const goal_lineage* c1 = lp.goal(rl, 1);
        assert(f.at(c0) == 4);
        assert(f.at(c1) == 4);
        t.pop();
    }

    // Test 4: resolve with 3-body rule produces three indexed children
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        const expr* b3 = ep.functor("b3", {});
        database db;
        db.push_back({h, {b1, b2, b3}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        f.resolve(rl);
        assert(f.size() == 3);
        assert(f.at(lp.goal(rl, 0)) == 1);
        assert(f.at(lp.goal(rl, 1)) == 1);
        assert(f.at(lp.goal(rl, 2)) == 1);
        t.pop();
    }

    // Test 5: resolve selects the correct rule from the database by index
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        database db;
        db.push_back({h, {}});         // rule 0: 0 body literals
        db.push_back({h, {b1}});       // rule 1: 1 body literal
        db.push_back({h, {b1, b2}});   // rule 2: 2 body literals
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        const resolution_lineage* rl = lp.resolution(gl, 2);  // use rule 2
        f.resolve(rl);
        assert(f.size() == 2);  // rule 2 has 2 body literals
        t.pop();
    }

    // Test 6: resolving one goal in a multi-goal frontier leaves others untouched
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        database db;
        db.push_back({h, {}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        f.insert(gl0, 100);
        f.insert(gl1, 200);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        f.resolve(rl);
        assert(f.size() == 1);
        assert(f.at(gl1) == 200);
        assert_throws(f.at(gl0), std::out_of_range);
        t.pop();
    }

    // Test 7: two consecutive resolves - chained resolution tree
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({h, {b, b}});  // rule 0: 2 body
        db.push_back({h, {}});      // rule 1: 0 body
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        // First resolve: gl → 2 children with value 1
        const resolution_lineage* rl0 = lp.resolution(gl, 0);
        f.resolve(rl0);
        assert(f.size() == 2);
        const goal_lineage* c0 = lp.goal(rl0, 0);
        const goal_lineage* c1 = lp.goal(rl0, 1);
        assert(f.at(c0) == 1);
        assert(f.at(c1) == 1);
        // Second resolve: c0 with rule 1 (no body) → c0 removed, c1 untouched
        const resolution_lineage* rl1 = lp.resolution(c0, 1);
        f.resolve(rl1);
        assert(f.size() == 1);
        assert(f.at(c1) == 1);
        assert_throws(f.at(c0), std::out_of_range);
        t.pop();
    }

    // Test 8: parent value is passed to expand correctly (high initial value propagates)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({h, {b}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 1000);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        f.resolve(rl);
        const goal_lineage* child = lp.goal(rl, 0);
        assert(f.at(child) == 1001);
        t.pop();
    }

    // Test 9: child goal lineage pointers are keyed by (resolution, body_index) pair
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        database db;
        db.push_back({h, {b1, b2}});
        lineage_pool lp;
        int_frontier f(db, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        f.insert(gl, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        f.resolve(rl);
        // Children are indexed 0 and 1 under the same resolution lineage
        const goal_lineage* c0 = lp.goal(rl, 0);
        const goal_lineage* c1 = lp.goal(rl, 1);
        assert(c0 != c1);
        assert(c0->parent == rl);
        assert(c0->idx == 0);
        assert(c1->parent == rl);
        assert(c1->idx == 1);
        t.pop();
    }
}

void test_weight_store_constructor() {
    auto near = [](double a, double b, double eps = 1e-9) {
        return std::abs(a - b) < eps;
    };

    // Empty goals: members stays empty, cgw initialised to 0
    {
        trail t;
        expr_pool ep(t);
        t.push();
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        assert(ws.members.empty());
        assert(near(ws.cgw, 0.0));

        t.pop();
    }

    // One goal: full unit mass in members under lp.goal(nullptr,0)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        database db;
        lineage_pool lp;
        goals gs = {h};

        weight_store ws(gs, db, lp);
        assert(ws.members.size() == 1);
        assert(near(ws.cgw, 0.0));
        const goal_lineage* gl = lp.goal(nullptr, 0);
        assert(ws.members.count(gl) == 1);
        assert(near(ws.members.at(gl), 1.0));

        t.pop();
    }

    // Three goals: 1/3 each in members, cgw still 0
    {
        trail t;
        expr_pool ep(t);
        t.push();
        database db;
        lineage_pool lp;
        goals gs = {ep.functor("a", {}), ep.functor("b", {}), ep.functor("c", {})};

        weight_store ws(gs, db, lp);
        assert(ws.members.size() == 3);
        assert(near(ws.cgw, 0.0));
        for (size_t i = 0; i < 3; ++i) {
            const goal_lineage* gl = lp.goal(nullptr, i);
            assert(ws.members.count(gl) == 1);
            assert(near(ws.members.at(gl), 1.0 / 3.0));
        }

        t.pop();
    }
}

void test_weight_store_total() {
    auto near = [](double a, double b, double eps = 1e-9) {
        return std::abs(a - b) < eps;
    };

    // total() returns cgw, which is 0 right after construction
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        database db;
        lineage_pool lp;
        goals gs = {h};

        weight_store ws(gs, db, lp);
        assert(near(ws.total(), 0.0));
        assert(near(ws.total(), ws.cgw));

        t.pop();
    }

    // total() matches cgw when there are no goals either
    {
        trail t;
        expr_pool ep(t);
        t.push();
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        assert(near(ws.total(), 0.0));
        assert(near(ws.total(), ws.cgw));

        t.pop();
    }

    // total() reads whatever cgw is set to directly
    {
        trail t;
        expr_pool ep(t);
        t.push();
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        ws.cgw = 0.75;
        assert(near(ws.total(), 0.75));
        ws.cgw = 1.0;
        assert(near(ws.total(), 1.0));

        t.pop();
    }
}

void test_weight_store_expand() {
    auto near = [](double a, double b, double eps = 1e-9) {
        return std::abs(a - b) < eps;
    };

    // Fact rule (empty body): returns empty vector, cgw accumulates weight
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        rule r{h, {}};
        auto children = ws.expand(1.0, r);
        assert(children.empty());
        assert(near(ws.cgw, 1.0));

        t.pop();
    }

    // 1-body rule: one child with full weight, cgw unchanged
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        rule r{h, {b}};
        auto children = ws.expand(1.0, r);
        assert(children.size() == 1);
        assert(near(children[0], 1.0));
        assert(near(ws.cgw, 0.0));

        t.pop();
    }

    // 2-body rule: two children each with half weight
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        rule r{h, {b1, b2}};
        auto children = ws.expand(1.0, r);
        assert(children.size() == 2);
        assert(near(children[0], 0.5));
        assert(near(children[1], 0.5));
        assert(near(ws.cgw, 0.0));

        t.pop();
    }

    // 3-body rule: three children each with a third
    {
        trail t;
        expr_pool ep(t);
        t.push();
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        const expr* b3 = ep.functor("b3", {});
        database db;
        lineage_pool lp;
        goals gs;

        weight_store ws(gs, db, lp);
        rule r{h, {b1, b2, b3}};
        auto children = ws.expand(1.0, r);
        assert(children.size() == 3);
        for (size_t i = 0; i < 3; ++i)
            assert(near(children[i], 1.0 / 3.0));
        assert(near(ws.cgw, 0.0));

        t.pop();
    }
}

void test_goal_store_constructor() {
    // Test 1: empty goals list - frontier is empty, all refs stored correctly
    {
        trail t;
        expr_pool ep(t);
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init;
        goal_store gs(db, gs_init, t, cp, bm, lp);
        assert(gs.empty());
        assert(gs.size() == 0);
        assert(&gs.db == &db);
        assert(&gs.cp == &cp);
        assert(&gs.bm == &bm);
        assert(&gs.lp == &lp);
    }

    // Test 2: single atom goal - frontier has one entry at lp.goal(nullptr, 0)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        const expr* a = ep.functor("p", {});
        goals gs_init = {a};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        assert(gs.size() == 1);
        assert(!gs.empty());
        assert(gs.at(lp.goal(nullptr, 0)) == a);
        t.pop();
    }

    // Test 3: two goals - stored at indices 0 and 1
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        const expr* a0 = ep.functor("first", {});
        const expr* a1 = ep.functor("second", {});
        goals gs_init = {a0, a1};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        assert(gs.size() == 2);
        assert(gs.at(lp.goal(nullptr, 0)) == a0);
        assert(gs.at(lp.goal(nullptr, 1)) == a1);
        t.pop();
    }

    // Test 4: five goals - all five pointers correct at indices 0-4
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        const expr* e0 = ep.functor("a", {});
        const expr* e1 = ep.functor("b", {});
        const expr* e2 = ep.functor("c", {});
        const expr* e3 = ep.functor("d", {});
        const expr* e4 = ep.functor("e", {});
        goals gs_init = {e0, e1, e2, e3, e4};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        assert(gs.size() == 5);
        assert(gs.at(lp.goal(nullptr, 0)) == e0);
        assert(gs.at(lp.goal(nullptr, 1)) == e1);
        assert(gs.at(lp.goal(nullptr, 2)) == e2);
        assert(gs.at(lp.goal(nullptr, 3)) == e3);
        assert(gs.at(lp.goal(nullptr, 4)) == e4);
        t.pop();
    }

    // Test 5: goals stored by pointer identity, not by value copy
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        const expr* a = ep.functor("goal", {});
        goals gs_init = {a};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* stored = gs.at(lp.goal(nullptr, 0));
        assert(stored == a);  // same pointer, not a copy
        t.pop();
    }
}

void test_goal_store_try_unify_head() {
    // Test 1: atom head == atom goal -> returns true, no bindings, translation_map untouched
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("match", {});
        rule r = {h, {}};
        const expr* goal = ep.functor("match", {});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(result == true);
        assert(bm.bindings.empty());
        assert(tm.empty());  // no vars in head
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 2: atom head != atom goal -> returns false, no bindings, no map entries
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("foo", {});
        rule r = {h, {}};
        const expr* goal = ep.functor("bar", {});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(!result);
        assert(bm.bindings.empty());
        assert(tm.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 3: cons head vs atom goal -> returns false (type mismatch, no vars)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        rule r = {h, {}};
        const expr* goal = ep.functor("a", {});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(!result);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 4: atom head vs cons goal -> returns false (type mismatch, no vars)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("h", {});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(!result);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 5: variable head, atom goal -> returns true; fresh var bound to atom
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        rule r = {v, {}};
        const expr* goal = ep.functor("x", {});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(result == true);
        assert(t.depth() == 1);
        // get the fresh index from the translation_map and verify the binding
        uint32_t var_idx = std::get<expr::var>(v->content).index;
        const expr* fresh_var = ep.var(tm.at(var_idx));
        assert(bm.whnf(fresh_var) == goal);
        t.pop();
    }

    // Test 6: variable head, cons goal -> returns true; fresh var bound to cons
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        rule r = {v, {}};
        const expr* goal = ep.functor("cons", {ep.functor("l", {}), ep.functor("r", {})});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(result == true);
        assert(t.depth() == 1);
        uint32_t var_idx = std::get<expr::var>(v->content).index;
        const expr* fresh_var = ep.var(tm.at(var_idx));
        assert(bm.whnf(fresh_var) == goal);
        t.pop();
    }

    // Test 7: translation_map is populated correctly after a single-var call
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        uint32_t var_idx = std::get<expr::var>(v->content).index;
        rule r = {v, {}};
        const expr* goal = ep.functor("target", {});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        gs.try_unify_head(goal, r, tm);
        assert(t.depth() == 1);
        assert(tm.size() == 1);
        assert(tm.count(var_idx) == 1);
        assert(bm.whnf(ep.var(tm.at(var_idx))) == goal);
        t.pop();
    }

    // Test 8: cons head with one var, matching cons goal -> var's fresh copy bound correctly
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        const expr* h = ep.functor("cons", {v, ep.functor("b", {})});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(result == true);
        assert(t.depth() == 1);
        uint32_t var_idx = std::get<expr::var>(v->content).index;
        assert(bm.whnf(ep.var(tm.at(var_idx))) == ep.functor("a", {}));
        t.pop();
    }

    // Test 9: cons head with two distinct vars -> translation_map has 2 entries, both bound
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        const expr* h = ep.functor("cons", {v1, v2});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        std::map<uint32_t, uint32_t> tm;
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(result == true);
        assert(tm.size() == 2);
        assert(t.depth() == 1);
        uint32_t idx1 = std::get<expr::var>(v1->content).index;
        uint32_t idx2 = std::get<expr::var>(v2->content).index;
        assert(bm.whnf(ep.var(tm.at(idx1))) == ep.functor("a", {}));
        assert(bm.whnf(ep.var(tm.at(idx2))) == ep.functor("b", {}));
        t.pop();
    }

    // Test 10: pre-populated translation_map -> copier reuses existing mapping, no new seq() call
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        uint32_t var_idx = std::get<expr::var>(v->content).index;
        rule r = {v, {}};
        // pre-populate translation_map: map var_idx to a manually allocated fresh index
        uint32_t pre_fresh = seq();
        std::map<uint32_t, uint32_t> tm;
        tm[var_idx] = pre_fresh;
        uint32_t idx_before = seq.index;  // snapshot: copier should not advance seq further
        const expr* goal = ep.functor("reuse", {});
        assert(t.depth() == 1);
        bool result = gs.try_unify_head(goal, r, tm);
        assert(result == true);
        assert(seq.index == idx_before);  // no new seq() call was made by the copier
        assert(tm.size() == 1);           // no new entry added to translation_map
        assert(bm.whnf(ep.var(pre_fresh)) == goal);
        assert(t.depth() == 1);
        t.pop();
    }
}

void test_goal_store_applicable() {
    // Test 1: atom head == atom goal -> returns true, bm unchanged
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("match", {});
        rule r = {h, {}};
        const expr* goal = ep.functor("match", {});
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(result == true);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 2: atom head != atom goal -> returns false, bm unchanged
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("foo", {});
        rule r = {h, {}};
        const expr* goal = ep.functor("bar", {});
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(!result);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 3: variable head, atom goal -> returns true; binding rolled back, bm empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        rule r = {v, {}};
        const expr* goal = ep.functor("x", {});
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(result == true);
        assert(bm.bindings.empty());  // binding was rolled back
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 4: cons head with var, fully matching cons goal -> true; binding rolled back
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        const expr* h = ep.functor("cons", {v, ep.functor("b", {})});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(result == true);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 5: type mismatch (atom head vs cons goal) -> false, bm empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* h = ep.functor("h", {});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(!result);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 6: partial-unification failure - cons(V, atom("c")) vs cons(atom("a"), atom("d"))
    // V's fresh copy binds to "a" before "c" != "d" fails; partial binding rolled back
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        const expr* h = ep.functor("cons", {v, ep.functor("c", {})});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("d", {})});
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(!result);
        assert(bm.bindings.empty());  // partial binding of v's fresh copy was rolled back
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 7: seq.index is restored after a successful call
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        rule r = {v, {}};
        const expr* goal = ep.functor("x", {});
        uint32_t idx_before = seq.index;
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(result == true);
        assert(seq.index == idx_before);  // fresh index allocation was rolled back
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 8: seq.index is restored after a failed call (partial unification with a var)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        const expr* h = ep.functor("cons", {v, ep.functor("c", {})});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("d", {})});
        uint32_t idx_before = seq.index;
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(!result);
        assert(seq.index == idx_before);  // seq rolled back even though var was allocated
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 9: two successive calls - no state bleed between them
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v = ep.var(seq());
        rule r = {v, {}};
        const expr* goal = ep.functor("x", {});
        assert(t.depth() == 1);
        bool result1 = gs.applicable(goal, r);
        assert(result1 == true);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        bool result2 = gs.applicable(goal, r);
        assert(result2 == true);
        assert(bm.bindings.empty());
        assert(t.depth() == 1);
        t.pop();
    }

    // Test 10: cons head with two distinct vars, cons goal -> true; both bindings and both
    // fresh indices rolled back, bm empty, seq.index restored
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        database db;
        goals gs_init = {};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        const expr* h = ep.functor("cons", {v1, v2});
        rule r = {h, {}};
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        uint32_t idx_before = seq.index;
        assert(t.depth() == 1);
        bool result = gs.applicable(goal, r);
        assert(result == true);
        assert(bm.bindings.empty());      // both bindings rolled back
        assert(seq.index == idx_before);  // both fresh index allocations rolled back
        assert(t.depth() == 1);
        t.pop();
    }
}

void test_goal_store_expand() {
    // ---- Group A: Body arity ----

    // Test 1: 0-body atom rule, atom goal - goal removed, frontier becomes empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("fact", {});
        database db;
        db.push_back({h, {}});
        goals gs_init = {ep.functor("fact", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        assert(gs.size() == 1);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.empty());
        t.pop();
    }

    // Test 2: 1-body atom rule, atom goal - one child equal to the body atom
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({h, {b}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == b);
        t.pop();
    }

    // Test 3: 2-body atom rule, atom goal - two children, both body atoms
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        database db;
        db.push_back({h, {b1, b2}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 2);
        assert(gs.at(lp.goal(rl, 0)) == b1);
        assert(gs.at(lp.goal(rl, 1)) == b2);
        t.pop();
    }

    // Test 4: 3-body atom rule, atom goal - three children in order
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* b1 = ep.functor("b1", {});
        const expr* b2 = ep.functor("b2", {});
        const expr* b3 = ep.functor("b3", {});
        database db;
        db.push_back({h, {b1, b2, b3}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 3);
        assert(gs.at(lp.goal(rl, 0)) == b1);
        assert(gs.at(lp.goal(rl, 1)) == b2);
        assert(gs.at(lp.goal(rl, 2)) == b3);
        t.pop();
    }

    // ---- Group B: Variable renaming ----

    // Test 5: variable head, empty body, atom goal
    // seq() used to get the rule variable index; fresh_idx tracks what index
    // expand will allocate next when it copies the head variable.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.var(seq());
        database db;
        db.push_back({h, {}});
        const expr* goal_atom = ep.functor("x", {});
        goals gs_init = {goal_atom};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        uint32_t fresh_idx = seq.index;  // next index expand will allocate
        gs.resolve(rl);
        assert(gs.empty());
        assert(bm.whnf(ep.var(fresh_idx)) == goal_atom);
        t.pop();
    }

    // Test 6: head var shared with body var - same translation map entry means same fresh copy
    // Rule: V :- V.  Goal: atom("x").
    // Both head and body copies of V get the same fresh index; child resolves to "x".
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v = ep.var(seq());
        database db;
        db.push_back({v, {v}});
        const expr* goal_atom = ep.functor("x", {});
        goals gs_init = {goal_atom};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        const expr* child_expr = gs.at(lp.goal(rl, 0));
        assert(std::holds_alternative<expr::var>(child_expr->content));
        assert(bm.whnf(child_expr) == goal_atom);
        t.pop();
    }

    // Test 7: distinct vars in head and body - separate fresh indices
    // Rule: V1 :- V2.  Goal: atom("x").
    // V1's fresh copy is bound to "x"; V2's fresh copy is unbound and becomes the child.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        database db;
        db.push_back({v1, {v2}});
        const expr* goal_atom = ep.functor("x", {});
        goals gs_init = {goal_atom};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        const expr* child_expr = gs.at(lp.goal(rl, 0));
        assert(std::holds_alternative<expr::var>(child_expr->content));
        assert(bm.whnf(child_expr) == child_expr);  // unbound fresh copy of v2
        assert(bm.bindings.size() == 1);  // only v1's fresh copy is bound to goal
        t.pop();
    }

    // Test 8: body-only variable (not in head) - gets a fresh rename, stays unbound
    // Rule: atom("h") :- V.  Goal: atom("h").
    // Head has no vars; V first appears during body copy so it gets a fresh index.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* v = ep.var(seq());
        database db;
        db.push_back({h, {v}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        const expr* child_expr = gs.at(lp.goal(rl, 0));
        assert(std::holds_alternative<expr::var>(child_expr->content));
        assert(bm.whnf(child_expr) == child_expr);  // unbound
        assert(bm.bindings.empty());  // atom head matched atom goal: no var bindings
        t.pop();
    }

    // Test 9: same var appears twice in body - both children are the same fresh pointer
    // Rule: atom("h") :- V, V.  Goal: atom("h").
    // Single translation map entry for V -> both body copies are identical.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* v = ep.var(seq());
        database db;
        db.push_back({h, {v, v}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 2);
        const expr* c0 = gs.at(lp.goal(rl, 0));
        const expr* c1 = gs.at(lp.goal(rl, 1));
        assert(c0 == c1);  // same pointer - same fresh var (same translation map entry)
        assert(std::holds_alternative<expr::var>(c0->content));
        assert(bm.whnf(c0) == c0);  // unbound
        t.pop();
    }

    // Test 10: two distinct body vars get distinct fresh indices and distinct pointers
    // Rule: atom("h") :- V1, V2.  Goal: atom("h").
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        database db;
        db.push_back({h, {v1, v2}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 2);
        const expr* c0 = gs.at(lp.goal(rl, 0));
        const expr* c1 = gs.at(lp.goal(rl, 1));
        assert(c0 != c1);  // different pointers - distinct fresh vars
        assert(std::holds_alternative<expr::var>(c0->content));
        assert(std::holds_alternative<expr::var>(c1->content));
        assert(bm.whnf(c0) == c0);  // both unbound
        assert(bm.whnf(c1) == c1);
        t.pop();
    }

    // ---- Group C: Unification semantics ----

    // Test 11: goal atom == head atom - succeeds, no bindings, child returned unchanged
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("match", {});
        const expr* b = ep.functor("result", {});
        database db;
        db.push_back({h, {b}});
        goals gs_init = {ep.functor("match", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == b);
        assert(bm.bindings.empty());  // no variable bindings were made
        t.pop();
    }

    // Test 12: goal atom "foo", head atom "bar" - unification fails, throws runtime_error
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("bar", {});
        database db;
        db.push_back({h, {}});
        goals gs_init = {ep.functor("foo", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        assert_throws(gs.resolve(rl), std::runtime_error);
        t.pop();
    }

    // Test 13: goal is atom, head is cons - type mismatch, throws
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* lhs = ep.functor("x", {});
        const expr* rhs = ep.functor("y", {});
        const expr* h = ep.functor("cons", {lhs, rhs});
        database db;
        db.push_back({h, {}});
        goals gs_init = {ep.functor("x", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        assert_throws(gs.resolve(rl), std::runtime_error);
        t.pop();
    }

    // Test 14: goal is cons, head is atom - type mismatch, throws
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        database db;
        db.push_back({h, {}});
        const expr* goal = ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})});
        goals gs_init = {goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        assert_throws(gs.resolve(rl), std::runtime_error);
        t.pop();
    }

    // Test 15: goal cons(a, b), head cons(a, c) where b != c - cons rhs mismatch, throws
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        const expr* c = ep.functor("c", {});
        const expr* h = ep.functor("cons", {a, c});
        database db;
        db.push_back({h, {}});
        const expr* goal = ep.functor("cons", {a, b});
        goals gs_init = {goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        assert_throws(gs.resolve(rl), std::runtime_error);
        t.pop();
    }

    // Test 16: goal cons(atom, atom), head cons(var, atom) - var binds to goal lhs
    // Rule: cons(V, atom("b")) :- atom("child").  Goal: cons(atom("a"), atom("b")).
    // V's fresh copy is bound to "a" after unification.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v = ep.var(seq());
        const expr* b_atom = ep.functor("b", {});
        const expr* h = ep.functor("cons", {v, b_atom});
        const expr* body_lit = ep.functor("child", {});
        database db;
        db.push_back({h, {body_lit}});
        const expr* ga = ep.functor("a", {});
        const expr* goal = ep.functor("cons", {ga, ep.functor("b", {})});
        goals gs_init = {goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        uint32_t fresh_idx = seq.index;  // fresh copy of v will get this index
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == body_lit);
        assert(bm.whnf(ep.var(fresh_idx)) == ga);
        t.pop();
    }

    // ---- Group D: Fresh variable isolation between calls ----

    // Test 17: two consecutive resolves use distinct fresh indices
    // Two goals resolved with the same var-head rule; each resolve allocates a new fresh index.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v = ep.var(seq());
        database db;
        db.push_back({v, {}});
        const expr* goal0 = ep.functor("first", {});
        const expr* goal1 = ep.functor("second", {});
        goals gs_init = {goal0, goal1};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        uint32_t fresh0 = seq.index;  // first fresh index to be allocated
        gs.resolve(rl0);
        assert(bm.whnf(ep.var(fresh0)) == goal0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(gl1, 0);
        uint32_t fresh1 = seq.index;  // second fresh index to be allocated
        gs.resolve(rl1);
        assert(fresh0 != fresh1);  // sequencer advanced between the two resolves
        assert(bm.whnf(ep.var(fresh1)) == goal1);
        assert(bm.whnf(ep.var(fresh0)) == goal0);  // first binding still intact
        assert(gs.empty());
        t.pop();
    }

    // Test 18: two different goals resolved independently - no binding interference
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h0 = ep.functor("p", {});
        const expr* h1 = ep.functor("q", {});
        const expr* b0 = ep.functor("bp", {});
        const expr* b1 = ep.functor("bq", {});
        database db;
        db.push_back({h0, {b0}});  // rule 0
        db.push_back({h1, {b1}});  // rule 1
        goals gs_init = {ep.functor("p", {}), ep.functor("q", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        const resolution_lineage* rl1 = lp.resolution(gl1, 1);
        gs.resolve(rl0);
        gs.resolve(rl1);
        assert(gs.size() == 2);
        assert(gs.at(lp.goal(rl0, 0)) == b0);
        assert(gs.at(lp.goal(rl1, 0)) == b1);
        t.pop();
    }

    // ---- Group E: Chained resolution and complex structures ----

    // Test 19: chained resolution - goal -> body literal -> resolve that too
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h1 = ep.functor("step1", {});
        const expr* h2 = ep.functor("step2", {});
        database db;
        db.push_back({h1, {ep.functor("step2", {})}});  // rule 0: step1 :- step2
        db.push_back({h2, {}});                   // rule 1: step2 (fact)
        goals gs_init = {ep.functor("step1", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl, 0);
        gs.resolve(rl0);
        assert(gs.size() == 1);
        const goal_lineage* child = lp.goal(rl0, 0);
        assert(gs.at(child) == ep.functor("step2", {}));
        const resolution_lineage* rl1 = lp.resolution(child, 1);
        gs.resolve(rl1);
        assert(gs.empty());
        t.pop();
    }

    // Test 20: nested cons head with two vars - both vars bound to matching goal parts
    // Rule: cons(V1, V2) :- atom("done").  Goal: cons(atom("a"), atom("b")).
    // Copier processes lhs first, then rhs: V1->fresh_start, V2->fresh_start+1.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        const expr* h = ep.functor("cons", {v1, v2});
        const expr* done = ep.functor("done", {});
        database db;
        db.push_back({h, {done}});
        const expr* ga = ep.functor("a", {});
        const expr* gb = ep.functor("b", {});
        const expr* goal = ep.functor("cons", {ga, gb});
        goals gs_init = {goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        uint32_t fresh_start = seq.index;
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == done);
        assert(bm.whnf(ep.var(fresh_start))     == ga);
        assert(bm.whnf(ep.var(fresh_start + 1)) == gb);
        t.pop();
    }

    // Test 21: nested cons in body - child stored in frontier is the exact cons pointer
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* body_cons = ep.functor("cons", {ep.functor("x", {}), ep.functor("y", {})});
        database db;
        db.push_back({h, {body_cons}});
        goals gs_init = {ep.functor("h", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == body_cons);
        t.pop();
    }

    // Test 22: goal is an unbound variable - unification symmetric, binds goal var to head
    // Rule: atom("h") :- atom("b").  Goal: V (unbound, created via seq).
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("h", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({h, {b}});
        const expr* goal_var = ep.var(seq());
        goals gs_init = {goal_var};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == b);
        assert(bm.whnf(goal_var) == h);
        t.pop();
    }

    // Test 23: head var shared with two body literals - binding flows to both children
    // Rule: V :- V, V.  Goal: atom("val").
    // Single fresh copy for V; both children are the same pointer, both resolve to "val".
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v = ep.var(seq());
        database db;
        db.push_back({v, {v, v}});
        const expr* goal_atom = ep.functor("val", {});
        goals gs_init = {goal_atom};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 2);
        const expr* c0 = gs.at(lp.goal(rl, 0));
        const expr* c1 = gs.at(lp.goal(rl, 1));
        assert(c0 == c1);  // same fresh var pointer
        assert(bm.whnf(c0) == goal_atom);
        assert(bm.whnf(c1) == goal_atom);
        t.pop();
    }

    // Test 24: rule with N vars - all consistently renamed, no index collisions
    // Rule: cons(V1, cons(V2, V3)) :- V1, V2, V3.
    // Goal: cons("a", cons("b", "c")).
    // Children are the fresh copies of V1, V2, V3, each bound to the matching atom.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        const expr* v3 = ep.var(seq());
        const expr* h = ep.functor("cons", {v1, ep.functor("cons", {v2, v3})});
        database db;
        db.push_back({h, {v1, v2, v3}});
        const expr* ga = ep.functor("a", {});
        const expr* gb = ep.functor("b", {});
        const expr* gc = ep.functor("c", {});
        const expr* goal = ep.functor("cons", {ga, ep.functor("cons", {gb, gc})});
        goals gs_init = {goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        gs.resolve(rl);
        assert(gs.size() == 3);
        const expr* c0 = gs.at(lp.goal(rl, 0));
        const expr* c1 = gs.at(lp.goal(rl, 1));
        const expr* c2 = gs.at(lp.goal(rl, 2));
        assert(c0 != c1 && c1 != c2 && c0 != c2);  // all distinct fresh vars
        assert(std::holds_alternative<expr::var>(c0->content));
        assert(std::holds_alternative<expr::var>(c1->content));
        assert(std::holds_alternative<expr::var>(c2->content));
        assert(bm.whnf(c0) == ga);
        assert(bm.whnf(c1) == gb);
        assert(bm.whnf(c2) == gc);
        t.pop();
    }

    // Test 25: failed resolve leaves frontier completely unchanged
    // expand() is called before members.erase(), so a throw in expand()
    // leaves the parent goal still in the frontier (never erased, no children added).
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h = ep.functor("wrong", {});
        database db;
        db.push_back({h, {}});
        const expr* original_goal = ep.functor("right", {});
        goals gs_init = {original_goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        assert(gs.size() == 1);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        assert_throws(gs.resolve(rl), std::runtime_error);
        assert(gs.size() == 1);
        assert(gs.at(gl) == original_goal);
        t.pop();
    }

    // Test 26: two-goal frontier, one resolves then one fails - failed goal still in frontier
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* h_good = ep.functor("good", {});
        const expr* h_bad  = ep.functor("bad", {});
        const expr* body   = ep.functor("result", {});
        database db;
        db.push_back({h_good, {body}});  // rule 0
        db.push_back({h_bad,  {}});      // rule 1
        goals gs_init = {ep.functor("good", {}), ep.functor("good", {})};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        gs.resolve(rl0);
        assert(gs.size() == 2);  // child of rl0 + gl1
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(gl1, 1);
        assert_throws(gs.resolve(rl1), std::runtime_error);
        assert(gs.size() == 2);  // gl1 was never erased
        assert(gs.at(gl1) == ep.functor("good", {}));
        t.pop();
    }

    // Test 27: deeply nested cons goal and matching head - all vars correctly bound
    // Rule: cons(V1, cons(atom("mid"), V2)) :- atom("leaf").
    // Copier traverses lhs-first: V1->fresh_start, V2->fresh_start+1.
    {
        trail t;
        expr_pool ep(t);
        t.push();
        sequencer seq(t);
        copier cp(seq, ep);
        bind_map bm(t);
        lineage_pool lp;
        const expr* v1 = ep.var(seq());
        const expr* v2 = ep.var(seq());
        const expr* mid = ep.functor("mid", {});
        const expr* h = ep.functor("cons", {v1, ep.functor("cons", {mid, v2})});
        const expr* leaf = ep.functor("leaf", {});
        database db;
        db.push_back({h, {leaf}});
        const expr* left  = ep.functor("left", {});
        const expr* right = ep.functor("right", {});
        const expr* goal = ep.functor("cons", {left, ep.functor("cons", {mid, right})});
        goals gs_init = {goal};
        goal_store gs(db, gs_init, t, cp, bm, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        uint32_t fresh_start = seq.index;
        gs.resolve(rl);
        assert(gs.size() == 1);
        assert(gs.at(lp.goal(rl, 0)) == leaf);
        assert(bm.whnf(ep.var(fresh_start))     == left);
        assert(bm.whnf(ep.var(fresh_start + 1)) == right);
        t.pop();
    }
}

void test_candidate_store_constructor() {
    // Test 1: Empty db, empty goals -> size 0, initial_candidates is empty
    {
        lineage_pool lp;
        database db;
        goals gs_init = {};
        candidate_store cs(db, gs_init, lp);
        assert(cs.size() == 0);
        assert(cs.initial_candidates.empty());
    }

    // Test 2: Non-empty db (3 rules), empty goals -> size 0, initial_candidates = [0,1,2]
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {};
        candidate_store cs(db, gs_init, lp);
        assert(cs.size() == 0);
        assert(cs.initial_candidates.size() == 3);
        std::vector<size_t> ic = cs.initial_candidates;
        std::sort(ic.begin(), ic.end());
        assert(ic == std::vector<size_t>({0, 1, 2}));
        t.pop();
    }

    // Test 3: Single rule, single goal -> size 1, candidate vector = [0]
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        assert(cs.size() == 1);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const std::vector<size_t>& cands = cs.at(gl0);
        assert(cands.size() == 1);
        assert(cands[0] == 0);
        t.pop();
    }

    // Test 4: 3 rules, 1 goal -> size 1, candidate vector contains {0,1,2}
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        assert(cs.size() == 1);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        std::vector<size_t> cands = cs.at(gl0);
        std::sort(cands.begin(), cands.end());
        assert(cands == std::vector<size_t>({0, 1, 2}));
        t.pop();
    }

    // Test 5: 3 rules, 2 goals -> size 2, both goals hold {0,1,2}
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        assert(cs.size() == 2);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        std::vector<size_t> c0 = cs.at(gl0);
        std::vector<size_t> c1 = cs.at(gl1);
        std::sort(c0.begin(), c0.end());
        std::sort(c1.begin(), c1.end());
        assert(c0 == std::vector<size_t>({0, 1, 2}));
        assert(c1 == std::vector<size_t>({0, 1, 2}));
        t.pop();
    }

    // Test 6: 3 rules, 3 goals -> size 3, all three goals hold {0,1,2}
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a, a};
        candidate_store cs(db, gs_init, lp);
        assert(cs.size() == 3);
        for (int i = 0; i < 3; ++i) {
            const goal_lineage* gl = lp.goal(nullptr, i);
            std::vector<size_t> cands = cs.at(gl);
            std::sort(cands.begin(), cands.end());
            assert(cands == std::vector<size_t>({0, 1, 2}));
        }
        t.pop();
    }
}

void test_candidate_store_eliminate() {
    // Test 1: pred always false -> returns 0, candidates unchanged
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t) { return false; });
        assert(removed == 0);
        for (int i = 0; i < 2; ++i) {
            std::vector<size_t> cands = cs.at(lp.goal(nullptr, i));
            std::sort(cands.begin(), cands.end());
            assert(cands == std::vector<size_t>({0, 1, 2}));
        }
        t.pop();
    }

    // Test 2: pred always true, 1 goal with 3 candidates -> returns 3, goal vector is empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t) { return true; });
        assert(removed == 3);
        assert(cs.at(lp.goal(nullptr, 0)).empty());
        t.pop();
    }

    // Test 3: eliminate one specific candidate index (1) from all goals -> returns goal count
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t c) { return c == 1; });
        assert(removed == 2);  // one per goal
        for (int i = 0; i < 2; ++i) {
            std::vector<size_t> cands = cs.at(lp.goal(nullptr, i));
            assert(cands.size() == 2);
            assert(std::find(cands.begin(), cands.end(), 1) == cands.end());
        }
        t.pop();
    }

    // Test 4: pred keyed on gl pointer -> only eliminates from one specific goal
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* target = lp.goal(nullptr, 0);
        size_t removed = cs.eliminate([target](const goal_lineage* gl, size_t) { return gl == target; });
        assert(removed == 3);
        assert(cs.at(lp.goal(nullptr, 0)).empty());
        std::vector<size_t> c1 = cs.at(lp.goal(nullptr, 1));
        std::sort(c1.begin(), c1.end());
        assert(c1 == std::vector<size_t>({0, 1, 2}));
        t.pop();
    }

    // Test 5: pred always true, 2 goals with 3 candidates each -> returns 6
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t) { return true; });
        assert(removed == 6);
        assert(cs.at(lp.goal(nullptr, 0)).empty());
        assert(cs.at(lp.goal(nullptr, 1)).empty());
        t.pop();
    }

    // Test 6: pred eliminating 2 of 3 candidates (indices 0 and 2) -> 1 remaining per goal
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t c) { return c == 0 || c == 2; });
        assert(removed == 2);
        const std::vector<size_t>& cands = cs.at(lp.goal(nullptr, 0));
        assert(cands.size() == 1);
        assert(cands[0] == 1);
        t.pop();
    }

    // Test 7: two successive eliminate calls are cumulative
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        size_t r1 = cs.eliminate([](const goal_lineage*, size_t c) { return c == 0; });
        assert(r1 == 1);
        size_t r2 = cs.eliminate([](const goal_lineage*, size_t c) { return c == 2; });
        assert(r2 == 1);
        const std::vector<size_t>& cands = cs.at(lp.goal(nullptr, 0));
        assert(cands.size() == 1);
        assert(cands[0] == 1);
        t.pop();
    }

    // Test 8: eliminate all but one from each of two goals
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t c) { return c != 1; });
        assert(removed == 4);  // 2 per goal
        for (int i = 0; i < 2; ++i) {
            const std::vector<size_t>& cands = cs.at(lp.goal(nullptr, i));
            assert(cands.size() == 1);
            assert(cands[0] == 1);
        }
        t.pop();
    }

    // Test 9: single goal, eliminate nothing matching -> returns 0, unchanged
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        size_t removed = cs.eliminate([](const goal_lineage*, size_t c) { return c == 99; });
        assert(removed == 0);
        std::vector<size_t> cands = cs.at(lp.goal(nullptr, 0));
        std::sort(cands.begin(), cands.end());
        assert(cands == std::vector<size_t>({0, 1}));
        t.pop();
    }

    // Test 10: pred depends on both gl and candidate simultaneously
    // eliminate candidate 0 from goal 0, and candidate 1 from goal 1
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        size_t removed = cs.eliminate([gl0, gl1](const goal_lineage* gl, size_t c) {
            return (gl == gl0 && c == 0) || (gl == gl1 && c == 1);
        });
        assert(removed == 2);
        const std::vector<size_t>& c0 = cs.at(gl0);
        const std::vector<size_t>& c1 = cs.at(gl1);
        assert(c0.size() == 1 && c0[0] == 1);
        assert(c1.size() == 1 && c1[0] == 0);
        t.pop();
    }
}

void test_candidate_store_unit() {
    // Test 1: empty frontier -> returns false
    {
        lineage_pool lp;
        database db;
        goals gs_init = {};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* gl = nullptr;
        size_t cand = 99;
        assert(!cs.unit(gl, cand));
    }

    // Test 2: all goals with 2+ candidates -> returns false
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* gl = nullptr;
        size_t cand = 99;
        assert(!cs.unit(gl, cand));
        t.pop();
    }

    // Test 3: one goal with 0 candidates, rest with 2+ -> returns false (0 != 1)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        cs.eliminate([&](const goal_lineage* gl, size_t) { return gl == lp.goal(nullptr, 0); });
        const goal_lineage* gl = nullptr;
        size_t cand = 99;
        assert(!cs.unit(gl, cand));
        t.pop();
    }

    // Test 4: exactly one goal with 1 candidate -> returns true, correct gl and candidate
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        // eliminate indices 0 and 2, leaving only 1
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 0 || c == 2; });
        const goal_lineage* gl = nullptr;
        size_t cand = 99;
        assert(cs.unit(gl, cand));
        assert(gl == lp.goal(nullptr, 0));
        assert(cand == 1);
        // out-param matches what's stored
        assert(cs.at(gl).size() == 1);
        assert(cs.at(gl)[0] == cand);
        t.pop();
    }

    // Test 5: one goal with 1 candidate, another with 0 -> returns true (finds the 1-candidate one)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        // gl0 gets 1 remaining (index 1), gl1 gets 0
        cs.eliminate([gl0, gl1](const goal_lineage* gl, size_t c) {
            if (gl == gl0) return c == 0 || c == 2;
            return true;  // eliminate all from gl1
        });
        const goal_lineage* out_gl = nullptr;
        size_t out_cand = 99;
        assert(cs.unit(out_gl, out_cand));
        assert(out_gl == gl0);
        assert(out_cand == 1);
        t.pop();
    }

    // Test 6: multiple goals each with 1 candidate -> returns true, result is a valid unit goal
    // (unordered_map iteration order is non-deterministic: only verify the returned pair is valid)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        // reduce both goals to exactly 1 candidate (index 1)
        cs.eliminate([](const goal_lineage*, size_t c) { return c != 1; });
        const goal_lineage* out_gl = nullptr;
        size_t out_cand = 99;
        assert(cs.unit(out_gl, out_cand));
        // verify the returned (gl, cand) pair is consistent with the store
        assert(out_gl != nullptr);
        assert(cs.at(out_gl).size() == 1);
        assert(cs.at(out_gl)[0] == out_cand);
        t.pop();
    }

    // Test 7: single goal, single candidate -> returns true, gl and candidate are correct
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* out_gl = nullptr;
        size_t out_cand = 99;
        assert(cs.unit(out_gl, out_cand));
        assert(out_gl == lp.goal(nullptr, 0));
        assert(out_cand == 0);
        t.pop();
    }

    // Test 8: first eliminate reduces goal to 2, then second reduces to 1 -> unit returns true
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        // after first call: 2 candidates remain (0 and 1)
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 2; });
        const goal_lineage* out_gl = nullptr;
        size_t out_cand = 99;
        assert(!cs.unit(out_gl, out_cand));  // still 2 candidates
        // after second call: 1 candidate remains (0)
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 1; });
        assert(cs.unit(out_gl, out_cand));
        assert(out_gl == lp.goal(nullptr, 0));
        assert(out_cand == 0);
        t.pop();
    }
}

void test_candidate_store_conflicted() {
    // Test 1: empty frontier -> returns false
    {
        lineage_pool lp;
        database db;
        goals gs_init = {};
        candidate_store cs(db, gs_init, lp);
        assert(!cs.conflicted());
    }

    // Test 2: all goals with 2+ candidates -> returns false
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        assert(!cs.conflicted());
        t.pop();
    }

    // Test 3: all goals with exactly 1 candidate -> returns false (1 >= 1, not empty)
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        cs.eliminate([](const goal_lineage*, size_t c) { return c != 0; });
        assert(!cs.conflicted());
        t.pop();
    }

    // Test 4: single goal, all candidates eliminated -> returns true
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        cs.eliminate([](const goal_lineage*, size_t) { return true; });
        assert(cs.conflicted());
        t.pop();
    }

    // Test 5: two goals, both emptied -> returns true
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        cs.eliminate([](const goal_lineage*, size_t) { return true; });
        assert(cs.conflicted());
        t.pop();
    }

    // Test 6: one of two goals emptied, other still has candidates -> returns true
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* target = lp.goal(nullptr, 0);
        cs.eliminate([target](const goal_lineage* gl, size_t) { return gl == target; });
        assert(cs.conflicted());
        t.pop();
    }

    // Test 7: partial elimination but each goal still has >= 1 -> returns false
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a, a};
        candidate_store cs(db, gs_init, lp);
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 0 || c == 2; });
        assert(!cs.conflicted());
        t.pop();
    }

    // Test 8: conflicted only after second eliminate empties the last candidate
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 0; });
        assert(!cs.conflicted());  // still 1 candidate remaining
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 1; });
        assert(cs.conflicted());   // now empty
        t.pop();
    }
}

void test_candidate_store_expand() {
    // Test 1: resolve with 0-body rule -> parent removed, frontier becomes empty
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        database db;
        db.push_back({a, {}});  // rule 0: 0-body
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        cs.resolve(rl);
        assert(cs.empty());
        t.pop();
    }

    // Test 2: resolve with 1-body rule -> single child holds initial_candidates
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({a, {b}});  // rule 0: 1-body
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        std::vector<size_t> expected = cs.initial_candidates;
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        cs.resolve(rl);
        assert(cs.size() == 1);
        std::vector<size_t> child_cands = cs.at(lp.goal(rl, 0));
        std::sort(child_cands.begin(), child_cands.end());
        std::sort(expected.begin(), expected.end());
        assert(child_cands == expected);
        t.pop();
    }

    // Test 3: resolve with 2-body rule -> both children hold initial_candidates
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({a, {b, b}});  // rule 0: 2-body
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        std::vector<size_t> expected = cs.initial_candidates;
        std::sort(expected.begin(), expected.end());
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        cs.resolve(rl);
        assert(cs.size() == 2);
        for (int i = 0; i < 2; ++i) {
            std::vector<size_t> child_cands = cs.at(lp.goal(rl, i));
            std::sort(child_cands.begin(), child_cands.end());
            assert(child_cands == expected);
        }
        t.pop();
    }

    // Test 4: resolve with 3-body rule -> all 3 children hold initial_candidates
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({a, {b, b, b}});  // rule 0: 3-body
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        std::vector<size_t> expected = cs.initial_candidates;
        std::sort(expected.begin(), expected.end());
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        cs.resolve(rl);
        assert(cs.size() == 3);
        for (int i = 0; i < 3; ++i) {
            std::vector<size_t> child_cands = cs.at(lp.goal(rl, i));
            std::sort(child_cands.begin(), child_cands.end());
            assert(child_cands == expected);
        }
        t.pop();
    }

    // Test 5: parent had candidates eliminated -> children still get full initial_candidates
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({a, {b}});  // rule 0: 1-body
        db.push_back({a, {}});
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        std::vector<size_t> full_initial = cs.initial_candidates;
        std::sort(full_initial.begin(), full_initial.end());
        // eliminate index 1 and 2 from the parent goal -> parent now has only candidate 0
        cs.eliminate([](const goal_lineage*, size_t c) { return c == 1 || c == 2; });
        assert(cs.at(lp.goal(nullptr, 0)).size() == 1);
        // resolve: child should get the FULL initial_candidates, not the parent's reduced set
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        cs.resolve(rl);
        assert(cs.size() == 1);
        std::vector<size_t> child_cands = cs.at(lp.goal(rl, 0));
        std::sort(child_cands.begin(), child_cands.end());
        assert(child_cands == full_initial);
        t.pop();
    }

    // Test 6: children's candidate vectors are independent copies
    // Modifying one child's vector does not affect the other's
    {
        trail t;
        expr_pool ep(t);
        t.push();
        lineage_pool lp;
        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        database db;
        db.push_back({a, {b, b}});  // rule 0: 2-body
        db.push_back({a, {}});
        goals gs_init = {a};
        candidate_store cs(db, gs_init, lp);
        std::vector<size_t> full_initial = cs.initial_candidates;
        std::sort(full_initial.begin(), full_initial.end());
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        cs.resolve(rl);
        // eliminate a candidate from child 0 only
        const goal_lineage* child0 = lp.goal(rl, 0);
        const goal_lineage* child1 = lp.goal(rl, 1);
        cs.eliminate([child0](const goal_lineage* gl, size_t c) {
            return gl == child0 && c == 0;
        });
        // child0 is reduced; child1 must still have the full initial set
        assert(cs.at(child0).size() == full_initial.size() - 1);
        std::vector<size_t> c1 = cs.at(child1);
        std::sort(c1.begin(), c1.end());
        assert(c1 == full_initial);
        t.pop();
    }
}

void test_mcts_decider_constructor() {
    // Test 1: Basic construction with empty stores
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        assert(&decider.cs == &cs);
        assert(&decider.sim == &sim);
    }
    
    // Test 2: Construction with non-empty stores
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        cs.insert(g1, std::vector<size_t>{0, 1});
        cs.insert(g2, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        mcts_decider decider(cs, sim);
        
        assert(&decider.cs == &cs);
        assert(decider.cs.size() == 2);
        assert(decider.cs.at(g1).size() == 2);
        assert(decider.cs.at(g2).size() == 1);
    }
}

void test_mcts_decider_choose_goal() {
    // Test 1: Single goal - should return that goal
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        size_t length_before = sim.length();
        
        const goal_lineage* chosen = decider.choose_goal();
        
        assert(chosen == g1);
        assert(sim.length() == length_before + 1);
        assert(cs.size() == 1);
    }
    
    // Test 2: Multiple goals with unvisited node - unvisited chosen first
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        cs.insert(g1, std::vector<size_t>{0});
        cs.insert(g2, std::vector<size_t>{0});
        cs.insert(g3, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 10;
        root.m_children[g1].m_visits = 5;
        root.m_children[g1].m_value = 10.0;
        
        root.m_children[g2].m_visits = 0;
        root.m_children[g2].m_value = 0.0;
        
        root.m_children[g3].m_visits = 5;
        root.m_children[g3].m_value = 10.0;
        
        mcts_decider decider(cs, sim);
        
        const goal_lineage* chosen = decider.choose_goal();
        
        assert(chosen == g2);
        assert(sim.length() == 1);
    }
    
    // Test 3: Multiple goals with different average rewards - highest chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        cs.insert(g1, std::vector<size_t>{0});
        cs.insert(g2, std::vector<size_t>{0});
        cs.insert(g3, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 100;
        
        root.m_children[g1].m_visits = 10;
        root.m_children[g1].m_value = 50.0;
        
        root.m_children[g2].m_visits = 10;
        root.m_children[g2].m_value = 900.0;
        
        root.m_children[g3].m_visits = 10;
        root.m_children[g3].m_value = 30.0;
        
        mcts_decider decider(cs, sim);
        
        const goal_lineage* chosen = decider.choose_goal();
        
        assert(chosen == g2);
        assert(sim.length() == 1);
    }
    
    // Test 4: Multiple goals, verify result is always valid
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        cs.insert(g1, std::vector<size_t>{0});
        cs.insert(g2, std::vector<size_t>{0});
        cs.insert(g3, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        mcts_decider decider(cs, sim);
        
        for (int i = 0; i < 20; i++) {
            const goal_lineage* chosen = decider.choose_goal();
            assert(chosen == g1 || chosen == g2 || chosen == g3);
        }
        
        assert(sim.length() == 20);
    }
    
    // Test 5: Two goals with similar rewards - both should be selectable
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        cs.insert(g1, std::vector<size_t>{0});
        cs.insert(g2, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.5, rng);
        
        root.m_visits = 50;
        root.m_children[g1].m_visits = 20;
        root.m_children[g1].m_value = 60.0;
        root.m_children[g2].m_visits = 20;
        root.m_children[g2].m_value = 62.0;
        
        mcts_decider decider(cs, sim);
        
        const goal_lineage* chosen = decider.choose_goal();
        
        assert(chosen == g2);
        assert(sim.length() == 1);
    }
    
    // Test 6: Many goals - verify only valid goals chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        std::vector<const goal_lineage*> gl_vec;
        for (int i = 0; i < 20; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            cs.insert(g, std::vector<size_t>{0});
            gl_vec.push_back(g);
        }
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(123);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        size_t length_before = sim.length();
        const goal_lineage* chosen = decider.choose_goal();
        
        bool found = false;
        for (const auto* g : gl_vec) {
            if (chosen == g) {
                found = true;
                break;
            }
        }
        assert(found == true);
        
        assert(sim.length() == length_before + 1);
    }
}

void test_mcts_decider_choose_candidate() {
    // Test 1: Single candidate - should return that candidate
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{5});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        size_t length_before = sim.length();
        
        size_t chosen = decider.choose_candidate(g1);
        
        assert(chosen == 5);
        assert(sim.length() == length_before + 1);
        assert(cs.at(g1).size() == 1);
    }
    
    // Test 2: Multiple candidates with unvisited node - unvisited chosen first
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{0, 1, 2});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 10;
        
        root.m_children[size_t(0)].m_visits = 3;
        root.m_children[size_t(0)].m_value = 9.0;
        
        root.m_children[size_t(1)].m_visits = 0;
        root.m_children[size_t(1)].m_value = 0.0;
        
        root.m_children[size_t(2)].m_visits = 3;
        root.m_children[size_t(2)].m_value = 9.0;
        
        mcts_decider decider(cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        assert(chosen == 1);
        assert(sim.length() == 1);
    }
    
    // Test 3: Multiple candidates with different average rewards - highest chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{0, 1, 2, 3});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 100;
        
        root.m_children[size_t(0)].m_visits = 10;
        root.m_children[size_t(0)].m_value = 40.0;
        
        root.m_children[size_t(1)].m_visits = 10;
        root.m_children[size_t(1)].m_value = 50.0;
        
        root.m_children[size_t(2)].m_visits = 10;
        root.m_children[size_t(2)].m_value = 800.0;
        
        root.m_children[size_t(3)].m_visits = 10;
        root.m_children[size_t(3)].m_value = 30.0;
        
        mcts_decider decider(cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        assert(chosen == 2);
        assert(sim.length() == 1);
    }
    
    // Test 4: Multiple candidates, verify result is always valid
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{0, 1, 2, 3, 4});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(999);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        mcts_decider decider(cs, sim);
        
        for (int i = 0; i < 15; i++) {
            size_t chosen = decider.choose_candidate(g1);
            assert(chosen >= 0 && chosen <= 4);
            const auto& vec = cs.at(g1);
            bool found = std::find(vec.begin(), vec.end(), chosen) != vec.end();
            assert(found == true);
        }
        
        assert(sim.length() == 15);
    }
    
    // Test 5: Candidates with non-contiguous indices
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{5, 17, 42});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 50;
        
        root.m_children[size_t(5)].m_visits = 10;
        root.m_children[size_t(5)].m_value = 20.0;
        
        root.m_children[size_t(17)].m_visits = 10;
        root.m_children[size_t(17)].m_value = 30.0;
        
        root.m_children[size_t(42)].m_visits = 10;
        root.m_children[size_t(42)].m_value = 500.0;
        
        mcts_decider decider(cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        assert(chosen == 42);
        assert(sim.length() == 1);
    }
    
    // Test 6: Many candidates - all valid
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        std::vector<size_t> cand;
        for (size_t i = 0; i < 30; i++)
            cand.push_back(i);
        cs.insert(g1, cand);
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(777);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        assert(chosen < 30);
        const auto& vec = cs.at(g1);
        bool found = std::find(vec.begin(), vec.end(), chosen) != vec.end();
        assert(found == true);
        
        assert(sim.length() == 1);
    }
    
    // Test 7: Verify UCB1 balances exploitation and exploration
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{0, 1});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        root.m_visits = 100;
        
        root.m_children[size_t(0)].m_visits = 90;
        root.m_children[size_t(0)].m_value = 600.0;
        
        root.m_children[size_t(1)].m_visits = 1;
        root.m_children[size_t(1)].m_value = 5.0;
        
        mcts_decider decider(cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        assert(chosen == 1);
    }
}

void test_mcts_decider() {
    // Test 1: Single goal, single candidate - both chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert(g1, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        size_t length_before = sim.length();
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        assert(chosen_goal == g1);
        assert(chosen_candidate == 0);
        assert(sim.length() == length_before + 2);
        assert(cs.size() == 1);
        assert(cs.at(g1).size() == 1);
    }
    
    // Test 2: Multiple goals and candidates - verify deterministic selection
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        cs.insert(g1, std::vector<size_t>{0, 1});
        cs.insert(g2, std::vector<size_t>{0, 1, 2});
        cs.insert(g3, std::vector<size_t>{0});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 100;
        
        root.m_children[g1].m_visits = 10;
        root.m_children[g1].m_value = 20.0;
        
        root.m_children[g2].m_visits = 10;
        root.m_children[g2].m_value = 800.0;
        
        root.m_children[g3].m_visits = 10;
        root.m_children[g3].m_value = 30.0;
        
        root.m_children[g2].m_visits = 50;
        
        root.m_children[g2].m_children[size_t(0)].m_visits = 10;
        root.m_children[g2].m_children[size_t(0)].m_value = 10.0;
        
        root.m_children[g2].m_children[size_t(1)].m_visits = 10;
        root.m_children[g2].m_children[size_t(1)].m_value = 900.0;
        
        root.m_children[g2].m_children[size_t(2)].m_visits = 10;
        root.m_children[g2].m_children[size_t(2)].m_value = 20.0;
        
        mcts_decider decider(cs, sim);
        
        size_t length_before = sim.length();
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        assert(chosen_goal == g2);
        assert(chosen_candidate == 1);
        assert(sim.length() == length_before + 2);
    }
    
    // Test 3: Verify operator() calls helpers in correct order (goal first, then candidate)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        cs.insert(g1, std::vector<size_t>{10, 20});
        cs.insert(g2, std::vector<size_t>{30});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 100;
        
        root.m_children[g1].m_visits = 20;
        root.m_children[g1].m_value = 1800.0;
        
        root.m_children[g2].m_visits = 20;
        root.m_children[g2].m_value = 60.0;
        
        root.m_children[g1].m_children[size_t(10)].m_visits = 5;
        root.m_children[g1].m_children[size_t(10)].m_value = 10.0;
        
        root.m_children[g1].m_children[size_t(20)].m_visits = 0;
        
        mcts_decider decider(cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        assert(chosen_goal == g1);
        assert(chosen_candidate == 20);
        assert(sim.length() == 2);
    }
    
    // Test 4: Multiple calls to operator() - all return valid pairs
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        cs.insert(g1, std::vector<size_t>{0, 1});
        cs.insert(g2, std::vector<size_t>{0, 1, 2});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(555);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.5, rng);
        
        mcts_decider decider(cs, sim);
        
        for (int i = 0; i < 10; i++) {
            auto [chosen_goal, chosen_candidate] = decider();
            
            assert(chosen_goal == g1 || chosen_goal == g2);
            const auto& vec = cs.at(chosen_goal);
            bool found = std::find(vec.begin(), vec.end(), chosen_candidate) != vec.end();
            assert(found == true);
        }
        
        assert(sim.length() == 20);
    }
    
    // Test 5: Deterministic test - force specific goal and candidate via tree setup
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        cs.insert(g1, std::vector<size_t>{0, 1, 2});
        cs.insert(g2, std::vector<size_t>{0, 1});
        cs.insert(g3, std::vector<size_t>{0, 1, 2, 3});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.0, rng);
        
        root.m_visits = 200;
        
        root.m_children[g1].m_visits = 20;
        root.m_children[g1].m_value = 40.0;
        
        root.m_children[g2].m_visits = 20;
        root.m_children[g2].m_value = 60.0;
        
        root.m_children[g3].m_visits = 20;
        root.m_children[g3].m_value = 2000.0;
        
        root.m_children[g3].m_visits = 100;
        
        root.m_children[g3].m_children[size_t(0)].m_visits = 10;
        root.m_children[g3].m_children[size_t(0)].m_value = 20.0;
        
        root.m_children[g3].m_children[size_t(1)].m_visits = 10;
        root.m_children[g3].m_children[size_t(1)].m_value = 30.0;
        
        root.m_children[g3].m_children[size_t(2)].m_visits = 10;
        root.m_children[g3].m_children[size_t(2)].m_value = 1000.0;
        
        root.m_children[g3].m_children[size_t(3)].m_visits = 10;
        root.m_children[g3].m_children[size_t(3)].m_value = 40.0;
        
        mcts_decider decider(cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        assert(chosen_goal == g3);
        assert(chosen_candidate == 2);
        assert(sim.length() == 2);
    }
    
    // Test 6: Verify using unvisited nodes (infinity UCB1) for candidate level
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        cs.insert(g1, std::vector<size_t>{0, 1});
        cs.insert(g2, std::vector<size_t>{0, 1, 2});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        root.m_visits = 100;
        
        root.m_children[g1].m_visits = 30;
        root.m_children[g1].m_value = 2700.0;
        
        root.m_children[g2].m_visits = 30;
        root.m_children[g2].m_value = 90.0;
        
        root.m_children[g1].m_children[size_t(0)].m_visits = 15;
        root.m_children[g1].m_children[size_t(0)].m_value = 50.0;
        root.m_children[g1].m_children[size_t(1)].m_visits = 0;
        
        mcts_decider decider(cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        assert(chosen_goal == g1);
        assert(chosen_candidate == 1);
        assert(sim.length() == 2);
    }
    
    // Test 7: Multiple calls - all return valid pairs
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        cs.insert(g1, std::vector<size_t>{5, 10, 15});
        cs.insert(g2, std::vector<size_t>{20, 25});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(999);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        mcts_decider decider(cs, sim);
        
        for (int i = 0; i < 15; i++) {
            auto [chosen_goal, chosen_candidate] = decider();
            
            const auto& vec = cs.at(chosen_goal);
            bool found = std::find(vec.begin(), vec.end(), chosen_candidate) != vec.end();
            assert(found == true);
        }
        
        assert(sim.length() == 30);
    }
    
    // Test 8: Complex tree with varied rewards - verify correct selection
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        
        cs.insert(g1, std::vector<size_t>{0});
        cs.insert(g2, std::vector<size_t>{0, 1});
        cs.insert(g3, std::vector<size_t>{0, 1, 2});
        cs.insert(g4, std::vector<size_t>{0, 1});
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(12345);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.5, rng);
        
        root.m_visits = 150;
        
        root.m_children[g1].m_visits = 15;
        root.m_children[g1].m_value = 30.0;
        
        root.m_children[g2].m_visits = 15;
        root.m_children[g2].m_value = 45.0;
        
        root.m_children[g3].m_visits = 15;
        root.m_children[g3].m_value = 1500.0;
        
        root.m_children[g4].m_visits = 15;
        root.m_children[g4].m_value = 60.0;
        
        root.m_children[g3].m_visits = 60;
        
        root.m_children[g3].m_children[size_t(0)].m_visits = 10;
        root.m_children[g3].m_children[size_t(0)].m_value = 30.0;
        
        root.m_children[g3].m_children[size_t(1)].m_visits = 10;
        root.m_children[g3].m_children[size_t(1)].m_value = 800.0;
        
        root.m_children[g3].m_children[size_t(2)].m_visits = 10;
        root.m_children[g3].m_children[size_t(2)].m_value = 50.0;
        
        mcts_decider decider(cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        assert(chosen_goal == g3);
        assert(chosen_candidate == 1);
        assert(sim.length() == 2);
    }
    
    // Test 9: Verify stores are never modified
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        database db;
        goals empty_goals;
        candidate_store cs(db, empty_goals, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        cs.insert(g1, std::vector<size_t>{0, 1});
        cs.insert(g2, std::vector<size_t>{0});
        
        size_t cs_size_before = cs.size();
        std::vector<size_t> g1_before = cs.at(g1);
        std::vector<size_t> g2_before = cs.at(g2);
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        mcts_decider decider(cs, sim);
        
        for (int i = 0; i < 10; i++) {
            decider();
        }
        
        assert(cs.size() == cs_size_before);
        assert(cs.at(g1) == g1_before);
        assert(cs.at(g2) == g2_before);
        assert(sim.length() == 20);
    }
}

void test_lemma_constructor() {
    // Test 1: Empty input — rs is empty
    {
        resolutions input;
        lemma l(input);
        assert(l.rs.empty());
    }

    // Test 2: Single root-level rl (parent goal has nullptr parent resolution) — kept as-is
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);

        resolutions input;
        input.insert(rl0);
        lemma l(input);

        assert(l.rs.size() == 1);
        assert(l.rs.count(rl0) == 1);
    }

    // Test 3: Two unrelated root-level rls — both kept (neither is an ancestor of the other)
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        lemma l(input);

        assert(l.rs.size() == 2);
        assert(l.rs.count(rl0) == 1);
        assert(l.rs.count(rl1) == 1);
    }

    // Test 4: Two-element chain {rl0, rl1} — ancestor rl0 removed, leaf rl1 kept
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        lemma l(input);

        assert(l.rs.size() == 1);
        assert(l.rs.count(rl0) == 0);
        assert(l.rs.count(rl1) == 1);
    }

    // Test 5: Three-element chain {rl0, rl1, rl2} — only deepest leaf rl2 kept
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const goal_lineage* g2 = lp.goal(rl1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        input.insert(rl2);
        lemma l(input);

        assert(l.rs.size() == 1);
        assert(l.rs.count(rl2) == 1);
        assert(l.rs.count(rl1) == 0);
        assert(l.rs.count(rl0) == 0);
    }

    // Test 6: Two siblings sharing one ancestor — ancestor removed, both siblings kept
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const goal_lineage* g2 = lp.goal(rl0, 1);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        input.insert(rl2);
        lemma l(input);

        assert(l.rs.size() == 2);
        assert(l.rs.count(rl1) == 1);
        assert(l.rs.count(rl2) == 1);
        assert(l.rs.count(rl0) == 0);
    }

    // Test 7: Only the leaf in input, ancestor absent — leaf preserved unchanged
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        resolutions input;
        input.insert(rl1); // rl0 absent from input
        lemma l(input);

        assert(l.rs.size() == 1);
        assert(l.rs.count(rl1) == 1);
    }

    // Test 8: Input is unchanged after construction (deep copy, not a reference)
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        lemma l(input);

        // The original input must be unmodified
        assert(input.size() == 2);
        assert(input.count(rl0) == 1);
        assert(input.count(rl1) == 1);
    }
}

void test_lemma_get_resolutions() {
    // Test 1: Empty lemma — get_resolutions returns empty set
    {
        resolutions input;
        lemma l(input);
        const resolutions& rs = l.get_resolutions();
        assert(rs.empty());
    }

    // Test 2: Single root-level rl — get_resolutions contains it
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);

        resolutions input;
        input.insert(rl0);
        lemma l(input);

        const resolutions& rs = l.get_resolutions();
        assert(rs.size() == 1);
        assert(rs.count(rl0) == 1);
    }

    // Test 3: Chain — get_resolutions contains only the leaf
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        lemma l(input);

        const resolutions& rs = l.get_resolutions();
        assert(rs.size() == 1);
        assert(rs.count(rl1) == 1);
        assert(rs.count(rl0) == 0);
    }

    // Test 4: Returns a const reference (same address as internal member)
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);

        resolutions input;
        input.insert(rl0);
        lemma l(input);

        const resolutions& rs1 = l.get_resolutions();
        const resolutions& rs2 = l.get_resolutions();
        assert(&rs1 == &rs2);
        assert(&rs1 == &l.rs);
    }
}

void test_lemma_remove_ancestors() {
    // Test 1: Root-level rl — grandparent is nullptr, rs unchanged
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);

        resolutions input;
        input.insert(rl0);
        lemma l(input);

        // After construction the root rl must still be present
        assert(l.rs.size() == 1);
        assert(l.rs.count(rl0) == 1);

        // Call remove_ancestors directly with a fresh visited set to confirm
        // the loop stops immediately at nullptr (visited once, rs untouched)
        std::set<const resolution_lineage*> visited;
        l.remove_ancestors(rl0, visited);

        assert(l.rs.size() == 1);
        assert(l.rs.count(rl0) == 1);
        assert(visited.count(nullptr) == 1);
    }

    // Test 2: One-level chain — direct call removes ancestor from rs
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        // Seed rs manually: both rl0 and rl1 present, then call remove_ancestors on rl1
        resolutions input;
        input.insert(rl0);
        input.insert(rl1);
        lemma l(input);

        // Constructor already trims; rl0 should already be gone.
        // Re-insert rl0 to test the helper in isolation.
        l.rs.insert(rl0);
        assert(l.rs.size() == 2);

        std::set<const resolution_lineage*> visited;
        l.remove_ancestors(rl1, visited);

        assert(l.rs.size() == 1);
        assert(l.rs.count(rl1) == 1);
        assert(l.rs.count(rl0) == 0);
        assert(visited.count(rl0) == 1);
        assert(visited.count(nullptr) == 1);
    }

    // Test 3: Pre-visited ancestor — walk stops early, no double removal
    {
        lineage_pool lp;
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_root = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl_root, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const goal_lineage* g2 = lp.goal(rl_root, 1);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        // Build a lemma that starts with all three and trim in constructor
        resolutions input;
        input.insert(rl_root);
        input.insert(rl1);
        input.insert(rl2);
        lemma l(input);

        // After construction: rl_root removed, leaves rl1 and rl2 kept
        assert(l.rs.size() == 2);
        assert(l.rs.count(rl1) == 1);
        assert(l.rs.count(rl2) == 1);

        // Re-insert rl_root so we can observe the early-break behaviour
        l.rs.insert(rl_root);

        std::set<const resolution_lineage*> visited;
        // First call: rl_root removed and marked visited
        l.remove_ancestors(rl1, visited);
        assert(l.rs.count(rl_root) == 0);
        assert(visited.count(rl_root) == 1);

        // Second call: rl_root already visited — early break, rl1 and rl2 intact
        l.remove_ancestors(rl2, visited);
        assert(l.rs.size() == 2);
        assert(l.rs.count(rl1) == 1);
        assert(l.rs.count(rl2) == 1);
    }
}

void test_cdcl_constructor() {
    // Test 1: Default construction - all containers empty, is_refuted false
    {
        cdcl c;
        assert(c.avoidances.empty());
        assert(c.watched_goals.empty());
        assert(c.eliminated_resolutions.empty());
        assert(!c.is_refuted);
    }

    // Test 2: Two independently constructed instances are independent
    {
        cdcl c1;
        cdcl c2;
        assert(&c1.avoidances != &c2.avoidances);
        assert(&c1.watched_goals != &c2.watched_goals);
        assert(&c1.eliminated_resolutions != &c2.eliminated_resolutions);
        assert(c1.avoidances.empty());
        assert(c2.avoidances.empty());
        assert(!c1.is_refuted);
        assert(!c2.is_refuted);
    }
}

void test_cdcl_upsert() {
    // Test 1: Upsert multi-element avoidance - stored, no elimination
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        avoidance av;
        av.insert(rl1);
        av.insert(rl2);

        c.upsert(0, av);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0) == av);
        assert(c.eliminated_resolutions.empty());
    }

    // Test 2: Upsert singleton avoidance - rl added to eliminated_resolutions
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        avoidance av;
        av.insert(rl1);

        c.upsert(0, av);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0) == av);
        assert(c.eliminated_resolutions.size() == 1);
        assert(c.eliminated_resolutions.count(rl1) == 1);
    }

    // Test 3: Upsert to overwrite existing id - avoidance replaced, elimination updated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        avoidance av_multi;
        av_multi.insert(rl1);
        av_multi.insert(rl2);

        c.upsert(0, av_multi);
        assert(c.avoidances.at(0) == av_multi);
        assert(c.eliminated_resolutions.empty());

        // Overwrite with singleton - now rl1 becomes eliminated
        avoidance av_single;
        av_single.insert(rl1);

        c.upsert(0, av_single);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0) == av_single);
        assert(c.eliminated_resolutions.size() == 1);
        assert(c.eliminated_resolutions.count(rl1) == 1);
    }

    // Test 4: Upsert empty avoidance - stored, no elimination, is_refuted NOT set by upsert
    {
        cdcl c;

        avoidance empty_av;
        c.upsert(0, empty_av);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0).empty());
        assert(c.eliminated_resolutions.empty());
        // is_refuted is only set by learn(), not upsert()
        assert(!c.is_refuted);
    }
}

void test_cdcl_erase() {
    // Test 1: Erase single avoidance with one rl - avoidance removed and watched_goals unlinked
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        avoidance av;
        av.insert(rl1);

        // Manually set up state as instructed
        c.avoidances[0] = av;
        c.watched_goals[g1].insert(0);

        assert(c.avoidances.size() == 1);
        assert(c.watched_goals.at(g1).size() == 1);

        c.erase(0);

        assert(c.avoidances.empty());
        assert(c.watched_goals.at(g1).empty());
    }

    // Test 2: Erase avoidance with multiple rls with distinct parents - all links removed
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const goal_lineage* g3 = lp.goal(nullptr, 2);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g3, 0);

        avoidance av;
        av.insert(rl1);
        av.insert(rl2);
        av.insert(rl3);

        c.avoidances[0] = av;
        c.watched_goals[g1].insert(0);
        c.watched_goals[g2].insert(0);
        c.watched_goals[g3].insert(0);

        c.erase(0);

        assert(c.avoidances.empty());
        assert(c.watched_goals.at(g1).empty());
        assert(c.watched_goals.at(g2).empty());
        assert(c.watched_goals.at(g3).empty());
    }

    // Test 3: Two avoidances sharing a parent - erasing one leaves the other's link intact
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g1, 1);

        avoidance av0;
        av0.insert(rl1);
        avoidance av1;
        av1.insert(rl2);

        c.avoidances[0] = av0;
        c.avoidances[1] = av1;
        c.watched_goals[g1].insert(0);
        c.watched_goals[g1].insert(1);

        assert(c.watched_goals.at(g1).size() == 2);

        c.erase(0);

        // avoidance 0 gone, avoidance 1 intact
        assert(c.avoidances.size() == 1);
        assert(c.avoidances.count(1) == 1);

        // link to avoidance 0 gone, link to avoidance 1 still there
        assert(c.watched_goals.at(g1).size() == 1);
        assert(c.watched_goals.at(g1).count(0) == 0);
        assert(c.watched_goals.at(g1).count(1) == 1);
    }

    // Test 4: Erase empty avoidance - no watched_goals cleanup needed, just removed from avoidances
    {
        cdcl c;

        avoidance empty_av;
        c.avoidances[0] = empty_av;

        c.erase(0);

        assert(c.avoidances.empty());
        assert(c.watched_goals.empty());
    }

    // Test 5: Multiple avoidances, multiple rls per avoidance - erasing one avoidance removes
    //         only its specific links; all links belonging to other avoidances remain intact
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const goal_lineage* g3 = lp.goal(nullptr, 2);

        const resolution_lineage* rl_a1 = lp.resolution(g1, 0); // av0: parent g1
        const resolution_lineage* rl_a2 = lp.resolution(g2, 0); // av0: parent g2
        const resolution_lineage* rl_b1 = lp.resolution(g2, 1); // av1: parent g2
        const resolution_lineage* rl_b2 = lp.resolution(g3, 0); // av1: parent g3

        avoidance av0;
        av0.insert(rl_a1);
        av0.insert(rl_a2);

        avoidance av1;
        av1.insert(rl_b1);
        av1.insert(rl_b2);

        // Manually set up: av0 watched by g1 and g2; av1 watched by g2 and g3
        c.avoidances[0] = av0;
        c.avoidances[1] = av1;
        c.watched_goals[g1].insert(0);
        c.watched_goals[g2].insert(0);
        c.watched_goals[g2].insert(1);
        c.watched_goals[g3].insert(1);

        assert(c.avoidances.size() == 2);
        assert(c.watched_goals.at(g1).size() == 1);
        assert(c.watched_goals.at(g2).size() == 2);
        assert(c.watched_goals.at(g3).size() == 1);

        // Erase av0 - should remove id 0 from g1 and g2, leave av1 and g3's link untouched
        c.erase(0);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.count(0) == 0);
        assert(c.avoidances.count(1) == 1);
        assert(c.avoidances.at(1) == av1);

        // g1 exclusively watched av0 - its set is now empty
        assert(c.watched_goals.at(g1).empty());

        // g2 watched both - only the link to av0 (id 0) is gone; link to av1 (id 1) remains
        assert(c.watched_goals.at(g2).size() == 1);
        assert(c.watched_goals.at(g2).count(0) == 0);
        assert(c.watched_goals.at(g2).count(1) == 1);

        // g3 only watched av1 - completely untouched
        assert(c.watched_goals.at(g3).size() == 1);
        assert(c.watched_goals.at(g3).count(1) == 1);
    }
}

void test_cdcl_learn() {
    // Test 1: Learn two-element decision store — avoidances and watched_goals populated at id 0
    //         (root-level rls have no ancestors, so lemma is identical to input)
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        decision_store ds;
        ds.insert(rl1);
        ds.insert(rl2);

        c.learn(lemma(ds));

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0) == ds);
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g2).count(0) == 1);
        assert(c.eliminated_resolutions.empty());
    }

    // Test 2: Learn two decision stores in sequence — assigned ids 0 and 1
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        decision_store ds1;
        ds1.insert(rl1);
        decision_store ds2;
        ds2.insert(rl2);

        c.learn(lemma(ds1));
        c.learn(lemma(ds2));

        assert(c.avoidances.size() == 2);
        assert(c.avoidances.at(0) == ds1);
        assert(c.avoidances.at(1) == ds2);
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g2).count(1) == 1);
    }

    // Test 3: Learn empty decision store — is_refuted becomes true
    {
        cdcl c;

        decision_store empty_ds;
        c.learn(lemma(empty_ds));

        assert(c.is_refuted);
        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0).empty());
    }

    // Test 4: Learn singleton decision store — eliminated_resolutions populated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        decision_store ds;
        ds.insert(rl1);

        c.learn(lemma(ds));

        assert(c.eliminated_resolutions.size() == 1);
        assert(c.eliminated_resolutions.count(rl1) == 1);
        assert(c.watched_goals.at(g1).count(0) == 1);
    }

    // Test 5: Two decision stores sharing the same parent goal — both links recorded in watched_goals
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g1, 1);

        decision_store ds1;
        ds1.insert(rl1);
        decision_store ds2;
        ds2.insert(rl2);

        c.learn(lemma(ds1));
        c.learn(lemma(ds2));

        // g1 watches both avoidances
        assert(c.watched_goals.at(g1).size() == 2);
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g1).count(1) == 1);
    }

    // Test 6: Decision store with predecessor — ancestor stripped, only leaf stored
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        decision_store ds;
        ds.insert(rl0); // ancestor
        ds.insert(rl1); // leaf (successor of rl0)

        c.learn(lemma(ds));

        // lemma trims rl0; stored avoidance is {rl1} only
        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0).size() == 1);
        assert(c.avoidances.at(0).count(rl1) == 1);
        assert(c.avoidances.at(0).count(rl0) == 0);
        // watched via rl1's parent (g1), not rl0's parent (g0)
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.count(g0) == 0);
        assert(c.eliminated_resolutions.size() == 1);
        assert(c.eliminated_resolutions.count(rl1) == 1);
    }

    // Test 7: Three sequential learns — ids are 0, 1, 2 respectively
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const goal_lineage* g3 = lp.goal(nullptr, 2);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g3, 0);

        decision_store ds1; ds1.insert(rl1);
        decision_store ds2; ds2.insert(rl2);
        decision_store ds3; ds3.insert(rl3);

        c.learn(lemma(ds1));
        c.learn(lemma(ds2));
        c.learn(lemma(ds3));

        assert(c.avoidances.size() == 3);
        assert(c.avoidances.count(0) == 1);
        assert(c.avoidances.count(1) == 1);
        assert(c.avoidances.count(2) == 1);
        assert(c.avoidances.at(0).count(rl1) == 1);
        assert(c.avoidances.at(1).count(rl2) == 1);
        assert(c.avoidances.at(2).count(rl3) == 1);
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g2).count(1) == 1);
        assert(c.watched_goals.at(g3).count(2) == 1);
    }

    // Test 8: Constraining the stripped ancestor has no effect on the stored avoidance.
    //         After reduction, the ancestor's parent goal is not watched, so constrain()
    //         ignores it entirely.
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0); // ancestor
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0); // leaf

        decision_store ds;
        ds.insert(rl0);
        ds.insert(rl1);
        c.learn(lemma(ds)); // stored avoidance = {rl1}; g0 not watched

        // Constrain with a sibling of rl0 — g0 is not watched, so no avoidance is affected
        const resolution_lineage* rl0_alt = lp.resolution(g0, 1);
        c.constrain(rl0_alt);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.at(0).size() == 1);
        assert(c.avoidances.at(0).count(rl1) == 1);
    }

    // Test 9: Same decision store learned twice — two independent avoidances at ids 0 and 1
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        decision_store ds;
        ds.insert(rl1);
        ds.insert(rl2);

        c.learn(lemma(ds));
        c.learn(lemma(ds));

        // Two distinct avoidance entries with the same content
        assert(c.avoidances.size() == 2);
        assert(c.avoidances.at(0) == c.avoidances.at(1));
        // Both avoidances watched by both goals
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g1).count(1) == 1);
        assert(c.watched_goals.at(g2).count(0) == 1);
        assert(c.watched_goals.at(g2).count(1) == 1);
    }

    // Test 10: is_refuted persists across subsequent learns
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        decision_store empty_ds;
        c.learn(lemma(empty_ds)); // triggers refutation
        assert(c.is_refuted);

        decision_store ds;
        ds.insert(rl1);
        c.learn(lemma(ds)); // additional learn after refutation

        assert(c.is_refuted); // still refuted
        assert(c.avoidances.size() == 2);
    }

    // Test 11: Multi-level chain (3 levels) — only the deepest leaf's parent is watched
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        const goal_lineage* g1 = lp.goal(rl0, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const goal_lineage* g2 = lp.goal(rl1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        decision_store ds;
        ds.insert(rl0);
        ds.insert(rl1);
        ds.insert(rl2);

        c.learn(lemma(ds));

        // Only rl2 survives reduction
        assert(c.avoidances.at(0).size() == 1);
        assert(c.avoidances.at(0).count(rl2) == 1);
        // Only g2 is watched; g0 and g1 are not
        assert(c.watched_goals.count(g0) == 0);
        assert(c.watched_goals.count(g1) == 0);
        assert(c.watched_goals.at(g2).count(0) == 1);
        // Singleton → rl2 immediately eliminated
        assert(c.eliminated_resolutions.count(rl2) == 1);
        assert(c.eliminated_resolutions.count(rl1) == 0);
        assert(c.eliminated_resolutions.count(rl0) == 0);
    }

    // Test 12: Eliminated resolution persists in eliminated_resolutions even after
    //          the singleton avoidance that caused it is subsequently erased.
    //          erase() removes the avoidance but does not clean eliminated_resolutions.
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0); // the singleton
        const resolution_lineage* rl1_alt = lp.resolution(g1, 1); // sibling (for conflict)

        decision_store ds;
        ds.insert(rl1);
        c.learn(lemma(ds)); // singleton → rl1 eliminated, avoidance watched by g1

        assert(c.eliminated_resolutions.count(rl1) == 1);
        assert(c.avoidances.size() == 1);

        // Constrain with sibling of rl1: g1 is watched but rl1_alt is not in avoidance → erase
        c.constrain(rl1_alt);

        assert(c.avoidances.empty()); // avoidance erased
        // rl1 remains in eliminated_resolutions even though the avoidance is gone
        assert(c.eliminated_resolutions.count(rl1) == 1);
    }

    // Test 13: Id collision bug — learn() uses avoidances.size() as the next id.
    //          After an erasure the size shrinks, so a subsequent learn() can assign
    //          an id that is already occupied by a surviving avoidance.
    //
    //          Sequence:
    //            learn(ds1) → id 0        avoidances = {0, 1} (size 2)
    //            learn(ds2) → id 1        |
    //            constrain (erase av0)   → avoidances = {1}   (size 1)
    //            learn(ds3) → id should be 2, but avoidances.size() == 1 → id 1 → COLLISION
    //
    //          Correct behaviour: three learns with one erasure in between should yield
    //          exactly two avoidances with distinct ids.
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const goal_lineage* g3 = lp.goal(nullptr, 2);
        const resolution_lineage* rl1 = lp.resolution(g1, 0); // in av0
        const resolution_lineage* rl1_alt = lp.resolution(g1, 1); // conflict for g1
        const resolution_lineage* rl2 = lp.resolution(g2, 0); // in av1
        const resolution_lineage* rl3 = lp.resolution(g3, 0); // in av2 (third learn)

        decision_store ds1; ds1.insert(rl1);
        decision_store ds2; ds2.insert(rl2);
        decision_store ds3; ds3.insert(rl3);

        c.learn(lemma(ds1)); // avoidances = {0: {rl1}}
        c.learn(lemma(ds2)); // avoidances = {0: {rl1}, 1: {rl2}}

        assert(c.avoidances.size() == 2);

        // Erase av0: constrain with sibling of rl1 (g1 is watched, rl1_alt not in av0 → erase)
        c.constrain(rl1_alt);

        assert(c.avoidances.size() == 1);
        assert(c.avoidances.count(0) == 0);
        assert(c.avoidances.count(1) == 1);
        assert(c.avoidances.at(1).count(rl2) == 1); // av1 intact

        // Third learn — must get a fresh id (2) and must NOT overwrite av1 at id 1
        c.learn(lemma(ds3));

        assert(c.avoidances.size() == 2);                   // av1 + av2
        assert(c.avoidances.at(1).count(rl2) == 1);         // av1 untouched
        assert(c.avoidances.count(2) == 1);                 // av2 at fresh id 2
        assert(c.avoidances.at(2).count(rl3) == 1);         // av2 content correct
        assert(c.watched_goals.at(g2).count(1) == 1);       // g2 still links to av1
        assert(c.watched_goals.at(g3).count(2) == 1);       // g3 links to av2 at id 2
    }
}

void test_cdcl_constrain() {
    // Test 1: rl IS in a two-element avoidance - rl removed, remaining rl becomes eliminated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        avoidance av;
        av.insert(rl1);
        av.insert(rl2);
        c.learn(lemma(av));

        assert(c.avoidances.at(0).size() == 2);
        assert(c.eliminated_resolutions.empty());

        c.constrain(rl1);

        // rl1 removed; avoidance is now singleton {rl2}
        assert(c.avoidances.at(0).size() == 1);
        assert(c.avoidances.at(0).count(rl1) == 0);
        assert(c.avoidances.at(0).count(rl2) == 1);

        // Singleton → rl2 is now eliminated
        assert(c.eliminated_resolutions.size() == 1);
        assert(c.eliminated_resolutions.count(rl2) == 1);
    }

    // Test 2: rl IS the singleton in an avoidance - rl removed, avoidance becomes empty;
    //         is_refuted NOT set (only learn() does that, not upsert())
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        avoidance av;
        av.insert(rl1);
        c.learn(lemma(av));

        // Singleton insert already marks rl1 eliminated
        assert(c.eliminated_resolutions.count(rl1) == 1);

        c.constrain(rl1);

        // rl1 removed; avoidance is now empty (upsert called, not insert - no refutation)
        assert(c.avoidances.at(0).empty());
        assert(!c.is_refuted);
    }

    // Test 3: rl NOT in avoidance but parent goal IS watched - conflicting, erase called;
    //         avoidance and all its watched_goals links removed
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        // rl_constrain has same parent g1 as rl1, but different idx - not in the avoidance
        const resolution_lineage* rl_constrain = lp.resolution(g1, 1);

        avoidance av;
        av.insert(rl1);
        av.insert(rl2);
        c.learn(lemma(av));

        assert(c.avoidances.size() == 1);
        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g2).count(0) == 1);

        // rl_constrain not in avoidance but g1 is watched → erase(0)
        c.constrain(rl_constrain);

        assert(c.avoidances.empty());
        // g1 is the constrained parent: its key is removed entirely from watched_goals
        assert(c.watched_goals.count(g1) == 0);
        // g2 had its link cleaned by erase(); key still present but set is now empty
        assert(c.watched_goals.at(g2).empty());
    }

    // Test 4: Multiple avoidances - constrain only affects those watched by the constrained rl's parent
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g2, 1);

        // av0 = {rl1, rl2}: watched by g1 (via rl1) and g2 (via rl2)
        avoidance av0;
        av0.insert(rl1);
        av0.insert(rl2);
        c.learn(lemma(av0));

        // av1 = {rl3}: watched only by g2; constrain(rl1) does not touch g2-only avoidances
        avoidance av1;
        av1.insert(rl3);
        c.learn(lemma(av1));

        assert(c.avoidances.size() == 2);

        c.constrain(rl1);

        // av0 reduced: rl1 removed → {rl2}, upsert → rl2 eliminated
        assert(c.avoidances.at(0).size() == 1);
        assert(c.avoidances.at(0).count(rl2) == 1);
        assert(c.eliminated_resolutions.count(rl2) == 1);

        // av1 untouched (g1 does not watch it)
        assert(c.avoidances.at(1) == av1);
    }

    // Test 5: Erase path - conflicting avoidance's watched_goals links for ALL its rls are cleaned up
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl_constrain = lp.resolution(g1, 1);

        // Avoidance contains rl1 (parent g1) and rl2 (parent g2)
        // rl_constrain (parent g1) is not in the avoidance → triggers erase
        avoidance av;
        av.insert(rl1);
        av.insert(rl2);
        c.learn(lemma(av));

        assert(c.watched_goals.at(g1).count(0) == 1);
        assert(c.watched_goals.at(g2).count(0) == 1);

        c.constrain(rl_constrain);

        // erase(0): avoidance removed, g2's link to 0 cleared by erase(); g1's key removed entirely
        assert(c.avoidances.empty());
        assert(c.watched_goals.count(g1) == 0);
        assert(c.watched_goals.at(g2).empty());
    }

    // Test 4: Multiple avoidances - constrain only affects those watched by the constrained rl's parent
    // (already exists above — this numbering continues from the existing tests)

    // Test 6: constrain() for a goal with no avoidances watching it — must be a no-op;
    //         no spurious empty entry should appear in watched_goals.
    //         Bug: operator[] in "watched_goals[rl->parent]" creates an empty set entry
    //         for any goal passed to constrain(), even if nothing watches it.
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        assert(c.watched_goals.empty());

        // g1 has no avoidances watching it — constrain must be a no-op
        c.constrain(rl1);

        assert(c.avoidances.empty());
        // watched_goals must remain empty; no spurious entry for g1 must be created
        assert(c.watched_goals.empty());
    }

    // Test 7: Stale watched_goals link after reduce-then-erase causes crash on a
    //         second constrain for the same parent.
    //
    //         Sequence:
    //           learn({rl1, rl2})          → av0 watched by g1 and g2
    //           constrain(rl1)             → rl1 found; av0 reduced to {rl2};
    //                                        watched_goals[g1] still = {0}  ← stale!
    //           constrain(rl2_sibling)     → g2 watched, rl2_sibling not in {rl2}
    //                                        → erase(av0); only g2 is unlinked by erase();
    //                                        watched_goals[g1] = {0} still dangling!
    //           constrain(rl1_sibling)     → ids = watched_goals[g1] = {0}
    //                                        → avoidances.at(0) throws (av0 was erased) → CRASH
    //
    //         Correct behaviour: after av0 is erased, constraining anything under g1 must
    //         be a no-op (g1 is no longer relevant to any live avoidance).
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1         = lp.resolution(g1, 0);
        const resolution_lineage* rl1_sibling = lp.resolution(g1, 1); // triggers the crash
        const resolution_lineage* rl2         = lp.resolution(g2, 0);
        const resolution_lineage* rl2_sibling = lp.resolution(g2, 1); // triggers erase

        decision_store ds;
        ds.insert(rl1);
        ds.insert(rl2);
        c.learn(lemma(ds)); // av0 = {rl1, rl2}, watched by g1 and g2

        // Step 1: constrain(rl1) — reduces av0 to {rl2}; watched_goals[g1] goes stale
        c.constrain(rl1);
        assert(c.avoidances.at(0).size() == 1);
        assert(c.avoidances.at(0).count(rl2) == 1);

        // Step 2: constrain(rl2_sibling) — g2 is watched but rl2_sibling not in {rl2}
        //         → erase(av0); only g2's link is cleaned up; g1's link is left dangling
        c.constrain(rl2_sibling);
        assert(c.avoidances.empty());

        // Step 3: constrain(rl1_sibling) — g1 has stale link to the now-deleted av0;
        //         must be a no-op, but currently crashes via avoidances.at(stale_id)
        c.constrain(rl1_sibling);

        assert(c.avoidances.empty()); // still empty — no avoidance resurrected
    }
}

void test_cdcl_refuted() {
    // Test 1: Insert non-empty avoidance - not refuted
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        avoidance av;
        av.insert(rl1);
        c.learn(lemma(av));

        assert(!c.refuted());
    }

    // Test 2: Insert empty avoidance - refuted immediately
    {
        cdcl c;

        avoidance empty_av;
        c.learn(lemma(empty_av));

        assert(c.refuted());
    }

    // Test 3: Insert non-empty then empty - refuted after second insert
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        avoidance av;
        av.insert(rl1);
        c.learn(lemma(av));
        assert(!c.refuted());

        avoidance empty_av;
        c.learn(lemma(empty_av));
        assert(c.refuted());
    }
}

void test_cdcl_eliminated() {
    // Test 1: Multi-element avoidance - no rls are eliminated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        avoidance av;
        av.insert(rl1);
        av.insert(rl2);
        c.learn(lemma(av));

        assert(!c.eliminated(rl1));
        assert(!c.eliminated(rl2));
    }

    // Test 2: Singleton avoidance - its rl is eliminated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);

        avoidance av;
        av.insert(rl1);
        c.learn(lemma(av));

        assert(c.eliminated(rl1));
    }

    // Test 3: rl not in any avoidance - not eliminated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        avoidance av;
        av.insert(rl1);
        c.learn(lemma(av));

        // rl1 is eliminated (singleton), rl2 was never inserted
        assert(c.eliminated(rl1));
        assert(!c.eliminated(rl2));
    }

    // Test 4: Constrain reduces avoidance to singleton - the remaining rl becomes eliminated
    {
        lineage_pool lp;
        cdcl c;

        const goal_lineage* g1 = lp.goal(nullptr, 0);
        const goal_lineage* g2 = lp.goal(nullptr, 1);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);

        avoidance av;
        av.insert(rl1);
        av.insert(rl2);
        c.learn(lemma(av));

        assert(!c.eliminated(rl1));
        assert(!c.eliminated(rl2));

        c.constrain(rl1);

        // avoidance reduced to {rl2} → rl2 now eliminated
        assert(!c.eliminated(rl1));
        assert(c.eliminated(rl2));
    }
}

// ---------------------------------------------------------------------------
// sim_mock: minimal concrete sim subclass used to test the sim base class.
// Overrides decide_one() with a scripted, deterministic sequence.
// Overrides on_resolve() to count calls while delegating to the real base.
// ---------------------------------------------------------------------------

struct sim_mock : sim {
    sim_mock(size_t mr, const database& db_, const goals& gs_,
             trail& t_, sequencer& seq_, expr_pool& ep_,
             bind_map& bm_, lineage_pool& lp_, cdcl c_)
        : sim(sim_args{mr, db_, gs_, t_, seq_, ep_, bm_, lp_, c_}) {}

    std::vector<const resolution_lineage*> scripted;
    size_t decision_idx   = 0;
    size_t on_resolve_cnt = 0;

    const resolution_lineage* decide_one() override {
        return scripted.at(decision_idx++);
    }
    void on_resolve(const resolution_lineage*) override {
        ++on_resolve_cnt;
    }
};

// ---------------------------------------------------------------------------

void test_sim_constructor() {
    // Test 1: Empty goals — verify base class fields initialized correctly
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        assert(s.max_resolutions == 10);
        assert(s.rs.size() == 0);
        assert(s.ds.size() == 0);
        assert(&s.db == &db);
        assert(&s.t  == &t);
        assert(&s.lp == &lp);
        assert(&s.cp.sequencer_ref == &seq);
        assert(&s.cp.expr_pool_ref == &ep);
        assert(&s.c  != &c);  // cdcl is copied by value
        assert(s.gs.empty());
        assert(s.cs.empty());
        assert(s.decision_idx   == 0);
        assert(s.on_resolve_cnt == 0);
    }

    // Test 2: max_resolutions=0
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(0, db, gs, t, seq, ep, bm, lp, c);

        assert(s.max_resolutions == 0);
        assert(s.rs.size() == 0);
        assert(s.ds.size() == 0);
    }

    // Test 3: max_resolutions=SIZE_MAX
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(SIZE_MAX, db, gs, t, seq, ep, bm, lp, c);

        assert(s.max_resolutions == SIZE_MAX);
    }

    // Test 4: Single goal — gs and cs populated on construction
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;
        sim_mock s(100, db, gs, t, seq, ep, bm, lp, c);

        assert(s.gs.size() == 1);
        assert(s.cs.size() == 1);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(s.gs.at(gl0) == ep.functor("p", {}));
        assert(s.cs.at(gl0) == std::vector<size_t>({0}));
    }

    // Test 5: Pre-populated cdcl is deep-copied, original unchanged
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs;
        cdcl c;
        const resolution_lineage* rl = lp.resolution(lp.goal(nullptr, 0), 0);
        avoidance av; av.insert(rl);
        c.learn(lemma(av));
        size_t c_size = c.avoidances.size();
        sim_mock s(100, db, gs, t, seq, ep, bm, lp, c);

        assert(&s.c != &c);
        assert(c.avoidances.size() == c_size);   // original unchanged
        assert(s.c.avoidances.size() == 1);
    }
}

void test_sim_get_resolutions() {
    // Returns const ref to rs; empty initially
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        const resolutions& r = s.get_resolutions();
        assert(r.size() == 0);
        assert(&r == &s.rs);
    }

    // Reference is live: direct mutation to rs is visible through get_resolutions()
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        const resolutions& r = s.get_resolutions();
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        s.rs.insert(rl);

        assert(r.size() == 1);
        assert(r.count(rl) == 1);
    }
}

void test_sim_get_decisions() {
    // Returns const ref to ds; empty initially
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        const decisions& d = s.get_decisions();
        assert(d.size() == 0);
        assert(&d == &s.ds);
    }

    // Reference is live: direct mutation to ds is visible through get_decisions()
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        const decisions& d = s.get_decisions();
        const goal_lineage* gl = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl, 0);
        s.ds.insert(rl);

        assert(d.size() == 1);
        assert(d.count(rl) == 1);
    }
}

void test_sim_solved() {
    // Test 1: Empty goals → gs.empty() → true
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        goals goals;
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.solved() == true);
        assert(sim.gs.empty() == true);
    }

    // Test 2: Single goal → gs not empty → false
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.solved() == false);
        assert(sim.gs.size() == 1);
    }

    // Test 3: Multiple goals → gs not empty → false
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("q", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        goals.push_back(ep.functor("q", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.solved() == false);
        assert(sim.gs.size() == 2);
    }
}

void test_sim_conflicted() {
    // Test 1: Goal with matching rule → head elimination keeps candidate → not conflicted
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.conflicted() == false);
        // Rule 0 still present for gl0
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(sim.cs.at(gl0).size() == 1);
        assert(sim.cs.at(gl0)[0] == 0);
    }

    // Test 2: Empty database → no candidates from the start → conflicted
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.conflicted() == true);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(sim.cs.at(gl0).empty());
    }

    // Test 3: Rule head doesn't match goal → eliminated → conflicted
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("q", {}), {}}); // rule for q, goal is p → mismatch
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.conflicted() == true);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(sim.cs.at(gl0).empty());
    }

    // Test 4: CDCL singleton avoidance eliminates the only candidate → conflicted
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));

        // Pre-populate cdcl: singleton avoidance {rl} → rl is eliminated
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        cdcl c;
        avoidance av;
        av.insert(rl);
        c.learn(lemma(av));
        assert(c.eliminated(rl));

        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.conflicted() == true);
        assert(sim.cs.at(gl0).empty());
    }

    // Test 5: Two goals, one goal's rule head mismatches → conflicted regardless of other goal
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {})); // gl0: matches rule 0
        goals.push_back(ep.functor("q", {})); // gl1: no matching rule
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.conflicted() == true);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        assert(!sim.cs.at(gl0).empty()); // p still has rule 0
        assert(sim.cs.at(gl1).empty());  // q has no matching rule
    }

    // Test 6: conflicted() is idempotent - calling twice gives same result
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.conflicted() == true);
        assert(sim.conflicted() == true); // second call same result
    }
}

void test_sim_derive_one() {
    // Test 1: Single goal with multiple candidates → not unit → nullptr
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("p", {}), {}}); // two rules for p → 2 candidates → not unit
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.derive_one() == nullptr);
    }

    // Test 2: Single rule in db → cs starts with 1 candidate → unit → returns expected rl
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* expected = lp.resolution(gl0, 0);
        const resolution_lineage* result = sim.derive_one();
        assert(result != nullptr);
        assert(result == expected);
    }

    // Test 3: Empty goal store → cs is also empty → unit() false → nullptr
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        goals goals;
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.derive_one() == nullptr);
    }

    // Test 4: Two rules but only one matches the goal → after conflicted() reduces to 1 → unit
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}}); // rule 0: p :- (matches)
        db.push_back(rule{ep.functor("q", {}), {}}); // rule 1: q :- (doesn't match goal p)
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        // Initially, derive_one() returns nullptr
        assert(sim.derive_one() == nullptr);

        // Initially 2 candidates; conflicted() eliminates rule 1 (q head ≠ p goal)
        assert(sim.conflicted() == false);
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(sim.cs.at(gl0).size() == 1);

        const resolution_lineage* expected = lp.resolution(gl0, 0);
        const resolution_lineage* result = sim.derive_one();
        assert(result != nullptr);
        assert(result == expected);
    }
}

void test_sim_resolve() {
    // Test 1: Resolve goal with fact (empty body) → gs and cs become empty
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}}); // p :-
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.gs.size() == 1);
        assert(sim.cs.size() == 1);

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        sim.resolve(rl);

        // resolution recorded in rs
        assert(sim.rs.size() == 1);
        assert(sim.rs.count(rl) == 1);
        // Empty body → no sub-goals added; both stores now empty
        assert(sim.gs.empty());
        assert(sim.cs.empty());
    }

    // Test 2: Resolve goal with non-empty body → old goal removed, sub-goal added
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}}); // p :- q.
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        sim.resolve(rl);

        // resolution recorded in rs
        assert(sim.rs.size() == 1);
        assert(sim.rs.count(rl) == 1);
        // p removed, q added as sub-goal
        assert(sim.gs.size() == 1);
        assert(!sim.gs.empty());

        // Sub-goal lineage: lp.goal(rl, 0)
        const goal_lineage* sub_gl = lp.goal(rl, 0);
        assert(sim.gs.at(sub_gl) == ep.functor("q", {}));

        // cs tracks the same sub-goal
        assert(sim.cs.size() == 1);
        // Old goal no longer in cs
        assert(sim.cs.at(sub_gl).size() == 1); // db has 1 rule
    }

    // Test 3: c.constrain effect - avoidance reduced from 2 to 1 → remaining rl eliminated
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}}); // p :-
        goals goals;
        goals.push_back(ep.functor("p", {}));

        // Set up cdcl: 2-element avoidance {rl0, rl1}; neither eliminated yet
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        const goal_lineage* gl_ghost = lp.goal(nullptr, 99); // sentinel goal for rl1
        const resolution_lineage* rl1 = lp.resolution(gl_ghost, 0);

        cdcl c;
        avoidance av;
        av.insert(rl0);
        av.insert(rl1);
        c.learn(lemma(av));
        assert(!c.eliminated(rl0));
        assert(!c.eliminated(rl1));

        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        // resolve(rl0) calls sim.c.constrain(rl0) → {rl0, rl1} reduces to {rl1} → rl1 eliminated
        sim.resolve(rl0);
        assert(sim.rs.size() == 1);
        assert(sim.rs.count(rl0) == 1);
        assert(sim.c.eliminated(rl1));
    }

    // Test 4: resolve with two-level chain - verify gs, cs, and rs sizes at each step
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {}), ep.functor("r", {})}}); // p :- q, r.
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(sim.gs.size() == 1);
        assert(sim.rs.size() == 0);

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        sim.resolve(rl);

        assert(sim.rs.size() == 1);
        assert(sim.rs.count(rl) == 1);
        // p removed; q and r added (body has 2 sub-goals)
        assert(sim.gs.size() == 2);
        assert(sim.cs.size() == 2);

        const goal_lineage* sub_gl0 = lp.goal(rl, 0);
        const goal_lineage* sub_gl1 = lp.goal(rl, 1);
        assert(sim.gs.at(sub_gl0) == ep.functor("q", {}));
        assert(sim.gs.at(sub_gl1) == ep.functor("r", {}));
    }
}

void test_sim() {
    // Test 1: Empty goals → solved() true immediately, loop never runs
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db; goals gs; cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        bool result = s();
        assert(result == true);
        assert(s.rs.size() == 0);
        assert(s.ds.size() == 0);
        assert(s.on_resolve_cnt == 0);
        assert(s.decision_idx == 0);
    }

    // Test 2: Conflicted immediately → returns false, loop body never runs
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("q", {}), {}});  // only rule for q
        goals gs;
        gs.push_back(ep.functor("p", {}));            // goal is p: no matching rule
        cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        bool result = s();
        assert(result == false);
        assert(s.rs.size() == 0);
        assert(s.ds.size() == 0);
        assert(s.on_resolve_cnt == 0);
        assert(s.decision_idx == 0);
    }

    // Test 3: max_resolutions=0 → while condition (0 < 0) is false immediately
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;
        sim_mock s(0, db, gs, t, seq, ep, bm, lp, c);

        bool result = s();
        assert(result == false);  // solved() = false: p still pending
        assert(s.rs.size() == 0);
        assert(s.on_resolve_cnt == 0);
        assert(s.decision_idx == 0);
    }

    // Test 4: Single fact → unit propagation resolves goal → solved
    // derive_one() path taken; decide_one() never called
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});  // p :- .
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        bool result = s();
        assert(result == true);
        assert(s.rs.size() == 1);
        assert(s.ds.size() == 0);      // derived, not decided
        assert(s.on_resolve_cnt == 1);
        assert(s.decision_idx == 0);   // decide_one never called
    }

    // Test 5: Two candidates for goal → decide_one() invoked with scripted decision
    // Rule 0 is a non-terminating branch (p :- r.); rule 1 is the fact (p :- .).
    // Scripted decision picks rule 1 to verify we correctly select a non-zero index.
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("r", {})}});  // rule 0: p :- r.
        db.push_back(rule{ep.functor("p", {}), {}});               // rule 1: p :- .
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);
        s.scripted.push_back(lp.resolution(gl0, 1));  // choose rule 1 (the fact)

        bool result = s();
        assert(result == true);
        assert(s.rs.size() == 1);
        assert(s.ds.size() == 1);          // it was a decision
        assert(s.on_resolve_cnt == 1);
        assert(s.decision_idx == 1);
        assert(s.rs.count(lp.resolution(gl0, 1)) == 1);
        assert(s.ds.count(lp.resolution(gl0, 1)) == 1);
    }

    // Test 6: Unit-propagation chain: p :- q. then q :- . → two derives → solved
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}});  // rule 0: p :- q.
        db.push_back(rule{ep.functor("q", {}), {}});               // rule 1: q :- .
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        bool result = s();
        assert(result == true);
        assert(s.rs.size() == 2);
        assert(s.ds.size() == 0);      // all derived, no decisions
        assert(s.on_resolve_cnt == 2);
        assert(s.decision_idx == 0);
    }

    // Test 7: max_resolutions cap mid-run: p :- q :- r :- . needs 3 steps but cap=2
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}});  // rule 0: p :- q.
        db.push_back(rule{ep.functor("q", {}), {ep.functor("r", {})}});  // rule 1: q :- r.
        db.push_back(rule{ep.functor("r", {}), {}});               // rule 2: r :- .
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;
        sim_mock s(2, db, gs, t, seq, ep, bm, lp, c);

        bool result = s();
        // Resolves p→q then q→r (2 steps hits cap); r still pending → not solved
        assert(result == false);
        assert(s.rs.size() == 2);
        assert(s.ds.size() == 0);
        assert(s.on_resolve_cnt == 2);
        assert(s.decision_idx == 0);
    }

    // Test 8: Decision then unit-propagation: two candidates for p,
    // rule 0 is a dead-end branch (p :- r.); rule 1 leads to q which unit-propagates.
    // Scripted to choose rule 1 (non-zero index) → sub-goal q → fact q :- . → solved.
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("r", {})}});  // rule 0: p :- r.
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}});  // rule 1: p :- q.
        db.push_back(rule{ep.functor("q", {}), {}});               // rule 2: q :- .
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);
        s.scripted.push_back(lp.resolution(gl0, 1));  // decide: choose rule 1

        bool result = s();
        assert(result == true);
        assert(s.rs.size() == 2);      // 1 decision + 1 derive
        assert(s.ds.size() == 1);      // only the decision
        assert(s.on_resolve_cnt == 2);
        assert(s.decision_idx == 1);

        const resolution_lineage* decision_rl = lp.resolution(gl0, 1);
        assert(s.ds.count(decision_rl) == 1);
        assert(s.rs.count(decision_rl) == 1);
    }

    // Test 9: get_resolutions() and get_decisions() reflect operator() results
    {
        trail t; t.push();
        expr_pool ep(t); bind_map bm(t); sequencer seq(t); lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals gs;
        gs.push_back(ep.functor("p", {}));
        cdcl c;
        sim_mock s(10, db, gs, t, seq, ep, bm, lp, c);

        s();

        const resolutions& r = s.get_resolutions();
        const decisions& d   = s.get_decisions();
        assert(r.size() == 1);
        assert(d.size() == 0);
        assert(&r == &s.rs);
        assert(&d == &s.ds);
    }
}

void test_ridge_sim_constructor() {
    // Test 1: Empty goals - verify initialization
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;  // Empty
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        {
            ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
            
            // Verify max_resolutions stored
            assert(simulation.max_resolutions == 100);
            
            // Verify references
            assert(&simulation.db == &db);
            assert(&simulation.t == &t);
            assert(&simulation.lp == &lp);
            
            // Verify stores initialized
            assert(simulation.gs.size() == 0);
            assert(simulation.cs.size() == 0);
            assert(simulation.rs.size() == 0);
            assert(simulation.ds.size() == 0);
            
            // Verify cdcl is a distinct copy, not the same object
            assert(&simulation.c != &c);
            assert(simulation.c.avoidances.size() == 0);
            
            // CRITICAL: Verify public a01_sim member references
            assert(&simulation.db == &db);
            assert(&simulation.t == &t);
            assert(&simulation.lp == &lp);
            
            // CRITICAL: Verify copier references (public in DEBUG)
            assert(&simulation.cp.sequencer_ref == &seq);
            assert(&simulation.cp.expr_pool_ref == &ep);
            
            // CRITICAL: Verify decider references (public in DEBUG)
            assert(&simulation.dec.cs == &simulation.cs);
            assert(&simulation.dec.sim == &sim);
        }
    }
    
    // Test 2: Single goal - verify goal_adder called correctly
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);

        {

            ridge_sim simulation(sim_args{50, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
            
            // CRITICAL: Goal added to goal_store with index 0
            assert(simulation.gs.size() == 1);
            const goal_lineage* gl = lp.goal(nullptr, 0);
            assert(gl->parent == nullptr);
            assert(gl->idx == 0);
            assert(simulation.gs.at(gl) == ep.functor("p", {}));
            
            // CRITICAL: Candidate added to candidate_store (1 goal * 1 db rule = 1 candidate)
            assert(simulation.cs.size() == 1);
            assert(simulation.cs.begin()->first == gl);
            assert(simulation.cs.at(gl) == std::vector<size_t>({0}));
            
            // Other stores empty
            assert(simulation.rs.size() == 0);
            assert(simulation.ds.size() == 0);
            
            // Max resolutions stored
            assert(simulation.max_resolutions == 50);
            
            // CRITICAL: Verify lineage comes from correct pool
            assert(lp.goal_lineages.count(*gl) == 1);
        }
    }
    
    // Test 3: Multiple goals - verify sequential indexing
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("q", {}), {}});
        db.push_back(rule{ep.functor("r", {}), {}});
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        goals.push_back(ep.functor("q", {}));
        goals.push_back(ep.functor("r", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        

        ridge_sim simulation(sim_args{200, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: Three goals added with indices 0, 1, 2
        assert(simulation.gs.size() == 3);
        
        // Find each lineage by index
        const goal_lineage* gl0 = nullptr;
        const goal_lineage* gl1 = nullptr;
        const goal_lineage* gl2 = nullptr;
        
        for (const auto& [gl, ge] : simulation.gs) {
            if (gl->idx == 0) {
                gl0 = gl;
                assert(ge == ep.functor("p", {}));
            } else if (gl->idx == 1) {
                gl1 = gl;
                assert(ge == ep.functor("q", {}));
            } else if (gl->idx == 2) {
                gl2 = gl;
                assert(ge == ep.functor("r", {}));
            }
        }
        
        assert(gl0 != nullptr);
        assert(gl1 != nullptr);
        assert(gl2 != nullptr);
        
        // CRITICAL: All have nullptr parent (root goals)
        assert(gl0->parent == nullptr);
        assert(gl1->parent == nullptr);
        assert(gl2->parent == nullptr);
        
        // CRITICAL: Three goals, each with all 3 db rules as candidates
        assert(simulation.cs.size() == 3);
        
        assert(simulation.cs.at(gl0).size() == 3);
        assert(simulation.cs.at(gl1).size() == 3);
        assert(simulation.cs.at(gl2).size() == 3);
        
        assert(std::set<size_t>(simulation.cs.at(gl0).begin(), simulation.cs.at(gl0).end()) == std::set<size_t>({0, 1, 2}));
        assert(std::set<size_t>(simulation.cs.at(gl1).begin(), simulation.cs.at(gl1).end()) == std::set<size_t>({0, 1, 2}));
        assert(std::set<size_t>(simulation.cs.at(gl2).begin(), simulation.cs.at(gl2).end()) == std::set<size_t>({0, 1, 2}));
        
        // Max resolutions
        assert(simulation.max_resolutions == 200);
        
        // CRITICAL: Verify all lineages from correct pool
        assert(lp.goal_lineages.count(*gl0) == 1);
        assert(lp.goal_lineages.count(*gl1) == 1);
        assert(lp.goal_lineages.count(*gl2) == 1);
        
        // CRITICAL: Verify resolution and decision stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
    }
    
    // Test 4: Goals with no matching database rules - candidates empty for some goals
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        // No rule for "q" or "r"
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        goals.push_back(ep.functor("q", {}));
        goals.push_back(ep.functor("r", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: All goals added to goal_store
        assert(simulation.gs.size() == 3);
        
        // CRITICAL: goal_adder adds ALL db rules to ALL goals (trivially, before elimination)
        // 3 goals * 1 db rule = 3 total candidates
        assert(simulation.cs.size() == 3);
        
        const goal_lineage* gl_p = nullptr;
        const goal_lineage* gl_q = nullptr;
        const goal_lineage* gl_r = nullptr;
        
        for (const auto& [gl, ge] : simulation.gs) {
            if (gl->idx == 0) gl_p = gl;
            else if (gl->idx == 1) gl_q = gl;
            else if (gl->idx == 2) gl_r = gl;
        }
        
        assert(simulation.cs.at(gl_p) == std::vector<size_t>({0}));
        assert(simulation.cs.at(gl_q) == std::vector<size_t>({0}));
        assert(simulation.cs.at(gl_r) == std::vector<size_t>({0}));
        
        // CRITICAL: Verify resolution and decision stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify goal expressions are correct
        assert(simulation.gs.at(gl_p) == ep.functor("p", {}));
        assert(simulation.gs.at(gl_q) == ep.functor("q", {}));
        assert(simulation.gs.at(gl_r) == ep.functor("r", {}));
        
        // CRITICAL: Verify lineages from correct pool
        assert(lp.goal_lineages.count(*gl_p) == 1);
        assert(lp.goal_lineages.count(*gl_q) == 1);
        assert(lp.goal_lineages.count(*gl_r) == 1);
    }
    
    // Test 5: Database with multiple candidates per goal
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}});
        db.push_back(rule{ep.functor("p", {}), {ep.functor("r", {})}});
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Goal added
        assert(simulation.gs.size() == 1);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        
        assert(simulation.cs.at(gl).size() == 3);
        assert(std::set<size_t>(simulation.cs.at(gl).begin(), simulation.cs.at(gl).end()) == std::set<size_t>({0, 1, 2}));
        
        // CRITICAL: Verify resolution/decision stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify goal expression content
        assert(simulation.gs.at(gl) == ep.functor("p", {}));
        
        // CRITICAL: Verify lineage from correct pool
        assert(lp.goal_lineages.count(*gl) == 1);
        assert(gl->parent == nullptr);
        assert(gl->idx == 0);
    }
    
    // Test 6: Avoidance store is copied, not referenced
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        
        // Pre-populate avoidance store
        cdcl c;
        const resolution_lineage* rl1 = lp.resolution(lp.goal(nullptr, 0), 0);
        const resolution_lineage* rl2 = lp.resolution(lp.goal(nullptr, 1), 1);
        
        decision_store avoid1;
        avoid1.insert(rl1);
        avoid1.insert(rl2);
        
        c.learn(lemma(avoid1));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        size_t c_size_before = c.avoidances.size();
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: cdcl is passed by value — simulation.c is a separate copy
        assert(&simulation.c != &c);
        assert(c.avoidances.size() == c_size_before); // original unchanged
        assert(simulation.c.avoidances.size() == 1);
        
        // Content matches
        assert(std::any_of(simulation.c.avoidances.begin(), simulation.c.avoidances.end(),
            [&](const auto& p){ return p.second == avoid1; }));
    }
    
    // Test 7: Complex database and goals - verify all candidates found
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("p", {}), {ep.functor("x", {})}});
        db.push_back(rule{ep.functor("q", {}), {}});
        db.push_back(rule{ep.functor("q", {}), {ep.functor("y", {})}});
        db.push_back(rule{ep.functor("q", {}), {ep.functor("z", {})}});
        db.push_back(rule{ep.functor("r", {}), {ep.functor("w", {})}});
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        goals.push_back(ep.functor("q", {}));
        goals.push_back(ep.functor("r", {}));
        goals.push_back(ep.functor("s", {}));  // No matching rule
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{75, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: 4 goals added
        assert(simulation.gs.size() == 4);
        
        // Find each lineage
        const goal_lineage* gl0 = nullptr;
        const goal_lineage* gl1 = nullptr;
        const goal_lineage* gl2 = nullptr;
        const goal_lineage* gl3 = nullptr;
        
        for (const auto& [gl, ge] : simulation.gs) {
            if (gl->idx == 0) {
                gl0 = gl;
                assert(ge == ep.functor("p", {}));
            } else if (gl->idx == 1) {
                gl1 = gl;
                assert(ge == ep.functor("q", {}));
            } else if (gl->idx == 2) {
                gl2 = gl;
                assert(ge == ep.functor("r", {}));
            } else if (gl->idx == 3) {
                gl3 = gl;
                assert(ge == ep.functor("s", {}));
            }
        }
        
        assert(gl0 && gl1 && gl2 && gl3);
        
        assert(simulation.cs.size() == 4);
        
        assert(simulation.cs.at(gl0).size() == 6);
        assert(simulation.cs.at(gl1).size() == 6);
        assert(simulation.cs.at(gl2).size() == 6);
        assert(simulation.cs.at(gl3).size() == 6);
        
        for (const goal_lineage* gl : {gl0, gl1, gl2, gl3}) {
            assert(std::set<size_t>(simulation.cs.at(gl).begin(), simulation.cs.at(gl).end()) == std::set<size_t>({0, 1, 2, 3, 4, 5}));
        }
        
        // Max resolutions
        assert(simulation.max_resolutions == 75);
        
        // CRITICAL: Verify resolution/decision stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify all lineages from correct pool and have correct properties
        assert(lp.goal_lineages.count(*gl0) == 1);
        assert(lp.goal_lineages.count(*gl1) == 1);
        assert(lp.goal_lineages.count(*gl2) == 1);
        assert(lp.goal_lineages.count(*gl3) == 1);
        
        assert(gl0->parent == nullptr && gl0->idx == 0);
        assert(gl1->parent == nullptr && gl1->idx == 1);
        assert(gl2->parent == nullptr && gl2->idx == 2);
        assert(gl3->parent == nullptr && gl3->idx == 3);
        
        // CRITICAL: Verify goal expressions match exactly
        assert(simulation.gs.at(gl0) == ep.functor("p", {}));
        assert(simulation.gs.at(gl1) == ep.functor("q", {}));
        assert(simulation.gs.at(gl2) == ep.functor("r", {}));
        assert(simulation.gs.at(gl3) == ep.functor("s", {}));
        
        // CRITICAL: Verify database reference holds correct content
        assert(simulation.db.size() == 6);
        assert(simulation.db[0].head == ep.functor("p", {}));
        assert(simulation.db[2].head == ep.functor("q", {}));
        assert(simulation.db[5].head == ep.functor("r", {}));
    }
    
    // Test 9: Pre-populated avoidance store is copied
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        
        // Pre-populate avoidance store with multiple avoidances
        cdcl c;
        
        const resolution_lineage* rl1 = lp.resolution(lp.goal(nullptr, 0), 0);
        const resolution_lineage* rl2 = lp.resolution(lp.goal(nullptr, 1), 1);
        const resolution_lineage* rl3 = lp.resolution(lp.goal(nullptr, 2), 2);
        
        decision_store avoid1;
        avoid1.insert(rl1);
        avoid1.insert(rl2);
        
        decision_store avoid2;
        avoid2.insert(rl3);
        
        c.learn(lemma(avoid1));
        c.learn(lemma(avoid2));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        size_t c_size_before = c.avoidances.size();
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: cdcl copy has same content but is a distinct object
        assert(&simulation.c != &c);
        assert(c.avoidances.size() == c_size_before); // original unchanged
        assert(simulation.c.avoidances.size() == 2);
        assert(std::any_of(simulation.c.avoidances.begin(), simulation.c.avoidances.end(),
            [&](const auto& p){ return p.second == avoid1; }));
        assert(std::any_of(simulation.c.avoidances.begin(), simulation.c.avoidances.end(),
            [&](const auto& p){ return p.second == avoid2; }));
    }
    
    // Test 10: Verify sequencer and expr_pool passed correctly to copier
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        goals.push_back(ep.functor("p", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Verify copier references match (via members)
        assert(&simulation.cp.expr_pool_ref == &ep);
        assert(&simulation.cp.sequencer_ref == &seq);
    }
    
    // Test 11: Different max_resolutions values stored correctly
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Test various limits
        {
            resolution_store rs;
            decision_store ds;
            ridge_sim sim1(sim_args{1, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
            assert(sim1.max_resolutions == 1);
        }
        
        {
            resolution_store rs;
            decision_store ds;
            ridge_sim sim2(sim_args{1000, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
            assert(sim2.max_resolutions == 1000);
        }
        
        {
            resolution_store rs;
            decision_store ds;
            ridge_sim sim3(sim_args{999999, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
            assert(sim3.max_resolutions == 999999);
        }
    }
    
    // Test 13: Verify decisions() returns const reference to ds
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
    }
    
    // Test 14: Verify component initialization with non-empty avoidance store
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        
        // Pre-populate avoidance store
        cdcl c;
        const resolution_lineage* rl1 = lp.resolution(lp.goal(nullptr, 0), 0);
        const resolution_lineage* rl2 = lp.resolution(lp.goal(nullptr, 1), 1);
        
        decision_store avoid;
        avoid.insert(rl1);
        avoid.insert(rl2);
        c.learn(lemma(avoid));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        size_t c_size_before = c.avoidances.size();
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: Verify cdcl copy is distinct and has expected content
        assert(&simulation.c != &c);
        assert(c.avoidances.size() == c_size_before); // original unchanged
        assert(simulation.c.avoidances.size() == 1);
        assert(std::any_of(simulation.c.avoidances.begin(), simulation.c.avoidances.end(),
            [&](const auto& p){ return p.second == avoid; }));
    }
    
    // Test 15: Verify all store sizes after construction with various goal counts
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("x", {}), {}});
        db.push_back(rule{ep.functor("y", {}), {}});
        
        // Add 5 goals
        goals goals;
        for (int i = 0; i < 5; i++) {
            goals.push_back(ep.functor("goal" + std::to_string(i), {}));
        }
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{500, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: 5 goals added
        assert(simulation.gs.size() == 5);
        
        assert(simulation.cs.size() == 5);
        
        assert(&simulation.c != &c);
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        assert(simulation.c.avoidances.size() == 0);
        assert(simulation.ds.size() == 0);
        assert(simulation.rs.size() == 0);
        
        for (const auto& [gl, ge] : simulation.gs) {
            assert(simulation.cs.at(gl).size() == 2);
            assert(gl->parent == nullptr);
            assert(gl->idx >= 0 && gl->idx < 5);
        }
    }
    
    // Test 16: Verify goal ordering matches list ordering
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        
        goals goals;
        const expr* goal_a = ep.functor("alpha", {});
        const expr* goal_b = ep.functor("beta", {});
        const expr* goal_c = ep.functor("gamma", {});
        const expr* goal_d = ep.functor("delta", {});
        
        goals.push_back(goal_a);
        goals.push_back(goal_b);
        goals.push_back(goal_c);
        goals.push_back(goal_d);
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: Verify ordering - idx matches position in list
        const goal_lineage* gl_idx0 = nullptr;
        const goal_lineage* gl_idx1 = nullptr;
        const goal_lineage* gl_idx2 = nullptr;
        const goal_lineage* gl_idx3 = nullptr;
        
        for (const auto& [gl, ge] : simulation.gs) {
            if (gl->idx == 0) {
                gl_idx0 = gl;
                assert(ge == goal_a);  // First in list
            } else if (gl->idx == 1) {
                gl_idx1 = gl;
                assert(ge == goal_b);  // Second in list
            } else if (gl->idx == 2) {
                gl_idx2 = gl;
                assert(ge == goal_c);  // Third in list
            } else if (gl->idx == 3) {
                gl_idx3 = gl;
                assert(ge == goal_d);  // Fourth in list
            }
        }
        
        assert(gl_idx0 && gl_idx1 && gl_idx2 && gl_idx3);
        
        // CRITICAL: All root goals (parent == nullptr)
        assert(gl_idx0->parent == nullptr);
        assert(gl_idx1->parent == nullptr);
        assert(gl_idx2->parent == nullptr);
        assert(gl_idx3->parent == nullptr);
    }
}

void test_ridge_sim_decide_one() {
    // Test 1: Single goal, two candidates, MCTS tree pre-populated to force rule 0
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}}); // rule 0
        db.push_back(rule{ep.functor("p", {}), {}}); // rule 1
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;

        const goal_lineage* gl0 = lp.goal(nullptr, 0);

        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);

        // Pre-populate: root visited → UCB1 mode; gl0 node visited → UCB1 for candidates
        root.m_visits = 10;
        root.m_children[mcts_decider::choice{gl0}].m_visits = 10;
        root.m_children[mcts_decider::choice{gl0}].m_value  = 10.0;
        // Rule 0 has high reward, rule 1 has low reward → rule 0 chosen
        root.m_children[mcts_decider::choice{gl0}].m_children[mcts_decider::choice{(size_t)0}].m_visits = 5;
        root.m_children[mcts_decider::choice{gl0}].m_children[mcts_decider::choice{(size_t)0}].m_value  = 50.0;
        root.m_children[mcts_decider::choice{gl0}].m_children[mcts_decider::choice{(size_t)1}].m_visits = 5;
        root.m_children[mcts_decider::choice{gl0}].m_children[mcts_decider::choice{(size_t)1}].m_value  = 1.0;

        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const resolution_lineage* result = sim.decide_one();
        assert(result != nullptr);
        // Rule 0 should be chosen (higher UCB1 score)
        assert(result == lp.resolution(gl0, 0));
    }

    // Test 2: Two goals, tree pre-populated to force choice of gl1 with rule 1
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}}); // rule 0
        db.push_back(rule{ep.functor("q", {}), {}}); // rule 1
        goals goals;
        goals.push_back(ep.functor("p", {})); // gl0
        goals.push_back(ep.functor("q", {})); // gl1
        cdcl c;

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);

        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);

        // Root visited; gl1 has much higher reward → gl1 chosen as goal
        root.m_visits = 20;
        root.m_children[mcts_decider::choice{gl0}].m_visits = 10;
        root.m_children[mcts_decider::choice{gl0}].m_value  = 5.0;   // low avg
        root.m_children[mcts_decider::choice{gl1}].m_visits = 10;
        root.m_children[mcts_decider::choice{gl1}].m_value  = 90.0;  // high avg → gl1 chosen

        // At gl1 node: cs.at(gl1) = {0, 1}; rule 1 has higher reward → rule 1 chosen
        root.m_children[mcts_decider::choice{gl1}].m_children[mcts_decider::choice{(size_t)0}].m_visits = 5;
        root.m_children[mcts_decider::choice{gl1}].m_children[mcts_decider::choice{(size_t)0}].m_value  = 2.0;
        root.m_children[mcts_decider::choice{gl1}].m_children[mcts_decider::choice{(size_t)1}].m_visits = 5;
        root.m_children[mcts_decider::choice{gl1}].m_children[mcts_decider::choice{(size_t)1}].m_value  = 80.0;

        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const resolution_lineage* result = sim.decide_one();
        assert(result != nullptr);
        assert(result == lp.resolution(gl1, 1));
    }

    // Test 3: Single goal, single candidate - unvisited root (rollout mode) still returns valid rl
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;

        monte_carlo::tree_node<mcts_decider::choice> root;
        // root unvisited → rollout mode → random, but only 1 goal / 1 candidate so forced
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);

        ridge_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* result = sim.decide_one();
        assert(result != nullptr);
        assert(result == lp.resolution(gl0, 0));
    }
}

void test_horizon_sim_reward() {
    auto near = [](double a, double b, double eps = 1e-9) {
        return std::abs(a - b) < eps;
    };

    // Test 1: Empty goals → ws.cgw starts at 0 → reward() == 0
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        goals goals;
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(near(sim.reward(), 0.0));
        assert(near(sim.ws.cgw, 0.0));
    }

    // Test 2: Single goal, no resolutions yet → reward() == 0
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(near(sim.reward(), 0.0));
    }

    // Test 3: Multiple goals, no resolutions → reward() == 0
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("q", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {}));
        goals.push_back(ep.functor("q", {}));
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(near(sim.reward(), 0.0));
    }

    // Test 4: reward() reads directly from ws.cgw - set it directly and verify
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        goals goals;
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        sim.ws.cgw = 0.5;
        assert(near(sim.reward(), 0.5));

        sim.ws.cgw = 1.0;
        assert(near(sim.reward(), 1.0));

        sim.ws.cgw = 0.0;
        assert(near(sim.reward(), 0.0));
    }
}

void test_horizon_sim_on_resolve() {
    auto near = [](double a, double b, double eps = 1e-9) {
        return std::abs(a - b) < eps;
    };

    // Test 1: Resolve single goal with fact → ws.cgw gets the full weight → reward() == 1.0
    // Also confirms base class effects (gs, cs emptied)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}}); // p :-
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1/1 = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(near(sim.reward(), 0.0));

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        sim.resolve(rl);

        // ws: fact rule → full weight accumulated
        assert(near(sim.ws.cgw, 1.0));
        assert(near(sim.reward(), 1.0));
        // base class: gs and cs emptied
        assert(sim.gs.empty());
        assert(sim.cs.empty());
    }

    // Test 2: Resolve goal with non-fact rule → no weight accumulates, sub-goals weighted
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}}); // p :- q.
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        sim.resolve(rl);

        // Non-fact: cgw unchanged
        assert(near(sim.ws.cgw, 0.0));
        assert(near(sim.reward(), 0.0));
        // Sub-goal carries the full weight (1 body element)
        const goal_lineage* sub_gl = lp.goal(rl, 0);
        assert(near(sim.ws.members.at(sub_gl), 1.0));
        // base class: gs gained sub-goal q
        assert(sim.gs.size() == 1);
        assert(sim.gs.at(sub_gl) == ep.functor("q", {}));
    }

    // Test 3: Two goals, resolve one with fact → reward() == 0.5 (half of total weight)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("q", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 0.5
        goals.push_back(ep.functor("q", {})); // weight = 0.5
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        assert(near(sim.reward(), 0.0));

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        sim.resolve(rl0);

        assert(near(sim.ws.cgw, 0.5));
        assert(near(sim.reward(), 0.5));
    }

    // Test 4: Two goals, resolve both with facts → reward() == 1.0
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("q", {}), {}});
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 0.5
        goals.push_back(ep.functor("q", {})); // weight = 0.5
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);
        sim.resolve(lp.resolution(gl0, 0)); // resolve p with rule 0
        assert(near(sim.reward(), 0.5));
        sim.resolve(lp.resolution(gl1, 1)); // resolve q with rule 1
        assert(near(sim.reward(), 1.0));
        assert(near(sim.ws.cgw, 1.0));
        assert(sim.gs.empty());
    }

    // Test 5: Resolve with 2-body rule → weight split equally, cgw unchanged,
    // both sub-goals get equal weight in ws
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {}), ep.functor("r", {})}}); // p :- q, r.
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 0);
        sim.resolve(rl);

        // 2-body rule: weight 1.0 split into 0.5 each, cgw unchanged
        assert(near(sim.ws.cgw, 0.0));
        assert(near(sim.reward(), 0.0));
        const goal_lineage* sub_gl0 = lp.goal(rl, 0);
        const goal_lineage* sub_gl1 = lp.goal(rl, 1);
        assert(near(sim.ws.members.at(sub_gl0), 0.5));
        assert(near(sim.ws.members.at(sub_gl1), 0.5));
        // base class: gs has q and r
        assert(sim.gs.size() == 2);
    }

    // Test 6: Chain resolution - p resolved with "p :- q", then q resolved with fact
    // → total reward accumulates to 1.0 across two resolve calls
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}}); // rule 0: p :- q.
        db.push_back(rule{ep.functor("q", {}), {}});              // rule 1: q :-
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl0, 0); // resolve p with rule 0

        sim.resolve(rl_p);
        assert(near(sim.reward(), 0.0));    // weight transferred to q sub-goal
        assert(sim.gs.size() == 1);

        // Now resolve q (the sub-goal) with rule 1 (fact)
        const goal_lineage* sub_gl = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(sub_gl, 1); // resolve q with rule 1
        sim.resolve(rl_q);

        assert(near(sim.reward(), 1.0)); // full weight accumulated
        assert(near(sim.ws.cgw, 1.0));
        assert(sim.gs.empty());
    }

    // Test 7: Branch p :- q, r; only q grounded → partial reward 0.5, r still pending
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {}), ep.functor("r", {})}}); // rule 0: p :- q, r.
        db.push_back(rule{ep.functor("q", {}), {}});                            // rule 1: q :-
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl0, 0);
        sim.resolve(rl_p); // p :- q, r → q=0.5, r=0.5

        assert(near(sim.reward(), 0.0));
        const goal_lineage* sub_q = lp.goal(rl_p, 0);
        const goal_lineage* sub_r = lp.goal(rl_p, 1);
        assert(near(sim.ws.members.at(sub_q), 0.5));
        assert(near(sim.ws.members.at(sub_r), 0.5));

        // Ground q → cgw += 0.5; r still pending
        const resolution_lineage* rl_q = lp.resolution(sub_q, 1);
        sim.resolve(rl_q);

        assert(near(sim.ws.cgw, 0.5));
        assert(near(sim.reward(), 0.5));
        // r still in ws, not yet grounded
        assert(near(sim.ws.members.at(sub_r), 0.5));
        assert(sim.gs.size() == 1); // only r remains
    }

    // Test 8: Deep chain p → q → r → fact; reward stays 0 until the leaf is grounded
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {})}}); // rule 0: p :- q.
        db.push_back(rule{ep.functor("q", {}), {ep.functor("r", {})}}); // rule 1: q :- r.
        db.push_back(rule{ep.functor("r", {}), {}});              // rule 2: r :-
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl0, 0);
        sim.resolve(rl_p); // p → q, weight 1.0 passes to q
        assert(near(sim.reward(), 0.0));
        assert(sim.gs.size() == 1);

        const goal_lineage* sub_q = lp.goal(rl_p, 0);
        assert(near(sim.ws.members.at(sub_q), 1.0));
        const resolution_lineage* rl_q = lp.resolution(sub_q, 1);
        sim.resolve(rl_q); // q → r, weight 1.0 passes to r
        assert(near(sim.reward(), 0.0));
        assert(sim.gs.size() == 1);

        const goal_lineage* sub_r = lp.goal(rl_q, 0);
        assert(near(sim.ws.members.at(sub_r), 1.0));
        const resolution_lineage* rl_r = lp.resolution(sub_r, 2);
        sim.resolve(rl_r); // r :- (fact) → cgw += 1.0
        assert(near(sim.reward(), 1.0));
        assert(sim.gs.empty());
    }

    // Test 9: 3 root goals (each 1/3); resolve one into two branches, ground only one branch
    // → only 1/6 of total weight accumulated
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("x", {}), ep.functor("y", {})}}); // rule 0: a :- x, y.
        db.push_back(rule{ep.functor("x", {}), {}});                            // rule 1: x :-
        goals goals;
        goals.push_back(ep.functor("a", {})); // weight = 1/3
        goals.push_back(ep.functor("b", {})); // weight = 1/3
        goals.push_back(ep.functor("c", {})); // weight = 1/3
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        sim.resolve(rl_a); // a :- x, y → x=1/6, y=1/6
        assert(near(sim.reward(), 0.0));

        const goal_lineage* sub_x = lp.goal(rl_a, 0);
        const goal_lineage* sub_y = lp.goal(rl_a, 1);
        assert(near(sim.ws.members.at(sub_x), 1.0 / 6.0));
        assert(near(sim.ws.members.at(sub_y), 1.0 / 6.0));

        const resolution_lineage* rl_x = lp.resolution(sub_x, 1);
        sim.resolve(rl_x); // ground x → cgw += 1/6
        assert(near(sim.reward(), 1.0 / 6.0));
        // y, b, c still pending; total unresolved = 1/6 + 1/3 + 1/3 = 5/6
        assert(sim.gs.size() == 3); // y, b, c
    }

    // Test 10: 3-body rule; ground all three sub-goals stepwise → reward accumulates in thirds
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {}), ep.functor("r", {}), ep.functor("s", {})}}); // rule 0: p :- q, r, s.
        db.push_back(rule{ep.functor("q", {}), {}}); // rule 1: q :-
        db.push_back(rule{ep.functor("r", {}), {}}); // rule 2: r :-
        db.push_back(rule{ep.functor("s", {}), {}}); // rule 3: s :-
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl0, 0);
        sim.resolve(rl_p); // p :- q, r, s → each gets 1/3

        const goal_lineage* sub_q = lp.goal(rl_p, 0);
        const goal_lineage* sub_r = lp.goal(rl_p, 1);
        const goal_lineage* sub_s = lp.goal(rl_p, 2);
        assert(near(sim.ws.members.at(sub_q), 1.0 / 3.0));
        assert(near(sim.ws.members.at(sub_r), 1.0 / 3.0));
        assert(near(sim.ws.members.at(sub_s), 1.0 / 3.0));
        assert(near(sim.reward(), 0.0));

        sim.resolve(lp.resolution(sub_q, 1)); // ground q
        assert(near(sim.reward(), 1.0 / 3.0));

        sim.resolve(lp.resolution(sub_r, 2)); // ground r
        assert(near(sim.reward(), 2.0 / 3.0));

        sim.resolve(lp.resolution(sub_s, 3)); // ground s
        assert(near(sim.reward(), 1.0));
        assert(sim.gs.empty());
    }

    // Test 11: Two root goals with different resolution depths; interleaved grounding
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});              // rule 0: a :- c.
        db.push_back(rule{ep.functor("c", {}), {}});                           // rule 1: c :-
        db.push_back(rule{ep.functor("b", {}), {ep.functor("d", {}), ep.functor("e", {})}}); // rule 2: b :- d, e.
        db.push_back(rule{ep.functor("d", {}), {}});                           // rule 3: d :-
        db.push_back(rule{ep.functor("e", {}), {}});                           // rule 4: e :-
        goals goals;
        goals.push_back(ep.functor("a", {})); // gl0, weight = 0.5
        goals.push_back(ep.functor("b", {})); // gl1, weight = 0.5
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const goal_lineage* gl1 = lp.goal(nullptr, 1);

        // a :- c → c gets 0.5
        const resolution_lineage* rl_a = lp.resolution(gl0, 0);
        sim.resolve(rl_a);
        assert(near(sim.reward(), 0.0));
        const goal_lineage* sub_c = lp.goal(rl_a, 0);
        assert(near(sim.ws.members.at(sub_c), 0.5));

        // Ground c → cgw = 0.5
        sim.resolve(lp.resolution(sub_c, 1));
        assert(near(sim.reward(), 0.5));

        // b :- d, e → d=0.25, e=0.25
        const resolution_lineage* rl_b = lp.resolution(gl1, 2);
        sim.resolve(rl_b);
        assert(near(sim.reward(), 0.5)); // unchanged until a leaf is grounded
        const goal_lineage* sub_d = lp.goal(rl_b, 0);
        const goal_lineage* sub_e = lp.goal(rl_b, 1);
        assert(near(sim.ws.members.at(sub_d), 0.25));
        assert(near(sim.ws.members.at(sub_e), 0.25));

        // Ground d
        sim.resolve(lp.resolution(sub_d, 3));
        assert(near(sim.reward(), 0.75));

        // Ground e
        sim.resolve(lp.resolution(sub_e, 4));
        assert(near(sim.reward(), 1.0));
        assert(sim.gs.empty());
    }

    // Test 12: Deep binary branching; only one grandchild grounded → reward == 0.25
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        database db;
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {}), ep.functor("r", {})}}); // rule 0: p :- q, r.
        db.push_back(rule{ep.functor("q", {}), {ep.functor("s", {}), ep.functor("t", {})}}); // rule 1: q :- s, t.
        db.push_back(rule{ep.functor("s", {}), {}});                            // rule 2: s :-
        goals goals;
        goals.push_back(ep.functor("p", {})); // weight = 1.0
        cdcl c;
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> mc(root, 1.414, rng);
        horizon_sim sim(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{mc});

        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl0, 0);
        sim.resolve(rl_p); // p :- q, r → q=0.5, r=0.5
        assert(near(sim.reward(), 0.0));

        const goal_lineage* sub_q = lp.goal(rl_p, 0);
        const goal_lineage* sub_r = lp.goal(rl_p, 1);

        const resolution_lineage* rl_q = lp.resolution(sub_q, 1);
        sim.resolve(rl_q); // q :- s, t → s=0.25, t=0.25
        assert(near(sim.reward(), 0.0));

        const goal_lineage* sub_s = lp.goal(rl_q, 0);
        const goal_lineage* sub_t = lp.goal(rl_q, 1);
        assert(near(sim.ws.members.at(sub_s), 0.25));
        assert(near(sim.ws.members.at(sub_t), 0.25));
        assert(near(sim.ws.members.at(sub_r), 0.5));

        // Ground only s → cgw = 0.25
        sim.resolve(lp.resolution(sub_s, 2));
        assert(near(sim.reward(), 0.25));
        // r=0.5 and t=0.25 still unresolved, total pending = 0.75
        assert(sim.gs.size() == 2); // r and t remain
    }
}

void test_horizon() {

    // Test 1: Budget = 0 — no iterations execute; operator() returns true with nullopt.
    // No work is done so refutation cannot be proved and no solution can be found.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("a", {}), {}});

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        soln = std::nullopt;
        bool result = true; // (0 iterations: no sim run)

        // CRITICAL: returns true (no refutation proved) and soln defaults to nullopt
        assert(result == true);
        assert(!soln.has_value());
        // No iterations ran, so no avoidances were recorded
        assert(solver.c.avoidances.empty());
    }

    // Test 2: Simple ground solution, then refuted on the next call.
    // db: {a.}, goals: {a}
    // The single goal is unit-propagated immediately. ds is empty, so c.learn({})
    // marks c.refuted() = true. A second call hits the refutation guard and returns false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("a", {}), {}});  // idx 0: a.

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        bool result = solver(soln);

        assert(result == true);
        assert(soln.has_value());

        // CRITICAL: exactly one resolution (unit-prop of a with rule 0)
        assert(soln.value().size() == 1);
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        assert(soln.value().count(rl0) == 1);

        // CRITICAL: empty-decision solution → c.learn({}) → c.refuted() = true
        assert(solver.c.avoidances.size() == 1);
        assert(solver.c.is_refuted);

        // CRITICAL: second call returns false immediately (refutation guard)
        bool result2 = solver(soln);
        assert(result2 == false);
        assert(!soln.has_value());
    }

    // Test 3: Variable binding verified via normalizer.
    // db: {answer(42).}, goals: {answer(X)}
    // The copier freshens the rule head; bm.unify binds X → atom("42").
    // After operator() returns, normalizer resolves X.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("answer", {}), ep.functor("42", {})}), {}});  // idx 0

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("answer", {}), X}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        bool result = solver(soln);

        assert(result == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        // CRITICAL: normalizer follows bm chain and returns atom("42")
        normalizer norm(ep, bm);
        const expr* X_val = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val->content));
        assert(std::get<expr::functor>(X_val->content).name == "42");
    }

    // Test 4: Immediate refutation — empty database, goal has no candidates.
    // Head-elimination fires before any resolution: conflict with ds = {} on the
    // very first sim_one → ds.empty() branch → operator() returns false immediately.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;  // intentionally empty

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        bool result;
        while ((result = solver(soln)) && !soln.has_value()) {}

        assert(result == false);
        assert(!soln.has_value());
    }

    // Test 5: Two-call CDCL-driven refutation.
    // db: {a :- b., a :- c.}  (no rules for b or c), goals: {a}
    //
    // Call 1 (iterations=1): MCTS picks one of {rule0, rule1} for goal a.
    //   The resulting sub-goal (b or c) has no candidates → conflict, ds has 1 decision.
    //   Avoidance learned; returns true, nullopt.
    //
    // Call 2 (iterations=1): CDCL eliminates the chosen rule; the other is unit-propagated.
    //   That sub-goal (b or c) also has no candidates → conflict, ds = {} → return false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0: a :- b.
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1: a :- c.

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;

        // Call 1: one avoidance recorded (singleton decision)
        bool result1 = solver(soln);
        assert(result1 == true);
        assert(!soln.has_value());
        assert(solver.c.avoidances.size() == 1);

        // Call 2: CDCL + unit-prop leads to conflict with empty ds → refutation
        bool result2 = solver(soln);
        assert(result2 == false);
        assert(!soln.has_value());
    }

    // Test 6: Unique-solution conjunction — is_a(X) ∧ is_b(X).
    // db: {is_a(1), is_a(2), is_b(2), is_b(3)}, goals: {is_a(X), is_b(X)}
    //
    // The only consistent binding is X=2.
    //
    // Call 1: MCTS finds the solution in up to 1000 iterations.
    //   Normalizer verifies X → "2". soln has exactly 2 resolutions.
    //
    // Call 2: CDCL blocks the X=2 path; remaining paths (X=1 or X=3) conflict
    //   immediately with the other goal → ds = {} → return false (refutation).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("is_a", {}), ep.functor("1", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("is_a", {}), ep.functor("2", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("is_b", {}), ep.functor("2", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("is_b", {}), ep.functor("3", {})}), {}});  // idx 3

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("is_a", {}), X}));  // goal 0: is_a(X)
        goals.push_back(ep.functor("cons", {ep.functor("is_b", {}), X}));  // goal 1: is_b(X)

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        // Call 1: unique solution X=2
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());

        // CRITICAL: bm binds X to "2"
        const expr* X_val = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val->content));
        assert(std::get<expr::functor>(X_val->content).name == "2");

        // CRITICAL: soln contains exactly 2 resolutions (one per goal)
        assert(soln.value().size() == 2);
        const goal_lineage* gl_a = solver.lp.goal(nullptr, 0);
        const goal_lineage* gl_b = solver.lp.goal(nullptr, 1);
        const resolution_lineage* rl_a1 = solver.lp.resolution(gl_a, 1);  // is_a(2)
        const resolution_lineage* rl_b2 = solver.lp.resolution(gl_b, 2);  // is_b(2)
        assert(soln.value().count(rl_a1) == 1);
        assert(soln.value().count(rl_b2) == 1);

        // Call 2: X=2 path blocked; all remaining paths conflict → refutation
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == false);
        assert(!soln.has_value());
    }

    // Test 7: Multi-solution enumeration — all parents of alice.
    // db: {parent(bob,alice), parent(carol,alice), parent(dave,bob)}, goals: {parent(X,alice)}
    //
    // Head-elimination removes dave's rule (child = bob ≠ alice). MCTS decides
    // between bob and carol. Each is a depth-1 solution.
    //
    // Call 1 finds one parent; call 2 (CDCL eliminates call-1 decision, other rule
    // becomes unit-prop) finds the other; call 3 proves refutation.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("bob", {})}), ep.functor("alice", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("carol", {})}), ep.functor("alice", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("dave", {})}), ep.functor("bob", {})}),  {}});   // idx 2

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), X}), ep.functor("alice", {})}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        // First parent of alice
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        const expr* X_val1 = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val1->content));
        std::string parent1 = std::get<expr::functor>(X_val1->content).name;
        assert(parent1 == "bob" || parent1 == "carol");

        // Second parent of alice — CDCL blocks the first decision; other rule is unit-prop'd
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        const expr* X_val2 = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val2->content));
        std::string parent2 = std::get<expr::functor>(X_val2->content).name;
        assert(parent2 == "bob" || parent2 == "carol");

        // CRITICAL: the two solutions bind X to different names
        assert(parent1 != parent2);

        // Third call: both paths have been learned; refutation proved
        bool result3;
        while ((result3 = solver(soln)) && !soln.has_value()) {}
        assert(result3 == false);
    }

    // Test 8: Graph 2-coloring — find valid 2-colorings of a 3-node path A-B-C.
    // db: {color(red), color(blue), diff(red,blue), diff(blue,red)}
    // goals: {color(A), color(B), color(C), diff(A,B), diff(B,C)}
    //
    // A single MCTS decision (e.g. on diff(A,B)) binds A and B and unit-propagates
    // the rest, producing one of two valid alternating colorings.
    //
    // Both calls return colorings satisfying A≠B, B≠C, A=C. The two solutions
    // assign opposite colors to A. Call 3 proves refutation.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("red", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("blue", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("blue", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("red", {})}),  {}});  // idx 3

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), A}));             // goal 0: color(A)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), B}));             // goal 1: color(B)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), C}));             // goal 2: color(C)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), B}));  // goal 3: diff(A, B)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), B}), C}));  // goal 4: diff(B, C)

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        auto is_valid_color = [](const std::string& s) {
            return s == "red" || s == "blue";
        };

        // First valid 2-coloring of the path A-B-C
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 5);

        std::string A1 = std::get<expr::functor>(norm(A)->content).name;
        std::string B1 = std::get<expr::functor>(norm(B)->content).name;
        std::string C1 = std::get<expr::functor>(norm(C)->content).name;

        assert(is_valid_color(A1) && is_valid_color(B1) && is_valid_color(C1));
        // CRITICAL: adjacent nodes have different colors
        assert(A1 != B1);
        assert(B1 != C1);
        // CRITICAL: endpoints share a color on a 2-colored 3-node path
        assert(A1 == C1);

        // Second valid 2-coloring — opposite assignment
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 5);

        std::string A2 = std::get<expr::functor>(norm(A)->content).name;
        std::string B2 = std::get<expr::functor>(norm(B)->content).name;
        std::string C2 = std::get<expr::functor>(norm(C)->content).name;

        assert(is_valid_color(A2) && is_valid_color(B2) && is_valid_color(C2));
        assert(A2 != B2);
        assert(B2 != C2);
        assert(A2 == C2);

        // CRITICAL: the two colorings assign opposite colors to A
        assert(A1 != A2);

        // Third call: all colorings enumerated, refutation proved
        bool result3;
        while ((result3 = solver(soln)) && !soln.has_value()) {}
        assert(result3 == false);
    }

    // Test 9: Multi-body rule with variables in rule head/body and in the goal.
    //
    // DB:
    //   idx 0: parent(alice, carol).
    //   idx 1: parent(bob,   carol).
    //   idx 2: parent(carol, dave).
    //   idx 3: grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
    //
    // Goal: grandparent(G, dave)  — G is a logic variable.
    //
    // Resolution trace per solution (3 steps each):
    //   1. grandparent(G, dave) via idx 3 → fresh Y'; adds parent(G, Y') and parent(Y', dave).
    //   2. parent(Y', dave)  → unit-prop via idx 2 → Y' = carol.
    //   3. parent(G, carol)  → MCTS picks idx 0 (G=alice) or idx 1 (G=bob).
    //
    // Both solutions are found before refutation is proved on the third call.
    // Each soln contains exactly 3 resolution_lineage pointers.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        // Facts: parent(alice,carol), parent(bob,carol), parent(carol,dave)
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("alice", {})}), ep.functor("carol", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("bob", {})}), ep.functor("carol", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("carol", {})}), ep.functor("dave", {})}),  {}});  // idx 2

        // grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("grandparent", {}), X}), Z}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), X}), Y}),
                 ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), Y}), Z})}
            });  // idx 3
        }

        const expr* G = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("grandparent", {}), G}), ep.functor("dave", {})}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        // Call 1: first grandparent of dave
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 3);

        const expr* G_val1 = norm(G);
        assert(std::holds_alternative<expr::functor>(G_val1->content));
        std::string gp1 = std::get<expr::functor>(G_val1->content).name;
        assert(gp1 == "alice" || gp1 == "bob");

        // Call 2: second grandparent of dave — must differ from the first
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 3);

        const expr* G_val2 = norm(G);
        assert(std::holds_alternative<expr::functor>(G_val2->content));
        std::string gp2 = std::get<expr::functor>(G_val2->content).name;
        assert(gp2 == "alice" || gp2 == "bob");

        // CRITICAL: the two solutions bind G to different names
        assert(gp1 != gp2);

        // Call 3: both paths exhausted → refutation
        bool result3;
        while ((result3 = solver(soln)) && !soln.has_value()) {}
        assert(result3 == false);
        assert(!soln.has_value());
    }

    // ---------------------------------------------------------------------------
    // Multi-solution enumeration tests using next_until_refuted
    //
    // The lambda enumerates all expected solutions in any order (deduplicating paths
    // that produce the same variable bindings), then asserts that the next call
    // proves refutation.
    // ---------------------------------------------------------------------------

    using solution = std::vector<const expr*>;

    auto next_until_refuted = [](
        horizon& solver,
        std::set<solution> expected,
        auto get_solution
    ) {
        const std::map<uint32_t, std::string> no_names;
        std::set<solution> visited;
        std::optional<resolution_store> soln;
        while (!expected.empty()) {
            solution s;
            do {
                bool r; while ((r = solver(soln)) && !soln.has_value()) {}
                assert(r == true);
                s = get_solution();
            } while (visited.count(s));
            std::cout << "Solution: " << visited.size() << " resolutions: " << soln.value().size() << std::endl;
            expr_printer printer(std::cout, no_names);
            for (const auto& e : s) {
                printer(e);
                std::cout << std::endl;
            }
            std::cout << std::endl;
            assert(expected.count(s) == 1);
            expected.erase(s);
            visited.insert(s);
        }
        // All solutions found — next call must refute
        bool r; while ((r = solver(soln)) && !soln.has_value()) {}
        assert(r == false);
        assert(!soln.has_value());
    };

    // Like next_until_refuted but without the final refutation assertion.
    // Used for problems where the full search space is too large to exhaust,
    // or where horizon is exploring a semi-decidable region where refutation
    // cannot be guaranteed in finite time.
    auto enumerate_all_solutions = [](
        horizon& solver,
        std::set<solution> expected,
        auto get_solution
    ) {
        const std::map<uint32_t, std::string> no_names;
        std::set<solution> visited;
        std::optional<resolution_store> soln;
        while (!expected.empty()) {
            solution s;
            do {
                bool r; while ((r = solver(soln)) && !soln.has_value()) {}
                assert(r == true);
                s = get_solution();
            } while (visited.count(s));
            std::cout << "Solution: " << visited.size()
                      << " resolutions: " << soln.value().size() << std::endl;
            expr_printer printer(std::cout, no_names);
            for (const auto& e : s) {
                printer(e);
                std::cout << std::endl;
            }
            std::cout << std::endl;
            assert(expected.count(s) == 1);
            expected.erase(s);
            visited.insert(s);
        }
    };

    // Test 10: 3-colouring of K3 (the triangle) with colours {red, green, blue}.
    //
    // All 3 nodes A, B, C are mutually adjacent, so every valid colouring assigns
    // a distinct colour to each node.  Exactly 3! = 6 proper 3-colourings exist.
    //
    // DB:
    //   idx 0-2: color(red), color(green), color(blue).
    //   idx 3-8: diff(X,Y) for every ordered pair of distinct colours (6 facts).
    //
    // Goals: color(A), color(B), color(C), diff(A,B), diff(A,C), diff(B,C).
    //
    // An MCTS decision (e.g. diff(A,B)→red-green) binds two nodes and unit-propagates
    // the remaining diff goals to the unique third colour.  Multiple resolution orderings
    // reaching the same binding are deduplicated by the visited loop.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("red", {})}),   {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("green", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("blue", {})}),  {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("green", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("blue", {})}),  {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("red", {})}),   {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("blue", {})}),  {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("red", {})}),   {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("green", {})}), {}});  // idx 8

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), A}));
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), B}));
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), C}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), B}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), C}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), B}), C}));

        const expr* red   = ep.functor("red", {});
        const expr* green = ep.functor("green", {});
        const expr* blue  = ep.functor("blue", {});

        std::set<solution> expected = {
            {red,   green, blue },
            {red,   blue,  green},
            {green, red,   blue },
            {green, blue,  red  },
            {blue,  red,   green},
            {blue,  green, red  },
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(A)), ep.import(norm(B)), ep.import(norm(C))};
        });
    }

    // Test 11: SAT — P ∧ (Q ∨ R) using relational OR/AND encoding.
    //
    // The OR and AND predicates are encoded relationally with a bool/1 body goal:
    //   or(true,  X, true) :- bool(X).   — true  ∨ anything = true
    //   or(false, X, X)    :- bool(X).   — false ∨ X = X
    //   and(true,  X, X)   :- bool(X).   — true  ∧ X = X
    //   and(false, X, false):- bool(X).  — false ∧ anything = false
    //
    // Formula: P ∧ (Q ∨ R).
    // Goals: bool(P), bool(Q), bool(R), or(Q,R,QR), and(P,QR,true).
    //
    // and(P,QR,true) unit-props P=true, QR=true.
    // or(Q,R,true) then has 3 satisfying assignments.
    // Solutions: (P=T,Q=T,R=T), (P=T,Q=T,R=F), (P=T,Q=F,R=T).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("true", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("false", {})}), {}});  // idx 1

        // or(true, X, true) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("true", {})}), X}), ep.functor("true", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 2
        }
        // or(false, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("false", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 3
        }
        // and(true, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("true", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 4
        }
        // and(false, X, false) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("false", {})}), X}), ep.functor("false", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 5
        }

        const expr* P  = ep.var(seq());
        const expr* Q  = ep.var(seq());
        const expr* R  = ep.var(seq());
        const expr* QR = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), P}));
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), Q}));
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), R}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), Q}), R}), QR}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), P}), QR}), ep.functor("true", {})}));

        const expr* T_ = ep.functor("true", {});
        const expr* F_ = ep.functor("false", {});

        std::set<solution> expected = {
            {T_, T_, T_},
            {T_, T_, F_},
            {T_, F_, T_},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(P)), ep.import(norm(Q)), ep.import(norm(R))};
        });
    }

    // Test 12: Peano arithmetic — enumerate all naturals less than 5.
    //
    // Rules:
    //   idx 0: nat(zero).
    //   idx 1: nat(suc(X))      :- nat(X).
    //   idx 2: lt(zero, suc(X)) :- nat(X).
    //   idx 3: lt(suc(X), suc(Y)) :- lt(X, Y).
    //
    // Goal: lt(N, five)   where five = suc^5(zero).
    // Exactly 5 solutions: N ∈ {0, 1, 2, 3, 4} in Peano encoding.
    // Reaching N=k requires k unfoldings of rule 3, then rule 2, then k of rule 1.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});  // idx 0

        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });  // idx 1
        }
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("zero", {})}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });  // idx 2
        }
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), ep.functor("cons", {ep.functor("suc", {}), Y})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), X}), Y})}
            });  // idx 3
        }

        const expr* N    = ep.var(seq());
        const expr* five = peano(5);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), N}), five}));

        std::set<solution> expected = {
            {peano(0)}, {peano(1)}, {peano(2)}, {peano(3)}, {peano(4)},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(N))};
        });
    }

    // Test 13: Sibling query — multi-body rule with shared existential variable.
    //
    // Rule: sibling(X, Y) :- parent(P, X), parent(P, Y).
    //   The intermediate variable P is existential: it does not appear in the head
    //   or the query goal, only in the two body atoms.  Both body goals must be
    //   resolved to the same parent, so P is unified across the two sub-goals.
    //
    // DB:
    //   idx 0: parent(tom,   alice).
    //   idx 1: parent(tom,   bob).
    //   idx 2: parent(sue,   carol).
    //   idx 3: sibling(X, Y) :- parent(P, X), parent(P, Y).
    //
    // Goal: sibling(X, alice).
    //
    // Resolutions:
    //   sibling(X, alice) via idx 3 → parent(P', X) and parent(P', alice).
    //   parent(P', alice) unit-props → P' = tom.
    //   parent(tom, X) → X = alice (idx 0) or X = bob (idx 1).
    //   carol is pruned because parent(sue, alice) has no match.
    //
    // Solutions: X = alice (self-sibling, since the rule carries no ≠ constraint),
    //            X = bob.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("tom", {})}), ep.functor("alice", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("tom", {})}), ep.functor("bob", {})}),   {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("sue", {})}), ep.functor("carol", {})}), {}});  // idx 2

        // sibling(X, Y) :- parent(P, X), parent(P, Y).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Pv = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("sibling", {}), X}), Y}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), Pv}), X}),
                 ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), Pv}), Y})}
            });  // idx 3
        }

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("sibling", {}), X}), ep.functor("alice", {})}));

        const expr* alice = ep.functor("alice", {});
        const expr* bob   = ep.functor("bob", {});

        std::set<solution> expected = {
            {alice},
            {bob},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X))};
        });
    }

    // Test 14: 3-colouring of "K3 + tail" — 4 nodes, 8 goals, 12 solutions.
    //
    // Graph: nodes A, B, C, D.
    //   Edges: A-B, A-C, B-C  (triangle — A, B, C all-different)
    //          A-D             (tail — D only constrained to differ from A)
    //
    // Colours: red, green, blue.
    //
    // Each of the 6 K3 colourings of (A,B,C) combines with 2 choices for D
    // (any colour ≠ A), giving 6 × 2 = 12 distinct solutions.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("red", {})}),   {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("green", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("blue", {})}),  {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("green", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("blue", {})}),  {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("red", {})}),   {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("blue", {})}),  {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("red", {})}),   {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("green", {})}), {}});  // idx 8

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* D = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), A}));
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), B}));
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), C}));
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), D}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), B}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), C}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), B}), C}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), D}));

        const expr* R_ = ep.functor("red", {});
        const expr* G_ = ep.functor("green", {});
        const expr* B_ = ep.functor("blue", {});

        std::set<solution> expected = {
            {R_, G_, B_, G_}, {R_, G_, B_, B_},
            {R_, B_, G_, G_}, {R_, B_, G_, B_},
            {G_, R_, B_, R_}, {G_, R_, B_, B_},
            {G_, B_, R_, R_}, {G_, B_, R_, B_},
            {B_, R_, G_, R_}, {B_, R_, G_, G_},
            {B_, G_, R_, R_}, {B_, G_, R_, G_},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(A)), ep.import(norm(B)), ep.import(norm(C)), ep.import(norm(D))};
        });
    }

    // Test 15: 4-variable SAT — (P ∨ Q) ∧ (R ∨ S) ∧ (¬P ∨ ¬R).
    //
    // Three clauses, four boolean variables.  Clause three forbids P and R
    // both being true, while the first two require each pair to cover true.
    //
    // Uses the same relational OR/AND/NOT encoding as Test 11, plus NOT rules.
    // Propagation chain:
    //   and(PQ_RS, NPR, true) → PQ_RS=true, NPR=true.
    //   and(PQ, RS, true)     → PQ=true, RS=true.
    //   Remaining: or(P,Q,true), or(R,S,true), or(NP,NR,true) each need a decision.
    //
    // Satisfying assignments (5):
    //   (T,T,F,T), (T,F,F,T), (F,T,T,T), (F,T,T,F), (F,T,F,T).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("true", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("false", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), ep.functor("true", {})}), ep.functor("false", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), ep.functor("false", {})}), ep.functor("true", {})}),  {}});  // idx 3

        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("true", {})}), X}), ep.functor("true", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 4
        }
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("false", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 5
        }
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("true", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 6
        }
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("false", {})}), X}), ep.functor("false", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 7
        }

        const expr* P     = ep.var(seq());
        const expr* Q     = ep.var(seq());
        const expr* R     = ep.var(seq());
        const expr* S     = ep.var(seq());
        const expr* PQ    = ep.var(seq());
        const expr* RS    = ep.var(seq());
        const expr* NP    = ep.var(seq());
        const expr* NR    = ep.var(seq());
        const expr* NPR   = ep.var(seq());
        const expr* PQ_RS = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), P}));
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), Q}));
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), R}));
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), S}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), P}), Q}), PQ}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), R}), S}), RS}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), P}), NP}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), R}), NR}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), NP}), NR}), NPR}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), PQ}), RS}), PQ_RS}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), PQ_RS}), NPR}), ep.functor("true", {})}));

        const expr* T_ = ep.functor("true", {});
        const expr* F_ = ep.functor("false", {});

        std::set<solution> expected = {
            {T_, T_, F_, T_},
            {T_, F_, F_, T_},
            {F_, T_, T_, T_},
            {F_, T_, T_, F_},
            {F_, T_, F_, T_},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(P)), ep.import(norm(Q)), ep.import(norm(R)), ep.import(norm(S))};
        });
    }

    // Test 16: Unsatisfiable problem — 0 solutions, immediate refutation.
    //
    // DB:
    //   idx 0: q(a).
    //   idx 1: p(X) :- q(X), r(X).   (no rules for r/1 at all)
    //
    // Goal: p(a).
    //
    // MCTS resolves p(a) via idx 1, introducing sub-goals q(a) and r(a).
    // q(a) unit-props via idx 0.  r(a) has no candidates → conflict.
    // The only decision (choosing idx 1) is learned.  On the next call p(a)
    // has no remaining candidates → ds = {} → immediate refutation.
    //
    // next_until_refuted with 0 expected solutions therefore asserts the very
    // first call returns false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 0: q(a).

        // p(X) :- q(X), r(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("p", {}), X}),
                {ep.functor("cons", {ep.functor("q", {}), X}),
                 ep.functor("cons", {ep.functor("r", {}), X})}
            });  // idx 1
        }

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        next_until_refuted(solver, {}, [&]() -> solution { return {}; });
    }

    // Test 17: Cartesian product — type(X) × type(Y) with 3 ground values.
    //
    // DB: type(a). type(b). type(c).
    // Goals: type(X), type(Y)   — X and Y are independent variables.
    //
    // X and Y have no shared goals, so each can be resolved independently.
    // Every combination is a valid solution; 3 × 3 = 9 solutions total.
    //
    // This tests that horizon correctly enumerates two-variable Cartesian
    // products where MCTS must make separate decisions for each variable.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("type", {}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("type", {}), ep.functor("b", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("type", {}), ep.functor("c", {})}), {}});  // idx 2

        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("type", {}), X}));
        goals.push_back(ep.functor("cons", {ep.functor("type", {}), Y}));

        const expr* a = ep.functor("a", {});
        const expr* b = ep.functor("b", {});
        const expr* c = ep.functor("c", {});

        std::set<solution> expected = {
            {a, a}, {a, b}, {a, c},
            {b, a}, {b, b}, {b, c},
            {c, a}, {c, b}, {c, c},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X)), ep.import(norm(Y))};
        });
    }

    // Test 18: Relational join / intersection — fruit(X) ∧ sweet(X).
    //
    // DB:
    //   idx 0: fruit(apple).
    //   idx 1: fruit(banana).
    //   idx 2: fruit(cherry).
    //   idx 3: sweet(banana).
    //   idx 4: sweet(cherry).
    //
    // Goals: fruit(X), sweet(X)   — X shared between both goals.
    //
    // Head-elimination prunes fruit(apple) because sweet(apple) has no matching
    // rule, leaving X ∈ {banana, cherry} as the only two solutions.
    //
    // This tests that unit propagation and head-elimination together narrow the
    // search space correctly when a single variable appears in multiple goals.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("fruit", {}), ep.functor("apple", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("fruit", {}), ep.functor("banana", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("fruit", {}), ep.functor("cherry", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("sweet", {}), ep.functor("banana", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("sweet", {}), ep.functor("cherry", {})}), {}});  // idx 4

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("fruit", {}), X}));
        goals.push_back(ep.functor("cons", {ep.functor("sweet", {}), X}));

        const expr* banana = ep.functor("banana", {});
        const expr* cherry = ep.functor("cherry", {});

        std::set<solution> expected = {
            {banana},
            {cherry},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X))};
        });
    }

    // Test 19: Peano addition — enumerate all pairs (X, Y) whose sum is 5.
    //
    // Rules:
    //   idx 0: nat(zero).
    //   idx 1: nat(suc(X))            :- nat(X).
    //   idx 2: add(zero, Y, Y)        :- nat(Y).
    //   idx 3: add(suc(X), Y, suc(Z)) :- add(X, Y, Z).
    //
    // Goal: add(X, Y, five)   where five = suc^5(zero).
    //
    // Exactly 6 solutions: (0,5),(1,4),(2,3),(3,2),(4,1),(5,0).
    // Each solution requires recursive unfolding of rule 3 (depth equal to X)
    // followed by rule 2 (base case), then nat(Y) resolution via rules 0/1.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});  // idx 0

        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });  // idx 1
        }
        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("zero", {})}), Y}), Y}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });  // idx 2
        }
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), ep.functor("cons", {ep.functor("suc", {}), Z})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), Z})}
            });  // idx 3
        }

        const expr* X    = ep.var(seq());
        const expr* Y    = ep.var(seq());
        const expr* five = peano(5);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), five}));

        std::set<solution> expected;
        for (int x = 0; x <= 5; ++x)
            expected.insert({peano(x), peano(5 - x)});
        assert(expected.size() == 6);

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X)), ep.import(norm(Y))};
        });
    }

    // Test 20: 3×3 Latin square — find all 12 valid grid completions.
    //
    // A 3×3 Latin square fills a 3×3 grid with values from {v1, v2, v3} such that
    // each value appears exactly once in every row and every column.
    //
    // This is the deepest horizon test: 27 conjoined goals (9 domain + 9 row-diff +
    // 9 col-diff) over 9 logic variables.  The MCTS weight-based reward provides a
    // continuous gradient — each grounded cell earns 1/27 of the total reward —
    // guiding exploration toward consistent partial assignments and away from dead ends
    // where a diff constraint has no matching rule.
    //
    // DB:
    //   idx 0-2: val(v1), val(v2), val(v3).
    //   idx 3-8: diff(vi, vj) for every ordered pair with i ≠ j  (6 facts).
    //
    // Variables: R11..R33 (row-major; Rij = cell at row i, column j).
    //
    // Goals (27 total):
    //   Domain    : val(R11), val(R12), ..., val(R33)
    //   Row diffs : diff(Ri1,Ri2), diff(Ri1,Ri3), diff(Ri2,Ri3)  for i ∈ {1,2,3}
    //   Col diffs : diff(R1j,R2j), diff(R1j,R3j), diff(R2j,R3j)  for j ∈ {1,2,3}
    //
    // Expected: exactly 12 solutions (the 12 distinct 3×3 Latin squares).
    // enumerate_all_solutions is used instead of next_until_refuted because we
    // only claim to find all solutions, not that the solver then proves refutation
    // (refuting the remaining 3^9 - 12 assignments could take longer).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        // Domain facts
        db.push_back(rule{ep.functor("cons", {ep.functor("val", {}), ep.functor("v1", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("val", {}), ep.functor("v2", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("val", {}), ep.functor("v3", {})}), {}});  // idx 2
        // Distinctness facts — all 6 ordered pairs of distinct values
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("v1", {})}), ep.functor("v2", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("v2", {})}), ep.functor("v1", {})}), {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("v1", {})}), ep.functor("v3", {})}), {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("v3", {})}), ep.functor("v1", {})}), {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("v2", {})}), ep.functor("v3", {})}), {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("v3", {})}), ep.functor("v2", {})}), {}});  // idx 8

        // 9 logic variables — one per grid cell
        const expr* R11 = ep.var(seq());
        const expr* R12 = ep.var(seq());
        const expr* R13 = ep.var(seq());
        const expr* R21 = ep.var(seq());
        const expr* R22 = ep.var(seq());
        const expr* R23 = ep.var(seq());
        const expr* R31 = ep.var(seq());
        const expr* R32 = ep.var(seq());
        const expr* R33 = ep.var(seq());

        goals goals;

        // 9 domain goals
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R11}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R12}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R13}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R21}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R22}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R23}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R31}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R32}));
        goals.push_back(ep.functor("cons", {ep.functor("val", {}), R33}));

        // 9 row-distinctness goals (3 pairs per row)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R11}), R12}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R11}), R13}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R12}), R13}));

        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R21}), R22}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R21}), R23}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R22}), R23}));

        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R31}), R32}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R31}), R33}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R32}), R33}));

        // 9 column-distinctness goals (3 pairs per column)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R11}), R21}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R11}), R31}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R21}), R31}));

        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R12}), R22}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R12}), R32}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R22}), R32}));

        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R13}), R23}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R13}), R33}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), R23}), R33}));

        // All 12 distinct 3×3 Latin squares over {v1, v2, v3}, listed row-major.
        // Each solution vector has 9 elements: {R11,R12,R13, R21,R22,R23, R31,R32,R33}.
        const expr* v1 = ep.functor("v1", {});
        const expr* v2 = ep.functor("v2", {});
        const expr* v3 = ep.functor("v3", {});

        std::set<solution> expected = {
            {v1,v2,v3, v2,v3,v1, v3,v1,v2},
            {v1,v2,v3, v3,v1,v2, v2,v3,v1},
            {v1,v3,v2, v2,v1,v3, v3,v2,v1},
            {v1,v3,v2, v3,v2,v1, v2,v1,v3},
            {v2,v1,v3, v1,v3,v2, v3,v2,v1},
            {v2,v1,v3, v3,v2,v1, v1,v3,v2},
            {v2,v3,v1, v1,v2,v3, v3,v1,v2},
            {v2,v3,v1, v3,v1,v2, v1,v2,v3},
            {v3,v1,v2, v1,v2,v3, v2,v3,v1},
            {v3,v1,v2, v2,v3,v1, v1,v2,v3},
            {v3,v2,v1, v1,v3,v2, v2,v1,v3},
            {v3,v2,v1, v2,v1,v3, v1,v3,v2},
        };
        assert(expected.size() == 12);

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        enumerate_all_solutions(solver, expected, [&]() -> solution {
            return {ep.import(norm(R11)), ep.import(norm(R12)), ep.import(norm(R13)),
                    ep.import(norm(R21)), ep.import(norm(R22)), ep.import(norm(R23)),
                    ep.import(norm(R31)), ep.import(norm(R32)), ep.import(norm(R33))};
        });
    }

    // Test 21: Constrained network routing — deep recursive path finding.
    //
    // This test exercises horizon's reward-guided advantage over ridge on a problem
    // where each resolution chain is deep and partial progress is meaningful.
    //
    // Predicate structure (3 chained rules):
    //   road/2          — raw directed edges (facts only)
    //   safe_node/1     — safety certificate, holds for all nodes except h (facts only)
    //   safe_edge(X,Y)  :- road(X,Y), safe_node(Y).
    //   safe_path(X,Y,suc(zero))    :- safe_edge(X,Y).                         — base
    //   safe_path(X,Z,suc(suc(N))) :- safe_edge(X,Y), safe_path(Y,Z,suc(N)).  — recursive
    //
    // One safe_path call triggers a 4-level chain:
    //   safe_path → safe_edge → road (fact) + safe_node (fact).
    //
    // Resolutions per solution:
    //   safe_path(s,M,two):   2 safe_path rules + 2 safe_edge rules + 4 facts = 8
    //   safe_path(M,t,three): 3 safe_path rules + 3 safe_edge rules + 6 facts = 12
    //   Total: ~20 resolutions per solution.
    //
    // Graph (layered DAG, 12 nodes, 18 road edges):
    //   s → a, b
    //   a → c, d       b → d, e
    //   c → f, g       d → g, h      e → g, h
    //   f → i          g → i, j      h → j   (h is UNSAFE)
    //   i → t          j → t
    //
    // Node h is the only unsafe node.  Any path through h fails at safe_node(h),
    // pruning 3 of the 10 possible length-5 paths from s to t.
    //
    // Conjoined initial goals (shared free variable M):
    //   Goal 0: safe_path(s, M, two)   — M is reachable from s in 2 safe hops
    //   Goal 1: safe_path(M, t, three) — t is reachable from M in 3 safe hops
    //
    // M must satisfy BOTH goals simultaneously.
    //
    // Expected solutions — 3 distinct midpoints:
    //   M=c: s→a→c (2 hops); c→{f,g}→{i,j}→t (3 hops, all safe) ✓
    //   M=d: s→{a,b}→d (2 hops); d→g→{i,j}→t (3 hops; d→h→j→t blocked) ✓
    //   M=e: s→b→e (2 hops); e→g→{i,j}→t (3 hops; e→h→j→t blocked) ✓
    //
    // enumerate_all_solutions is used: the solver need not refute the (large)
    // remaining search space, only confirm that all 3 witnesses are found.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        // Peano length constants
        const expr* zero  = ep.functor("zero", {});
        const expr* one   = ep.functor("cons", {ep.functor("suc", {}), zero});
        const expr* two   = ep.functor("cons", {ep.functor("suc", {}), one});
        const expr* three = ep.functor("cons", {ep.functor("suc", {}), two});

        database db;

        // --- road facts (18) ---
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("s", {})}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("s", {})}), ep.functor("b", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("a", {})}), ep.functor("c", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("a", {})}), ep.functor("d", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("b", {})}), ep.functor("d", {})}), {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("b", {})}), ep.functor("e", {})}), {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("c", {})}), ep.functor("f", {})}), {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("c", {})}), ep.functor("g", {})}), {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("d", {})}), ep.functor("g", {})}), {}});  // idx 8
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("d", {})}), ep.functor("h", {})}), {}});  // idx 9
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("e", {})}), ep.functor("g", {})}), {}});  // idx 10
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("e", {})}), ep.functor("h", {})}), {}});  // idx 11
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("f", {})}), ep.functor("i", {})}), {}});  // idx 12
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("g", {})}), ep.functor("i", {})}), {}});  // idx 13
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("g", {})}), ep.functor("j", {})}), {}});  // idx 14
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("h", {})}), ep.functor("j", {})}), {}});  // idx 15
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("i", {})}), ep.functor("t", {})}), {}});  // idx 16
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), ep.functor("j", {})}), ep.functor("t", {})}), {}});  // idx 17

        // --- safe_node facts (10) — all nodes except h ---
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("a", {})}), {}});  // idx 18
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("b", {})}), {}});  // idx 19
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("c", {})}), {}});  // idx 20
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("d", {})}), {}});  // idx 21
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("e", {})}), {}});  // idx 22
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("f", {})}), {}});  // idx 23
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("g", {})}), {}});  // idx 24
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("i", {})}), {}});  // idx 25
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("j", {})}), {}});  // idx 26
        db.push_back(rule{ep.functor("cons", {ep.functor("safe_node", {}), ep.functor("t", {})}), {}});  // idx 27
        // NOTE: safe_node(h) is deliberately absent — h is the unsafe node.

        // --- safe_edge(X, Y) :- road(X, Y), safe_node(Y). ---
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            db.push_back(rule{                                                // idx 28
                ep.functor("cons", {ep.functor("cons", {ep.functor("safe_edge", {}), X}), Y}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("road", {}), X}), Y}),
                 ep.functor("cons", {ep.functor("safe_node", {}), Y})}
            });
        }

        // --- safe_path(X, Y, suc(zero)) :- safe_edge(X, Y).   [base case] ---
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            db.push_back(rule{                                                // idx 29
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_path", {}), X}), Y}), one}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_edge", {}), X}), Y})}
            });
        }

        // --- safe_path(X, Z, suc(suc(N))) :- safe_edge(X, Y), safe_path(Y, Z, suc(N)).
        //     [recursive case: length ≥ 2, avoids ambiguity with base at length 1] ---
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            const expr* N = ep.var(seq());
            db.push_back(rule{                                                // idx 30
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_path", {}), X}), Z}), 
                        ep.functor("cons", {ep.functor("suc", {}), ep.functor("cons", {ep.functor("suc", {}), N})})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_edge", {}), X}), Y}),
                 ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_path", {}), Y}), Z}), 
                         ep.functor("cons", {ep.functor("suc", {}), N})})}
            });
        }

        // Shared free variable: M is the "safe midpoint" between the two goals.
        const expr* M = ep.var(seq());

        goals goals;
        // Goal 0: safe_path(s, M, two)   — M is 2 safe hops from s
        goals.push_back(
            ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_path", {}), ep.functor("s", {})}), M}), two}));
        // Goal 1: safe_path(M, t, three) — t is 3 safe hops from M
        goals.push_back(
            ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("safe_path", {}), M}), ep.functor("t", {})}), three}));

        // Expected solutions: M ∈ {c, d, e}
        const expr* c_node = ep.functor("c", {});
        const expr* d_node = ep.functor("d", {});
        const expr* e_node = ep.functor("e", {});

        std::set<solution> expected = {
            {c_node},
            {d_node},
            {e_node},
        };

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        enumerate_all_solutions(solver, expected, [&]() -> solution {
            return {ep.import(norm(M))};
        });
    }

    // Test 22: Mini Sudoku (4×4).
    //
    // A 4×4 Sudoku requires filling a grid from domain {d1,d2,d3,d4} such that
    // every row, column, and 2×2 box contains all four values exactly once.
    //
    // This showcases horizon's ability to navigate a deeply constrained search
    // space with 79 conjoined goals and 7 free variables.  The weight-based MCTS
    // reward gives partial credit for each grounded cell, guiding the solver
    // toward consistent assignments rather than treating every early conflict
    // the same way.
    //
    // Puzzle layout (d_i = ground atom, Vrc = free variable at row r, col c):
    //
    //   d1   d2   V13  d4
    //   V21  d4   d1   V24
    //   V31  d1   d4   V34
    //   d4   V42  V43  d1
    //
    // The extra pre-filled cell d2 at (1,2) breaks the symmetry that would
    // otherwise allow a second valid solution (d2↔d3 swap throughout).
    //
    // Unique solution:
    //   d1  d2  d3  d4
    //   d3  d4  d1  d2
    //   d2  d1  d4  d3
    //   d4  d3  d2  d1
    //
    // So: V13=d3, V21=d3, V24=d2, V31=d2, V34=d3, V42=d3, V43=d2.
    //
    // Derivation — all 7 free cells are logically forced:
    //   Row 1 [d1,d2,V13,d4]: V13 must be d3 (only remaining digit).
    //   Box 2 [V13,d4,d1,V24]: V13=d3 ⇒ V24=d2.
    //   Row 2 [V21,d4,d1,d2]: V21 must be d3.
    //   Col 1 [d1,d3,V31,d4]: V31 must be d2.
    //   Col 2 [d2,d4,d1,V42]: V42 must be d3.
    //   Row 4 [d4,d3,V43,d1]: V43 must be d2.
    //   Row 3 [d2,d1,d4,V34]: V34 must be d3.
    //
    // Goals (79 total):
    //   7  domain:  digit(Vrc)   for each variable cell
    //   24 row:     diff(cell[r][i], cell[r][j])  for i<j, all 4 rows
    //   24 col:     diff(cell[i][c], cell[j][c])  for i<j, all 4 cols
    //   24 box:     diff(box[i], box[j])           for i<j, all 4 2×2 boxes
    //
    // Of the 72 diff goals, 12 are ground-ground (trivially resolved in 1 step);
    // the remaining 60 involve at least one variable and drive the search.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;

        const expr* d1 = ep.functor("d1", {});
        const expr* d2 = ep.functor("d2", {});
        const expr* d3 = ep.functor("d3", {});
        const expr* d4 = ep.functor("d4", {});

        // digit/1 facts — the 4-element domain
        db.push_back(rule{ep.functor("cons", {ep.functor("digit", {}), d1}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("digit", {}), d2}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("digit", {}), d3}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("digit", {}), d4}), {}});  // idx 3

        // diff/2 facts — all 12 ordered pairs of distinct values
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d1}), d2}), {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d2}), d1}), {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d1}), d3}), {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d3}), d1}), {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d1}), d4}), {}});  // idx 8
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d4}), d1}), {}});  // idx 9
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d2}), d3}), {}});  // idx 10
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d3}), d2}), {}});  // idx 11
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d2}), d4}), {}});  // idx 12
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d4}), d2}), {}});  // idx 13
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d3}), d4}), {}});  // idx 14
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), d4}), d3}), {}});  // idx 15

        // Variable cells — 7 unknowns (row 1 col 2 pre-filled as d2 to force uniqueness)
        const expr* V13 = ep.var(seq());  // row 1, col 3
        const expr* V21 = ep.var(seq());  // row 2, col 1
        const expr* V24 = ep.var(seq());  // row 2, col 4
        const expr* V31 = ep.var(seq());  // row 3, col 1
        const expr* V34 = ep.var(seq());  // row 3, col 4
        const expr* V42 = ep.var(seq());  // row 4, col 2
        const expr* V43 = ep.var(seq());  // row 4, col 3

        // Grid (0-indexed): cell[row][col]
        const expr* cell[4][4] = {
            {d1,  d2,  V13, d4 },
            {V21, d4,  d1,  V24},
            {V31, d1,  d4,  V34},
            {d4,  V42, V43, d1 },
        };

        goals goals;

        // Domain goals — only for variable cells
        for (auto* v : {V13, V21, V24, V31, V34, V42, V43}) {
            goals.push_back(ep.functor("cons", {ep.functor("digit", {}), v}));
        }

        // Row distinctness constraints (i < j, one direction per pair)
        for (int r = 0; r < 4; r++) {
            for (int i = 0; i < 4; i++) {
                for (int j = i + 1; j < 4; j++) {
                    goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), cell[r][i]}), cell[r][j]}));
                }
            }
        }

        // Column distinctness constraints
        for (int c = 0; c < 4; c++) {
            for (int i = 0; i < 4; i++) {
                for (int j = i + 1; j < 4; j++) {
                    goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), cell[i][c]}), cell[j][c]}));
                }
            }
        }

        // Box distinctness constraints — four 2×2 sub-grids
        for (int br = 0; br < 2; br++) {
            for (int bc = 0; bc < 2; bc++) {
                const expr* box[4] = {
                    cell[br * 2    ][bc * 2],
                    cell[br * 2    ][bc * 2 + 1],
                    cell[br * 2 + 1][bc * 2],
                    cell[br * 2 + 1][bc * 2 + 1],
                };
                for (int i = 0; i < 4; i++) {
                    for (int j = i + 1; j < 4; j++) {
                        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), box[i]}), box[j]}));
                    }
                }
            }
        }

        assert(goals.size() == 79);

        // Expected: the single valid completion of the puzzle
        const std::set<solution> expected = {{d3, d3, d2, d2, d3, d3, d2}};

        std::mt19937 rng(42);
        horizon solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});
        normalizer norm(ep, bm);

        enumerate_all_solutions(solver, expected, [&]() -> solution {
            return {
                ep.import(norm(V13)),
                ep.import(norm(V21)), ep.import(norm(V24)),
                ep.import(norm(V31)), ep.import(norm(V34)),
                ep.import(norm(V42)), ep.import(norm(V43)),
            };
        });
    }
}

void test_ridge_sim() {
    // Test 1: Immediate solution - single goal with matching fact
    // Database: a.
    // Goals: :- a.
    // Expected: Solution found (unit propagation resolves immediately)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {}});  // Fact: a.
        
        goals goals;
        goals.push_back(ep.functor("a", {}));  // Goal: :- a.
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Execute simulation
        bool result = simulation();
        
        // CRITICAL: Should return true (solution found)
        assert(result == true);
        
        // CRITICAL: Goal store is empty (solution_detector check)
        assert(simulation.gs.size() == 0);
        
        // CRITICAL: No decisions made (unit propagation only, no dec() calls)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Resolution store has exactly 1 resolution (the unit propagation)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify exact resolution using lineage_pool
        const resolution_lineage* expected_rl = lp.resolution(lp.goal(nullptr, 0), 0);
        assert(simulation.rs.count(expected_rl) == 1);
        
        // CRITICAL: Candidate store should be empty (goal resolved)
        assert(simulation.cs.size() == 0);
        
        // CRITICAL: Trail still valid after execution
        assert(t.depth() > 0);
        
        // CRITICAL: Max resolutions not exceeded
        assert(simulation.rs.size() < simulation.max_resolutions);
    }
    
    // Test 2: Immediate conflict - no matching rules
    // Database: empty
    // Goal: :- a.
    // Expected: Conflict (no candidates for goal)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;  // Empty database
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Should return false (conflict)
        assert(result == false);
        
        // CRITICAL: Goal store NOT empty (unresolved goal remains)
        assert(simulation.gs.size() == 1);
        
        const goal_lineage* gl_conflict = lp.goal(nullptr, 0);
        assert(simulation.cs.size() == 1);
        assert(simulation.cs.at(gl_conflict).empty());
        
        // CRITICAL: No resolutions or decisions made
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        assert(simulation.rs.size() == 0);
    }
    
    // Test 3: Head elimination removes non-unifying candidates
    // Database: a., b.
    // Goal: :- a.
    // Expected: Head elim removes b, then unit prop on a
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {}});  // idx 0
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 1
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Goal store empty
        assert(simulation.gs.size() == 0);
        
        // CRITICAL: Exactly 1 resolution (unit prop on a with idx 0)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify exact resolution using lineage_pool
        const resolution_lineage* expected_rl = lp.resolution(lp.goal(nullptr, 0), 0);
        assert(simulation.rs.count(expected_rl) == 1);
        
        // CRITICAL: No decisions (only unit prop)
        assert(simulation.ds.size() == 0);
        
        // Candidate store empty after resolution
        assert(simulation.cs.size() == 0);
    }
    
    // Test 4: CDCL elimination removes avoided candidates
    // Database: a :- b., a :- c., b., c.
    // Goal: :- a.
    // Avoidance: avoid (gl0, idx 0)
    // Expected: CDCL removes idx 0, decision/unit prop on idx 1
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 2
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 3
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        // Pre-populate avoidance: avoid (gl0, idx 0)
        const goal_lineage* gl0_avoid = lp.goal(nullptr, 0);
        const resolution_lineage* rl0_avoid = lp.resolution(gl0_avoid, 0);
        
        decision_store avoid;
        avoid.insert(rl0_avoid);
        
        cdcl c;
        c.learn(lemma(avoid));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Goal store empty
        assert(simulation.gs.size() == 0);
        
        // CRITICAL: Exactly 2 resolutions (a with idx 1, then c)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact resolutions using lineage_pool
        // First resolution: root goal (idx 0) with db rule idx 1
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl0, 1);
        assert(simulation.rs.count(rl_a) == 1);
        
        // Second resolution: child goal (body idx 0 of rl_a) with db rule idx 3
        const goal_lineage* gl_c = lp.goal(rl_a, 0);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 3);
        assert(simulation.rs.count(rl_c) == 1);
        
        // CRITICAL: No decisions (unit propagations)
        assert(simulation.ds.size() == 0);
        
    }
    
    // Test 5: Unit propagation chain
    // Database: a :- b., b :- c., c.
    // Goal: :- a.
    // Expected: Chain of 3 unit propagations
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("b", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 2
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions (a, b, c)
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: No decisions (all unit propagations)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify exact lineage chain using lineage_pool
        // First: root goal (idx 0) with db rule idx 0
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        // Second: child goal (body idx 0 of rl_a) with db rule idx 1
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 1);
        assert(simulation.rs.count(rl_b) == 1);
        
        // Third: child goal (body idx 0 of rl_b) with db rule idx 2
        const goal_lineage* gl_c = lp.goal(rl_b, 0);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 2);
        assert(simulation.rs.count(rl_c) == 1);
        
        // Goal and candidate stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 6: Single decision made (pre-populated MCTS)
    // Database: a :- b., a :- c., b., c.
    // Goal: :- a.
    // Force decision on (a, idx 1) via MCTS tree
    // Expected: Decision on a→c, then unit prop on c
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 2
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 3
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // CRITICAL: Pre-populate MCTS tree to force decision on (gl0, idx 1)
        // Get the actual goal pointer from simulation.gs
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        
        // Force gl0 selection at level 1 (only goal anyway)
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_value = 100.0;
        
        // Force idx 1 selection at level 2 (using unvisited node)
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_value = 20.0;
        
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 0;  // UNVISITED - will be chosen!
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (decision on a→c, unit prop on c)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: Verify exact resolutions
        // First: decision on root goal (idx 0) with db rule idx 1
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl0, 1);
        assert(simulation.rs.count(rl_a) == 1);
        assert(simulation.ds.count(rl_a) == 1);  // This is the decision
        
        // Second: unit prop on child goal (body idx 0 of rl_a) with db rule idx 3
        const goal_lineage* gl_c = lp.goal(rl_a, 0);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 3);
        assert(simulation.rs.count(rl_c) == 1);
        assert(simulation.ds.count(rl_c) == 0);  // Not a decision
        
        // CRITICAL: MCTS simulation length is 2 (one decision call = goal + candidate)
        assert(sim.length() == 2);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 7: Decision followed by unit propagation
    // Database: a :- b., b.
    // Goal: :- a.
    // Force decision via MCTS (even though could be unit prop)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 1
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Note: With only 1 candidate per goal, both will be unit props
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 1);
        assert(simulation.rs.count(rl_b) == 1);
        
        // CRITICAL: Both are unit propagations (no decisions)
        assert(simulation.ds.size() == 0);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 8: Conflict after multiple resolutions with unsat children
    // Database: a :- b., a :- c., b :- d., c :- e.
    // Goal: :- a.
    // Expected: Decision on a, spawn children, conflict on unsat child
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("b", {}), {ep.functor("d", {})}});  // idx 2
        db.push_back(rule{ep.functor("c", {}), {ep.functor("e", {})}});  // idx 3
        // No rules for d or e
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force decision on idx 0
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 0;  // Force idx 0
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 20.0;
        
        bool result = simulation();
        
        // CRITICAL: Should return false (conflict)
        assert(result == false);
        
        // CRITICAL: Exactly 2 resolutions (a→b decision, b→d unit prop)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 2);
        assert(simulation.rs.count(rl_b) == 1);
        
        // CRITICAL: First is decision, second is unit prop
        assert(simulation.ds.size() == 1);
        assert(simulation.ds.count(rl_a) == 1);
        assert(simulation.ds.count(rl_b) == 0);
        
        // CRITICAL: Goal store NOT empty (d is unresolved)
        assert(simulation.gs.size() == 1);
        
        // CRITICAL: Conflict detected (d has no candidates)
        const goal_lineage* gl_d = lp.goal(rl_b, 0);
        assert(simulation.cs.at(gl_d).empty());
        
        // Verify MCTS was called once
        assert(sim.length() == 2);
    }
    
    // Test 9: Max resolutions exceeded
    // Database: a :- b., b :- c., c :- d., d :- e., e :- f., f :- g., g.
    // Goal: :- a.
    // max_resolutions = 3
    // Expected: Stops after 3 resolutions, returns false
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});
        db.push_back(rule{ep.functor("b", {}), {ep.functor("c", {})}});
        db.push_back(rule{ep.functor("c", {}), {ep.functor("d", {})}});
        db.push_back(rule{ep.functor("d", {}), {ep.functor("e", {})}});
        db.push_back(rule{ep.functor("e", {}), {ep.functor("f", {})}});
        db.push_back(rule{ep.functor("f", {}), {ep.functor("g", {})}});
        db.push_back(rule{ep.functor("g", {}), {}});
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{3, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});  // Max 3 resolutions!
        
        bool result = simulation();
        
        // CRITICAL: Should return false (max exceeded, not solved)
        assert(result == false);
        
        // CRITICAL: Exactly 3 resolutions (limit reached)
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Goal store NOT empty (not all goals resolved)
        assert(simulation.gs.size() > 0);
        
        // CRITICAL: No conflict (still have candidates)
        // After 3 resolutions: a→b→c→d, so d is in gs and should have candidates
        bool has_candidates = false;
        for (const auto& [gl, ge] : simulation.gs) {
            if (!simulation.cs.at(gl).empty()) {
                has_candidates = true;
                break;
            }
        }
        assert(has_candidates == true);
        
        // All 3 resolutions are unit propagations (no decisions made at limit)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify exact resolutions (a, b, c)
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 1);
        assert(simulation.rs.count(rl_b) == 1);
        
        const goal_lineage* gl_c = lp.goal(rl_b, 0);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 2);
        assert(simulation.rs.count(rl_c) == 1);
    }
    
    // Test 10: Fixpoint iteration - multiple head eliminations before decision
    // Database: a., b., c., d., e :- f.
    // Goal: :- e.
    // Expected: Head elim removes a,b,c,d (4 candidates), unit prop on e→f, conflict on f
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {}});  // idx 0
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 1
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 2
        db.push_back(rule{ep.functor("d", {}), {}});  // idx 3
        db.push_back(rule{ep.functor("e", {}), {ep.functor("f", {})}});  // idx 4
        
        goals goals;
        goals.push_back(ep.functor("e", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Before execution, verify 5 candidates added
        const goal_lineage* gl0_for_check = lp.goal(nullptr, 0);
        assert(simulation.cs.at(gl0_for_check).size() == 5);
        
        bool result = simulation();
        
        // CRITICAL: Should return false (conflict on f)
        assert(result == false);
        
        // CRITICAL: Exactly 1 resolution (e→f)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify exact resolution
        const goal_lineage* gl_e = lp.goal(nullptr, 0);
        const resolution_lineage* rl_e = lp.resolution(gl_e, 4);
        assert(simulation.rs.count(rl_e) == 1);
        
        // CRITICAL: No decisions (unit prop)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Goal store has f (unresolved)
        assert(simulation.gs.size() == 1);
        
        const goal_lineage* gl_f = lp.goal(rl_e, 0);
        assert(simulation.cs.at(gl_f).empty());
        
        assert(simulation.cs.size() == 1);
    }
    
    // Test 11: Avoidance store erasure during resolution
    // Database: a :- b., b.
    // Goal: :- a.
    // Avoidance: {{rl(gl0,0), rl_dummy}}
    // Expected: After making rl(gl0,0), it gets erased from avoidance
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 1
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        // Pre-populate avoidance with the future resolution plus a dummy
        const goal_lineage* gl0_pre = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a0_pre = lp.resolution(gl0_pre, 0);
        
        // Add dummy resolution to keep avoidance non-singleton
        const goal_lineage* gl_dummy = lp.goal(nullptr, 99);
        const resolution_lineage* rl_dummy = lp.resolution(gl_dummy, 99);
        
        decision_store avoid;
        avoid.insert(rl_a0_pre);
        avoid.insert(rl_dummy);
        
        cdcl c;
        c.learn(lemma(avoid));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Verify avoidance copied
        assert(simulation.c.avoidances.size() == 1);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (a, b)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: All unit propagations (no decisions)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Avoidance store modified (rl(gl0,0) erased after resolution)
        assert(simulation.c.avoidances.size() == 1);
        const avoidance& remaining = simulation.c.avoidances.begin()->second;
        assert(remaining.size() == 1);  // Only dummy remains
        assert(remaining.count(rl_dummy) == 1);
        assert(remaining.count(rl_a0_pre) == 0);  // This was erased
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 12: Mixed decisions and unit propagations (complex)
    // Database: a :- b., a :- c., b :- d., c :- e., d., e.
    // Goal: :- a.
    // Force decision on idx 0 (a→b), then unit prop on d
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("b", {}), {ep.functor("d", {})}});  // idx 2
        db.push_back(rule{ep.functor("c", {}), {ep.functor("e", {})}});  // idx 3
        db.push_back(rule{ep.functor("d", {}), {}});  // idx 4
        db.push_back(rule{ep.functor("e", {}), {}});  // idx 5
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force decision on (gl0, idx 0)
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 0;  // Force idx 0
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 20.0;
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions (a→b decision, b→d unit prop, d unit prop)
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: ds ⊆ rs (decision store is subset of resolution store)
        for (const resolution_lineage* rl : simulation.ds) {
            assert(simulation.rs.count(rl) == 1);
        }
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        assert(simulation.ds.count(rl_a) == 1);  // Decision
        
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 2);
        assert(simulation.rs.count(rl_b) == 1);
        assert(simulation.ds.count(rl_b) == 0);  // Unit prop
        
        const goal_lineage* gl_d = lp.goal(rl_b, 0);
        const resolution_lineage* rl_d = lp.resolution(gl_d, 4);
        assert(simulation.rs.count(rl_d) == 1);
        assert(simulation.ds.count(rl_d) == 0);  // Unit prop
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 13: Empty goals - already solved
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;  // Empty - already solved!
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Returns true (solution - no goals to resolve)
        assert(result == true);
        
        // CRITICAL: No resolutions made
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Stores all empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
    }
    
    // Test 14: Multi-goal resolution with sub-goals
    // Database: a :- b, c., b., c.
    // Goal: :- a.
    // Expected: Unit prop on a spawns b and c, both resolve via unit prop
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {}), ep.functor("c", {})}});  // idx 0
        db.push_back(rule{ep.functor("b", {}), {}});  // idx 1
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 2
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions (a, b, c)
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        // b is body index 0, c is body index 1
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 1);
        assert(simulation.rs.count(rl_b) == 1);
        
        const goal_lineage* gl_c = lp.goal(rl_a, 1);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 2);
        assert(simulation.rs.count(rl_c) == 1);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 15: Simple variable unification
    // Database: p(X) :- q(X)., q(a).
    // Goal: :- p(a).
    // Expected: Unify and solve
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        // p(X) :- q(X).
        const expr* X = ep.var(seq());
        db.push_back(rule{
            ep.functor("cons", {ep.functor("p", {}), X}),
            {ep.functor("cons", {ep.functor("q", {}), X})}
        });
        // q(a).
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));  // :- p(a).
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        // CRITICAL: No decisions
        assert(simulation.ds.size() == 0);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 16: Variable binds to variable
    // Database: p(X) :- q(X)., q(Y) :- r(Y)., r(a).
    // Goal: :- p(a).
    // Expected: Chain of variable unifications
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {ep.functor("cons", {ep.functor("q", {}), X})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), Y}), {ep.functor("cons", {ep.functor("r", {}), Y})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("a", {})}), {}});
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        const goal_lineage* gl_r = lp.goal(rl_q, 0);
        const resolution_lineage* rl_r = lp.resolution(gl_r, 2);
        assert(simulation.rs.count(rl_r) == 1);
        
        // No decisions
        assert(simulation.ds.size() == 0);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 17: Complex multi-variable with multiple goals
    // Database: p(X,Y) :- q(X), r(Y)., q(a)., r(b).
    // Goal: :- p(a,b).
    // Expected: Resolve p, spawn q(a) and r(b), both resolve
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{
            ep.functor("cons", {ep.functor("p", {}), ep.functor("cons", {X, Y})}),
            {ep.functor("cons", {ep.functor("q", {}), X}), ep.functor("cons", {ep.functor("r", {}), Y})}
        });
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("b", {})}), {}});
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        const goal_lineage* gl_r = lp.goal(rl_p, 1);
        const resolution_lineage* rl_r = lp.resolution(gl_r, 2);
        assert(simulation.rs.count(rl_r) == 1);
        
        // No decisions
        assert(simulation.ds.size() == 0);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 18: Complex nested variables with decisions
    // Database: p(X) :- q(X)., p(X) :- r(X)., q(a)., r(b).
    // Goal: :- p(a).
    // Force decision on idx 0, verify unification
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X1 = ep.var(seq());
        const expr* X2 = ep.var(seq());
        
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X1}), {ep.functor("cons", {ep.functor("q", {}), X1})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X2}), {ep.functor("cons", {ep.functor("r", {}), X2})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("b", {})}), {}});  // idx 3
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Force decision on idx 0
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_value = 5000.0;  // Massive reward
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 10.0;
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl0, 0);
        assert(simulation.rs.count(rl_p) == 1);
        assert(simulation.ds.count(rl_p) == 1);  // Decision
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 2);
        assert(simulation.rs.count(rl_q) == 1);
        assert(simulation.ds.count(rl_q) == 0);  // Unit prop
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 19: Many database entries (20 rules)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        
        // Add 19 non-matching rules
        for (int i = 0; i < 19; i++) {
            db.push_back(rule{ep.functor("x" + std::to_string(i), {}), {}});
        }
        // Add 1 matching rule
        db.push_back(rule{ep.functor("target", {}), {}});  // idx 19
        
        goals goals;
        goals.push_back(ep.functor("target", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Initial state: 20 candidates
        const goal_lineage* gl0_for_check = lp.goal(nullptr, 0);
        assert(simulation.cs.at(gl0_for_check).size() == 20);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution (head elim removes 19, unit prop on idx 19)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify exact resolution
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl0, 19);
        assert(simulation.rs.count(rl) == 1);
        
        // No decisions
        assert(simulation.ds.size() == 0);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 20: Many goals (10 goals, all solvable)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        goals goals;
        
        // Create 10 independent facts and goals
        for (int i = 0; i < 10; i++) {
            db.push_back(rule{ep.functor("g" + std::to_string(i), {}), {}});
            goals.push_back(ep.functor("g" + std::to_string(i), {}));
        }
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 10 resolutions (one per goal)
        assert(simulation.rs.size() == 10);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify all 10 exact resolutions
        for (int i = 0; i < 10; i++) {
            const goal_lineage* gl = lp.goal(nullptr, i);
            const resolution_lineage* rl = lp.resolution(gl, i);
            assert(simulation.rs.count(rl) == 1);
        }
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 21: Complex with decisions, unit props, and avoidance reduction by learn()
    // Database: a :- b., a :- c., b :- d., c., d.
    // Goal: :- a.
    // Decision store submitted: {rl(gl0,0), rl(gl_b,2)}
    // After reduce(): rl(gl0,0) is ancestor of rl(gl_b,2) → stripped; stored avoidance = {rl(gl_b,2)}
    // Force decision on idx 1 (a→c); avoidance watched by gl_b, which is never visited → stays
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("b", {}), {ep.functor("d", {})}});  // idx 2
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 3
        db.push_back(rule{ep.functor("d", {}), {}});  // idx 4
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        // Pre-populate avoidance
        const goal_lineage* gl0_pre = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a0_pre = lp.resolution(gl0_pre, 0);
        const goal_lineage* gl_b_pre = lp.goal(rl_a0_pre, 0);
        const resolution_lineage* rl_b_pre = lp.resolution(gl_b_pre, 2);
        
        decision_store avoid;
        avoid.insert(rl_a0_pre);
        avoid.insert(rl_b_pre);
        
        cdcl c;
        c.learn(lemma(avoid));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Force decision on idx 1
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_value = 10.0;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 0;  // Force idx 1
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (a→c decision, c unit prop)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl0, 1);
        assert(simulation.rs.count(rl_a) == 1);
        assert(simulation.ds.count(rl_a) == 1);  // Decision
        
        const goal_lineage* gl_c = lp.goal(rl_a, 0);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 3);
        assert(simulation.rs.count(rl_c) == 1);
        assert(simulation.ds.count(rl_c) == 0);  // Unit prop
        
        // CRITICAL: Avoidance preserved — learn() reduced {rl(gl0,0), rl(gl_b,2)} to
        // {rl(gl_b,2)} (ancestor stripped). The singleton watches gl_b, which is never
        // visited in the a→c path, so the avoidance is never erased.
        assert(simulation.c.avoidances.size() == 1);
        assert(simulation.c.avoidances.at(0).size() == 1);
        assert(simulation.c.avoidances.at(0).count(rl_b_pre) == 1);
        // Singleton avoidance → rl_b_pre is immediately eliminated
        assert(simulation.c.eliminated_resolutions.count(rl_b_pre) == 1);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 22: Conflict with variables - unification fails
    // Database: p(a) :- q(b).
    // Goal: :- p(a).
    // Expected: Spawn q(b), conflict (no rule for q(b))
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {ep.functor("cons", {ep.functor("q", {}), ep.functor("b", {})})}});
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Conflict detected
        assert(result == false);
        
        // CRITICAL: Exactly 1 resolution (p(a)→q(b))
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify exact resolution
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl) == 1);
        
        // CRITICAL: No decisions (unit prop)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Goal store has q(b) unresolved
        assert(simulation.gs.size() == 1);
        
        const goal_lineage* gl_q = lp.goal(rl, 0);
        assert(simulation.cs.at(gl_q).empty());
    }
    
    // Test 23: Multiple decisions with pre-populated MCTS
    // Database: a :- b., a :- c., b :- d., b :- e., c., d., e.
    // Goal: :- a.
    // Force decision on (a, idx 0), then decision on (b, idx 3)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1
        db.push_back(rule{ep.functor("b", {}), {ep.functor("d", {})}});  // idx 2
        db.push_back(rule{ep.functor("b", {}), {ep.functor("e", {})}});  // idx 3
        db.push_back(rule{ep.functor("c", {}), {}});  // idx 4
        db.push_back(rule{ep.functor("d", {}), {}});  // idx 5
        db.push_back(rule{ep.functor("e", {}), {}});  // idx 6
        
        goals goals;
        goals.push_back(ep.functor("a", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Force first decision on (gl0, idx 0)
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 0;  // Force idx 0
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 10.0;
        
        // After first decision, need to pre-populate for second decision
        // gl_b will be created during simulation, can't pre-populate here
        // Just verify result
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions (a→b decision, b→? decision, ? unit prop)
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Exactly 2 decisions (a and b both have 2 candidates)
        assert(simulation.ds.size() == 2);
        
        // CRITICAL: Verify first decision (a with idx 0)
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        assert(simulation.ds.count(rl_a) == 1);
        
        // CRITICAL: ds ⊆ rs
        for (const resolution_lineage* rl : simulation.ds) {
            assert(simulation.rs.count(rl) == 1);
        }
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 24: Complex variable scenario with list structures
    // Database: append(nil, X, X)., append(cons(H,T), X, cons(H,R)) :- append(T, X, R).
    // Goal: :- append(cons(a,nil), cons(b,nil), cons(a,cons(b,nil))).
    // Expected: Resolution with recursive rule
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        
        // append(nil, X, X).
        const expr* X1 = ep.var(seq());
        db.push_back(rule{
            ep.functor("cons", {ep.functor("append", {}), ep.functor("cons", {ep.functor("nil", {}), ep.functor("cons", {X1, X1})})}),
            {}
        });
        
        // append(cons(H,T), X, cons(H,R)) :- append(T, X, R).
        const expr* H = ep.var(seq());
        const expr* T = ep.var(seq());
        const expr* X2 = ep.var(seq());
        const expr* R = ep.var(seq());
        
        db.push_back(rule{
            ep.functor("cons", {ep.functor("append", {}), ep.functor("cons", {
                ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {H, T})}), 
                ep.functor("cons", {X2, ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {H, R})})})
            })}),
            {ep.functor("cons", {ep.functor("append", {}), ep.functor("cons", {T, ep.functor("cons", {X2, R})})})}
        });
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("append", {}), ep.functor("cons", {
            ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("nil", {})})}), 
            ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("b", {}), ep.functor("nil", {})})}), 
                    ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("b", {}), ep.functor("nil", {})})})})})})
        })}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (recursive case, base case)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_append1 = lp.goal(nullptr, 0);
        const resolution_lineage* rl_recursive = lp.resolution(gl_append1, 1);
        assert(simulation.rs.count(rl_recursive) == 1);
        
        const goal_lineage* gl_append2 = lp.goal(rl_recursive, 0);
        const resolution_lineage* rl_base = lp.resolution(gl_append2, 0);
        assert(simulation.rs.count(rl_base) == 1);
        
        // No decisions (both unit props after head elim)
        assert(simulation.ds.size() == 0);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // Test 25: Large complex problem with 30 rules and multiple goals
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        
        // Create a complex dependency graph
        // 30 rules total
        for (int i = 0; i < 10; i++) {
            db.push_back(rule{ep.functor("noise" + std::to_string(i), {}), {}});
        }
        
        // Main rules: p :- q, r., q., r.
        db.push_back(rule{ep.functor("p", {}), {ep.functor("q", {}), ep.functor("r", {})}});
        db.push_back(rule{ep.functor("q", {}), {}});
        db.push_back(rule{ep.functor("r", {}), {}});
        
        // More noise
        for (int i = 10; i < 27; i++) {
            db.push_back(rule{ep.functor("noise" + std::to_string(i), {}), {}});
        }
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Verify 30 candidates initially
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(simulation.cs.at(gl0).size() == 30);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions (p, q, r)
        assert(simulation.rs.size() == 3);
        
        // No decisions (all unit props after massive head elimination)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify exact resolutions (should be db indices 10, 11, 12)
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 10);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 11);
        assert(simulation.rs.count(rl_q) == 1);
        
        const goal_lineage* gl_r = lp.goal(rl_p, 1);
        const resolution_lineage* rl_r = lp.resolution(gl_r, 12);
        assert(simulation.rs.count(rl_r) == 1);
        
        // Stores empty
        assert(simulation.gs.size() == 0);
        assert(simulation.cs.size() == 0);
    }
    
    // ========== TESTS WITH VARIABLES IN GOALS ==========
    
    // Test 26: Single variable in goal binds to atom
    // Database: p(a)., p(b).
    // Goal: :- p(X).
    // Expected: X binds to a (forced by MCTS to choose idx 0)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("b", {})}), {}});  // idx 1
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force decision on idx 0
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 0;  // Force idx 0
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 10.0;
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: Verify exact resolution
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        assert(simulation.rs.count(rl0) == 1);
        assert(simulation.ds.count(rl0) == 1);
        
        // CRITICAL: Verify X binds to exactly 'a'
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        assert(X_normalized == ep.functor("a", {}));
    }
    
    // Test 27: Variable binds to compound term
    // Database: q(cons(a,nil)).
    // Goal: :- q(X).
    // Expected: X binds to cons(a,nil)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* cons_a_nil = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("nil", {})})});
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), cons_a_nil}), {}});  // idx 0
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("q", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: X binds to cons(a,nil)
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        assert(X_normalized == cons_a_nil);
    }
    
    // Test 28: Multiple variables in single goal
    // Database: pair(a,b).
    // Goal: :- pair(X,Y).
    // Expected: X binds to a, Y binds to b
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* pair_a_b = ep.functor("cons", {ep.functor("pair", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})});
        db.push_back(rule{pair_a_b, {}});  // idx 0
        
        goals goals;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* pair_X_Y = ep.functor("cons", {ep.functor("pair", {}), ep.functor("cons", {X, Y})});
        goals.push_back(pair_X_Y);
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Verify X and Y bindings
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        const expr* Y_normalized = norm(Y);
        assert(X_normalized == ep.functor("a", {}));
        assert(Y_normalized == ep.functor("b", {}));
    }
    
    // Test 29: Variable binds through rule chain
    // Database: p(X) :- q(X)., q(hello).
    // Goal: :- p(Y).
    // Expected: Y binds to hello
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X_rule = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X_rule}), {ep.functor("cons", {ep.functor("q", {}), X_rule})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("hello", {})}), {}});  // idx 1
        
        goals goals;
        const expr* Y_goal = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), Y_goal}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Verify Y binds to hello through chain
        normalizer norm(ep, bm);
        const expr* Y_normalized = norm(Y_goal);
        assert(Y_normalized == ep.functor("hello", {}));
    }
    
    // Test 30: Two starting goals with variables
    // Database: p(a)., q(b).
    // Goals: :- p(X), q(Y).
    // Expected: X binds to a, Y binds to b
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("b", {})}), {}});  // idx 1
        
        goals goals;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), X}));
        goals.push_back(ep.functor("cons", {ep.functor("q", {}), Y}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 2 resolutions (one per goal)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(nullptr, 1);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        // CRITICAL: No decisions (both unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify both variable bindings
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        const expr* Y_normalized = norm(Y);
        assert(X_normalized == ep.functor("a", {}));
        assert(Y_normalized == ep.functor("b", {}));
    }
    
    // Test 31: Three starting goals with shared variable
    // Database: p(a)., q(a)., r(a).
    // Goals: :- p(X), q(X), r(X).
    // Expected: X binds to a (unifies across all three)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("a", {})}), {}});  // idx 2
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), X}));
        goals.push_back(ep.functor("cons", {ep.functor("q", {}), X}));
        goals.push_back(ep.functor("cons", {ep.functor("r", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 3 resolutions
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Verify exact lineages for all 3 resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(nullptr, 1);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        const goal_lineage* gl_r = lp.goal(nullptr, 2);
        const resolution_lineage* rl_r = lp.resolution(gl_r, 2);
        assert(simulation.rs.count(rl_r) == 1);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: X binds to a
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        assert(X_normalized == ep.functor("a", {}));
    }
    
    // Test 32: Multiple goals with dependent variables
    // Database: f(a,b)., g(b,c).
    // Goals: :- f(X,Y), g(Y,Z).
    // Expected: X=a, Y=b, Z=c (Y is shared and must unify)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* f_a_b = ep.functor("cons", {ep.functor("f", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})});
        const expr* g_b_c = ep.functor("cons", {ep.functor("g", {}), ep.functor("cons", {ep.functor("b", {}), ep.functor("c", {})})});
        db.push_back(rule{f_a_b, {}});  // idx 0
        db.push_back(rule{g_b_c, {}});  // idx 1
        
        goals goals;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("f", {}), ep.functor("cons", {X, Y})}));
        goals.push_back(ep.functor("cons", {ep.functor("g", {}), ep.functor("cons", {Y, Z})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify exact lineages
        const goal_lineage* gl_f = lp.goal(nullptr, 0);
        const resolution_lineage* rl_f = lp.resolution(gl_f, 0);
        assert(simulation.rs.count(rl_f) == 1);
        
        const goal_lineage* gl_g = lp.goal(nullptr, 1);
        const resolution_lineage* rl_g = lp.resolution(gl_g, 1);
        assert(simulation.rs.count(rl_g) == 1);
        
        // CRITICAL: No decisions (both unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Verify all three variables
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        const expr* Y_normalized = norm(Y);
        const expr* Z_normalized = norm(Z);
        assert(X_normalized == ep.functor("a", {}));
        assert(Y_normalized == ep.functor("b", {}));
        assert(Z_normalized == ep.functor("c", {}));
    }
    
    // Test 33: Variable binds to complex nested structure
    // Database: tree(node(leaf(1), leaf(2))).
    // Goal: :- tree(X).
    // Expected: X binds to node(leaf(1), leaf(2))
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* leaf1 = ep.functor("cons", {ep.functor("leaf", {}), ep.functor("1", {})});
        const expr* leaf2 = ep.functor("cons", {ep.functor("leaf", {}), ep.functor("2", {})});
        const expr* node = ep.functor("cons", {ep.functor("node", {}), ep.functor("cons", {leaf1, leaf2})});
        const expr* tree = ep.functor("cons", {ep.functor("tree", {}), node});
        db.push_back(rule{tree, {}});  // idx 0
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("tree", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: X binds to the nested structure
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        assert(X_normalized == node);
    }
    
    // Test 34: Multiple goals with rules spawning sub-goals with variables
    // Database: p(X) :- q(X)., q(Y) :- r(Y)., r(hello).
    // Goals: :- p(A), p(B).
    // Expected: A=hello, B=hello
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {ep.functor("cons", {ep.functor("q", {}), X})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), Y}), {ep.functor("cons", {ep.functor("r", {}), Y})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("hello", {})}), {}});  // idx 2
        
        goals goals;
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), A}));
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), B}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 6 resolutions (p(A), q(...), r(...), p(B), q(...), r(...))
        assert(simulation.rs.size() == 6);
        
        // CRITICAL: Verify exact lineages for first chain (A)
        const goal_lineage* gl_pA = lp.goal(nullptr, 0);
        const resolution_lineage* rl_pA = lp.resolution(gl_pA, 0);
        assert(simulation.rs.count(rl_pA) == 1);
        
        const goal_lineage* gl_qA = lp.goal(rl_pA, 0);
        const resolution_lineage* rl_qA = lp.resolution(gl_qA, 1);
        assert(simulation.rs.count(rl_qA) == 1);
        
        const goal_lineage* gl_rA = lp.goal(rl_qA, 0);
        const resolution_lineage* rl_rA = lp.resolution(gl_rA, 2);
        assert(simulation.rs.count(rl_rA) == 1);
        
        // CRITICAL: Verify exact lineages for second chain (B)
        const goal_lineage* gl_pB = lp.goal(nullptr, 1);
        const resolution_lineage* rl_pB = lp.resolution(gl_pB, 0);
        assert(simulation.rs.count(rl_pB) == 1);
        
        const goal_lineage* gl_qB = lp.goal(rl_pB, 0);
        const resolution_lineage* rl_qB = lp.resolution(gl_qB, 1);
        assert(simulation.rs.count(rl_qB) == 1);
        
        const goal_lineage* gl_rB = lp.goal(rl_qB, 0);
        const resolution_lineage* rl_rB = lp.resolution(gl_rB, 2);
        assert(simulation.rs.count(rl_rB) == 1);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Both A and B bind to hello
        normalizer norm(ep, bm);
        const expr* A_normalized = norm(A);
        const expr* B_normalized = norm(B);
        assert(A_normalized == ep.functor("hello", {}));
        assert(B_normalized == ep.functor("hello", {}));
    }
    
    // Test 35: Five starting goals with mixed variables
    // Database: a(1)., b(2)., c(3)., d(4)., e(5).
    // Goals: :- a(V1), b(V2), c(V3), d(V4), e(V5).
    // Expected: V1=1, V2=2, V3=3, V4=4, V5=5
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("a", {}), ep.functor("1", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("b", {}), ep.functor("2", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("c", {}), ep.functor("3", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("d", {}), ep.functor("4", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("e", {}), ep.functor("5", {})}), {}});  // idx 4
        
        goals goals;
        const expr* V1 = ep.var(seq());
        const expr* V2 = ep.var(seq());
        const expr* V3 = ep.var(seq());
        const expr* V4 = ep.var(seq());
        const expr* V5 = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("a", {}), V1}));
        goals.push_back(ep.functor("cons", {ep.functor("b", {}), V2}));
        goals.push_back(ep.functor("cons", {ep.functor("c", {}), V3}));
        goals.push_back(ep.functor("cons", {ep.functor("d", {}), V4}));
        goals.push_back(ep.functor("cons", {ep.functor("e", {}), V5}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 5 resolutions
        assert(simulation.rs.size() == 5);
        
        // CRITICAL: Verify exact lineages for all 5 resolutions
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        const goal_lineage* gl_b = lp.goal(nullptr, 1);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 1);
        assert(simulation.rs.count(rl_b) == 1);
        
        const goal_lineage* gl_c = lp.goal(nullptr, 2);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 2);
        assert(simulation.rs.count(rl_c) == 1);
        
        const goal_lineage* gl_d = lp.goal(nullptr, 3);
        const resolution_lineage* rl_d = lp.resolution(gl_d, 3);
        assert(simulation.rs.count(rl_d) == 1);
        
        const goal_lineage* gl_e = lp.goal(nullptr, 4);
        const resolution_lineage* rl_e = lp.resolution(gl_e, 4);
        assert(simulation.rs.count(rl_e) == 1);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Verify all five variables
        normalizer norm(ep, bm);
        assert(norm(V1) == ep.functor("1", {}));
        assert(norm(V2) == ep.functor("2", {}));
        assert(norm(V3) == ep.functor("3", {}));
        assert(norm(V4) == ep.functor("4", {}));
        assert(norm(V5) == ep.functor("5", {}));
    }
    
    // Test 36: Variable in goal with decision required
    // Database: p(X) :- q(X)., p(X) :- r(X)., q(apple)., r(banana).
    // Goal: :- p(Fruit).
    // Expected: Fruit binds to apple or banana (depends on MCTS decision)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X1 = ep.var(seq());
        const expr* X2 = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X1}), {ep.functor("cons", {ep.functor("q", {}), X1})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X2}), {ep.functor("cons", {ep.functor("r", {}), X2})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("apple", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("banana", {})}), {}});  // idx 3
        
        goals goals;
        const expr* Fruit = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), Fruit}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Force decision on idx 0 (q path -> apple)
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 0;  // Force idx 0
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 10.0;
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: Fruit binds to apple (based on forced decision)
        normalizer norm(ep, bm);
        const expr* Fruit_normalized = norm(Fruit);
        assert(Fruit_normalized == ep.functor("apple", {}));
    }
    
    // Test 37: Partial instantiation with variable (base case)
    // Database: link(a,b)., link(b,c)., path(X,Y) :- link(X,Y)., path(X,Z) :- link(X,Y), path(Y,Z).
    // Goal: :- path(a,Dest).
    // Expected: Dest binds to b (forced to use base case idx 2)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* link_a_b = ep.functor("cons", {ep.functor("link", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})});
        const expr* link_b_c = ep.functor("cons", {ep.functor("link", {}), ep.functor("cons", {ep.functor("b", {}), ep.functor("c", {})})});
        db.push_back(rule{link_a_b, {}});  // idx 0
        db.push_back(rule{link_b_c, {}});  // idx 1
        
        const expr* X1 = ep.var(seq());
        const expr* Y1 = ep.var(seq());
        const expr* path_base = ep.functor("cons", {ep.functor("path", {}), ep.functor("cons", {X1, Y1})});
        const expr* link_X1_Y1 = ep.functor("cons", {ep.functor("link", {}), ep.functor("cons", {X1, Y1})});
        db.push_back(rule{path_base, {link_X1_Y1}});  // idx 2
        
        const expr* X2 = ep.var(seq());
        const expr* Y2 = ep.var(seq());
        const expr* Z2 = ep.var(seq());
        const expr* path_rec = ep.functor("cons", {ep.functor("path", {}), ep.functor("cons", {X2, Z2})});
        const expr* link_X2_Y2 = ep.functor("cons", {ep.functor("link", {}), ep.functor("cons", {X2, Y2})});
        const expr* path_Y2_Z2 = ep.functor("cons", {ep.functor("path", {}), ep.functor("cons", {Y2, Z2})});
        db.push_back(rule{path_rec, {link_X2_Y2, path_Y2_Z2}});  // idx 3
        
        goals goals;
        const expr* Dest = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("path", {}), ep.functor("cons", {ep.functor("a", {}), Dest})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force base case (idx 2)
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(2)].m_visits = 0;  // Force idx 2
        root.m_children[gl0_for_mcts].m_children[size_t(3)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(3)].m_value = 10.0;
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (path→link)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Exactly 1 decision
        assert(simulation.ds.size() == 1);
        
        // CRITICAL: Verify exact lineage chain
        const goal_lineage* gl_path = lp.goal(nullptr, 0);
        const resolution_lineage* rl_path = lp.resolution(gl_path, 2);  // Base case
        assert(simulation.rs.count(rl_path) == 1);
        assert(simulation.ds.count(rl_path) == 1);  // This is the decision
        
        const goal_lineage* gl_link = lp.goal(rl_path, 0);
        const resolution_lineage* rl_link = lp.resolution(gl_link, 0);  // link(a,b)
        assert(simulation.rs.count(rl_link) == 1);
        assert(simulation.ds.count(rl_link) == 0);  // Unit prop
        
        // CRITICAL: MCTS called once (2 calls: goal + candidate)
        assert(sim.length() == 2);
        
        // CRITICAL: Dest binds to exactly 'b'
        normalizer norm(ep, bm);
        const expr* Dest_normalized = norm(Dest);
        assert(Dest_normalized == ep.functor("b", {}));
    }
    
    // Test 38: Multiple goals with complex variable sharing
    // Database: add(X,Y,Z) :- plus(X,Y,Z)., plus(1,2,3)., mul(A,B,C) :- times(A,B,C)., times(2,3,6).
    // Goals: :- add(1,2,Sum), mul(2,3,Prod).
    // Expected: Sum=3, Prod=6
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        const expr* add_rule = ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {X, ep.functor("cons", {Y, Z})})});
        const expr* plus_body = ep.functor("cons", {ep.functor("plus", {}), ep.functor("cons", {X, ep.functor("cons", {Y, Z})})});
        db.push_back(rule{add_rule, {plus_body}});  // idx 0
        
        const expr* plus_fact = ep.functor("cons", {ep.functor("plus", {}), ep.functor("cons", {ep.functor("1", {}), ep.functor("cons", {ep.functor("2", {}), ep.functor("3", {})})})});
        db.push_back(rule{plus_fact, {}});  // idx 1
        
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* mul_rule = ep.functor("cons", {ep.functor("mul", {}), ep.functor("cons", {A, ep.functor("cons", {B, C})})});
        const expr* times_body = ep.functor("cons", {ep.functor("times", {}), ep.functor("cons", {A, ep.functor("cons", {B, C})})});
        db.push_back(rule{mul_rule, {times_body}});  // idx 2
        
        const expr* times_fact = ep.functor("cons", {ep.functor("times", {}), ep.functor("cons", {ep.functor("2", {}), ep.functor("cons", {ep.functor("3", {}), ep.functor("6", {})})})});
        db.push_back(rule{times_fact, {}});  // idx 3
        
        goals goals;
        const expr* Sum = ep.var(seq());
        const expr* Prod = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("1", {}), ep.functor("cons", {ep.functor("2", {}), Sum})})}));
        goals.push_back(ep.functor("cons", {ep.functor("mul", {}), ep.functor("cons", {ep.functor("2", {}), ep.functor("cons", {ep.functor("3", {}), Prod})})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 4 resolutions (add->plus fact, mul->times fact)
        assert(simulation.rs.size() == 4);
        
        // CRITICAL: Verify exact lineages for add chain
        const goal_lineage* gl_add = lp.goal(nullptr, 0);
        const resolution_lineage* rl_add = lp.resolution(gl_add, 0);
        assert(simulation.rs.count(rl_add) == 1);
        
        const goal_lineage* gl_plus = lp.goal(rl_add, 0);
        const resolution_lineage* rl_plus = lp.resolution(gl_plus, 1);
        assert(simulation.rs.count(rl_plus) == 1);
        
        // CRITICAL: Verify exact lineages for mul chain
        const goal_lineage* gl_mul = lp.goal(nullptr, 1);
        const resolution_lineage* rl_mul = lp.resolution(gl_mul, 2);
        assert(simulation.rs.count(rl_mul) == 1);
        
        const goal_lineage* gl_times = lp.goal(rl_mul, 0);
        const resolution_lineage* rl_times = lp.resolution(gl_times, 3);
        assert(simulation.rs.count(rl_times) == 1);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Verify both results
        normalizer norm(ep, bm);
        const expr* Sum_normalized = norm(Sum);
        const expr* Prod_normalized = norm(Prod);
        assert(Sum_normalized == ep.functor("3", {}));
        assert(Prod_normalized == ep.functor("6", {}));
    }
    
    // ========== NEW TESTS: CONFLICT AND EDGE CASES ==========
    
    // Test 39: Unification failure with shared variable
    // Database: p(a)., q(b).
    // Goals: :- p(X), q(X).
    // Expected: CONFLICT (X cannot be both 'a' and 'b')
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("b", {})}), {}});  // idx 1
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), X}));
        goals.push_back(ep.functor("cons", {ep.functor("q", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Conflict (unification failure)
        assert(result == false);
        
        // CRITICAL: Exactly 1 resolution (one goal resolves, other becomes unsatisfiable)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: One goal still in goal store with no candidates
        assert(simulation.gs.size() == 1);
        
        // CRITICAL: The remaining goal has no candidates
        const goal_lineage* remaining_goal = simulation.gs.begin()->first;
        assert(simulation.cs.at(remaining_goal).empty());
        
        // CRITICAL: No decisions
        assert(simulation.ds.size() == 0);
    }
    
    // Test 40: Same variable multiple times in one goal
    // Database: pair(a,a)., pair(a,b)., pair(b,b).
    // Goal: :- pair(X,X).
    // Expected: X=b (head elim removes pair(a,b), MCTS forces idx 2)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("pair", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("a", {})})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("pair", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("pair", {}), ep.functor("cons", {ep.functor("b", {}), ep.functor("b", {})})}), {}});  // idx 2
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("pair", {}), ep.functor("cons", {X, X})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force idx 2 (NOT idx 0, to prove MCTS works!)
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_value = 10.0;
        root.m_children[gl0_for_mcts].m_children[size_t(2)].m_visits = 0;  // Force idx 2
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify resolution used idx 2 (NOT idx 0!)
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl2 = lp.resolution(gl0, 2);
        assert(simulation.rs.count(rl2) == 1);
        assert(simulation.ds.count(rl2) == 1);  // Decision
        
        // CRITICAL: Head elim should have removed idx 1
        // Initial: 3 candidates, after head elim: 2 candidates (idx 0, 2)
        // MCTS forced idx 2, proving it works correctly
        
        // CRITICAL: X binds to exactly 'b' (NOT 'a', because we used idx 2)
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        assert(X_normalized == ep.functor("b", {}));
        
        // CRITICAL: MCTS called once
        assert(sim.length() == 2);
    }
    
    // Test 41: Conflict with variable goal (no matching rules)
    // Database: q(a).
    // Goal: :- p(X).
    // Expected: CONFLICT (no candidates for p(X))
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 0
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict (no candidates)
        assert(result == false);
        
        // CRITICAL: No resolutions made
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal still in store
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.gs.members.count(gl_p) == 1);
        assert(simulation.cs.at(gl_p).empty());
        
        // CRITICAL: No decisions
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
    }
    
    // Test 42: Complex variable chain with conflict
    // Database: p(X) :- q(X)., q(Y) :- r(Y).
    // Goal: :- p(Z).
    // Expected: CONFLICT (no r facts)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X_rule = ep.var(seq());
        const expr* Y_rule = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X_rule}), {ep.functor("cons", {ep.functor("q", {}), X_rule})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), Y_rule}), {ep.functor("cons", {ep.functor("r", {}), Y_rule})}});  // idx 1
        // No r facts
        
        goals goals;
        const expr* Z_goal = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), Z_goal}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Conflict after chain
        assert(result == false);
        
        // CRITICAL: Exactly 2 resolutions (p→q, q→r)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify lineage chain
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        // CRITICAL: r has no candidates
        const goal_lineage* gl_r = lp.goal(rl_q, 0);
        assert(simulation.gs.members.count(gl_r) == 1);
        assert(simulation.cs.at(gl_r).empty());
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
    }
    
    // Test 43: Mixed instantiated and variable goals
    // Database: p(a)., q(b).
    // Goals: :- p(a), q(X).
    // Expected: X=b
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("b", {})}), {}});  // idx 1
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));  // Instantiated
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("q", {}), X}));  // Variable
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify lineages
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(nullptr, 1);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        // CRITICAL: No decisions
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: X binds to 'b'
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        assert(X_normalized == ep.functor("b", {}));
    }
    
    // Test 44: Deep variable chain (6 levels)
    // Database: a(X) :- b(X)., b(Y) :- c(Y)., c(Z) :- d(Z).,
    //           d(W) :- e(W)., e(V) :- f(V)., f(hello).
    // Goal: :- a(Result).
    // Expected: Result = hello (verify all 6 resolutions)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        const expr* W = ep.var(seq());
        const expr* V = ep.var(seq());
        
        db.push_back(rule{ep.functor("cons", {ep.functor("a", {}), X}), {ep.functor("cons", {ep.functor("b", {}), X})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("b", {}), Y}), {ep.functor("cons", {ep.functor("c", {}), Y})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("c", {}), Z}), {ep.functor("cons", {ep.functor("d", {}), Z})}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("d", {}), W}), {ep.functor("cons", {ep.functor("e", {}), W})}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("e", {}), V}), {ep.functor("cons", {ep.functor("f", {}), V})}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("f", {}), ep.functor("hello", {})}), {}});  // idx 5
        
        goals goals;
        const expr* Result = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("a", {}), Result}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 6 resolutions
        assert(simulation.rs.size() == 6);
        
        // CRITICAL: Verify entire lineage chain
        const goal_lineage* gl_a = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 0);
        assert(simulation.rs.count(rl_a) == 1);
        
        const goal_lineage* gl_b = lp.goal(rl_a, 0);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 1);
        assert(simulation.rs.count(rl_b) == 1);
        
        const goal_lineage* gl_c = lp.goal(rl_b, 0);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 2);
        assert(simulation.rs.count(rl_c) == 1);
        
        const goal_lineage* gl_d = lp.goal(rl_c, 0);
        const resolution_lineage* rl_d = lp.resolution(gl_d, 3);
        assert(simulation.rs.count(rl_d) == 1);
        
        const goal_lineage* gl_e = lp.goal(rl_d, 0);
        const resolution_lineage* rl_e = lp.resolution(gl_e, 4);
        assert(simulation.rs.count(rl_e) == 1);
        
        const goal_lineage* gl_f = lp.goal(rl_e, 0);
        const resolution_lineage* rl_f = lp.resolution(gl_f, 5);
        assert(simulation.rs.count(rl_f) == 1);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Result binds to 'hello'
        normalizer norm(ep, bm);
        const expr* Result_normalized = norm(Result);
        assert(Result_normalized == ep.functor("hello", {}));
    }
    
    // Test 45: Variables in multiple body goals
    // Database: foo(X,Y,Z) :- bar(X,Y), baz(Y,Z)., bar(1,2)., baz(2,3).
    // Goal: :- foo(A,B,C).
    // Expected: A=1, B=2, C=3 (Y propagates through)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        
        const expr* foo_head = ep.functor("cons", {ep.functor("foo", {}), ep.functor("cons", {X, ep.functor("cons", {Y, Z})})});
        const expr* bar_body = ep.functor("cons", {ep.functor("bar", {}), ep.functor("cons", {X, Y})});
        const expr* baz_body = ep.functor("cons", {ep.functor("baz", {}), ep.functor("cons", {Y, Z})});
        db.push_back(rule{foo_head, {bar_body, baz_body}});  // idx 0
        
        const expr* bar_fact = ep.functor("cons", {ep.functor("bar", {}), ep.functor("cons", {ep.functor("1", {}), ep.functor("2", {})})});
        db.push_back(rule{bar_fact, {}});  // idx 1
        
        const expr* baz_fact = ep.functor("cons", {ep.functor("baz", {}), ep.functor("cons", {ep.functor("2", {}), ep.functor("3", {})})});
        db.push_back(rule{baz_fact, {}});  // idx 2
        
        goals goals;
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("foo", {}), ep.functor("cons", {A, ep.functor("cons", {B, C})})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 3 resolutions (foo, bar, baz)
        assert(simulation.rs.size() == 3);
        
        // CRITICAL: Verify lineages
        const goal_lineage* gl_foo = lp.goal(nullptr, 0);
        const resolution_lineage* rl_foo = lp.resolution(gl_foo, 0);
        assert(simulation.rs.count(rl_foo) == 1);
        
        const goal_lineage* gl_bar = lp.goal(rl_foo, 0);
        const resolution_lineage* rl_bar = lp.resolution(gl_bar, 1);
        assert(simulation.rs.count(rl_bar) == 1);
        
        const goal_lineage* gl_baz = lp.goal(rl_foo, 1);
        const resolution_lineage* rl_baz = lp.resolution(gl_baz, 2);
        assert(simulation.rs.count(rl_baz) == 1);
        
        // CRITICAL: No decisions
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Verify all three variables
        normalizer norm(ep, bm);
        const expr* A_normalized = norm(A);
        const expr* B_normalized = norm(B);
        const expr* C_normalized = norm(C);
        assert(A_normalized == ep.functor("1", {}));
        assert(B_normalized == ep.functor("2", {}));
        assert(C_normalized == ep.functor("3", {}));
    }
    
    // Test 46: Head elimination with variables
    // Database: p(cons(a,X))., p(cons(b,Y))., p(atom(z)).
    // Goal: :- p(cons(a, nil)).
    // Expected: Only first rule remains after head elim
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        const expr* cons_a_X = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), X})});
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), cons_a_X}), {}});  // idx 0
        
        const expr* cons_b_Y = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("b", {}), Y})});
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), cons_b_Y}), {}});  // idx 1
        
        const expr* atom_z = ep.functor("cons", {ep.functor("atom", {}), ep.functor("z", {})});
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), atom_z}), {}});  // idx 2
        
        goals goals;
        const expr* cons_a_nil = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("nil", {})})});
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), cons_a_nil}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution (head elim removed idx 1 and 2)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify resolution
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl0, 0);
        assert(simulation.rs.count(rl0) == 1);
        
        // CRITICAL: No decisions (unit prop)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
    }
    
    // Test 47: Empty body rule query (multiple facts)
    // Database: person(alice)., person(bob)., person(charlie).
    // Goal: :- person(Who).
    // Expected: Who binds to one of {alice, bob, charlie}
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("person", {}), ep.functor("alice", {})}), {}});    // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("person", {}), ep.functor("bob", {})}), {}});      // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("person", {}), ep.functor("charlie", {})}), {}});  // idx 2
        
        goals goals;
        const expr* Who = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("person", {}), Who}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force idx 1 (bob)
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_value = 10.0;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 0;  // Force idx 1
        root.m_children[gl0_for_mcts].m_children[size_t(2)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(2)].m_value = 10.0;
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify resolution (forced idx 1)
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(gl0, 1);
        assert(simulation.rs.count(rl1) == 1);
        assert(simulation.ds.count(rl1) == 1);  // Decision
        
        // CRITICAL: MCTS called once
        assert(sim.length() == 2);
        
        // CRITICAL: Who binds to exactly 'bob'
        normalizer norm(ep, bm);
        const expr* Who_normalized = norm(Who);
        assert(Who_normalized == ep.functor("bob", {}));
    }
    
    // Test 48: Avoidance store with variable resolution
    // Database: p(X) :- q(X)., p(Y) :- r(Y)., q(a)., r(b).
    // Goal: :- p(Z).
    // Avoidance: {rl(p(Z), idx 0)} (avoid p→q path)
    // Expected: Z=b (forced to use p→r path)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {ep.functor("cons", {ep.functor("q", {}), X})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), Y}), {ep.functor("cons", {ep.functor("r", {}), Y})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("b", {})}), {}});  // idx 3
        
        goals goals;
        const expr* Z = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), Z}));
        
        // Pre-populate avoidance store to avoid idx 0
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_avoid = lp.resolution(gl_p, 0);
        decision_store avoidance;
        avoidance.insert(rl_avoid);
        cdcl c;
        c.learn(lemma(avoidance));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (p→r, r)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify lineages (used idx 1, not idx 0)
        const resolution_lineage* rl_p = lp.resolution(gl_p, 1);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_r = lp.goal(rl_p, 0);
        const resolution_lineage* rl_r = lp.resolution(gl_r, 3);
        assert(simulation.rs.count(rl_r) == 1);
        
        // CRITICAL: No decisions (CDCL elim removed idx 0, leaving unit prop)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: MCTS never called
        assert(sim.length() == 0);
        
        // CRITICAL: Z binds to 'b'
        normalizer norm(ep, bm);
        const expr* Z_normalized = norm(Z);
        assert(Z_normalized == ep.functor("b", {}));
    }
    
    // Test 49: Existential variables (variables only in body)
    // Database: p(X) :- q(Y)., q(a).
    // Goal: :- p(hello).
    // Expected: Solution (X=hello, Y=a, Y is existential)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {ep.functor("cons", {ep.functor("q", {}), Y})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 1
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("hello", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify lineages
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        // CRITICAL: No decisions
        assert(simulation.ds.size() == 0);
        
        // NOTE: X and Y are database variables that get copied during resolution.
        // The COPIED versions bind correctly (X'=hello, Y'=a), but we can't check
        // the original database variables since they remain unbound.
    }
    
    // Test 50: Self-referential rule (immediate cycle)
    // Database: p(X) :- p(X).
    // Goal: :- p(a).
    // Expected: Max resolutions exceeded (infinite loop detection)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {ep.functor("cons", {ep.functor("p", {}), X})}});  // idx 0: p(X) :- p(X)
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{5, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});  // Low max_resolutions
        
        bool result = simulation();
        
        // CRITICAL: Max resolutions exceeded (not a solution)
        assert(result == false);
        
        // CRITICAL: Exactly 5 resolutions (hit the limit)
        assert(simulation.rs.size() == 5);
        
        // CRITICAL: All unit propagations (only one candidate)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Goal store not empty (still has unresolved goals)
        assert(simulation.gs.size() > 0);
    }
    
    // Test 51: Empty database
    // Database: (empty)
    // Goal: :- p(X).
    // Expected: Immediate conflict (no rules at all)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;  // Empty!
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict
        assert(result == false);
        
        // CRITICAL: No resolutions
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal still in store with no candidates
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.gs.members.count(gl_p) == 1);
        assert(simulation.cs.at(gl_p).empty());
    }
    
    // Test 52: Fact with variable in head
    // Database: p(X).
    // Goal: :- p(hello).
    // Expected: Solution (X unifies with hello)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {}});  // idx 0: p(X). (fact with variable)
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("hello", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify resolution
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        // CRITICAL: No decisions (unit prop)
        assert(simulation.ds.size() == 0);
        
        // NOTE: X is a database variable that gets copied during resolution.
        // The COPIED version X' binds to 'hello', but we can't check the
        // original database variable since it remains unbound.
    }
    
    // Test 53: Very wide goal (many body goals)
    // Database: big(W,X,Y,Z) :- a(W), b(X), c(Y), d(Z)., a(1)., b(2)., c(3)., d(4).
    // Goal: :- big(A,B,C,D).
    // Expected: A=1, B=2, C=3, D=4 (4 sub-goals spawned)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* W = ep.var(seq());
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        
        const expr* big_head = ep.functor("cons", {ep.functor("big", {}), ep.functor("cons", {W, ep.functor("cons", {X, ep.functor("cons", {Y, Z})})})});
        const expr* a_body = ep.functor("cons", {ep.functor("a", {}), W});
        const expr* b_body = ep.functor("cons", {ep.functor("b", {}), X});
        const expr* c_body = ep.functor("cons", {ep.functor("c", {}), Y});
        const expr* d_body = ep.functor("cons", {ep.functor("d", {}), Z});
        db.push_back(rule{big_head, {a_body, b_body, c_body, d_body}});  // idx 0
        
        db.push_back(rule{ep.functor("cons", {ep.functor("a", {}), ep.functor("1", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("b", {}), ep.functor("2", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("c", {}), ep.functor("3", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("d", {}), ep.functor("4", {})}), {}});  // idx 4
        
        goals goals;
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* D = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("big", {}), ep.functor("cons", {A, ep.functor("cons", {B, ep.functor("cons", {C, D})})})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 5 resolutions (big + 4 sub-goals)
        assert(simulation.rs.size() == 5);
        
        // CRITICAL: Verify lineages
        const goal_lineage* gl_big = lp.goal(nullptr, 0);
        const resolution_lineage* rl_big = lp.resolution(gl_big, 0);
        assert(simulation.rs.count(rl_big) == 1);
        
        const goal_lineage* gl_a = lp.goal(rl_big, 0);
        const resolution_lineage* rl_a = lp.resolution(gl_a, 1);
        assert(simulation.rs.count(rl_a) == 1);
        
        const goal_lineage* gl_b = lp.goal(rl_big, 1);
        const resolution_lineage* rl_b = lp.resolution(gl_b, 2);
        assert(simulation.rs.count(rl_b) == 1);
        
        const goal_lineage* gl_c = lp.goal(rl_big, 2);
        const resolution_lineage* rl_c = lp.resolution(gl_c, 3);
        assert(simulation.rs.count(rl_c) == 1);
        
        const goal_lineage* gl_d = lp.goal(rl_big, 3);
        const resolution_lineage* rl_d = lp.resolution(gl_d, 4);
        assert(simulation.rs.count(rl_d) == 1);
        
        // CRITICAL: No decisions (all unit props)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify all four variables
        normalizer norm(ep, bm);
        assert(norm(A) == ep.functor("1", {}));
        assert(norm(B) == ep.functor("2", {}));
        assert(norm(C) == ep.functor("3", {}));
        assert(norm(D) == ep.functor("4", {}));
    }
    
    // Test 54: Multiple avoidances in avoidance store
    // Database: p(X) :- q(X)., p(Y) :- r(Y)., q(a)., r(b).
    // Goal: :- p(Z).
    // Avoidances: {{rl1}, {rl2}} where both become singletons
    // Expected: Both get CDCL eliminated, leaving conflict
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X}), {ep.functor("cons", {ep.functor("q", {}), X})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), Y}), {ep.functor("cons", {ep.functor("r", {}), Y})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("b", {})}), {}});  // idx 3
        
        goals goals;
        const expr* Z = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), Z}));
        
        // Pre-populate avoidance store with BOTH resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl_p, 0);
        const resolution_lineage* rl1 = lp.resolution(gl_p, 1);
        
        decision_store avoidance1;
        avoidance1.insert(rl0);
        
        decision_store avoidance2;
        avoidance2.insert(rl1);
        
        cdcl c;
        c.learn(lemma(avoidance1));
        c.learn(lemma(avoidance2));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Conflict (both candidates eliminated)
        assert(result == false);
        
        // CRITICAL: No resolutions made
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal still in store with no candidates
        assert(simulation.gs.members.count(gl_p) == 1);
        assert(simulation.cs.at(gl_p).empty());  // Both eliminated by CDCL
    }
    
    // Test 55: Head elimination removes all candidates
    // Database: p(cons(a,X))., p(cons(b,Y)).
    // Goal: :- p(atom(z)).
    // Expected: Immediate conflict (head elim removes both)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        const expr* cons_a_X = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), X})});
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), cons_a_X}), {}});  // idx 0
        
        const expr* cons_b_Y = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("b", {}), Y})});
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), cons_b_Y}), {}});  // idx 1
        
        goals goals;
        const expr* atom_z = ep.functor("cons", {ep.functor("atom", {}), ep.functor("z", {})});
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), atom_z}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict (head elim removes all)
        assert(result == false);
        
        // CRITICAL: No resolutions
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal in store with no candidates
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.gs.members.count(gl_p) == 1);
        assert(simulation.cs.at(gl_p).empty());
    }
    
    // Test 56: Deeply nested compound terms
    // Database: deep(cons(cons(cons(cons(cons(a,b),c),d),e),f)).
    // Goal: :- deep(X).
    // Expected: X binds to the deeply nested structure
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* level1 = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})});
        const expr* level2 = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {level1, ep.functor("c", {})})});
        const expr* level3 = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {level2, ep.functor("d", {})})});
        const expr* level4 = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {level3, ep.functor("e", {})})});
        const expr* level5 = ep.functor("cons", {ep.functor("cons", {}), ep.functor("cons", {level4, ep.functor("f", {})})});
        db.push_back(rule{ep.functor("cons", {ep.functor("deep", {}), level5}), {}});  // idx 0
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("deep", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: X binds to deeply nested structure
        normalizer norm(ep, bm);
        assert(norm(X) == level5);
    }
    
    // Test 57: Max resolutions hit during variable binding
    // Database: 10 chained rules
    // Goal: :- a(X).
    // Max resolutions: 5
    // Expected: Returns false (max exceeded)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* vars[10];
        for (int i = 0; i < 10; i++) {
            vars[i] = ep.var(seq());
        }
        
        // Build chain: a→b→c→d→e→f→g→h→i→j→end
        db.push_back(rule{ep.functor("cons", {ep.functor("a", {}), vars[0]}), {ep.functor("cons", {ep.functor("b", {}), vars[0]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("b", {}), vars[1]}), {ep.functor("cons", {ep.functor("c", {}), vars[1]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("c", {}), vars[2]}), {ep.functor("cons", {ep.functor("d", {}), vars[2]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("d", {}), vars[3]}), {ep.functor("cons", {ep.functor("e", {}), vars[3]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("e", {}), vars[4]}), {ep.functor("cons", {ep.functor("f", {}), vars[4]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("f", {}), vars[5]}), {ep.functor("cons", {ep.functor("g", {}), vars[5]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("g", {}), vars[6]}), {ep.functor("cons", {ep.functor("h", {}), vars[6]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("h", {}), vars[7]}), {ep.functor("cons", {ep.functor("i", {}), vars[7]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("i", {}), vars[8]}), {ep.functor("cons", {ep.functor("j", {}), vars[8]})}});
        db.push_back(rule{ep.functor("cons", {ep.functor("j", {}), ep.functor("end", {})}), {}});
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("a", {}), X}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{5, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});  // Max 5
        
        bool result = simulation();
        
        // CRITICAL: Max exceeded (not solution)
        assert(result == false);
        
        // CRITICAL: Exactly 5 resolutions
        assert(simulation.rs.size() == 5);
        
        // CRITICAL: Goal store not empty
        assert(simulation.gs.size() > 0);
    }
    
    // Test 58: Decision creates conflict immediately
    // Database: p(a) :- q(b)., q(c).
    // Goal: :- p(a).
    // Expected: Conflict (q(b) has no candidates)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}), {ep.functor("cons", {ep.functor("q", {}), ep.functor("b", {})})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("c", {})}), {}});  // idx 1: q(c) not q(b)!
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("a", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Conflict after first resolution
        assert(result == false);
        
        // CRITICAL: Exactly 1 resolution (p→q(b))
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Verify resolution
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_p = lp.resolution(gl_p, 0);
        assert(simulation.rs.count(rl_p) == 1);
        
        // CRITICAL: q(b) has no candidates
        const goal_lineage* gl_q = lp.goal(rl_p, 0);
        assert(simulation.gs.members.count(gl_q) == 1);
        assert(simulation.cs.at(gl_q).empty());
    }
    
    // Test 59: Variables in goal match multiple rules (ambiguity)
    // Database: edge(a,b)., edge(a,c)., edge(a,d).
    // Goal: :- edge(a,X).
    // Expected: X could be b, c, or d (MCTS decides, force d)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("edge", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("b", {})})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("edge", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("c", {})})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("edge", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("d", {})})}), {}});  // idx 2
        
        goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("edge", {}), ep.functor("cons", {ep.functor("a", {}), X})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        // Pre-populate MCTS to force idx 2
        const goal_lineage* gl0_for_mcts = lp.goal(nullptr, 0);
        root.m_visits = 100;
        root.m_children[gl0_for_mcts].m_visits = 50;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(0)].m_value = 10.0;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_visits = 10;
        root.m_children[gl0_for_mcts].m_children[size_t(1)].m_value = 10.0;
        root.m_children[gl0_for_mcts].m_children[size_t(2)].m_visits = 0;  // Force idx 2
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution (forced idx 2)
        assert(simulation.rs.size() == 1);
        
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl2 = lp.resolution(gl0, 2);
        assert(simulation.rs.count(rl2) == 1);
        assert(simulation.ds.count(rl2) == 1);  // Decision
        
        // CRITICAL: X binds to 'd'
        normalizer norm(ep, bm);
        assert(norm(X) == ep.functor("d", {}));
    }
    
    // Test 60: Unification failure due to nested structure mismatch
    // Database: p(cons(a,cons(b,nil))).
    // Goal: :- p(cons(a,cons(c,nil))).
    // Expected: Conflict (b ≠ c in nested position)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* nested_db = ep.functor("cons", {ep.functor("cons", {}), 
                                   ep.functor("cons", {ep.functor("a", {}), 
                                      ep.functor("cons", {ep.functor("cons", {}), 
                                         ep.functor("cons", {ep.functor("b", {}), ep.functor("nil", {})})})})});
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), nested_db}), {}});  // idx 0
        
        goals goals;
        const expr* nested_goal = ep.functor("cons", {ep.functor("cons", {}), 
                                     ep.functor("cons", {ep.functor("a", {}), 
                                        ep.functor("cons", {ep.functor("cons", {}), 
                                           ep.functor("cons", {ep.functor("c", {}), ep.functor("nil", {})})})})});
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), nested_goal}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict (head elim fails)
        assert(result == false);
        
        // CRITICAL: No resolutions
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal has no candidates
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.cs.at(gl_p).empty());
    }
    
    // Test 61: Variable occurs in multiple positions in same goal
    // Database: weird(X,X,X).
    // Goal: :- weird(a,Y,a).
    // Expected: Y=a (all three positions must unify)
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        const expr* weird = ep.functor("cons", {ep.functor("weird", {}), ep.functor("cons", {X, ep.functor("cons", {X, X})})});
        db.push_back(rule{weird, {}});  // idx 0: weird(X,X,X)
        
        goals goals;
        const expr* Y = ep.var(seq());
        const expr* goal = ep.functor("cons", {ep.functor("weird", {}), ep.functor("cons", {ep.functor("a", {}), ep.functor("cons", {Y, ep.functor("a", {})})})});
        goals.push_back(goal);
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Y binds to 'a'
        normalizer norm(ep, bm);
        assert(norm(Y) == ep.functor("a", {}));
    }
    
    // Test 62: CDCL elimination chain reaction
    // Pre-populate avoidance to create cascade of eliminations
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X1 = ep.var(seq());
        const expr* X2 = ep.var(seq());
        const expr* X3 = ep.var(seq());
        
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X1}), {ep.functor("cons", {ep.functor("q", {}), X1})}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X2}), {ep.functor("cons", {ep.functor("r", {}), X2})}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("p", {}), X3}), {ep.functor("cons", {ep.functor("s", {}), X3})}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("r", {}), ep.functor("b", {})}), {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("s", {}), ep.functor("c", {})}), {}});  // idx 5
        
        goals goals;
        const expr* Z = ep.var(seq());
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), Z}));
        
        // Pre-populate avoidance to eliminate idx 0 and 1
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl_p, 0);
        const resolution_lineage* rl1 = lp.resolution(gl_p, 1);
        
        decision_store av1;
        av1.insert(rl0);
        decision_store av2;
        av2.insert(rl1);
        
        cdcl c;
        c.learn(lemma(av1));
        c.learn(lemma(av2));
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found (only idx 2 remains)
        assert(result == true);
        
        // CRITICAL: 2 resolutions (p→s, s)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Z binds to 'c'
        normalizer norm(ep, bm);
        assert(norm(Z) == ep.functor("c", {}));
    }
    
    // Test 63: Unbound variables in normalizer
    // Goal with variable that never binds
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        db.push_back(rule{ep.functor("p", {}), {}});  // idx 0: p. (no variables)
        
        goals goals;
        goals.push_back(ep.functor("p", {}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        resolution_store rs;
        decision_store ds;
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // Create an unbound variable AFTER solving
        const expr* unbound_var = ep.var(seq());
        
        // CRITICAL: Normalizer returns variable itself (identity)
        normalizer norm(ep, bm);
        assert(norm(unbound_var) == unbound_var);
    }
    
    // Test 64: Rule whose head IS a variable
    // Database: X :- q(a).
    // Goal: :- p(hello).
    // Expected: Solution (X unifies with p(hello))
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        database db;
        const expr* X = ep.var(seq());
        db.push_back(rule{X, {ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})})}});  // idx 0: X :- q(a).
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}), {}});  // idx 1: q(a).
        
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("p", {}), ep.functor("hello", {})}));
        
        cdcl c;
        
        monte_carlo::tree_node<mcts_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        ridge_sim simulation(sim_args{100, db, goals, t, seq, ep, bm, lp, c}, mcts_sim_args{sim});
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: 2 resolutions
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Verify lineages
        const goal_lineage* gl_goal = lp.goal(nullptr, 0);
        const resolution_lineage* rl_goal = lp.resolution(gl_goal, 0);
        assert(simulation.rs.count(rl_goal) == 1);
        
        const goal_lineage* gl_q = lp.goal(rl_goal, 0);
        const resolution_lineage* rl_q = lp.resolution(gl_q, 1);
        assert(simulation.rs.count(rl_q) == 1);
        
        // NOTE: X is a database variable (used as rule head!) that gets copied
        // during resolution. The copied version X' binds to p(hello), but we
        // can't check the original database variable.
    }
}

void test_ridge_constructor_and_destructor() {
    // Test 1: Basic construction - verify trail frame pushed and all fields stored correctly
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        goals goals;
        std::mt19937 rng(42);

        size_t depth_before = t.depth();
        assert(depth_before == 1);

        {
            ridge solver(solver_args{db, goals, t, seq, bm, 100}, mcts_solver_args{1.414, rng});

            // CRITICAL: Constructor pushes one trail frame
            assert(t.depth() == depth_before + 1);

            // CRITICAL: References stored correctly
            assert(&solver.db == &db);
            assert(&solver.gl == &goals);
            assert(&solver.t == &t);
            assert(&solver.vars == &seq);
            assert(&solver.bm == &bm);
            assert(&solver.rng == &rng);

            // CRITICAL: Scalar fields stored correctly
            assert(solver.max_resolutions == 100);
            assert(solver.exploration_constant == 1.414);

            // CRITICAL: Avoidance store initialized empty
            assert(solver.c.avoidances.empty());
        }

        // CRITICAL: Destructor restores trail depth
        assert(t.depth() == depth_before);
    }

    // Test 2: Construction on a fresh trail (depth 0 → 1 → 0)
    {
        trail t;

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        goals goals;
        std::mt19937 rng(0);

        assert(t.depth() == 0);

        {
            ridge solver(solver_args{db, goals, t, seq, bm, 1}, mcts_solver_args{0.0, rng});

            // CRITICAL: Depth goes 0 → 1
            assert(t.depth() == 1);
            assert(solver.max_resolutions == 1);
            assert(solver.exploration_constant == 0.0);
        }

        // CRITICAL: Depth returns to 0
        assert(t.depth() == 0);
    }

    // Test 3: Constructor only adds a frame boundary, no undo actions
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        goals goals;
        std::mt19937 rng(42);

        size_t undo_size_before = t.undo_stack.size();
        size_t boundary_size_before = t.frame_boundary_stack.size();

        {
            ridge solver(solver_args{db, goals, t, seq, bm, 100}, mcts_solver_args{1.414, rng});

            // CRITICAL: Only a frame boundary is pushed, not any undo action
            assert(t.undo_stack.size() == undo_size_before);
            assert(t.frame_boundary_stack.size() == boundary_size_before + 1);
        }

        // CRITICAL: Destructor removes the frame boundary; undo stack is unchanged
        assert(t.frame_boundary_stack.size() == boundary_size_before);
        assert(t.undo_stack.size() == undo_size_before);
    }

    // Test 4: Destructor rolls back undo actions logged within the a01 frame
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        goals goals;
        std::mt19937 rng(42);

        bool undone = false;

        {
            ridge solver(solver_args{db, goals, t, seq, bm, 100}, mcts_solver_args{1.414, rng});

            // Log an undo action into the a01's frame
            t.log([&undone]() { undone = true; });

            assert(!undone);
        }

        // CRITICAL: Destructor popped the a01 frame, triggering the logged undo action
        assert(undone);
    }

    // Test 5: Destructor only pops the a01's frame, not the caller's frame
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        goals goals;
        std::mt19937 rng(42);

        bool caller_undone = false;
        t.log([&caller_undone]() { caller_undone = true; });

        {
            ridge solver(solver_args{db, goals, t, seq, bm, 100}, mcts_solver_args{1.414, rng});

            // a01's frame is stacked on top of the caller's frame
            assert(t.depth() == 2);
        }

        // CRITICAL: Only the a01's frame was popped; caller's undo action not triggered
        assert(!caller_undone);
        assert(t.depth() == 1);
    }

    // Test 6: Non-empty db and goals - verify references and contents accessible via solver
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("p", {}), {}});
        db.push_back(rule{ep.functor("cons", {ep.functor("q", {}), ep.var(seq())}), {ep.functor("p", {})}});

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("q", {}), ep.functor("a", {})}));

        std::mt19937 rng(999);

        size_t depth_before = t.depth();

        {
            ridge solver(solver_args{db, goals, t, seq, bm, 50}, mcts_solver_args{1.0, rng});

            // CRITICAL: Frame pushed
            assert(t.depth() == depth_before + 1);

            // CRITICAL: db reference correct; content accessible through it
            assert(&solver.db == &db);
            assert(solver.db.size() == 2);

            // CRITICAL: goals reference correct; content accessible through it
            assert(&solver.gl == &goals);
            assert(solver.gl.size() == 1);

            // CRITICAL: Avoidance store empty on construction regardless of db/goals content
            assert(solver.c.avoidances.empty());
        }

        // CRITICAL: Destructor restores depth
        assert(t.depth() == depth_before);
    }
}

void test_ridge() {

    // Test 1: Budget = 0 — no iterations execute; operator() returns true with nullopt.
    // The solver makes no progress at all, so it cannot prove refutation or find a
    // solution. This validates the edge case of calling with zero budget.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("a", {}), {}});

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        soln = std::nullopt;
        bool result = true; // (0 iterations: no sim run)

        // CRITICAL: returns true (no refutation proved) and soln defaults to nullopt
        assert(result == true);
        assert(!soln.has_value());
        // The avoidance store is empty — no iterations ran, no avoidances recorded
        assert(solver.c.avoidances.empty());
    }

    // Test 2: Simple ground solution — DB: a.  Goal: a
    // The single goal is unit-propagated immediately. The resolution store contains
    // exactly one lineage entry: the unit-prop of goal 0 via rule 0.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("a", {}), {}});  // idx 0: a.

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        bool result = solver(soln);

        assert(result == true);
        assert(soln.has_value());

        // CRITICAL: exactly one resolution (unit-prop of a with rule 0)
        assert(soln.value().size() == 1);
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        assert(soln.value().count(rl0) == 1);

        // CRITICAL: avoidance store has one entry — the empty decision set from the
        // unit-prop solution (no MCTS decision was needed)
        assert(solver.c.avoidances.size() == 1);
    }

    // Test 3: Variable binding verified via normalizer
    // DB: answer(42).   Goal: answer(X)
    // The copier creates a fresh copy of the ground rule (no vars to freshen), then
    // the goal-resolver unifies cons("answer","42") with cons("answer",X), binding X.
    // After operator() returns, bm holds X → atom("42"), which the normalizer resolves.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        // idx 0: answer(42).
        db.push_back(rule{ep.functor("cons", {ep.functor("answer", {}), ep.functor("42", {})}), {}});

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("answer", {}), X}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        bool result = solver(soln);

        assert(result == true);
        assert(soln.has_value());

        // CRITICAL: one resolution, binding X to "42"
        assert(soln.value().size() == 1);
        assert(soln.value().count(solver.lp.resolution(solver.lp.goal(nullptr, 0), 0)) == 1);

        // CRITICAL: normalizer follows bm chain and returns atom("42")
        normalizer norm(ep, bm);
        const expr* X_val = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val->content));
        assert(std::get<expr::functor>(X_val->content).name == "42");
    }

    // Test 4: Immediate refutation — empty database, goal has no candidates.
    // head-elimination fires before any MCTS decision: conflict with ds = {} on
    // the very first sim_one. next_avoidance returns false → operator() returns false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;  // intentionally empty

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;
        bool result;
        while ((result = solver(soln)) && !soln.has_value()) {}

        assert(result == false);
        assert(!soln.has_value());
    }

    // Test 5: 2-call CDCL-driven refutation — unsatisfiable, 2-path problem.
    // DB: a :- b.  (idx 0)   a :- c.  (idx 1)   (no rules for b or c)
    //
    // Call 1: MCTS picks one of {rule0, rule1} for goal a. Whichever is chosen, the
    // resulting sub-goal (b or c) has no candidates → conflict. ds = {rl_chosen}.
    // After 1000 inner iterations the minimum conflict is size 1. Avoidance recorded.
    //
    // Call 2: CDCL eliminates the avoided rule. The remaining rule is unit-propagated.
    // The new sub-goal (c or b) again has no candidates → conflict with ds = {} (the
    // unit-prop added no decision) → next_avoidance returns false → operator() false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("a", {}), {ep.functor("b", {})}});  // idx 0: a :- b.
        db.push_back(rule{ep.functor("a", {}), {ep.functor("c", {})}});  // idx 1: a :- c.

        goals goals;
        goals.push_back(ep.functor("a", {}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        std::optional<resolution_store> soln;

        // Call 1: one depth-1 avoidance recorded
        bool result1 = solver(soln);
        assert(result1 == true);
        assert(!soln.has_value());
        assert(solver.c.avoidances.size() == 1);

        // Call 2: CDCL + unit-prop leads to conflict with empty ds → refutation
        bool result2 = solver(soln);
        assert(result2 == false);
        assert(!soln.has_value());
    }

    // Test 6: Unique-solution conjunction query — is_a(X) ∧ is_b(X)
    // DB:
    //   idx 0: is_a(1).   idx 1: is_a(2).
    //   idx 2: is_b(2).   idx 3: is_b(3).
    //
    // The only satisfying X is 2. Each of the 4 MCTS choices leads to either
    // a solution (X=2) or immediate conflict (X=1 kills is_b; X=3 kills is_a).
    //
    // Call 1: MCTS finds X=2 via one decision (size-1 avoidance recorded).
    //         Normalizer verifies X → "2". Lineage is deterministic:
    //         soln always = {rl(gl_a,1), rl(gl_b,2)}, regardless of which goal MCTS
    //         decided first — both rules apply to X=2, the other is unit-propagated.
    //
    // Call 2: CDCL eliminates the solution decision (rule idx for X=2). The only
    //         remaining candidate for that goal resolves to X=1 or X=3, which
    //         immediately conflicts with the other goal (0 candidates, ds={}) →
    //         next_avoidance returns false → operator() returns false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("is_a", {}), ep.functor("1", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("is_a", {}), ep.functor("2", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("is_b", {}), ep.functor("2", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("is_b", {}), ep.functor("3", {})}), {}});  // idx 3

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("is_a", {}), X}));  // goal 0: is_a(X)
        goals.push_back(ep.functor("cons", {ep.functor("is_b", {}), X}));  // goal 1: is_b(X)

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        // Call 1: unique solution X=2
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());

        // CRITICAL: bm binds X to "2"
        const expr* X_val = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val->content));
        assert(std::get<expr::functor>(X_val->content).name == "2");

        // CRITICAL: soln contains exactly the two resolutions for X=2 — one per goal.
        // Regardless of which goal MCTS decided first, both rl(gl_a,1) and rl(gl_b,2)
        // always appear (one as the decision, the other as unit-propagation).
        assert(soln.value().size() == 2);
        const goal_lineage* gl_a = solver.lp.goal(nullptr, 0);
        const goal_lineage* gl_b = solver.lp.goal(nullptr, 1);
        const resolution_lineage* rl_a1 = solver.lp.resolution(gl_a, 1);  // is_a(2)
        const resolution_lineage* rl_b2 = solver.lp.resolution(gl_b, 2);  // is_b(2)
        assert(soln.value().count(rl_a1) == 1);
        assert(soln.value().count(rl_b2) == 1);

        // Call 2: the solution path is blocked; the only remaining path conflicts → refutation
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == false);
        assert(!soln.has_value());
    }

    // Test 7: Multi-solution enumeration — all parents of alice
    // DB:
    //   idx 0: parent(bob,   alice).
    //   idx 1: parent(carol, alice).
    //   idx 2: parent(dave,  bob).     ← head-elim removes this (alice ≠ bob)
    //
    // Goal: parent(X, alice)  — head-elim leaves only rules 0 and 1.
    // MCTS must decide between bob and carol. Each is a depth-1 solution.
    //
    // Call 1 finds one parent; Call 2 finds the other (via CDCL elimination of the
    // first decision, leaving a unit-prop that resolves to the remaining parent).
    // bm binds X differently after each call; normalizer verifies the two names differ.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        // parent(A, B) encoded as cons(cons(atom("parent"), A), B)
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("bob", {})}), ep.functor("alice", {})}), {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("carol", {})}), ep.functor("alice", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), ep.functor("dave", {})}), ep.functor("bob", {})}),  {}});   // idx 2

        const expr* X = ep.var(seq());
        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("parent", {}), X}), ep.functor("alice", {})}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        // First parent of alice
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        const expr* X_val1 = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val1->content));
        std::string parent1 = std::get<expr::functor>(X_val1->content).name;
        assert(parent1 == "bob" || parent1 == "carol");

        // Second parent of alice — sim_one rolls back bm then unit-props the other rule
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        const expr* X_val2 = norm(X);
        assert(std::holds_alternative<expr::functor>(X_val2->content));
        std::string parent2 = std::get<expr::functor>(X_val2->content).name;
        assert(parent2 == "bob" || parent2 == "carol");

        // CRITICAL: the two solutions bind X to different names
        assert(parent1 != parent2);

        // Test for refutation
        bool result3;
        while ((result3 = solver(soln)) && !soln.has_value()) {}
        assert(result3 == false);
    }

    // Test 8: Boolean SAT — formula (P ∨ Q) ∧ (¬P ∨ Q)
    //
    // This 2-clause formula is equivalent to Q. The satisfying assignments are:
    //   Solution A: P = true,  Q = true,  NP = false
    //   Solution B: P = false, Q = true,  NP = true
    //
    // Encoding (all predicates as right-nested cons cells):
    //   bool(X)    → cons(atom("bool"), X)
    //   not(X, Y)  → cons(cons(atom("not"), X), Y)
    //   or(X, Y, Z)→ cons(cons(cons(atom("or"), X), Y), Z)
    //
    // DB: bool(true) idx0, bool(false) idx1,
    //     not(true,false) idx2, not(false,true) idx3,
    //     or(true,true,true) idx4, or(true,false,true) idx5,
    //     or(false,true,true) idx6, or(false,false,false) idx7.
    //
    // Goals: bool(P), bool(Q), or(P,Q,true), not(P,NP), or(NP,Q,true).
    //
    // With 1000 inner iterations MCTS learns to decide bool(P) first (1-decision
    // solutions, negative reward -1), rather than bool(Q) (requires 2+ decisions).
    // Deciding P=true propagates: NP=false, or(false,Q,true)→Q=true, bool(Q)→true,
    //   or(true,true,true). Deciding P=false propagates analogously.
    //
    // Both calls return with Q=true and opposite values for P.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("true", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("false", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), ep.functor("true", {})}), ep.functor("false", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), ep.functor("false", {})}), ep.functor("true", {})}),  {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("true", {})}), ep.functor("true", {})}), ep.functor("true", {})}),  {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("true", {})}), ep.functor("false", {})}), ep.functor("true", {})}),  {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("false", {})}), ep.functor("true", {})}), ep.functor("true", {})}),  {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("false", {})}), ep.functor("false", {})}), ep.functor("false", {})}), {}});  // idx 7

        const expr* P  = ep.var(seq());
        const expr* Q  = ep.var(seq());
        const expr* NP = ep.var(seq());

        goals goals;
        // goal 0: bool(P)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), P}));
        // goal 1: bool(Q)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), Q}));
        // goal 2: or(P, Q, true) — P ∨ Q = true
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), P}), Q}), ep.functor("true", {})}));
        // goal 3: not(P, NP) — compute ¬P
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), P}), NP}));
        // goal 4: or(NP, Q, true) — ¬P ∨ Q = true
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), NP}), Q}), ep.functor("true", {})}));

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        // First solution: P=true or P=false, but Q must be true
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());

        const expr* Q_val1 = norm(Q);
        const expr* P_val1 = norm(P);
        assert(std::holds_alternative<expr::functor>(Q_val1->content));
        assert(std::holds_alternative<expr::functor>(P_val1->content));

        // CRITICAL: Q = true in every solution of (P∨Q)∧(¬P∨Q)
        assert(std::get<expr::functor>(Q_val1->content).name == "true");
        std::string P_str1 = std::get<expr::functor>(P_val1->content).name;
        assert(P_str1 == "true" || P_str1 == "false");

        // Second solution: CDCL eliminates first P choice; the other propagates
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == true);
        assert(soln.has_value());

        const expr* Q_val2 = norm(Q);
        const expr* P_val2 = norm(P);
        assert(std::holds_alternative<expr::functor>(Q_val2->content));
        assert(std::holds_alternative<expr::functor>(P_val2->content));

        // CRITICAL: Q still true after finding the second solution
        assert(std::get<expr::functor>(Q_val2->content).name == "true");
        std::string P_str2 = std::get<expr::functor>(P_val2->content).name;

        // CRITICAL: the two solutions assign opposite values to P
        assert(P_str2 != P_str1);

        // Test for refutation
        bool result3;
        while ((result3 = solver(soln)) && !soln.has_value()) {}
        assert(result3 == false);
    }

    // Test 9: Graph 2-coloring synthesis — find valid 2-colorings of a 3-node path A-B-C
    //
    // DB:
    //   idx 0: color(red).     idx 1: color(blue).
    //   idx 2: diff(red, blue).  idx 3: diff(blue, red).
    //
    // Goals:
    //   goal 0: color(A)   goal 1: color(B)   goal 2: color(C)
    //   goal 3: diff(A, B)  goal 4: diff(B, C)
    //
    // A single MCTS decision (e.g. deciding diff(A,B)→red-blue) immediately binds
    // A and B, unit-propagates color(A), color(B), diff(B,C), and color(C).
    // This produces exactly two valid 2-colorings:
    //   Coloring 1: A=red,  B=blue, C=red
    //   Coloring 2: A=blue, B=red,  C=blue
    //
    // Both calls return valid alternating colorings that satisfy:
    //   A ≠ B,  B ≠ C,  A = C  (necessary for a 2-colored path of 3 nodes).
    // The two solutions bind A differently, confirming both colorings are enumerated.
    // All 5 goals appear in the solution's resolution store.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("red", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("blue", {})}), {}});  // idx 1
        // diff(X, Y) encoded as cons(cons(atom("diff"), X), Y)
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("blue", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("red", {})}),  {}});  // idx 3

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), A}));                 // goal 0: color(A)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), B}));                 // goal 1: color(B)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), C}));                 // goal 2: color(C)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), B}));      // goal 3: diff(A, B)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), B}), C}));      // goal 4: diff(B, C)

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);
        std::optional<resolution_store> soln;

        auto is_valid_color = [](const std::string& s) {
            return s == "red" || s == "blue";
        };

        // First valid 2-coloring of the path A-B-C
        bool result1;
        while ((result1 = solver(soln)) && !soln.has_value()) {}
        assert(result1 == true);
        assert(soln.has_value());

        // All 5 goals are resolved in every solution
        assert(soln.value().size() == 5);

        std::string A1 = std::get<expr::functor>(norm(A)->content).name;
        std::string B1 = std::get<expr::functor>(norm(B)->content).name;
        std::string C1 = std::get<expr::functor>(norm(C)->content).name;

        assert(is_valid_color(A1) && is_valid_color(B1) && is_valid_color(C1));
        // CRITICAL: adjacent nodes have different colors
        assert(A1 != B1);
        assert(B1 != C1);
        // CRITICAL: the path is symmetrically 2-colored — endpoints share a color
        assert(A1 == C1);

        // Second valid 2-coloring — bm is refreshed by the next sim_one's trail pop
        bool result2;
        while ((result2 = solver(soln)) && !soln.has_value()) {}
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 5);

        std::string A2 = std::get<expr::functor>(norm(A)->content).name;
        std::string B2 = std::get<expr::functor>(norm(B)->content).name;
        std::string C2 = std::get<expr::functor>(norm(C)->content).name;

        assert(is_valid_color(A2) && is_valid_color(B2) && is_valid_color(C2));
        assert(A2 != B2);
        assert(B2 != C2);
        assert(A2 == C2);

        // CRITICAL: the second coloring is the complement of the first
        assert(A2 != A1);

        // Test for refutation
        bool result3;
        while ((result3 = solver(soln)) && !soln.has_value()) {}
        assert(result3 == false);
    }

    // =========================================================================
    // Structured multi-solution enumeration helpers (Tests 10–19)
    // =========================================================================

    // solution = ordered vector of normalised const expr* values for the variables
    // of interest. Comparison uses pointer identity, which is correct because
    // expr_pool interns: same content → same pointer within the same pool.
    using solution = std::vector<const expr*>;

    // ridge() pushes a trail frame; sim_one() starts with t.pop() for that frame, undoing
    // expr_pool inserts logged after the push. Build every std::set<solution> expected (and
    // any ep.atom / peano / ep.import used only for it) *before* constructing ridge.

    // Enumerate all expected solutions in any order, skipping calls that return
    // the same variable bindings via a different resolution path (valid behaviour),
    // then assert that the solver refutes when the search space is exhausted.
    auto next_until_refuted = [](
        ridge& solver,
        std::set<solution> expected,
        auto get_solution
    ) {
        const std::map<uint32_t, std::string> no_names;
        std::set<solution> visited;
        std::optional<resolution_store> soln;
        while (!expected.empty()) {
            solution s;
            do {
                bool r; while ((r = solver(soln)) && !soln.has_value()) {}
                assert(r == true);
                s = get_solution();
            } while (visited.count(s));
            std::cout << "Solution: " << visited.size() << " resolutions: " << soln.value().size() << std::endl;
            expr_printer printer(std::cout, no_names);
            for (const auto& e : s) {
                printer(e);
                std::cout << std::endl;
            }
            std::cout << std::endl;
            assert(expected.count(s) == 1);
            expected.erase(s);
            visited.insert(s);
        }
        // All solutions found — next call must refute
        bool r; while ((r = solver(soln)) && !soln.has_value()) {}
        assert(r == false);
        assert(!soln.has_value());
    };

    // Test 10: 3-colouring of K3 (the triangle) with colours {red, green, blue}
    //
    // All 3 nodes A, B, C are mutually adjacent, so every valid colouring assigns
    // a distinct colour to each node.  Exactly 3! = 6 proper 3-colourings exist.
    //
    // DB:
    //   idx 0-2: color(red), color(green), color(blue).
    //   idx 3-8: diff(X,Y) for every ordered pair of distinct colours (6 facts).
    //
    // Goals: color(A), color(B), color(C), diff(A,B), diff(A,C), diff(B,C).
    //
    // One MCTS decision (e.g. diff(A,B)→red-green) propagates A and B immediately.
    // diff(A,C) and diff(B,C) then narrow C to the unique remaining colour, which
    // unit-propagates everything else.  Multiple resolution orderings reach the same
    // (A,B,C) binding; the visited loop deduplicates those paths.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("red", {})}),   {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("green", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("blue", {})}),  {}});  // idx 2
        // diff(X, Y) = cons(cons(atom("diff"), X), Y) — all 6 asymmetric pairs
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("green", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("blue", {})}),  {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("red", {})}),   {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("blue", {})}),  {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("red", {})}),   {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("green", {})}), {}});  // idx 8

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), A}));                 // goal 0: color(A)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), B}));                 // goal 1: color(B)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), C}));                 // goal 2: color(C)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), B}));      // goal 3: diff(A,B)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), C}));      // goal 4: diff(A,C)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), B}), C}));      // goal 5: diff(B,C)

        // Interned atoms from ep — same pointer as those embedded in the rules
        const expr* red   = ep.functor("red", {});
        const expr* green = ep.functor("green", {});
        const expr* blue  = ep.functor("blue", {});

        // Every permutation of {red, green, blue} assigned to {A, B, C}
        std::set<solution> expected = {
            {red,   green, blue },
            {red,   blue,  green},
            {green, red,   blue },
            {green, blue,  red  },
            {blue,  red,   green},
            {blue,  green, red  },
        };

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(A)), ep.import(norm(B)), ep.import(norm(C))};
        });
    }

    // Test 11: SAT — P ∧ (Q ∨ R) using relational OR/AND encoding
    //
    // The OR and AND predicates are encoded relationally with a bool/1 body goal
    // that constrains the free argument to the boolean domain:
    //
    //   or(true,  X, true) :- bool(X).   — true ∨ anything = true
    //   or(false, X, X)    :- bool(X).   — false ∨ X = X
    //   and(true,  X, X)   :- bool(X).   — true ∧ X = X
    //   and(false, X, false):- bool(X).  — false ∧ anything = false
    //
    // Formula: P ∧ (Q ∨ R)
    // Goals: bool(P), bool(Q), bool(R), or(Q, R, QR), and(P, QR, true)
    //
    // Propagation chain (derived):
    //   and(P, QR, true): head-elim removes and(false,X,false) (result false≠true)
    //   → unit-prop via and(true,X,X): P=true, QR=true.
    //   or(Q, R, true):
    //     or(true,X,true)  → Q=true,  R free, adds bool(R) subgoal  (2 solutions)
    //     or(false,X,X)    → Q=false, X=R=true, adds bool(true) subgoal (1 solution)
    //
    // Solutions: (P=T,Q=T,R=T), (P=T,Q=T,R=F), (P=T,Q=F,R=T).
    // Note: different resolution orderings of the two bool(R) goals (the initial one
    // and the subgoal from the or rule) reach the same binding — the visited loop
    // deduplicates these identical-binding, distinct-path solutions.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("true", {})}),  {}});  // idx 0: bool(true).
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("false", {})}), {}});  // idx 1: bool(false).

        // or(true, X, true) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("true", {})}), X}), ep.functor("true", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 2
        }
        // or(false, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("false", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 3
        }
        // and(true, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("true", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 4
        }
        // and(false, X, false) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("false", {})}), X}), ep.functor("false", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 5
        }

        const expr* P  = ep.var(seq());
        const expr* Q  = ep.var(seq());
        const expr* R  = ep.var(seq());
        const expr* QR = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), P}));                                           // goal 0: bool(P)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), Q}));                                           // goal 1: bool(Q)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), R}));                                           // goal 2: bool(R)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), Q}), R}), QR}));                 // goal 3: or(Q,R,QR)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), P}), QR}), ep.functor("true", {})}));    // goal 4: and(P,QR,true)

        const expr* T_ = ep.functor("true", {});
        const expr* F_ = ep.functor("false", {});

        std::set<solution> expected = {
            {T_, T_, T_},   // P=T, Q=T, R=T
            {T_, T_, F_},   // P=T, Q=T, R=F
            {T_, F_, T_},   // P=T, Q=F, R=T
        };

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(P)), ep.import(norm(Q)), ep.import(norm(R))};
        });
    }

    // Test 12: 3-colouring of "K3 + tail" — 4 nodes, 8 goals, 12 solutions
    //
    // Graph: nodes A, B, C, D.
    //   Edges: A-B, A-C, B-C  (triangle — A, B, C all-different)
    //          A-D             (tail — D only constrained to differ from A)
    //
    // Colours: red, green, blue.
    //
    // Each of the 6 K3 colourings of (A,B,C) combines with 2 choices for D
    // (any colour ≠ A), giving 6 × 2 = 12 distinct solutions.
    //
    // Compared to Test 10 (K3, 6 goals): one extra node, one extra colour goal,
    // one extra diff goal, and twice as many solutions — a meaningfully harder instance.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("red", {})}),   {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("green", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("color", {}), ep.functor("blue", {})}),  {}});  // idx 2
        // All 6 ordered diff pairs among {red, green, blue}
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("green", {})}), {}});  // idx 3
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("red", {})}), ep.functor("blue", {})}),  {}});  // idx 4
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("red", {})}),   {}});  // idx 5
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("green", {})}), ep.functor("blue", {})}),  {}});  // idx 6
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("red", {})}),   {}});  // idx 7
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), ep.functor("blue", {})}), ep.functor("green", {})}), {}});  // idx 8

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* D = ep.var(seq());

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), A}));                 // goal 0: color(A)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), B}));                 // goal 1: color(B)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), C}));                 // goal 2: color(C)
        goals.push_back(ep.functor("cons", {ep.functor("color", {}), D}));                 // goal 3: color(D)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), B}));      // goal 4: diff(A,B)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), C}));      // goal 5: diff(A,C)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), B}), C}));      // goal 6: diff(B,C)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("diff", {}), A}), D}));      // goal 7: diff(A,D)

        const expr* R_ = ep.functor("red", {});
        const expr* G_ = ep.functor("green", {});
        const expr* B_ = ep.functor("blue", {});

        // 6 K3 colourings of (A,B,C), each with 2 choices for D (any colour ≠ A)
        std::set<solution> expected = {
            {R_, G_, B_, G_}, {R_, G_, B_, B_},
            {R_, B_, G_, G_}, {R_, B_, G_, B_},
            {G_, R_, B_, R_}, {G_, R_, B_, B_},
            {G_, B_, R_, R_}, {G_, B_, R_, B_},
            {B_, R_, G_, R_}, {B_, R_, G_, G_},
            {B_, G_, R_, R_}, {B_, G_, R_, G_},
        };

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(A)), ep.import(norm(B)), ep.import(norm(C)), ep.import(norm(D))};
        });
    }

    // Test 13: 4-variable SAT — (P ∨ Q) ∧ (R ∨ S) ∧ (¬P ∨ ¬R)
    //
    // Three clauses, four boolean variables. The third clause forbids P and R
    // both being true, while the first two require each pair to cover true.
    //
    // Using the same relational OR/AND/NOT encoding as Test 11, plus NOT rules
    // for each primary variable. Intermediate result variables (PQ, RS, NP, NR,
    // NPR, PQ_RS) connect the clause structure. All 11 goals are listed below.
    //
    // Propagation chain (deterministic, no MCTS decisions needed for these steps):
    //   and(PQ_RS, NPR, true): head-elim removes and(false,_,false); unit-prop
    //     → PQ_RS = true, NPR = true.
    //   and(PQ, RS, PQ_RS=true): same → PQ = true, RS = true.
    //   Remaining: or(P,Q,true), or(R,S,true), or(NP,NR,true) each have 2 candidates.
    //   not rules propagate as soon as P or R is bound.
    //
    // Satisfying assignments (5):
    //   (T,T,F,T), (T,F,F,T), (F,T,T,T), (F,T,T,F), (F,T,F,T)
    //
    // Every solution needs ≥ 2 MCTS decisions, so all avoidances are non-empty
    // and CDCL learning drives refutation after all 5 are found.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        database db;
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("true", {})}),  {}});  // idx 0
        db.push_back(rule{ep.functor("cons", {ep.functor("bool", {}), ep.functor("false", {})}), {}});  // idx 1
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), ep.functor("true", {})}), ep.functor("false", {})}), {}});  // idx 2
        db.push_back(rule{ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), ep.functor("false", {})}), ep.functor("true", {})}),  {}});  // idx 3
        // or(true, X, true) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("true", {})}), X}), ep.functor("true", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 4
        }
        // or(false, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), ep.functor("false", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 5
        }
        // and(true, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("true", {})}), X}), X}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 6
        }
        // and(false, X, false) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), ep.functor("false", {})}), X}), ep.functor("false", {})}),
                {ep.functor("cons", {ep.functor("bool", {}), X})}
            });  // idx 7
        }

        // Primary variables
        const expr* P     = ep.var(seq());
        const expr* Q     = ep.var(seq());
        const expr* R     = ep.var(seq());
        const expr* S     = ep.var(seq());
        // Intermediate result variables
        const expr* PQ    = ep.var(seq());   // P ∨ Q
        const expr* RS    = ep.var(seq());   // R ∨ S
        const expr* NP    = ep.var(seq());   // ¬P
        const expr* NR    = ep.var(seq());   // ¬R
        const expr* NPR   = ep.var(seq());   // ¬P ∨ ¬R
        const expr* PQ_RS = ep.var(seq());   // (P∨Q) ∧ (R∨S)

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), P}));                                               // goal  0: bool(P)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), Q}));                                               // goal  1: bool(Q)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), R}));                                               // goal  2: bool(R)
        goals.push_back(ep.functor("cons", {ep.functor("bool", {}), S}));                                               // goal  3: bool(S)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), P}), Q}), PQ}));                    // goal  4: or(P,Q,PQ)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), R}), S}), RS}));                    // goal  5: or(R,S,RS)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), P}), NP}));                                   // goal  6: not(P,NP)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("not", {}), R}), NR}));                                   // goal  7: not(R,NR)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("or", {}), NP}), NR}), NPR}));                   // goal  8: or(NP,NR,NPR)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), PQ}), RS}), PQ_RS}));                // goal  9: and(PQ,RS,PQ_RS)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("and", {}), PQ_RS}), NPR}), ep.functor("true", {})}));   // goal 10: and(PQ_RS,NPR,true)

        const expr* T_ = ep.functor("true", {});
        const expr* F_ = ep.functor("false", {});

        // All 5 satisfying assignments to (P, Q, R, S)
        std::set<solution> expected = {
            {T_, T_, F_, T_},   // P=T, Q=T, R=F, S=T
            {T_, F_, F_, T_},   // P=T, Q=F, R=F, S=T
            {F_, T_, T_, T_},   // P=F, Q=T, R=T, S=T
            {F_, T_, T_, F_},   // P=F, Q=T, R=T, S=F
            {F_, T_, F_, T_},   // P=F, Q=T, R=F, S=T
        };

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(P)), ep.import(norm(Q)), ep.import(norm(R)), ep.import(norm(S))};
        });
    }

    // Test 14: Peano arithmetic — enumerate all naturals less than 7
    //
    // Rules:
    //   idx 0: nat(zero).
    //   idx 1: nat(suc(X))    :- nat(X).
    //   idx 2: lt(zero, suc(X)) :- nat(X).
    //   idx 3: lt(suc(X), suc(Y)) :- lt(X, Y).
    //
    // Encoding:
    //   zero    = atom("zero")
    //   suc(X)  = cons(atom("suc"), X)
    //   nat(X)  = cons(atom("nat"), X)
    //   lt(X,Y) = cons(cons(atom("lt"), X), Y)
    //
    // Goal: lt(N, seven)   where seven = suc^7(zero)
    //
    // Exactly 7 solutions exist: N ∈ {0, 1, 2, 3, 4, 5, 6} in Peano encoding.
    // Reaching N=k requires k unfoldings of rule 3, one application of rule 2,
    // then k unfoldings of rule 1 (to discharge the nat subgoal), bottoming out
    // at rule 0.  next_until_refuted verifies all 7 solutions are found and that
    // the solver then proves refutation.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        // Build the Peano numeral for n: suc^n(zero)
        auto peano = [&](int n) -> const expr* {
            const expr* result = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                result = ep.functor("cons", {ep.functor("suc", {}), result});
            return result;
        };

        database db;

        // idx 0: nat(zero).
        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});

        // idx 1: nat(suc(X)) :- nat(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        // idx 2: lt(zero, suc(X)) :- nat(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("zero", {})}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        // idx 3: lt(suc(X), suc(Y)) :- lt(X, Y).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), ep.functor("cons", {ep.functor("suc", {}), Y})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), X}), Y})}
            });
        }

        const expr* N     = ep.var(seq());
        const expr* seven = peano(7);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), N}), seven}));  // goal 0: lt(N, seven)

        // All 7 Peano naturals strictly less than 7
        std::set<solution> expected = {
            {peano(0)},
            {peano(1)},
            {peano(2)},
            {peano(3)},
            {peano(4)},
            {peano(5)},
            {peano(6)},
        };

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(N))};
        });
    }

    // Test 15: Addition + less-than — enumerate all pairs (X, Y) with X + Y < 10
    //
    // Rules:
    //   idx 0: nat(zero).
    //   idx 1: nat(suc(X))           :- nat(X).
    //   idx 2: add(zero, Y, Y)       :- nat(Y).
    //   idx 3: add(suc(X), Y, suc(Z)):- add(X, Y, Z).
    //   idx 4: lt(zero, suc(X))      :- nat(X).
    //   idx 5: lt(suc(X), suc(Y))    :- lt(X, Y).
    //
    // Goals: add(X, Y, S),  lt(S, ten)   where ten = suc^10(zero)
    //
    // Exactly 55 pairs satisfy X + Y < 10: sum s = 0..9 contributes (10-s) pairs.
    // next_until_refuted verifies all 55 are found and the solver then refutes.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        database db;

        // idx 0: nat(zero).
        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});

        // idx 1: nat(suc(X)) :- nat(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        // idx 2: add(zero, Y, Y) :- nat(Y).
        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("zero", {})}), Y}), Y}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });
        }

        // idx 3: add(suc(X), Y, suc(Z)) :- add(X, Y, Z).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), ep.functor("cons", {ep.functor("suc", {}), Z})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), Z})}
            });
        }

        // idx 4: lt(zero, suc(X)) :- nat(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("zero", {})}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        // idx 5: lt(suc(X), suc(Y)) :- lt(X, Y).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), ep.functor("cons", {ep.functor("suc", {}), Y})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), X}), Y})}
            });
        }

        const expr* X   = ep.var(seq());
        const expr* Y   = ep.var(seq());
        const expr* S   = ep.var(seq());
        const expr* ten = peano(10);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), S}));  // goal 0: add(X, Y, S)
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), S}), ten}));              // goal 1: lt(S, ten)

        // All 55 pairs (x, y) with x + y < 10
        std::set<solution> expected;
        for (int x = 0; x < 10; ++x)
            for (int y = 0; y < 10 - x; ++y)
                expected.insert({peano(x), peano(y)});
        assert(expected.size() == 55);

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X)), ep.import(norm(Y))};
        });
    }

    // Test 16: Enumerate all pairs (X, Y) whose sum is exactly 10
    //
    // Rules:
    //   idx 0: nat(zero).
    //   idx 1: nat(suc(X))           :- nat(X).
    //   idx 2: add(zero, Y, Y)       :- nat(Y).
    //   idx 3: add(suc(X), Y, suc(Z)):- add(X, Y, Z).
    //
    // Goal: add(X, Y, ten)   where ten = suc^10(zero)
    //
    // Exactly 11 solutions: (0,10),(1,9),(2,8),...,(10,0).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        database db;

        // idx 0: nat(zero).
        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});

        // idx 1: nat(suc(X)) :- nat(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        // idx 2: add(zero, Y, Y) :- nat(Y).
        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("zero", {})}), Y}), Y}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });
        }

        // idx 3: add(suc(X), Y, suc(Z)) :- add(X, Y, Z).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), ep.functor("cons", {ep.functor("suc", {}), Z})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), Z})}
            });
        }

        const expr* X   = ep.var(seq());
        const expr* Y   = ep.var(seq());
        const expr* ten = peano(10);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), ten}));  // goal 0: add(X, Y, ten)

        // All 11 pairs (x, y) with x + y = 10
        std::set<solution> expected;
        for (int x = 0; x <= 10; ++x)
            expected.insert({peano(x), peano(10 - x)});
        assert(expected.size() == 11);

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X)), ep.import(norm(Y))};
        });
    }

    // Test 17: Enumerate all pairs (X, Y) whose product is exactly 8
    //
    // Rules:
    //   idx 0: nat(zero).
    //   idx 1: nat(suc(X))           :- nat(X).
    //   idx 2: add(zero, Y, Y)       :- nat(Y).
    //   idx 3: add(suc(X), Y, suc(Z)):- add(X, Y, Z).
    //   idx 4: mul(zero, Y, zero)    :- nat(Y).
    //   idx 5: mul(suc(X), Y, Z)     :- mul(X, Y, W), add(W, Y, Z).
    //
    // Goal: mul(X, Y, eight)   where eight = suc^8(zero)
    //
    // Exactly 4 solutions: (1,8),(2,4),(4,2),(8,1).
    // Zero is excluded because mul(zero,Y,zero) can never unify with eight.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        database db;

        // idx 0: nat(zero).
        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});

        // idx 1: nat(suc(X)) :- nat(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        // idx 2: add(zero, Y, Y) :- nat(Y).
        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("zero", {})}), Y}), Y}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });
        }

        // idx 3: add(suc(X), Y, suc(Z)) :- add(X, Y, Z).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), ep.functor("cons", {ep.functor("suc", {}), Z})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), Z})}
            });
        }

        // idx 4: mul(zero, Y, zero) :- nat(Y).
        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("mul", {}), ep.functor("zero", {})}), Y}), ep.functor("zero", {})}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });
        }

        // idx 5: mul(suc(X), Y, Z) :- mul(X, Y, W), add(W, Y, Z).
        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            const expr* W = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("mul", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), Z}),
                {
                    ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("mul", {}), X}), Y}), W}),
                    ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), W}), Y}), Z})
                }
            });
        }

        const expr* X     = ep.var(seq());
        const expr* Y     = ep.var(seq());
        const expr* eight = peano(8);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("mul", {}), X}), Y}), eight}));  // goal 0: mul(X, Y, eight)

        // All 4 factor pairs of 8
        std::set<solution> expected = {
            {peano(1), peano(8)},
            {peano(2), peano(4)},
            {peano(4), peano(2)},
            {peano(8), peano(1)},
        };

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X)), ep.import(norm(Y))};
        });
    }

    // Test 18: Dual bounded sums with shared X — add(X,Y,S), add(X,Z,T), both sums < 4
    //
    // Reuses nat/add/lt from Test 15. Four goals tie the same X to two independent sums S and T,
    // each strictly below B = suc^4(zero) (numeric sums in {0,1,2,3}).
    //
    // For x in 0..3: y,z each range 0..(3-x), giving 16+9+4+1 = 30 triples (X,Y,Z).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        database db;

        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});

        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("zero", {})}), Y}), Y}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });
        }

        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), ep.functor("cons", {ep.functor("suc", {}), Z})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), Z})}
            });
        }

        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("zero", {})}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), ep.functor("cons", {ep.functor("suc", {}), Y})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), X}), Y})}
            });
        }

        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        const expr* S = ep.var(seq());
        const expr* T = ep.var(seq());
        const expr* B = peano(4);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), S}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Z}), T}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), S}), B}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("lt", {}), T}), B}));

        std::set<solution> expected;
        for (int x = 0; x < 4; ++x)
            for (int y = 0; y < 4 - x; ++y)
                for (int z = 0; z < 4 - x; ++z)
                    expected.insert({peano(x), peano(y), peano(z)});
        assert(expected.size() == 30);

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 1000}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(X)), ep.import(norm(Y)), ep.import(norm(Z))};
        });
    }

    // Test 19: Catalan binary trees with exactly 5 nodes (C_5 = 42).
    //
    // nil = 0 nodes; bin(L,R) = cons(cons(bin,L),R) counts 1+|L|+|R|.  Two coupled goals on
    // the same T: wf(T) and nodes(T, suc^5(zero)).  Only nat/add (no lt).  Expected trees are
    // enumerated explicitly below (42 total), built bottom-up from named sub-shapes — no search
    // or recurrence over the Catalan family.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        auto peano = [&](int n) -> const expr* {
            const expr* r = ep.functor("zero", {});
            for (int i = 0; i < n; ++i)
                r = ep.functor("cons", {ep.functor("suc", {}), r});
            return r;
        };

        auto B = [&](const expr* L, const expr* R) -> const expr* {
            return ep.functor("cons", {ep.functor("cons", {ep.functor("bin", {}), L}), R});
        };

        database db;

        db.push_back(rule{ep.functor("cons", {ep.functor("nat", {}), ep.functor("zero", {})}), {}});

        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("nat", {}), ep.functor("cons", {ep.functor("suc", {}), X})}),
                {ep.functor("cons", {ep.functor("nat", {}), X})}
            });
        }

        {
            const expr* Y = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("zero", {})}), Y}), Y}),
                {ep.functor("cons", {ep.functor("nat", {}), Y})}
            });
        }

        {
            const expr* X = ep.var(seq());
            const expr* Y = ep.var(seq());
            const expr* Z = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), ep.functor("cons", {ep.functor("suc", {}), X})}), Y}), ep.functor("cons", {ep.functor("suc", {}), Z})}),
                {ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), X}), Y}), Z})}
            });
        }

        db.push_back(rule{ep.functor("cons", {ep.functor("wf", {}), ep.functor("nil", {})}), {}});

        {
            const expr* L = ep.var(seq());
            const expr* R = ep.var(seq());
            db.push_back(rule{
                ep.functor("cons", {ep.functor("wf", {}), ep.functor("cons", {ep.functor("cons", {ep.functor("bin", {}), L}), R})}),
                {
                    ep.functor("cons", {ep.functor("wf", {}), L}),
                    ep.functor("cons", {ep.functor("wf", {}), R}),
                }
            });
        }

        db.push_back(rule{
            ep.functor("cons", {ep.functor("cons", {ep.functor("nodes", {}), ep.functor("nil", {})}), ep.functor("zero", {})}),
            {},
        });

        {
            const expr* L = ep.var(seq());
            const expr* R = ep.var(seq());
            const expr* S = ep.var(seq());
            const expr* NL = ep.var(seq());
            const expr* NR = ep.var(seq());
            const expr* Tmp = ep.var(seq());
            const expr* one = peano(1);
            db.push_back(rule{
                ep.functor("cons", {ep.functor("cons", {ep.functor("nodes", {}), ep.functor("cons", {ep.functor("cons", {ep.functor("bin", {}), L}), R})}), S}),
                {
                    ep.functor("cons", {ep.functor("cons", {ep.functor("nodes", {}), L}), NL}),
                    ep.functor("cons", {ep.functor("cons", {ep.functor("nodes", {}), R}), NR}),
                    ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), NL}), NR}), Tmp}),
                    ep.functor("cons", {ep.functor("cons", {ep.functor("cons", {ep.functor("add", {}), one}), Tmp}), S}),
                }
            });
        }

        const expr* N  = ep.functor("nil", {});
        const expr* s1 = B(N, N);                 // 1 node
        const expr* s2_left_chain  = B(N, s1);    // 2 nodes (spine left)
        const expr* s2_right_chain = B(s1, N);    // 2 nodes (spine right)

        // 3 nodes (5 shapes)
        const expr* s3_0 = B(N, s2_left_chain);
        const expr* s3_1 = B(N, s2_right_chain);
        const expr* s3_2 = B(s1, s1);
        const expr* s3_3 = B(s2_left_chain, N);
        const expr* s3_4 = B(s2_right_chain, N);

        // 4 nodes (14 shapes): B(N,·) on each 3-node tree; B(s1,·) on each 2-node; B(2-node,s1);
        // B(3-node,N)
        const expr* s4_0  = B(N, s3_0);
        const expr* s4_1  = B(N, s3_1);
        const expr* s4_2  = B(N, s3_2);
        const expr* s4_3  = B(N, s3_3);
        const expr* s4_4  = B(N, s3_4);
        const expr* s4_5  = B(s1, s2_left_chain);
        const expr* s4_6  = B(s1, s2_right_chain);
        const expr* s4_7  = B(s2_left_chain, s1);
        const expr* s4_8  = B(s2_right_chain, s1);
        const expr* s4_9  = B(s3_0, N);
        const expr* s4_10 = B(s3_1, N);
        const expr* s4_11 = B(s3_2, N);
        const expr* s4_12 = B(s3_3, N);
        const expr* s4_13 = B(s3_4, N);

        const expr* T    = ep.var(seq());
        const expr* five = peano(5);

        goals goals;
        goals.push_back(ep.functor("cons", {ep.functor("wf", {}), T}));
        goals.push_back(ep.functor("cons", {ep.functor("cons", {ep.functor("nodes", {}), T}), five}));

        // Expected trees must be interned *before* ridge's ctor runs: ridge() calls t.push(),
        // and the first sim_one() does t.pop() for that frame, undoing all expr_pool inserts
        // logged after that push — so building expected after ridge leaves dangling pointers in
        // std::set<solution> (import is fine; the stored expr* were rolled back off the trail).
        // 5 nodes — 42 trees: split (0,4): B(N,s4_i); (1,3): B(s1,s3_j); (2,2): all B(si,sj) for
        // si,sj in {s2_left_chain,s2_right_chain}; (3,1): B(s3_j,s1); (4,0): B(s4_i,N).
        std::set<solution> expected;
        expected.insert({ep.import(B(N, s4_0))});
        expected.insert({ep.import(B(N, s4_1))});
        expected.insert({ep.import(B(N, s4_2))});
        expected.insert({ep.import(B(N, s4_3))});
        expected.insert({ep.import(B(N, s4_4))});
        expected.insert({ep.import(B(N, s4_5))});
        expected.insert({ep.import(B(N, s4_6))});
        expected.insert({ep.import(B(N, s4_7))});
        expected.insert({ep.import(B(N, s4_8))});
        expected.insert({ep.import(B(N, s4_9))});
        expected.insert({ep.import(B(N, s4_10))});
        expected.insert({ep.import(B(N, s4_11))});
        expected.insert({ep.import(B(N, s4_12))});
        expected.insert({ep.import(B(N, s4_13))});

        expected.insert({ep.import(B(s1, s3_0))});
        expected.insert({ep.import(B(s1, s3_1))});
        expected.insert({ep.import(B(s1, s3_2))});
        expected.insert({ep.import(B(s1, s3_3))});
        expected.insert({ep.import(B(s1, s3_4))});

        expected.insert({ep.import(B(s2_left_chain, s2_left_chain))});
        expected.insert({ep.import(B(s2_left_chain, s2_right_chain))});
        expected.insert({ep.import(B(s2_right_chain, s2_left_chain))});
        expected.insert({ep.import(B(s2_right_chain, s2_right_chain))});

        expected.insert({ep.import(B(s3_0, s1))});
        expected.insert({ep.import(B(s3_1, s1))});
        expected.insert({ep.import(B(s3_2, s1))});
        expected.insert({ep.import(B(s3_3, s1))});
        expected.insert({ep.import(B(s3_4, s1))});

        expected.insert({ep.import(B(s4_0, N))});
        expected.insert({ep.import(B(s4_1, N))});
        expected.insert({ep.import(B(s4_2, N))});
        expected.insert({ep.import(B(s4_3, N))});
        expected.insert({ep.import(B(s4_4, N))});
        expected.insert({ep.import(B(s4_5, N))});
        expected.insert({ep.import(B(s4_6, N))});
        expected.insert({ep.import(B(s4_7, N))});
        expected.insert({ep.import(B(s4_8, N))});
        expected.insert({ep.import(B(s4_9, N))});
        expected.insert({ep.import(B(s4_10, N))});
        expected.insert({ep.import(B(s4_11, N))});
        expected.insert({ep.import(B(s4_12, N))});
        expected.insert({ep.import(B(s4_13, N))});

        assert(expected.size() == 42);

        std::mt19937 rng(42);
        ridge solver(solver_args{db, goals, t, seq, bm, 70}, mcts_solver_args{1.414, rng});

        normalizer norm(ep, bm);

        next_until_refuted(solver, expected, [&]() -> solution {
            return {ep.import(norm(T))};
        });
    }
}

void unit_test_main() {

    constexpr bool ENABLE_DEBUG_LOGS = true;

    // test cases
    TEST(test_trail_constructor);
    TEST(test_trail_push_pop);
    TEST(test_trail_log);
    TEST(test_functor_constructor);
    TEST(test_var_constructor);
    TEST(test_functor_cons_constructor);
    TEST(test_expr_constructor);
    TEST(test_expr_pool_functor_constructor);
    TEST(test_expr_pool_functor);
    TEST(test_expr_pool_var);
    TEST(test_expr_pool_functor_cons);
    TEST(test_expr_pool_import);
    TEST(test_bind_map_bind);
    TEST(test_bind_map_whnf);
    TEST(test_bind_map_occurs_check);
    TEST(test_bind_map_unify);
    TEST(test_lineage_pool_constructor);
    TEST(test_lineage_pool_intern_goal);
    TEST(test_lineage_pool_intern_resolution);
    TEST(test_lineage_pool_goal);
    TEST(test_lineage_pool_resolution);
    TEST(test_lineage_pool_pin_goal);
    TEST(test_lineage_pool_pin_resolution);
    TEST(test_lineage_pool_trim);
    TEST(test_lineage_pool_import);
    TEST(test_sequencer_constructor);
    TEST(test_sequencer);
    TEST(test_copier_constructor);
    TEST(test_copier);
    TEST(test_normalizer_constructor);
    TEST(test_normalizer);
    TEST(test_expr_printer_constructor);
    TEST(test_expr_printer);
    TEST(test_frontier_constructor);
    TEST(test_frontier_insert);
    TEST(test_frontier_empty);
    TEST(test_frontier_size);
    TEST(test_frontier_at);
    TEST(test_frontier_begin_end);
    TEST(test_frontier_resolve);
    TEST(test_weight_store_constructor);
    TEST(test_weight_store_total);
    TEST(test_weight_store_expand);
    TEST(test_goal_store_constructor);
    TEST(test_goal_store_try_unify_head);
    TEST(test_goal_store_applicable);
    TEST(test_goal_store_expand);
    TEST(test_candidate_store_constructor);
    TEST(test_candidate_store_eliminate);
    TEST(test_candidate_store_unit);
    TEST(test_candidate_store_conflicted);
    TEST(test_candidate_store_expand);
    TEST(test_mcts_decider_constructor);
    TEST(test_mcts_decider_choose_goal);
    TEST(test_mcts_decider_choose_candidate);
    TEST(test_mcts_decider);
    TEST(test_lemma_constructor);
    TEST(test_lemma_get_resolutions);
    TEST(test_lemma_remove_ancestors);
    TEST(test_cdcl_constructor);
    TEST(test_cdcl_upsert);
    TEST(test_cdcl_erase);
    TEST(test_cdcl_learn);
    TEST(test_cdcl_constrain);
    TEST(test_cdcl_refuted);
    TEST(test_cdcl_eliminated);
    TEST(test_sim_constructor);
    TEST(test_sim_get_resolutions);
    TEST(test_sim_get_decisions);
    TEST(test_sim_solved);
    TEST(test_sim_conflicted);
    TEST(test_sim_derive_one);
    TEST(test_sim_resolve);
    TEST(test_sim);
    TEST(test_ridge_sim_constructor);
    TEST(test_ridge_sim_decide_one);
    TEST(test_horizon_sim_reward);
    TEST(test_horizon_sim_on_resolve);
    TEST(test_horizon);
    TEST(test_ridge_sim);
    TEST(test_ridge_constructor_and_destructor);
    TEST(test_ridge);
}

int main() {
    unit_test_main();
    return 0;
}

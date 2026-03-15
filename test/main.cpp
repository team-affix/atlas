#include "../hpp/expr.hpp"
#include "../hpp/bind_map.hpp"
#include "../hpp/lineage.hpp"
#include "../hpp/sequencer.hpp"
#include "../hpp/copier.hpp"
#include "../hpp/normalizer.hpp"
#include "../hpp/rule.hpp"
#include "../hpp/a01_defs.hpp"
#include "../hpp/a01_goal_adder.hpp"
#include "../hpp/a01_goal_resolver.hpp"
#include "../hpp/a01_head_elimination_detector.hpp"
#include "../hpp/unit_propagation_detector.hpp"
#include "../hpp/solution_detector.hpp"
#include "../hpp/conflict_detector.hpp"
#include "../hpp/a01_cdcl_elimination_detector.hpp"
#include "../hpp/a01_decider.hpp"
#include "../hpp/a01_sim.hpp"
#include "../hpp/a01.hpp"
#include "test_utils.hpp"

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

void test_atom_constructor() {
    // Basic string
    expr::atom a1{"hello"};
    assert(a1.value == "hello");
    
    // Empty string
    expr::atom a2{""};
    assert(a2.value == "");
    
    // Single character
    expr::atom a3{"x"};
    assert(a3.value == "x");
    
    // Numeric strings
    expr::atom a4{"0"};
    assert(a4.value == "0");
    expr::atom a5{"12345"};
    assert(a5.value == "12345");
    expr::atom a6{"-42"};
    assert(a6.value == "-42");
    
    // Special characters
    expr::atom a7{"test_123"};
    assert(a7.value == "test_123");
    expr::atom a8{"hello world"};
    assert(a8.value == "hello world");
    expr::atom a9{"!@#$%^&*()"};
    assert(a9.value == "!@#$%^&*()");
    
    // Whitespace variations
    expr::atom a10{" "};
    assert(a10.value == " ");
    expr::atom a11{"\t"};
    assert(a11.value == "\t");
    expr::atom a12{"\n"};
    assert(a12.value == "\n");
    
    // Long string
    std::string long_str(1000, 'a');
    expr::atom a13{long_str};
    assert(a13.value == long_str);
    
    // Unicode/special characters
    expr::atom a14{"αβγδ"};
    assert(a14.value == "αβγδ");
    expr::atom a15{"日本語"};
    assert(a15.value == "日本語");
    
    // Escape sequences
    expr::atom a16{"line1\nline2"};
    assert(a16.value == "line1\nline2");
    
    // Multiple atoms with same value
    expr::atom a17{"duplicate"};
    expr::atom a18{"duplicate"};
    assert(a17.value == a18.value);
    assert((a17 <=> a18) == 0);
    
    // Test spaceship operator with different values
    expr::atom a19{"aaa"};
    expr::atom a20{"bbb"};
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

void test_cons_constructor() {
    // Basic cons with raw pointers (testing the struct itself)
    expr e1{expr::atom{"left"}};
    expr e2{expr::atom{"right"}};
    expr::cons c1{&e1, &e2};
    
    assert(c1.lhs == &e1);
    assert(c1.rhs == &e2);
    assert(std::get<expr::atom>(c1.lhs->content).value == "left");
    assert(std::get<expr::atom>(c1.rhs->content).value == "right");
    
    // Cons with variables
    expr e3{expr::var{0}};
    expr e4{expr::var{1}};
    expr::cons c2{&e3, &e4};
    
    assert(std::get<expr::var>(c2.lhs->content).index == 0);
    assert(std::get<expr::var>(c2.rhs->content).index == 1);
    
    // Cons with mixed types
    expr e5{expr::atom{"atom"}};
    expr e6{expr::var{42}};
    expr::cons c3{&e5, &e6};
    
    assert(std::get<expr::atom>(c3.lhs->content).value == "atom");
    assert(std::get<expr::var>(c3.rhs->content).index == 42);
    
    // Cons with same expr on both sides
    expr e7{expr::atom{"same"}};
    expr::cons c4{&e7, &e7};
    
    assert(c4.lhs == c4.rhs);
    assert(c4.lhs == &e7);
    
    // Test spaceship operator
    expr e8{expr::atom{"a"}};
    expr e9{expr::atom{"b"}};
    expr::cons c5{&e8, &e9};
    expr::cons c6{&e8, &e9};
    
    assert((c5 <=> c6) == 0);
    
    // Different cons
    expr e10{expr::atom{"c"}};
    expr::cons c7{&e8, &e10};
    
    assert((c5 <=> c7) != 0);
}

void test_expr_constructor() {
    // Expr with atom
    expr e1{expr::atom{"test"}};
    assert(std::holds_alternative<expr::atom>(e1.content));
    assert(std::get<expr::atom>(e1.content).value == "test");
    
    // Expr with empty atom
    expr e2{expr::atom{""}};
    assert(std::holds_alternative<expr::atom>(e2.content));
    assert(std::get<expr::atom>(e2.content).value == "");
    
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
    expr left{expr::atom{"left"}};
    expr right{expr::atom{"right"}};
    expr e6{expr::cons{&left, &right}};
    
    assert(std::holds_alternative<expr::cons>(e6.content));
    const expr::cons& c1 = std::get<expr::cons>(e6.content);
    assert(std::get<expr::atom>(c1.lhs->content).value == "left");
    assert(std::get<expr::atom>(c1.rhs->content).value == "right");
    
    // Test spaceship operator
    expr e7{expr::atom{"aaa"}};
    expr e8{expr::atom{"aaa"}};
    assert((e7 <=> e8) == 0);
    
    expr e9{expr::atom{"bbb"}};
    assert((e7 <=> e9) < 0);
    assert((e9 <=> e7) > 0);
    
    // Different variant types
    expr e10{expr::var{0}};
    // Comparison between different variant types is well-defined by spaceship
    assert((e7 <=> e10) != 0);
}

void test_expr_pool_constructor() {
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
    const expr* e1 = pool1.atom("test");
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
    const expr* e2 = pool2.atom("test2");
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

void test_expr_pool_atom() {
    trail t;
    expr_pool pool(t);
    
    // Push initial frame before any operations
    t.push();
    
    // Basic atom creation
    const expr* e1 = pool.atom("test");
    assert(e1 != nullptr);
    assert(std::holds_alternative<expr::atom>(e1->content));
    assert(std::get<expr::atom>(e1->content).value == "test");
    assert(pool.size() == 1);
    assert(pool.exprs.size() == 1);
    assert(pool.exprs.count(*e1) == 1);
    
    // Empty string
    const expr* e2 = pool.atom("");
    assert(std::get<expr::atom>(e2->content).value == "");
    assert(pool.size() == 2);
    assert(pool.exprs.size() == 2);
    assert(pool.exprs.count(*e2) == 1);
    assert(e1 != e2);
    
    // Interning - same string should return same pointer
    const expr* e3 = pool.atom("test");
    assert(e1 == e3);
    assert(pool.size() == 2);  // No new entry added
    assert(pool.exprs.size() == 2);
    
    // Different strings should return different pointers
    const expr* e4 = pool.atom("different");
    assert(e1 != e4);
    assert(pool.size() == 3);
    assert(pool.exprs.size() == 3);
    assert(pool.exprs.count(*e4) == 1);
    
    // Multiple calls with same string
    const expr* e5 = pool.atom("shared");
    assert(pool.exprs.size() == 4);
    const expr* e6 = pool.atom("shared");
    const expr* e7 = pool.atom("shared");
    assert(e5 == e6);
    assert(e6 == e7);
    assert(pool.size() == 4);  // Only one "shared" added
    assert(pool.exprs.size() == 4);
    assert(pool.exprs.count(*e5) == 1);
    
    // Special characters
    const expr* e8 = pool.atom("!@#$");
    assert(pool.exprs.size() == 5);
    const expr* e9 = pool.atom("!@#$");
    assert(e8 == e9);
    assert(pool.size() == 5);
    assert(pool.exprs.size() == 5);
    assert(pool.exprs.count(*e8) == 1);
    
    // Long strings
    std::string long_str(1000, 'x');
    const expr* e10 = pool.atom(long_str);
    assert(pool.exprs.size() == 6);
    const expr* e11 = pool.atom(long_str);
    assert(e10 == e11);
    assert(pool.size() == 6);
    assert(pool.exprs.size() == 6);
    assert(pool.exprs.count(*e10) == 1);
    
    // Test backtracking: push frame, add content, pop frame
    size_t size_before = pool.size();
    assert(pool.exprs.size() == size_before);
    t.push();
    const expr* temp1 = pool.atom("temporary1");
    const expr* temp2 = pool.atom("temporary2");
    assert(pool.size() == size_before + 2);
    assert(pool.exprs.size() == size_before + 2);
    assert(pool.exprs.count(*temp1) == 1);
    assert(pool.exprs.count(*temp2) == 1);
    t.pop();
    assert(pool.size() == size_before);  // Should be back to original size
    assert(pool.exprs.size() == size_before);
    
    // Test corner case: intern same content in nested frames
    t.push();  // Frame 1
    const expr* content_c = pool.atom("content_c");
    size_t checkpoint1 = pool.size();
    assert(content_c != nullptr);
    assert(pool.exprs.size() == checkpoint1);
    assert(pool.exprs.count(*content_c) == 1);
    
    t.push();  // Frame 2
    const expr* content_c_again = pool.atom("content_c");  // Should return same pointer, no log
    assert(content_c == content_c_again);
    assert(pool.size() == checkpoint1);  // Size unchanged
    assert(pool.exprs.size() == checkpoint1);
    
    t.pop();  // Pop frame 2
    assert(pool.size() == checkpoint1);  // Size still unchanged
    assert(pool.exprs.size() == checkpoint1);
    
    // Verify content_c is still there
    const expr* content_c_verify = pool.atom("content_c");
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
    pool.atom("level1_a");
    pool.atom("level1_b");
    size_t checkpoint_level1 = pool.size();
    assert(checkpoint_level1 == checkpoint_start + 2);
    assert(pool.exprs.size() == checkpoint_level1);
    
    t.push();  // Level 2
    pool.atom("level2_a");
    pool.atom("level2_b");
    pool.atom("level2_c");
    size_t checkpoint_level2 = pool.size();
    assert(checkpoint_level2 == checkpoint_level1 + 3);
    assert(pool.exprs.size() == checkpoint_level2);
    
    t.push();  // Level 3
    pool.atom("level3_a");
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
    
    const expr* early_content_1 = pool.atom("early_1");
    assert(pool.exprs.size() == checkpoint_start + 1);
    assert(pool.exprs.count(*early_content_1) == 1);
    
    const expr* early_content_2 = pool.atom("early_2");
    assert(pool.exprs.size() == checkpoint_start + 2);
    assert(pool.exprs.count(*early_content_2) == 1);
    
    size_t checkpoint_a = pool.size();
    assert(checkpoint_a == checkpoint_start + 2);
    assert(pool.exprs.size() == checkpoint_a);
    
    t.push();  // Frame B
    assert(pool.exprs.size() == checkpoint_a);
    
    const expr* mid_content = pool.atom("mid_content");
    assert(pool.exprs.size() == checkpoint_a + 1);
    assert(pool.exprs.count(*mid_content) == 1);
    
    size_t checkpoint_b = pool.size();
    assert(checkpoint_b == checkpoint_a + 1);
    assert(pool.exprs.size() == checkpoint_b);
    
    // Re-intern early content in Frame B - should not log since already exists
    const expr* early_content_1_again = pool.atom("early_1");
    assert(early_content_1 == early_content_1_again);
    assert(pool.size() == checkpoint_b);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b);  // Set size also unchanged
    assert(pool.exprs.count(*early_content_1) == 1);  // Still in set
    
    // Add more new content in Frame B
    const expr* late_content_1 = pool.atom("late_1");
    assert(pool.exprs.size() == checkpoint_b + 1);
    assert(pool.exprs.count(*late_content_1) == 1);
    
    const expr* late_content_2 = pool.atom("late_2");
    assert(pool.exprs.size() == checkpoint_b + 2);
    assert(pool.exprs.count(*late_content_2) == 1);
    
    size_t checkpoint_b_final = pool.size();
    assert(checkpoint_b_final == checkpoint_b + 2);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    t.push();  // Frame C
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Re-intern content from both Frame A and Frame B
    const expr* early_content_2_again = pool.atom("early_2");
    assert(early_content_2 == early_content_2_again);
    assert(pool.exprs.size() == checkpoint_b_final);  // No change
    assert(pool.exprs.count(*early_content_2) == 1);
    
    const expr* mid_content_again = pool.atom("mid_content");
    assert(mid_content == mid_content_again);
    assert(pool.size() == checkpoint_b_final);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b_final);  // Set size unchanged
    assert(pool.exprs.count(*mid_content) == 1);
    
    // Add new content in Frame C
    const expr* frame_c_content = pool.atom("frame_c");
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
    const expr* verify_early_1 = pool.atom("early_1");
    assert(verify_early_1 == early_content_1);
    assert(pool.exprs.count(*verify_early_1) == 1);
    
    const expr* verify_mid = pool.atom("mid_content");
    assert(verify_mid == mid_content);
    assert(pool.exprs.count(*verify_mid) == 1);
    
    const expr* verify_late_1 = pool.atom("late_1");
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
    const expr* verify_early_1_after_b = pool.atom("early_1");
    assert(verify_early_1_after_b == early_content_1);
    assert(pool.exprs.count(*verify_early_1_after_b) == 1);
    
    const expr* verify_early_2_after_b = pool.atom("early_2");
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

void test_expr_pool_cons() {
    trail t;
    expr_pool pool(t);
    
    // Push initial frame before any operations
    t.push();
    
    // Basic cons creation
    const expr* left = pool.atom("left");
    const expr* right = pool.atom("right");
    assert(pool.exprs.size() == 2);
    const expr* c1 = pool.cons(left, right);
    
    assert(c1 != nullptr);
    assert(std::holds_alternative<expr::cons>(c1->content));
    const expr::cons& cons1 = std::get<expr::cons>(c1->content);
    assert(cons1.lhs == left);
    assert(cons1.rhs == right);
    assert(pool.size() == 3);  // left, right, cons
    assert(pool.exprs.size() == 3);
    assert(pool.exprs.count(*c1) == 1);
    
    // Interning - same cons should return same pointer
    const expr* c2 = pool.cons(left, right);
    assert(c1 == c2);
    assert(pool.size() == 3);  // No new entry added
    assert(pool.exprs.size() == 3);
    
    // Different cons should return different pointers
    const expr* c3 = pool.cons(right, left);  // Swapped
    assert(c1 != c3);
    assert(pool.size() == 4);
    assert(pool.exprs.size() == 4);
    assert(pool.exprs.count(*c3) == 1);
    
    // Cons with variables
    const expr* v1 = pool.var(10);
    const expr* v2 = pool.var(20);
    assert(pool.exprs.size() == 6);
    const expr* c4 = pool.cons(v1, v2);
    assert(pool.exprs.size() == 7);
    const expr* c5 = pool.cons(v1, v2);
    assert(c4 == c5);
    assert(pool.size() == 7);  // v1, v2, cons(v1,v2)
    assert(pool.exprs.size() == 7);
    assert(pool.exprs.count(*c4) == 1);
    
    // Nested cons
    const expr* inner = pool.cons(pool.atom("a"), pool.atom("b"));
    assert(pool.exprs.count(*inner) == 1);
    const expr* outer = pool.cons(inner, pool.atom("c"));
    assert(pool.exprs.count(*outer) == 1);
    const expr* outer2 = pool.cons(inner, pool.atom("c"));
    assert(outer == outer2);
    size_t size_after_nested = pool.size();
    assert(pool.exprs.size() == size_after_nested);
    
    // Same expr on both sides
    const expr* same = pool.atom("same");
    const expr* c6 = pool.cons(same, same);
    assert(pool.exprs.count(*c6) == 1);
    const expr* c7 = pool.cons(same, same);
    assert(c6 == c7);
    assert(pool.size() == size_after_nested + 2);  // same, cons(same,same)
    assert(pool.exprs.size() == size_after_nested + 2);
    
    // Deep nesting with interning
    const expr* d1 = pool.cons(pool.atom("x"), pool.atom("y"));
    assert(pool.exprs.count(*d1) == 1);
    const expr* d2 = pool.cons(d1, d1);
    assert(pool.exprs.count(*d2) == 1);
    const expr* d3 = pool.cons(d2, d2);
    assert(pool.exprs.count(*d3) == 1);
    
    const expr* d1_dup = pool.cons(pool.atom("x"), pool.atom("y"));
    const expr* d2_dup = pool.cons(d1_dup, d1_dup);
    const expr* d3_dup = pool.cons(d2_dup, d2_dup);
    
    assert(d1 == d1_dup);
    assert(d2 == d2_dup);
    assert(d3 == d3_dup);
    size_t size_after_deep = pool.size();
    
    // Test backtracking: push frame, add content, pop frame
    size_t size_before = pool.size();
    t.push();
    const expr* temp_left = pool.atom("temp_left");
    const expr* temp_right = pool.atom("temp_right");
    const expr* temp_cons = pool.cons(temp_left, temp_right);
    assert(pool.size() == size_before + 3);
    t.pop();
    assert(pool.size() == size_before);  // Should be back to original size
    
    // Test corner case: intern same cons in nested frames
    t.push();  // Frame 1
    const expr* cons_a = pool.atom("cons_a");
    const expr* cons_b = pool.atom("cons_b");
    const expr* cons_ab = pool.cons(cons_a, cons_b);
    size_t checkpoint1 = pool.size();
    assert(cons_ab != nullptr);
    
    t.push();  // Frame 2
    const expr* cons_ab_again = pool.cons(cons_a, cons_b);  // Should return same pointer, no log
    assert(cons_ab == cons_ab_again);
    assert(pool.size() == checkpoint1);  // Size unchanged
    
    t.pop();  // Pop frame 2
    assert(pool.size() == checkpoint1);  // Size still unchanged
    
    // Verify cons_ab is still there
    const expr* cons_ab_verify = pool.cons(cons_a, cons_b);
    assert(cons_ab == cons_ab_verify);
    assert(pool.size() == checkpoint1);
    
    t.pop();  // Pop frame 1
    // Now cons_ab, cons_a, cons_b should be removed
    
    // Test nested pushes with checkpoints
    size_t checkpoint_start = pool.size();
    
    t.push();  // Level 1
    const expr* l1_a = pool.atom("l1_a");
    const expr* l1_b = pool.atom("l1_b");
    pool.cons(l1_a, l1_b);
    size_t checkpoint_level1 = pool.size();
    assert(checkpoint_level1 == checkpoint_start + 3);
    
    t.push();  // Level 2
    const expr* l2_a = pool.var(100);
    const expr* l2_b = pool.var(200);
    pool.cons(l2_a, l2_b);
    pool.cons(l1_a, l2_a);  // Mix from level 1 and level 2
    size_t checkpoint_level2 = pool.size();
    assert(checkpoint_level2 == checkpoint_level1 + 4);  // l2_a, l2_b, cons(l2_a,l2_b), cons(l1_a,l2_a)
    
    t.push();  // Level 3
    pool.cons(l1_a, l1_b);  // Re-intern from level 1, no new entry
    const expr* l3_cons = pool.cons(l2_a, l1_a);
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
    
    const expr* early_atom_1 = pool.atom("early_atom_1");
    assert(pool.exprs.size() == checkpoint_start + 1);
    assert(pool.exprs.count(*early_atom_1) == 1);
    
    const expr* early_atom_2 = pool.atom("early_atom_2");
    assert(pool.exprs.size() == checkpoint_start + 2);
    assert(pool.exprs.count(*early_atom_2) == 1);
    
    const expr* early_cons = pool.cons(early_atom_1, early_atom_2);
    assert(pool.exprs.size() == checkpoint_start + 3);
    assert(pool.exprs.count(*early_cons) == 1);
    
    size_t checkpoint_a = pool.size();
    assert(checkpoint_a == checkpoint_start + 3);
    assert(pool.exprs.size() == checkpoint_a);
    
    t.push();  // Frame B
    assert(pool.exprs.size() == checkpoint_a);
    
    const expr* mid_atom = pool.atom("mid_atom");
    assert(pool.exprs.size() == checkpoint_a + 1);
    assert(pool.exprs.count(*mid_atom) == 1);
    
    const expr* mid_cons = pool.cons(early_atom_1, mid_atom);  // Uses early_atom_1
    assert(pool.exprs.size() == checkpoint_a + 2);
    assert(pool.exprs.count(*mid_cons) == 1);
    
    size_t checkpoint_b = pool.size();
    assert(checkpoint_b == checkpoint_a + 2);  // mid_atom, mid_cons
    assert(pool.exprs.size() == checkpoint_b);
    
    // Re-intern early cons in Frame B - should not log since already exists
    const expr* early_cons_again = pool.cons(early_atom_1, early_atom_2);
    assert(early_cons == early_cons_again);
    assert(pool.size() == checkpoint_b);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b);  // Set size also unchanged
    assert(pool.exprs.count(*early_cons) == 1);  // Still in set
    
    // Add more new content in Frame B
    const expr* late_atom = pool.atom("late_atom");
    assert(pool.exprs.size() == checkpoint_b + 1);
    assert(pool.exprs.count(*late_atom) == 1);
    
    const expr* late_cons = pool.cons(late_atom, mid_atom);
    assert(pool.exprs.size() == checkpoint_b + 2);
    assert(pool.exprs.count(*late_cons) == 1);
    
    size_t checkpoint_b_final = pool.size();
    assert(checkpoint_b_final == checkpoint_b + 2);
    assert(pool.exprs.size() == checkpoint_b_final);
    
    t.push();  // Frame C
    assert(pool.exprs.size() == checkpoint_b_final);
    
    // Re-intern content from both Frame A and Frame B
    const expr* early_cons_again2 = pool.cons(early_atom_1, early_atom_2);
    assert(early_cons == early_cons_again2);
    assert(pool.exprs.size() == checkpoint_b_final);  // No change
    assert(pool.exprs.count(*early_cons) == 1);
    
    const expr* mid_cons_again = pool.cons(early_atom_1, mid_atom);
    assert(mid_cons == mid_cons_again);
    assert(pool.size() == checkpoint_b_final);  // Size unchanged
    assert(pool.exprs.size() == checkpoint_b_final);  // Set size unchanged
    assert(pool.exprs.count(*mid_cons) == 1);
    
    // Add new content in Frame C
    const expr* frame_c_atom = pool.atom("frame_c_atom");
    assert(pool.exprs.size() == checkpoint_b_final + 1);
    assert(pool.exprs.count(*frame_c_atom) == 1);
    
    const expr* frame_c_cons = pool.cons(frame_c_atom, early_atom_1);
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
    const expr* verify_early_cons = pool.cons(early_atom_1, early_atom_2);
    assert(verify_early_cons == early_cons);
    assert(pool.exprs.count(*verify_early_cons) == 1);
    
    const expr* verify_mid_cons = pool.cons(early_atom_1, mid_atom);
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
    const expr* verify_early_cons_after_b = pool.cons(early_atom_1, early_atom_2);
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
    trail t;
    expr_pool pool(t);
    t.push();

    // Test 1: Import an atom from the stack - result must be a pool pointer
    {
        expr stack_atom{expr::atom{"import_t1_atom"}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_atom);
        assert(imported != nullptr);
        assert(imported != &stack_atom);
        assert(std::holds_alternative<expr::atom>(imported->content));
        assert(std::get<expr::atom>(imported->content).value == "import_t1_atom");
        assert(pool.exprs.count(*imported) == 1);
        assert(pool.size() == before + 1);
    }

    // Test 2: Import a var from the stack - result must be a pool pointer
    {
        expr stack_var{expr::var{701}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_var);
        assert(imported != nullptr);
        assert(imported != &stack_var);
        assert(std::holds_alternative<expr::var>(imported->content));
        assert(std::get<expr::var>(imported->content).index == 701);
        assert(pool.exprs.count(*imported) == 1);
        assert(pool.size() == before + 1);
    }

    // Test 3: Import a var with edge-case indices (0 and UINT32_MAX)
    {
        expr stack_var0{expr::var{0}};
        size_t before = pool.size();
        const expr* imported0 = pool.import(&stack_var0);
        assert(imported0 != &stack_var0);
        assert(std::get<expr::var>(imported0->content).index == 0);
        assert(pool.exprs.count(*imported0) == 1);
        assert(pool.size() == before + 1);

        expr stack_var_max{expr::var{UINT32_MAX}};
        before = pool.size();
        const expr* imported_max = pool.import(&stack_var_max);
        assert(imported_max != &stack_var_max);
        assert(std::get<expr::var>(imported_max->content).index == UINT32_MAX);
        assert(pool.exprs.count(*imported_max) == 1);
        assert(pool.size() == before + 1);
    }

    // Test 4: Import a simple cons (all three nodes on stack) - children must land in pool
    {
        expr l{expr::atom{"import_t4_l"}};
        expr r{expr::atom{"import_t4_r"}};
        expr c{expr::cons{&l, &r}};
        size_t before = pool.size();
        const expr* imported = pool.import(&c);
        assert(imported != nullptr);
        assert(imported != &c);
        assert(std::holds_alternative<expr::cons>(imported->content));
        const expr::cons& ic = std::get<expr::cons>(imported->content);
        assert(ic.lhs != &l);   // pool copy, not the stack object
        assert(ic.rhs != &r);
        assert(pool.exprs.count(*ic.lhs) == 1);
        assert(pool.exprs.count(*ic.rhs) == 1);
        assert(pool.exprs.count(*imported) == 1);
        assert(std::get<expr::atom>(ic.lhs->content).value == "import_t4_l");
        assert(std::get<expr::atom>(ic.rhs->content).value == "import_t4_r");
        assert(pool.size() == before + 3);  // l, r, cons
    }

    // Test 5: Import nested structure entirely on stack: cons(cons(atom, var), atom)
    {
        expr a{expr::atom{"import_t5_a"}};
        expr v{expr::var{702}};
        expr inner{expr::cons{&a, &v}};
        expr b{expr::atom{"import_t5_b"}};
        expr root{expr::cons{&inner, &b}};
        size_t before = pool.size();
        const expr* imported = pool.import(&root);
        assert(imported != nullptr);
        assert(pool.size() == before + 5);  // a, v, inner, b, root
        const expr::cons& rc = std::get<expr::cons>(imported->content);
        const expr::cons& ic = std::get<expr::cons>(rc.lhs->content);
        assert(std::get<expr::atom>(ic.lhs->content).value == "import_t5_a");
        assert(std::get<expr::var>(ic.rhs->content).index == 702);
        assert(std::get<expr::atom>(rc.rhs->content).value == "import_t5_b");
        assert(pool.exprs.count(*rc.lhs) == 1);
        assert(pool.exprs.count(*rc.rhs) == 1);
        assert(pool.exprs.count(*imported) == 1);
    }

    // Test 6: cons with LHS already in pool, RHS on stack - pool pointer must be preserved
    {
        const expr* pool_x = pool.atom("import_t6_x");
        expr stack_r{expr::atom{"import_t6_r"}};
        expr stack_c{expr::cons{pool_x, &stack_r}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::cons& ic = std::get<expr::cons>(imported->content);
        assert(ic.lhs == pool_x);       // pool LHS pointer is preserved exactly
        assert(ic.rhs != &stack_r);     // stack RHS got a fresh pool pointer
        assert(pool.exprs.count(*ic.rhs) == 1);
        assert(std::get<expr::atom>(ic.rhs->content).value == "import_t6_r");
        assert(pool.size() == before + 2);  // stack_r atom + cons
    }

    // Test 7: cons with LHS on stack, RHS already in pool - pool pointer must be preserved
    {
        const expr* pool_y = pool.atom("import_t7_y");
        expr stack_l{expr::atom{"import_t7_l"}};
        expr stack_c{expr::cons{&stack_l, pool_y}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::cons& ic = std::get<expr::cons>(imported->content);
        assert(ic.lhs != &stack_l);     // stack LHS got a fresh pool pointer
        assert(ic.rhs == pool_y);       // pool RHS pointer is preserved exactly
        assert(pool.exprs.count(*ic.lhs) == 1);
        assert(pool.size() == before + 2);  // stack_l atom + cons
    }

    // Test 8: nested cons where the inner cons is already in pool but outer is on stack
    {
        const expr* pool_p = pool.atom("import_t8_p");
        const expr* pool_q = pool.atom("import_t8_q");
        const expr* pool_inner = pool.cons(pool_p, pool_q);
        expr stack_r{expr::atom{"import_t8_r"}};
        expr stack_outer{expr::cons{pool_inner, &stack_r}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_outer);
        assert(imported != nullptr);
        const expr::cons& oc = std::get<expr::cons>(imported->content);
        assert(oc.lhs == pool_inner);   // inner cons pointer is preserved exactly
        assert(oc.rhs != &stack_r);
        assert(pool.exprs.count(*oc.rhs) == 1);
        assert(pool.size() == before + 2);  // stack_r + outer cons
    }

    // Test 9: Import atom already entirely in pool - returns same pointer, pool unchanged
    {
        const expr* pool_atom = pool.atom("import_t9_already");
        size_t before = pool.size();
        const expr* imported = pool.import(pool_atom);
        assert(imported == pool_atom);
        assert(pool.size() == before);
    }

    // Test 10: Import var already entirely in pool - returns same pointer, pool unchanged
    {
        const expr* pool_var = pool.var(703);
        size_t before = pool.size();
        const expr* imported = pool.import(pool_var);
        assert(imported == pool_var);
        assert(pool.size() == before);
    }

    // Test 11: Import cons already entirely in pool - returns same pointer, pool unchanged
    {
        const expr* pl = pool.atom("import_t11_pl");
        const expr* pr = pool.atom("import_t11_pr");
        const expr* pc = pool.cons(pl, pr);
        size_t before = pool.size();
        const expr* imported = pool.import(pc);
        assert(imported == pc);
        assert(pool.size() == before);
    }

    // Test 12: Import stack atom with same value as existing pool atom - must deduplicate
    // (i.e. return the pool pointer, not the stack pointer)
    {
        const expr* pool_dup = pool.atom("import_t12_dup");
        expr stack_dup{expr::atom{"import_t12_dup"}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_dup);
        assert(imported == pool_dup);   // must return pool pointer, not &stack_dup
        assert(pool.size() == before);  // pool must not grow
    }

    // Test 13: Import stack cons structurally identical to an existing pool cons - must deduplicate
    {
        const expr* pa = pool.atom("import_t13_a");
        const expr* pb = pool.atom("import_t13_b");
        const expr* pc = pool.cons(pa, pb);
        expr stack_a{expr::atom{"import_t13_a"}};
        expr stack_b{expr::atom{"import_t13_b"}};
        expr stack_c{expr::cons{&stack_a, &stack_b}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_c);
        assert(imported == pc);         // must deduplicate to the existing pool pointer
        assert(pool.size() == before);
    }

    // Test 14: Deeply nested all-stack structure - 9 distinct nodes
    // Shape: cons(cons(cons(atom,var), cons(var,atom)), atom)
    {
        expr d0{expr::atom{"import_t14_d0"}};
        expr x0{expr::var{704}};
        expr ll{expr::cons{&d0, &x0}};
        expr x1{expr::var{705}};
        expr d1{expr::atom{"import_t14_d1"}};
        expr lr{expr::cons{&x1, &d1}};
        expr l{expr::cons{&ll, &lr}};
        expr d2{expr::atom{"import_t14_d2"}};
        expr root{expr::cons{&l, &d2}};
        size_t before = pool.size();
        const expr* imported = pool.import(&root);
        assert(imported != nullptr);
        assert(pool.size() == before + 9);
        const expr::cons& rc = std::get<expr::cons>(imported->content);
        assert(std::get<expr::atom>(rc.rhs->content).value == "import_t14_d2");
        assert(pool.exprs.count(*rc.rhs) == 1);
        const expr::cons& lc = std::get<expr::cons>(rc.lhs->content);
        assert(pool.exprs.count(*rc.lhs) == 1);
        const expr::cons& llc = std::get<expr::cons>(lc.lhs->content);
        assert(pool.exprs.count(*lc.lhs) == 1);
        assert(std::get<expr::atom>(llc.lhs->content).value == "import_t14_d0");
        assert(std::get<expr::var>(llc.rhs->content).index == 704);
        const expr::cons& lrc = std::get<expr::cons>(lc.rhs->content);
        assert(pool.exprs.count(*lc.rhs) == 1);
        assert(std::get<expr::var>(lrc.lhs->content).index == 705);
        assert(std::get<expr::atom>(lrc.rhs->content).value == "import_t14_d1");
        assert(pool.exprs.count(*imported) == 1);
    }

    // Test 15: cons with the same pool pointer on both sides
    {
        const expr* pool_s = pool.atom("import_t15_s");
        expr stack_c{expr::cons{pool_s, pool_s}};
        size_t before = pool.size();
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::cons& ic = std::get<expr::cons>(imported->content);
        assert(ic.lhs == pool_s);
        assert(ic.rhs == pool_s);
        assert(pool.size() == before + 1);  // only the cons itself is new
    }

    // Test 16: Trail integration - import in a nested frame; pop removes imported expressions
    {
        size_t before = pool.size();
        t.push();
        expr stack_trail{expr::atom{"import_t16_trail"}};
        const expr* imported = pool.import(&stack_trail);
        assert(imported != nullptr);
        assert(pool.size() == before + 1);
        t.pop();
        assert(pool.size() == before);
    }

    // Test 17: Trail - import cons in inner frame where LHS comes from outer frame
    {
        const expr* outer_atom = pool.atom("import_t17_outer");
        size_t before = pool.size();
        t.push();
        expr stack_inner{expr::atom{"import_t17_inner"}};
        expr stack_c{expr::cons{outer_atom, &stack_inner}};
        const expr* imported = pool.import(&stack_c);
        assert(imported != nullptr);
        const expr::cons& ic = std::get<expr::cons>(imported->content);
        assert(ic.lhs == outer_atom);   // outer-frame pool pointer is preserved
        assert(pool.size() == before + 2);  // inner atom + cons
        t.pop();
        // inner atom and cons were rolled back; outer_atom must survive
        assert(pool.size() == before);
        assert(pool.exprs.count(*outer_atom) == 1);
    }

    // Test 18: Importing an expression that is already in pool must not alter pool size,
    // even when called multiple times in a row
    {
        const expr* pa = pool.atom("import_t18_a");
        const expr* pb = pool.var(706);
        const expr* pc = pool.cons(pa, pb);
        size_t before = pool.size();
        assert(pool.import(pa) == pa);
        assert(pool.import(pb) == pb);
        assert(pool.import(pc) == pc);
        assert(pool.import(pa) == pa);  // repeat
        assert(pool.import(pc) == pc);  // repeat
        assert(pool.size() == before);  // absolutely no growth
    }

    // Test 19: cons created via pool.cons() with raw stack pointers as children,
    // then imported. The cons itself is already in the pool's set, but its children
    // are not - import must still recurse into them and return a fully-interned cons.
    {
        expr stack_e1{expr::atom{"import_t19_e1"}};
        expr stack_e2{expr::atom{"import_t19_e2"}};

        // Deliberately construct a cons in the pool whose children are stack pointers
        const expr* pool_cons_bad = pool.cons(&stack_e1, &stack_e2);
        assert(pool.exprs.count(*pool_cons_bad) == 1);  // cons is in pool
        assert(pool.exprs.count(stack_e1) == 0);        // but children are NOT
        assert(pool.exprs.count(stack_e2) == 0);

        size_t before = pool.size();
        const expr* imported = pool.import(pool_cons_bad);

        // import must recurse through the children even though the cons itself
        // was already in the set; result must be a fully-interned cons
        assert(imported != nullptr);
        const expr::cons& ic = std::get<expr::cons>(imported->content);
        assert(pool.exprs.count(*ic.lhs) == 1);  // e1 now in pool
        assert(pool.exprs.count(*ic.rhs) == 1);  // e2 now in pool
        assert(ic.lhs != &stack_e1);              // pool pointer, not stack
        assert(ic.rhs != &stack_e2);
        assert(std::get<expr::atom>(ic.lhs->content).value == "import_t19_e1");
        assert(std::get<expr::atom>(ic.rhs->content).value == "import_t19_e2");
        // pool must have grown by at least the two children
        assert(pool.size() >= before + 2);
    }

    t.pop();
    assert(pool.size() == 0);
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
        
        expr a1{expr::atom{"test"}};
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
        
        expr a1{expr::atom{"first"}};
        expr a2{expr::atom{"second"}};
        expr a3{expr::atom{"third"}};
        
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
        expr a1{expr::atom{"old"}};
        bm.bind(5, &a1);
        assert(bm.bindings.at(5) == &a1);
        assert(bm.bindings.size() == 1);
        
        // Frame 2: Update to new value
        t.push();
        expr a2{expr::atom{"new"}};
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
        
        expr a1{expr::atom{"same"}};
        
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
        
        expr a1{expr::atom{"first"}};
        expr a2{expr::atom{"second"}};
        expr a3{expr::atom{"third"}};
        
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
        expr a1{expr::atom{"frame1"}};
        bm.bind(20, &a1);
        assert(bm.bindings.size() == 1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2
        t.push();
        expr a2{expr::atom{"frame2"}};
        bm.bind(21, &a2);
        assert(bm.bindings.size() == 2);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3
        t.push();
        expr a3{expr::atom{"frame3"}};
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
        expr a1{expr::atom{"v1"}};
        bm.bind(30, &a1);
        assert(bm.bindings.at(30) == &a1);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        // Frame 2: update index 30 to a2
        t.push();
        expr a2{expr::atom{"v2"}};
        bm.bind(30, &a2);
        assert(bm.bindings.at(30) == &a2);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        // Frame 3: update index 30 to a3
        t.push();
        expr a3{expr::atom{"v3"}};
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        bm.bind(40, &a1);
        bm.bind(41, &a2);
        std::map<uint32_t, const expr*> checkpoint1 = bm.bindings;
        
        t.push();
        expr a3{expr::atom{"c"}};
        expr a4{expr::atom{"d"}};
        bm.bind(42, &a3);
        bm.bind(43, &a4);
        std::map<uint32_t, const expr*> checkpoint2 = bm.bindings;
        
        t.push();
        expr a5{expr::atom{"e"}};
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
        
        expr a1{expr::atom{"atom"}};
        expr v1{expr::var{50}};
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr c1{expr::cons{&a2, &a3}};
        
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
        expr a1{expr::atom{"end"}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build nested: cons(cons(v1, a1), v2)
        expr inner{expr::cons{&v1, &a1}};
        expr outer{expr::cons{&inner, &v2}};
        
        expr a2{expr::atom{"bound1"}};
        expr a3{expr::atom{"bound2"}};
        
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
        
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        expr a4{expr::atom{"d"}};
        expr a5{expr::atom{"e"}};
        
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
        
        expr a1{expr::atom{"same"}};
        
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
        
        expr a1{expr::atom{"val1"}};
        expr a2{expr::atom{"val2"}};
        
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
        expr a1{expr::atom{"end"}};
        
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
        
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"z"}};
        expr c2{expr::cons{&c1, &a3}};
        
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
        
        expr a1{expr::atom{"1"}};
        expr a2{expr::atom{"2"}};
        expr a3{expr::atom{"3"}};
        expr a4{expr::atom{"4"}};
        expr a5{expr::atom{"5"}};
        expr a6{expr::atom{"6"}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build: cons(cons(v1, v2), cons(v3, a1))
        expr left{expr::cons{&v1, &v2}};
        expr right{expr::cons{&v3, &a1}};
        expr outer{expr::cons{&left, &right}};
        
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr a4{expr::atom{"z"}};
        
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
        
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
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
        
        expr a1{expr::atom{"same"}};
        
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
        expr a1{expr::atom{"test"}};
        const expr* result = bm.whnf(&a1);
        assert(result == &a1);
        assert(bm.bindings.size() == 0);  // No bindings created for non-vars
    }
    
    // Test 2: whnf of cons returns itself
    {
        trail t;
        bind_map bm(t);
        expr a1{expr::atom{"left"}};
        expr a2{expr::atom{"right"}};
        expr c1{expr::cons{&a1, &a2}};
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
        expr a1{expr::atom{"bound"}};
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
        expr a1{expr::atom{"end"}};
        
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
        expr a1{expr::atom{"final"}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        bm.bindings[30] = &c1;
        assert(bm.bindings.size() == 1);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &c1);
        assert(std::holds_alternative<expr::cons>(result->content));
        assert(bm.bindings.size() == 1);
    }
    
    // Test 8: whnf with var bound to another var bound to cons
    {
        trail t;
        bind_map bm(t);
        t.push();  // Need frame for path compression
        
        expr v1{expr::var{40}};
        expr v2{expr::var{41}};
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr c1{expr::cons{&a1, &a2}};
        
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
        expr a1{expr::atom{"repeated"}};
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
        expr a1{expr::atom{"inner"}};
        expr a2{expr::atom{"outer"}};
        expr inner_cons{expr::cons{&a1, &a2}};
        expr a3{expr::atom{"wrap"}};
        expr outer_cons{expr::cons{&inner_cons, &a3}};
        
        bm.bindings[80] = &outer_cons;
        assert(bm.bindings.size() == 1);
        
        const expr* result = bm.whnf(&v1);
        assert(result == &outer_cons);
        assert(std::holds_alternative<expr::cons>(result->content));
        assert(bm.bindings.size() == 1);
    }
    
    // Test 13: whnf with different var indices
    {
        trail t;
        bind_map bm(t);
        expr v_small{expr::var{0}};
        expr v_large{expr::var{UINT32_MAX}};
        expr a1{expr::atom{"small"}};
        expr a2{expr::atom{"large"}};
        
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
        expr a1{expr::atom{""}};
        expr a2{expr::atom{"test123"}};
        expr a3{expr::atom{"!@#$%"}};
        
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
        expr a1{expr::atom{"left"}};
        expr a2{expr::atom{"right"}};
        expr c1{expr::cons{&a1, &a2}};
        
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
        expr a1{expr::atom{"bound_atom"}};
        
        // Bind v2 to an atom
        bm.bindings[101] = &a1;
        
        // Create cons with v1 and v2 as children
        expr c1{expr::cons{&v1, &v2}};
        
        // whnf of the cons should return the cons itself, NOT reduce children
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        assert(std::holds_alternative<expr::cons>(result->content));
        
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        // Children should still be the original var pointers, NOT reduced
        assert(cons_ref.lhs == &v1);
        assert(cons_ref.rhs == &v2);
        assert(bm.bindings.size() == 1);
    }
    
    // Test 17: Var bound to cons containing bound vars - children not reduced
    {
        trail t;
        bind_map bm(t);
        expr v_outer{expr::var{110}};
        expr v_left{expr::var{111}};
        expr v_right{expr::var{112}};
        expr a1{expr::atom{"left_val"}};
        expr a2{expr::atom{"right_val"}};
        
        // Bind the inner vars
        bm.bindings[111] = &a1;
        bm.bindings[112] = &a2;
        
        // Create cons with bound vars as children
        expr c1{expr::cons{&v_left, &v_right}};
        bm.bindings[110] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // The cons children should still point to vars, not atoms
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v_left);
        assert(cons_ref.rhs == &v_right);
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
        expr a1{expr::atom{"inner_bound"}};
        
        bm.bindings[122] = &a1;
        
        expr c1{expr::cons{&v_inner1, &v_inner2}};
        bm.bindings[120] = &v_chain2;
        bm.bindings[121] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_chain1);
        assert(result == &c1);
        
        // Cons children should be unchanged
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v_inner1);
        assert(cons_ref.rhs == &v_inner2);
        
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
        expr a1{expr::atom{"bound1"}};
        expr a2{expr::atom{"bound2"}};
        bm.bindings[131] = &a1;
        bm.bindings[133] = &a2;
        
        // Create nested structure: cons(cons(v1, v2), cons(v3, v4))
        expr inner_left{expr::cons{&v1, &v2}};
        expr inner_right{expr::cons{&v3, &v4}};
        expr outer{expr::cons{&inner_left, &inner_right}};
        
        bm.bindings[130] = &outer;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &outer);
        
        // Verify structure is unchanged - vars inside cons are not reduced
        const expr::cons& outer_cons = std::get<expr::cons>(result->content);
        assert(outer_cons.lhs == &inner_left);
        assert(outer_cons.rhs == &inner_right);
        
        const expr::cons& left_cons = std::get<expr::cons>(outer_cons.lhs->content);
        assert(left_cons.lhs == &v1);  // Still points to var, not a1
        assert(left_cons.rhs == &v2);
        
        const expr::cons& right_cons = std::get<expr::cons>(outer_cons.rhs->content);
        assert(right_cons.lhs == &v3);  // Still points to var, not a2
        assert(right_cons.rhs == &v4);
        
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
        expr a1{expr::atom{"chained"}};
        
        // v_left chains to atom
        bm.bindings[141] = &v_chain;
        bm.bindings[143] = &a1;
        
        expr c1{expr::cons{&v_left, &v_right}};
        bm.bindings[140] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Cons children should be unchanged - v_left is NOT reduced to a1
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v_left);
        assert(cons_ref.rhs == &v_right);
        assert(bm.bindings.size() == 3);
    }
    
    // Test 21: Multiple vars bound to same atom
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{150}};
        expr v2{expr::var{151}};
        expr v3{expr::var{152}};
        expr a1{expr::atom{"shared"}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
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
        expr c1{expr::cons{&v1, &v2}};
        
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        
        // Cons children remain as unbound vars
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v1);
        assert(cons_ref.rhs == &v2);
        assert(bm.bindings.size() == 0);  // No bindings created
    }
    
    // Test 24: Cons of mix (atom and var)
    {
        trail t;
        bind_map bm(t);
        expr a1{expr::atom{"atom"}};
        expr v1{expr::var{180}};
        expr c1{expr::cons{&a1, &v1}};
        
        const expr* result = bm.whnf(&c1);
        assert(result == &c1);
        
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &a1);
        assert(cons_ref.rhs == &v1);  // Var not reduced
        assert(bm.bindings.size() == 0);
    }
    
    // Test 25: Cons of mix (var and cons)
    {
        trail t;
        bind_map bm(t);
        expr v1{expr::var{190}};
        expr a1{expr::atom{"inner"}};
        expr a2{expr::atom{"inner2"}};
        expr inner_cons{expr::cons{&a1, &a2}};
        expr outer_cons{expr::cons{&v1, &inner_cons}};
        
        const expr* result = bm.whnf(&outer_cons);
        assert(result == &outer_cons);
        
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v1);  // Var not reduced
        assert(cons_ref.rhs == &inner_cons);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 26: Var bound to cons of bound vars - only outer reduced
    {
        trail t;
        bind_map bm(t);
        expr v_outer{expr::var{200}};
        expr v_inner1{expr::var{201}};
        expr v_inner2{expr::var{202}};
        expr a1{expr::atom{"val1"}};
        expr a2{expr::atom{"val2"}};
        
        // Bind inner vars
        bm.bindings[201] = &a1;
        bm.bindings[202] = &a2;
        
        // Create cons with bound vars
        expr c1{expr::cons{&v_inner1, &v_inner2}};
        bm.bindings[200] = &c1;
        assert(bm.bindings.size() == 3);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Children should STILL be vars, not reduced to atoms
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v_inner1);
        assert(cons_ref.rhs == &v_inner2);
        assert(std::holds_alternative<expr::var>(cons_ref.lhs->content));
        assert(std::holds_alternative<expr::var>(cons_ref.rhs->content));
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
        expr a1{expr::atom{"left_end"}};
        
        // v_left chains to atom
        bm.bindings[212] = &v_left_chain;
        bm.bindings[214] = &a1;
        
        // Cons contains chained var and unbound var
        expr c1{expr::cons{&v_left, &v_right}};
        bm.bindings[210] = &v_mid;
        bm.bindings[211] = &c1;
        assert(bm.bindings.size() == 4);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Cons children should be unchanged - not reduced
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v_left);  // Still v_left, not a1
        assert(cons_ref.rhs == &v_right);
        
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
        expr a1{expr::atom{"deep"}};
        
        // Bind v4
        bm.bindings[223] = &a1;
        
        // Create: cons(cons(v1, v2), cons(v3, v4))
        expr inner_left{expr::cons{&v1, &v2}};
        expr inner_right{expr::cons{&v3, &v4}};
        expr outer{expr::cons{&inner_left, &inner_right}};
        
        const expr* result = bm.whnf(&outer);
        assert(result == &outer);
        
        // All structure should be preserved, vars not reduced
        const expr::cons& outer_cons = std::get<expr::cons>(result->content);
        assert(outer_cons.lhs == &inner_left);
        assert(outer_cons.rhs == &inner_right);
        
        const expr::cons& left_cons = std::get<expr::cons>(outer_cons.lhs->content);
        assert(left_cons.lhs == &v1);
        assert(left_cons.rhs == &v2);
        
        const expr::cons& right_cons = std::get<expr::cons>(outer_cons.rhs->content);
        assert(right_cons.lhs == &v3);
        assert(right_cons.rhs == &v4);  // Still v4, not a1
        
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
        expr a_left{expr::atom{"left"}};
        expr a_right{expr::atom{"right"}};
        
        // Setup chains
        bm.bindings[232] = &v_left_chain;
        bm.bindings[233] = &a_left;
        bm.bindings[234] = &a_right;
        
        expr c1{expr::cons{&v_left, &v_right}};
        bm.bindings[230] = &v_outer_chain;
        bm.bindings[231] = &c1;
        assert(bm.bindings.size() == 5);
        
        const expr* result = bm.whnf(&v_outer);
        assert(result == &c1);
        
        // Cons children are still original vars, not reduced
        const expr::cons& cons_ref = std::get<expr::cons>(result->content);
        assert(cons_ref.lhs == &v_left);
        assert(cons_ref.rhs == &v_right);
        
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
        expr a1{expr::atom{"frame1"}};
        
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
        expr a1{expr::atom{"outer"}};
        expr a2{expr::atom{"inner"}};
        
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
        expr a1{expr::atom{"end"}};
        
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
        expr a1{expr::atom{"first"}};
        expr a2{expr::atom{"second"}};
        
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
        expr a1{expr::atom{"final"}};
        
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
        expr a1{expr::atom{"left"}};
        expr a2{expr::atom{"right"}};
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr a1{expr::atom{"chain1"}};
        expr a2{expr::atom{"chain2"}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
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
        expr a1{expr::atom{"target"}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&v2, &a1}};
        expr c2{expr::cons{&v3, &a2}};
        
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
        expr a1{expr::atom{"1"}};
        expr a2{expr::atom{"2"}};
        expr a3{expr::atom{"3"}};
        
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
        expr a1{expr::atom{"end"}};
        
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
        
        expr a1{expr::atom{"test"}};
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
        expr a1{expr::atom{"bound"}};
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
        expr a1{expr::atom{"end"}};
        
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
        
        expr a1{expr::atom{"left"}};
        expr a2{expr::atom{"right"}};
        expr c1{expr::cons{&a1, &a2}};
        
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
        expr a1{expr::atom{"right"}};
        expr c1{expr::cons{&v1, &a1}};
        
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
        
        expr a1{expr::atom{"left"}};
        expr v1{expr::var{60}};
        expr c1{expr::cons{&a1, &v1}};
        
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
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr a1{expr::atom{"inner"}};
        expr inner_cons{expr::cons{&v1, &a1}};
        expr a2{expr::atom{"outer"}};
        expr outer_cons{expr::cons{&inner_cons, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
        // Build: cons(cons(cons(v1, a1), a2), a3)
        expr inner1{expr::cons{&v1, &a1}};
        expr inner2{expr::cons{&inner1, &a2}};
        expr outer{expr::cons{&inner2, &a3}};
        
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
    //     expr a1{expr::atom{"test"}};
    //     expr c1{expr::cons{&v2, &a1}};
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
        expr a1{expr::atom{"test"}};
        expr c1{expr::cons{&v2, &a1}};
        
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
        expr a1{expr::atom{"test"}};
        expr c1{expr::cons{&v3, &a1}};
        
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
        expr a1{expr::atom{"bound"}};
        
        // Bind v2 to atom
        bm.bindings[101] = &a1;
        
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr a1{expr::atom{"left_bound"}};
        expr a2{expr::atom{"right_bound"}};
        
        bm.bindings[110] = &a1;
        bm.bindings[111] = &a2;
        
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr a1{expr::atom{"rhs"}};
        
        // v1 -> v2 -> v3 (v3 unbound)
        bm.bindings[115] = &v2;
        bm.bindings[116] = &v3;
        
        expr c1{expr::cons{&v1, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        
        expr inner{expr::cons{&v1, &a1}};
        expr outer{expr::cons{&inner, &v2}};
        
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
        expr a1{expr::atom{"deep"}};
        expr a2{expr::atom{"deeper"}};
        
        // Build nested: cons(cons(v_inner, a1), a2)
        expr inner{expr::cons{&v_inner, &a1}};
        expr outer{expr::cons{&inner, &a2}};
        
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
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr a1{expr::atom{"base"}};
        
        // Build: cons(cons(a1, v2), a1) where v2 is unbound
        expr inner{expr::cons{&a1, &v2}};
        expr outer{expr::cons{&inner, &a1}};
        
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
        
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
        expr c1{expr::cons{&a1, &a2}};
        expr c2{expr::cons{&c1, &a3}};
        
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
        expr a1{expr::atom{"test"}};
        
        expr c1{expr::cons{&v3, &a1}};  // v3 unbound
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
        expr a1{expr::atom{"end"}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        expr a4{expr::atom{"d"}};
        
        // Build: cons(cons(cons(cons(cons(v1, a1), a2), a3), a4), a1)
        expr level1{expr::cons{&v1, &a1}};
        expr level2{expr::cons{&level1, &a2}};
        expr level3{expr::cons{&level2, &a3}};
        expr level4{expr::cons{&level3, &a4}};
        expr level5{expr::cons{&level4, &a1}};
        
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
        expr a1{expr::atom{"x"}};
        
        // Build: cons(cons(v1, v2), cons(a1, v3))
        expr left{expr::cons{&v1, &v2}};
        expr right{expr::cons{&a1, &v3}};
        expr outer{expr::cons{&left, &right}};
        
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
        expr a1{expr::atom{"bound1"}};
        expr a2{expr::atom{"bound2"}};
        
        // Bind some vars to atoms
        bm.bindings[231] = &a1;
        bm.bindings[232] = &a2;
        
        // Build: cons(cons(v1, v2), cons(v3, a1))
        expr left{expr::cons{&v1, &v2}};
        expr right{expr::cons{&v3, &a1}};
        expr outer{expr::cons{&left, &right}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Build nested cons: cons(cons(cons(v_target, a1), a2), a1)
        expr inner1{expr::cons{&v_target, &a1}};
        expr inner2{expr::cons{&inner1, &a2}};
        expr outer{expr::cons{&inner2, &a1}};
        
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
        
        expr c1{expr::cons{&v_left1, &v_right1}};
        
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
        expr a1{expr::atom{"atom"}};
        
        // Build: cons(cons(v1, cons(v2, v3)), cons(cons(v4, v5), a1))
        expr inner_left_right{expr::cons{&v2, &v3}};
        expr inner_left{expr::cons{&v1, &inner_left_right}};
        expr inner_right_left{expr::cons{&v4, &v5}};
        expr inner_right{expr::cons{&inner_right_left, &a1}};
        expr outer{expr::cons{&inner_left, &inner_right}};
        
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
        expr a_left{expr::atom{"left_end"}};
        
        expr v_right1{expr::var{274}};
        expr v_right2{expr::var{275}};
        expr a_right{expr::atom{"right_end"}};
        
        // Left chain: v_left1 -> v_left2 -> a_left
        bm.bindings[272] = &v_left2;
        bm.bindings[273] = &a_left;
        
        // Right chain: v_right1 -> v_right2 -> a_right
        bm.bindings[274] = &v_right2;
        bm.bindings[275] = &a_right;
        
        expr c1{expr::cons{&v_left1, &v_right1}};
        
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
        expr a1{expr::atom{"other"}};
        
        // Chain: v1 -> v2 -> v3 -> v4 -> v5 (unbound)
        bm.bindings[280] = &v2;
        bm.bindings[281] = &v3;
        bm.bindings[282] = &v4;
        bm.bindings[283] = &v5;
        
        expr c1{expr::cons{&v1, &a1}};
        
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
        expr a1{expr::atom{"x"}};
        
        // Build: cons(cons(v1, a1), cons(v2, v3))
        expr left{expr::cons{&v1, &a1}};
        expr right{expr::cons{&v2, &v3}};
        expr outer{expr::cons{&left, &right}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build 10 levels of nesting
        expr* current = &v1;
        expr level1{expr::cons{current, &a1}};
        expr level2{expr::cons{&level1, &a1}};
        expr level3{expr::cons{&level2, &a1}};
        expr level4{expr::cons{&level3, &a1}};
        expr level5{expr::cons{&level4, &a1}};
        expr level6{expr::cons{&level5, &a1}};
        expr level7{expr::cons{&level6, &a1}};
        expr level8{expr::cons{&level7, &a1}};
        expr level9{expr::cons{&level8, &a1}};
        expr level10{expr::cons{&level9, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        expr level1{expr::cons{&v_left1, &v_right1}};  // Chains in both children
        expr level2{expr::cons{&level1, &a1}};
        expr level3{expr::cons{&level2, &a1}};
        expr level4{expr::cons{&level3, &a1}};
        
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
        
        expr left{expr::cons{&va1, &vb1}};
        expr right{expr::cons{&vc1, &vd1}};
        expr outer{expr::cons{&left, &right}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build: outer_cons = cons(v_left, v_right)
        //        v_left -> inner_left_cons = cons(v_target, a1)
        //        v_right -> inner_right_cons = cons(a1, a1)
        expr inner_left_cons{expr::cons{&v_target, &a1}};
        expr inner_right_cons{expr::cons{&a1, &a1}};
        
        bm.bindings[602] = &inner_left_cons;
        bm.bindings[603] = &inner_right_cons;
        
        expr outer_cons{expr::cons{&v_left, &v_right}};
        
        bm.bindings[600] = &v1;
        bm.bindings[601] = &outer_cons;
        
        // v0 -> v1 -> outer_cons
        // outer_cons.lhs = v_left -> inner_left_cons
        // inner_left_cons.lhs = v_target
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
        expr a1{expr::atom{"shared"}};
        
        // Both vars bound to the same atom
        bm.bindings[700] = &a1;
        bm.bindings[701] = &a1;
        
        expr c1{expr::cons{&v1, &v2}};
        
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
        
        expr c1{expr::cons{&v1, &v2}};
        
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
        expr a1{expr::atom{"x"}};
        expr shared_cons{expr::cons{&v_inner, &a1}};
        
        // Both vars bound to the same cons
        bm.bindings[720] = &shared_cons;
        bm.bindings[721] = &shared_cons;
        
        expr c1{expr::cons{&v1, &v2}};
        
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
        
        expr c1{expr::cons{&v_left1, &v_right1}};
        
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
        expr a_target{expr::atom{"convergence"}};
        
        // Left chain: v_left1 -> v_left2 -> a_target
        bm.bindings[740] = &v_left2;
        bm.bindings[741] = &a_target;
        
        // Right chain: v_right1 -> v_right2 -> a_target (same target!)
        bm.bindings[742] = &v_right2;
        bm.bindings[743] = &a_target;
        
        expr c1{expr::cons{&v_left1, &v_right1}};
        
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
        expr a1{expr::atom{"y"}};
        expr target_cons{expr::cons{&v_inner, &a1}};
        
        // Left chain: v_left1 -> v_left2 -> target_cons
        bm.bindings[750] = &v_left2;
        bm.bindings[751] = &target_cons;
        
        // Right chain: v_right1 -> v_right2 -> target_cons (same target!)
        bm.bindings[752] = &v_right2;
        bm.bindings[753] = &target_cons;
        
        expr c1{expr::cons{&v_left1, &v_right1}};
        
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
        expr a1{expr::atom{"test"}};
        assert(bm.unify(&a1, &a1));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 2: Two different atoms - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"foo"}};
        expr a2{expr::atom{"bar"}};
        assert(!bm.unify(&a1, &a2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 3: Two different atoms (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"foo"}};
        expr a2{expr::atom{"bar"}};
        assert(!bm.unify(&a2, &a1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 4: Two identical atoms (different objects) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"same"}};
        expr a2{expr::atom{"same"}};
        assert(bm.unify(&a1, &a2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 5: Two identical atoms (commuted) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"same"}};
        expr a2{expr::atom{"same"}};
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
        expr a1{expr::atom{"bound"}};
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
        expr a1{expr::atom{"bound"}};
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
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
        expr a1{expr::atom{"test"}};
        expr c1{expr::cons{&v2, &a1}};
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
        expr a1{expr::atom{"test"}};
        expr c1{expr::cons{&v2, &a1}};
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
        expr a1{expr::atom{"test"}};
        
        // v2 -> v3 (which is same index as v1)
        bm.bindings[13] = &v3;
        
        expr c1{expr::cons{&v2, &a1}};
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
        expr a1{expr::atom{"test"}};
        
        bm.bindings[15] = &v3;
        
        expr c1{expr::cons{&v2, &a1}};
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
        expr a1{expr::atom{"a"}};
        
        // Build: cons(cons(cons(v2, a1), a1), a1)
        expr inner1{expr::cons{&v2, &a1}};
        expr inner2{expr::cons{&inner1, &a1}};
        expr outer{expr::cons{&inner2, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        
        expr inner1{expr::cons{&v2, &a1}};
        expr inner2{expr::cons{&inner1, &a1}};
        expr outer{expr::cons{&inner2, &a1}};
        
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
        expr a1{expr::atom{"target"}};
        
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
        expr a1{expr::atom{"target"}};
        
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
        expr a1{expr::atom{"end"}};
        
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
        expr a1{expr::atom{"end"}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"x"}};
        expr a4{expr::atom{"y"}};
        expr c2{expr::cons{&a3, &a4}};
        
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 25: Two cons with same atoms (commuted) - should succeed
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"x"}};
        expr a4{expr::atom{"y"}};
        expr c2{expr::cons{&a3, &a4}};
        
        assert(bm.unify(&c2, &c1));  // Commuted
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 26: Two cons with different atoms in lhs - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"z"}};  // Different
        expr a4{expr::atom{"y"}};
        expr c2{expr::cons{&a3, &a4}};
        
        assert(!bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 0);  // No bindings created (all atoms)

        t.pop();
    }
    
    // Test 27: Two cons with different atoms in rhs - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"x"}};
        expr a4{expr::atom{"z"}};  // Different
        expr c2{expr::cons{&a3, &a4}};
        
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
        expr a1{expr::atom{"y"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr c2{expr::cons{&a2, &a3}};
        
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
        expr a1{expr::atom{"y"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr c2{expr::cons{&a2, &a3}};
        
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
        expr a1{expr::atom{"y"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"z"}};  // Conflicts with a1
        expr c2{expr::cons{&a2, &a3}};
        
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
        expr a1{expr::atom{"y"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"z"}};
        expr c2{expr::cons{&a2, &a3}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr inner1{expr::cons{&a1, &a2}};
        expr a3{expr::atom{"c"}};
        expr outer1{expr::cons{&inner1, &a3}};
        
        expr a4{expr::atom{"a"}};
        expr a5{expr::atom{"b"}};
        expr inner2{expr::cons{&a4, &a5}};
        expr a6{expr::atom{"c"}};
        expr outer2{expr::cons{&inner2, &a6}};
        
        assert(bm.unify(&outer1, &outer2));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 33: Nested cons structures (commuted) - should recursively unify
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr inner1{expr::cons{&a1, &a2}};
        expr a3{expr::atom{"c"}};
        expr outer1{expr::cons{&inner1, &a3}};
        
        expr a4{expr::atom{"a"}};
        expr a5{expr::atom{"b"}};
        expr inner2{expr::cons{&a4, &a5}};
        expr a6{expr::atom{"c"}};
        expr outer2{expr::cons{&inner2, &a6}};
        
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
        expr a1{expr::atom{"test"}};
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr c1{expr::cons{&a2, &a3}};
        
        assert(!bm.unify(&a1, &c1));
        assert(bm.bindings.size() == 0);

        t.pop();
    }
    
    // Test 35: Cons with atom (commuted) - should fail
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"test"}};
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr c1{expr::cons{&a2, &a3}};
        
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
        expr a1{expr::atom{"a"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr v2{expr::var{51}};
        expr a2{expr::atom{"a"}};
        expr c2{expr::cons{&v2, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr v2{expr::var{53}};
        expr a2{expr::atom{"a"}};
        expr c2{expr::cons{&v2, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr inner1{expr::cons{&v2, &a1}};
        expr c1{expr::cons{&v1, &inner1}};
        
        expr a2{expr::atom{"a"}};
        expr a3{expr::atom{"b"}};
        expr a4{expr::atom{"a"}};
        expr inner2{expr::cons{&a3, &a4}};
        expr c2{expr::cons{&a2, &inner2}};
        
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
        expr a1{expr::atom{"a"}};
        expr inner1{expr::cons{&v2, &a1}};
        expr c1{expr::cons{&v1, &inner1}};
        
        expr a2{expr::atom{"a"}};
        expr a3{expr::atom{"b"}};
        expr a4{expr::atom{"a"}};
        expr inner2{expr::cons{&a3, &a4}};
        expr c2{expr::cons{&a2, &inner2}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr a3{expr::atom{"z"}};
        
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
        expr a1{expr::atom{"first"}};
        
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
        expr a1{expr::atom{"bound"}};
        
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
        expr a1{expr::atom{"same"}};
        
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
        expr a1{expr::atom{"first"}};
        expr a2{expr::atom{"second"}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build: cons(cons(v1, v2), cons(v3, a1))
        expr left1{expr::cons{&v1, &v2}};
        expr right1{expr::cons{&v3, &a1}};
        expr outer1{expr::cons{&left1, &right1}};
        
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr a4{expr::atom{"z"}};
        expr a5{expr::atom{"a"}};
        
        // Build: cons(cons(a2, a3), cons(a4, a5))
        expr left2{expr::cons{&a2, &a3}};
        expr right2{expr::cons{&a4, &a5}};
        expr outer2{expr::cons{&left2, &right2}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"x"}};
        expr a4{expr::atom{"z"}};  // Different from a2
        expr c2{expr::cons{&a3, &a4}};
        
        assert(!bm.unify(&c1, &c2));  // lhs matches, rhs doesn't
        assert(bm.bindings.size() == 0);  // No bindings created (all atoms)

        t.pop();
    }
    
    // Test 49: Cons unification where first child fails (short circuit)
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&a1, &a2}};
        
        expr a3{expr::atom{"z"}};  // Different from a1
        expr a4{expr::atom{"y"}};
        expr c2{expr::cons{&a3, &a4}};
        
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
        expr a1{expr::atom{"a"}};
        expr inner1{expr::cons{&v1, &a1}};
        expr inner2{expr::cons{&inner1, &v2}};
        expr outer1{expr::cons{&inner2, &a1}};
        
        // Build matching structure with atoms
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"a"}};
        expr inner3{expr::cons{&a2, &a3}};
        expr a4{expr::atom{"y"}};
        expr inner4{expr::cons{&inner3, &a4}};
        expr a5{expr::atom{"a"}};
        expr outer2{expr::cons{&inner4, &a5}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr c1{expr::cons{&v1, &a1}};
        
        expr a3{expr::atom{"z"}};
        expr a4{expr::atom{"w"}};  // Different from a1
        expr c2{expr::cons{&a3, &a4}};
        
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
        expr c1{expr::cons{&v1, &v1}};
        
        // Second structure: cons(a, V2)
        expr a1{expr::atom{"a"}};
        expr c2{expr::cons{&a1, &v2}};
        
        // Unify cons(V1, V1) with cons(a, V2)
        // This should bind V1 to 'a' and V2 to 'a'
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // V1->a and V2->a (or V2->V1)
        assert(bm.whnf(&v1) == &a1);
        assert(bm.whnf(&v2) == &a1);
        
        // Now try to unify cons(V1, V1) with cons(V3, k)
        expr a2{expr::atom{"k"}};
        expr c3{expr::cons{&v3, &a2}};
        
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
        expr c1{expr::cons{&v1, &v1}};
        
        // Second structure: cons(V2, V3)
        expr c2{expr::cons{&v2, &v3}};
        
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
        expr a1{expr::atom{"a"}};
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        expr c1{expr::cons{&v1, &a1}};
        
        // Build cons(cons(V1, b), a) - V1 appears nested inside
        expr inner{expr::cons{&v2, &a2}};
        expr c2{expr::cons{&inner, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build cons(V1, V2)
        expr c1{expr::cons{&v1, &v2}};
        
        // Build cons(cons(V1, a), V2)
        expr inner{expr::cons{&v3, &a1}};
        expr c2{expr::cons{&inner, &v4}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Build cons(V1, cons(V2, V1))
        expr inner1{expr::cons{&v2, &v3}};
        expr c1{expr::cons{&v1, &inner1}};
        
        // Build cons(cons(V1, a), b)
        expr inner2{expr::cons{&v4, &a1}};
        expr c2{expr::cons{&inner2, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Pre-existing chain: V4 -> V1
        bm.bindings[95] = &v1;
        
        expr c1{expr::cons{&v4, &v2}};
        
        // Build cons(a, cons(V1, b))
        expr inner{expr::cons{&v3, &a2}};
        expr c2{expr::cons{&a1, &inner}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build cons(V1, cons(a, V1)) - V1 appears twice
        expr inner{expr::cons{&a1, &v2}};
        expr c1{expr::cons{&v1, &inner}};
        
        // Build cons(cons(V1, a), cons(a, b))
        expr a2{expr::atom{"b"}};
        expr inner2{expr::cons{&v3, &a1}};
        expr inner3{expr::cons{&a1, &a2}};
        expr c2{expr::cons{&inner2, &inner3}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"a"}};
        expr a3{expr::atom{"a"}};
        
        // Build cons(V1, cons(V1, V1))
        expr inner1{expr::cons{&v2, &v3}};
        expr c1{expr::cons{&v1, &inner1}};
        
        // Build cons(a, cons(a, a))
        expr inner2{expr::cons{&a2, &a3}};
        expr c2{expr::cons{&a1, &inner2}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"x"}};
        
        // Build cons(cons(V1, V1), V1)
        expr inner1{expr::cons{&v1, &v2}};
        expr c1{expr::cons{&inner1, &v3}};
        
        // Build cons(cons(x, x), x)
        expr inner2{expr::cons{&a1, &a2}};
        expr c2{expr::cons{&inner2, &a3}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Build cons(V1, cons(V1, V1))
        expr inner1{expr::cons{&v2, &v3}};
        expr c1{expr::cons{&v1, &inner1}};
        
        // Build cons(cons(V1, a), b)
        expr inner2{expr::cons{&v4, &a1}};
        expr c2{expr::cons{&inner2, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Pre-bind V2 to V1
        bm.bindings[101] = &v1;
        
        expr c1{expr::cons{&v1, &v2}};
        expr c2{expr::cons{&a1, &v1}};
        
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
        expr inner1{expr::cons{&v2, &v1}};
        expr c1{expr::cons{&v1, &inner1}};
        
        // Build cons(V2, cons(V1, V2))
        expr inner2{expr::cons{&v1, &v2}};
        expr c2{expr::cons{&v2, &inner2}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Pre-existing chain: V1 -> V2
        bm.bindings[104] = &v2;
        
        expr c1{expr::cons{&v1, &a1}};
        expr c2{expr::cons{&v3, &a1}};
        
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
        expr c1{expr::cons{&v2, &v4}};
        expr c2{expr::cons{&v5, &v5}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Create shared inner: cons(V1, a)
        expr inner{expr::cons{&v1, &a1}};
        
        // Build cons(inner, V2) and cons(inner, b)
        expr c1{expr::cons{&inner, &v2}};
        expr c2{expr::cons{&inner, &a2}};
        
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
        expr a1{expr::atom{"x"}};
        expr a2{expr::atom{"y"}};
        expr a3{expr::atom{"z"}};
        
        // First failure
        t.push();
        expr c1{expr::cons{&v1, &a1}};
        expr c2{expr::cons{&a2, &a3}};  // Mismatched rhs
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
        expr a4{expr::atom{"w"}};
        expr a5{expr::atom{"q"}};
        expr c3{expr::cons{&v2, &a4}};
        expr c4{expr::cons{&a5, &a1}};  // Mismatched rhs
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"a"}};
        expr a3{expr::atom{"a"}};
        expr a4{expr::atom{"a"}};
        
        // Build cons(cons(V1, V1), cons(V1, V1))
        expr left1{expr::cons{&v1, &v2}};
        expr right1{expr::cons{&v3, &v4}};
        expr c1{expr::cons{&left1, &right1}};
        
        // Build cons(cons(a, a), cons(a, a))
        expr left2{expr::cons{&a1, &a2}};
        expr right2{expr::cons{&a3, &a4}};
        expr c2{expr::cons{&left2, &right2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Create cons(a, b)
        expr c_ab{expr::cons{&a1, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Create cons(a, b) and bind V1 to it
        expr c_ab{expr::cons{&a1, &a2}};
        bm.bindings[119] = &c_ab;
        
        // Unify V1 with cons(V2, V3)
        expr c2{expr::cons{&v2, &v3}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
        // Pre-bind V1 to 'c'
        bm.bindings[122] = &a3;
        
        // Unify cons(V1, a) with cons(b, V2)
        expr c1{expr::cons{&v1, &a1}};
        expr c2{expr::cons{&a2, &v2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
        // Pre-bind V2 to 'c'
        bm.bindings[125] = &a3;
        
        // Unify cons(V1, a) with cons(b, V2)
        expr c1{expr::cons{&v1, &a1}};
        expr c2{expr::cons{&a2, &v2}};
        
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
        expr a1{expr::atom{"w"}};
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr a4{expr::atom{"z"}};
        
        // Build cons(cons(cons(V1, V2), V3), V4)
        expr inner1{expr::cons{&v1, &v2}};
        expr inner2{expr::cons{&inner1, &v3}};
        expr c1{expr::cons{&inner2, &v4}};
        
        // Build cons(cons(cons(w, x), y), z)
        expr inner3{expr::cons{&a1, &a2}};
        expr inner4{expr::cons{&inner3, &a3}};
        expr c2{expr::cons{&inner4, &a4}};
        
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
        expr a1{expr::atom{"w"}};
        expr a2{expr::atom{"x"}};
        expr a3{expr::atom{"y"}};
        expr a4{expr::atom{"z"}};
        
        // Pre-bind V2 to x
        bm.bindings[131] = &a2;
        
        // Build cons(cons(cons(V1, V2), V3), V4)
        expr inner1{expr::cons{&v1, &v2}};
        expr inner2{expr::cons{&inner1, &v3}};
        expr c1{expr::cons{&inner2, &v4}};
        
        // Build cons(cons(cons(w, x), y), z)
        expr inner3{expr::cons{&a1, &a2}};
        expr inner4{expr::cons{&inner3, &a3}};
        expr c2{expr::cons{&inner4, &a4}};
        
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
        expr c1{expr::cons{&v1, &v2}};
        expr c2{expr::cons{&v3, &v4}};
        assert(bm.unify(&c1, &c2));
        size_t bindings_after_first = bm.bindings.size();
        assert(bindings_after_first == 2);  // V1 and V2 each bound
        
        // Second unification: cons(V3, V5) with cons(V6, V1)
        expr c3{expr::cons{&v3, &v5}};
        expr c4{expr::cons{&v6, &v1}};
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
        expr a1{expr::atom{"a"}};
        
        // Build cons(cons(V1, V1), a) - V1 appears twice in nested structure
        expr inner{expr::cons{&v2, &v3}};
        expr c1{expr::cons{&inner, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"x"}};
        
        // Build deeply nested structure: cons(cons(cons(V1, a), a), a)
        expr inner1{expr::cons{&v1, &a1}};
        expr inner2{expr::cons{&inner1, &a1}};
        expr inner3{expr::cons{&inner2, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        
        // Build cons(V1, V2)
        expr c1{expr::cons{&v1, &v2}};
        
        // Build cons(a, cons(V1, a)) - V1 appears in rhs
        expr inner{expr::cons{&v3, &a1}};
        expr c2{expr::cons{&a1, &inner}};
        
        // Unifying cons(V1, V2) with cons(a, cons(V1, a))
        // lhs: V1 with a - succeeds, binds V1 to a
        // rhs: V2 with cons(V1, a) - V1 now reduces to a, so cons(a, a), should succeed
        assert(bm.unify(&c1, &c2));
        assert(bm.bindings.size() == 2);  // V1->a, V2->cons(a,a)
        assert(bm.whnf(&v1) == &a1);
        // V2 should be bound to a cons cell
        const expr* v2_result = bm.whnf(&v2);
        assert(std::holds_alternative<expr::cons>(v2_result->content));
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        expr c1{expr::cons{&a1, &v1}};
        
        // Build cons(a, cons(b, V1))
        expr inner{expr::cons{&a2, &v2}};
        expr c2{expr::cons{&a1, &inner}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
        // Build cons(cons(V1, a), b)
        expr inner1{expr::cons{&v1, &a1}};
        expr c1{expr::cons{&inner1, &a2}};
        
        // Build cons(cons(cons(V1, c), a), b)
        expr inner2{expr::cons{&v2, &a3}};
        expr inner3{expr::cons{&inner2, &a1}};
        expr c2{expr::cons{&inner3, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        expr c1{expr::cons{&v1, &v2}};
        
        // Build cons(cons(V1, a), cons(b, V1))
        expr inner1{expr::cons{&v3, &a1}};
        expr inner2{expr::cons{&a2, &v4}};
        expr c2{expr::cons{&inner1, &inner2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        expr a3{expr::atom{"c"}};
        
        expr c1{expr::cons{&v1, &a1}};
        
        // Build cons(cons(cons(V1, b), c), a) - V1 deeply nested in lhs
        expr inner1{expr::cons{&v2, &a2}};
        expr inner2{expr::cons{&inner1, &a3}};
        expr c2{expr::cons{&inner2, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Pre-existing chain: V2 -> V1
        bm.bindings[151] = &v1;
        
        expr c1{expr::cons{&v1, &a1}};
        
        // Build cons(cons(V2, b), a) - V2 chains to V1
        expr inner{expr::cons{&v3, &a2}};
        expr c2{expr::cons{&inner, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Build cons(cons(V1, cons(V1, a)), b) - V1 appears twice in nested structure
        expr inner1{expr::cons{&v2, &a1}};
        expr inner2{expr::cons{&v3, &inner1}};
        expr c1{expr::cons{&inner2, &a2}};
        
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
        expr a1{expr::atom{"a"}};
        expr a2{expr::atom{"b"}};
        
        // Build cons(cons(V1, V2), a)
        expr inner1{expr::cons{&v1, &v2}};
        expr c1{expr::cons{&inner1, &a1}};
        
        // Build cons(cons(cons(V1, b), V2), a)
        expr inner2{expr::cons{&v3, &a2}};
        expr inner3{expr::cons{&inner2, &v4}};
        expr c2{expr::cons{&inner3, &a1}};
        
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
        expr a1{expr::atom{"a"}};
        
        expr c1{expr::cons{&v1, &v2}};
        
        // Build cons(V2, cons(V1, a))
        expr inner{expr::cons{&v4, &a1}};
        expr c2{expr::cons{&v3, &inner}};
        
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

        const expr* a1 = ep1.atom("hello");
        const expr* a2 = ep2.atom("hello");  // Same value, different pool
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

        const expr* c1 = ep1.cons(ep1.atom("x"), ep1.atom("y"));
        const expr* c2 = ep2.cons(ep2.atom("x"), ep2.atom("y"));
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
        const expr* atom = ep2.atom("target");

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
        const expr* atom = ep1.atom("value");

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
        const expr* c1 = ep1.cons(ep1.var(205), ep1.atom("fixed"));

        // ep2: cons("bound_val", "fixed")
        const expr* bound_val = ep2.atom("bound_val");
        const expr* c2        = ep2.cons(bound_val, ep2.atom("fixed"));

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
        expr a_hello{expr::atom{"hello"}};
        expr c_stack{expr::cons{&v_stack, &a_hello}};

        // Pool: cons("world", "hello")
        const expr* c_pool = ep.cons(ep.atom("world"), ep.atom("hello"));

        assert(bm.unify(&c_stack, c_pool));
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(206) == 1);

        const expr* world = ep.atom("world");
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
        const expr* c1 = ep1.cons(ep1.var(207), ep1.var(208));
        // ep2: cons(var(207), var(208)) — same indices, different pool
        const expr* c2 = ep2.cons(ep2.var(207), ep2.var(208));
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
        const expr* c2 = ep2.cons(ep2.var(209), ep2.atom("a"));

        // ep3: cons("bound", "a")
        const expr* bound = ep3.atom("bound");
        const expr* c3    = ep3.cons(bound, ep3.atom("a"));

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
        const expr* c_pool = ep.cons(ep.var(210), ep.atom("a"));

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
        const expr* c_pool = ep2.cons(ep2.var(211), ep2.atom("b"));  // contains index 211

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
        const expr* c1 = ep1.cons(
            ep1.cons(ep1.var(212), ep1.var(213)),
            ep1.cons(ep1.var(214), ep1.atom("leaf"))
        );

        // ep2: cons(cons("a", "b"), cons("c", "leaf"))
        const expr* a   = ep2.atom("a");
        const expr* b   = ep2.atom("b");
        const expr* c   = ep2.atom("c");
        const expr* c2  = ep2.cons(
            ep2.cons(a, b),
            ep2.cons(c, ep2.atom("leaf"))
        );

        assert(bm.unify(c1, c2));
        assert(bm.bindings.size() == 3);
        assert(bm.whnf(ep1.var(212)) == a);
        assert(bm.whnf(ep1.var(213)) == b);
        assert(bm.whnf(ep1.var(214)) == c);

        t.pop();
    }

    // ========== MIXED PRE-BOUND AND FRESH VARS — ALL DIFFERENT ALLOCATIONS ==========

    // Test 101: Pre-bound var from ep1 resolves during cons unification across ep1/ep2/ep3.
    // var(300) is pre-bound to ep2.atom("x"). Structure A from ep1 holds var(300) and
    // an unbound var(301). Structure B is entirely from ep3. Unification succeeds:
    // the pre-bound lhs position matches "x" structurally; the unbound rhs gets a new binding.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t);

        bm.bindings[300] = ep2.atom("x");  // pre-bind via raw map — no trail entry needed here

        const expr* A = ep1.cons(ep1.var(300), ep1.var(301));
        const expr* B = ep3.cons(ep3.atom("x"), ep3.atom("y"));

        assert(bm.unify(A, B));
        // lhs: var(300) → ep2.atom("x"); ep3.atom("x") → "x"=="x" → no new binding
        // rhs: var(301) unbound → bind 301 → ep3.atom("y")
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.count(301) == 1);
        assert(bm.whnf(ep1.var(300)) == ep2.atom("x"));
        assert(bm.whnf(ep1.var(301)) == ep3.atom("y"));

        t.pop();
    }

    // Test 102: Pre-bound chain spanning three pools — ep1.var(302)→ep2.var(303)→ep3.atom("end").
    // Unifying ep4.var(302) with ep5.atom("end") succeeds by traversing the chain:
    // ep4's allocation is irrelevant; only the index matters. No new binding is created.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t), ep5(t);

        bm.bindings[302] = ep2.var(303);
        bm.bindings[303] = ep3.atom("end");

        // ep4.var(302) and ep5.atom("end") are wholly new allocations
        assert(bm.unify(ep4.var(302), ep5.atom("end")));
        assert(bm.bindings.size() == 2);  // no new binding; chain resolved to matching atom

        // All allocations of the same index follow the same chain
        assert(bm.whnf(ep4.var(302)) == ep3.atom("end"));
        assert(bm.whnf(ep5.var(303)) == ep3.atom("end"));

        t.pop();
    }

    // Test 103: Two cons from different pools, both sides with pre-bound vars —
    // the pre-bound values are mutually consistent so no new binding is needed.
    // var(304) pre-bound to ep2.atom("q"), var(305) pre-bound to ep4.atom("p").
    // ep1.cons(var(304), "p") unified with ep3.cons("q", var(305)).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t);

        bm.bindings[304] = ep2.atom("q");
        bm.bindings[305] = ep4.atom("p");

        const expr* A = ep1.cons(ep1.var(304), ep1.atom("p"));
        const expr* B = ep3.cons(ep3.atom("q"), ep3.var(305));

        assert(bm.unify(A, B));
        assert(bm.bindings.size() == 2);  // pre-bindings only — no new binding created
        assert(bm.whnf(ep1.var(304)) == ep2.atom("q"));
        assert(bm.whnf(ep3.var(305)) == ep4.atom("p"));

        t.pop();
    }

    // Test 104: Nested cons across three pools; one var pre-bound, two freshly bound,
    // one var left unbound inside the structure that gets captured as a binding target.
    // Structure A (ep1): cons(cons(var(306), var(307)), var(308))
    //   var(306) → ep2.atom("alpha"),  var(307) and var(308) unbound.
    // Structure B (ep3): cons(cons("alpha","beta"), cons(var(309), "gamma"))
    //   var(309) unbound (it lives inside the cons that var(308) gets bound to).
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t);

        bm.bindings[306] = ep2.atom("alpha");

        const expr* A = ep1.cons(
            ep1.cons(ep1.var(306), ep1.var(307)),
            ep1.var(308)
        );
        const expr* B = ep3.cons(
            ep3.cons(ep3.atom("alpha"), ep3.atom("beta")),
            ep3.cons(ep3.var(309), ep3.atom("gamma"))
        );

        assert(bm.unify(A, B));
        // outer.lhs: var(306)→"alpha" == "alpha" ✓; var(307) → ep3.atom("beta")
        // outer.rhs: var(308) → ep3.cons(var(309), "gamma")  (var(309) stays unbound inside)
        assert(bm.bindings.size() == 3);
        assert(bm.whnf(ep1.var(306)) == ep2.atom("alpha"));
        assert(bm.whnf(ep1.var(307)) == ep3.atom("beta"));

        const expr* rhs308 = bm.whnf(ep1.var(308));
        assert(std::holds_alternative<expr::cons>(rhs308->content));

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

        const expr* target = ep4.cons(ep4.var(310), ep4.atom("x"));
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

        bm.bindings[312] = ep2.cons(ep2.atom("a"), ep2.atom("b"));

        // ep4: cons(var(312), var(313))
        const expr* A = ep4.cons(ep4.var(312), ep4.var(313));

        // ep5: cons(cons("a","b"), "result")  — structurally matches the pre-binding
        const expr* B = ep5.cons(ep5.cons(ep5.atom("a"), ep5.atom("b")), ep5.atom("result"));

        assert(bm.unify(A, B));
        // lhs: var(312) → ep2.cons("a","b"); ep5.cons("a","b") — different alloc, same structure → ✓
        // rhs: var(313) unbound → bind 313 → ep5.atom("result")
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.count(313) == 1);
        assert(bm.whnf(ep4.var(313)) == ep5.atom("result"));

        t.pop();
    }

    // Test 107: Pre-bound chain ends in an unbound var; structural unification extends
    // the chain by binding the tail var to an atom from a fourth pool.
    // var(314) → ep2.var(315),  var(315) unbound.
    // Unify ep3.var(315) with ep4.atom("merged") — binds 315; now var(314) transitively
    // resolves to ep4.atom("merged") through the chain.
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t);

        bm.bindings[314] = ep2.var(315);

        assert(bm.unify(ep3.var(315), ep4.atom("merged")));
        assert(bm.bindings.size() == 2);

        // var(314) from ep1 now resolves through the full chain
        assert(bm.whnf(ep1.var(314)) == ep4.atom("merged"));
        assert(bm.whnf(ep3.var(315)) == ep4.atom("merged"));

        t.pop();
    }

    // Test 108: Rich three-pool scenario — nested cons with a mix of pre-bound vars
    // (one direct, one chaining through another var) and two freshly unbound vars.
    // Every atom target comes from a different pool than the var it is matched against.
    // Structure A (ep1): cons(cons(var(316), var(317)), cons(var(318), var(320)))
    //   var(316) → ep1.atom("x")   (direct pre-binding)
    //   var(318) → ep1.var(319)    (chain; var(319) itself unbound)
    //   var(317), var(320) unbound
    // Structure B: cons(cons("x","y"), cons("z","w")) — atoms from ep2 and ep3
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t);

        bm.bindings[316] = ep1.atom("x");
        bm.bindings[318] = ep1.var(319);

        const expr* A = ep1.cons(
            ep1.cons(ep1.var(316), ep1.var(317)),
            ep1.cons(ep1.var(318), ep1.var(320))
        );
        const expr* B = ep2.cons(
            ep2.cons(ep2.atom("x"), ep2.atom("y")),
            ep2.cons(ep3.atom("z"), ep2.atom("w"))   // ep3 atom for extra cross-pool coverage
        );

        assert(bm.unify(A, B));
        // outer.lhs: var(316)→"x" == "x" ✓;  var(317) → ep2.atom("y")
        // outer.rhs: var(318)→var(319)→unbound → bind 319 → ep3.atom("z");  var(320) → ep2.atom("w")
        assert(bm.bindings.size() == 5);  // 316(pre) + 318(pre) + 317 + 319 + 320

        assert(bm.whnf(ep1.var(316)) == ep1.atom("x"));
        assert(bm.whnf(ep1.var(317)) == ep2.atom("y"));
        assert(bm.whnf(ep1.var(318)) == ep3.atom("z"));  // transitive: 318→319→ep3.atom("z")
        assert(bm.whnf(ep1.var(319)) == ep3.atom("z"));
        assert(bm.whnf(ep1.var(320)) == ep2.atom("w"));

        t.pop();
    }

    // Test 109: Pre-bound chain where the tail var is also used as a structural position;
    // unification extends the chain via a new binding from yet another pool allocation.
    // var(321) → ep2.var(322),  var(322) unbound.
    // ep3.cons(var(321), "x") unified with ep4.cons("a", "x"):
    //   lhs: var(321)→var(322)→unbound → bind 322 → ep4.atom("a")
    //   rhs: "x" == "x" ✓
    // var(321) from ep1 (a different allocation) now also resolves to ep4.atom("a").
    {
        trail t;
        bind_map bm(t);
        t.push();
        expr_pool ep1(t), ep2(t), ep3(t), ep4(t);

        bm.bindings[321] = ep2.var(322);

        const expr* A = ep3.cons(ep3.var(321), ep3.atom("x"));
        const expr* B = ep4.cons(ep4.atom("a"), ep4.atom("x"));

        assert(bm.unify(A, B));
        assert(bm.bindings.size() == 2);
        assert(bm.bindings.count(322) == 1);

        assert(bm.whnf(ep1.var(321)) == ep4.atom("a"));  // ep1 allocation, same chain
        assert(bm.whnf(ep3.var(322)) == ep4.atom("a"));

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
        bm.bindings[325] = ep2.cons(ep2.atom("L"), ep2.atom("R"));

        // Structure A: spine from ep6; var(325) ref from ep1, var(326) ref from ep3
        const expr* A = ep6.cons(ep1.var(325), ep3.var(326));

        // Structure B: outer spine from ep5; inner cons from ep4; atoms from ep4
        const expr* B = ep5.cons(
            ep4.cons(ep4.atom("L"), ep4.atom("R")),  // same structure as the pre-binding
            ep5.atom("result")
        );

        assert(bm.unify(A, B));
        // lhs: var(325) → ep2.cons("L","R"); ep4.cons("L","R") — different alloc, same shape
        //   "L"=="L" ✓  "R"=="R" ✓  (no new bindings for atoms)
        // rhs: var(326) unbound → bind 326 → ep5.atom("result")
        assert(bm.bindings.size() == 2);  // pre-binding 325 + new 326
        assert(bm.bindings.count(326) == 1);
        assert(bm.whnf(ep1.var(325)) == ep2.cons(ep2.atom("L"), ep2.atom("R")));
        assert(bm.whnf(ep3.var(326)) == ep5.atom("result"));

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
        
        const expr* original = pool.atom("test");
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
        
        const expr* a = pool.atom("a");
        const expr* b = pool.atom("b");
        const expr* original = pool.cons(a, b);
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
        const expr* original = pool.cons(v1, v2);
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied != original);  // Different cons (different vars)
        assert(std::holds_alternative<expr::cons>(copied->content));
        
        const expr::cons& copied_cons = std::get<expr::cons>(copied->content);
        assert(std::get<expr::var>(copied_cons.lhs->content).index == 0);
        assert(std::get<expr::var>(copied_cons.rhs->content).index == 1);
        
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
        assert(pool.exprs.count(*copied_cons.lhs) == 1);
        assert(pool.exprs.count(*copied_cons.rhs) == 1);
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
        const expr* original = pool.cons(v, v);
        assert(pool.size() == 2);  // var(5) and cons
        
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        const expr::cons& copied_cons = std::get<expr::cons>(copied->content);
        assert(std::get<expr::var>(copied_cons.lhs->content).index == 0);
        assert(std::get<expr::var>(copied_cons.rhs->content).index == 0);
        assert(copied_cons.lhs == copied_cons.rhs);  // Same variable
        
        assert(var_map.size() == 1);
        assert(var_map.at(5) == 0);
        assert(vars.index == 1);  // Only one fresh variable created
        
        // Pool: var(5), original cons, var(0), new cons
        assert(pool.size() == 4);
        assert(pool.exprs.size() == 4);
        assert(pool.exprs.count(*v) == 1);
        assert(pool.exprs.count(*original) == 1);
        assert(pool.exprs.count(*copied_cons.lhs) == 1);
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
        
        const expr* a = pool.atom("atom");
        const expr* v = pool.var(7);
        const expr* inner = pool.cons(a, v);
        const expr* original = pool.cons(inner, a);
        std::map<uint32_t, uint32_t> var_map;
        
        const expr* copied = copy(original, var_map);
        
        assert(copied != original);
        const expr::cons& outer_cons = std::get<expr::cons>(copied->content);
        assert(outer_cons.rhs == a);  // Atom unchanged
        
        const expr::cons& inner_cons = std::get<expr::cons>(outer_cons.lhs->content);
        assert(inner_cons.lhs == a);  // Atom unchanged
        assert(std::get<expr::var>(inner_cons.rhs->content).index == 0);
        
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
        const expr* c1 = pool.cons(v1, v2);
        const expr* c2 = pool.cons(c1, v3);
        const expr* original = pool.cons(c2, v1);  // v1 appears twice
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 3);
        assert(var_map.at(10) == 0);
        assert(var_map.at(20) == 1);
        assert(var_map.at(30) == 2);
        assert(vars.index == 3);
        
        // Verify structure
        const expr::cons& top = std::get<expr::cons>(copied->content);
        assert(std::get<expr::var>(top.rhs->content).index == 0);  // v1 mapped to 0
        
        const expr::cons& middle = std::get<expr::cons>(top.lhs->content);
        assert(std::get<expr::var>(middle.rhs->content).index == 2);  // v3 mapped to 2
        
        const expr::cons& bottom = std::get<expr::cons>(middle.lhs->content);
        assert(std::get<expr::var>(bottom.lhs->content).index == 0);  // v1 mapped to 0
        assert(std::get<expr::var>(bottom.rhs->content).index == 1);  // v2 mapped to 1
        
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
        const expr* original = pool.cons(v5, v10);
        
        // Pre-populate map
        std::map<uint32_t, uint32_t> var_map;
        var_map[5] = 100;  // Map 5 to 100
        
        const expr* copied = copy(original, var_map);
        
        // v5 should use existing mapping, v10 should get fresh
        assert(var_map.size() == 2);
        assert(var_map.at(5) == 100);  // Unchanged
        assert(var_map.at(10) == 0);  // Fresh variable
        
        const expr::cons& copied_cons = std::get<expr::cons>(copied->content);
        assert(std::get<expr::var>(copied_cons.lhs->content).index == 100);
        assert(std::get<expr::var>(copied_cons.rhs->content).index == 0);
        
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
        const expr* expr1 = pool.cons(v1, v2);
        const expr* expr2 = pool.cons(v2, v1);  // Swapped
        
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
        const expr::cons& cons1 = std::get<expr::cons>(copied1->content);
        const expr::cons& cons2 = std::get<expr::cons>(copied2->content);
        assert(std::get<expr::var>(cons1.lhs->content).index == 0);
        assert(std::get<expr::var>(cons1.rhs->content).index == 1);
        assert(std::get<expr::var>(cons2.lhs->content).index == 1);
        assert(std::get<expr::var>(cons2.rhs->content).index == 0);
        
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
        const expr* a = pool.atom("a");
        const expr* left = pool.cons(v1, a);
        const expr* right = pool.cons(v2, v1);
        const expr* original = pool.cons(left, right);
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 2);
        assert(var_map.at(1) == 0);
        assert(var_map.at(2) == 1);
        assert(vars.index == 2);
        
        // Verify structure: cons(cons(var(0), atom("a")), cons(var(1), var(0)))
        const expr::cons& top = std::get<expr::cons>(copied->content);
        
        const expr::cons& left_cons = std::get<expr::cons>(top.lhs->content);
        assert(std::get<expr::var>(left_cons.lhs->content).index == 0);
        assert(left_cons.rhs == a);
        
        const expr::cons& right_cons = std::get<expr::cons>(top.rhs->content);
        assert(std::get<expr::var>(right_cons.lhs->content).index == 1);
        assert(std::get<expr::var>(right_cons.rhs->content).index == 0);
        
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
        const expr* original = pool.cons(v, pool.atom("x"));
        
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
        const expr* a = pool.atom("a");
        const expr* b = pool.atom("b");
        const expr* v5 = pool.var(5);
        const expr* inner = pool.cons(v5, b);
        const expr* original = pool.cons(a, inner);
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 1);
        assert(var_map.at(5) == 0);
        assert(vars.index == 1);
        
        // Verify structure
        const expr::cons& top = std::get<expr::cons>(copied->content);
        assert(top.lhs == a);  // Atom unchanged
        
        const expr::cons& inner_cons = std::get<expr::cons>(top.rhs->content);
        assert(std::get<expr::var>(inner_cons.lhs->content).index == 0);
        assert(inner_cons.rhs == b);  // Atom unchanged
        
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
        const expr* left = pool.cons(v1, v2);
        const expr* right = pool.cons(v2, v1);
        const expr* original = pool.cons(left, right);
        
        std::map<uint32_t, uint32_t> var_map;
        const expr* copied = copy(original, var_map);
        
        assert(var_map.size() == 2);
        assert(var_map.at(1) == 0);
        assert(var_map.at(2) == 1);
        assert(vars.index == 2);
        
        // Verify all occurrences use consistent mapping
        const expr::cons& top = std::get<expr::cons>(copied->content);
        const expr::cons& left_cons = std::get<expr::cons>(top.lhs->content);
        const expr::cons& right_cons = std::get<expr::cons>(top.rhs->content);
        
        assert(std::get<expr::var>(left_cons.lhs->content).index == 0);  // v1
        assert(std::get<expr::var>(left_cons.rhs->content).index == 1);  // v2
        assert(std::get<expr::var>(right_cons.lhs->content).index == 1);  // v2
        assert(std::get<expr::var>(right_cons.rhs->content).index == 0);  // v1
        
        t.pop();
    }
    
    // Test 17: Copy atom-only structure
    {
        trail t;
        sequencer vars(t);
        expr_pool pool(t);
        copier copy(vars, pool);
        
        t.push();
        
        const expr* a1 = pool.atom("a1");
        const expr* a2 = pool.atom("a2");
        const expr* a3 = pool.atom("a3");
        const expr* c1 = pool.cons(a1, a2);
        const expr* original = pool.cons(c1, a3);
        
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
        const expr* a = pool.atom("a");
        const expr* original = pool.cons(v1, a);
        
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
        const expr* original = pool.cons(v0, v1);
        
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
        
        const expr* a = pool.atom("test");
        const expr* result = norm(a);
        
        assert(result == a);
        assert(std::holds_alternative<expr::atom>(result->content));
        
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
        const expr* a = pool.atom("hello");
        bm.bind(1, a);
        
        const expr* result = norm(v);
        
        assert(result == a);
        assert(std::holds_alternative<expr::atom>(result->content));
        assert(std::get<expr::atom>(result->content).value == "hello");
        
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
        const expr* a = pool.atom("end");
        
        bm.bind(1, v2);
        bm.bind(2, v3);
        bm.bind(3, a);
        
        const expr* result = norm(v1);
        
        assert(result == a);
        assert(std::get<expr::atom>(result->content).value == "end");
        
        t.pop();
    }
    
    // Test 5: Normalize cons with atoms
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* a1 = pool.atom("left");
        const expr* a2 = pool.atom("right");
        const expr* c = pool.cons(a1, a2);
        
        const expr* result = norm(c);
        
        assert(result == c);
        const expr::cons& result_cons = std::get<expr::cons>(result->content);
        assert(result_cons.lhs == a1);
        assert(result_cons.rhs == a2);
        
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
        const expr* c = pool.cons(v1, v2);
        
        const expr* result = norm(c);
        
        assert(result == c);
        const expr::cons& result_cons = std::get<expr::cons>(result->content);
        assert(result_cons.lhs == v1);
        assert(result_cons.rhs == v2);
        
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
        const expr* a1 = pool.atom("a");
        const expr* a2 = pool.atom("b");
        const expr* c = pool.cons(v1, v2);
        
        bm.bind(1, a1);
        bm.bind(2, a2);
        
        const expr* result = norm(c);
        
        assert(result != c);
        const expr::cons& result_cons = std::get<expr::cons>(result->content);
        assert(result_cons.lhs == a1);
        assert(result_cons.rhs == a2);
        
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
        const expr* a = pool.atom("bound");
        const expr* c = pool.cons(v1, v2);
        
        bm.bind(1, a);
        
        const expr* result = norm(c);
        
        const expr::cons& result_cons = std::get<expr::cons>(result->content);
        assert(result_cons.lhs == a);
        assert(result_cons.rhs == v2);
        
        t.pop();
    }
    
    // Test 9: Normalize nested cons with all atoms
    {
        trail t;
        expr_pool pool(t);
        bind_map bm(t);
        normalizer norm(pool, bm);
        
        t.push();
        
        const expr* a1 = pool.atom("a");
        const expr* a2 = pool.atom("b");
        const expr* a3 = pool.atom("c");
        const expr* inner = pool.cons(a1, a2);
        const expr* outer = pool.cons(inner, a3);
        
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
        const expr* a1 = pool.atom("x");
        const expr* a2 = pool.atom("y");
        const expr* inner = pool.cons(v1, v2);
        const expr* outer = pool.cons(inner, a1);
        
        bm.bind(1, a1);
        bm.bind(2, a2);
        
        const expr* result = norm(outer);
        
        const expr::cons& outer_cons = std::get<expr::cons>(result->content);
        assert(outer_cons.rhs == a1);
        
        const expr::cons& inner_cons = std::get<expr::cons>(outer_cons.lhs->content);
        assert(inner_cons.lhs == a1);
        assert(inner_cons.rhs == a2);
        
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
        const expr* a1 = pool.atom("left");
        const expr* a2 = pool.atom("right");
        const expr* c = pool.cons(a1, a2);
        
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
        const expr* a1 = pool.atom("a");
        const expr* a2 = pool.atom("b");
        const expr* c = pool.cons(v2, v3);
        
        bm.bind(1, c);
        bm.bind(2, a1);
        bm.bind(3, a2);
        
        const expr* result = norm(v1);
        
        const expr::cons& result_cons = std::get<expr::cons>(result->content);
        assert(result_cons.lhs == a1);
        assert(result_cons.rhs == a2);
        
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
        const expr* a = pool.atom("atom");
        
        const expr* c1 = pool.cons(v1, v2);
        const expr* c2 = pool.cons(c1, v3);
        
        bm.bind(1, a);
        bm.bind(2, a);
        bm.bind(3, a);
        
        const expr* result = norm(c2);
        
        const expr::cons& top = std::get<expr::cons>(result->content);
        assert(top.rhs == a);
        
        const expr::cons& bottom = std::get<expr::cons>(top.lhs->content);
        assert(bottom.lhs == a);
        assert(bottom.rhs == a);
        
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
        const expr* a = pool.atom("shared");
        const expr* c = pool.cons(v, v);
        
        bm.bind(1, a);
        
        const expr* result = norm(c);
        
        const expr::cons& result_cons = std::get<expr::cons>(result->content);
        assert(result_cons.lhs == a);
        assert(result_cons.rhs == a);
        assert(result_cons.lhs == result_cons.rhs);
        
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
        const expr* a1 = pool.atom("first");
        const expr* a2 = pool.atom("second");
        
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
        const expr* a = pool.atom("atom");
        
        const expr* c1 = pool.cons(v1, a);
        const expr* c2 = pool.cons(v2, v3);
        const expr* c3 = pool.cons(c1, c2);
        
        bm.bind(1, a);
        bm.bind(2, v4);
        bm.bind(4, a);
        // v3 remains unbound
        
        const expr* result = norm(c3);
        
        const expr::cons& top = std::get<expr::cons>(result->content);
        
        const expr::cons& left = std::get<expr::cons>(top.lhs->content);
        assert(left.lhs == a);
        assert(left.rhs == a);
        
        const expr::cons& right = std::get<expr::cons>(top.rhs->content);
        assert(right.lhs == a);
        assert(right.rhs == v3);
        
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
        const expr* a = pool.atom("unified");
        const expr* c1 = pool.cons(v1, a);
        const expr* c2 = pool.cons(a, v2);
        
        bool unified = bm.unify(c1, c2);
        assert(unified);
        
        const expr* result1 = norm(c1);
        const expr* result2 = norm(c2);
        
        const expr::cons& r1 = std::get<expr::cons>(result1->content);
        const expr::cons& r2 = std::get<expr::cons>(result2->content);
        
        assert(r1.lhs == a);
        assert(r1.rhs == a);
        assert(r2.lhs == a);
        assert(r2.rhs == a);
        
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
        const expr* a = pool.atom("final");
        
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
        const expr* a1 = pool.atom("a1");
        const expr* a2 = pool.atom("a2");
        
        const expr* inner1 = pool.cons(v1, a1);
        const expr* inner2 = pool.cons(v2, v3);
        const expr* outer = pool.cons(inner1, inner2);
        
        bm.bind(1, a2);
        bm.bind(3, a1);
        // v2 remains unbound
        
        const expr* result = norm(outer);
        
        const expr::cons& top = std::get<expr::cons>(result->content);
        
        const expr::cons& left = std::get<expr::cons>(top.lhs->content);
        assert(left.lhs == a2);
        assert(left.rhs == a1);
        
        const expr::cons& right = std::get<expr::cons>(top.rhs->content);
        assert(right.lhs == v2);
        assert(right.rhs == a1);
        
        t.pop();
    }
}

void test_a01_goal_adder_constructor() {
    // Test 1: Basic construction with empty stores and empty database
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        std::vector<rule> database;
        
        a01_goal_adder adder(goals, candidates, database);
        
        assert(adder.goals.size() == 0);
        assert(adder.candidates.size() == 0);
        assert(adder.database.size() == 0);
    }
    
    // Test 2: Construction with empty stores but non-empty database
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        expr::atom a1{"p"};
        expr e1{a1};
        rule r1{&e1, {}};
        
        expr::atom a2{"q"};
        expr e2{a2};
        rule r2{&e2, {}};
        
        std::vector<rule> database = {r1, r2};
        
        a01_goal_adder adder(goals, candidates, database);
        
        assert(adder.goals.size() == 0);
        assert(adder.candidates.size() == 0);
        assert(adder.database.size() == 2);
    }
    
    // Test 3: Construction with non-empty stores
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        goal_lineage g1{nullptr, 1};
        expr::atom a1{"test"};
        expr e1{a1};
        goals.insert({&g1, &e1});
        candidates.insert({&g1, 0});
        candidates.insert({&g1, 1});
        
        expr::atom a2{"p"};
        expr e2{a2};
        rule r1{&e2, {}};
        std::vector<rule> database = {r1};
        
        a01_goal_adder adder(goals, candidates, database);
        
        assert(adder.goals.size() == 1);
        assert(adder.candidates.size() == 2);
        assert(adder.database.size() == 1);
    }
    
    // Test 4: Verify references are stored correctly (modification propagates)
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        std::vector<rule> database;
        
        a01_goal_adder adder(goals, candidates, database);
        
        // Add to the stores directly
        goal_lineage g1{nullptr, 1};
        expr::atom a1{"test"};
        expr e1{a1};
        goals.insert({&g1, &e1});
        
        // Verify the adder sees the change
        assert(adder.goals.size() == 1);
    }
}

void test_a01_goal_adder() {
    // Test 1: Add goal with empty database - no candidates added
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        std::vector<rule> database;
        
        a01_goal_adder adder(goals, candidates, database);
        
        goal_lineage g1{nullptr, 1};
        expr::atom a1{"goal1"};
        expr e1{a1};
        
        adder(&g1, &e1);
        
        assert(goals.size() == 1);
        assert(goals.count(&g1) == 1);
        assert(goals.at(&g1) == &e1);
        assert(candidates.size() == 0);  // No candidates since database is empty
    }
    
    // Test 2: Add goal with single rule in database
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        expr::atom a_rule{"rule_head"};
        expr e_rule{a_rule};
        rule r1{&e_rule, {}};
        std::vector<rule> database = {r1};
        
        a01_goal_adder adder(goals, candidates, database);
        
        goal_lineage g1{nullptr, 1};
        expr::atom a1{"goal1"};
        expr e1{a1};
        
        adder(&g1, &e1);
        
        assert(goals.size() == 1);
        assert(goals.at(&g1) == &e1);
        assert(candidates.size() == 1);  // 1 candidate for 1 rule
        assert(candidates.count(&g1) == 1);
        
        // Verify the candidate is rule index 0
        auto it = candidates.find(&g1);
        assert(it != candidates.end());
        assert(it->second == 0);
    }
    
    // Test 3: Add goal with multiple rules - all should become candidates
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        // Create simple rules
        expr::atom a1{"p"};
        expr e1{a1};
        rule r1{&e1, {}};
        
        expr::atom a2{"q"};
        expr e2{a2};
        rule r2{&e2, {}};
        
        expr::atom a3{"r"};
        expr e3{a3};
        rule r3{&e3, {}};
        
        std::vector<rule> database = {r1, r2, r3};
        
        a01_goal_adder adder(goals, candidates, database);
        
        goal_lineage g1{nullptr, 10};
        expr::atom goal_atom{"test_goal"};
        expr goal_expr{goal_atom};
        
        adder(&g1, &goal_expr);
        
        assert(goals.size() == 1);
        assert(goals.at(&g1) == &goal_expr);
        assert(candidates.size() == 3);  // All 3 rules are candidates
        assert(candidates.count(&g1) == 3);
        
        // Verify all rule indices (0, 1, 2) are present
        auto range = candidates.equal_range(&g1);
        std::vector<size_t> indices;
        for (auto it = range.first; it != range.second; ++it) {
            indices.push_back(it->second);
        }
        std::sort(indices.begin(), indices.end());
        assert(indices.size() == 3);
        assert(indices[0] == 0);
        assert(indices[1] == 1);
        assert(indices[2] == 2);
    }
    
    // Test 4: Add goal with complex rules (rules with body)
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        // Rule 1: p :- q, r (complex)
        expr::atom a_p{"p"};
        expr e_p{a_p};
        expr::atom a_q{"q"};
        expr e_q{a_q};
        expr::atom a_r{"r"};
        expr e_r{a_r};
        rule r1{&e_p, {&e_q, &e_r}};
        
        // Rule 2: s (simple fact)
        expr::atom a_s{"s"};
        expr e_s{a_s};
        rule r2{&e_s, {}};
        
        std::vector<rule> database = {r1, r2};
        
        a01_goal_adder adder(goals, candidates, database);
        
        goal_lineage g1{nullptr, 5};
        expr::atom goal_atom{"my_goal"};
        expr goal_expr{goal_atom};
        
        adder(&g1, &goal_expr);
        
        assert(goals.size() == 1);
        assert(candidates.size() == 2);
        assert(candidates.count(&g1) == 2);
        
        // Verify both rule indices are present
        auto range = candidates.equal_range(&g1);
        std::vector<size_t> indices;
        for (auto it = range.first; it != range.second; ++it) {
            indices.push_back(it->second);
        }
        std::sort(indices.begin(), indices.end());
        assert(indices[0] == 0);
        assert(indices[1] == 1);
    }
    
    // Test 5: Add multiple different goals to same stores
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        expr::atom a1{"rule1"};
        expr e1{a1};
        rule r1{&e1, {}};
        
        expr::atom a2{"rule2"};
        expr e2{a2};
        rule r2{&e2, {}};
        
        std::vector<rule> database = {r1, r2};
        
        a01_goal_adder adder(goals, candidates, database);
        
        // Add first goal
        goal_lineage g1{nullptr, 1};
        expr::atom goal1_atom{"goal1"};
        expr goal1_expr{goal1_atom};
        adder(&g1, &goal1_expr);
        
        assert(goals.size() == 1);
        assert(candidates.size() == 2);
        
        // Add second goal
        goal_lineage g2{nullptr, 2};
        expr::atom goal2_atom{"goal2"};
        expr goal2_expr{goal2_atom};
        adder(&g2, &goal2_expr);
        
        assert(goals.size() == 2);
        assert(candidates.size() == 4);  // 2 goals * 2 rules
        assert(candidates.count(&g1) == 2);
        assert(candidates.count(&g2) == 2);
        assert(goals.at(&g1) == &goal1_expr);
        assert(goals.at(&g2) == &goal2_expr);
    }
    
    // Test 6: Add goal with goal_lineage that has parent
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        expr::atom a1{"rule1"};
        expr e1{a1};
        rule r1{&e1, {}};
        std::vector<rule> database = {r1};
        
        a01_goal_adder adder(goals, candidates, database);
        
        // Create goal_lineage with parent
        resolution_lineage parent{nullptr, 5};
        goal_lineage g1{&parent, 10};
        expr::atom goal_atom{"child_goal"};
        expr goal_expr{goal_atom};
        
        adder(&g1, &goal_expr);
        
        assert(goals.size() == 1);
        assert(goals.at(&g1) == &goal_expr);
        assert(candidates.size() == 1);
        assert(candidates.count(&g1) == 1);
    }
    
    // Test 7: Add goal with complex nested expr
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        expr::atom a1{"rule1"};
        expr e1{a1};
        rule r1{&e1, {}};
        
        expr::atom a2{"rule2"};
        expr e2{a2};
        rule r2{&e2, {}};
        
        std::vector<rule> database = {r1, r2};
        
        a01_goal_adder adder(goals, candidates, database);
        
        // Create nested goal expression: cons(var(0), atom("test"))
        expr::var v1{0};
        expr e_v1{v1};
        expr::atom a_test{"test"};
        expr e_test{a_test};
        expr::cons c1{&e_v1, &e_test};
        expr goal_expr{c1};
        
        goal_lineage g1{nullptr, 7};
        
        adder(&g1, &goal_expr);
        
        assert(goals.size() == 1);
        assert(goals.at(&g1) == &goal_expr);
        assert(candidates.size() == 2);
    }
    
    // Test 8: Large database - verify all indices are added
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        // Create 10 rules
        expr::atom atoms[10] = {
            expr::atom{"r0"}, expr::atom{"r1"}, expr::atom{"r2"}, expr::atom{"r3"}, expr::atom{"r4"},
            expr::atom{"r5"}, expr::atom{"r6"}, expr::atom{"r7"}, expr::atom{"r8"}, expr::atom{"r9"}
        };
        expr exprs[10] = {
            expr{atoms[0]}, expr{atoms[1]}, expr{atoms[2]}, expr{atoms[3]}, expr{atoms[4]},
            expr{atoms[5]}, expr{atoms[6]}, expr{atoms[7]}, expr{atoms[8]}, expr{atoms[9]}
        };
        rule rules[10] = {
            rule{&exprs[0], {}}, rule{&exprs[1], {}}, rule{&exprs[2], {}}, rule{&exprs[3], {}}, rule{&exprs[4], {}},
            rule{&exprs[5], {}}, rule{&exprs[6], {}}, rule{&exprs[7], {}}, rule{&exprs[8], {}}, rule{&exprs[9], {}}
        };
        std::vector<rule> database = {
            rules[0], rules[1], rules[2], rules[3], rules[4],
            rules[5], rules[6], rules[7], rules[8], rules[9]
        };
        
        a01_goal_adder adder(goals, candidates, database);
        
        goal_lineage g1{nullptr, 100};
        expr::atom goal_atom{"big_goal"};
        expr goal_expr{goal_atom};
        
        adder(&g1, &goal_expr);
        
        assert(goals.size() == 1);
        assert(candidates.size() == 10);  // All 10 rules
        assert(candidates.count(&g1) == 10);
        
        // Verify all indices 0-9 are present
        auto range = candidates.equal_range(&g1);
        std::vector<size_t> indices;
        for (auto it = range.first; it != range.second; ++it) {
            indices.push_back(it->second);
        }
        std::sort(indices.begin(), indices.end());
        assert(indices.size() == 10);
        for (size_t i = 0; i < 10; ++i) {
            assert(indices[i] == i);
        }
    }
    
    // Test 9: Add multiple goals with same database
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        expr::atom a1{"rule1"};
        expr e1{a1};
        rule r1{&e1, {}};
        
        expr::atom a2{"rule2"};
        expr e2{a2};
        rule r2{&e2, {}};
        
        expr::atom a3{"rule3"};
        expr e3{a3};
        rule r3{&e3, {}};
        
        std::vector<rule> database = {r1, r2, r3};
        
        a01_goal_adder adder(goals, candidates, database);
        
        // Add first goal
        goal_lineage g1{nullptr, 1};
        expr::atom goal1_atom{"goal1"};
        expr goal1_expr{goal1_atom};
        adder(&g1, &goal1_expr);
        
        assert(goals.size() == 1);
        assert(candidates.size() == 3);
        assert(candidates.count(&g1) == 3);
        
        // Add second goal
        goal_lineage g2{nullptr, 2};
        expr::atom goal2_atom{"goal2"};
        expr goal2_expr{goal2_atom};
        adder(&g2, &goal2_expr);
        
        assert(goals.size() == 2);
        assert(candidates.size() == 6);  // 2 goals * 3 rules = 6 total
        assert(candidates.count(&g1) == 3);
        assert(candidates.count(&g2) == 3);
        
        // Add third goal
        goal_lineage g3{nullptr, 3};
        expr::atom goal3_atom{"goal3"};
        expr goal3_expr{goal3_atom};
        adder(&g3, &goal3_expr);
        
        assert(goals.size() == 3);
        assert(candidates.size() == 9);  // 3 goals * 3 rules = 9 total
        assert(candidates.count(&g1) == 3);
        assert(candidates.count(&g2) == 3);
        assert(candidates.count(&g3) == 3);
    }
    
    // Test 10: Verify candidates are correct indices for complex database
    {
        std::map<const goal_lineage*, const expr*> goals;
        std::multimap<const goal_lineage*, size_t> candidates;
        
        // Create varied rules: some simple, some with bodies
        expr::atom a1{"fact"};
        expr e1{a1};
        rule r1{&e1, {}};  // Simple fact
        
        expr::atom a2{"head"};
        expr e2{a2};
        expr::atom a3{"body1"};
        expr e3{a3};
        expr::atom a4{"body2"};
        expr e4{a4};
        rule r2{&e2, {&e3, &e4}};  // Rule with body
        
        expr::var v1{0};
        expr e5{v1};
        expr::atom a5{"pred"};
        expr e6{a5};
        expr::cons c1{&e6, &e5};
        expr e7{c1};
        rule r3{&e7, {}};  // Rule with nested expr
        
        std::vector<rule> database = {r1, r2, r3};
        
        a01_goal_adder adder(goals, candidates, database);
        
        goal_lineage g1{nullptr, 42};
        expr::atom goal_atom{"complex_goal"};
        expr::var v_goal{1};
        expr e_var{v_goal};
        expr e_atom{goal_atom};
        expr::cons c_goal{&e_atom, &e_var};
        expr goal_expr{c_goal};
        
        adder(&g1, &goal_expr);
        
        assert(goals.size() == 1);
        assert(goals.at(&g1) == &goal_expr);
        assert(candidates.size() == 3);
        
        // Extract and verify all candidate indices
        auto range = candidates.equal_range(&g1);
        std::vector<size_t> indices;
        for (auto it = range.first; it != range.second; ++it) {
            indices.push_back(it->second);
        }
        std::sort(indices.begin(), indices.end());
        assert(indices.size() == 3);
        assert(indices[0] == 0);
        assert(indices[1] == 1);
        assert(indices[2] == 2);
    }
}

void test_a01_goal_resolver_constructor() {
    // Test 1: Basic construction with all required references
    {
        trail t;
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        a01_database db;
        a01_avoidance_store as;
        a01_goal_adder ga(gs, cs, db);
        
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        assert(&resolver.rs == &rs);
        assert(&resolver.gs == &gs);
        assert(&resolver.cs == &cs);
        assert(&resolver.db == &db);
        assert(&resolver.cp == &cp);
        assert(&resolver.bm == &bm);
        assert(&resolver.lp == &lp);
        assert(&resolver.ga == &ga);
        assert(&resolver.as == &as);
    }
    
    // Test 2: Construction with non-empty stores including avoidances
    {
        trail t;
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        a01_avoidance_store as;
        
        // Add some initial data to resolution store
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(g0, 0);
        rs.insert(rl0);
        
        // Add some initial data to goal store
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* e1 = ep.atom("test");
        gs.insert({g1, e1});
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        
        expr::atom a1{"rule1"};
        expr rule_expr{a1};
        rule r1{&rule_expr, {}};
        a01_database db = {r1};
        
        // Pre-populate avoidance store with some avoidances
        a01_decision_store avoidance1;
        avoidance1.insert(rl0);
        as.insert(avoidance1);
        
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        a01_decision_store avoidance2;
        avoidance2.insert(rl0);
        avoidance2.insert(rl2);
        as.insert(avoidance2);
        
        a01_goal_adder ga(gs, cs, db);
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        assert(resolver.rs.size() == 1);
        assert(resolver.gs.size() == 1);
        assert(resolver.cs.size() == 2);
        assert(resolver.db.size() == 1);
        assert(resolver.as.size() == 2);
    }
}

void test_a01_goal_resolver() {
    // Test 1: Resolve with simple fact (empty body) - no new goals
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: single fact "p"
        const expr* p_expr = ep.atom("p");
        rule r_fact{p_expr, {}};
        a01_database db = {r_fact};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        
        // Pre-populate avoidance store: single avoidance containing the rl we're about to create
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl_expected = lp.resolution(g1, 0);
        
        a01_decision_store avoidance1;
        avoidance1.insert(rl_expected);
        as.insert(avoidance1);
        
        assert(as.size() == 1);
        assert(as.begin()->size() == 1);
        assert(as.begin()->count(rl_expected) == 1);
        
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal "p" using goal_adder
        const expr* goal_p = ep.atom("p");
        ga(g1, goal_p);
        
        // Pre-resolution assertions: goal and candidates exist
        assert(gs.size() == 1);
        assert(cs.size() == 1);
        assert(gs.count(g1) == 1);
        assert(cs.count(g1) == 1);
        assert(gs.at(g1) == goal_p);
        assert(rs.size() == 0); // No resolutions yet
        
        // Check goal lineage structure before resolution
        assert(g1->parent == nullptr);
        assert(g1->idx == 1);
        
        // Store initial bind_map size to check unification
        size_t bindings_before = bm.bindings.size();
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        
        // Get the resolution lineage that was created
        const resolution_lineage* rl = lp.resolution(g1, 0);
        assert(rl == rl_expected); // Should be same interned instance
        
        // Check resolution lineage structure
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Check resolution was added to resolution store
        assert(rs.size() == 1);
        assert(rs.count(rl) == 1);
        
        // CRITICAL: Verify rl was erased from all avoidances
        assert(as.size() == 1);
        assert(as.begin()->count(rl) == 0); // rl should be removed
        assert(as.begin()->size() == 0);    // Avoidance now empty
        
        // Check goal was removed from goal store
        assert(gs.size() == 0);
        assert(gs.count(g1) == 0);
        
        // Check candidates were removed from candidate store
        assert(cs.size() == 0);
        assert(cs.count(g1) == 0);
        
        // Check unification occurred (p unifies with p trivially, may not add bindings)
        // For atoms, unification should succeed without new bindings
        size_t bindings_after = bm.bindings.size();
        assert(bindings_after >= bindings_before);
        
        // Check no new goals were added (empty body)
        assert(gs.size() == 0);
        assert(cs.size() == 0);
        
        // Check lineage pool internals
        assert(lp.resolution_lineages.count(*rl) == 1);
    }
    
    // Test 2: Resolve with rule with single body clause
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p :- q"
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        rule r1{p_expr, {q_expr}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        
        // Pre-populate avoidance store with mixed avoidances
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl_expected = lp.resolution(g1, 0);
        
        // Create some unrelated resolutions for avoidances
        const goal_lineage* g_other1 = lp.goal(nullptr, 100);
        const goal_lineage* g_other2 = lp.goal(nullptr, 200);
        const resolution_lineage* rl_other1 = lp.resolution(g_other1, 0);
        const resolution_lineage* rl_other2 = lp.resolution(g_other2, 1);
        
        // Avoidance 1: contains rl_expected + another resolution
        a01_decision_store avoidance1;
        avoidance1.insert(rl_expected);
        avoidance1.insert(rl_other1);
        as.insert(avoidance1);
        
        // Avoidance 2: contains only rl_expected
        a01_decision_store avoidance2;
        avoidance2.insert(rl_expected);
        as.insert(avoidance2);
        
        // Avoidance 3: doesn't contain rl_expected at all
        a01_decision_store avoidance3;
        avoidance3.insert(rl_other1);
        avoidance3.insert(rl_other2);
        as.insert(avoidance3);
        
        assert(as.size() == 3);
        
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal "p" using goal_adder
        const expr* goal_p = ep.atom("p");
        ga(g1, goal_p);
        
        // Pre-resolution checks
        assert(gs.size() == 1);
        assert(gs.at(g1) == goal_p);
        assert(cs.size() == 1); // One rule in database
        assert(rs.size() == 0); // No resolutions yet
        assert(g1->parent == nullptr);
        assert(g1->idx == 1);
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        
        // Get the resolution lineage that was created
        const resolution_lineage* rl = lp.resolution(g1, 0);
        assert(rl == rl_expected);
        
        // Check resolution lineage structure
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Check resolution was added to resolution store
        assert(rs.size() == 1);
        assert(rs.count(rl) == 1);
        
        // Goal "p" should be removed from stores
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution lineage should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // One new goal "q" should be added as child of resolution
        assert(gs.size() == 1);
        const goal_lineage* child_g = lp.goal(rl, 0);
        assert(gs.count(child_g) == 1);
        assert(child_g->parent == rl);
        assert(child_g->idx == 0);
        
        // Check the goal expression is "q"
        const expr* child_expr = gs.at(child_g);
        assert(child_expr != nullptr);
        assert(std::holds_alternative<expr::atom>(child_expr->content));
        assert(std::get<expr::atom>(child_expr->content).value == std::string("q"));
        
        // Check it's the weak head normal form version
        const expr* normalized = bm.whnf(child_expr);
        assert(normalized == child_expr);
        
        // New goal should have been added via goal_adder, so it has candidates
        assert(cs.size() == 1);
        assert(cs.count(child_g) == 1);
        
        // Verify child goal is in lineage pool
        assert(lp.goal_lineages.count(*child_g) == 1);
        
        // CRITICAL: Verify avoidance store state after resolution
        assert(as.size() == 3);
        
        // Count avoidances by size and content (order not guaranteed by std::set)
        int empty_count = 0;
        int size_one_count = 0;
        int size_two_count = 0;
        bool found_rl_other1_only = false;
        bool found_rl_other1_and_rl_other2 = false;
        
        for (const auto& avoidance : as) {
            assert(avoidance.count(rl) == 0); // rl should be gone from all
            
            if (avoidance.size() == 0) {
                empty_count++;
            } else if (avoidance.size() == 1) {
                size_one_count++;
                if (avoidance.count(rl_other1) == 1) {
                    found_rl_other1_only = true;
                }
            } else if (avoidance.size() == 2) {
                size_two_count++;
                if (avoidance.count(rl_other1) == 1 && avoidance.count(rl_other2) == 1) {
                    found_rl_other1_and_rl_other2 = true;
                }
            }
        }
        
        // Verify we have the expected avoidances
        assert(empty_count == 1);      // One became empty (was {rl})
        assert(size_one_count == 1);   // One has {rl_other1} (was {rl, rl_other1})
        assert(size_two_count == 1);   // One unchanged {rl_other1, rl_other2}
        assert(found_rl_other1_only);
        assert(found_rl_other1_and_rl_other2);
    }
    
    // Test 3: Resolve with rule with multiple body clauses
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p :- q, r, s"
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        const expr* s_expr = ep.atom("s");
        rule r1{p_expr, {q_expr, r_expr, s_expr}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        
        // Empty avoidance store for this test (baseline)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal "p" using goal_adder
        const expr* goal_p = ep.atom("p");
        ga(g1, goal_p);
        
        // Pre-resolution state
        assert(gs.size() == 1);
        assert(cs.size() == 1);
        assert(gs.at(g1) == goal_p);
        assert(as.size() == 0); // Empty avoidance store
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Goal "p" should be removed
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Three new goals should be added: q, r, s
        assert(gs.size() == 3);
        const goal_lineage* child_g0 = lp.goal(rl, 0);
        const goal_lineage* child_g1 = lp.goal(rl, 1);
        const goal_lineage* child_g2 = lp.goal(rl, 2);
        
        // Check all children are in goal store
        assert(gs.count(child_g0) == 1);
        assert(gs.count(child_g1) == 1);
        assert(gs.count(child_g2) == 1);
        
        // Check lineage structure for each child
        assert(child_g0->parent == rl);
        assert(child_g0->idx == 0);
        assert(child_g1->parent == rl);
        assert(child_g1->idx == 1);
        assert(child_g2->parent == rl);
        assert(child_g2->idx == 2);
        
        // Check goal expressions match body literals
        const expr* expr0 = gs.at(child_g0);
        const expr* expr1 = gs.at(child_g1);
        const expr* expr2 = gs.at(child_g2);
        
        assert(std::holds_alternative<expr::atom>(expr0->content));
        assert(std::get<expr::atom>(expr0->content).value == std::string("q"));
        assert(std::holds_alternative<expr::atom>(expr1->content));
        assert(std::get<expr::atom>(expr1->content).value == std::string("r"));
        assert(std::holds_alternative<expr::atom>(expr2->content));
        assert(std::get<expr::atom>(expr2->content).value == std::string("s"));
        
        // Each new goal should have candidates (via goal_adder)
        assert(cs.size() == 3);
        assert(cs.count(child_g0) == 1);
        assert(cs.count(child_g1) == 1);
        assert(cs.count(child_g2) == 1);
        
        // All children should be in lineage pool
        assert(lp.goal_lineages.count(*child_g0) == 1);
        assert(lp.goal_lineages.count(*child_g1) == 1);
        assert(lp.goal_lineages.count(*child_g2) == 1);
        
        // Avoidance store should still be empty
        assert(as.size() == 0);
    }
    
    // Test 4: Resolve with variable unification - VERIFY COPYING AND RENAMING
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(X) :- q(X)" where X is an original variable
        const expr* var_x = ep.var(seq());
        uint32_t original_var_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        const expr* q_x = ep.cons(ep.atom("q"), var_x);
        rule r1{p_x, {q_x}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        
        // Pre-populate: avoidance containing the rl about to be created
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl_expected = lp.resolution(g1, 0);
        a01_decision_store avoidance1;
        avoidance1.insert(rl_expected);
        as.insert(avoidance1);
        
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Record sequencer state before resolution
        uint32_t seq_before = seq.index;
        
        // Add goal "p(a)" using goal_adder
        const expr* atom_a = ep.atom("a");
        const expr* goal_p_a = ep.cons(ep.atom("p"), atom_a);
        ga(g1, goal_p_a);
        
        // Pre-resolution checks
        assert(gs.size() == 1);
        assert(gs.at(g1) == goal_p_a);
        assert(cs.size() == 1);
        size_t bindings_before = bm.bindings.size();
        
        // CRITICAL: Original variable should NOT be in bind_map yet
        assert(bm.bindings.count(original_var_idx) == 0);
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check sequencer advanced (new variable was allocated for copying)
        uint32_t seq_after = seq.index;
        assert(seq_after > seq_before); // At least one new variable was created
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Goal should be removed from stores
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // One new goal should be added: q(a) after substitution
        assert(gs.size() == 1);
        const goal_lineage* child_g = lp.goal(rl, 0);
        assert(gs.count(child_g) == 1);
        assert(child_g->parent == rl);
        assert(child_g->idx == 0);
        
        // Get the child goal expression (this is the copied body literal)
        const expr* child_expr = gs.at(child_g);
        assert(child_expr != nullptr);
        assert(std::holds_alternative<expr::cons>(child_expr->content));
        
        // Extract the variable from the copied expression before normalization
        const expr::cons& child_cons_raw = std::get<expr::cons>(child_expr->content);
        const expr* child_arg_raw = child_cons_raw.rhs;
        
        // This should be a RENAMED variable (not the original)
        if (std::holds_alternative<expr::var>(child_arg_raw->content)) {
            uint32_t renamed_var_idx = std::get<expr::var>(child_arg_raw->content).index;
            // CRITICAL: Renamed variable should be DIFFERENT from original
            assert(renamed_var_idx != original_var_idx);
            // CRITICAL: Renamed variable SHOULD be in bind_map (bound to "a")
            assert(bm.bindings.count(renamed_var_idx) == 1);
        }
        
        // Normalize to get final form after unification
        const expr* normalized_child = bm.whnf(child_expr);
        assert(std::holds_alternative<expr::cons>(normalized_child->content));
        
        // Should be q(...) where ... is either "a" or a variable bound to "a"
        const expr::cons& child_cons = std::get<expr::cons>(normalized_child->content);
        const expr* child_head = child_cons.lhs;
        const expr* child_tail = child_cons.rhs;
        assert(std::holds_alternative<expr::atom>(child_head->content));
        assert(std::get<expr::atom>(child_head->content).value == std::string("q"));
        
        // The argument should normalize to "a"
        const expr* normalized_arg = bm.whnf(child_tail);
        assert(std::holds_alternative<expr::atom>(normalized_arg->content));
        assert(std::get<expr::atom>(normalized_arg->content).value == std::string("a"));
        
        // Unification should have created bindings (the renamed var bound to "a")
        size_t bindings_after = bm.bindings.size();
        assert(bindings_after > bindings_before);
        
        // CRITICAL: Original variable should STILL NOT be in bind_map
        // (only the renamed copy should be bound)
        assert(bm.bindings.count(original_var_idx) == 0);
        
        // Verify child is in lineage pool
        assert(lp.goal_lineages.count(*child_g) == 1);
        
        // Verify candidate was added
        assert(cs.count(child_g) == 1);
        
        // CRITICAL: Verify rl was erased from avoidance
        assert(as.size() == 1);
        assert(as.begin()->count(rl) == 0);
        assert(as.begin()->size() == 0);
    }
    
    // Test 5: Multiple goals with multiple candidates - resolve only one
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: two facts
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        rule r1{p_expr, {}};
        rule r2{q_expr, {}};
        a01_database db = {r1, r2};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add two goals using goal_adder
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* goal1 = ep.atom("p");
        ga(g1, goal1);
        
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const expr* goal2 = ep.atom("q");
        ga(g2, goal2);
        
        // Pre-resolution: both goals present with all candidates
        assert(gs.size() == 2);
        assert(gs.at(g1) == goal1);
        assert(gs.at(g2) == goal2);
        assert(cs.size() == 4); // Each goal has 2 candidates
        assert(cs.count(g1) == 2);
        assert(cs.count(g2) == 2);
        
        // Resolve g1 with rule 0 (first rule "p")
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // g1 should be removed, g2 should remain
        assert(gs.count(g1) == 0);
        assert(gs.count(g2) == 1);
        assert(gs.size() == 1);
        assert(gs.at(g2) == goal2);
        
        // g1 candidates removed, g2 candidates remain
        assert(cs.count(g1) == 0);
        assert(cs.count(g2) == 2);
        assert(cs.size() == 2);
        
        // Resolution lineage should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // No new goals added (empty body)
        assert(gs.size() == 1);
        
        // Empty avoidance store (no avoidances for this test)
        assert(as.size() == 0);
    }
    
    // Test 6: Verify lineage structure after resolution (deep tree)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p :- q, r"
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        rule r1{p_expr, {q_expr, r_expr}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal with specific parent lineage using goal_adder
        const resolution_lineage* parent_rl = lp.resolution(nullptr, 5);
        const goal_lineage* g1 = lp.goal(parent_rl, 10);
        const expr* goal_p = ep.atom("p");
        ga(g1, goal_p);
        
        // Check pre-resolution lineage structure
        assert(g1->parent == parent_rl);
        assert(g1->idx == 10);
        assert(parent_rl->parent == nullptr);
        assert(parent_rl->idx == 5);
        assert(gs.size() == 1);
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check returned resolution lineage structure
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Original goal should be removed
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Check new goal lineages structure
        const goal_lineage* child_g0 = lp.goal(rl, 0);
        const goal_lineage* child_g1 = lp.goal(rl, 1);
        
        assert(child_g0->parent == rl);
        assert(child_g0->idx == 0);
        assert(child_g1->parent == rl);
        assert(child_g1->idx == 1);
        
        // Both children should be in goal store
        assert(gs.count(child_g0) == 1);
        assert(gs.count(child_g1) == 1);
        assert(gs.size() == 2);
        
        // Check expressions of children
        const expr* expr0 = gs.at(child_g0);
        const expr* expr1 = gs.at(child_g1);
        assert(std::holds_alternative<expr::atom>(expr0->content));
        assert(std::get<expr::atom>(expr0->content).value == std::string("q"));
        assert(std::holds_alternative<expr::atom>(expr1->content));
        assert(std::get<expr::atom>(expr1->content).value == std::string("r"));
        
        // Both should be in lineage pool
        assert(lp.goal_lineages.count(*child_g0) == 1);
        assert(lp.goal_lineages.count(*child_g1) == 1);
        
        // Both should have candidates
        assert(cs.count(child_g0) == 1);
        assert(cs.count(child_g1) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 7: CRITICAL - Verify copier uses consistent translation_map
    // Same variable X in original rule must map to same renamed variable in all occurrences
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(X) :- q(X), r(X)"
        // The same variable X appears in head and both body literals
        const expr* var_x = ep.var(seq());
        uint32_t original_var_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        const expr* q_x = ep.cons(ep.atom("q"), var_x);
        const expr* r_x = ep.cons(ep.atom("r"), var_x);
        rule r1{p_x, {q_x, r_x}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Record sequencer state
        uint32_t seq_before = seq.index;
        
        // Add goal "p(a)" using goal_adder
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_a = ep.atom("a");
        const expr* goal_p_a = ep.cons(ep.atom("p"), atom_a);
        ga(g1, goal_p_a);
        
        // Pre-resolution
        assert(gs.size() == 1);
        assert(cs.size() == 1);
        size_t bindings_before = bm.bindings.size();
        
        // CRITICAL: Original variable should NOT be in bind_map
        assert(bm.bindings.count(original_var_idx) == 0);
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check sequencer advanced (new variable allocated)
        uint32_t seq_after = seq.index;
        assert(seq_after > seq_before);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Goal removed
        assert(gs.count(g1) == 0);
        
        // Two new goals should be added: q(X') and r(X') where X' is the renamed variable
        assert(gs.size() == 2);
        const goal_lineage* child_g0 = lp.goal(rl, 0);
        const goal_lineage* child_g1 = lp.goal(rl, 1);
        
        // Both should be in goal store
        assert(gs.count(child_g0) == 1);
        assert(gs.count(child_g1) == 1);
        
        // Get the RAW (not normalized) expressions for both goals
        const expr* expr0 = gs.at(child_g0);
        const expr* expr1 = gs.at(child_g1);
        
        // Both should be cons cells
        assert(std::holds_alternative<expr::cons>(expr0->content));
        assert(std::holds_alternative<expr::cons>(expr1->content));
        
        // Extract the argument variables from BOTH expressions (before whnf)
        const expr::cons& cons0_raw = std::get<expr::cons>(expr0->content);
        const expr::cons& cons1_raw = std::get<expr::cons>(expr1->content);
        const expr* arg0_raw = cons0_raw.rhs;
        const expr* arg1_raw = cons1_raw.rhs;
        
        // CRITICAL CHECK: Both arguments should be THE SAME RENAMED VARIABLE
        // This verifies the translation_map was consistent across both body literals
        uint32_t renamed_var_idx_0 = 0;
        uint32_t renamed_var_idx_1 = 0;
        
        if (std::holds_alternative<expr::var>(arg0_raw->content)) {
            renamed_var_idx_0 = std::get<expr::var>(arg0_raw->content).index;
            // Should NOT be the original variable
            assert(renamed_var_idx_0 != original_var_idx);
        }
        
        if (std::holds_alternative<expr::var>(arg1_raw->content)) {
            renamed_var_idx_1 = std::get<expr::var>(arg1_raw->content).index;
            // Should NOT be the original variable
            assert(renamed_var_idx_1 != original_var_idx);
        }
        
        // CRITICAL: Both should be the SAME renamed variable
        // This proves translation_map was consistent!
        assert(renamed_var_idx_0 == renamed_var_idx_1);
        assert(renamed_var_idx_0 != 0); // Should have found a variable
        
        // Both renamed variables should be bound in bind_map to "a"
        assert(bm.bindings.count(renamed_var_idx_0) == 1);
        
        // Normalize both expressions to verify they both become "a"
        const expr* norm0 = bm.whnf(expr0);
        const expr* norm1 = bm.whnf(expr1);
        
        // Both should be cons cells
        assert(std::holds_alternative<expr::cons>(norm0->content));
        assert(std::holds_alternative<expr::cons>(norm1->content));
        
        // Check heads are q and r
        const expr::cons& cons0 = std::get<expr::cons>(norm0->content);
        const expr::cons& cons1 = std::get<expr::cons>(norm1->content);
        assert(std::holds_alternative<expr::atom>(cons0.lhs->content));
        assert(std::get<expr::atom>(cons0.lhs->content).value == std::string("q"));
        assert(std::holds_alternative<expr::atom>(cons1.lhs->content));
        assert(std::get<expr::atom>(cons1.lhs->content).value == std::string("r"));
        
        // Check arguments normalize to "a" - this verifies the binding worked
        const expr* arg0 = bm.whnf(cons0.rhs);
        const expr* arg1 = bm.whnf(cons1.rhs);
        
        assert(std::holds_alternative<expr::atom>(arg0->content));
        assert(std::get<expr::atom>(arg0->content).value == std::string("a"));
        assert(std::holds_alternative<expr::atom>(arg1->content));
        assert(std::get<expr::atom>(arg1->content).value == std::string("a"));
        
        // Unification created bindings
        size_t bindings_after = bm.bindings.size();
        assert(bindings_after > bindings_before);
        
        // CRITICAL: Original variable should STILL NOT be in bind_map
        assert(bm.bindings.count(original_var_idx) == 0);
        
        // Both should have candidates
        assert(cs.count(child_g0) == 1);
        assert(cs.count(child_g1) == 1);
        
        // Both should be in lineage pool
        assert(lp.goal_lineages.count(*child_g0) == 1);
        assert(lp.goal_lineages.count(*child_g1) == 1);
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 8: Database with multiple rules, resolve with non-zero index
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: multiple rules
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        rule r1{p_expr, {}};         // Rule 0: p
        rule r2{q_expr, {r_expr}};   // Rule 1: q :- r
        rule r3{r_expr, {}};         // Rule 2: r
        a01_database db = {r1, r2, r3};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal "q" using goal_adder (adds all rules as candidates)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* goal_q = ep.atom("q");
        ga(g1, goal_q);
        
        // Pre-resolution: goal has all 3 rules as candidates
        assert(gs.size() == 1);
        assert(gs.at(g1) == goal_q);
        assert(cs.size() == 3);
        assert(cs.count(g1) == 3);
        
        // Resolve g1 with rule 1 (index 1) - "q :- r"
        resolver(g1, 1);
        const resolution_lineage* rl = lp.resolution(g1, 1);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 1);  // CRITICAL: idx should match the rule index
        
        // Goal removed from stores
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // One new goal "r" should be added
        assert(gs.size() == 1);
        const goal_lineage* child_g = lp.goal(rl, 0);
        assert(gs.count(child_g) == 1);
        assert(child_g->parent == rl);
        assert(child_g->idx == 0);
        
        // Check the goal expression is "r"
        const expr* child_expr = gs.at(child_g);
        assert(std::holds_alternative<expr::atom>(child_expr->content));
        assert(std::get<expr::atom>(child_expr->content).value == std::string("r"));
        
        // New goal has all database rules as candidates
        assert(cs.size() == 3);
        assert(cs.count(child_g) == 3);
        
        // Verify in lineage pool
        assert(lp.goal_lineages.count(*child_g) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 9: Resolve goal that's part of an existing AND-OR tree (deep goal)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p :- q"
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        rule r1{p_expr, {q_expr}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Create a goal that's deep in the tree (Level 0 -> Level 1 -> Level 2)
        const goal_lineage* root = lp.goal(nullptr, 0);
        const resolution_lineage* r_level1 = lp.resolution(root, 0);
        const goal_lineage* g_level2 = lp.goal(r_level1, 0);
        
        // Verify tree structure before resolution
        assert(root->parent == nullptr);
        assert(root->idx == 0);
        assert(r_level1->parent == root);
        assert(r_level1->idx == 0);
        assert(g_level2->parent == r_level1);
        assert(g_level2->idx == 0);
        
        const expr* goal_p = ep.atom("p");
        ga(g_level2, goal_p);
        
        // Pre-resolution state
        assert(gs.size() == 1);
        assert(gs.at(g_level2) == goal_p);
        assert(cs.size() == 1);
        
        // Resolve the deep goal
        resolver(g_level2, 0);
        const resolution_lineage* rl = lp.resolution(g_level2, 0);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g_level2);
        assert(rl->idx == 0);
        
        // Goal removed
        assert(gs.count(g_level2) == 0);
        assert(cs.count(g_level2) == 0);
        
        // Resolution should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // New goal created as child of new resolution (Level 3)
        const goal_lineage* new_goal = lp.goal(rl, 0);
        assert(gs.count(new_goal) == 1);
        assert(new_goal->parent == rl);
        assert(new_goal->idx == 0);
        
        // Verify tree integrity: new_goal -> rl -> g_level2 -> r_level1 -> root -> nullptr
        assert(new_goal->parent == rl);
        assert(rl->parent == g_level2);
        assert(g_level2->parent == r_level1);
        assert(r_level1->parent == root);
        assert(root->parent == nullptr);
        
        // Check new goal expression
        const expr* new_expr = gs.at(new_goal);
        assert(std::holds_alternative<expr::atom>(new_expr->content));
        assert(std::get<expr::atom>(new_expr->content).value == std::string("q"));
        
        // Verify in pool
        assert(lp.goal_lineages.count(*new_goal) == 1);
        
        // Verify candidates
        assert(cs.count(new_goal) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 10: Resolve with multiple distinct variables - verify independent renaming
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(X, Y) :- q(X), r(Y)"
        // Two distinct variables X and Y
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        uint32_t original_x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t original_y_idx = std::get<expr::var>(var_y->content).index;
        assert(original_x_idx != original_y_idx); // Sanity check
        const expr* p_xy = ep.cons(ep.atom("p"), ep.cons(var_x, var_y));
        const expr* q_x = ep.cons(ep.atom("q"), var_x);
        const expr* r_y = ep.cons(ep.atom("r"), var_y);
        rule r1{p_xy, {q_x, r_y}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Record sequencer state
        uint32_t seq_before = seq.index;
        
        // Add goal "p(a, b)" using goal_adder
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_a = ep.atom("a");
        const expr* atom_b = ep.atom("b");
        const expr* goal_p_ab = ep.cons(ep.atom("p"), ep.cons(atom_a, atom_b));
        ga(g1, goal_p_ab);
        
        // Pre-resolution
        assert(gs.size() == 1);
        assert(gs.at(g1) == goal_p_ab);
        assert(cs.size() == 1);
        size_t bindings_before = bm.bindings.size();
        
        // CRITICAL: Original variables should NOT be in bind_map
        assert(bm.bindings.count(original_x_idx) == 0);
        assert(bm.bindings.count(original_y_idx) == 0);
        
        // Resolve g1 with rule 0
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check sequencer advanced (two new variables allocated: X' and Y')
        uint32_t seq_after = seq.index;
        assert(seq_after > seq_before);
        // Should have allocated at least 2 new variables
        assert(seq_after >= seq_before + 2);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Goal removed
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Two new goals should be added: q(X') and r(Y') where X' and Y' are renamed
        assert(gs.size() == 2);
        const goal_lineage* child_g0 = lp.goal(rl, 0);
        const goal_lineage* child_g1 = lp.goal(rl, 1);
        assert(gs.count(child_g0) == 1);
        assert(gs.count(child_g1) == 1);
        
        // Check lineage structure
        assert(child_g0->parent == rl);
        assert(child_g0->idx == 0);
        assert(child_g1->parent == rl);
        assert(child_g1->idx == 1);
        
        // Get RAW goal expressions (before normalization)
        const expr* expr0 = gs.at(child_g0);
        const expr* expr1 = gs.at(child_g1);
        
        // Extract variables from raw expressions
        assert(std::holds_alternative<expr::cons>(expr0->content));
        assert(std::holds_alternative<expr::cons>(expr1->content));
        const expr::cons& cons0_raw = std::get<expr::cons>(expr0->content);
        const expr::cons& cons1_raw = std::get<expr::cons>(expr1->content);
        const expr* arg0_raw = cons0_raw.rhs;
        const expr* arg1_raw = cons1_raw.rhs;
        
        // Extract renamed variable indices
        uint32_t renamed_x_idx = 0;
        uint32_t renamed_y_idx = 0;
        
        if (std::holds_alternative<expr::var>(arg0_raw->content)) {
            renamed_x_idx = std::get<expr::var>(arg0_raw->content).index;
            // Should NOT be the original X
            assert(renamed_x_idx != original_x_idx);
            assert(renamed_x_idx != original_y_idx);
        }
        
        if (std::holds_alternative<expr::var>(arg1_raw->content)) {
            renamed_y_idx = std::get<expr::var>(arg1_raw->content).index;
            // Should NOT be the original Y
            assert(renamed_y_idx != original_y_idx);
            assert(renamed_y_idx != original_x_idx);
        }
        
        // CRITICAL: The two renamed variables should be DIFFERENT
        // (X and Y were distinct, so X' and Y' should also be distinct)
        assert(renamed_x_idx != renamed_y_idx);
        assert(renamed_x_idx != 0);
        assert(renamed_y_idx != 0);
        
        // Both renamed variables should be in bind_map
        assert(bm.bindings.count(renamed_x_idx) == 1);
        assert(bm.bindings.count(renamed_y_idx) == 1);
        
        // Normalize both
        const expr* norm0 = bm.whnf(expr0);
        const expr* norm1 = bm.whnf(expr1);
        
        // Both should be cons cells
        assert(std::holds_alternative<expr::cons>(norm0->content));
        assert(std::holds_alternative<expr::cons>(norm1->content));
        
        // Check heads: q and r
        const expr::cons& cons0 = std::get<expr::cons>(norm0->content);
        const expr::cons& cons1 = std::get<expr::cons>(norm1->content);
        const expr* head0 = cons0.lhs;
        const expr* head1 = cons1.lhs;
        assert(std::holds_alternative<expr::atom>(head0->content));
        assert(std::get<expr::atom>(head0->content).value == std::string("q"));
        assert(std::holds_alternative<expr::atom>(head1->content));
        assert(std::get<expr::atom>(head1->content).value == std::string("r"));
        
        // Check arguments: q(a) and r(b)
        const expr* arg0 = bm.whnf(cons0.rhs);
        const expr* arg1 = bm.whnf(cons1.rhs);
        
        assert(std::holds_alternative<expr::atom>(arg0->content));
        assert(std::get<expr::atom>(arg0->content).value == std::string("a"));
        assert(std::holds_alternative<expr::atom>(arg1->content));
        assert(std::get<expr::atom>(arg1->content).value == std::string("b"));
        
        // Unification created bindings (X'=a and Y'=b)
        size_t bindings_after = bm.bindings.size();
        assert(bindings_after > bindings_before);
        assert(bindings_after >= bindings_before + 2); // At least 2 new bindings
        
        // CRITICAL: Original variables should STILL NOT be in bind_map
        assert(bm.bindings.count(original_x_idx) == 0);
        assert(bm.bindings.count(original_y_idx) == 0);
        
        // Both should have candidates
        assert(cs.count(child_g0) == 1);
        assert(cs.count(child_g1) == 1);
        assert(cs.size() == 2);
        
        // Both should be in lineage pool
        assert(lp.goal_lineages.count(*child_g0) == 1);
        assert(lp.goal_lineages.count(*child_g1) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 11: Variable-on-variable unification during resolution
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(Y) :- q(Y)" where Y is a variable in the rule
        const expr* var_y = ep.var(seq());
        uint32_t original_y_idx = std::get<expr::var>(var_y->content).index;
        const expr* p_y = ep.cons(ep.atom("p"), var_y);
        const expr* q_y = ep.cons(ep.atom("q"), var_y);
        rule r1{p_y, {q_y}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Record sequencer state
        uint32_t seq_before = seq.index;
        
        // Create goal "p(X)" where X is ALSO a variable (not a constant!)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* var_x = ep.var(seq());
        uint32_t goal_var_x_idx = std::get<expr::var>(var_x->content).index;
        const expr* goal_p_x = ep.cons(ep.atom("p"), var_x);
        ga(g1, goal_p_x);
        
        // Pre-resolution
        assert(gs.size() == 1);
        assert(gs.at(g1) == goal_p_x);
        assert(cs.size() == 1);
        size_t bindings_before = bm.bindings.size();
        
        // CRITICAL: Neither the original rule variable nor the goal variable
        // should be in bind_map yet
        assert(bm.bindings.count(original_y_idx) == 0);
        assert(bm.bindings.count(goal_var_x_idx) == 0);
        
        // Resolve g1 with rule 0
        // This will unify p(X) with p(Y') where Y' is the renamed version of Y
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Check sequencer advanced (Y was renamed to Y')
        uint32_t seq_after = seq.index;
        assert(seq_after > seq_before);
        
        // Check return value
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 0);
        
        // Goal removed
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution should be in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // One new goal should be added: q(Y') or q(X) depending on binding direction
        assert(gs.size() == 1);
        const goal_lineage* child_g = lp.goal(rl, 0);
        assert(gs.count(child_g) == 1);
        
        // Get the RAW child goal expression
        const expr* child_expr = gs.at(child_g);
        assert(std::holds_alternative<expr::cons>(child_expr->content));
        const expr::cons& child_cons = std::get<expr::cons>(child_expr->content);
        
        // Head should be "q"
        assert(std::holds_alternative<expr::atom>(child_cons.lhs->content));
        assert(std::get<expr::atom>(child_cons.lhs->content).value == std::string("q"));
        
        // Tail should be a variable
        const expr* child_arg = child_cons.rhs;
        assert(std::holds_alternative<expr::var>(child_arg->content));
        uint32_t child_var_idx = std::get<expr::var>(child_arg->content).index;
        
        // The child variable should be the renamed Y' (not the original Y)
        assert(child_var_idx != original_y_idx);
        
        // CRITICAL: Variable-on-variable unification occurred
        // Either X=Y' or Y'=X, but at least one should be in bind_map
        // The unification should have created a binding between X and Y'
        size_t bindings_after = bm.bindings.size();
        assert(bindings_after > bindings_before);
        
        // At least one of the variables should be bound
        bool x_bound = bm.bindings.count(goal_var_x_idx) > 0;
        bool y_prime_bound = bm.bindings.count(child_var_idx) > 0;
        assert(x_bound || y_prime_bound);
        
        // If we normalize the child argument, it should give us a variable
        // (either X or Y' depending on binding direction)
        const expr* normalized_arg = bm.whnf(child_arg);
        assert(std::holds_alternative<expr::var>(normalized_arg->content));
        
        // CRITICAL: Original rule variable Y should STILL NOT be in bind_map
        assert(bm.bindings.count(original_y_idx) == 0);
        
        // Verify in pools
        assert(lp.goal_lineages.count(*child_g) == 1);
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Should have candidates
        assert(cs.count(child_g) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 12: Multiple database rules - resolve with middle rule to verify indexing
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: 5 different rules with different heads and bodies
        // We'll resolve using rule at index 2 (middle)
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        const expr* s_expr = ep.atom("s");
        const expr* t_expr = ep.atom("t");
        const expr* u_expr = ep.atom("u");
        
        rule r0{p_expr, {}};                    // Rule 0: p
        rule r1{q_expr, {r_expr}};              // Rule 1: q :- r
        rule r2{r_expr, {s_expr, t_expr}};      // Rule 2: r :- s, t  <-- We'll use this one
        rule r3{s_expr, {u_expr}};              // Rule 3: s :- u
        rule r4{t_expr, {}};                    // Rule 4: t
        a01_database db = {r0, r1, r2, r3, r4};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal "r" which matches rule 2
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, r_expr);
        
        // Pre-resolution: goal has all 5 rules as candidates
        assert(gs.size() == 1);
        assert(gs.at(g1) == r_expr);
        assert(cs.size() == 5);
        assert(cs.count(g1) == 5);
        
        // Resolve with rule 2 (middle of database)
        resolver(g1, 2);
        const resolution_lineage* rl = lp.resolution(g1, 2);
        
        // Verify return value has correct index
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 2);  // CRITICAL: Must be index 2, not 0 or 1
        
        // Goal removed
        assert(gs.count(g1) == 0);
        assert(cs.count(g1) == 0);
        
        // Resolution in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Two new goals should be added from rule 2's body: s and t
        assert(gs.size() == 2);
        const goal_lineage* child_g0 = lp.goal(rl, 0);
        const goal_lineage* child_g1 = lp.goal(rl, 1);
        
        // Verify lineage structure
        assert(child_g0->parent == rl);
        assert(child_g0->idx == 0);
        assert(child_g1->parent == rl);
        assert(child_g1->idx == 1);
        
        // Verify expressions are from rule 2's body (s and t, NOT r or u)
        const expr* expr0 = gs.at(child_g0);
        const expr* expr1 = gs.at(child_g1);
        
        assert(std::holds_alternative<expr::atom>(expr0->content));
        assert(std::get<expr::atom>(expr0->content).value == std::string("s"));
        assert(std::holds_alternative<expr::atom>(expr1->content));
        assert(std::get<expr::atom>(expr1->content).value == std::string("t"));
        
        // Both children should have all 5 database rules as candidates
        assert(cs.size() == 10); // 2 goals * 5 rules each
        assert(cs.count(child_g0) == 5);
        assert(cs.count(child_g1) == 5);
        
        // Verify in pools
        assert(lp.goal_lineages.count(*child_g0) == 1);
        assert(lp.goal_lineages.count(*child_g1) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 13: Multiple goals in store - resolve specific one, others untouched
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: simple rules for various predicates
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        const expr* s_expr = ep.atom("s");
        const expr* t_expr = ep.atom("t");
        
        rule r0{p_expr, {q_expr}};              // p :- q
        rule r1{q_expr, {r_expr, s_expr}};      // q :- r, s  <-- We'll resolve with this
        rule r2{r_expr, {}};                    // r
        rule r3{s_expr, {}};                    // s
        rule r4{t_expr, {}};                    // t
        a01_database db = {r0, r1, r2, r3, r4};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add 5 different goals with different lineage structures
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        ga(g0, p_expr);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, q_expr);  // <-- We'll resolve THIS one
        
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        ga(g2, r_expr);
        
        const resolution_lineage* rl_other = lp.resolution(nullptr, 99);
        const goal_lineage* g3 = lp.goal(rl_other, 3);
        ga(g3, s_expr);
        
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        ga(g4, t_expr);
        
        // Pre-resolution: 5 goals, each with 5 candidates
        assert(gs.size() == 5);
        assert(gs.at(g0) == p_expr);
        assert(gs.at(g1) == q_expr);
        assert(gs.at(g2) == r_expr);
        assert(gs.at(g3) == s_expr);
        assert(gs.at(g4) == t_expr);
        assert(cs.size() == 25); // 5 goals * 5 rules each
        
        // Record the exact expressions for later comparison
        const expr* g0_expr = gs.at(g0);
        const expr* g2_expr = gs.at(g2);
        const expr* g3_expr = gs.at(g3);
        const expr* g4_expr = gs.at(g4);
        
        // Resolve ONLY g1 (the middle one) with rule 1
        resolver(g1, 1);
        const resolution_lineage* rl = lp.resolution(g1, 1);
        
        // Verify resolution lineage
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 1);
        
        // CRITICAL: g1 should be removed, ALL OTHERS should remain
        assert(gs.count(g1) == 0);
        assert(gs.count(g0) == 1);  // Still there
        assert(gs.count(g2) == 1);  // Still there
        assert(gs.count(g3) == 1);  // Still there
        assert(gs.count(g4) == 1);  // Still there
        
        // CRITICAL: Expressions of remaining goals should be UNCHANGED
        assert(gs.at(g0) == g0_expr);  // Same pointer
        assert(gs.at(g2) == g2_expr);  // Same pointer
        assert(gs.at(g3) == g3_expr);  // Same pointer
        assert(gs.at(g4) == g4_expr);  // Same pointer
        
        // g1's candidates removed, others remain
        assert(cs.count(g1) == 0);
        assert(cs.count(g0) == 5);  // Still has all candidates
        assert(cs.count(g2) == 5);  // Still has all candidates
        assert(cs.count(g3) == 5);  // Still has all candidates
        assert(cs.count(g4) == 5);  // Still has all candidates
        
        // Two new goals added as children of g1's resolution (rule 1: q :- r, s)
        const goal_lineage* child0 = lp.goal(rl, 0);
        const goal_lineage* child1 = lp.goal(rl, 1);
        
        assert(gs.count(child0) == 1);
        assert(gs.count(child1) == 1);
        
        // Total goals: 4 original (g0, g2, g3, g4) + 2 new (child0, child1) = 6
        assert(gs.size() == 6);
        
        // Verify new children have correct expressions (from rule 1's body: r, s)
        const expr* child0_expr = gs.at(child0);
        const expr* child1_expr = gs.at(child1);
        
        assert(std::holds_alternative<expr::atom>(child0_expr->content));
        assert(std::get<expr::atom>(child0_expr->content).value == std::string("r"));
        assert(std::holds_alternative<expr::atom>(child1_expr->content));
        assert(std::get<expr::atom>(child1_expr->content).value == std::string("s"));
        
        // Verify lineage structure: new children are under rl, which is under g1
        assert(child0->parent == rl);
        assert(child0->idx == 0);
        assert(child1->parent == rl);
        assert(child1->idx == 1);
        assert(rl->parent == g1);
        
        // New children should have candidates
        assert(cs.count(child0) == 5);
        assert(cs.count(child1) == 5);
        
        // Total candidates: 4 old goals * 5 + 2 new goals * 5 = 30
        assert(cs.size() == 30);
        
        // Verify all goals are in lineage pool
        assert(lp.goal_lineages.count(*g0) == 1);
        assert(lp.goal_lineages.count(*g2) == 1);
        assert(lp.goal_lineages.count(*g3) == 1);
        assert(lp.goal_lineages.count(*g4) == 1);
        assert(lp.goal_lineages.count(*child0) == 1);
        assert(lp.goal_lineages.count(*child1) == 1);
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 14: Database indexing with variables - resolve with last rule
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rules with variables at different indices
        const expr* var0 = ep.var(seq());
        const expr* var1 = ep.var(seq());
        const expr* var2 = ep.var(seq());
        
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        
        const expr* p_v0 = ep.cons(p_expr, var0);
        const expr* q_v0 = ep.cons(q_expr, var0);
        const expr* q_v1 = ep.cons(q_expr, var1);
        const expr* r_v1 = ep.cons(r_expr, var1);
        const expr* r_v2 = ep.cons(r_expr, var2);
        
        rule r0{p_v0, {q_v0}};                  // Rule 0: p(X) :- q(X)
        rule r1{p_expr, {}};                    // Rule 1: p
        rule r2{q_v1, {r_v1}};                  // Rule 2: q(Y) :- r(Y)
        rule r3{r_v2, {}};                      // Rule 3: r(Z)  <-- We'll use this (last)
        a01_database db = {r0, r1, r2, r3};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal "r(a)"
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_a = ep.atom("a");
        const expr* goal_r_a = ep.cons(r_expr, atom_a);
        ga(g1, goal_r_a);
        
        // Pre-resolution
        assert(gs.size() == 1);
        assert(gs.at(g1) == goal_r_a);
        assert(cs.size() == 4);
        
        // Record sequencer and bindings before
        uint32_t seq_before = seq.index;
        size_t bindings_before = bm.bindings.size();
        
        // Resolve with rule 3 (LAST rule, index 3)
        resolver(g1, 3);
        const resolution_lineage* rl = lp.resolution(g1, 3);
        
        // Verify correct rule was used
        assert(rl != nullptr);
        assert(rl->parent == g1);
        assert(rl->idx == 3);  // CRITICAL: Must be 3 (last rule)
        
        // Sequencer should have advanced (var2 was copied)
        assert(seq.index > seq_before);
        
        // Goal removed
        assert(gs.count(g1) == 0);
        
        // Rule 3 has empty body, so no new goals
        assert(gs.size() == 0);
        
        // Unification should have occurred (r(Z') with r(a))
        assert(bm.bindings.size() > bindings_before);
        
        // Resolution in pool
        assert(lp.resolution_lineages.count(*rl) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 15: Multiple goals, resolve multiple times in sequence
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        const expr* r_expr = ep.atom("r");
        
        rule r0{p_expr, {q_expr}};      // p :- q
        rule r1{q_expr, {r_expr}};      // q :- r
        rule r2{r_expr, {}};            // r
        a01_database db = {r0, r1, r2};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add three goals
        const goal_lineage* g_p = lp.goal(nullptr, 10);
        ga(g_p, p_expr);
        
        const goal_lineage* g_q = lp.goal(nullptr, 20);
        ga(g_q, q_expr);
        
        const goal_lineage* g_r = lp.goal(nullptr, 30);
        ga(g_r, r_expr);
        
        // Pre-resolution: 3 goals
        assert(gs.size() == 3);
        assert(cs.size() == 9); // 3 goals * 3 rules
        
        // First resolution: resolve g_q with rule 1 (q :- r)
        resolver(g_q, 1);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        
        assert(rl_q->parent == g_q);
        assert(rl_q->idx == 1);
        
        // g_q removed, g_p and g_r remain
        assert(gs.count(g_q) == 0);
        assert(gs.count(g_p) == 1);
        assert(gs.count(g_r) == 1);
        
        // One new goal from q's resolution
        const goal_lineage* child_of_q = lp.goal(rl_q, 0);
        assert(gs.count(child_of_q) == 1);
        assert(gs.size() == 3); // g_p, g_r, child_of_q
        
        // Verify child_of_q has correct expression (r)
        const expr* child_of_q_expr = gs.at(child_of_q);
        assert(std::holds_alternative<expr::atom>(child_of_q_expr->content));
        assert(std::get<expr::atom>(child_of_q_expr->content).value == std::string("r"));
        
        // Second resolution: resolve g_p with rule 0 (p :- q)
        resolver(g_p, 0);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        
        assert(rl_p->parent == g_p);
        assert(rl_p->idx == 0);
        
        // g_p removed, g_r and child_of_q remain
        assert(gs.count(g_p) == 0);
        assert(gs.count(g_q) == 0);  // Still gone from first resolution
        assert(gs.count(g_r) == 1);
        assert(gs.count(child_of_q) == 1);
        
        // One new goal from p's resolution
        const goal_lineage* child_of_p = lp.goal(rl_p, 0);
        assert(gs.count(child_of_p) == 1);
        assert(gs.size() == 3); // g_r, child_of_q, child_of_p
        
        // Verify child_of_p has correct expression (q)
        const expr* child_of_p_expr = gs.at(child_of_p);
        assert(std::holds_alternative<expr::atom>(child_of_p_expr->content));
        assert(std::get<expr::atom>(child_of_p_expr->content).value == std::string("q"));
        
        // Third resolution: resolve g_r with rule 2 (r - fact)
        resolver(g_r, 2);
        const resolution_lineage* rl_r = lp.resolution(g_r, 2);
        
        assert(rl_r->parent == g_r);
        assert(rl_r->idx == 2);
        
        // g_r removed, only children remain
        assert(gs.count(g_r) == 0);
        assert(gs.count(child_of_q) == 1);
        assert(gs.count(child_of_p) == 1);
        assert(gs.size() == 2); // child_of_q, child_of_p
        
        // No new goals from g_r (empty body)
        
        // Verify all resolutions in pool
        assert(lp.resolution_lineages.count(*rl_p) == 1);
        assert(lp.resolution_lineages.count(*rl_q) == 1);
        assert(lp.resolution_lineages.count(*rl_r) == 1);
        
        // Verify all resolutions in resolution store
        assert(rs.size() == 3);
        assert(rs.count(rl_p) == 1);
        assert(rs.count(rl_q) == 1);
        assert(rs.count(rl_r) == 1);
        
        // Verify lineage structures remain correct
        assert(child_of_p->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(child_of_q->parent == rl_q);
        assert(rl_q->parent == g_q);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 16: Resolution store with pre-existing resolutions
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database
        const expr* p_expr = ep.atom("p");
        const expr* q_expr = ep.atom("q");
        rule r0{p_expr, {}};
        rule r1{q_expr, {}};
        a01_database db = {r0, r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Pre-populate resolution store with some existing resolutions
        const goal_lineage* g_old1 = lp.goal(nullptr, 100);
        const goal_lineage* g_old2 = lp.goal(nullptr, 200);
        const resolution_lineage* rl_old1 = lp.resolution(g_old1, 5);
        const resolution_lineage* rl_old2 = lp.resolution(g_old2, 7);
        rs.insert(rl_old1);
        rs.insert(rl_old2);
        
        // Pre-resolution: 2 existing resolutions
        assert(rs.size() == 2);
        assert(rs.count(rl_old1) == 1);
        assert(rs.count(rl_old2) == 1);
        
        // Add a new goal and resolve it
        const goal_lineage* g_new = lp.goal(nullptr, 1);
        ga(g_new, p_expr);
        
        assert(gs.size() == 1);
        
        // Resolve the new goal
        resolver(g_new, 0);
        const resolution_lineage* rl_new = lp.resolution(g_new, 0);
        
        // Verify resolution store now has 3 resolutions (2 old + 1 new)
        assert(rs.size() == 3);
        assert(rs.count(rl_old1) == 1);  // Old ones still there
        assert(rs.count(rl_old2) == 1);  // Old ones still there
        assert(rs.count(rl_new) == 1);   // New one added
        
        // Verify new resolution has correct structure
        assert(rl_new->parent == g_new);
        assert(rl_new->idx == 0);
        
        // Goal was removed
        assert(gs.count(g_new) == 0);
        
        // All resolutions in pool
        assert(lp.resolution_lineages.count(*rl_old1) == 1);
        assert(lp.resolution_lineages.count(*rl_old2) == 1);
        assert(lp.resolution_lineages.count(*rl_new) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 17: Complex nested expressions with deep structures AND variables
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Build nested expression with VARIABLE: p(f(g(h(X))))
        const expr* var_x = ep.var(seq());
        uint32_t original_x_idx = std::get<expr::var>(var_x->content).index;
        const expr* h_x = ep.cons(ep.atom("h"), var_x);
        const expr* g_h_x = ep.cons(ep.atom("g"), h_x);
        const expr* f_g_h_x = ep.cons(ep.atom("f"), g_h_x);
        const expr* p_nested = ep.cons(ep.atom("p"), f_g_h_x);
        
        // Build rule with nested expression in body: p(f(g(h(X)))) :- q(f(g(h(X))))
        const expr* q_nested = ep.cons(ep.atom("q"), f_g_h_x);
        rule r1{p_nested, {q_nested}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal with concrete value: p(f(g(h(a))))
        const expr* atom_a = ep.atom("a");
        const expr* h_a = ep.cons(ep.atom("h"), atom_a);
        const expr* g_h_a = ep.cons(ep.atom("g"), h_a);
        const expr* f_g_h_a = ep.cons(ep.atom("f"), g_h_a);
        const expr* goal_p_nested = ep.cons(ep.atom("p"), f_g_h_a);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, goal_p_nested);
        
        // Store expression pool size and original pointers
        size_t ep_size_before = ep.size();
        const expr* original_body_expr = q_nested;
        
        // Pre-resolution: original variable not bound
        assert(bm.bindings.count(original_x_idx) == 0);
        
        uint32_t seq_before = seq.index;
        
        // Resolve
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Sequencer advanced (variable copied)
        assert(seq.index > seq_before);
        
        // Verify resolution created
        assert(rs.size() == 1);
        assert(rs.count(rl) == 1);
        
        // One child goal added
        assert(gs.size() == 1);
        const goal_lineage* child = lp.goal(rl, 0);
        assert(gs.count(child) == 1);
        
        // Get the child expression
        const expr* child_expr = gs.at(child);
        
        // CRITICAL: Copied expression must be DIFFERENT pointer from original
        assert(child_expr != original_body_expr);
        
        // CRITICAL: Expression pool grew (new expressions created during copy)
        assert(ep.size() > ep_size_before);
        
        // Navigate through the nested structure: q(f(g(h(a))))
        assert(std::holds_alternative<expr::cons>(child_expr->content));
        const expr::cons& cons1 = std::get<expr::cons>(child_expr->content);
        assert(std::holds_alternative<expr::atom>(cons1.lhs->content));
        assert(std::get<expr::atom>(cons1.lhs->content).value == "q");
        
        // Get f(g(h(a))) after unification
        const expr* f_part = bm.whnf(cons1.rhs);
        assert(std::holds_alternative<expr::cons>(f_part->content));
        const expr::cons& cons2 = std::get<expr::cons>(f_part->content);
        assert(std::get<expr::atom>(cons2.lhs->content).value == "f");
        
        // Get g(h(a))
        const expr* g_part = bm.whnf(cons2.rhs);
        assert(std::holds_alternative<expr::cons>(g_part->content));
        const expr::cons& cons3 = std::get<expr::cons>(g_part->content);
        assert(std::get<expr::atom>(cons3.lhs->content).value == "g");
        
        // Get h(a)
        const expr* h_part = bm.whnf(cons3.rhs);
        assert(std::holds_alternative<expr::cons>(h_part->content));
        const expr::cons& cons4 = std::get<expr::cons>(h_part->content);
        assert(std::get<expr::atom>(cons4.lhs->content).value == "h");
        
        // Get a (after following all bindings)
        const expr* a_part = bm.whnf(cons4.rhs);
        assert(std::holds_alternative<expr::atom>(a_part->content));
        assert(std::get<expr::atom>(a_part->content).value == "a");
        
        // CRITICAL: Original variable still not in bind_map
        assert(bm.bindings.count(original_x_idx) == 0);
        
        // CRITICAL: Renamed variable IS in bind_map
        assert(bm.bindings.size() == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 18: Large body clauses (10+ literals)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Create rule with 15 body literals: p :- q1, q2, ..., q15
        const expr* p_expr = ep.atom("p");
        std::vector<const expr*> body_literals;
        for (int i = 1; i <= 15; i++) {
            body_literals.push_back(ep.atom("q" + std::to_string(i)));
        }
        
        rule r1{p_expr, {body_literals.begin(), body_literals.end()}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, p_expr);
        
        assert(gs.size() == 1);
        assert(cs.size() == 1);
        
        // Resolve
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Verify resolution
        assert(rs.size() == 1);
        assert(rs.count(rl) == 1);
        
        // Goal removed
        assert(gs.count(g1) == 0);
        
        // 15 child goals should be added
        assert(gs.size() == 15);
        
        // Verify each child
        for (size_t i = 0; i < 15; i++) {
            const goal_lineage* child = lp.goal(rl, i);
            assert(gs.count(child) == 1);
            assert(child->parent == rl);
            assert(child->idx == i);
            
            // Verify expression
            const expr* child_expr = gs.at(child);
            assert(std::holds_alternative<expr::atom>(child_expr->content));
            assert(std::get<expr::atom>(child_expr->content).value == "q" + std::to_string(i + 1));
            
            // Each child should have candidates
            assert(cs.count(child) == 1);
        }
        
        // Total: 15 children * 1 candidate each
        assert(cs.size() == 15);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 19: CRITICAL - Goal with multiple variables
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(A, B) :- q(A), r(B)"
        const expr* var_a = ep.var(seq());
        const expr* var_b = ep.var(seq());
        uint32_t original_a_idx = std::get<expr::var>(var_a->content).index;
        uint32_t original_b_idx = std::get<expr::var>(var_b->content).index;
        
        const expr* p_ab = ep.cons(ep.atom("p"), ep.cons(var_a, var_b));
        const expr* q_a = ep.cons(ep.atom("q"), var_a);
        const expr* r_b = ep.cons(ep.atom("r"), var_b);
        rule r1{p_ab, {q_a, r_b}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // CRITICAL: Create goal with VARIABLES: p(X, Y) where X and Y are goal variables
        const expr* goal_var_x = ep.var(seq());
        const expr* goal_var_y = ep.var(seq());
        uint32_t goal_x_idx = std::get<expr::var>(goal_var_x->content).index;
        uint32_t goal_y_idx = std::get<expr::var>(goal_var_y->content).index;
        
        const expr* goal_p_xy = ep.cons(ep.atom("p"), ep.cons(goal_var_x, goal_var_y));
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, goal_p_xy);
        
        // Pre-resolution: NO variables should be in bind_map
        assert(bm.bindings.count(original_a_idx) == 0);
        assert(bm.bindings.count(original_b_idx) == 0);
        assert(bm.bindings.count(goal_x_idx) == 0);
        assert(bm.bindings.count(goal_y_idx) == 0);
        
        uint32_t seq_before = seq.index;
        
        // Resolve - this will do variable-on-variable unification
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Sequencer advanced (rule variables renamed)
        assert(seq.index > seq_before);
        
        // CRITICAL: Original rule variables should STILL NOT be in bind_map
        assert(bm.bindings.count(original_a_idx) == 0);
        assert(bm.bindings.count(original_b_idx) == 0);
        
        // Two children added
        assert(gs.size() == 2);
        const goal_lineage* child0 = lp.goal(rl, 0);
        const goal_lineage* child1 = lp.goal(rl, 1);
        
        // Get child expressions
        const expr* expr0 = gs.at(child0);
        const expr* expr1 = gs.at(child1);
        
        // Both should be cons cells with variables
        assert(std::holds_alternative<expr::cons>(expr0->content));
        assert(std::holds_alternative<expr::cons>(expr1->content));
        
        const expr::cons& cons0 = std::get<expr::cons>(expr0->content);
        const expr::cons& cons1 = std::get<expr::cons>(expr1->content);
        
        // Heads should be q and r
        assert(std::get<expr::atom>(cons0.lhs->content).value == "q");
        assert(std::get<expr::atom>(cons1.lhs->content).value == "r");
        
        // Arguments should be variables (unified with goal variables)
        const expr* arg0 = cons0.rhs;
        const expr* arg1 = cons1.rhs;
        
        assert(std::holds_alternative<expr::var>(arg0->content));
        assert(std::holds_alternative<expr::var>(arg1->content));
        
        // These are the renamed rule variables
        uint32_t renamed_a_idx = std::get<expr::var>(arg0->content).index;
        uint32_t renamed_b_idx = std::get<expr::var>(arg1->content).index;
        
        // They should be different from originals
        assert(renamed_a_idx != original_a_idx);
        assert(renamed_b_idx != original_b_idx);
        
        // CRITICAL: Check bind_map bindings - variable-on-variable unification occurred
        // Either goal vars are bound to renamed vars, or renamed vars are bound to goal vars
        bool a_binding_exists = (bm.bindings.count(goal_x_idx) > 0) || (bm.bindings.count(renamed_a_idx) > 0);
        bool b_binding_exists = (bm.bindings.count(goal_y_idx) > 0) || (bm.bindings.count(renamed_b_idx) > 0);
        assert(a_binding_exists);
        assert(b_binding_exists);
        
        // Normalize both to verify they point to same ultimate variable
        const expr* norm0 = bm.whnf(arg0);
        const expr* norm1 = bm.whnf(arg1);
        
        // Both should still be variables (not atoms)
        assert(std::holds_alternative<expr::var>(norm0->content));
        assert(std::holds_alternative<expr::var>(norm1->content));
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 20: EXTREMELY IMPORTANT - Check exact binding contents
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(X, Y) :- q(X), r(Y)"
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        uint32_t original_x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t original_y_idx = std::get<expr::var>(var_y->content).index;
        
        const expr* p_xy = ep.cons(ep.atom("p"), ep.cons(var_x, var_y));
        const expr* q_x = ep.cons(ep.atom("q"), var_x);
        const expr* r_y = ep.cons(ep.atom("r"), var_y);
        rule r1{p_xy, {q_x, r_y}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Goal: p(a, b) - instantiated
        const expr* atom_a = ep.atom("a");
        const expr* atom_b = ep.atom("b");
        const expr* goal_p_ab = ep.cons(ep.atom("p"), ep.cons(atom_a, atom_b));
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, goal_p_ab);
        
        // Pre-resolution: bind_map should be empty
        assert(bm.bindings.size() == 0);
        assert(bm.bindings.count(original_x_idx) == 0);
        assert(bm.bindings.count(original_y_idx) == 0);
        
        uint32_t seq_before = seq.index;
        
        // Resolve
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Two new variables were created (renamed X and Y)
        uint32_t seq_after = seq.index;
        assert(seq_after >= seq_before + 2);
        
        // CRITICAL: Original variables still not in bind_map
        assert(bm.bindings.count(original_x_idx) == 0);
        assert(bm.bindings.count(original_y_idx) == 0);
        
        // Get the child goals to extract renamed variables
        const goal_lineage* child0 = lp.goal(rl, 0);
        const goal_lineage* child1 = lp.goal(rl, 1);
        
        const expr* expr0 = gs.at(child0);
        const expr* expr1 = gs.at(child1);
        
        // Extract renamed variables from children
        const expr::cons& cons0 = std::get<expr::cons>(expr0->content);
        const expr::cons& cons1 = std::get<expr::cons>(expr1->content);
        const expr* arg0 = cons0.rhs;
        const expr* arg1 = cons1.rhs;
        
        uint32_t renamed_x_idx = 0;
        uint32_t renamed_y_idx = 0;
        
        if (std::holds_alternative<expr::var>(arg0->content)) {
            renamed_x_idx = std::get<expr::var>(arg0->content).index;
        }
        if (std::holds_alternative<expr::var>(arg1->content)) {
            renamed_y_idx = std::get<expr::var>(arg1->content).index;
        }
        
        // CRITICAL: Verify exact bind_map contents
        // Bind_map should have exactly 2 entries (one for each renamed variable)
        assert(bm.bindings.size() == 2);
        
        // CRITICAL: Renamed X should be bound to "a"
        assert(bm.bindings.count(renamed_x_idx) == 1);
        const expr* x_binding = bm.bindings.at(renamed_x_idx);
        assert(x_binding != nullptr);
        assert(std::holds_alternative<expr::atom>(x_binding->content));
        assert(std::get<expr::atom>(x_binding->content).value == "a");
        
        // CRITICAL: Renamed Y should be bound to "b"
        assert(bm.bindings.count(renamed_y_idx) == 1);
        const expr* y_binding = bm.bindings.at(renamed_y_idx);
        assert(y_binding != nullptr);
        assert(std::holds_alternative<expr::atom>(y_binding->content));
        assert(std::get<expr::atom>(y_binding->content).value == "b");
        
        // Verify whnf returns the atoms
        const expr* x_normalized = bm.whnf(arg0);
        const expr* y_normalized = bm.whnf(arg1);
        assert(x_normalized == atom_a);
        assert(y_normalized == atom_b);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 21: EXTREMELY IMPORTANT - Verify copied expressions are new instances
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: rule "p(X) :- q(X), r(X)"
        const expr* var_x = ep.var(seq());
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        const expr* q_x = ep.cons(ep.atom("q"), var_x);
        const expr* r_x = ep.cons(ep.atom("r"), var_x);
        rule r1{p_x, {q_x, r_x}};
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Store original expression pointers
        const expr* original_head = r1.head;
        const expr* original_body0 = *r1.body.begin();
        const expr* original_body1 = *(++r1.body.begin());
        
        // Goal: p(a)
        const expr* goal_p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, goal_p_a);
        
        size_t ep_size_before = ep.size();
        
        // Resolve
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // CRITICAL: Expression pool should have grown
        size_t ep_size_after = ep.size();
        assert(ep_size_after > ep_size_before);
        
        // Get child expressions
        const goal_lineage* child0 = lp.goal(rl, 0);
        const goal_lineage* child1 = lp.goal(rl, 1);
        const expr* copied_body0 = gs.at(child0);
        const expr* copied_body1 = gs.at(child1);
        
        // CRITICAL: Copied expressions should be DIFFERENT pointers from originals
        assert(copied_body0 != original_body0);
        assert(copied_body1 != original_body1);
        
        // CRITICAL: But they should be EQUAL in structure (before substitution)
        // Since variables were renamed, the structure is similar but vars differ
        // The head atom "q" should match
        const expr::cons& copied_cons0 = std::get<expr::cons>(copied_body0->content);
        const expr::cons& copied_cons1 = std::get<expr::cons>(copied_body1->content);
        const expr::cons& original_cons0 = std::get<expr::cons>(original_body0->content);
        const expr::cons& original_cons1 = std::get<expr::cons>(original_body1->content);
        
        // Heads match
        assert(std::get<expr::atom>(copied_cons0.lhs->content).value == 
               std::get<expr::atom>(original_cons0.lhs->content).value);
        assert(std::get<expr::atom>(copied_cons1.lhs->content).value == 
               std::get<expr::atom>(original_cons1.lhs->content).value);
        
        // But the tail (variables) should be different pointers
        assert(copied_cons0.rhs != original_cons0.rhs);
        assert(copied_cons1.rhs != original_cons1.rhs);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 22: Exact candidate indices verification
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: 5 rules
        const expr* p = ep.atom("p");
        const expr* q = ep.atom("q");
        rule r0{p, {}};
        rule r1{p, {q}};
        rule r2{q, {}};
        rule r3{q, {p}};
        rule r4{p, {q, q}};
        a01_database db = {r0, r1, r2, r3, r4};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add goal p
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, p);
        
        // Verify exact candidate indices for g1 (should be 0,1,2,3,4)
        assert(cs.count(g1) == 5);
        auto range = cs.equal_range(g1);
        std::vector<size_t> candidates;
        for (auto it = range.first; it != range.second; ++it) {
            candidates.push_back(it->second);
        }
        std::sort(candidates.begin(), candidates.end());
        assert(candidates.size() == 5);
        assert(candidates[0] == 0);
        assert(candidates[1] == 1);
        assert(candidates[2] == 2);
        assert(candidates[3] == 3);
        assert(candidates[4] == 4);
        
        // Resolve with rule 1
        resolver(g1, 1);
        const resolution_lineage* rl = lp.resolution(g1, 1);
        
        // CRITICAL: All candidates for g1 should be removed
        assert(cs.count(g1) == 0);
        
        // New child goal q should have been added
        const goal_lineage* child = lp.goal(rl, 0);
        assert(gs.count(child) == 1);
        
        // CRITICAL: Verify exact candidate indices for child (should be 0,1,2,3,4)
        assert(cs.count(child) == 5);
        auto child_range = cs.equal_range(child);
        std::vector<size_t> child_candidates;
        for (auto it = child_range.first; it != child_range.second; ++it) {
            child_candidates.push_back(it->second);
        }
        std::sort(child_candidates.begin(), child_candidates.end());
        assert(child_candidates.size() == 5);
        assert(child_candidates[0] == 0);
        assert(child_candidates[1] == 1);
        assert(child_candidates[2] == 2);
        assert(child_candidates[3] == 3);
        assert(child_candidates[4] == 4);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 23: Variable indirection chains through multiple resolutions
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database:
        // p(X) :- q(X)
        // q(Y) :- r(Y)
        // r(a)
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        const expr* atom_a = ep.atom("a");
        
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        const expr* q_x = ep.cons(ep.atom("q"), var_x);
        const expr* q_y = ep.cons(ep.atom("q"), var_y);
        const expr* r_y = ep.cons(ep.atom("r"), var_y);
        const expr* r_a = ep.cons(ep.atom("r"), atom_a);
        
        rule r0{p_x, {q_x}};
        rule r1{q_y, {r_y}};
        rule r2{r_a, {}};
        a01_database db = {r0, r1, r2};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Start with goal p(Z) where Z is a variable
        const expr* var_z = ep.var(seq());
        uint32_t z_idx = std::get<expr::var>(var_z->content).index;
        const expr* goal_p_z = ep.cons(ep.atom("p"), var_z);
        const goal_lineage* g_p = lp.goal(nullptr, 1);
        ga(g_p, goal_p_z);
        
        // Resolution 1: p(Z) with rule 0 creates q(Z') where Z binds to Z'
        resolver(g_p, 0);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const goal_lineage* g_q = lp.goal(rl_p, 0);
        
        // Extract variable from q(Z')
        const expr* expr_q = gs.at(g_q);
        const expr::cons& cons_q = std::get<expr::cons>(expr_q->content);
        const expr* arg_q = cons_q.rhs;
        assert(std::holds_alternative<expr::var>(arg_q->content));
        uint32_t z_prime_idx = std::get<expr::var>(arg_q->content).index;
        
        // Check binding: either Z→Z' or Z'→Z
        bool first_binding_exists = (bm.bindings.count(z_idx) > 0) || (bm.bindings.count(z_prime_idx) > 0);
        assert(first_binding_exists);
        
        // Resolution 2: q(Z') with rule 1 creates r(Z'') where Z' binds to Z''
        resolver(g_q, 1);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        const goal_lineage* g_r = lp.goal(rl_q, 0);
        
        // Extract variable from r(Z'')
        const expr* expr_r = gs.at(g_r);
        const expr::cons& cons_r = std::get<expr::cons>(expr_r->content);
        const expr* arg_r = cons_r.rhs;
        assert(std::holds_alternative<expr::var>(arg_r->content));
        uint32_t z_double_prime_idx = std::get<expr::var>(arg_r->content).index;
        
        // Resolution 3: r(Z'') with rule 2 (r(a)) unifies Z'' with 'a'
        resolver(g_r, 2);
        
        // CRITICAL: Now test the chain: Z → Z' → Z'' → 'a'
        // whnf should resolve the entire chain by following bindings
        
        // Test 1: whnf on Z'' (most recent variable in chain)
        const expr* resolved_z_double = bm.whnf(arg_r);
        // MUST resolve to 'a' (no fallback)
        assert(std::holds_alternative<expr::atom>(resolved_z_double->content));
        assert(std::get<expr::atom>(resolved_z_double->content).value == "a");
        
        // Test 2: whnf on Z' (middle variable in chain)
        const expr* resolved_z_prime = bm.whnf(arg_q);
        // MUST resolve to 'a' through the chain (no fallback)
        assert(std::holds_alternative<expr::atom>(resolved_z_prime->content));
        assert(std::get<expr::atom>(resolved_z_prime->content).value == "a");
        
        // Test 3: whnf on Z (original goal variable) - STRONGEST TEST
        // Create a variable expression pointing to Z
        const expr* test_var_z = ep.var(z_idx);
        const expr* resolved_z = bm.whnf(test_var_z);
        // MUST resolve to 'a' through the full chain: Z → Z' → Z'' → 'a'
        // This is the CRITICAL assertion - no fallback paths allowed
        assert(std::holds_alternative<expr::atom>(resolved_z->content));
        assert(std::get<expr::atom>(resolved_z->content).value == "a");
        
        // Verify the chain works by checking that each step is connected
        // At least 3 bindings should exist (connecting the chain)
        assert(bm.bindings.size() >= 3);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 24: Empty body with variables in head (fact with variable)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: p(X). (fact with variable - empty body)
        const expr* var_x = ep.var(seq());
        uint32_t original_x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        rule r1{p_x, {}};  // Empty body
        a01_database db = {r1};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Goal: p(a) - concrete atom
        const expr* atom_a = ep.atom("a");
        const expr* goal_p_a = ep.cons(ep.atom("p"), atom_a);
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, goal_p_a);
        
        // Pre-resolution
        assert(gs.size() == 1);
        assert(bm.bindings.count(original_x_idx) == 0);
        
        uint32_t seq_before = seq.index;
        
        // Resolve with the fact
        resolver(g1, 0);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Sequencer advanced (variable was copied)
        assert(seq.index > seq_before);
        
        // Resolution created
        assert(rs.size() == 1);
        assert(rs.count(rl) == 1);
        
        // Goal removed
        assert(gs.count(g1) == 0);
        
        // CRITICAL: No new goals (empty body)
        assert(gs.size() == 0);
        
        // CRITICAL: But unification occurred (X' = a)
        // Exactly one variable should be in bind_map
        assert(bm.bindings.size() == 1);
        
        // CRITICAL: Original variable still not in bind_map
        assert(bm.bindings.count(original_x_idx) == 0);
        
        // Extract the renamed variable from bind_map (don't assume index)
        // There should be exactly one binding
        auto it = bm.bindings.begin();
        uint32_t renamed_x_idx = it->first;
        const expr* x_binding = it->second;
        
        // Verify the renamed variable is different from original
        assert(renamed_x_idx != original_x_idx);
        
        // Verify the binding content is 'a'
        assert(x_binding != nullptr);
        assert(std::holds_alternative<expr::atom>(x_binding->content));
        assert(std::get<expr::atom>(x_binding->content).value == "a");
        
        // Double-check through whnf
        const expr* test_var = ep.var(renamed_x_idx);
        const expr* normalized = bm.whnf(test_var);
        assert(normalized == atom_a);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 25: Multiple rules with same head
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: Three rules all with head p(X)
        const expr* var_x1 = ep.var(seq());
        const expr* var_x2 = ep.var(seq());
        const expr* var_x3 = ep.var(seq());
        
        const expr* p_x1 = ep.cons(ep.atom("p"), var_x1);
        const expr* p_x2 = ep.cons(ep.atom("p"), var_x2);
        const expr* p_x3 = ep.cons(ep.atom("p"), var_x3);
        
        const expr* q_x1 = ep.cons(ep.atom("q"), var_x1);
        const expr* r_x2 = ep.cons(ep.atom("r"), var_x2);
        const expr* s_x3 = ep.cons(ep.atom("s"), var_x3);
        
        rule r0{p_x1, {q_x1}};  // p(X) :- q(X)
        rule r1{p_x2, {r_x2}};  // p(X) :- r(X)
        rule r2{p_x3, {s_x3}};  // p(X) :- s(X)
        a01_database db = {r0, r1, r2};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Goal: p(a)
        const expr* goal_p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        ga(g1, goal_p_a);
        
        // All three rules are candidates
        assert(cs.count(g1) == 3);
        
        // Resolve with rule 1 (middle one: p(X) :- r(X))
        resolver(g1, 1);
        const resolution_lineage* rl = lp.resolution(g1, 1);
        
        // Resolution has correct index
        assert(rl->idx == 1);
        
        // One child: r(a)
        assert(gs.size() == 1);
        const goal_lineage* child = lp.goal(rl, 0);
        const expr* child_expr = gs.at(child);
        
        // CRITICAL: Child should be r(a), NOT q(a) or s(a)
        const expr::cons& cons = std::get<expr::cons>(child_expr->content);
        assert(std::get<expr::atom>(cons.lhs->content).value == "r");
        
        // Verify the argument
        const expr* arg = bm.whnf(cons.rhs);
        assert(std::holds_alternative<expr::atom>(arg->content));
        assert(std::get<expr::atom>(arg->content).value == "a");
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 26: Resolution sequence - parent then child (depth-first order)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: chained rules
        const expr* p = ep.atom("p");
        const expr* q = ep.atom("q");
        const expr* r = ep.atom("r");
        
        rule r0{p, {q}};      // p :- q
        rule r1{q, {r}};      // q :- r
        rule r2{r, {}};       // r
        a01_database db = {r0, r1, r2};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add root goal p
        const goal_lineage* g_p = lp.goal(nullptr, 0);
        ga(g_p, p);
        
        // Resolution 1: p with rule 0 → creates q
        resolver(g_p, 0);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const goal_lineage* g_q = lp.goal(rl_p, 0);
        
        // Verify lineage: g_q → rl_p → g_p → nullptr
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // Resolution 2: q with rule 1 → creates r (child immediately after parent)
        resolver(g_q, 1);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        const goal_lineage* g_r = lp.goal(rl_q, 0);
        
        // CRITICAL: Verify full lineage chain: g_r → rl_q → g_q → rl_p → g_p → nullptr
        assert(g_r->parent == rl_q);
        assert(g_r->idx == 0);
        assert(rl_q->parent == g_q);
        assert(rl_q->idx == 1);
        assert(g_q->parent == rl_p);
        assert(g_q->idx == 0);
        assert(rl_p->parent == g_p);
        assert(rl_p->idx == 0);
        assert(g_p->parent == nullptr);
        assert(g_p->idx == 0);
        
        // Resolution 3: r with rule 2
        resolver(g_r, 2);
        const resolution_lineage* rl_r = lp.resolution(g_r, 2);
        
        // Verify lineage still correct after third resolution
        assert(rl_r->parent == g_r);
        assert(g_r->parent == rl_q);
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // Verify all in stores
        assert(rs.size() == 3);
        assert(rs.count(rl_p) == 1);
        assert(rs.count(rl_q) == 1);
        assert(rs.count(rl_r) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 27: Resolution sequence - parent, independent goal, then child
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database
        const expr* p = ep.atom("p");
        const expr* q = ep.atom("q");
        const expr* x = ep.atom("x");
        const expr* y = ep.atom("y");
        const expr* z = ep.atom("z");
        
        rule r0{p, {q}};      // p :- q
        rule r1{q, {}};       // q
        rule r2{x, {y}};      // x :- y (independent branch)
        rule r3{y, {z}};      // y :- z
        rule r4{z, {}};       // z
        a01_database db = {r0, r1, r2, r3, r4};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Add two independent root goals
        const goal_lineage* g_p = lp.goal(nullptr, 0);
        ga(g_p, p);
        const goal_lineage* g_x = lp.goal(nullptr, 1);
        ga(g_x, x);
        
        assert(gs.size() == 2);
        
        // Resolution 1: p with rule 0 → creates child q
        resolver(g_p, 0);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const goal_lineage* g_q = lp.goal(rl_p, 0);
        
        // Verify lineage for first branch: g_q → rl_p → g_p → nullptr
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // Resolution 2: x (independent goal) with rule 2 → creates y
        resolver(g_x, 2);
        const resolution_lineage* rl_x = lp.resolution(g_x, 2);
        const goal_lineage* g_y = lp.goal(rl_x, 0);
        
        // CRITICAL: Verify lineage for second branch: g_y → rl_x → g_x → nullptr
        assert(g_y->parent == rl_x);
        assert(rl_x->parent == g_x);
        assert(g_x->parent == nullptr);
        
        // CRITICAL: First branch lineage should be UNCHANGED
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // Resolution 3: q (child of first branch) with rule 1
        resolver(g_q, 1);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        
        // CRITICAL: Verify first branch lineage still correct after resolving child
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // CRITICAL: Second branch lineage should STILL be unchanged
        assert(g_y->parent == rl_x);
        assert(rl_x->parent == g_x);
        assert(g_x->parent == nullptr);
        
        // Resolution 4: y (child of second branch)
        resolver(g_y, 3);
        const resolution_lineage* rl_y = lp.resolution(g_y, 3);
        const goal_lineage* g_z = lp.goal(rl_y, 0);
        
        // Verify second branch lineage: g_z → rl_y → g_y → rl_x → g_x → nullptr
        assert(g_z->parent == rl_y);
        assert(rl_y->parent == g_y);
        assert(g_y->parent == rl_x);
        assert(rl_x->parent == g_x);
        assert(g_x->parent == nullptr);
        
        // First branch still unchanged
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        
        // Resolution 5: z (leaf of second branch)
        resolver(g_z, 4);
        const resolution_lineage* rl_z = lp.resolution(g_z, 4);
        
        // Full second branch: rl_z → g_z → rl_y → g_y → rl_x → g_x → nullptr
        assert(rl_z->parent == g_z);
        assert(g_z->parent == rl_y);
        assert(rl_y->parent == g_y);
        assert(g_y->parent == rl_x);
        assert(rl_x->parent == g_x);
        assert(g_x->parent == nullptr);
        
        // All resolutions tracked
        assert(rs.size() == 5);
        assert(rs.count(rl_p) == 1);
        assert(rs.count(rl_x) == 1);
        assert(rs.count(rl_q) == 1);
        assert(rs.count(rl_y) == 1);
        assert(rs.count(rl_z) == 1);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 28: Complex resolution order - branching tree
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database
        const expr* p = ep.atom("p");
        const expr* q = ep.atom("q");
        const expr* r = ep.atom("r");
        const expr* s = ep.atom("s");
        const expr* t_atom = ep.atom("t");
        
        rule r0{p, {q, r}};   // p :- q, r (2 children)
        rule r1{q, {s}};      // q :- s
        rule r2{r, {t_atom}};  // r :- t
        rule r3{s, {}};       // s
        rule r4{t_atom, {}};  // t
        a01_database db = {r0, r1, r2, r3, r4};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Start with root p
        const goal_lineage* g_p = lp.goal(nullptr, 0);
        ga(g_p, p);
        
        // Resolution 1: p → creates q and r (two children)
        resolver(g_p, 0);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const goal_lineage* g_q = lp.goal(rl_p, 0);
        const goal_lineage* g_r = lp.goal(rl_p, 1);
        
        // Verify both children have same parent
        assert(g_q->parent == rl_p);
        assert(g_q->idx == 0);
        assert(g_r->parent == rl_p);
        assert(g_r->idx == 1);
        assert(rl_p->parent == g_p);
        
        // Resolution 2: r (right child) BEFORE q (left child)
        resolver(g_r, 2);
        const resolution_lineage* rl_r = lp.resolution(g_r, 2);
        const goal_lineage* g_t = lp.goal(rl_r, 0);
        
        // Verify right branch: g_t → rl_r → g_r → rl_p → g_p → nullptr
        assert(g_t->parent == rl_r);
        assert(rl_r->parent == g_r);
        assert(g_r->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // CRITICAL: Left branch (g_q) should be UNCHANGED
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        
        // Resolution 3: q (left child) AFTER resolving right child
        resolver(g_q, 1);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        const goal_lineage* g_s = lp.goal(rl_q, 0);
        
        // CRITICAL: Verify left branch lineage: g_s → rl_q → g_q → rl_p → g_p → nullptr
        // This should be correct even though we resolved right branch first
        assert(g_s->parent == rl_q);
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // CRITICAL: Right branch lineage should still be correct
        assert(g_t->parent == rl_r);
        assert(rl_r->parent == g_r);
        assert(g_r->parent == rl_p);
        
        // Both branches share the same resolution parent (rl_p)
        assert(g_q->parent == rl_p);
        assert(g_r->parent == rl_p);
        
        // Resolution 4: t (leaf of right branch)
        resolver(g_t, 4);
        const resolution_lineage* rl_t = lp.resolution(g_t, 4);
        
        // Resolution 5: s (leaf of left branch)
        resolver(g_s, 3);
        const resolution_lineage* rl_s = lp.resolution(g_s, 3);
        
        // Verify final lineages for both branches
        // Right: rl_t → g_t → rl_r → g_r → rl_p → g_p → nullptr
        assert(rl_t->parent == g_t);
        assert(g_t->parent == rl_r);
        assert(rl_r->parent == g_r);
        assert(g_r->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // Left: rl_s → g_s → rl_q → g_q → rl_p → g_p → nullptr
        assert(rl_s->parent == g_s);
        assert(g_s->parent == rl_q);
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // All 5 resolutions tracked
        assert(rs.size() == 5);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 29: Complex interleaved resolution order - 7 resolutions
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database with various rules
        const expr* a = ep.atom("a");
        const expr* b = ep.atom("b");
        const expr* c = ep.atom("c");
        const expr* d = ep.atom("d");
        const expr* e = ep.atom("e");
        const expr* f = ep.atom("f");
        const expr* g = ep.atom("g");
        
        rule r0{a, {b, c}};   // a :- b, c
        rule r1{b, {d}};      // b :- d
        rule r2{c, {e}};      // c :- e
        rule r3{d, {}};       // d
        rule r4{e, {f, g}};   // e :- f, g
        rule r5{f, {}};       // f
        rule r6{g, {}};       // g
        a01_database db = {r0, r1, r2, r3, r4, r5, r6};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Root: a
        const goal_lineage* g_a = lp.goal(nullptr, 0);
        ga(g_a, a);
        
        // Res 1: a → {b, c}
        resolver(g_a, 0);
        const resolution_lineage* rl_a = lp.resolution(g_a, 0);
        const goal_lineage* g_b = lp.goal(rl_a, 0);
        const goal_lineage* g_c = lp.goal(rl_a, 1);
        
        // Lineages after res 1
        assert(g_b->parent == rl_a);
        assert(g_c->parent == rl_a);
        assert(rl_a->parent == g_a);
        assert(g_a->parent == nullptr);
        
        // Res 2: c → {e} (resolve RIGHT child first)
        resolver(g_c, 2);
        const resolution_lineage* rl_c = lp.resolution(g_c, 2);
        const goal_lineage* g_e = lp.goal(rl_c, 0);
        
        // Right branch: g_e → rl_c → g_c → rl_a → g_a → nullptr
        assert(g_e->parent == rl_c);
        assert(rl_c->parent == g_c);
        assert(g_c->parent == rl_a);
        assert(rl_a->parent == g_a);
        
        // CRITICAL: Left branch (g_b) unchanged
        assert(g_b->parent == rl_a);
        
        // Res 3: e → {f, g} (continue on right branch)
        resolver(g_e, 4);
        const resolution_lineage* rl_e = lp.resolution(g_e, 4);
        const goal_lineage* g_f = lp.goal(rl_e, 0);
        const goal_lineage* g_g = lp.goal(rl_e, 1);
        
        // Right branch splits: both g_f and g_g → rl_e → g_e → rl_c → g_c → rl_a → g_a
        assert(g_f->parent == rl_e);
        assert(g_g->parent == rl_e);
        assert(rl_e->parent == g_e);
        assert(g_e->parent == rl_c);
        assert(rl_c->parent == g_c);
        assert(g_c->parent == rl_a);
        
        // CRITICAL: Left branch still unchanged
        assert(g_b->parent == rl_a);
        assert(rl_a->parent == g_a);
        
        // Res 4: b → {d} (NOW resolve left child - was waiting)
        resolver(g_b, 1);
        const resolution_lineage* rl_b = lp.resolution(g_b, 1);
        const goal_lineage* g_d = lp.goal(rl_b, 0);
        
        // CRITICAL: Left branch lineage: g_d → rl_b → g_b → rl_a → g_a → nullptr
        // Should be correct even though we resolved many other things first
        assert(g_d->parent == rl_b);
        assert(rl_b->parent == g_b);
        assert(g_b->parent == rl_a);
        assert(rl_a->parent == g_a);
        assert(g_a->parent == nullptr);
        
        // CRITICAL: Right branch still correct
        assert(g_f->parent == rl_e);
        assert(g_g->parent == rl_e);
        assert(rl_e->parent == g_e);
        assert(g_e->parent == rl_c);
        assert(g_c->parent == rl_a);
        
        // Res 5: f (leaf of right-left branch)
        resolver(g_f, 5);
        const resolution_lineage* rl_f = lp.resolution(g_f, 5);
        
        // Res 6: g (leaf of right-right branch)
        resolver(g_g, 6);
        const resolution_lineage* rl_g = lp.resolution(g_g, 6);
        
        // Res 7: d (leaf of left branch)
        resolver(g_d, 3);
        const resolution_lineage* rl_d = lp.resolution(g_d, 3);
        
        // FINAL VERIFICATION: All lineages correct after 7 resolutions
        
        // Right-left leaf: rl_f → g_f → rl_e → g_e → rl_c → g_c → rl_a → g_a → nullptr
        assert(rl_f->parent == g_f);
        assert(g_f->parent == rl_e);
        assert(rl_e->parent == g_e);
        assert(g_e->parent == rl_c);
        assert(rl_c->parent == g_c);
        assert(g_c->parent == rl_a);
        assert(rl_a->parent == g_a);
        assert(g_a->parent == nullptr);
        
        // Right-right leaf: rl_g → g_g → rl_e → g_e → rl_c → g_c → rl_a → g_a → nullptr
        assert(rl_g->parent == g_g);
        assert(g_g->parent == rl_e);
        // (rest same as right-left)
        
        // Left leaf: rl_d → g_d → rl_b → g_b → rl_a → g_a → nullptr
        assert(rl_d->parent == g_d);
        assert(g_d->parent == rl_b);
        assert(rl_b->parent == g_b);
        assert(g_b->parent == rl_a);
        assert(rl_a->parent == g_a);
        assert(g_a->parent == nullptr);
        
        // All 7 resolutions in store
        assert(rs.size() == 7);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 30: Large sequence - 10 resolutions with random order
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: chain of 10 rules
        std::vector<const expr*> atoms;
        for (int i = 0; i < 10; i++) {
            atoms.push_back(ep.atom("p" + std::to_string(i)));
        }
        
        std::vector<rule> rules;
        for (int i = 0; i < 9; i++) {
            rules.push_back(rule{atoms[i], {atoms[i+1]}});  // p0 :- p1, p1 :- p2, ...
        }
        rules.push_back(rule{atoms[9], {}});  // p9 (fact)
        
        a01_database db(rules.begin(), rules.end());
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Start with root p0
        const goal_lineage* g_p0 = lp.goal(nullptr, 0);
        ga(g_p0, atoms[0]);
        
        // Store all goal lineages and resolution lineages
        std::vector<const goal_lineage*> goals;
        std::vector<const resolution_lineage*> resolutions;
        
        goals.push_back(g_p0);
        
        // Resolve in order: p0, p1, p2, ..., p9
        for (int i = 0; i < 10; i++) {
            const goal_lineage* current_goal = goals[i];
            
            // Resolve current goal
            resolver(current_goal, i);
            const resolution_lineage* rl = lp.resolution(current_goal, i);
            resolutions.push_back(rl);
            
            // Verify resolution parent and index
            assert(rl->parent == current_goal);
            assert(rl->idx == (size_t)i);
            
            // If not the last rule, a child goal was created
            if (i < 9) {
                const goal_lineage* child = lp.goal(rl, 0);
                goals.push_back(child);
                assert(child->parent == rl);
                assert(child->idx == 0);
            }
        }
        
        // CRITICAL: Verify full chain from leaf to root
        // Start at p9's resolution (deepest) and walk back to root
        const resolution_lineage* current_rl = resolutions[9];
        const goal_lineage* current_g = goals[9];
        
        // Walk backward verifying the chain
        for (int i = 9; i >= 0; i--) {
            assert(current_rl == resolutions[i]);
            assert(current_g == goals[i]);
            assert(current_rl->parent == current_g);
            assert(current_rl->idx == (size_t)i);
            
            if (i > 0) {
                // Not root, should have resolution parent
                const resolution_lineage* parent_rl = resolutions[i-1];
                assert(current_g->parent == parent_rl);
                assert(current_g->idx == 0);
                current_g = goals[i-1];
                current_rl = parent_rl;
            } else {
                // Root goal
                assert(current_g->parent == nullptr);
                assert(current_g->idx == 0);
            }
        }
        
        // All 10 resolutions tracked
        assert(rs.size() == 10);
        for (int i = 0; i < 10; i++) {
            assert(rs.count(resolutions[i]) == 1);
        }
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 31: Out-of-order resolution with multiple independent trees
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database
        const expr* p1 = ep.atom("p1");
        const expr* p2 = ep.atom("p2");
        const expr* p3 = ep.atom("p3");
        const expr* q1 = ep.atom("q1");
        const expr* q2 = ep.atom("q2");
        const expr* q3 = ep.atom("q3");
        
        rule r0{p1, {p2}};    // p1 :- p2
        rule r1{p2, {p3}};    // p2 :- p3
        rule r2{p3, {}};      // p3
        rule r3{q1, {q2}};    // q1 :- q2
        rule r4{q2, {q3}};    // q2 :- q3
        rule r5{q3, {}};      // q3
        a01_database db = {r0, r1, r2, r3, r4, r5};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Create TWO independent root goals
        const goal_lineage* g_p1 = lp.goal(nullptr, 10);
        ga(g_p1, p1);
        const goal_lineage* g_q1 = lp.goal(nullptr, 20);
        ga(g_q1, q1);
        
        // Resolution order: p1, q1, q2, p2, q3, p3
        // (alternating between two trees)
        
        // Res 1: p1 → p2 (tree 1, level 1)
        resolver(g_p1, 0);
        const resolution_lineage* rl_p1 = lp.resolution(g_p1, 0);
        const goal_lineage* g_p2 = lp.goal(rl_p1, 0);
        
        assert(g_p2->parent == rl_p1);
        assert(rl_p1->parent == g_p1);
        assert(g_p1->parent == nullptr);
        assert(g_p1->idx == 10);
        
        // Res 2: q1 → q2 (tree 2, level 1)
        resolver(g_q1, 3);
        const resolution_lineage* rl_q1 = lp.resolution(g_q1, 3);
        const goal_lineage* g_q2 = lp.goal(rl_q1, 0);
        
        assert(g_q2->parent == rl_q1);
        assert(rl_q1->parent == g_q1);
        assert(g_q1->parent == nullptr);
        assert(g_q1->idx == 20);
        
        // CRITICAL: Tree 1 unchanged after resolving tree 2
        assert(g_p2->parent == rl_p1);
        assert(rl_p1->parent == g_p1);
        
        // Res 3: q2 → q3 (tree 2, level 2)
        resolver(g_q2, 4);
        const resolution_lineage* rl_q2 = lp.resolution(g_q2, 4);
        const goal_lineage* g_q3 = lp.goal(rl_q2, 0);
        
        assert(g_q3->parent == rl_q2);
        assert(rl_q2->parent == g_q2);
        assert(g_q2->parent == rl_q1);
        assert(rl_q1->parent == g_q1);
        assert(g_q1->parent == nullptr);
        
        // CRITICAL: Tree 1 still unchanged
        assert(g_p2->parent == rl_p1);
        assert(rl_p1->parent == g_p1);
        assert(g_p1->parent == nullptr);
        
        // Res 4: p2 → p3 (back to tree 1, level 2)
        resolver(g_p2, 1);
        const resolution_lineage* rl_p2 = lp.resolution(g_p2, 1);
        const goal_lineage* g_p3 = lp.goal(rl_p2, 0);
        
        // CRITICAL: Tree 1 full chain: g_p3 → rl_p2 → g_p2 → rl_p1 → g_p1 → nullptr
        assert(g_p3->parent == rl_p2);
        assert(rl_p2->parent == g_p2);
        assert(g_p2->parent == rl_p1);
        assert(rl_p1->parent == g_p1);
        assert(g_p1->parent == nullptr);
        
        // CRITICAL: Tree 2 unchanged
        assert(g_q3->parent == rl_q2);
        assert(rl_q2->parent == g_q2);
        assert(g_q2->parent == rl_q1);
        assert(rl_q1->parent == g_q1);
        
        // Res 5: q3 (tree 2 leaf)
        resolver(g_q3, 5);
        const resolution_lineage* rl_q3 = lp.resolution(g_q3, 5);
        
        // Tree 2 complete: rl_q3 → g_q3 → rl_q2 → g_q2 → rl_q1 → g_q1 → nullptr
        assert(rl_q3->parent == g_q3);
        assert(g_q3->parent == rl_q2);
        assert(rl_q2->parent == g_q2);
        assert(g_q2->parent == rl_q1);
        assert(rl_q1->parent == g_q1);
        assert(g_q1->parent == nullptr);
        assert(g_q1->idx == 20);
        
        // Res 6: p3 (tree 1 leaf)
        resolver(g_p3, 2);
        const resolution_lineage* rl_p3 = lp.resolution(g_p3, 2);
        
        // CRITICAL: Tree 1 complete: rl_p3 → g_p3 → rl_p2 → g_p2 → rl_p1 → g_p1 → nullptr
        assert(rl_p3->parent == g_p3);
        assert(g_p3->parent == rl_p2);
        assert(rl_p2->parent == g_p2);
        assert(g_p2->parent == rl_p1);
        assert(rl_p1->parent == g_p1);
        assert(g_p1->parent == nullptr);
        assert(g_p1->idx == 10);
        
        // CRITICAL: Both trees coexist with correct independent lineages
        // Tree 1 root has idx 10, Tree 2 root has idx 20
        // All resolutions tracked
        assert(rs.size() == 6);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 32: Resolution order - breadth-first style (all siblings before children)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database
        const expr* root = ep.atom("root");
        const expr* a = ep.atom("a");
        const expr* b = ep.atom("b");
        const expr* c = ep.atom("c");
        const expr* a1 = ep.atom("a1");
        const expr* b1 = ep.atom("b1");
        const expr* c1 = ep.atom("c1");
        
        rule r0{root, {a, b, c}};  // root :- a, b, c (3 children)
        rule r1{a, {a1}};          // a :- a1
        rule r2{b, {b1}};          // b :- b1
        rule r3{c, {c1}};          // c :- c1
        rule r4{a1, {}};           // a1
        rule r5{b1, {}};           // b1
        rule r6{c1, {}};           // c1
        a01_database db = {r0, r1, r2, r3, r4, r5, r6};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Root
        const goal_lineage* g_root = lp.goal(nullptr, 0);
        ga(g_root, root);
        
        // Res 1: root → {a, b, c}
        resolver(g_root, 0);
        const resolution_lineage* rl_root = lp.resolution(g_root, 0);
        const goal_lineage* g_a = lp.goal(rl_root, 0);
        const goal_lineage* g_b = lp.goal(rl_root, 1);
        const goal_lineage* g_c = lp.goal(rl_root, 2);
        
        // All three children share same parent
        assert(g_a->parent == rl_root);
        assert(g_a->idx == 0);
        assert(g_b->parent == rl_root);
        assert(g_b->idx == 1);
        assert(g_c->parent == rl_root);
        assert(g_c->idx == 2);
        
        // Res 2: a → a1 (first sibling)
        resolver(g_a, 1);
        const resolution_lineage* rl_a = lp.resolution(g_a, 1);
        const goal_lineage* g_a1 = lp.goal(rl_a, 0);
        
        // Res 3: b → b1 (second sibling)
        resolver(g_b, 2);
        const resolution_lineage* rl_b = lp.resolution(g_b, 2);
        const goal_lineage* g_b1 = lp.goal(rl_b, 0);
        
        // Res 4: c → c1 (third sibling)
        resolver(g_c, 3);
        const resolution_lineage* rl_c = lp.resolution(g_c, 3);
        const goal_lineage* g_c1 = lp.goal(rl_c, 0);
        
        // CRITICAL: After resolving all siblings, verify each branch maintains lineage
        
        // Branch A: g_a1 → rl_a → g_a → rl_root → g_root → nullptr
        assert(g_a1->parent == rl_a);
        assert(rl_a->parent == g_a);
        assert(g_a->parent == rl_root);
        assert(rl_root->parent == g_root);
        assert(g_root->parent == nullptr);
        
        // Branch B: g_b1 → rl_b → g_b → rl_root → g_root → nullptr
        assert(g_b1->parent == rl_b);
        assert(rl_b->parent == g_b);
        assert(g_b->parent == rl_root);
        assert(rl_root->parent == g_root);
        
        // Branch C: g_c1 → rl_c → g_c → rl_root → g_root → nullptr
        assert(g_c1->parent == rl_c);
        assert(rl_c->parent == g_c);
        assert(g_c->parent == rl_root);
        assert(rl_root->parent == g_root);
        
        // CRITICAL: All three branches share same rl_root
        assert(g_a->parent == rl_root);
        assert(g_b->parent == rl_root);
        assert(g_c->parent == rl_root);
        
        // Res 5-7: Resolve all three leaf goals
        resolver(g_a1, 4);
        const resolution_lineage* rl_a1 = lp.resolution(g_a1, 4);
        
        resolver(g_b1, 5);
        const resolution_lineage* rl_b1 = lp.resolution(g_b1, 5);
        
        resolver(g_c1, 6);
        const resolution_lineage* rl_c1 = lp.resolution(g_c1, 6);
        
        // Final verification - all three branches still have correct lineages
        assert(rl_a1->parent == g_a1);
        assert(g_a1->parent == rl_a);
        assert(rl_a->parent == g_a);
        assert(g_a->parent == rl_root);
        
        assert(rl_b1->parent == g_b1);
        assert(g_b1->parent == rl_b);
        assert(rl_b->parent == g_b);
        assert(g_b->parent == rl_root);
        
        assert(rl_c1->parent == g_c1);
        assert(g_c1->parent == rl_c);
        assert(rl_c->parent == g_c);
        assert(g_c->parent == rl_root);
        
        // All share same root
        assert(rl_root->parent == g_root);
        assert(g_root->parent == nullptr);
        
        // All 7 resolutions tracked
        assert(rs.size() == 7);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
    
    // Test 33: Random interleaving - resolve grandchild before parent's sibling
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        copier cp(seq, ep);
        lineage_pool lp;
        
        a01_resolution_store rs;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database creating deep tree
        const expr* p = ep.atom("p");
        const expr* q = ep.atom("q");
        const expr* r = ep.atom("r");
        const expr* s = ep.atom("s");
        const expr* t_atom = ep.atom("t");
        const expr* u = ep.atom("u");
        
        rule r0{p, {q, r}};   // p :- q, r
        rule r1{q, {s}};      // q :- s
        rule r2{r, {t_atom}};  // r :- t
        rule r3{s, {u}};      // s :- u
        rule r4{t_atom, {}};  // t
        rule r5{u, {}};       // u
        a01_database db = {r0, r1, r2, r3, r4, r5};
        
        a01_goal_adder ga(gs, cs, db);
        a01_avoidance_store as;
        a01_goal_resolver resolver(rs, gs, cs, db, cp, bm, lp, ga, as);
        
        // Root
        const goal_lineage* g_p = lp.goal(nullptr, 0);
        ga(g_p, p);
        
        // Res 1: p → {q, r}
        resolver(g_p, 0);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const goal_lineage* g_q = lp.goal(rl_p, 0);
        const goal_lineage* g_r = lp.goal(rl_p, 1);
        
        // Res 2: q → {s} (left branch level 2)
        resolver(g_q, 1);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        const goal_lineage* g_s = lp.goal(rl_q, 0);
        
        // Res 3: s → {u} (left branch level 3, going deep before touching right)
        resolver(g_s, 3);
        const resolution_lineage* rl_s = lp.resolution(g_s, 3);
        const goal_lineage* g_u = lp.goal(rl_s, 0);
        
        // Verify left branch (went 3 levels deep): g_u → rl_s → g_s → rl_q → g_q → rl_p → g_p
        assert(g_u->parent == rl_s);
        assert(rl_s->parent == g_s);
        assert(g_s->parent == rl_q);
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // CRITICAL: Right branch (g_r) should be untouched at level 1
        assert(g_r->parent == rl_p);
        assert(rl_p->parent == g_p);
        
        // Res 4: r → {t} (NOW resolve right branch)
        resolver(g_r, 2);
        const resolution_lineage* rl_r = lp.resolution(g_r, 2);
        const goal_lineage* g_t = lp.goal(rl_r, 0);
        
        // CRITICAL: Right branch: g_t → rl_r → g_r → rl_p → g_p → nullptr
        // Should have correct lineage even though left branch went 3 levels deep first
        assert(g_t->parent == rl_r);
        assert(rl_r->parent == g_r);
        assert(g_r->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // CRITICAL: Left branch still correct after resolving right
        assert(g_u->parent == rl_s);
        assert(rl_s->parent == g_s);
        assert(g_s->parent == rl_q);
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        
        // Both branches share rl_p and g_p
        assert(g_q->parent == rl_p);
        assert(g_r->parent == rl_p);
        
        // Res 5: u (left leaf)
        resolver(g_u, 5);
        const resolution_lineage* rl_u = lp.resolution(g_u, 5);
        
        // Res 6: t (right leaf)
        resolver(g_t, 4);
        const resolution_lineage* rl_t = lp.resolution(g_t, 4);
        
        // Final verification: both branches have complete lineages to root
        // Left: rl_u → g_u → rl_s → g_s → rl_q → g_q → rl_p → g_p → nullptr
        assert(rl_u->parent == g_u);
        assert(g_u->parent == rl_s);
        assert(rl_s->parent == g_s);
        assert(g_s->parent == rl_q);
        assert(rl_q->parent == g_q);
        assert(g_q->parent == rl_p);
        assert(rl_p->parent == g_p);
        assert(g_p->parent == nullptr);
        
        // Right: rl_t → g_t → rl_r → g_r → rl_p → g_p → nullptr
        assert(rl_t->parent == g_t);
        assert(g_t->parent == rl_r);
        assert(rl_r->parent == g_r);
        assert(g_r->parent == rl_p);
        assert(rl_p->parent == g_p);
        
        // All 6 resolutions
        assert(rs.size() == 6);
        
        // Empty avoidance store
        assert(as.size() == 0);
    }
}

void test_a01_head_elimination_detector_constructor() {
    // Test 1: Basic construction - should not crash
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Constructor should succeed without crashing
        // Members are private, so we can't verify directly
    }
    
    // Test 2: Construction with non-empty stores - should not crash
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Pre-populate stores
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* e1 = ep.atom("test");
        gs.insert({g1, e1});
        
        const expr* p = ep.atom("p");
        rule r1{p, {}};
        db.push_back(r1);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Constructor should succeed with pre-populated stores
        // Verify stores still contain data (not modified by constructor)
        assert(gs.size() == 1);
        assert(db.size() == 1);
    }
}

void test_a01_head_elimination_detector() {
    // Test 1: Unification succeeds - candidate should NOT be eliminated
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(a)
        const expr* atom_a = ep.atom("a");
        const expr* p_a = ep.cons(ep.atom("p"), atom_a);
        rule r0{p_a, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a) - matches perfectly
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, p_a});
        
        // Pre-call state
        size_t trail_depth_before = t.depth();
        size_t bindings_before = bm.bindings.size();
        assert(bindings_before == 0);
        
        // Test unification
        bool should_eliminate = detector(g1, 0);
        
        // CRITICAL: Should NOT eliminate (unification succeeded)
        assert(should_eliminate == false);
        
        // CRITICAL: Trail depth unchanged
        assert(t.depth() == trail_depth_before);
        
        // CRITICAL: Bindings unchanged (rolled back)
        assert(bm.bindings.size() == bindings_before);
        assert(bm.bindings.size() == 0);
        
        // Goal store unchanged
        assert(gs.size() == 1);
        assert(gs.at(g1) == p_a);
    }
    
    // Test 2: Unification fails - candidate SHOULD be eliminated
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(a)
        const expr* atom_a = ep.atom("a");
        const expr* p_a = ep.cons(ep.atom("p"), atom_a);
        rule r0{p_a, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(b) - does NOT match
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_b = ep.atom("b");
        const expr* p_b = ep.cons(ep.atom("p"), atom_b);
        gs.insert({g1, p_b});
        
        // Pre-call state
        size_t trail_depth_before = t.depth();
        size_t bindings_before = bm.bindings.size();
        
        // Test unification
        bool should_eliminate = detector(g1, 0);
        
        // CRITICAL: SHOULD eliminate (unification failed)
        assert(should_eliminate == true);
        
        // CRITICAL: Trail depth unchanged
        assert(t.depth() == trail_depth_before);
        
        // CRITICAL: Bindings unchanged
        assert(bm.bindings.size() == bindings_before);
        
        // Goal store unchanged
        assert(gs.size() == 1);
        assert(gs.at(g1) == p_b);
    }
    
    // Test 3: Variable in rule head - unification succeeds
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X) - variable in head
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        rule r0{p_x, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a) - should unify with p(X)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        gs.insert({g1, p_a});
        
        // Pre-call state
        size_t trail_depth_before = t.depth();
        size_t bindings_before = bm.bindings.size();
        assert(bindings_before == 0);
        assert(bm.bindings.count(x_idx) == 0);
        
        // Test
        bool should_eliminate = detector(g1, 0);
        
        // Should NOT eliminate (X can unify with a)
        assert(should_eliminate == false);
        
        // Trail unchanged
        assert(t.depth() == trail_depth_before);
        
        // CRITICAL: Bindings rolled back - X should NOT be in bind_map
        assert(bm.bindings.size() == bindings_before);
        assert(bm.bindings.count(x_idx) == 0);
        
        // Stores unchanged
        assert(gs.size() == 1);
        assert(db.size() == 1);
    }
    
    // Test 4: Multiple independent calls - no state leakage
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: three rules
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        const expr* p_b = ep.cons(ep.atom("p"), ep.atom("b"));
        const expr* q_c = ep.cons(ep.atom("q"), ep.atom("c"));
        
        rule r0{p_a, {}};
        rule r1{p_b, {}};
        rule r2{q_c, {}};
        db.push_back(r0);
        db.push_back(r1);
        db.push_back(r2);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goals
        const goal_lineage* g_pa = lp.goal(nullptr, 1);
        gs.insert({g_pa, p_a});
        const goal_lineage* g_pb = lp.goal(nullptr, 2);
        gs.insert({g_pb, p_b});
        
        size_t trail_depth = t.depth();
        
        // Call 1: p(a) vs p(a) - should NOT eliminate
        bool result1 = detector(g_pa, 0);
        assert(result1 == false);
        assert(t.depth() == trail_depth);
        assert(bm.bindings.size() == 0);
        
        // Call 2: p(a) vs p(b) - SHOULD eliminate
        bool result2 = detector(g_pa, 1);
        assert(result2 == true);
        assert(t.depth() == trail_depth);
        assert(bm.bindings.size() == 0);
        
        // Call 3: p(b) vs p(a) - SHOULD eliminate
        bool result3 = detector(g_pb, 0);
        assert(result3 == true);
        assert(t.depth() == trail_depth);
        assert(bm.bindings.size() == 0);
        
        // Call 4: p(b) vs p(b) - should NOT eliminate
        bool result4 = detector(g_pb, 1);
        assert(result4 == false);
        assert(t.depth() == trail_depth);
        assert(bm.bindings.size() == 0);
        
        // Call 5: p(a) vs q(c) - SHOULD eliminate (different predicate)
        bool result5 = detector(g_pa, 2);
        assert(result5 == true);
        assert(t.depth() == trail_depth);
        assert(bm.bindings.size() == 0);
        
        // CRITICAL: All calls independent, no state leakage
        assert(gs.size() == 2);
        assert(db.size() == 3);
    }
    
    // Test 5: Complex unification - p(X, X) patterns
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X, X) - same variable twice
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x_x = ep.cons(ep.atom("p"), ep.cons(var_x, var_x));
        rule r0{p_x_x, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal 1: p(a, a) - SHOULD unify (X=a)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_a = ep.atom("a");
        const expr* p_a_a = ep.cons(ep.atom("p"), ep.cons(atom_a, atom_a));
        gs.insert({g1, p_a_a});
        
        assert(bm.bindings.count(x_idx) == 0);
        
        bool result1 = detector(g1, 0);
        
        // Should NOT eliminate (X=a works)
        assert(result1 == false);
        
        // CRITICAL: X still not in bind_map after call
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.size() == 0);
        
        // Goal 2: p(a, b) - should NOT unify (X can't be both a and b)
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const expr* atom_b = ep.atom("b");
        const expr* p_a_b = ep.cons(ep.atom("p"), ep.cons(atom_a, atom_b));
        gs.insert({g2, p_a_b});
        
        bool result2 = detector(g2, 0);
        
        // SHOULD eliminate (X=a and X=b conflicts)
        assert(result2 == true);
        
        // Still no bindings leaked
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 6: Multiple variables - independent bindings
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X, Y) - two independent variables
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t y_idx = std::get<expr::var>(var_y->content).index;
        const expr* p_xy = ep.cons(ep.atom("p"), ep.cons(var_x, var_y));
        rule r0{p_xy, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a, b)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p_ab = ep.cons(ep.atom("p"), ep.cons(ep.atom("a"), ep.atom("b")));
        gs.insert({g1, p_ab});
        
        // Pre-call
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        size_t trail_depth = t.depth();
        
        // Test
        bool result = detector(g1, 0);
        
        // Should NOT eliminate (X=a, Y=b works)
        assert(result == false);
        
        // CRITICAL: Both variables still not in bind_map
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.size() == 0);
        assert(t.depth() == trail_depth);
    }
    
    // Test 7: Variable in goal - variable-on-variable unification
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X)
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        rule r0{p_x, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(Y) where Y is also a variable
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* var_y = ep.var(seq());
        uint32_t y_idx = std::get<expr::var>(var_y->content).index;
        const expr* p_y = ep.cons(ep.atom("p"), var_y);
        gs.insert({g1, p_y});
        
        // Pre-call: neither variable bound
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        
        // Test
        bool result = detector(g1, 0);
        
        // Should NOT eliminate (X and Y can unify)
        assert(result == false);
        
        // CRITICAL: Both variables still not bound (rolled back)
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 8: Multiple calls with existing bindings in bind_map
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X)
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        rule r0{p_x, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Pre-bind some other variable
        const expr* var_z = ep.var(seq());
        uint32_t z_idx = std::get<expr::var>(var_z->content).index;
        const expr* atom_other = ep.atom("other");
        bm.bind(z_idx, atom_other);
        
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(z_idx) == 1);
        
        // Goal: p(a)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        gs.insert({g1, p_a});
        
        // Test
        bool result = detector(g1, 0);
        
        // Should NOT eliminate
        assert(result == false);
        
        // CRITICAL: Pre-existing binding still there, nothing new added
        assert(bm.bindings.size() == 1);
        assert(bm.bindings.count(z_idx) == 1);
        assert(bm.bindings.count(x_idx) == 0);
        
        // The pre-existing binding unchanged
        assert(bm.bindings.at(z_idx) == atom_other);
    }
    
    // Test 9: Deep nested expressions - unification succeeds
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(f(g(X)))
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* g_x = ep.cons(ep.atom("g"), var_x);
        const expr* f_g_x = ep.cons(ep.atom("f"), g_x);
        const expr* p_nested = ep.cons(ep.atom("p"), f_g_x);
        rule r0{p_nested, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(f(g(a)))
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_a = ep.atom("a");
        const expr* g_a = ep.cons(ep.atom("g"), atom_a);
        const expr* f_g_a = ep.cons(ep.atom("f"), g_a);
        const expr* goal_p = ep.cons(ep.atom("p"), f_g_a);
        gs.insert({g1, goal_p});
        
        // Pre-call
        assert(bm.bindings.count(x_idx) == 0);
        size_t trail_depth = t.depth();
        
        // Test
        bool result = detector(g1, 0);
        
        // Should NOT eliminate (deep unification X=a succeeds)
        assert(result == false);
        
        // CRITICAL: X still not bound after rollback
        assert(bm.bindings.count(x_idx) == 0);
        assert(t.depth() == trail_depth);
    }
    
    // Test 10: Deep nested expressions - unification fails
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(f(g(a)))
        const expr* g_a = ep.cons(ep.atom("g"), ep.atom("a"));
        const expr* f_g_a = ep.cons(ep.atom("f"), g_a);
        const expr* p_fga = ep.cons(ep.atom("p"), f_g_a);
        rule r0{p_fga, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(f(g(b))) - 'b' instead of 'a'
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* g_b = ep.cons(ep.atom("g"), ep.atom("b"));
        const expr* f_g_b = ep.cons(ep.atom("f"), g_b);
        const expr* goal_p = ep.cons(ep.atom("p"), f_g_b);
        gs.insert({g1, goal_p});
        
        size_t trail_depth = t.depth();
        
        // Test
        bool result = detector(g1, 0);
        
        // SHOULD eliminate (a ≠ b)
        assert(result == true);
        assert(t.depth() == trail_depth);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 11: Same variable multiple times - conflicting values
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X, X, X) - same variable 3 times
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_xxx = ep.cons(ep.atom("p"), 
                            ep.cons(var_x, 
                            ep.cons(var_x, var_x)));
        rule r0{p_xxx, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal 1: p(a, a, a) - should unify (X=a consistently)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* atom_a = ep.atom("a");
        const expr* p_aaa = ep.cons(ep.atom("p"), 
                            ep.cons(atom_a, 
                            ep.cons(atom_a, atom_a)));
        gs.insert({g1, p_aaa});
        
        bool result1 = detector(g1, 0);
        assert(result1 == false); // Should NOT eliminate
        assert(bm.bindings.count(x_idx) == 0);
        
        // Goal 2: p(a, a, b) - should NOT unify (X=a and X=b conflict)
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const expr* atom_b = ep.atom("b");
        const expr* p_aab = ep.cons(ep.atom("p"), 
                            ep.cons(atom_a, 
                            ep.cons(atom_a, atom_b)));
        gs.insert({g2, p_aab});
        
        bool result2 = detector(g2, 0);
        assert(result2 == true); // SHOULD eliminate
        assert(bm.bindings.count(x_idx) == 0);
        
        // Goal 3: p(a, b, a) - should NOT unify
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const expr* p_aba = ep.cons(ep.atom("p"), 
                            ep.cons(atom_a, 
                            ep.cons(atom_b, atom_a)));
        gs.insert({g3, p_aba});
        
        bool result3 = detector(g3, 0);
        assert(result3 == true); // SHOULD eliminate
        assert(bm.bindings.count(x_idx) == 0);
        
        // CRITICAL: All calls independent
        assert(bm.bindings.size() == 0);
    }
    
    // Test 12: Multiple variables with variable goal
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X, Y)
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t y_idx = std::get<expr::var>(var_y->content).index;
        const expr* p_xy = ep.cons(ep.atom("p"), ep.cons(var_x, var_y));
        rule r0{p_xy, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(A, B) where A and B are also variables
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* var_a = ep.var(seq());
        const expr* var_b = ep.var(seq());
        uint32_t a_idx = std::get<expr::var>(var_a->content).index;
        uint32_t b_idx = std::get<expr::var>(var_b->content).index;
        const expr* p_ab = ep.cons(ep.atom("p"), ep.cons(var_a, var_b));
        gs.insert({g1, p_ab});
        
        // Pre-call: no variables bound
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.count(a_idx) == 0);
        assert(bm.bindings.count(b_idx) == 0);
        
        // Test
        bool result = detector(g1, 0);
        
        // Should NOT eliminate (variable-on-variable unification succeeds)
        assert(result == false);
        
        // CRITICAL: All four variables still not bound
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.count(a_idx) == 0);
        assert(bm.bindings.count(b_idx) == 0);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 13: Different predicates - immediate failure
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(a)
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        rule r0{p_a, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: q(a) - different predicate
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* q_a = ep.cons(ep.atom("q"), ep.atom("a"));
        gs.insert({g1, q_a});
        
        // Test
        bool result = detector(g1, 0);
        
        // SHOULD eliminate (different predicates)
        assert(result == true);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 14: Arity mismatch
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(a, b)
        const expr* p_ab = ep.cons(ep.atom("p"), ep.cons(ep.atom("a"), ep.atom("b")));
        rule r0{p_ab, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a) - different arity
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        gs.insert({g1, p_a});
        
        // Test
        bool result = detector(g1, 0);
        
        // SHOULD eliminate (arity mismatch)
        assert(result == true);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 15: Multiple database entries - test different indices
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: multiple rules with variables
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        const expr* var_z = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t y_idx = std::get<expr::var>(var_y->content).index;
        uint32_t z_idx = std::get<expr::var>(var_z->content).index;
        
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        const expr* p_y = ep.cons(ep.atom("p"), var_y);
        const expr* q_z = ep.cons(ep.atom("q"), var_z);
        
        rule r0{p_x, {}};  // p(X)
        rule r1{p_y, {}};  // p(Y) - same head, different variable
        rule r2{q_z, {}};  // q(Z)
        db.push_back(r0);
        db.push_back(r1);
        db.push_back(r2);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        gs.insert({g1, p_a});
        
        // Test all three rules
        bool result0 = detector(g1, 0);  // p(X) vs p(a)
        assert(result0 == false);        // Should NOT eliminate
        assert(bm.bindings.count(x_idx) == 0);
        
        bool result1 = detector(g1, 1);  // p(Y) vs p(a)
        assert(result1 == false);        // Should NOT eliminate
        assert(bm.bindings.count(y_idx) == 0);
        
        bool result2 = detector(g1, 2);  // q(Z) vs p(a)
        assert(result2 == true);         // SHOULD eliminate (different predicate)
        assert(bm.bindings.count(z_idx) == 0);
        
        // CRITICAL: All calls independent, no cross-contamination
        assert(bm.bindings.size() == 0);
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.count(z_idx) == 0);
    }
    
    // Test 16: Stress test - many calls in sequence
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: 10 rules with different patterns
        std::vector<const expr*> vars;
        for (int i = 0; i < 10; i++) {
            vars.push_back(ep.var(seq()));
        }
        
        // Create rules: p0(X0), p1(X1), ..., p9(X9)
        for (int i = 0; i < 10; i++) {
            const expr* p = ep.cons(ep.atom("p" + std::to_string(i)), vars[i]);
            rule r{p, {}};
            db.push_back(r);
        }
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p5(a) - will match rule 5 only
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p5_a = ep.cons(ep.atom("p5"), ep.atom("a"));
        gs.insert({g1, p5_a});
        
        size_t trail_depth = t.depth();
        
        // Test all 10 rules
        for (size_t i = 0; i < 10; i++) {
            bool result = detector(g1, i);
            
            if (i == 5) {
                // Rule 5: p5(X5) should match p5(a)
                assert(result == false);
            } else {
                // All others: different predicates
                assert(result == true);
            }
            
            // CRITICAL: After each call, state unchanged
            assert(t.depth() == trail_depth);
            assert(bm.bindings.size() == 0);
            
            // Verify all original variables still not bound
            for (int j = 0; j < 10; j++) {
                uint32_t var_idx = std::get<expr::var>(vars[j]->content).index;
                assert(bm.bindings.count(var_idx) == 0);
            }
        }
    }
    
    // Test 17: Trail depth verification with nested pushes
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(a)
        const expr* p_a = ep.cons(ep.atom("p"), ep.atom("a"));
        rule r0{p_a, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, p_a});
        
        // Start at depth 1
        assert(t.depth() == 1);
        
        // Call detector (internally does push/pop)
        bool result1 = detector(g1, 0);
        assert(result1 == false);
        assert(t.depth() == 1); // Back to 1
        
        // Push another frame
        t.push();
        assert(t.depth() == 2);
        
        // Call detector again
        bool result2 = detector(g1, 0);
        assert(result2 == false);
        assert(t.depth() == 2); // Still at 2
        
        // Pop
        t.pop();
        assert(t.depth() == 1);
        
        // Call detector at depth 1 again
        bool result3 = detector(g1, 0);
        assert(result3 == false);
        assert(t.depth() == 1);
    }
    
    // Test 18: Complex variables - same variable in rule and goal
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X, Y, X) - X appears twice
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t y_idx = std::get<expr::var>(var_y->content).index;
        const expr* p_xyx = ep.cons(ep.atom("p"), 
                            ep.cons(var_x, 
                            ep.cons(var_y, var_x)));
        rule r0{p_xyx, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(A, B, A) where A and B are goal variables
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* var_a = ep.var(seq());
        const expr* var_b = ep.var(seq());
        uint32_t a_idx = std::get<expr::var>(var_a->content).index;
        uint32_t b_idx = std::get<expr::var>(var_b->content).index;
        const expr* p_aba = ep.cons(ep.atom("p"), 
                            ep.cons(var_a, 
                            ep.cons(var_b, var_a)));
        gs.insert({g1, p_aba});
        
        // Pre-call: no variables bound
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.count(a_idx) == 0);
        assert(bm.bindings.count(b_idx) == 0);
        
        // Test - complex variable-on-variable unification
        bool result = detector(g1, 0);
        
        // Should NOT eliminate (X=A, Y=B, and consistency X=A maintained)
        assert(result == false);
        
        // CRITICAL: All variables still unbound after rollback
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.count(y_idx) == 0);
        assert(bm.bindings.count(a_idx) == 0);
        assert(bm.bindings.count(b_idx) == 0);
        assert(bm.bindings.size() == 0);
    }
    
    // Test 19: CRITICAL - Verify bind_map exact state preservation
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Pre-bind some variables to create initial state
        const expr* var_pre1 = ep.var(seq());
        const expr* var_pre2 = ep.var(seq());
        uint32_t pre1_idx = std::get<expr::var>(var_pre1->content).index;
        uint32_t pre2_idx = std::get<expr::var>(var_pre2->content).index;
        bm.bind(pre1_idx, ep.atom("existing1"));
        bm.bind(pre2_idx, ep.atom("existing2"));
        
        assert(bm.bindings.size() == 2);
        const expr* binding1_before = bm.bindings.at(pre1_idx);
        const expr* binding2_before = bm.bindings.at(pre2_idx);
        
        // Database: p(X, Y)
        const expr* var_x = ep.var(seq());
        const expr* var_y = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        uint32_t y_idx = std::get<expr::var>(var_y->content).index;
        const expr* p_xy = ep.cons(ep.atom("p"), ep.cons(var_x, var_y));
        rule r0{p_xy, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(a, b)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* p_ab = ep.cons(ep.atom("p"), ep.cons(ep.atom("a"), ep.atom("b")));
        gs.insert({g1, p_ab});
        
        // Test
        bool result = detector(g1, 0);
        
        // Should NOT eliminate
        assert(result == false);
        
        // CRITICAL: Exact binding state preserved
        assert(bm.bindings.size() == 2);  // Still only 2
        assert(bm.bindings.count(pre1_idx) == 1);
        assert(bm.bindings.count(pre2_idx) == 1);
        assert(bm.bindings.count(x_idx) == 0);  // Rule vars not added
        assert(bm.bindings.count(y_idx) == 0);
        
        // CRITICAL: Exact same pointer values
        assert(bm.bindings.at(pre1_idx) == binding1_before);
        assert(bm.bindings.at(pre2_idx) == binding2_before);
    }
    
    // Test 20: Occurs check failure
    {
        trail t;
        t.push();
        expr_pool ep(t);
        sequencer seq(t);
        bind_map bm(t);
        lineage_pool lp;
        
        a01_goal_store gs;
        a01_database db;
        
        // Database: p(X) where X is a variable
        const expr* var_x = ep.var(seq());
        uint32_t x_idx = std::get<expr::var>(var_x->content).index;
        const expr* p_x = ep.cons(ep.atom("p"), var_x);
        rule r0{p_x, {}};
        db.push_back(r0);
        
        a01_head_elimination_detector detector(t, bm, gs, db);
        
        // Goal: p(f(X)) - X occurs inside its own binding (occurs check)
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* f_x = ep.cons(ep.atom("f"), var_x);
        const expr* p_fx = ep.cons(ep.atom("p"), f_x);
        gs.insert({g1, p_fx});
        
        // Test - this attempts to unify X with f(X), which should fail
        bool result = detector(g1, 0);
        
        // SHOULD eliminate (occurs check failure)
        assert(result == true);
        
        // Variable still not bound
        assert(bm.bindings.count(x_idx) == 0);
        assert(bm.bindings.size() == 0);
    }
}

void test_unit_propagation_detector_constructor() {
    // Test 1: Basic construction with empty candidate store
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        unit_propagation_detector detector(cs);
        
        // Verify reference stored
        assert(&detector.cs == &cs);
    }
    
    // Test 2: Construction with non-empty candidate store
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        // Pre-populate candidate store
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g2, 0});
        
        assert(cs.size() == 3);
        
        unit_propagation_detector detector(cs);
        
        // Verify reference and that store wasn't modified
        assert(&detector.cs == &cs);
        assert(detector.cs.size() == 3);
    }
}

void test_unit_propagation_detector() {
    // Test 1: Goal with exactly 1 candidate - should return true (unit propagation)
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 0});  // Exactly one candidate
        
        assert(cs.count(g1) == 1);
        
        unit_propagation_detector detector(cs);
        
        bool result = detector(g1);
        
        // CRITICAL: Should return true (unit propagation detected)
        assert(result == true);
        
        // Candidate store unchanged
        assert(cs.count(g1) == 1);
        assert(cs.size() == 1);
    }
    
    // Test 2: Goal with 0 candidates - should return false (no unit propagation)
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        // Don't add any candidates for g1
        
        assert(cs.count(g1) == 0);
        
        unit_propagation_detector detector(cs);
        
        bool result = detector(g1);
        
        // Should return false (no candidates at all)
        assert(result == false);
        
        // Store unchanged
        assert(cs.size() == 0);
    }
    
    // Test 3: Goal with 2 candidates - should return false (no unit propagation)
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 0});
        cs.insert({g1, 1});  // Two candidates
        
        assert(cs.count(g1) == 2);
        
        unit_propagation_detector detector(cs);
        
        bool result = detector(g1);
        
        // Should return false (multiple candidates, no forced choice)
        assert(result == false);
        
        // Store unchanged
        assert(cs.count(g1) == 2);
        assert(cs.size() == 2);
    }
    
    // Test 4: Goal with many candidates - should return false
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        
        // Add 10 candidates
        for (size_t i = 0; i < 10; i++) {
            cs.insert({g1, i});
        }
        
        assert(cs.count(g1) == 10);
        
        unit_propagation_detector detector(cs);
        
        bool result = detector(g1);
        
        // Should return false (many candidates)
        assert(result == false);
        
        // Store unchanged
        assert(cs.count(g1) == 10);
        assert(cs.size() == 10);
    }
    
    // Test 5: Multiple goals with varying candidate counts
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        // Goal 1: 0 candidates
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        
        // Goal 2: 1 candidate (unit)
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        cs.insert({g2, 0});
        
        // Goal 3: 2 candidates
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        cs.insert({g3, 0});
        cs.insert({g3, 1});
        
        // Goal 4: 1 candidate (unit)
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        cs.insert({g4, 5});
        
        // Goal 5: 3 candidates
        const goal_lineage* g5 = lp.goal(nullptr, 5);
        cs.insert({g5, 0});
        cs.insert({g5, 1});
        cs.insert({g5, 2});
        
        assert(cs.size() == 7); // Total entries
        
        unit_propagation_detector detector(cs);
        
        // Test each goal
        assert(detector(g1) == false);  // 0 candidates
        assert(detector(g2) == true);   // 1 candidate (UNIT)
        assert(detector(g3) == false);  // 2 candidates
        assert(detector(g4) == true);   // 1 candidate (UNIT)
        assert(detector(g5) == false);  // 3 candidates
        
        // CRITICAL: Multiple calls should be consistent (no side effects)
        assert(detector(g2) == true);
        assert(detector(g4) == true);
        assert(detector(g1) == false);
        
        // Store completely unchanged
        assert(cs.size() == 7);
        assert(cs.count(g1) == 0);
        assert(cs.count(g2) == 1);
        assert(cs.count(g3) == 2);
        assert(cs.count(g4) == 1);
        assert(cs.count(g5) == 3);
    }
    
    // Test 6: Goal not in candidate store at all - should return false
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        // Add some goals to the store
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 0});
        
        // But check a different goal not in the store
        const goal_lineage* g_missing = lp.goal(nullptr, 999);
        
        unit_propagation_detector detector(cs);
        
        bool result = detector(g_missing);
        
        // Should return false (not in store means 0 candidates)
        assert(result == false);
        assert(cs.count(g_missing) == 0);
    }
    
    // Test 7: Same goal multiple times - verify idempotence
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 0});  // Single candidate
        
        unit_propagation_detector detector(cs);
        
        // Call multiple times
        for (int i = 0; i < 100; i++) {
            bool result = detector(g1);
            assert(result == true);
        }
        
        // CRITICAL: Store unchanged after 100 calls
        assert(cs.count(g1) == 1);
        assert(cs.size() == 1);
    }
    
    // Test 8: Transition from multiple to unit after elimination simulation
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        
        unit_propagation_detector detector(cs);
        
        // Initially not a unit (3 candidates)
        assert(detector(g1) == false);
        assert(cs.count(g1) == 3);
        
        // Simulate elimination: remove candidates
        auto range = cs.equal_range(g1);
        auto it = range.first;
        ++it; // Skip first
        cs.erase(it); // Erase second
        
        // Now 2 candidates
        assert(cs.count(g1) == 2);
        assert(detector(g1) == false);
        
        // Remove one more
        range = cs.equal_range(g1);
        it = range.first;
        ++it;
        cs.erase(it);
        
        // Now exactly 1 candidate (UNIT!)
        assert(cs.count(g1) == 1);
        assert(detector(g1) == true);
    }
    
    // Test 9: Stress test - many goals with varying counts
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        std::vector<const goal_lineage*> goals;
        std::vector<size_t> expected_counts;
        
        // Create 50 goals with varying candidate counts (0-5 each)
        for (int i = 0; i < 50; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            goals.push_back(g);
            
            size_t num_candidates = i % 6; // 0, 1, 2, 3, 4, 5, 0, 1, ...
            expected_counts.push_back(num_candidates);
            
            for (size_t j = 0; j < num_candidates; j++) {
                cs.insert({g, j});
            }
        }
        
        unit_propagation_detector detector(cs);
        
        // Verify each goal
        for (int i = 0; i < 50; i++) {
            bool result = detector(goals[i]);
            bool expected_unit = (expected_counts[i] == 1);
            
            assert(result == expected_unit);
            assert(cs.count(goals[i]) == expected_counts[i]);
        }
        
        // Verify total count
        size_t total = 0;
        for (size_t count : expected_counts) {
            total += count;
        }
        assert(cs.size() == total);
    }
    
    // Test 10: Multiple detectors on same store - verify independence
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 0});
        
        unit_propagation_detector detector1(cs);
        unit_propagation_detector detector2(cs);
        
        // Both should give same result
        assert(detector1(g1) == true);
        assert(detector2(g1) == true);
        
        // Store unchanged
        assert(cs.count(g1) == 1);
    }
    
    // Test 11: Edge case - exactly 1 vs exactly 2 (boundary condition)
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g_one = lp.goal(nullptr, 1);
        cs.insert({g_one, 0});
        
        const goal_lineage* g_two = lp.goal(nullptr, 2);
        cs.insert({g_two, 0});
        cs.insert({g_two, 1});
        
        unit_propagation_detector detector(cs);
        
        // CRITICAL: 1 should return true, 2 should return false
        assert(detector(g_one) == true);
        assert(detector(g_two) == false);
        
        // Verify exact counts
        assert(cs.count(g_one) == 1);
        assert(cs.count(g_two) == 2);
    }
    
    // Test 12: Same candidate index for different goals
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        // All have candidate at index 0, but different counts
        cs.insert({g1, 0});  // 1 candidate
        
        cs.insert({g2, 0});  // 2 candidates
        cs.insert({g2, 1});
        
        cs.insert({g3, 0});  // 3 candidates
        cs.insert({g3, 1});
        cs.insert({g3, 2});
        
        unit_propagation_detector detector(cs);
        
        // Only g1 is a unit
        assert(detector(g1) == true);
        assert(detector(g2) == false);
        assert(detector(g3) == false);
    }
    
    // Test 13: Candidates with various indices - verify index doesn't matter
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 42});  // Single candidate at index 42
        
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        cs.insert({g2, 0});   // Single candidate at index 0
        
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        cs.insert({g3, 999}); // Single candidate at index 999
        
        unit_propagation_detector detector(cs);
        
        // CRITICAL: All are units regardless of index value
        assert(detector(g1) == true);
        assert(detector(g2) == true);
        assert(detector(g3) == true);
        
        // Verify indices preserved
        auto range1 = cs.equal_range(g1);
        assert(range1.first->second == 42);
        
        auto range2 = cs.equal_range(g2);
        assert(range2.first->second == 0);
        
        auto range3 = cs.equal_range(g3);
        assert(range3.first->second == 999);
    }
    
    // Test 14: Deep lineage tree - unit propagation at different levels
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        // Create tree structure
        const goal_lineage* g_root = lp.goal(nullptr, 0);
        const resolution_lineage* rl_1 = lp.resolution(g_root, 0);
        const goal_lineage* g_child1 = lp.goal(rl_1, 0);
        const goal_lineage* g_child2 = lp.goal(rl_1, 1);
        const resolution_lineage* rl_2 = lp.resolution(g_child1, 0);
        const goal_lineage* g_grandchild = lp.goal(rl_2, 0);
        
        // Root: 2 candidates
        cs.insert({g_root, 0});
        cs.insert({g_root, 1});
        
        // Child 1: 1 candidate (unit)
        cs.insert({g_child1, 0});
        
        // Child 2: 3 candidates
        cs.insert({g_child2, 0});
        cs.insert({g_child2, 1});
        cs.insert({g_child2, 2});
        
        // Grandchild: 1 candidate (unit)
        cs.insert({g_grandchild, 5});
        
        unit_propagation_detector detector(cs);
        
        // CRITICAL: Only child1 and grandchild are units
        assert(detector(g_root) == false);      // 2 candidates
        assert(detector(g_child1) == true);     // 1 candidate (UNIT)
        assert(detector(g_child2) == false);    // 3 candidates
        assert(detector(g_grandchild) == true); // 1 candidate (UNIT)
    }
    
    // Test 15: Simulate typical solver workflow
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        // Initial state: 3 goals, each with multiple candidates
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        
        cs.insert({g3, 0});
        cs.insert({g3, 1});
        cs.insert({g3, 2});
        cs.insert({g3, 3});
        
        unit_propagation_detector detector(cs);
        
        // Initially no units
        assert(detector(g1) == false);  // 3 candidates
        assert(detector(g2) == false);  // 2 candidates
        assert(detector(g3) == false);  // 4 candidates
        
        // Simulate head elimination on g1: remove 2 candidates
        auto range = cs.equal_range(g1);
        auto it = range.first;
        ++it;
        cs.erase(it);  // Remove second
        
        range = cs.equal_range(g1);
        it = range.first;
        ++it;
        cs.erase(it);  // Remove third (now second)
        
        // g1 now has 1 candidate (became a unit!)
        assert(cs.count(g1) == 1);
        assert(detector(g1) == true);  // NOW it's a unit!
        
        // Simulate head elimination on g2: remove 1 candidate
        range = cs.equal_range(g2);
        it = range.first;
        ++it;
        cs.erase(it);
        
        // g2 now has 1 candidate (became a unit!)
        assert(cs.count(g2) == 1);
        assert(detector(g2) == true);  // NOW it's a unit!
        
        // g3 still has 4 candidates
        assert(detector(g3) == false);
        
        // Final verification
        assert(cs.count(g1) == 1);
        assert(cs.count(g2) == 1);
        assert(cs.count(g3) == 4);
    }
    
    // Test 16: Empty candidate store - all queries return false
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        unit_propagation_detector detector(cs);
        
        // Create various goals but don't add candidates
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        // All should return false (empty store)
        assert(detector(g1) == false);
        assert(detector(g2) == false);
        assert(detector(g3) == false);
        
        assert(cs.size() == 0);
    }
    
    // Test 17: Large candidate indices - verify only count matters, not index values
    {
        lineage_pool lp;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        cs.insert({g1, 1000000});  // Very large index, but still just 1 candidate
        
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        cs.insert({g2, 5000});
        cs.insert({g2, 9999});  // Two large indices
        
        unit_propagation_detector detector(cs);
        
        // CRITICAL: Index value doesn't matter, only count
        assert(detector(g1) == true);   // 1 candidate (unit)
        assert(detector(g2) == false);  // 2 candidates
    }
}

void test_solution_detector_constructor() {
    // Test 1: Basic construction with empty goal store
    {
        a01_goal_store gs;
        
        solution_detector detector(gs);
        
        // Verify reference stored
        assert(&detector.gs == &gs);
    }
    
    // Test 2: Construction with non-empty goal store
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        // Pre-populate goal store
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const expr* e1 = ep.atom("p");
        const expr* e2 = ep.atom("q");
        gs.insert({g1, e1});
        gs.insert({g2, e2});
        
        assert(gs.size() == 2);
        
        solution_detector detector(gs);
        
        // Verify reference and that store wasn't modified
        assert(&detector.gs == &gs);
        assert(detector.gs.size() == 2);
    }
}

void test_solution_detector() {
    // Test 1: Empty goal store - solution found (returns true)
    {
        a01_goal_store gs;
        
        assert(gs.empty());
        
        solution_detector detector(gs);
        
        bool result = detector();
        
        // CRITICAL: Should return true (solution found, no goals remain)
        assert(result == true);
        
        // Store unchanged
        assert(gs.empty());
    }
    
    // Test 2: Goal store with 1 goal - no solution (returns false)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* e1 = ep.atom("p");
        gs.insert({g1, e1});
        
        assert(gs.size() == 1);
        
        solution_detector detector(gs);
        
        bool result = detector();
        
        // Should return false (still have goals to resolve)
        assert(result == false);
        
        // Store unchanged
        assert(gs.size() == 1);
        assert(gs.count(g1) == 1);
    }
    
    // Test 3: Goal store with 2 goals - no solution
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const expr* e1 = ep.atom("p");
        const expr* e2 = ep.atom("q");
        gs.insert({g1, e1});
        gs.insert({g2, e2});
        
        assert(gs.size() == 2);
        
        solution_detector detector(gs);
        
        bool result = detector();
        
        // Should return false
        assert(result == false);
        
        // Store unchanged
        assert(gs.size() == 2);
    }
    
    // Test 4: Goal store with many goals - no solution
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        // Add 10 goals
        for (int i = 0; i < 10; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            const expr* e = ep.atom("p" + std::to_string(i));
            gs.insert({g, e});
        }
        
        assert(gs.size() == 10);
        
        solution_detector detector(gs);
        
        bool result = detector();
        
        // Should return false (many unresolved goals)
        assert(result == false);
        
        // Store unchanged
        assert(gs.size() == 10);
    }
    
    // Test 5: Multiple calls on empty store - verify idempotence
    {
        a01_goal_store gs;
        
        solution_detector detector(gs);
        
        // Call 100 times
        for (int i = 0; i < 100; i++) {
            bool result = detector();
            assert(result == true);
        }
        
        // CRITICAL: Store still empty after 100 calls
        assert(gs.empty());
    }
    
    // Test 6: Multiple calls on non-empty store - verify idempotence
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        solution_detector detector(gs);
        
        // Call multiple times
        for (int i = 0; i < 50; i++) {
            bool result = detector();
            assert(result == false);
        }
        
        // CRITICAL: Store unchanged
        assert(gs.size() == 1);
    }
    
    // Test 7: Simulate solver progression - goals → empty (solution reached)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        // Start with 3 goals
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        solution_detector detector(gs);
        
        // Initially no solution
        assert(detector() == false);
        assert(gs.size() == 3);
        
        // "Resolve" goal 1 (simulate by removing)
        gs.erase(g1);
        assert(detector() == false);
        assert(gs.size() == 2);
        
        // "Resolve" goal 2
        gs.erase(g2);
        assert(detector() == false);
        assert(gs.size() == 1);
        
        // "Resolve" goal 3 (last one)
        gs.erase(g3);
        
        // NOW we have a solution!
        assert(detector() == true);
        assert(gs.empty());
    }
    
    // Test 8: Multiple detectors on same store
    {
        a01_goal_store gs;
        
        solution_detector detector1(gs);
        solution_detector detector2(gs);
        
        // Both should return same result
        assert(detector1() == true);
        assert(detector2() == true);
        
        // Store unchanged
        assert(gs.empty());
    }
    
    // Test 9: Goals at different lineage depths - only count matters
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        // Root goal
        const goal_lineage* g_root = lp.goal(nullptr, 0);
        
        // Child goal (level 1)
        const resolution_lineage* rl1 = lp.resolution(g_root, 0);
        const goal_lineage* g_child = lp.goal(rl1, 0);
        
        // Grandchild goal (level 2)
        const resolution_lineage* rl2 = lp.resolution(g_child, 0);
        const goal_lineage* g_grandchild = lp.goal(rl2, 0);
        
        // Add all three to store
        gs.insert({g_root, ep.atom("p")});
        gs.insert({g_child, ep.atom("q")});
        gs.insert({g_grandchild, ep.atom("r")});
        
        solution_detector detector(gs);
        
        // CRITICAL: Lineage depth doesn't matter, only that goals exist
        assert(detector() == false);
        assert(gs.size() == 3);
        
        // Remove all
        gs.clear();
        
        // Now solution found
        assert(detector() == true);
        assert(gs.empty());
    }
    
    // Test 10: Boundary - exactly 1 goal vs empty
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const expr* e1 = ep.atom("p");
        
        solution_detector detector(gs);
        
        // Empty: solution
        assert(detector() == true);
        
        // Add 1 goal: no solution
        gs.insert({g1, e1});
        assert(detector() == false);
        assert(gs.size() == 1);
        
        // Remove it: solution again
        gs.erase(g1);
        assert(detector() == true);
        assert(gs.empty());
    }
    
    // Test 11: Simulate complete resolution sequence with goal_adder integration
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Database: p :- q, q :- r, r (chain)
        const expr* p = ep.atom("p");
        const expr* q = ep.atom("q");
        const expr* r = ep.atom("r");
        rule r0{p, {q}};
        rule r1{q, {r}};
        rule r2{r, {}};
        a01_database db = {r0, r1, r2};
        
        a01_goal_adder ga(gs, cs, db);
        solution_detector detector(gs);
        
        // Initially empty - solution found (trivially)
        assert(detector() == true);
        
        // Add root goal p
        const goal_lineage* g_p = lp.goal(nullptr, 0);
        ga(g_p, p);
        
        // Now have 1 goal - no solution
        assert(detector() == false);
        assert(gs.size() == 1);
        
        // "Resolve" p → creates q
        gs.erase(g_p);
        cs.erase(g_p);
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const goal_lineage* g_q = lp.goal(rl_p, 0);
        ga(g_q, q);
        
        // Still have 1 goal (q) - no solution
        assert(detector() == false);
        assert(gs.size() == 1);
        
        // "Resolve" q → creates r
        gs.erase(g_q);
        cs.erase(g_q);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        const goal_lineage* g_r = lp.goal(rl_q, 0);
        ga(g_r, r);
        
        // Still have 1 goal (r) - no solution
        assert(detector() == false);
        assert(gs.size() == 1);
        
        // "Resolve" r (fact with empty body)
        gs.erase(g_r);
        cs.erase(g_r);
        
        // NOW empty - solution found!
        assert(detector() == true);
        assert(gs.empty());
    }
    
    // Test 12: Stress test - transition from many goals to solution
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        
        // Add 100 goals
        std::vector<const goal_lineage*> goals;
        for (int i = 0; i < 100; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            gs.insert({g, ep.atom("p" + std::to_string(i))});
            goals.push_back(g);
        }
        
        solution_detector detector(gs);
        
        // Initially no solution
        assert(detector() == false);
        
        // Remove goals one by one
        for (int i = 0; i < 99; i++) {
            gs.erase(goals[i]);
            // Still have goals remaining
            assert(detector() == false);
            assert(gs.size() == 100 - i - 1);
        }
        
        // Remove last goal
        gs.erase(goals[99]);
        
        // NOW solution found!
        assert(detector() == true);
        assert(gs.empty());
    }
}

void test_conflict_detector_constructor() {
    // Test 1: Basic construction with empty stores
    {
        a01_goal_store gs;
        a01_candidate_store cs;
        
        conflict_detector detector(gs, cs);
        
        // Verify references stored
        assert(&detector.gs == &gs);
        assert(&detector.cs == &cs);
    }
    
    // Test 2: Construction with non-empty stores
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Pre-populate stores
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        
        assert(gs.size() == 2);
        assert(cs.size() == 3);
        
        conflict_detector detector(gs, cs);
        
        // Verify references and stores unchanged
        assert(&detector.gs == &gs);
        assert(&detector.cs == &cs);
        assert(detector.gs.size() == 2);
        assert(detector.cs.size() == 3);
    }
}

void test_conflict_detector() {
    // Test 1: No goals in store - no conflict (returns false)
    {
        a01_goal_store gs;
        a01_candidate_store cs;
        
        assert(gs.empty());
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: No goals = no conflict
        assert(result == false);
        
        // Stores unchanged
        assert(gs.empty());
        assert(cs.empty());
    }
    
    // Test 2: One goal with 1 candidate - no conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        cs.insert({g1, 0});
        
        assert(gs.size() == 1);
        assert(cs.count(g1) == 1);
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // Should return false (goal has a candidate)
        assert(result == false);
        
        // Stores unchanged
        assert(gs.size() == 1);
        assert(cs.size() == 1);
    }
    
    // Test 3: One goal with 0 candidates - CONFLICT (returns true)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        // Don't add any candidates for g1
        
        assert(gs.size() == 1);
        assert(cs.count(g1) == 0);
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should return true (CONFLICT - no candidates for g1)
        assert(result == true);
        
        // Stores unchanged
        assert(gs.size() == 1);
        assert(cs.count(g1) == 0);
    }
    
    // Test 4: Multiple goals, all have candidates - no conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        cs.insert({g1, 0});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g3, 0});
        cs.insert({g3, 1});
        cs.insert({g3, 2});
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // Should return false (all goals have candidates)
        assert(result == false);
        
        // Verify counts
        assert(cs.count(g1) == 1);
        assert(cs.count(g2) == 2);
        assert(cs.count(g3) == 3);
    }
    
    // Test 5: Multiple goals, one has 0 candidates - CONFLICT
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        // g1 and g3 have candidates, g2 has NONE
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g3, 0});
        // g2 has no candidates
        
        assert(cs.count(g2) == 0);
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should return true (CONFLICT - g2 has no candidates)
        assert(result == true);
        
        // Stores unchanged
        assert(gs.size() == 3);
        assert(cs.count(g1) == 2);
        assert(cs.count(g2) == 0);
        assert(cs.count(g3) == 1);
    }
    
    // Test 6: Multiple goals with 0 candidates - still just one conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        // All three have NO candidates
        assert(cs.count(g1) == 0);
        assert(cs.count(g2) == 0);
        assert(cs.count(g3) == 0);
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should return true (multiple conflicts)
        assert(result == true);
    }
    
    // Test 7: Simulate head elimination creating conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        // Initially has 3 candidates
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        
        conflict_detector detector(gs, cs);
        
        // Initially no conflict
        assert(detector() == false);
        assert(cs.count(g1) == 3);
        
        // Simulate head elimination: remove candidates one by one
        auto range = cs.equal_range(g1);
        auto it = range.first;
        cs.erase(it);
        
        // Still 2 candidates - no conflict
        assert(detector() == false);
        assert(cs.count(g1) == 2);
        
        // Remove another
        range = cs.equal_range(g1);
        it = range.first;
        cs.erase(it);
        
        // Still 1 candidate - no conflict
        assert(detector() == false);
        assert(cs.count(g1) == 1);
        
        // Remove last candidate
        range = cs.equal_range(g1);
        it = range.first;
        cs.erase(it);
        
        // NOW conflict!
        assert(cs.count(g1) == 0);
        assert(detector() == true);
    }
    
    // Test 8: Multiple calls - verify idempotence with conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        // No candidates
        
        conflict_detector detector(gs, cs);
        
        // Call 100 times - should always return true
        for (int i = 0; i < 100; i++) {
            bool result = detector();
            assert(result == true);
        }
        
        // CRITICAL: Stores unchanged
        assert(gs.size() == 1);
        assert(cs.count(g1) == 0);
    }
    
    // Test 9: Multiple calls - verify idempotence without conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        cs.insert({g1, 0});
        
        conflict_detector detector(gs, cs);
        
        // Call 100 times - should always return false
        for (int i = 0; i < 100; i++) {
            bool result = detector();
            assert(result == false);
        }
        
        // CRITICAL: Stores unchanged
        assert(gs.size() == 1);
        assert(cs.count(g1) == 1);
    }
    
    // Test 10: Mixed scenario - some goals with candidates, some without
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        gs.insert({g4, ep.atom("s")});
        
        // g1: 5 candidates
        for (int i = 0; i < 5; i++) cs.insert({g1, (size_t)i});
        
        // g2: 0 candidates (CONFLICT!)
        
        // g3: 1 candidate
        cs.insert({g3, 0});
        
        // g4: 3 candidates
        cs.insert({g4, 0});
        cs.insert({g4, 1});
        cs.insert({g4, 2});
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should return true (g2 has no candidates)
        assert(result == true);
        
        // Verify specific counts
        assert(cs.count(g1) == 5);
        assert(cs.count(g2) == 0);  // The conflict
        assert(cs.count(g3) == 1);
        assert(cs.count(g4) == 3);
    }
    
    // Test 11: Transition from no conflict to conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        
        conflict_detector detector(gs, cs);
        
        // Initially no conflict
        assert(detector() == false);
        
        // Remove g1's only candidate
        cs.erase(cs.find(g1));
        
        // NOW conflict!
        assert(cs.count(g1) == 0);
        assert(detector() == true);
    }
    
    // Test 12: Transition from conflict to no conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        // No candidates initially
        
        conflict_detector detector(gs, cs);
        
        // Initially conflict
        assert(detector() == true);
        
        // Add a candidate
        cs.insert({g1, 0});
        
        // NOW no conflict!
        assert(cs.count(g1) == 1);
        assert(detector() == false);
    }
    
    // Test 13: Goals at different lineage depths - all positions matter
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Create lineage tree
        const goal_lineage* g_root = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g_root, 0);
        const goal_lineage* g_child = lp.goal(rl1, 0);
        const resolution_lineage* rl2 = lp.resolution(g_child, 0);
        const goal_lineage* g_grandchild = lp.goal(rl2, 0);
        
        gs.insert({g_root, ep.atom("p")});
        gs.insert({g_child, ep.atom("q")});
        gs.insert({g_grandchild, ep.atom("r")});
        
        // Root: 1 candidate
        cs.insert({g_root, 0});
        
        // Child: 0 candidates (CONFLICT!)
        // (no candidates for g_child)
        
        // Grandchild: 2 candidates
        cs.insert({g_grandchild, 0});
        cs.insert({g_grandchild, 1});
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should detect conflict even though it's at level 1
        assert(result == true);
        assert(cs.count(g_child) == 0);
    }
    
    // Test 14: Many goals, all with candidates - no conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Add 20 goals, each with at least 1 candidate
        for (int i = 0; i < 20; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            gs.insert({g, ep.atom("p" + std::to_string(i))});
            
            // Each goal has (i+1) candidates
            for (int j = 0; j <= i; j++) {
                cs.insert({g, (size_t)j});
            }
        }
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // Should return false (all have candidates)
        assert(result == false);
        
        // Verify all have candidates
        for (int i = 0; i < 20; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            assert(cs.count(g) == i + 1);
        }
    }
    
    // Test 15: Many goals, one in middle has no candidates - conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Add 10 goals
        for (int i = 0; i < 10; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            gs.insert({g, ep.atom("p" + std::to_string(i))});
            
            // All except g5 get candidates
            if (i != 5) {
                cs.insert({g, 0});
            }
        }
        
        // g5 specifically has no candidates
        const goal_lineage* g5 = lp.goal(nullptr, 5);
        assert(cs.count(g5) == 0);
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should detect conflict (g5 has no candidates)
        assert(result == true);
    }
    
    // Test 16: Resolve goals to eliminate conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        // g2 has no candidates - conflict
        
        conflict_detector detector(gs, cs);
        
        // Conflict detected
        assert(detector() == true);
        
        // "Resolve" conflicting goal by removing it from goal store
        // (simulating backtrack/restart)
        gs.erase(g2);
        
        // Now no conflict (conflicting goal removed)
        assert(detector() == false);
        assert(gs.size() == 1);
    }
    
    // Test 17: Boundary - exactly 0 candidates vs exactly 1 candidate
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g_zero = lp.goal(nullptr, 1);
        const goal_lineage* g_one = lp.goal(nullptr, 2);
        
        gs.insert({g_zero, ep.atom("p")});
        gs.insert({g_one, ep.atom("q")});
        
        // g_zero: 0 candidates
        // g_one: 1 candidate
        cs.insert({g_one, 0});
        
        conflict_detector detector(gs, cs);
        
        // CRITICAL: Conflict because g_zero has 0 (boundary)
        assert(detector() == true);
        assert(cs.count(g_zero) == 0);
        assert(cs.count(g_one) == 1);
        
        // Add candidate to g_zero
        cs.insert({g_zero, 0});
        
        // Now no conflict (both have 1)
        assert(detector() == false);
    }
    
    // Test 18: Empty candidate store with goals - all goals conflict
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Add goals but no candidates
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        assert(gs.size() == 3);
        assert(cs.empty());
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Conflict (all goals have 0 candidates)
        assert(result == true);
    }
    
    // Test 19: Verify std::any_of short-circuits (first conflict found)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Add 10 goals - first one has no candidates
        for (int i = 0; i < 10; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            gs.insert({g, ep.atom("p" + std::to_string(i))});
            
            if (i > 0) {
                cs.insert({g, 0}); // All except first have candidates
            }
        }
        
        const goal_lineage* g0 = lp.goal(nullptr, 0);
        assert(cs.count(g0) == 0);
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // Should detect conflict (first goal has none)
        assert(result == true);
    }
    
    // Test 20: Multiple detectors on same stores
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        // No candidates - conflict
        
        conflict_detector detector1(gs, cs);
        conflict_detector detector2(gs, cs);
        
        // Both should return same result
        assert(detector1() == true);
        assert(detector2() == true);
        
        // Stores unchanged
        assert(gs.size() == 1);
        assert(cs.count(g1) == 0);
    }
    
    // Test 21: Typical solver workflow - no conflict initially, then conflict after eliminations
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Three goals with multiple candidates each
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g2, 2});
        
        cs.insert({g3, 0});
        
        conflict_detector detector(gs, cs);
        
        // Initially no conflict
        assert(detector() == false);
        
        // Simulate eliminations on g1
        cs.erase(cs.find(g1));
        cs.erase(cs.find(g1));
        
        // g1 now has 0 candidates - CONFLICT!
        assert(cs.count(g1) == 0);
        assert(detector() == true);
        
        // "Backtrack" by removing conflicting goal
        gs.erase(g1);
        
        // No conflict now (conflicting goal gone)
        assert(detector() == false);
        assert(gs.size() == 2);
    }
    
    // Test 22: Stress test - 100 goals with varying candidate counts
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Add 100 goals
        for (int i = 0; i < 100; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            gs.insert({g, ep.atom("p" + std::to_string(i))});
            
            // Goals get (i % 5) candidates: 0, 1, 2, 3, 4, 0, 1, ...
            int num_candidates = i % 5;
            for (int j = 0; j < num_candidates; j++) {
                cs.insert({g, (size_t)j});
            }
        }
        
        conflict_detector detector(gs, cs);
        
        bool result = detector();
        
        // CRITICAL: Should detect conflict (goals with i%5==0 have no candidates)
        assert(result == true);
        
        // Verify at least one goal has 0 candidates
        bool found_zero = false;
        for (int i = 0; i < 100; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            if (cs.count(g) == 0) {
                found_zero = true;
                break;
            }
        }
        assert(found_zero == true);
    }
}

void test_a01_cdcl_elimination_detector_constructor() {
    // Test 1: Basic construction with empty stores
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Verify references stored
        assert(&detector.as == &as);
        assert(&detector.lp == &lp);
    }
    
    // Test 2: Construction with non-empty avoidance store
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Pre-populate avoidance store
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        
        a01_decision_store avoidance1;
        avoidance1.insert(rl1);
        avoidance1.insert(rl2);
        as.insert(avoidance1);
        
        a01_decision_store avoidance2;
        avoidance2.insert(rl1);
        as.insert(avoidance2);
        
        assert(as.size() == 2);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Verify references and stores unchanged
        assert(&detector.as == &as);
        assert(&detector.lp == &lp);
        assert(detector.as.size() == 2);
    }
}

void test_a01_cdcl_elimination_detector() {
    // Test 1: Empty avoidance store - no eliminations
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        assert(as.empty());
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test arbitrary goal/index combination
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        
        bool result = detector(g1, 0);
        
        // Should return false (no avoidances, no elimination)
        assert(result == false);
        
        // Store unchanged
        assert(as.empty());
    }
    
    // Test 2: Avoidance with singleton containing exact rl - SHOULD ELIMINATE
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Create singleton avoidance containing rl
        a01_decision_store avoidance;
        avoidance.insert(rl);
        as.insert(avoidance);
        
        assert(as.size() == 1);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test the exact goal/index combination
        bool result = detector(g1, 0);
        
        // CRITICAL: Should return true (rl is singleton in avoidance)
        assert(result == true);
        
        // Stores unchanged
        assert(as.size() == 1);
    }
    
    // Test 3: Avoidance with singleton, but different rl - should NOT eliminate
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const resolution_lineage* rl_in_avoidance = lp.resolution(g1, 0);
        
        // Avoidance contains rl for g1,0
        a01_decision_store avoidance;
        avoidance.insert(rl_in_avoidance);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test different goal/index (g2, 0)
        bool result = detector(g2, 0);
        
        // Should return false (different resolution)
        assert(result == false);
        
        // Store unchanged
        assert(as.size() == 1);
    }
    
    // Test 4: Avoidance with 2+ resolutions - should NOT eliminate
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        
        // Avoidance with 2 resolutions (NOT singleton)
        a01_decision_store avoidance;
        avoidance.insert(rl1);
        avoidance.insert(rl2);
        as.insert(avoidance);
        
        assert(as.size() == 1);
        assert(as.begin()->size() == 2);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test rl1 - should NOT eliminate (not singleton)
        bool result1 = detector(g1, 0);
        assert(result1 == false);
        
        // Test rl2 - should NOT eliminate (not singleton)
        bool result2 = detector(g2, 0);
        assert(result2 == false);
        
        // Stores unchanged
        assert(as.size() == 1);
    }
    
    // Test 5: Multiple avoidances, one is singleton - SHOULD ELIMINATE
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g3, 0);
        
        // Avoidance 1: {rl1, rl2} - not singleton
        a01_decision_store avoidance1;
        avoidance1.insert(rl1);
        avoidance1.insert(rl2);
        as.insert(avoidance1);
        
        // Avoidance 2: {rl3} - SINGLETON!
        a01_decision_store avoidance2;
        avoidance2.insert(rl3);
        as.insert(avoidance2);
        
        // Avoidance 3: {rl1, rl3} - not singleton
        a01_decision_store avoidance3;
        avoidance3.insert(rl1);
        avoidance3.insert(rl3);
        as.insert(avoidance3);
        
        assert(as.size() == 3);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test rl1 - should NOT eliminate (in non-singletons only)
        bool result1 = detector(g1, 0);
        assert(result1 == false);
        
        // Test rl2 - should NOT eliminate (in non-singleton only)
        bool result2 = detector(g2, 0);
        assert(result2 == false);
        
        // Test rl3 - SHOULD ELIMINATE (exists as singleton)
        bool result3 = detector(g3, 0);
        assert(result3 == true);
    }
    
    // Test 6: Same goal, different indices - independent
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl_idx0 = lp.resolution(g1, 0);
        const resolution_lineage* rl_idx1 = lp.resolution(g1, 1);
        const resolution_lineage* rl_idx2 = lp.resolution(g1, 2);
        
        // Avoidance: only {rl_idx1} as singleton
        a01_decision_store avoidance;
        avoidance.insert(rl_idx1);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test different indices for same goal
        bool result0 = detector(g1, 0);  // rl_idx0
        assert(result0 == false);
        
        bool result1 = detector(g1, 1);  // rl_idx1 - SHOULD ELIMINATE
        assert(result1 == true);
        
        bool result2 = detector(g1, 2);  // rl_idx2
        assert(result2 == false);
        
        // Stores unchanged
        assert(as.size() == 1);
    }
    
    // Test 7: Multiple goals in goal store with various avoidances
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        // Create various resolutions
        const resolution_lineage* rl1_0 = lp.resolution(g1, 0);
        const resolution_lineage* rl1_1 = lp.resolution(g1, 1);
        const resolution_lineage* rl2_0 = lp.resolution(g2, 0);
        const resolution_lineage* rl3_0 = lp.resolution(g3, 0);
        
        // Avoidance 1: {rl1_0} - singleton
        a01_decision_store avoidance1;
        avoidance1.insert(rl1_0);
        as.insert(avoidance1);
        
        // Avoidance 2: {rl2_0, rl3_0} - not singleton
        a01_decision_store avoidance2;
        avoidance2.insert(rl2_0);
        avoidance2.insert(rl3_0);
        as.insert(avoidance2);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // g1, idx 0 - SHOULD ELIMINATE (singleton)
        assert(detector(g1, 0) == true);
        
        // g1, idx 1 - should NOT eliminate
        assert(detector(g1, 1) == false);
        
        // g2, idx 0 - should NOT eliminate (in non-singleton)
        assert(detector(g2, 0) == false);
        
        // g3, idx 0 - should NOT eliminate (in non-singleton)
        assert(detector(g3, 0) == false);
    }
    
    // Test 8: Multiple calls - verify idempotence
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        a01_decision_store avoidance;
        avoidance.insert(rl);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Call 100 times
        for (int i = 0; i < 100; i++) {
            bool result = detector(g1, 0);
            assert(result == true);
        }
        
        // CRITICAL: Stores unchanged after 100 calls
        assert(as.size() == 1);
    }
    
    // Test 9: Simulate progressive narrowing (CDCL workflow)
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g3, 0);
        
        // Initial learned clause: {rl1, rl2, rl3} - avoid all three together
        a01_decision_store avoidance;
        avoidance.insert(rl1);
        avoidance.insert(rl2);
        avoidance.insert(rl3);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Initially none should be eliminated (not singletons)
        assert(detector(g1, 0) == false);
        assert(detector(g2, 0) == false);
        assert(detector(g3, 0) == false);
        
        // Simulate making resolution rl1 (would be done by goal_resolver)
        // Extract, modify, re-insert
        auto it = as.find(avoidance);
        auto node = as.extract(it);
        node.value().erase(rl1);
        as.insert(std::move(node));
        
        // Now avoidance = {rl2, rl3} - still not singleton
        assert(detector(g1, 0) == false);  // Not in avoidance anymore
        assert(detector(g2, 0) == false);  // Still not singleton
        assert(detector(g3, 0) == false);  // Still not singleton
        
        // Simulate making resolution rl2
        it = as.begin();
        node = as.extract(it);
        node.value().erase(rl2);
        as.insert(std::move(node));
        
        // Now avoidance = {rl3} - SINGLETON!
        assert(detector(g1, 0) == false);  // Not in avoidance
        assert(detector(g2, 0) == false);  // Not in avoidance
        assert(detector(g3, 0) == true);   // MUST ELIMINATE (singleton!)
    }
    
    // Test 10: Multiple singletons for same goal with different indices
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl_idx0 = lp.resolution(g1, 0);
        const resolution_lineage* rl_idx1 = lp.resolution(g1, 1);
        const resolution_lineage* rl_idx2 = lp.resolution(g1, 2);
        
        // Three separate singleton avoidances
        a01_decision_store avoidance0;
        avoidance0.insert(rl_idx0);
        as.insert(avoidance0);
        
        a01_decision_store avoidance1;
        avoidance1.insert(rl_idx1);
        as.insert(avoidance1);
        
        // Don't add avoidance for rl_idx2
        
        assert(as.size() == 2);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: idx 0 and 1 should be eliminated, idx 2 should not
        assert(detector(g1, 0) == true);   // In singleton
        assert(detector(g1, 1) == true);   // In singleton
        assert(detector(g1, 2) == false);  // Not in any avoidance
    }
    
    // Test 11: Deep lineage structure - verify interning works correctly
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Create deep tree
        const goal_lineage* g_root = lp.goal(nullptr, 0);
        const resolution_lineage* rl1 = lp.resolution(g_root, 0);
        const goal_lineage* g_child = lp.goal(rl1, 0);
        const resolution_lineage* rl2 = lp.resolution(g_child, 5);
        const goal_lineage* g_grandchild = lp.goal(rl2, 10);
        
        // Avoidance: singleton containing grandchild's potential resolution
        const resolution_lineage* rl_grandchild = lp.resolution(g_grandchild, 3);
        a01_decision_store avoidance;
        avoidance.insert(rl_grandchild);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Test the grandchild resolution - should eliminate
        bool result = detector(g_grandchild, 3);
        assert(result == true);
        
        // Test other levels - should not eliminate
        assert(detector(g_root, 0) == false);
        assert(detector(g_child, 5) == false);
    }
    
    // Test 12: Same avoidance checked multiple times - idempotence
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        a01_decision_store avoidance;
        avoidance.insert(rl);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Multiple detectors on same stores
        a01_cdcl_elimination_detector detector2(as, lp);
        
        // Both should give same result
        assert(detector(g1, 0) == true);
        assert(detector2(g1, 0) == true);
        
        // Call first detector many times
        for (int i = 0; i < 50; i++) {
            assert(detector(g1, 0) == true);
        }
        
        // Stores unchanged
        assert(as.size() == 1);
    }
    
    // Test 13: Empty avoidance in store - should not cause elimination
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Add empty avoidance (this could happen after all resolutions erased)
        a01_decision_store empty_avoidance;
        as.insert(empty_avoidance);
        
        assert(as.size() == 1);
        assert(as.begin()->size() == 0);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        
        // Should NOT eliminate (empty set != singleton)
        bool result = detector(g1, 0);
        assert(result == false);
    }
    
    // Test 14: Multiple singletons, testing various combinations
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Create 5 different resolutions
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        const goal_lineage* g5 = lp.goal(nullptr, 5);
        
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g3, 0);
        const resolution_lineage* rl4 = lp.resolution(g4, 0);
        const resolution_lineage* rl5 = lp.resolution(g5, 0);
        
        // Add singletons for rl1 and rl3 only
        a01_decision_store av1;
        av1.insert(rl1);
        as.insert(av1);
        
        a01_decision_store av3;
        av3.insert(rl3);
        as.insert(av3);
        
        // Add non-singleton for rl2, rl4, rl5
        a01_decision_store av_multi;
        av_multi.insert(rl2);
        av_multi.insert(rl4);
        av_multi.insert(rl5);
        as.insert(av_multi);
        
        assert(as.size() == 3);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: Only rl1 and rl3 should be eliminated
        assert(detector(g1, 0) == true);   // In singleton
        assert(detector(g2, 0) == false);  // In non-singleton
        assert(detector(g3, 0) == true);   // In singleton
        assert(detector(g4, 0) == false);  // In non-singleton
        assert(detector(g5, 0) == false);  // In non-singleton
    }
    
    // Test 15: Verify lineage_pool interning - same goal/index returns same rl
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        
        // Pre-construct rl
        const resolution_lineage* rl_first = lp.resolution(g1, 0);
        
        // Add to avoidance
        a01_decision_store avoidance;
        avoidance.insert(rl_first);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: detector internally calls lp.resolution(g1, 0) again
        // Due to interning, it should get the SAME pointer
        bool result = detector(g1, 0);
        
        // Should eliminate (same interned instance)
        assert(result == true);
        
        // Verify it's actually the same instance
        const resolution_lineage* rl_second = lp.resolution(g1, 0);
        assert(rl_first == rl_second); // Same pointer (interned)
    }
    
    // Test 16: Complex avoidance patterns - multiple sizes
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        const resolution_lineage* rl3 = lp.resolution(g3, 0);
        const resolution_lineage* rl4 = lp.resolution(g4, 0);
        
        // Avoidance size 0: empty
        a01_decision_store av0;
        as.insert(av0);
        
        // Avoidance size 1: {rl1}
        a01_decision_store av1;
        av1.insert(rl1);
        as.insert(av1);
        
        // Avoidance size 2: {rl2, rl3}
        a01_decision_store av2;
        av2.insert(rl2);
        av2.insert(rl3);
        as.insert(av2);
        
        // Avoidance size 3: {rl1, rl2, rl4}
        a01_decision_store av3;
        av3.insert(rl1);
        av3.insert(rl2);
        av3.insert(rl4);
        as.insert(av3);
        
        assert(as.size() == 4);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: Only rl1 should be eliminated (exists as singleton)
        assert(detector(g1, 0) == true);   // In singleton av1
        assert(detector(g2, 0) == false);  // In non-singletons only
        assert(detector(g3, 0) == false);  // In non-singleton only
        assert(detector(g4, 0) == false);  // In non-singleton only
    }
    
    // Test 17: Stress test - many avoidances, one singleton
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Create 50 resolutions
        std::vector<const resolution_lineage*> resolutions;
        for (int i = 0; i < 50; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            const resolution_lineage* rl = lp.resolution(g, 0);
            resolutions.push_back(rl);
        }
        
        // Add 25 non-singleton avoidances
        for (int i = 0; i < 25; i++) {
            a01_decision_store avoidance;
            avoidance.insert(resolutions[i * 2]);
            avoidance.insert(resolutions[i * 2 + 1]);
            as.insert(avoidance);
        }
        
        // Add ONE singleton for rl at index 25
        a01_decision_store singleton;
        singleton.insert(resolutions[25]);
        as.insert(singleton);
        
        assert(as.size() == 26);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: Only goal 25, idx 0 should be eliminated
        for (int i = 0; i < 50; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            bool result = detector(g, 0);
            
            if (i == 25) {
                assert(result == true);  // Singleton
            } else {
                assert(result == false); // Not in singleton
            }
        }
    }
    
    // Test 18: No goals in lineage pool - detector should still work
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Create goal and resolution without adding to any stores
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        a01_decision_store avoidance;
        avoidance.insert(rl);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Should eliminate (rl exists as singleton)
        bool result = detector(g1, 0);
        assert(result == true);
    }
    
    // Test 19: Different goal same index vs same goal different index
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        const resolution_lineage* rl_g1_idx5 = lp.resolution(g1, 5);
        
        // Singleton for g1,idx5
        a01_decision_store avoidance;
        avoidance.insert(rl_g1_idx5);
        as.insert(avoidance);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: Only g1,idx5 should eliminate
        assert(detector(g1, 5) == true);   // Exact match
        assert(detector(g1, 0) == false);  // Same goal, different index
        assert(detector(g2, 5) == false);  // Different goal, same index
        assert(detector(g2, 0) == false);  // Different goal, different index
    }
    
    // Test 20: Multiple singleton avoidances for same resolution (duplicates)
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const resolution_lineage* rl = lp.resolution(g1, 0);
        
        // Add same singleton multiple times (set will deduplicate)
        a01_decision_store avoidance1;
        avoidance1.insert(rl);
        as.insert(avoidance1);
        
        a01_decision_store avoidance2;
        avoidance2.insert(rl);
        as.insert(avoidance2);  // Duplicate - will be ignored by set
        
        // std::set deduplicates
        assert(as.size() == 1);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Should still eliminate
        bool result = detector(g1, 0);
        assert(result == true);
    }
    
    // Test 21: Verify count() check (as.count({rl}) > 0)
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const resolution_lineage* rl1 = lp.resolution(g1, 0);
        const resolution_lineage* rl2 = lp.resolution(g2, 0);
        
        // Multiple avoidances with different sizes containing rl1
        a01_decision_store av_single;
        av_single.insert(rl1);
        as.insert(av_single);
        
        a01_decision_store av_double;
        av_double.insert(rl1);
        av_double.insert(rl2);
        as.insert(av_double);
        
        assert(as.size() == 2);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // CRITICAL: rl1 should eliminate because {rl1} exists as singleton
        // Even though rl1 also appears in a non-singleton
        bool result = detector(g1, 0);
        assert(result == true);
    }
    
    // Test 22: Simulate complete CDCL scenario
    {
        lineage_pool lp;
        a01_avoidance_store as;
        
        // Learned conflict clause: "Don't resolve p with rule 0, q with rule 1, and r with rule 2 together"
        const goal_lineage* g_p = lp.goal(nullptr, 1);
        const goal_lineage* g_q = lp.goal(nullptr, 2);
        const goal_lineage* g_r = lp.goal(nullptr, 3);
        
        const resolution_lineage* rl_p = lp.resolution(g_p, 0);
        const resolution_lineage* rl_q = lp.resolution(g_q, 1);
        const resolution_lineage* rl_r = lp.resolution(g_r, 2);
        
        // Initial learned clause
        a01_decision_store learned_clause;
        learned_clause.insert(rl_p);
        learned_clause.insert(rl_q);
        learned_clause.insert(rl_r);
        as.insert(learned_clause);
        
        a01_cdcl_elimination_detector detector(as, lp);
        
        // Phase 1: No eliminations yet (all in 3-way avoidance)
        assert(detector(g_p, 0) == false);
        assert(detector(g_q, 1) == false);
        assert(detector(g_r, 2) == false);
        
        // Phase 2: Make resolution rl_p (simulated by extraction/modification)
        auto it = as.find(learned_clause);
        auto node = as.extract(it);
        node.value().erase(rl_p);
        as.insert(std::move(node));
        
        // Now clause = {rl_q, rl_r}
        assert(detector(g_p, 0) == false);  // Not in avoidance anymore
        assert(detector(g_q, 1) == false);  // Still size 2
        assert(detector(g_r, 2) == false);  // Still size 2
        
        // Phase 3: Make resolution rl_q
        it = as.begin();
        node = as.extract(it);
        node.value().erase(rl_q);
        as.insert(std::move(node));
        
        // Now clause = {rl_r} - SINGLETON!
        assert(detector(g_p, 0) == false);  // Not in avoidance
        assert(detector(g_q, 1) == false);  // Not in avoidance
        assert(detector(g_r, 2) == true);   // MUST ELIMINATE!
        
        // This prevents taking rl_r, which would complete the conflict set
    }
}

void test_a01_decider_constructor() {
    // Test 1: Basic construction with empty stores
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Verify references stored
        assert(&decider.gs == &gs);
        assert(&decider.cs == &cs);
        assert(&decider.sim == &sim);
    }
    
    // Test 2: Construction with non-empty stores
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        // Pre-populate stores
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g2, 0});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Verify references and stores unchanged
        assert(&decider.gs == &gs);
        assert(&decider.cs == &cs);
        assert(decider.gs.size() == 2);
        assert(decider.cs.size() == 3);
    }
}

void test_a01_decider_choose_goal() {
    // Test 1: Single goal - should return that goal
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        size_t length_before = sim.length();
        
        const goal_lineage* chosen = decider.choose_goal();
        
        // CRITICAL: Should return the only goal
        assert(chosen == g1);
        
        // CRITICAL: Simulation length increments by 1
        assert(sim.length() == length_before + 1);
        
        // Store unchanged
        assert(gs.size() == 1);
    }
    
    // Test 2: Multiple goals with unvisited node - unvisited chosen first
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Pre-populate tree: g1 and g3 visited, g2 unvisited
        root.m_visits = 10;
        root.m_children[g1].m_visits = 5;
        root.m_children[g1].m_value = 10.0;
        
        root.m_children[g2].m_visits = 0;  // UNVISITED - infinity UCB1!
        root.m_children[g2].m_value = 0.0;
        
        root.m_children[g3].m_visits = 5;
        root.m_children[g3].m_value = 10.0;
        
        a01_decider decider(gs, cs, sim);
        
        const goal_lineage* chosen = decider.choose_goal();
        
        // CRITICAL: Should choose g2 (unvisited = infinity UCB1)
        assert(chosen == g2);
        
        // Simulation length incremented
        assert(sim.length() == 1);
    }
    
    // Test 3: Multiple goals with different average rewards - highest chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Pre-populate tree: all visited, g2 has highest average reward
        root.m_visits = 100;
        
        // g1: avg = 50/10 = 5.0
        root.m_children[g1].m_visits = 10;
        root.m_children[g1].m_value = 50.0;
        
        // g2: avg = 900/10 = 90.0 (HIGHEST!)
        root.m_children[g2].m_visits = 10;
        root.m_children[g2].m_value = 900.0;
        
        // g3: avg = 30/10 = 3.0
        root.m_children[g3].m_visits = 10;
        root.m_children[g3].m_value = 30.0;
        
        a01_decider decider(gs, cs, sim);
        
        const goal_lineage* chosen = decider.choose_goal();
        
        // CRITICAL: Should choose g2 (highest average reward)
        assert(chosen == g2);
        
        // Length incremented
        assert(sim.length() == 1);
    }
    
    // Test 4: Multiple goals, verify result is always valid
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Call multiple times
        for (int i = 0; i < 20; i++) {
            const goal_lineage* chosen = decider.choose_goal();
            
            // CRITICAL: Result must be one of the three goals
            assert(chosen == g1 || chosen == g2 || chosen == g3);
            assert(gs.count(chosen) == 1);
        }
        
        // Simulation length should be 20
        assert(sim.length() == 20);
    }
    
    // Test 5: Two goals with similar rewards - both should be selectable
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.5, rng);
        
        // Pre-populate with similar rewards
        root.m_visits = 50;
        root.m_children[g1].m_visits = 20;
        root.m_children[g1].m_value = 60.0;  // avg = 3.0
        root.m_children[g2].m_visits = 20;
        root.m_children[g2].m_value = 62.0;  // avg = 3.1 (slightly higher)
        
        a01_decider decider(gs, cs, sim);
        
        const goal_lineage* chosen = decider.choose_goal();
        
        // Should choose g2 (slightly higher average)
        assert(chosen == g2);
        assert(sim.length() == 1);
    }
    
    // Test 6: Many goals - verify only valid goals chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        std::vector<const goal_lineage*> goals;
        for (int i = 0; i < 20; i++) {
            const goal_lineage* g = lp.goal(nullptr, i);
            gs.insert({g, ep.atom("p" + std::to_string(i))});
            goals.push_back(g);
        }
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(123);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        size_t length_before = sim.length();
        const goal_lineage* chosen = decider.choose_goal();
        
        // CRITICAL: Result must be in goal store
        assert(gs.count(chosen) == 1);
        
        // Verify it's one of our goals
        bool found = false;
        for (const auto* g : goals) {
            if (chosen == g) {
                found = true;
                break;
            }
        }
        assert(found == true);
        
        // Length incremented by 1
        assert(sim.length() == length_before + 1);
    }
}

void test_a01_decider_choose_candidate() {
    // Test 1: Single candidate - should return that candidate
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        cs.insert({g1, 5});  // Only candidate is index 5
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        size_t length_before = sim.length();
        
        size_t chosen = decider.choose_candidate(g1);
        
        // CRITICAL: Should return the only candidate
        assert(chosen == 5);
        
        // CRITICAL: Simulation length increments by 1
        assert(sim.length() == length_before + 1);
        
        // Store unchanged
        assert(cs.count(g1) == 1);
    }
    
    // Test 2: Multiple candidates with unvisited node - unvisited chosen first
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Pre-populate tree: indices 0 and 2 visited, index 1 unvisited
        root.m_visits = 10;
        
        root.m_children[size_t(0)].m_visits = 3;
        root.m_children[size_t(0)].m_value = 9.0;
        
        root.m_children[size_t(1)].m_visits = 0;  // UNVISITED - infinity UCB1!
        root.m_children[size_t(1)].m_value = 0.0;
        
        root.m_children[size_t(2)].m_visits = 3;
        root.m_children[size_t(2)].m_value = 9.0;
        
        a01_decider decider(gs, cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        // CRITICAL: Should choose index 1 (unvisited = infinity UCB1)
        assert(chosen == 1);
        
        // Length incremented
        assert(sim.length() == 1);
    }
    
    // Test 3: Multiple candidates with different average rewards - highest chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        cs.insert({g1, 3});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Pre-populate tree: all visited, index 2 has highest average reward
        root.m_visits = 100;
        
        // idx 0: avg = 40/10 = 4.0
        root.m_children[size_t(0)].m_visits = 10;
        root.m_children[size_t(0)].m_value = 40.0;
        
        // idx 1: avg = 50/10 = 5.0
        root.m_children[size_t(1)].m_visits = 10;
        root.m_children[size_t(1)].m_value = 50.0;
        
        // idx 2: avg = 800/10 = 80.0 (HIGHEST!)
        root.m_children[size_t(2)].m_visits = 10;
        root.m_children[size_t(2)].m_value = 800.0;
        
        // idx 3: avg = 30/10 = 3.0
        root.m_children[size_t(3)].m_visits = 10;
        root.m_children[size_t(3)].m_value = 30.0;
        
        a01_decider decider(gs, cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        // CRITICAL: Should choose index 2 (highest average reward)
        assert(chosen == 2);
        
        // Length incremented
        assert(sim.length() == 1);
    }
    
    // Test 4: Multiple candidates, verify result is always valid
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        cs.insert({g1, 3});
        cs.insert({g1, 4});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(999);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Call multiple times
        for (int i = 0; i < 15; i++) {
            size_t chosen = decider.choose_candidate(g1);
            
            // CRITICAL: Result must be valid candidate index
            assert(chosen >= 0 && chosen <= 4);
            
            // Verify it's actually in candidate store
            bool found = false;
            auto range = cs.equal_range(g1);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == chosen) {
                    found = true;
                    break;
                }
            }
            assert(found == true);
        }
        
        // Simulation length should be 15
        assert(sim.length() == 15);
    }
    
    // Test 5: Candidates with non-contiguous indices
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        // Sparse indices: 5, 17, 42
        cs.insert({g1, 5});
        cs.insert({g1, 17});
        cs.insert({g1, 42});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Pre-populate: index 42 has highest reward
        root.m_visits = 50;
        
        root.m_children[size_t(5)].m_visits = 10;
        root.m_children[size_t(5)].m_value = 20.0;  // avg = 2.0
        
        root.m_children[size_t(17)].m_visits = 10;
        root.m_children[size_t(17)].m_value = 30.0; // avg = 3.0
        
        root.m_children[size_t(42)].m_visits = 10;
        root.m_children[size_t(42)].m_value = 500.0; // avg = 50.0 (HIGHEST!)
        
        a01_decider decider(gs, cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        // CRITICAL: Should choose 42 (highest average)
        assert(chosen == 42);
        assert(sim.length() == 1);
    }
    
    // Test 6: Many candidates - all valid
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        // Add 30 candidates
        for (size_t i = 0; i < 30; i++) {
            cs.insert({g1, i});
        }
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(777);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        // Result must be valid
        assert(chosen < 30);
        
        // Verify in store
        auto range = cs.equal_range(g1);
        bool found = false;
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == chosen) {
                found = true;
                break;
            }
        }
        assert(found == true);
        
        assert(sim.length() == 1);
    }
    
    // Test 7: Verify UCB1 balances exploitation and exploration
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        // Pre-populate: idx 0 higher reward BUT much more visited (less exploration bonus)
        // idx 1 lower reward but less visited (higher exploration bonus)
        root.m_visits = 100;
        
        // idx 0: avg = 600/90 = 6.67, but exploration = sqrt(ln(100)/90) = very small
        root.m_children[size_t(0)].m_visits = 90;
        root.m_children[size_t(0)].m_value = 600.0;
        
        // idx 1: avg = 5/1 = 5.0, but exploration = sqrt(ln(100)/1) = 2.145 (LARGE!)
        // UCB1 = 5.0 + 2.0 * 2.145 = 9.29 vs 6.67 + 2.0 * 0.048 = 6.77
        root.m_children[size_t(1)].m_visits = 1;
        root.m_children[size_t(1)].m_value = 5.0;
        
        a01_decider decider(gs, cs, sim);
        
        size_t chosen = decider.choose_candidate(g1);
        
        // CRITICAL: Should choose idx 1 (exploration bonus outweighs lower average)
        assert(chosen == 1);
    }
}

void test_a01_decider() {
    // Test 1: Single goal, single candidate - both chosen
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        gs.insert({g1, ep.atom("p")});
        cs.insert({g1, 0});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        size_t length_before = sim.length();
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        // CRITICAL: Should choose g1 and index 0
        assert(chosen_goal == g1);
        assert(chosen_candidate == 0);
        
        // CRITICAL: Simulation length increments by 2 (two choose() calls)
        assert(sim.length() == length_before + 2);
        
        // Stores unchanged
        assert(gs.size() == 1);
        assert(cs.count(g1) == 1);
    }
    
    // Test 2: Multiple goals and candidates - verify deterministic selection
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g2, 2});
        cs.insert({g3, 0});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Pre-populate tree to force g2 selection (level 1)
        root.m_visits = 100;
        
        // g1: avg = 20/10 = 2.0
        root.m_children[g1].m_visits = 10;
        root.m_children[g1].m_value = 20.0;
        
        // g2: avg = 800/10 = 80.0 (HIGHEST!)
        root.m_children[g2].m_visits = 10;
        root.m_children[g2].m_value = 800.0;
        
        // g3: avg = 30/10 = 3.0
        root.m_children[g3].m_visits = 10;
        root.m_children[g3].m_value = 30.0;
        
        // Now pre-populate g2's children to force candidate index 1 selection (level 2)
        root.m_children[g2].m_visits = 50;
        
        // idx 0: avg = 10/10 = 1.0
        root.m_children[g2].m_children[size_t(0)].m_visits = 10;
        root.m_children[g2].m_children[size_t(0)].m_value = 10.0;
        
        // idx 1: avg = 900/10 = 90.0 (HIGHEST!)
        root.m_children[g2].m_children[size_t(1)].m_visits = 10;
        root.m_children[g2].m_children[size_t(1)].m_value = 900.0;
        
        // idx 2: avg = 20/10 = 2.0
        root.m_children[g2].m_children[size_t(2)].m_visits = 10;
        root.m_children[g2].m_children[size_t(2)].m_value = 20.0;
        
        a01_decider decider(gs, cs, sim);
        
        size_t length_before = sim.length();
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        // CRITICAL: Should choose g2 (highest goal reward) and index 1 (highest candidate reward)
        assert(chosen_goal == g2);
        assert(chosen_candidate == 1);
        
        // CRITICAL: Two choose() calls made
        assert(sim.length() == length_before + 2);
    }
    
    // Test 3: Verify operator() calls helpers in correct order (goal first, then candidate)
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 10});
        cs.insert({g1, 20});
        cs.insert({g2, 30});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Force g1 selection at level 1 (highest reward)
        root.m_visits = 100;
        
        root.m_children[g1].m_visits = 20;
        root.m_children[g1].m_value = 1800.0;  // avg = 90.0 (HIGHEST!)
        
        root.m_children[g2].m_visits = 20;
        root.m_children[g2].m_value = 60.0;   // avg = 3.0
        
        // Pre-populate g1's children: force index 20 selection
        root.m_children[g1].m_children[size_t(10)].m_visits = 5;
        root.m_children[g1].m_children[size_t(10)].m_value = 10.0;  // avg = 2.0
        
        root.m_children[g1].m_children[size_t(20)].m_visits = 0;  // UNVISITED - infinity UCB1!
        
        a01_decider decider(gs, cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        // CRITICAL: g1 chosen (highest avg), then index 20 chosen (unvisited)
        assert(chosen_goal == g1);
        assert(chosen_candidate == 20);
        
        // Two steps
        assert(sim.length() == 2);
    }
    
    // Test 4: Multiple calls to operator() - all return valid pairs
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g2, 2});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(555);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.5, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Call 10 times
        for (int i = 0; i < 10; i++) {
            auto [chosen_goal, chosen_candidate] = decider();
            
            // CRITICAL: Goal must be in goal store
            assert(gs.count(chosen_goal) == 1);
            assert(chosen_goal == g1 || chosen_goal == g2);
            
            // CRITICAL: Candidate must be valid for chosen goal
            bool found = false;
            auto range = cs.equal_range(chosen_goal);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == chosen_candidate) {
                    found = true;
                    break;
                }
            }
            assert(found == true);
        }
        
        // Simulation length should be 20 (10 calls * 2 steps each)
        assert(sim.length() == 20);
    }
    
    // Test 5: Deterministic test - force specific goal and candidate via tree setup
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g1, 2});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g3, 0});
        cs.insert({g3, 1});
        cs.insert({g3, 2});
        cs.insert({g3, 3});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.0, rng);
        
        // Force selection of g3 (level 1): give it massively higher average reward
        root.m_visits = 200;
        
        root.m_children[g1].m_visits = 20;
        root.m_children[g1].m_value = 40.0;   // avg = 2.0
        
        root.m_children[g2].m_visits = 20;
        root.m_children[g2].m_value = 60.0;   // avg = 3.0
        
        root.m_children[g3].m_visits = 20;
        root.m_children[g3].m_value = 2000.0; // avg = 100.0 (MASSIVELY HIGHER!)
        
        // Force selection of index 2 for g3 (level 2): give it highest reward
        root.m_children[g3].m_visits = 100;
        
        root.m_children[g3].m_children[size_t(0)].m_visits = 10;
        root.m_children[g3].m_children[size_t(0)].m_value = 20.0;  // avg = 2.0
        
        root.m_children[g3].m_children[size_t(1)].m_visits = 10;
        root.m_children[g3].m_children[size_t(1)].m_value = 30.0;  // avg = 3.0
        
        root.m_children[g3].m_children[size_t(2)].m_visits = 10;
        root.m_children[g3].m_children[size_t(2)].m_value = 1000.0; // avg = 100.0 (HIGHEST!)
        
        root.m_children[g3].m_children[size_t(3)].m_visits = 10;
        root.m_children[g3].m_children[size_t(3)].m_value = 40.0;  // avg = 4.0
        
        a01_decider decider(gs, cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        // CRITICAL: Should choose g3 and index 2 (both have highest rewards)
        assert(chosen_goal == g3);
        assert(chosen_candidate == 2);
        
        // Two choose() calls
        assert(sim.length() == 2);
    }
    
    // Test 6: Verify using unvisited nodes (infinity UCB1) for candidate level
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g2, 2});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Force g1 (highest reward at level 1)
        root.m_visits = 100;
        
        root.m_children[g1].m_visits = 30;
        root.m_children[g1].m_value = 2700.0;  // avg = 90.0 (HIGHEST!)
        
        root.m_children[g2].m_visits = 30;
        root.m_children[g2].m_value = 90.0;    // avg = 3.0
        
        // Force index 1 for g1 (unvisited at level 2)
        root.m_children[g1].m_children[size_t(0)].m_visits = 15;
        root.m_children[g1].m_children[size_t(0)].m_value = 50.0;
        root.m_children[g1].m_children[size_t(1)].m_visits = 0;  // UNVISITED - infinity UCB1!
        
        a01_decider decider(gs, cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        // CRITICAL: Should choose g1 (highest avg) and index 1 (unvisited)
        assert(chosen_goal == g1);
        assert(chosen_candidate == 1);
        
        // Two steps
        assert(sim.length() == 2);
    }
    
    // Test 7: Multiple calls - all return valid pairs
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 5});
        cs.insert({g1, 10});
        cs.insert({g1, 15});
        cs.insert({g2, 20});
        cs.insert({g2, 25});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(999);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 2.0, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Call 15 times
        for (int i = 0; i < 15; i++) {
            auto [chosen_goal, chosen_candidate] = decider();
            
            // CRITICAL: Goal must be valid
            assert(gs.count(chosen_goal) == 1);
            
            // CRITICAL: Candidate must be valid for chosen goal
            bool found = false;
            auto range = cs.equal_range(chosen_goal);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == chosen_candidate) {
                    found = true;
                    break;
                }
            }
            assert(found == true);
        }
        
        // CRITICAL: 15 calls * 2 steps = 30 total
        assert(sim.length() == 30);
    }
    
    // Test 8: Complex tree with varied rewards - verify correct selection
    {
        trail t;
        t.push();
        expr_pool ep(t);
        lineage_pool lp;
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        const goal_lineage* g3 = lp.goal(nullptr, 3);
        const goal_lineage* g4 = lp.goal(nullptr, 4);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        gs.insert({g3, ep.atom("r")});
        gs.insert({g4, ep.atom("s")});
        
        cs.insert({g1, 0});
        cs.insert({g2, 0});
        cs.insert({g2, 1});
        cs.insert({g3, 0});
        cs.insert({g3, 1});
        cs.insert({g3, 2});
        cs.insert({g4, 0});
        cs.insert({g4, 1});
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(12345);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.5, rng);
        
        // Setup level 1: force g3 selection
        root.m_visits = 150;
        
        root.m_children[g1].m_visits = 15;
        root.m_children[g1].m_value = 30.0;   // avg = 2.0
        
        root.m_children[g2].m_visits = 15;
        root.m_children[g2].m_value = 45.0;   // avg = 3.0
        
        root.m_children[g3].m_visits = 15;
        root.m_children[g3].m_value = 1500.0; // avg = 100.0 (HIGHEST!)
        
        root.m_children[g4].m_visits = 15;
        root.m_children[g4].m_value = 60.0;   // avg = 4.0
        
        // Setup level 2: force index 1 for g3
        root.m_children[g3].m_visits = 60;
        
        root.m_children[g3].m_children[size_t(0)].m_visits = 10;
        root.m_children[g3].m_children[size_t(0)].m_value = 30.0;  // avg = 3.0
        
        root.m_children[g3].m_children[size_t(1)].m_visits = 10;
        root.m_children[g3].m_children[size_t(1)].m_value = 800.0; // avg = 80.0 (HIGHEST!)
        
        root.m_children[g3].m_children[size_t(2)].m_visits = 10;
        root.m_children[g3].m_children[size_t(2)].m_value = 50.0;  // avg = 5.0
        
        a01_decider decider(gs, cs, sim);
        
        auto [chosen_goal, chosen_candidate] = decider();
        
        // CRITICAL: Should choose g3 and index 1
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
        a01_goal_store gs;
        a01_candidate_store cs;
        
        const goal_lineage* g1 = lp.goal(nullptr, 1);
        const goal_lineage* g2 = lp.goal(nullptr, 2);
        
        gs.insert({g1, ep.atom("p")});
        gs.insert({g2, ep.atom("q")});
        
        cs.insert({g1, 0});
        cs.insert({g1, 1});
        cs.insert({g2, 0});
        
        size_t gs_size_before = gs.size();
        size_t cs_size_before = cs.size();
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_decider decider(gs, cs, sim);
        
        // Call 10 times
        for (int i = 0; i < 10; i++) {
            decider();
        }
        
        // CRITICAL: Stores completely unchanged
        assert(gs.size() == gs_size_before);
        assert(cs.size() == cs_size_before);
        assert(gs.count(g1) == 1);
        assert(gs.count(g2) == 1);
        assert(cs.count(g1) == 2);
        assert(cs.count(g2) == 1);
        
        // Simulation length = 20 (10 calls * 2)
        assert(sim.length() == 20);
    }
}

void test_a01_sim_constructor() {
    // Test 1: Empty goals - verify initialization
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        a01_goals goals;  // Empty
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        {
            a01_resolution_store rs;
            a01_decision_store ds;
            a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
            
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
            
            // Verify avoidance store is a COPY
            assert(&simulation.as_copy != &as);
            assert(simulation.as_copy.size() == 0);
            
            // CRITICAL: Verify public a01_sim member references
            assert(&simulation.db == &db);
            assert(&simulation.t == &t);
            assert(&simulation.lp == &lp);
            assert(&simulation.as_copy != &as);
            
            // CRITICAL: Verify copier references (public in DEBUG)
            assert(&simulation.cp.sequencer_ref == &seq);
            assert(&simulation.cp.expr_pool_ref == &ep);
            
            // CRITICAL: Verify decider references (public in DEBUG)
            assert(&simulation.dec.gs == &simulation.gs);
            assert(&simulation.dec.cs == &simulation.cs);
            assert(&simulation.dec.sim == &sim);
            
            // CRITICAL: Verify goal_adder references (public in DEBUG)
            assert(&simulation.ga.goals == &simulation.gs);
            assert(&simulation.ga.candidates == &simulation.cs);
            assert(&simulation.ga.database == &db);
            
            // CRITICAL: Verify goal_resolver references (public in DEBUG)
            assert(&simulation.gr.rs == &simulation.rs);
            assert(&simulation.gr.gs == &simulation.gs);
            assert(&simulation.gr.cs == &simulation.cs);
            assert(&simulation.gr.db == &db);
            assert(&simulation.gr.bm == &bm);
            assert(&simulation.gr.lp == &lp);
            assert(&simulation.rs == &rs);
            assert(&simulation.ds == &ds);
            assert(&simulation.gr.ga == &simulation.ga);
            assert(&simulation.gr.as == &simulation.as_copy);
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
        
        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);

        {
            a01_resolution_store rs;
            a01_decision_store ds;

            a01_sim simulation(50, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
            
            // CRITICAL: Goal added to goal_store with index 0
            assert(simulation.gs.size() == 1);
            const goal_lineage* gl = lp.goal(nullptr, 0);
            assert(gl->parent == nullptr);
            assert(gl->idx == 0);
            assert(simulation.gs.at(gl) == ep.atom("p"));
            
            // CRITICAL: Candidate added to candidate_store (1 goal * 1 db rule = 1 candidate)
            assert(simulation.cs.size() == 1);
            assert(simulation.cs.count(gl) == 1);
            assert(simulation.cs.begin()->first == gl);
            assert(simulation.cs.begin()->second == 0);  // db[0] as candidate
            
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
        
        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});
        db.push_back(rule{ep.atom("q"), {}});
        db.push_back(rule{ep.atom("r"), {}});
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        goals.push_back(ep.atom("q"));
        goals.push_back(ep.atom("r"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;

        a01_sim simulation(200, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // CRITICAL: Three goals added with indices 0, 1, 2
        assert(simulation.gs.size() == 3);
        
        // Find each lineage by index
        const goal_lineage* gl0 = nullptr;
        const goal_lineage* gl1 = nullptr;
        const goal_lineage* gl2 = nullptr;
        
        for (const auto& [gl, ge] : simulation.gs) {
            if (gl->idx == 0) {
                gl0 = gl;
                assert(ge == ep.atom("p"));
            } else if (gl->idx == 1) {
                gl1 = gl;
                assert(ge == ep.atom("q"));
            } else if (gl->idx == 2) {
                gl2 = gl;
                assert(ge == ep.atom("r"));
            }
        }
        
        assert(gl0 != nullptr);
        assert(gl1 != nullptr);
        assert(gl2 != nullptr);
        
        // CRITICAL: All have nullptr parent (root goals)
        assert(gl0->parent == nullptr);
        assert(gl1->parent == nullptr);
        assert(gl2->parent == nullptr);
        
        // CRITICAL: Total candidates = 3 goals * 3 db rules = 9
        assert(simulation.cs.size() == 9);
        
        // CRITICAL: Each goal has ALL 3 rules as candidates (goal_adder adds all, trivially)
        assert(simulation.cs.count(gl0) == 3);
        assert(simulation.cs.count(gl1) == 3);
        assert(simulation.cs.count(gl2) == 3);
        
        // Verify each goal has candidates 0, 1, 2
        auto range0 = simulation.cs.equal_range(gl0);
        std::set<size_t> indices0;
        for (auto it = range0.first; it != range0.second; ++it) {
            indices0.insert(it->second);
        }
        assert(indices0 == std::set<size_t>({0, 1, 2}));
        
        auto range1 = simulation.cs.equal_range(gl1);
        std::set<size_t> indices1;
        for (auto it = range1.first; it != range1.second; ++it) {
            indices1.insert(it->second);
        }
        assert(indices1 == std::set<size_t>({0, 1, 2}));
        
        auto range2 = simulation.cs.equal_range(gl2);
        std::set<size_t> indices2;
        for (auto it = range2.first; it != range2.second; ++it) {
            indices2.insert(it->second);
        }
        assert(indices2 == std::set<size_t>({0, 1, 2}));
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});
        // No rule for "q" or "r"
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        goals.push_back(ep.atom("q"));
        goals.push_back(ep.atom("r"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        // Each goal has the single db rule as a candidate
        assert(simulation.cs.count(gl_p) == 1);
        assert(simulation.cs.count(gl_q) == 1);
        assert(simulation.cs.count(gl_r) == 1);
        
        // All have db[0] as candidate
        assert(simulation.cs.find(gl_p)->second == 0);
        assert(simulation.cs.find(gl_q)->second == 0);
        assert(simulation.cs.find(gl_r)->second == 0);
        
        // CRITICAL: Verify resolution and decision stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify goal expressions are correct
        assert(simulation.gs.at(gl_p) == ep.atom("p"));
        assert(simulation.gs.at(gl_q) == ep.atom("q"));
        assert(simulation.gs.at(gl_r) == ep.atom("r"));
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});
        db.push_back(rule{ep.atom("p"), {ep.atom("q")}});
        db.push_back(rule{ep.atom("p"), {ep.atom("r")}});
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // Goal added
        assert(simulation.gs.size() == 1);
        const goal_lineage* gl = lp.goal(nullptr, 0);
        
        // CRITICAL: Three candidates for the same goal
        assert(simulation.cs.count(gl) == 3);
        
        // Verify candidate indices are 0, 1, 2
        auto range = simulation.cs.equal_range(gl);
        std::set<size_t> indices;
        for (auto it = range.first; it != range.second; ++it) {
            indices.insert(it->second);
        }
        assert(indices.size() == 3);
        assert(indices.count(0) == 1);
        assert(indices.count(1) == 1);
        assert(indices.count(2) == 1);
        
        // CRITICAL: Verify resolution/decision stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Verify goal expression content
        assert(simulation.gs.at(gl) == ep.atom("p"));
        
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
        
        a01_database db;
        a01_goals goals;
        
        // Pre-populate avoidance store
        a01_avoidance_store as;
        const resolution_lineage* rl1 = lp.resolution(lp.goal(nullptr, 0), 0);
        const resolution_lineage* rl2 = lp.resolution(lp.goal(nullptr, 1), 1);
        
        a01_decision_store avoid1;
        avoid1.insert(rl1);
        avoid1.insert(rl2);
        
        as.insert(avoid1);
        
        size_t as_size_before = as.size();
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // CRITICAL: as_copy is a COPY, not a reference
        assert(&simulation.as_copy != &as);
        assert(simulation.as_copy.size() == 1);
        
        // Original unchanged
        assert(as.size() == as_size_before);
        
        // Content matches
        assert(simulation.as_copy.count(avoid1) == 1);
        assert(as.count(avoid1) == 1);
    }
    
    // Test 7: Complex database and goals - verify all candidates found
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});
        db.push_back(rule{ep.atom("p"), {ep.atom("x")}});
        db.push_back(rule{ep.atom("q"), {}});
        db.push_back(rule{ep.atom("q"), {ep.atom("y")}});
        db.push_back(rule{ep.atom("q"), {ep.atom("z")}});
        db.push_back(rule{ep.atom("r"), {ep.atom("w")}});
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        goals.push_back(ep.atom("q"));
        goals.push_back(ep.atom("r"));
        goals.push_back(ep.atom("s"));  // No matching rule
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(75, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
                assert(ge == ep.atom("p"));
            } else if (gl->idx == 1) {
                gl1 = gl;
                assert(ge == ep.atom("q"));
            } else if (gl->idx == 2) {
                gl2 = gl;
                assert(ge == ep.atom("r"));
            } else if (gl->idx == 3) {
                gl3 = gl;
                assert(ge == ep.atom("s"));
            }
        }
        
        assert(gl0 && gl1 && gl2 && gl3);
        
        // CRITICAL: goal_adder adds ALL rules to ALL goals (trivially)
        // 4 goals * 6 db rules = 24 total candidates
        assert(simulation.cs.size() == 24);
        
        // Each goal has all 6 rules as candidates
        assert(simulation.cs.count(gl0) == 6);
        assert(simulation.cs.count(gl1) == 6);
        assert(simulation.cs.count(gl2) == 6);
        assert(simulation.cs.count(gl3) == 6);
        
        // Verify all have indices 0-5
        for (const goal_lineage* gl : {gl0, gl1, gl2, gl3}) {
            auto range = simulation.cs.equal_range(gl);
            std::set<size_t> indices;
            for (auto it = range.first; it != range.second; ++it) {
                indices.insert(it->second);
            }
            assert(indices == std::set<size_t>({0, 1, 2, 3, 4, 5}));
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
        assert(simulation.gs.at(gl0) == ep.atom("p"));
        assert(simulation.gs.at(gl1) == ep.atom("q"));
        assert(simulation.gs.at(gl2) == ep.atom("r"));
        assert(simulation.gs.at(gl3) == ep.atom("s"));
        
        // CRITICAL: Verify database reference holds correct content
        assert(simulation.db.size() == 6);
        assert(simulation.db[0].head == ep.atom("p"));
        assert(simulation.db[2].head == ep.atom("q"));
        assert(simulation.db[5].head == ep.atom("r"));
    }
    
    // Test 9: Pre-populated avoidance store is copied
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        a01_goals goals;
        
        // Pre-populate avoidance store with multiple avoidances
        a01_avoidance_store as;
        
        const resolution_lineage* rl1 = lp.resolution(lp.goal(nullptr, 0), 0);
        const resolution_lineage* rl2 = lp.resolution(lp.goal(nullptr, 1), 1);
        const resolution_lineage* rl3 = lp.resolution(lp.goal(nullptr, 2), 2);
        
        a01_decision_store avoid1;
        avoid1.insert(rl1);
        avoid1.insert(rl2);
        
        a01_decision_store avoid2;
        avoid2.insert(rl3);
        
        as.insert(avoid1);
        as.insert(avoid2);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        size_t as_size_before = as.size();
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // CRITICAL: as_copy has same content
        assert(simulation.as_copy.size() == 2);
        assert(simulation.as_copy.count(avoid1) == 1);
        assert(simulation.as_copy.count(avoid2) == 1);
        
        // CRITICAL: Original unchanged
        assert(as.size() == as_size_before);
    }
    
    // Test 10: Verify sequencer and expr_pool passed correctly to copier
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        a01_goals goals;
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        // Test various limits
        {
            a01_resolution_store rs;
            a01_decision_store ds;
            a01_sim sim1(1, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
            assert(sim1.max_resolutions == 1);
        }
        
        {
            a01_resolution_store rs;
            a01_decision_store ds;
            a01_sim sim2(1000, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
            assert(sim2.max_resolutions == 1000);
        }
        
        {
            a01_resolution_store rs;
            a01_decision_store ds;
            a01_sim sim3(999999, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
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
        
        a01_database db;
        a01_goals goals;
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // CRITICAL: decisions() returns reference to ds
        assert(&simulation.ds == &ds);
        assert(&simulation.rs == &rs);
    }
    
    // Test 14: Verify component initialization with non-empty avoidance store
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        a01_goals goals;
        
        // Pre-populate avoidance store
        a01_avoidance_store as;
        const resolution_lineage* rl1 = lp.resolution(lp.goal(nullptr, 0), 0);
        const resolution_lineage* rl2 = lp.resolution(lp.goal(nullptr, 1), 1);
        
        a01_decision_store avoid;
        avoid.insert(rl1);
        avoid.insert(rl2);
        as.insert(avoid);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // CRITICAL: Verify as_copy has content, original unchanged
        assert(simulation.as_copy.size() == 1);
        assert(simulation.as_copy.count(avoid) == 1);
        assert(as.size() == 1);
        
        // CRITICAL: Verify gr resolver references the copy (public in DEBUG)
        assert(&simulation.gr.as == &simulation.as_copy);
        
        assert(&simulation.ds == &ds);
        assert(&simulation.rs == &rs);
    }
    
    // Test 15: Verify all store sizes after construction with various goal counts
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        db.push_back(rule{ep.atom("x"), {}});
        db.push_back(rule{ep.atom("y"), {}});
        
        // Add 5 goals
        a01_goals goals;
        for (int i = 0; i < 5; i++) {
            goals.push_back(ep.atom("goal" + std::to_string(i)));
        }
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(500, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // CRITICAL: 5 goals added
        assert(simulation.gs.size() == 5);
        
        // CRITICAL: 5 goals * 2 db rules = 10 candidates
        assert(simulation.cs.size() == 10);
        
        // CRITICAL: All other stores empty
        assert(simulation.rs.size() == 0);
        assert(simulation.ds.size() == 0);
        assert(simulation.as_copy.size() == 0);
        assert(simulation.ds.size() == 0);
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Verify each goal has 2 candidates
        for (const auto& [gl, ge] : simulation.gs) {
            assert(simulation.cs.count(gl) == 2);
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
        
        a01_database db;
        
        a01_goals goals;
        const expr* goal_a = ep.atom("alpha");
        const expr* goal_b = ep.atom("beta");
        const expr* goal_c = ep.atom("gamma");
        const expr* goal_d = ep.atom("delta");
        
        goals.push_back(goal_a);
        goals.push_back(goal_b);
        goals.push_back(goal_c);
        goals.push_back(goal_d);
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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

void test_a01_sim() {
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // Fact: a.
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));  // Goal: :- a.
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;  // Empty database
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Should return false (conflict)
        assert(result == false);
        
        // CRITICAL: Goal store NOT empty (unresolved goal remains)
        assert(simulation.gs.size() == 1);
        
        // CRITICAL: Candidate store empty (no candidates available)
        assert(simulation.cs.size() == 0);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // idx 0
        db.push_back(rule{ep.atom("b"), {}});  // idx 1
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {}});  // idx 2
        db.push_back(rule{ep.atom("c"), {}});  // idx 3
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        // Pre-populate avoidance: avoid (gl0, idx 0)
        const goal_lineage* gl0_avoid = lp.goal(nullptr, 0);
        const resolution_lineage* rl0_avoid = lp.resolution(gl0_avoid, 0);
        
        a01_decision_store avoid;
        avoid.insert(rl0_avoid);
        
        a01_avoidance_store as;
        as.insert(avoid);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        // CRITICAL: Original avoidance store unchanged
        assert(as.size() == 1);
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("b"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("c"), {}});  // idx 2
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {}});  // idx 2
        db.push_back(rule{ep.atom("c"), {}});  // idx 3
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("b"), {}});  // idx 1
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {ep.atom("d")}});  // idx 2
        db.push_back(rule{ep.atom("c"), {ep.atom("e")}});  // idx 3
        // No rules for d or e
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(simulation.cs.count(gl_d) == 0);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});
        db.push_back(rule{ep.atom("b"), {ep.atom("c")}});
        db.push_back(rule{ep.atom("c"), {ep.atom("d")}});
        db.push_back(rule{ep.atom("d"), {ep.atom("e")}});
        db.push_back(rule{ep.atom("e"), {ep.atom("f")}});
        db.push_back(rule{ep.atom("f"), {ep.atom("g")}});
        db.push_back(rule{ep.atom("g"), {}});
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(3, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);  // Max 3 resolutions!
        
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
            if (simulation.cs.count(gl) > 0) {
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // idx 0
        db.push_back(rule{ep.atom("b"), {}});  // idx 1
        db.push_back(rule{ep.atom("c"), {}});  // idx 2
        db.push_back(rule{ep.atom("d"), {}});  // idx 3
        db.push_back(rule{ep.atom("e"), {ep.atom("f")}});  // idx 4
        
        a01_goals goals;
        goals.push_back(ep.atom("e"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // Before execution, verify 5 candidates added
        const goal_lineage* gl0_for_check = lp.goal(nullptr, 0);
        assert(simulation.cs.count(gl0_for_check) == 5);
        
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
        assert(simulation.cs.count(gl_f) == 0);  // No candidates for f
        
        // CRITICAL: Candidate store empty (head elim removed 4, resolution removed 1)
        assert(simulation.cs.size() == 0);
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("b"), {}});  // idx 1
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        // Pre-populate avoidance with the future resolution plus a dummy
        const goal_lineage* gl0_pre = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a0_pre = lp.resolution(gl0_pre, 0);
        
        // Add dummy resolution to keep avoidance non-singleton
        const goal_lineage* gl_dummy = lp.goal(nullptr, 99);
        const resolution_lineage* rl_dummy = lp.resolution(gl_dummy, 99);
        
        a01_decision_store avoid;
        avoid.insert(rl_a0_pre);
        avoid.insert(rl_dummy);
        
        a01_avoidance_store as;
        as.insert(avoid);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // Verify avoidance copied
        assert(simulation.as_copy.size() == 1);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 2 resolutions (a, b)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: All unit propagations (no decisions)
        assert(simulation.ds.size() == 0);
        
        // CRITICAL: Avoidance store modified (rl(gl0,0) erased after resolution)
        assert(simulation.as_copy.size() == 1);
        const a01_decision_store& remaining = *simulation.as_copy.begin();
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {ep.atom("d")}});  // idx 2
        db.push_back(rule{ep.atom("c"), {ep.atom("e")}});  // idx 3
        db.push_back(rule{ep.atom("d"), {}});  // idx 4
        db.push_back(rule{ep.atom("e"), {}});  // idx 5
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        a01_goals goals;  // Empty - already solved!
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b"), ep.atom("c")}});  // idx 0
        db.push_back(rule{ep.atom("b"), {}});  // idx 1
        db.push_back(rule{ep.atom("c"), {}});  // idx 2
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        // p(X) :- q(X).
        const expr* X = ep.var(seq());
        db.push_back(rule{
            ep.cons(ep.atom("p"), X),
            {ep.cons(ep.atom("q"), X)}
        });
        // q(a).
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));  // :- p(a).
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{ep.cons(ep.atom("p"), X), {ep.cons(ep.atom("q"), X)}});
        db.push_back(rule{ep.cons(ep.atom("q"), Y), {ep.cons(ep.atom("r"), Y)}});
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("a")), {}});
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{
            ep.cons(ep.atom("p"), ep.cons(X, Y)),
            {ep.cons(ep.atom("q"), X), ep.cons(ep.atom("r"), Y)}
        });
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("b")), {}});
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.cons(ep.atom("a"), ep.atom("b"))));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* X1 = ep.var(seq());
        const expr* X2 = ep.var(seq());
        
        db.push_back(rule{ep.cons(ep.atom("p"), X1), {ep.cons(ep.atom("q"), X1)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("p"), X2), {ep.cons(ep.atom("r"), X2)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("b")), {}});  // idx 3
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        
        // Add 19 non-matching rules
        for (int i = 0; i < 19; i++) {
            db.push_back(rule{ep.atom("x" + std::to_string(i)), {}});
        }
        // Add 1 matching rule
        db.push_back(rule{ep.atom("target"), {}});  // idx 19
        
        a01_goals goals;
        goals.push_back(ep.atom("target"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // Initial state: 20 candidates
        const goal_lineage* gl0_for_check = lp.goal(nullptr, 0);
        assert(simulation.cs.count(gl0_for_check) == 20);
        
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
        
        a01_database db;
        a01_goals goals;
        
        // Create 10 independent facts and goals
        for (int i = 0; i < 10; i++) {
            db.push_back(rule{ep.atom("g" + std::to_string(i)), {}});
            goals.push_back(ep.atom("g" + std::to_string(i)));
        }
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
    
    // Test 21: Complex with decisions, unit props, and avoidance erasure
    // Database: a :- b., a :- c., b :- d., c., d.
    // Goal: :- a.
    // Avoidance: {{rl(gl0,0), rl(gl_b,2)}}
    // Force decision on idx 1 (a→c), verify avoidance erasure
    {
        trail t;
        t.push();
        
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);
        lineage_pool lp;
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {ep.atom("d")}});  // idx 2
        db.push_back(rule{ep.atom("c"), {}});  // idx 3
        db.push_back(rule{ep.atom("d"), {}});  // idx 4
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        // Pre-populate avoidance
        const goal_lineage* gl0_pre = lp.goal(nullptr, 0);
        const resolution_lineage* rl_a0_pre = lp.resolution(gl0_pre, 0);
        const goal_lineage* gl_b_pre = lp.goal(rl_a0_pre, 0);
        const resolution_lineage* rl_b_pre = lp.resolution(gl_b_pre, 2);
        
        a01_decision_store avoid;
        avoid.insert(rl_a0_pre);
        avoid.insert(rl_b_pre);
        
        a01_avoidance_store as;
        as.insert(avoid);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        // CRITICAL: Avoidance modified (rl_a erased after resolution)
        assert(simulation.as_copy.size() == 1);
        const a01_decision_store& remaining = *simulation.as_copy.begin();
        // After making rl(gl0,1), it should be erased from avoidance
        // But our avoidance had rl(gl0,0) and rl(gl_b,2), not rl(gl0,1)
        // So avoidance should be unchanged
        assert(remaining.size() == 2);
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {ep.cons(ep.atom("q"), ep.atom("b"))}});
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(simulation.cs.count(gl_q) == 0);
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
        
        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {ep.atom("d")}});  // idx 2
        db.push_back(rule{ep.atom("b"), {ep.atom("e")}});  // idx 3
        db.push_back(rule{ep.atom("c"), {}});  // idx 4
        db.push_back(rule{ep.atom("d"), {}});  // idx 5
        db.push_back(rule{ep.atom("e"), {}});  // idx 6
        
        a01_goals goals;
        goals.push_back(ep.atom("a"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        
        // append(nil, X, X).
        const expr* X1 = ep.var(seq());
        db.push_back(rule{
            ep.cons(ep.atom("append"), ep.cons(ep.atom("nil"), ep.cons(X1, X1))),
            {}
        });
        
        // append(cons(H,T), X, cons(H,R)) :- append(T, X, R).
        const expr* H = ep.var(seq());
        const expr* T = ep.var(seq());
        const expr* X2 = ep.var(seq());
        const expr* R = ep.var(seq());
        
        db.push_back(rule{
            ep.cons(ep.atom("append"), ep.cons(
                ep.cons(ep.atom("cons"), ep.cons(H, T)),
                ep.cons(X2, ep.cons(ep.atom("cons"), ep.cons(H, R)))
            )),
            {ep.cons(ep.atom("append"), ep.cons(T, ep.cons(X2, R)))}
        });
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("append"), ep.cons(
            ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), ep.atom("nil"))),
            ep.cons(ep.cons(ep.atom("cons"), ep.cons(ep.atom("b"), ep.atom("nil"))),
                    ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), ep.cons(ep.atom("cons"), ep.cons(ep.atom("b"), ep.atom("nil"))))))
        )));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        
        // Create a complex dependency graph
        // 30 rules total
        for (int i = 0; i < 10; i++) {
            db.push_back(rule{ep.atom("noise" + std::to_string(i)), {}});
        }
        
        // Main rules: p :- q, r., q., r.
        db.push_back(rule{ep.atom("p"), {ep.atom("q"), ep.atom("r")}});
        db.push_back(rule{ep.atom("q"), {}});
        db.push_back(rule{ep.atom("r"), {}});
        
        // More noise
        for (int i = 10; i < 27; i++) {
            db.push_back(rule{ep.atom("noise" + std::to_string(i)), {}});
        }
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        // Verify 30 candidates initially
        const goal_lineage* gl0 = lp.goal(nullptr, 0);
        assert(simulation.cs.count(gl0) == 30);
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("b")), {}});  // idx 1
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(X_normalized == ep.atom("a"));
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
        
        a01_database db;
        const expr* cons_a_nil = ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), ep.atom("nil")));
        db.push_back(rule{ep.cons(ep.atom("q"), cons_a_nil), {}});  // idx 0
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("q"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* pair_a_b = ep.cons(ep.atom("pair"), ep.cons(ep.atom("a"), ep.atom("b")));
        db.push_back(rule{pair_a_b, {}});  // idx 0
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* pair_X_Y = ep.cons(ep.atom("pair"), ep.cons(X, Y));
        goals.push_back(pair_X_Y);
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Verify X and Y bindings
        normalizer norm(ep, bm);
        const expr* X_normalized = norm(X);
        const expr* Y_normalized = norm(Y);
        assert(X_normalized == ep.atom("a"));
        assert(Y_normalized == ep.atom("b"));
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
        
        a01_database db;
        const expr* X_rule = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X_rule), {ep.cons(ep.atom("q"), X_rule)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("hello")), {}});  // idx 1
        
        a01_goals goals;
        const expr* Y_goal = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), Y_goal));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Verify Y binds to hello through chain
        normalizer norm(ep, bm);
        const expr* Y_normalized = norm(Y_goal);
        assert(Y_normalized == ep.atom("hello"));
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("b")), {}});  // idx 1
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), X));
        goals.push_back(ep.cons(ep.atom("q"), Y));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(X_normalized == ep.atom("a"));
        assert(Y_normalized == ep.atom("b"));
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("a")), {}});  // idx 2
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), X));
        goals.push_back(ep.cons(ep.atom("q"), X));
        goals.push_back(ep.cons(ep.atom("r"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(X_normalized == ep.atom("a"));
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
        
        a01_database db;
        const expr* f_a_b = ep.cons(ep.atom("f"), ep.cons(ep.atom("a"), ep.atom("b")));
        const expr* g_b_c = ep.cons(ep.atom("g"), ep.cons(ep.atom("b"), ep.atom("c")));
        db.push_back(rule{f_a_b, {}});  // idx 0
        db.push_back(rule{g_b_c, {}});  // idx 1
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("f"), ep.cons(X, Y)));
        goals.push_back(ep.cons(ep.atom("g"), ep.cons(Y, Z)));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(X_normalized == ep.atom("a"));
        assert(Y_normalized == ep.atom("b"));
        assert(Z_normalized == ep.atom("c"));
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
        
        a01_database db;
        const expr* leaf1 = ep.cons(ep.atom("leaf"), ep.atom("1"));
        const expr* leaf2 = ep.cons(ep.atom("leaf"), ep.atom("2"));
        const expr* node = ep.cons(ep.atom("node"), ep.cons(leaf1, leaf2));
        const expr* tree = ep.cons(ep.atom("tree"), node);
        db.push_back(rule{tree, {}});  // idx 0
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("tree"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X), {ep.cons(ep.atom("q"), X)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), Y), {ep.cons(ep.atom("r"), Y)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("hello")), {}});  // idx 2
        
        a01_goals goals;
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), A));
        goals.push_back(ep.cons(ep.atom("p"), B));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(A_normalized == ep.atom("hello"));
        assert(B_normalized == ep.atom("hello"));
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("1")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("2")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("c"), ep.atom("3")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("d"), ep.atom("4")), {}});  // idx 3
        db.push_back(rule{ep.cons(ep.atom("e"), ep.atom("5")), {}});  // idx 4
        
        a01_goals goals;
        const expr* V1 = ep.var(seq());
        const expr* V2 = ep.var(seq());
        const expr* V3 = ep.var(seq());
        const expr* V4 = ep.var(seq());
        const expr* V5 = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("a"), V1));
        goals.push_back(ep.cons(ep.atom("b"), V2));
        goals.push_back(ep.cons(ep.atom("c"), V3));
        goals.push_back(ep.cons(ep.atom("d"), V4));
        goals.push_back(ep.cons(ep.atom("e"), V5));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(norm(V1) == ep.atom("1"));
        assert(norm(V2) == ep.atom("2"));
        assert(norm(V3) == ep.atom("3"));
        assert(norm(V4) == ep.atom("4"));
        assert(norm(V5) == ep.atom("5"));
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
        
        a01_database db;
        const expr* X1 = ep.var(seq());
        const expr* X2 = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X1), {ep.cons(ep.atom("q"), X1)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("p"), X2), {ep.cons(ep.atom("r"), X2)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("apple")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("banana")), {}});  // idx 3
        
        a01_goals goals;
        const expr* Fruit = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), Fruit));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(Fruit_normalized == ep.atom("apple"));
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
        
        a01_database db;
        const expr* link_a_b = ep.cons(ep.atom("link"), ep.cons(ep.atom("a"), ep.atom("b")));
        const expr* link_b_c = ep.cons(ep.atom("link"), ep.cons(ep.atom("b"), ep.atom("c")));
        db.push_back(rule{link_a_b, {}});  // idx 0
        db.push_back(rule{link_b_c, {}});  // idx 1
        
        const expr* X1 = ep.var(seq());
        const expr* Y1 = ep.var(seq());
        const expr* path_base = ep.cons(ep.atom("path"), ep.cons(X1, Y1));
        const expr* link_X1_Y1 = ep.cons(ep.atom("link"), ep.cons(X1, Y1));
        db.push_back(rule{path_base, {link_X1_Y1}});  // idx 2
        
        const expr* X2 = ep.var(seq());
        const expr* Y2 = ep.var(seq());
        const expr* Z2 = ep.var(seq());
        const expr* path_rec = ep.cons(ep.atom("path"), ep.cons(X2, Z2));
        const expr* link_X2_Y2 = ep.cons(ep.atom("link"), ep.cons(X2, Y2));
        const expr* path_Y2_Z2 = ep.cons(ep.atom("path"), ep.cons(Y2, Z2));
        db.push_back(rule{path_rec, {link_X2_Y2, path_Y2_Z2}});  // idx 3
        
        a01_goals goals;
        const expr* Dest = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("path"), ep.cons(ep.atom("a"), Dest)));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(Dest_normalized == ep.atom("b"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        const expr* add_rule = ep.cons(ep.atom("add"), ep.cons(X, ep.cons(Y, Z)));
        const expr* plus_body = ep.cons(ep.atom("plus"), ep.cons(X, ep.cons(Y, Z)));
        db.push_back(rule{add_rule, {plus_body}});  // idx 0
        
        const expr* plus_fact = ep.cons(ep.atom("plus"), ep.cons(ep.atom("1"), ep.cons(ep.atom("2"), ep.atom("3"))));
        db.push_back(rule{plus_fact, {}});  // idx 1
        
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* mul_rule = ep.cons(ep.atom("mul"), ep.cons(A, ep.cons(B, C)));
        const expr* times_body = ep.cons(ep.atom("times"), ep.cons(A, ep.cons(B, C)));
        db.push_back(rule{mul_rule, {times_body}});  // idx 2
        
        const expr* times_fact = ep.cons(ep.atom("times"), ep.cons(ep.atom("2"), ep.cons(ep.atom("3"), ep.atom("6"))));
        db.push_back(rule{times_fact, {}});  // idx 3
        
        a01_goals goals;
        const expr* Sum = ep.var(seq());
        const expr* Prod = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("add"), ep.cons(ep.atom("1"), ep.cons(ep.atom("2"), Sum))));
        goals.push_back(ep.cons(ep.atom("mul"), ep.cons(ep.atom("2"), ep.cons(ep.atom("3"), Prod))));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(Sum_normalized == ep.atom("3"));
        assert(Prod_normalized == ep.atom("6"));
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("b")), {}});  // idx 1
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), X));
        goals.push_back(ep.cons(ep.atom("q"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Conflict (unification failure)
        assert(result == false);
        
        // CRITICAL: Exactly 1 resolution (one goal resolves, other becomes unsatisfiable)
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: One goal still in goal store with no candidates
        assert(simulation.gs.size() == 1);
        
        // CRITICAL: The remaining goal has no candidates
        const goal_lineage* remaining_goal = simulation.gs.begin()->first;
        assert(simulation.cs.count(remaining_goal) == 0);
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("pair"), ep.cons(ep.atom("a"), ep.atom("a"))), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("pair"), ep.cons(ep.atom("a"), ep.atom("b"))), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("pair"), ep.cons(ep.atom("b"), ep.atom("b"))), {}});  // idx 2
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("pair"), ep.cons(X, X)));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(X_normalized == ep.atom("b"));
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 0
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict (no candidates)
        assert(result == false);
        
        // CRITICAL: No resolutions made
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal still in store
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.gs.count(gl_p) == 1);
        assert(simulation.cs.count(gl_p) == 0);  // No candidates
        
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
        
        a01_database db;
        const expr* X_rule = ep.var(seq());
        const expr* Y_rule = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X_rule), {ep.cons(ep.atom("q"), X_rule)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), Y_rule), {ep.cons(ep.atom("r"), Y_rule)}});  // idx 1
        // No r facts
        
        a01_goals goals;
        const expr* Z_goal = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), Z_goal));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(simulation.gs.count(gl_r) == 1);
        assert(simulation.cs.count(gl_r) == 0);
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("b")), {}});  // idx 1
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));  // Instantiated
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("q"), X));  // Variable
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(X_normalized == ep.atom("b"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        const expr* W = ep.var(seq());
        const expr* V = ep.var(seq());
        
        db.push_back(rule{ep.cons(ep.atom("a"), X), {ep.cons(ep.atom("b"), X)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("b"), Y), {ep.cons(ep.atom("c"), Y)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("c"), Z), {ep.cons(ep.atom("d"), Z)}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("d"), W), {ep.cons(ep.atom("e"), W)}});  // idx 3
        db.push_back(rule{ep.cons(ep.atom("e"), V), {ep.cons(ep.atom("f"), V)}});  // idx 4
        db.push_back(rule{ep.cons(ep.atom("f"), ep.atom("hello")), {}});  // idx 5
        
        a01_goals goals;
        const expr* Result = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("a"), Result));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(Result_normalized == ep.atom("hello"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        
        const expr* foo_head = ep.cons(ep.atom("foo"), ep.cons(X, ep.cons(Y, Z)));
        const expr* bar_body = ep.cons(ep.atom("bar"), ep.cons(X, Y));
        const expr* baz_body = ep.cons(ep.atom("baz"), ep.cons(Y, Z));
        db.push_back(rule{foo_head, {bar_body, baz_body}});  // idx 0
        
        const expr* bar_fact = ep.cons(ep.atom("bar"), ep.cons(ep.atom("1"), ep.atom("2")));
        db.push_back(rule{bar_fact, {}});  // idx 1
        
        const expr* baz_fact = ep.cons(ep.atom("baz"), ep.cons(ep.atom("2"), ep.atom("3")));
        db.push_back(rule{baz_fact, {}});  // idx 2
        
        a01_goals goals;
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("foo"), ep.cons(A, ep.cons(B, C))));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(A_normalized == ep.atom("1"));
        assert(B_normalized == ep.atom("2"));
        assert(C_normalized == ep.atom("3"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        const expr* cons_a_X = ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), X));
        db.push_back(rule{ep.cons(ep.atom("p"), cons_a_X), {}});  // idx 0
        
        const expr* cons_b_Y = ep.cons(ep.atom("cons"), ep.cons(ep.atom("b"), Y));
        db.push_back(rule{ep.cons(ep.atom("p"), cons_b_Y), {}});  // idx 1
        
        const expr* atom_z = ep.cons(ep.atom("atom"), ep.atom("z"));
        db.push_back(rule{ep.cons(ep.atom("p"), atom_z), {}});  // idx 2
        
        a01_goals goals;
        const expr* cons_a_nil = ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), ep.atom("nil")));
        goals.push_back(ep.cons(ep.atom("p"), cons_a_nil));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("person"), ep.atom("alice")), {}});    // idx 0
        db.push_back(rule{ep.cons(ep.atom("person"), ep.atom("bob")), {}});      // idx 1
        db.push_back(rule{ep.cons(ep.atom("person"), ep.atom("charlie")), {}});  // idx 2
        
        a01_goals goals;
        const expr* Who = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("person"), Who));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(Who_normalized == ep.atom("bob"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{ep.cons(ep.atom("p"), X), {ep.cons(ep.atom("q"), X)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("p"), Y), {ep.cons(ep.atom("r"), Y)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("b")), {}});  // idx 3
        
        a01_goals goals;
        const expr* Z = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), Z));
        
        // Pre-populate avoidance store to avoid idx 0
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl_avoid = lp.resolution(gl_p, 0);
        a01_decision_store avoidance;
        avoidance.insert(rl_avoid);
        a01_avoidance_store as;
        as.insert(avoidance);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(Z_normalized == ep.atom("b"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X), {ep.cons(ep.atom("q"), Y)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 1
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("hello")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X), {ep.cons(ep.atom("p"), X)}});  // idx 0: p(X) :- p(X)
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(5, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);  // Low max_resolutions
        
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
        
        a01_database db;  // Empty!
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict
        assert(result == false);
        
        // CRITICAL: No resolutions
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal still in store with no candidates
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.gs.count(gl_p) == 1);
        assert(simulation.cs.count(gl_p) == 0);
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        db.push_back(rule{ep.cons(ep.atom("p"), X), {}});  // idx 0: p(X). (fact with variable)
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("hello")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* W = ep.var(seq());
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        const expr* Z = ep.var(seq());
        
        const expr* big_head = ep.cons(ep.atom("big"), ep.cons(W, ep.cons(X, ep.cons(Y, Z))));
        const expr* a_body = ep.cons(ep.atom("a"), W);
        const expr* b_body = ep.cons(ep.atom("b"), X);
        const expr* c_body = ep.cons(ep.atom("c"), Y);
        const expr* d_body = ep.cons(ep.atom("d"), Z);
        db.push_back(rule{big_head, {a_body, b_body, c_body, d_body}});  // idx 0
        
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("1")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("2")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("c"), ep.atom("3")), {}});  // idx 3
        db.push_back(rule{ep.cons(ep.atom("d"), ep.atom("4")), {}});  // idx 4
        
        a01_goals goals;
        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* D = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("big"), ep.cons(A, ep.cons(B, ep.cons(C, D)))));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(norm(A) == ep.atom("1"));
        assert(norm(B) == ep.atom("2"));
        assert(norm(C) == ep.atom("3"));
        assert(norm(D) == ep.atom("4"));
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        db.push_back(rule{ep.cons(ep.atom("p"), X), {ep.cons(ep.atom("q"), X)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("p"), Y), {ep.cons(ep.atom("r"), Y)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("b")), {}});  // idx 3
        
        a01_goals goals;
        const expr* Z = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), Z));
        
        // Pre-populate avoidance store with BOTH resolutions
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl_p, 0);
        const resolution_lineage* rl1 = lp.resolution(gl_p, 1);
        
        a01_decision_store avoidance1;
        avoidance1.insert(rl0);
        
        a01_decision_store avoidance2;
        avoidance2.insert(rl1);
        
        a01_avoidance_store as;
        as.insert(avoidance1);
        as.insert(avoidance2);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Conflict (both candidates eliminated)
        assert(result == false);
        
        // CRITICAL: No resolutions made
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal still in store with no candidates
        assert(simulation.gs.count(gl_p) == 1);
        assert(simulation.cs.count(gl_p) == 0);  // Both eliminated by CDCL
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());
        
        const expr* cons_a_X = ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), X));
        db.push_back(rule{ep.cons(ep.atom("p"), cons_a_X), {}});  // idx 0
        
        const expr* cons_b_Y = ep.cons(ep.atom("cons"), ep.cons(ep.atom("b"), Y));
        db.push_back(rule{ep.cons(ep.atom("p"), cons_b_Y), {}});  // idx 1
        
        a01_goals goals;
        const expr* atom_z = ep.cons(ep.atom("atom"), ep.atom("z"));
        goals.push_back(ep.cons(ep.atom("p"), atom_z));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict (head elim removes all)
        assert(result == false);
        
        // CRITICAL: No resolutions
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal in store with no candidates
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.gs.count(gl_p) == 1);
        assert(simulation.cs.count(gl_p) == 0);
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
        
        a01_database db;
        const expr* level1 = ep.cons(ep.atom("cons"), ep.cons(ep.atom("a"), ep.atom("b")));
        const expr* level2 = ep.cons(ep.atom("cons"), ep.cons(level1, ep.atom("c")));
        const expr* level3 = ep.cons(ep.atom("cons"), ep.cons(level2, ep.atom("d")));
        const expr* level4 = ep.cons(ep.atom("cons"), ep.cons(level3, ep.atom("e")));
        const expr* level5 = ep.cons(ep.atom("cons"), ep.cons(level4, ep.atom("f")));
        db.push_back(rule{ep.cons(ep.atom("deep"), level5), {}});  // idx 0
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("deep"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* vars[10];
        for (int i = 0; i < 10; i++) {
            vars[i] = ep.var(seq());
        }
        
        // Build chain: a→b→c→d→e→f→g→h→i→j→end
        db.push_back(rule{ep.cons(ep.atom("a"), vars[0]), {ep.cons(ep.atom("b"), vars[0])}});
        db.push_back(rule{ep.cons(ep.atom("b"), vars[1]), {ep.cons(ep.atom("c"), vars[1])}});
        db.push_back(rule{ep.cons(ep.atom("c"), vars[2]), {ep.cons(ep.atom("d"), vars[2])}});
        db.push_back(rule{ep.cons(ep.atom("d"), vars[3]), {ep.cons(ep.atom("e"), vars[3])}});
        db.push_back(rule{ep.cons(ep.atom("e"), vars[4]), {ep.cons(ep.atom("f"), vars[4])}});
        db.push_back(rule{ep.cons(ep.atom("f"), vars[5]), {ep.cons(ep.atom("g"), vars[5])}});
        db.push_back(rule{ep.cons(ep.atom("g"), vars[6]), {ep.cons(ep.atom("h"), vars[6])}});
        db.push_back(rule{ep.cons(ep.atom("h"), vars[7]), {ep.cons(ep.atom("i"), vars[7])}});
        db.push_back(rule{ep.cons(ep.atom("i"), vars[8]), {ep.cons(ep.atom("j"), vars[8])}});
        db.push_back(rule{ep.cons(ep.atom("j"), ep.atom("end")), {}});
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("a"), X));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(5, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);  // Max 5
        
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("p"), ep.atom("a")), {ep.cons(ep.atom("q"), ep.atom("b"))}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("c")), {}});  // idx 1: q(c) not q(b)!
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("a")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(simulation.gs.count(gl_q) == 1);
        assert(simulation.cs.count(gl_q) == 0);
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
        
        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("edge"), ep.cons(ep.atom("a"), ep.atom("b"))), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("edge"), ep.cons(ep.atom("a"), ep.atom("c"))), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("edge"), ep.cons(ep.atom("a"), ep.atom("d"))), {}});  // idx 2
        
        a01_goals goals;
        const expr* X = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("edge"), ep.cons(ep.atom("a"), X)));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        assert(norm(X) == ep.atom("d"));
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
        
        a01_database db;
        const expr* nested_db = ep.cons(ep.atom("cons"), 
                                   ep.cons(ep.atom("a"), 
                                      ep.cons(ep.atom("cons"), 
                                         ep.cons(ep.atom("b"), ep.atom("nil")))));
        db.push_back(rule{ep.cons(ep.atom("p"), nested_db), {}});  // idx 0
        
        a01_goals goals;
        const expr* nested_goal = ep.cons(ep.atom("cons"), 
                                     ep.cons(ep.atom("a"), 
                                        ep.cons(ep.atom("cons"), 
                                           ep.cons(ep.atom("c"), ep.atom("nil")))));
        goals.push_back(ep.cons(ep.atom("p"), nested_goal));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Immediate conflict (head elim fails)
        assert(result == false);
        
        // CRITICAL: No resolutions
        assert(simulation.rs.size() == 0);
        
        // CRITICAL: Goal has no candidates
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        assert(simulation.cs.count(gl_p) == 0);
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        const expr* weird = ep.cons(ep.atom("weird"), ep.cons(X, ep.cons(X, X)));
        db.push_back(rule{weird, {}});  // idx 0: weird(X,X,X)
        
        a01_goals goals;
        const expr* Y = ep.var(seq());
        const expr* goal = ep.cons(ep.atom("weird"), ep.cons(ep.atom("a"), ep.cons(Y, ep.atom("a"))));
        goals.push_back(goal);
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Solution found
        assert(result == true);
        
        // CRITICAL: Exactly 1 resolution
        assert(simulation.rs.size() == 1);
        
        // CRITICAL: Y binds to 'a'
        normalizer norm(ep, bm);
        assert(norm(Y) == ep.atom("a"));
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
        
        a01_database db;
        const expr* X1 = ep.var(seq());
        const expr* X2 = ep.var(seq());
        const expr* X3 = ep.var(seq());
        
        db.push_back(rule{ep.cons(ep.atom("p"), X1), {ep.cons(ep.atom("q"), X1)}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("p"), X2), {ep.cons(ep.atom("r"), X2)}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("p"), X3), {ep.cons(ep.atom("s"), X3)}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 3
        db.push_back(rule{ep.cons(ep.atom("r"), ep.atom("b")), {}});  // idx 4
        db.push_back(rule{ep.cons(ep.atom("s"), ep.atom("c")), {}});  // idx 5
        
        a01_goals goals;
        const expr* Z = ep.var(seq());
        goals.push_back(ep.cons(ep.atom("p"), Z));
        
        // Pre-populate avoidance to eliminate idx 0 and 1
        const goal_lineage* gl_p = lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = lp.resolution(gl_p, 0);
        const resolution_lineage* rl1 = lp.resolution(gl_p, 1);
        
        a01_decision_store av1;
        av1.insert(rl0);
        a01_decision_store av2;
        av2.insert(rl1);
        
        a01_avoidance_store as;
        as.insert(av1);
        as.insert(av2);
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
        bool result = simulation();
        
        // CRITICAL: Solution found (only idx 2 remains)
        assert(result == true);
        
        // CRITICAL: 2 resolutions (p→s, s)
        assert(simulation.rs.size() == 2);
        
        // CRITICAL: Z binds to 'c'
        normalizer norm(ep, bm);
        assert(norm(Z) == ep.atom("c"));
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
        
        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});  // idx 0: p. (no variables)
        
        a01_goals goals;
        goals.push_back(ep.atom("p"));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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
        
        a01_database db;
        const expr* X = ep.var(seq());
        db.push_back(rule{X, {ep.cons(ep.atom("q"), ep.atom("a"))}});  // idx 0: X :- q(a).
        db.push_back(rule{ep.cons(ep.atom("q"), ep.atom("a")), {}});  // idx 1: q(a).
        
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("p"), ep.atom("hello")));
        
        a01_avoidance_store as;
        
        monte_carlo::tree_node<a01_decider::choice> root;
        std::mt19937 rng(42);
        monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, 1.414, rng);
        
        a01_resolution_store rs;
        a01_decision_store ds;
        a01_sim simulation(100, db, goals, t, seq, ep, bm, lp, rs, ds, as, sim);
        
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

void test_a01_constructor_and_destructor() {
    // Test 1: Basic construction - verify trail frame pushed and all fields stored correctly
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        a01_goals goals;
        std::mt19937 rng(42);

        size_t depth_before = t.depth();
        assert(depth_before == 1);

        {
            a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

            // CRITICAL: Constructor pushes one trail frame
            assert(t.depth() == depth_before + 1);

            // CRITICAL: References stored correctly
            assert(&solver.db == &db);
            assert(&solver.goals == &goals);
            assert(&solver.t == &t);
            assert(&solver.vars == &seq);
            assert(&solver.bm == &bm);
            assert(&solver.rng == &rng);

            // CRITICAL: Scalar fields stored correctly
            assert(solver.max_resolutions == 100);
            assert(solver.iterations_per_avoidance == 10);
            assert(solver.c == 1.414);

            // CRITICAL: Avoidance store initialized empty
            assert(solver.as.empty());
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

        a01_database db;
        a01_goals goals;
        std::mt19937 rng(0);

        assert(t.depth() == 0);

        {
            a01 solver(db, goals, t, seq, bm, 1, 1, 0.0, rng);

            // CRITICAL: Depth goes 0 → 1
            assert(t.depth() == 1);
            assert(solver.max_resolutions == 1);
            assert(solver.iterations_per_avoidance == 1);
            assert(solver.c == 0.0);
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

        a01_database db;
        a01_goals goals;
        std::mt19937 rng(42);

        size_t undo_size_before = t.undo_stack.size();
        size_t boundary_size_before = t.frame_boundary_stack.size();

        {
            a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

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

        a01_database db;
        a01_goals goals;
        std::mt19937 rng(42);

        bool undone = false;

        {
            a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

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

        a01_database db;
        a01_goals goals;
        std::mt19937 rng(42);

        bool caller_undone = false;
        t.log([&caller_undone]() { caller_undone = true; });

        {
            a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

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

        a01_database db;
        db.push_back(rule{ep.atom("p"), {}});
        db.push_back(rule{ep.cons(ep.atom("q"), ep.var(seq())), {ep.atom("p")}});

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("q"), ep.atom("a")));

        std::mt19937 rng(999);

        size_t depth_before = t.depth();

        {
            a01 solver(db, goals, t, seq, bm, 50, 5, 1.0, rng);

            // CRITICAL: Frame pushed
            assert(t.depth() == depth_before + 1);

            // CRITICAL: db reference correct; content accessible through it
            assert(&solver.db == &db);
            assert(solver.db.size() == 2);

            // CRITICAL: goals reference correct; content accessible through it
            assert(&solver.goals == &goals);
            assert(solver.goals.size() == 1);

            // CRITICAL: Avoidance store empty on construction regardless of db/goals content
            assert(solver.as.empty());
        }

        // CRITICAL: Destructor restores depth
        assert(t.depth() == depth_before);
    }
}

void test_a01_sim_one() {
    // Test 1: Immediate solution via unit propagation
    // db: {a.}, goals: {a.}
    // Expected: returns true, rs has 1 resolution, ds empty,
    //           trail depth invariant, MCTS root gets 1 visit
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // idx 0: a.

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        monte_carlo::tree_node<a01_decider::choice> root;
        a01_decision_store ds;
        a01_resolution_store rs;

        size_t depth_before = t.depth();

        bool result = solver.sim_one(root, ds, rs);

        // CRITICAL: Solution found
        assert(result == true);

        // CRITICAL: ds empty (unit prop, no MCTS decision)
        assert(ds.empty());

        // CRITICAL: rs has the single resolution
        assert(rs.size() == 1);
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        assert(rs.count(rl0) == 1);

        // CRITICAL: Trail depth is invariant (pop + push inside sim_one)
        assert(t.depth() == depth_before);

        // CRITICAL: MCTS root visited exactly once (terminate() always touches root)
        assert(root.m_visits == 1);

        // CRITICAL: Reward is -ds.size() where ds was EMPTY going in → value == 0
        assert(root.m_value == 0.0);
    }

    // Test 2: Immediate conflict - empty database
    // db: {}, goals: {a.}
    // Expected: returns false, ds empty, rs empty, trail depth invariant, root visits == 1
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        monte_carlo::tree_node<a01_decider::choice> root;
        a01_decision_store ds;
        a01_resolution_store rs;

        size_t depth_before = t.depth();

        bool result = solver.sim_one(root, ds, rs);

        // CRITICAL: Conflict detected
        assert(result == false);

        // CRITICAL: No resolutions or decisions
        assert(ds.empty());
        assert(rs.empty());

        // CRITICAL: Trail depth invariant
        assert(t.depth() == depth_before);

        // CRITICAL: Root visited once, value == 0 (ds was empty going in)
        assert(root.m_visits == 1);
        assert(root.m_value == 0.0);
    }

    // Test 3: Trail pop/push behavior via secondary sequencer
    // The a01 constructor pushes a frame. Logging actions into that frame
    // (e.g. calling a secondary sequencer) must be rolled back when sim_one
    // calls t.pop() at the start of the simulation.
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        // Create a secondary sequencer on the SAME trail (after a01's constructor push)
        sequencer seq2(t);
        assert(seq2.index == 0);

        // Allocate a variable - this logs a decrement undo into the a01's current frame
        uint32_t v = seq2();
        assert(v == 0);
        assert(seq2.index == 1);

        monte_carlo::tree_node<a01_decider::choice> root;
        a01_decision_store ds;
        a01_resolution_store rs;

        size_t depth_before = t.depth();

        solver.sim_one(root, ds, rs);

        // CRITICAL: t.pop() at start of sim_one triggered the seq2 undo action,
        //           rolling seq2.index back to 0
        assert(seq2.index == 0);

        // CRITICAL: t.push() after the pop restored the same depth
        assert(t.depth() == depth_before);
    }

    // Test 4: MCTS termination reward uses the OUTGOING (simulation's) ds, not the incoming ds.
    // db: {a :- b., a :- c., b., c.} — 2 candidates for a → MCTS must make exactly 1 decision.
    // The incoming ds is empty on the first call, but the output ds has 1 element (the decision).
    // With the fix: terminate(-1.0) → root.m_value = -1.0.
    // With the old bug: terminate(-0.0) → root.m_value = 0.0.
    // So root.m_value == -1.0 distinguishes correct from buggy behavior.
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {}});               // idx 2
        db.push_back(rule{ep.atom("c"), {}});               // idx 3

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        monte_carlo::tree_node<a01_decider::choice> root;
        a01_decision_store ds;
        a01_resolution_store rs;

        solver.sim_one(root, ds, rs);

        // CRITICAL: MCTS made exactly 1 decision (outgoing ds has 1 element)
        assert(ds.size() == 1);
        assert(root.m_visits == 1);

        // CRITICAL: Reward is based on output ds size (1), NOT input ds size (0).
        // Old buggy behavior would leave root.m_value == 0.0.
        assert(root.m_value == -1.0);
    }

    // Test 5: CDCL avoidance injected via solver.as after construction
    // db: {a :- b., a :- c., b., c.}, goals: {a.}
    // After construction, inject a singleton avoidance for (gl0, idx 0) directly into solver.as.
    // sim_one's a01_sim will pick up this copy and CDCL-eliminate idx 0,
    // leaving only idx 1 → unit propagation, so ds remains empty.
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {}});               // idx 2
        db.push_back(rule{ep.atom("c"), {}});               // idx 3

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        // Inject avoidance: singleton {rl(gl0, 0)} causes CDCL elimination of idx 0
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        a01_decision_store avoid;
        avoid.insert(rl0);
        solver.as.insert(avoid);

        monte_carlo::tree_node<a01_decider::choice> root;
        a01_decision_store ds;
        a01_resolution_store rs;

        bool result = solver.sim_one(root, ds, rs);

        // CRITICAL: Solution found (idx 1 chosen via unit propagation after CDCL elim)
        assert(result == true);

        // CRITICAL: No decisions made (CDCL reduced to 1 candidate → unit prop)
        assert(ds.empty());

        // CRITICAL: 2 resolutions: a via idx 1, c via idx 3
        assert(rs.size() == 2);
        const resolution_lineage* rl_a = solver.lp.resolution(gl0, 1);
        assert(rs.count(rl_a) == 1);
        const resolution_lineage* rl_c = solver.lp.resolution(solver.lp.goal(rl_a, 0), 3);
        assert(rs.count(rl_c) == 1);
    }

    // Test 6: Pre-populated MCTS tree forces a specific decision; verify decisions and resolutions
    // db: {a :- b., a :- c., b., c.}, goals: {a.}
    // Force MCTS to pick idx 1 (a :- c.) as the decision.
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {}});               // idx 2
        db.push_back(rule{ep.atom("c"), {}});               // idx 3

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        monte_carlo::tree_node<a01_decider::choice> root;

        // Pre-populate the MCTS tree so that UCB1 selects idx 1 for the candidate choice.
        // The only goal is gl0; make its child visited so UCB1 is active at the next level.
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);

        root.m_visits = 100;
        root.m_children[gl0].m_visits = 50;
        root.m_children[gl0].m_value = 100.0;
        // idx 0 visited → UCB1 can score it; idx 1 unvisited → score = +∞ → chosen
        root.m_children[gl0].m_children[size_t(0)].m_visits = 10;
        root.m_children[gl0].m_children[size_t(0)].m_value = 20.0;
        root.m_children[gl0].m_children[size_t(1)].m_visits = 0;  // Unvisited → chosen!

        a01_decision_store ds;
        a01_resolution_store rs;

        bool result = solver.sim_one(root, ds, rs);

        // CRITICAL: Solution found
        assert(result == true);

        // CRITICAL: Exactly 1 decision (MCTS chose a→c)
        assert(ds.size() == 1);

        // CRITICAL: Exactly 2 resolutions (decision on a→idx1, unit prop on c→idx3)
        assert(rs.size() == 2);

        const resolution_lineage* rl_a = solver.lp.resolution(gl0, 1);
        assert(rs.count(rl_a) == 1);
        assert(ds.count(rl_a) == 1);  // This was the decision

        const goal_lineage* gl_c = solver.lp.goal(rl_a, 0);
        const resolution_lineage* rl_c = solver.lp.resolution(gl_c, 3);
        assert(rs.count(rl_c) == 1);
        assert(ds.count(rl_c) == 0);  // Unit propagation, not a decision

        // CRITICAL: terminate() uses the OUTPUT ds (1 element) → reward = -1.0
        assert(root.m_visits == 101);  // 100 + 1 from terminate()
        assert(root.m_value == -1.0);  // 0.0 (initial) + (-1.0) from terminate(-ds.size())
    }

    // Test 7: Multiple sim_one calls sharing one root - MCTS statistics accumulate
    // Each call increments root.m_visits by exactly 1 via terminate().
    {
        trail t;
        t.push();

        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1
        db.push_back(rule{ep.atom("b"), {}});               // idx 2
        db.push_back(rule{ep.atom("c"), {}});               // idx 3

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 100, 10, 1.414, rng);

        monte_carlo::tree_node<a01_decider::choice> root;
        a01_decision_store ds;
        a01_resolution_store rs;

        assert(root.m_visits == 0);

        solver.sim_one(root, ds, rs);
        assert(root.m_visits == 1);

        solver.sim_one(root, ds, rs);
        assert(root.m_visits == 2);

        solver.sim_one(root, ds, rs);
        assert(root.m_visits == 3);
    }
}

void test_a01_next_avoidance() {

    // Test 1: Immediate refutation — goal has no candidates from the start.
    // DB: empty, goals: {a}
    // First sim_one: conflict immediately, ds empty → returns false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns false (proven refutation — conflict with no decisions)
        assert(result == false);

        // CRITICAL: soln is nullopt
        assert(!soln.has_value());
    }

    // Test 2: Immediate solution via unit propagation.
    // DB: {a.}, goals: {a}
    // First sim_one: unit-prop resolves a → solution, ds empty.
    // Loop first iteration: solution again → soln set, avoidance = {}.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // idx 0: a.

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (solution found)
        assert(result == true);

        // CRITICAL: soln has value
        assert(soln.has_value());

        // CRITICAL: soln has exactly 1 resolution (unit-prop of a with rule 0)
        assert(soln.value().size() == 1);
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        assert(soln.value().count(rl0) == 1);

        // CRITICAL: avoidance is empty — unit propagation, no MCTS decisions
        assert(avoidance.empty());
    }

    // Test 3: Solution found via MCTS after exploring a conflicting path.
    // DB:
    //   Rule 0: q :- r.  (conflict — r has no rules)
    //   Rule 1: q :- p.  (solution — p is a fact)
    //   Rule 2: p.
    // Goals: {q}
    // q has 2 candidates, so MCTS must decide. Rule 1 leads to solution.
    // With 1000 iterations the solver reliably finds rule 1.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("q"), {ep.atom("r")}});  // idx 0: q :- r.
        db.push_back(rule{ep.atom("q"), {ep.atom("p")}});  // idx 1: q :- p.
        db.push_back(rule{ep.atom("p"), {}});               // idx 2: p.

        a01_goals goals;
        goals.push_back(ep.atom("q"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (solution found)
        assert(result == true);

        // CRITICAL: soln has value
        assert(soln.has_value());

        // CRITICAL: soln has exactly 2 resolutions (q→rule1 as decision, p→rule2 as unit prop)
        assert(soln.value().size() == 2);

        const goal_lineage* gl_q = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl_q1 = solver.lp.resolution(gl_q, 1);
        assert(soln.value().count(rl_q1) == 1);

        const goal_lineage* gl_p = solver.lp.goal(rl_q1, 0);
        const resolution_lineage* rl_p2 = solver.lp.resolution(gl_p, 2);
        assert(soln.value().count(rl_p2) == 1);

        // CRITICAL: avoidance = decisions of the solution sim = {rl_q1}
        // (the decision to pick rule 1 for q; p was unit-propagated, not a decision)
        assert(avoidance.size() == 1);
        assert(avoidance.count(rl_q1) == 1);
    }

    // Test 4: Depth-1 conflict minimum — avoidance.size() == 1.
    // DB:
    //   Rule 0: a :- f.  (a → f, no rules for f)
    //   Rule 1: a :- g.  (a → g, no rules for g)
    // Goals: {a}
    // Every MCTS path: exactly 1 decision (choose rule 0 or 1 for a), then
    // head-elimination leaves f/g with 0 candidates → immediate conflict.
    // After 1000 iterations the minimum observed ds size is 1.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("f")}});  // idx 0: a :- f.
        db.push_back(rule{ep.atom("a"), {ep.atom("g")}});  // idx 1: a :- g.
        // No rules for f or g

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (first sim had 1 decision, not a proven refutation)
        assert(result == true);

        // CRITICAL: No solution (all paths conflict)
        assert(!soln.has_value());

        // CRITICAL: Minimum conflict depth is 1 → avoidance.size() == 1
        assert(avoidance.size() == 1);

        // CRITICAL: The avoided resolution is on the initial goal (gl0) with either rule 0 or 1
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        const resolution_lineage* rl1 = solver.lp.resolution(gl0, 1);
        const resolution_lineage* avoided = *avoidance.begin();
        assert(avoided == rl0 || avoided == rl1);
    }

    // Test 5: Depth-2 conflict minimum — avoidance.size() == 2.
    // DB:
    //   Rule 0: a :- p.   Rule 1: a :- q.
    //   Rule 2: p :- f1.  Rule 3: p :- f2.
    //   Rule 4: q :- f3.  Rule 5: q :- f4.
    //   No rules for f1, f2, f3, f4.
    // Goals: {a}
    // Decision 1: a → rule 0 or 1 (p or q, each with 2 candidates).
    // Decision 2: p/q → rule 2/3 or 4/5 (f1/f2/f3/f4, each with 0 candidates → conflict).
    // Every path needs exactly 2 decisions. avoidance.size() == 2 after 1000 iterations.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("p")}});   // idx 0: a :- p.
        db.push_back(rule{ep.atom("a"), {ep.atom("q")}});   // idx 1: a :- q.
        db.push_back(rule{ep.atom("p"), {ep.atom("f1")}});  // idx 2: p :- f1.
        db.push_back(rule{ep.atom("p"), {ep.atom("f2")}});  // idx 3: p :- f2.
        db.push_back(rule{ep.atom("q"), {ep.atom("f3")}});  // idx 4: q :- f3.
        db.push_back(rule{ep.atom("q"), {ep.atom("f4")}});  // idx 5: q :- f4.
        // No rules for f1, f2, f3, f4

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (decisions were made, not a proven refutation)
        assert(result == true);

        // CRITICAL: No solution (all paths conflict)
        assert(!soln.has_value());

        // CRITICAL: Minimum conflict depth is 2 → avoidance.size() == 2
        assert(avoidance.size() == 2);
    }

    // Test 6: Depth-3 conflict minimum — avoidance.size() == 3.
    // DB forms a complete binary tree of decisions, 3 levels deep:
    //   Level 0: a → {p, q}
    //   Level 1: p → {r, s},  q → {t, u}
    //   Level 2: r → {f1, f2}, s → {f3, f4}, t → {f5, f6}, u → {f7, f8}
    //   No rules for f1-f8 → all leaves conflict immediately.
    // Every path takes exactly 3 decisions. avoidance.size() == 3 after 1000 iterations.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("p")}});   // idx 0
        db.push_back(rule{ep.atom("a"), {ep.atom("q")}});   // idx 1
        db.push_back(rule{ep.atom("p"), {ep.atom("r")}});   // idx 2
        db.push_back(rule{ep.atom("p"), {ep.atom("s")}});   // idx 3
        db.push_back(rule{ep.atom("q"), {ep.atom("t")}});   // idx 4
        db.push_back(rule{ep.atom("q"), {ep.atom("u")}});   // idx 5
        db.push_back(rule{ep.atom("r"), {ep.atom("f1")}});  // idx 6
        db.push_back(rule{ep.atom("r"), {ep.atom("f2")}});  // idx 7
        db.push_back(rule{ep.atom("s"), {ep.atom("f3")}});  // idx 8
        db.push_back(rule{ep.atom("s"), {ep.atom("f4")}});  // idx 9
        db.push_back(rule{ep.atom("t"), {ep.atom("f5")}});  // idx 10
        db.push_back(rule{ep.atom("t"), {ep.atom("f6")}});  // idx 11
        db.push_back(rule{ep.atom("u"), {ep.atom("f7")}});  // idx 12
        db.push_back(rule{ep.atom("u"), {ep.atom("f8")}});  // idx 13
        // No rules for f1-f8

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (decisions were made, not a proven refutation)
        assert(result == true);

        // CRITICAL: No solution (all paths conflict)
        assert(!soln.has_value());

        // CRITICAL: Minimum conflict depth is 3 → avoidance.size() == 3
        assert(avoidance.size() == 3);
    }

    // Test 7: Refutation via pre-existing avoidances injected into solver.as.
    // DB: {a.} (one fact), goals: {a}
    // Inject singleton avoidance {rl(gl0, 0)} into solver.as before calling.
    // CDCL-elimination removes the only candidate → conflict with ds empty → returns false.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // idx 0: a.

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        // Inject singleton avoidance: CDCL will eliminate rule 0 for gl0
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        a01_decision_store avoid;
        avoid.insert(rl0);
        solver.as.insert(avoid);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns false (CDCL eliminated all candidates → conflict, ds empty → refutation)
        assert(result == false);

        // CRITICAL: soln is nullopt
        assert(!soln.has_value());
    }

    // Test 8: Mixed conflict depths — MCTS converges to the shallower branch.
    //
    // Goal {a} has two candidates:
    //   rule0  →  a :- p.   (depth-4 branch)
    //   rule1  →  a :- q.   (depth-3 branch)
    //
    // Depth-4 subtree (via p):
    //   p → {r, s}  →  r/s → {t, u, v, w}  →  t/u/v/w → {fail1-8}
    //   (4 decisions: a, p, r/s, t/u/v/w)
    //
    // Depth-3 subtree (via q):
    //   q → {x, y}  →  x/y → {fail9-12}
    //   (3 decisions: a, q, x/y)
    //
    // After 1000 MCTS iterations the minimum observed ds is 3.
    // The avoidance must contain the "a → rule1" resolution (the first step of the
    // depth-3 path) because any 3-decision conflict must go through rule1 for a.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        // Root
        db.push_back(rule{ep.atom("a"), {ep.atom("p")}});   // idx 0  (depth-4)
        db.push_back(rule{ep.atom("a"), {ep.atom("q")}});   // idx 1  (depth-3)
        // Depth-4 branch: p layer
        db.push_back(rule{ep.atom("p"), {ep.atom("r")}});   // idx 2
        db.push_back(rule{ep.atom("p"), {ep.atom("s")}});   // idx 3
        // Depth-4 branch: r/s layer
        db.push_back(rule{ep.atom("r"), {ep.atom("t")}});   // idx 4
        db.push_back(rule{ep.atom("r"), {ep.atom("u")}});   // idx 5
        db.push_back(rule{ep.atom("s"), {ep.atom("v")}});   // idx 6
        db.push_back(rule{ep.atom("s"), {ep.atom("w")}});   // idx 7
        // Depth-4 leaves (each has 2 candidates → one more decision, then conflict)
        db.push_back(rule{ep.atom("t"), {ep.atom("fail1")}});  // idx 8
        db.push_back(rule{ep.atom("t"), {ep.atom("fail2")}});  // idx 9
        db.push_back(rule{ep.atom("u"), {ep.atom("fail3")}});  // idx 10
        db.push_back(rule{ep.atom("u"), {ep.atom("fail4")}});  // idx 11
        db.push_back(rule{ep.atom("v"), {ep.atom("fail5")}});  // idx 12
        db.push_back(rule{ep.atom("v"), {ep.atom("fail6")}});  // idx 13
        db.push_back(rule{ep.atom("w"), {ep.atom("fail7")}});  // idx 14
        db.push_back(rule{ep.atom("w"), {ep.atom("fail8")}});  // idx 15
        // Depth-3 branch: q layer
        db.push_back(rule{ep.atom("q"), {ep.atom("x")}});   // idx 16
        db.push_back(rule{ep.atom("q"), {ep.atom("y")}});   // idx 17
        // Depth-3 leaves
        db.push_back(rule{ep.atom("x"), {ep.atom("fail9")}});  // idx 18
        db.push_back(rule{ep.atom("x"), {ep.atom("fail10")}});  // idx 19
        db.push_back(rule{ep.atom("y"), {ep.atom("fail11")}});  // idx 20
        db.push_back(rule{ep.atom("y"), {ep.atom("fail12")}});  // idx 21
        // No rules for fail1-fail12

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (decisions were made, not a proven refutation)
        assert(result == true);

        // CRITICAL: No solution (all paths conflict)
        assert(!soln.has_value());

        // CRITICAL: Minimum conflict depth is 3 (the q branch), not 4 (the p branch)
        assert(avoidance.size() == 3);

        // CRITICAL: The avoidance must include "a → rule1" (the depth-3 branch entry
        // point). Any 3-decision conflict necessarily begins with that choice.
        const goal_lineage* gl_a = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl_a1 = solver.lp.resolution(gl_a, 1);
        assert(avoidance.count(rl_a1) == 1);
    }

    // Test 9: Needle in a haystack — two depth-4 branches plus one depth-3 branch.
    //
    // Goal {a} has THREE candidates:
    //   rule0  →  a :- p_alpha.  (depth-4)
    //   rule1  →  a :- p_beta.   (depth-4)
    //   rule2  →  a :- q.        (depth-3, the needle)
    //
    // Two out of three first-level choices lead to a 4-decision conflict.
    // Only rule2 leads to the shallower 3-decision conflict.
    // After 1000 MCTS iterations the minimum is 3, and the avoidance identifies
    // the shallow entry point ("a → rule2").
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        // Three choices for a
        db.push_back(rule{ep.atom("a"), {ep.atom("p_alpha")}});  // idx 0  (depth-4)
        db.push_back(rule{ep.atom("a"), {ep.atom("p_beta")}});   // idx 1  (depth-4)
        db.push_back(rule{ep.atom("a"), {ep.atom("q")}});        // idx 2  (depth-3)

        // Depth-4 branch A (via p_alpha)
        db.push_back(rule{ep.atom("p_alpha"), {ep.atom("r_alpha")}});  // idx 3
        db.push_back(rule{ep.atom("p_alpha"), {ep.atom("s_alpha")}});  // idx 4
        db.push_back(rule{ep.atom("r_alpha"), {ep.atom("t_alpha")}});  // idx 5
        db.push_back(rule{ep.atom("r_alpha"), {ep.atom("u_alpha")}});  // idx 6
        db.push_back(rule{ep.atom("t_alpha"), {ep.atom("fail1")}});    // idx 7
        db.push_back(rule{ep.atom("t_alpha"), {ep.atom("fail2")}});    // idx 8
        db.push_back(rule{ep.atom("u_alpha"), {ep.atom("fail3")}});    // idx 9
        db.push_back(rule{ep.atom("u_alpha"), {ep.atom("fail4")}});    // idx 10
        db.push_back(rule{ep.atom("s_alpha"), {ep.atom("v_alpha")}});  // idx 11
        db.push_back(rule{ep.atom("s_alpha"), {ep.atom("w_alpha")}});  // idx 12
        db.push_back(rule{ep.atom("v_alpha"), {ep.atom("fail5")}});    // idx 13
        db.push_back(rule{ep.atom("v_alpha"), {ep.atom("fail6")}});    // idx 14
        db.push_back(rule{ep.atom("w_alpha"), {ep.atom("fail7")}});    // idx 15
        db.push_back(rule{ep.atom("w_alpha"), {ep.atom("fail8")}});    // idx 16

        // Depth-4 branch B (via p_beta)
        db.push_back(rule{ep.atom("p_beta"), {ep.atom("r_beta")}});   // idx 17
        db.push_back(rule{ep.atom("p_beta"), {ep.atom("s_beta")}});   // idx 18
        db.push_back(rule{ep.atom("r_beta"), {ep.atom("t_beta")}});   // idx 19
        db.push_back(rule{ep.atom("r_beta"), {ep.atom("u_beta")}});   // idx 20
        db.push_back(rule{ep.atom("t_beta"), {ep.atom("fail9")}});    // idx 21
        db.push_back(rule{ep.atom("t_beta"), {ep.atom("fail10")}});   // idx 22
        db.push_back(rule{ep.atom("u_beta"), {ep.atom("fail11")}});   // idx 23
        db.push_back(rule{ep.atom("u_beta"), {ep.atom("fail12")}});   // idx 24
        db.push_back(rule{ep.atom("s_beta"), {ep.atom("v_beta")}});   // idx 25
        db.push_back(rule{ep.atom("s_beta"), {ep.atom("w_beta")}});   // idx 26
        db.push_back(rule{ep.atom("v_beta"), {ep.atom("fail13")}});   // idx 27
        db.push_back(rule{ep.atom("v_beta"), {ep.atom("fail14")}});   // idx 28
        db.push_back(rule{ep.atom("w_beta"), {ep.atom("fail15")}});   // idx 29
        db.push_back(rule{ep.atom("w_beta"), {ep.atom("fail16")}});   // idx 30

        // Depth-3 branch (via q — the needle)
        db.push_back(rule{ep.atom("q"), {ep.atom("x")}});             // idx 31
        db.push_back(rule{ep.atom("q"), {ep.atom("y")}});             // idx 32
        db.push_back(rule{ep.atom("x"), {ep.atom("fail17")}});        // idx 33
        db.push_back(rule{ep.atom("x"), {ep.atom("fail18")}});        // idx 34
        db.push_back(rule{ep.atom("y"), {ep.atom("fail19")}});        // idx 35
        db.push_back(rule{ep.atom("y"), {ep.atom("fail20")}});        // idx 36
        // No rules for fail1-fail20

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true
        assert(result == true);

        // CRITICAL: No solution (all paths conflict)
        assert(!soln.has_value());

        // CRITICAL: Despite two out of three first-level choices leading to depth-4
        // conflicts, MCTS finds the depth-3 needle (rule2 → q).
        assert(avoidance.size() == 3);

        // CRITICAL: "a → rule2" (the q branch entry point) is in the avoidance.
        // This is the distinguishing feature of the depth-3 path — rules 0 and 1
        // only appear in 4-decision avoidances, never in 3-decision ones.
        const goal_lineage* gl_a = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl_a2 = solver.lp.resolution(gl_a, 2);
        assert(avoidance.count(rl_a2) == 1);
    }

    // Test 10: Two goals share variable X — MCTS finds the unique intersection value.
    //
    // This is the motivating example for shared-variable parallel constraints:
    //   "initial goal A -> rule 1 and initial goal B -> rule 2 causes conflict because
    //    of having a common variable"
    //
    // Goals: { a(X), b(X) }   — same variable X shared across both goals
    //   Rule 0: a(123).        — X=123; then b(123) has no candidate → CONFLICT
    //   Rule 1: a(234).        — X=234; then b(234) is the only matching candidate → SOLUTION
    //   Rule 2: b(234).        — X=234; then a(234) unit-propagates → SOLUTION
    //   Rule 3: b(345).        — X=345; then a(345) has no candidate → CONFLICT
    //
    // Every path is depth-1. The "parallel" effect: choosing a value for X via one goal
    // immediately determines whether the other goal succeeds or fails — both goals are
    // constrained in parallel by the same variable binding.
    //
    // With 1000 MCTS iterations the solver finds the solution (X=234 satisfies both).
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        const expr* X = ep.var(seq());

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("123")), {}});  // idx 0: a(123).
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("234")), {}});  // idx 1: a(234).
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("234")), {}});  // idx 2: b(234).
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("345")), {}});  // idx 3: b(345).

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("a"), X));  // a(X)
        goals.push_back(ep.cons(ep.atom("b"), X));  // b(X) — shares X with a

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Solution found (X=234 satisfies both)
        assert(result == true);
        assert(soln.has_value());

        // CRITICAL: Exactly 2 resolutions — one decision (the decided goal) plus one
        // unit-propagation (the other goal, whose candidate was forced by X's binding)
        assert(soln.value().size() == 2);

        // CRITICAL: The solution involves rule 1 for a (a→234) and rule 2 for b (b→234)
        const goal_lineage* gl_a = solver.lp.goal(nullptr, 0);
        const goal_lineage* gl_b = solver.lp.goal(nullptr, 1);
        const resolution_lineage* rl_a1 = solver.lp.resolution(gl_a, 1);  // a(234)
        const resolution_lineage* rl_b2 = solver.lp.resolution(gl_b, 2);  // b(234)
        // One of these was the MCTS decision, the other the unit-propagation.
        // Both must appear in the solution's resolution store.
        assert(soln.value().count(rl_a1) == 1);
        assert(soln.value().count(rl_b2) == 1);

        // CRITICAL: The avoidance contains exactly 1 decision (the one MCTS chose)
        assert(avoidance.size() == 1);
        assert(avoidance.count(rl_a1) == 1 || avoidance.count(rl_b2) == 1);
    }

    // Test 11: Three goals share variable X — no single X value satisfies all three.
    //
    // Goals: { a(X), b(X), c(X) }   — all three share variable X
    //   Rule 0: a(1).   Rule 1: a(2).        →  X ∈ {1, 2}  for a
    //   Rule 2: b(2).   Rule 3: b(3).        →  X ∈ {2, 3}  for b
    //   Rule 4: c(1).   Rule 5: c(3).        →  X ∈ {1, 3}  for c
    //
    // There is no X that satisfies all three simultaneously:
    //   X=1 → b(1) has no candidate (b needs 2 or 3)
    //   X=2 → c(2) has no candidate (c needs 1 or 3)
    //   X=3 → a(3) has no candidate (a needs 1 or 2)
    //
    // Every first MCTS decision immediately propagates X to a value that kills one of
    // the other goals — minimum conflict depth is 1 regardless of which goal is chosen.
    // MCTS finds and records this depth-1 avoidance after 1000 iterations.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        const expr* X = ep.var(seq());

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("1")), {}});  // idx 0: a(1).
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("2")), {}});  // idx 1: a(2).
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("2")), {}});  // idx 2: b(2).
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("3")), {}});  // idx 3: b(3).
        db.push_back(rule{ep.cons(ep.atom("c"), ep.atom("1")), {}});  // idx 4: c(1).
        db.push_back(rule{ep.cons(ep.atom("c"), ep.atom("3")), {}});  // idx 5: c(3).

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("a"), X));  // a(X)
        goals.push_back(ep.cons(ep.atom("b"), X));  // b(X) — shares X with a
        goals.push_back(ep.cons(ep.atom("c"), X));  // c(X) — shares X with a and b

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (at least one decision was made — not an unconditional refutation)
        assert(result == true);

        // CRITICAL: No solution (the three goals' X-constraints are mutually exclusive)
        assert(!soln.has_value());

        // CRITICAL: The minimum conflict depth is 1 — one MCTS decision immediately
        // binds X to a value that eliminates all candidates in at least one other goal
        assert(avoidance.size() == 1);

        // CRITICAL: The avoided resolution is on one of the three initial goals.
        // Whichever goal MCTS chose first, that single decision is the avoidance.
        const goal_lineage* gl_a = solver.lp.goal(nullptr, 0);
        const goal_lineage* gl_b = solver.lp.goal(nullptr, 1);
        const goal_lineage* gl_c = solver.lp.goal(nullptr, 2);
        const resolution_lineage* avoided = *avoidance.begin();
        // The avoided resolution must involve one of the three goals and one of their rules
        bool is_a_rule = avoided == solver.lp.resolution(gl_a, 0) || avoided == solver.lp.resolution(gl_a, 1);
        bool is_b_rule = avoided == solver.lp.resolution(gl_b, 2) || avoided == solver.lp.resolution(gl_b, 3);
        bool is_c_rule = avoided == solver.lp.resolution(gl_c, 4) || avoided == solver.lp.resolution(gl_c, 5);
        assert(is_a_rule || is_b_rule || is_c_rule);
    }

    // Test 12: Two independent variables, compound goal c(X,Y) teaches MCTS goal ordering.
    //
    // Goals: { a(X), b(Y), c(X,Y) }   — X shared by a and c; Y shared by b and c
    //   Rule 0: a(1).  Rule 1: a(2).        →  X ∈ {1, 2}
    //   Rule 2: b(1).  Rule 3: b(2).        →  Y ∈ {1, 2}
    //   Rule 4: c(1,5).  Rule 5: c(1,6).   →  (X=1,Y=5) or (X=1,Y=6)
    //   Rule 6: c(2,5).  Rule 7: c(2,6).   →  (X=2,Y=5) or (X=2,Y=6)
    //
    // c requires Y ∈ {5, 6}, but b only produces Y ∈ {1, 2} — ranges are disjoint.
    // The conflict DEPTH depends on which goal MCTS resolves first:
    //
    //   Resolving b first  (Y ∈ {1,2}): c(X, 1/2) has 0 candidates → CONFLICT at depth-1.
    //   Resolving c first  (X=1/2, Y=5/6): b(5/6) has 0 candidates → CONFLICT at depth-1.
    //   Resolving a first  (X ∈ {1,2}): c(X,Y) still has 2 candidates, b(Y) still has 2
    //                       candidates → needs a SECOND decision → depth-2 conflict.
    //
    // With 1000 MCTS iterations, the solver learns that starting with b or c (rather than a)
    // yields a shallower conflict. The minimum avoidance size is 1.
    // This demonstrates MCTS learning the optimal GOAL ORDERING via shared-variable
    // constraint propagation.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        const expr* X = ep.var(seq());
        const expr* Y = ep.var(seq());

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("1")), {}});  // idx 0: a(1).
        db.push_back(rule{ep.cons(ep.atom("a"), ep.atom("2")), {}});  // idx 1: a(2).
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("1")), {}});  // idx 2: b(1).
        db.push_back(rule{ep.cons(ep.atom("b"), ep.atom("2")), {}});  // idx 3: b(2).
        // c(X, Y) represented as ((c . X) . Y)
        db.push_back(rule{ep.cons(ep.cons(ep.atom("c"), ep.atom("1")), ep.atom("5")), {}});  // idx 4: c(1,5).
        db.push_back(rule{ep.cons(ep.cons(ep.atom("c"), ep.atom("1")), ep.atom("6")), {}});  // idx 5: c(1,6).
        db.push_back(rule{ep.cons(ep.cons(ep.atom("c"), ep.atom("2")), ep.atom("5")), {}});  // idx 6: c(2,5).
        db.push_back(rule{ep.cons(ep.cons(ep.atom("c"), ep.atom("2")), ep.atom("6")), {}});  // idx 7: c(2,6).

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("a"), X));                // a(X)
        goals.push_back(ep.cons(ep.atom("b"), Y));                // b(Y)
        goals.push_back(ep.cons(ep.cons(ep.atom("c"), X), Y));   // c(X, Y)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        // CRITICAL: Returns true (not a proven refutation — the first sim had ≥1 decision)
        assert(result == true);

        // CRITICAL: No solution (c requires Y ∈ {5,6}, b only gives Y ∈ {1,2} — disjoint)
        assert(!soln.has_value());

        // CRITICAL: MCTS learns to pick b or c before a, yielding a depth-1 conflict.
        // The minimum avoidance size is 1, not 2 (which would result from picking a first).
        assert(avoidance.size() == 1);

        // CRITICAL: The single avoided resolution is on b or c (not on a), confirming
        // MCTS discovered the optimal goal-first ordering.
        // Picking b (Y∈{1,2}) → c(X, 1/2) immediately fails.
        // Picking c (X=1/2, Y=5/6) → b(5/6) immediately fails.
        // Picking a alone (X∈{1,2}) → c still has 2 candidates → depth-2, NOT chosen.
        const goal_lineage* gl_b = solver.lp.goal(nullptr, 1);
        const goal_lineage* gl_c = solver.lp.goal(nullptr, 2);
        const resolution_lineage* avoided = *avoidance.begin();
        bool is_b_choice = avoided == solver.lp.resolution(gl_b, 2) || avoided == solver.lp.resolution(gl_b, 3);
        bool is_c_choice = avoided == solver.lp.resolution(gl_c, 4) || avoided == solver.lp.resolution(gl_c, 5)
                        || avoided == solver.lp.resolution(gl_c, 6) || avoided == solver.lp.resolution(gl_c, 7);
        assert(is_b_choice || is_c_choice);
    }

    // Test 13: Depth-4 needle in a depth-10 haystack.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;

        // say which root to start from
        db.push_back(rule{ep.atom("root"), {ep.atom("stunt0")}});
        db.push_back(rule{ep.atom("root"), {ep.atom("stuntX")}});

        // NEEDLE: add the rules which provide a faster route to a conflict
        db.push_back(rule{ep.atom("stuntX"), {ep.atom("stunt1")}});
        db.push_back(rule{ep.atom("stuntX"), {ep.atom("stuntY")}});
        db.push_back(rule{ep.atom("stuntY"), {ep.atom("stunt2")}});
        db.push_back(rule{ep.atom("stuntY"), {ep.atom("stuntZ")}});
        db.push_back(rule{ep.atom("stuntZ"), {ep.atom("stunt3")}});
        db.push_back(rule{ep.atom("stuntZ"), {ep.atom("stuntW")}});

        // HAYSTACK: add 2 of each to make fictitious decisions necessary (simulates a perfect binary tree)
        for (int i = 0; i < 2; ++i) {
            db.push_back(rule{ep.atom("stunt0"), {ep.atom("stunt1")}});
            db.push_back(rule{ep.atom("stunt1"), {ep.atom("stunt2")}});
            db.push_back(rule{ep.atom("stunt2"), {ep.atom("stunt3")}});
            db.push_back(rule{ep.atom("stunt3"), {ep.atom("stunt4")}});
            db.push_back(rule{ep.atom("stunt4"), {ep.atom("stunt5")}});
            db.push_back(rule{ep.atom("stunt5"), {ep.atom("stunt6")}});
            db.push_back(rule{ep.atom("stunt6"), {ep.atom("stunt7")}});
            db.push_back(rule{ep.atom("stunt7"), {ep.atom("stunt8")}});
            db.push_back(rule{ep.atom("stunt8"), {ep.atom("stunt9")}});
        }
        
        a01_goals goals;

        goals.push_back(ep.atom("root"));


        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        assert(result == true);
        assert(!soln.has_value());

        // CRITICAL: MCTS finds the depth-4 needle.
        assert(avoidance.size() == 4);

        // CRITICAL: The first decision in the avoidance is root→rule0 (the needle branch).
        const goal_lineage* gl_root = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl_root = solver.lp.resolution(gl_root, 1);
        assert(avoidance.count(rl_root) == 1);
        const goal_lineage* gl_stuntX = solver.lp.goal(rl_root, 0);
        const resolution_lineage* rl_stuntX = solver.lp.resolution(gl_stuntX, 3);
        assert(avoidance.count(rl_stuntX) == 1);
        const goal_lineage* gl_stuntY = solver.lp.goal(rl_stuntX, 0);
        const resolution_lineage* rl_stuntY = solver.lp.resolution(gl_stuntY, 5);
        assert(avoidance.count(rl_stuntY) == 1);
        const goal_lineage* gl_stuntZ = solver.lp.goal(rl_stuntY, 0);
        const resolution_lineage* rl_stuntZ = solver.lp.resolution(gl_stuntZ, 7);
        assert(avoidance.count(rl_stuntZ) == 1);
    }

    // Test 14: Depth-5 shared-variable needle in a depth-10 haystack.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;

        // say which root to start from
        db.push_back(rule{ep.atom("root"), {ep.atom("stunt0")}});
        db.push_back(rule{ep.atom("root"), {ep.atom("stuntX")}});

        // NEEDLE: add the rules which provide a faster route to a conflict
        db.push_back(rule{ep.atom("stuntX"), {ep.atom("stunt1")}});
        db.push_back(rule{ep.atom("stuntX"), {ep.atom("stuntY")}});
        db.push_back(rule{ep.atom("stuntY"), {ep.atom("stunt2")}});
        db.push_back(rule{ep.atom("stuntY"), {ep.atom("stuntZ")}});
        db.push_back(rule{ep.atom("stuntZ"), {ep.atom("stunt3")}});
        db.push_back(rule{ep.atom("stuntZ"), {ep.atom("stuntW")}});
        db.push_back(rule{ep.atom("stuntW"), {ep.atom("stunt4")}});
        db.push_back(rule{ep.atom("stuntW"), {ep.atom("stuntU")}});

        // HAYSTACK: add 2 of each to make fictitious decisions necessary (simulates a perfect binary tree)
        for (int i = 0; i < 2; ++i) {
            db.push_back(rule{ep.atom("stunt0"), {ep.atom("stunt1")}});
            db.push_back(rule{ep.atom("stunt1"), {ep.atom("stunt2")}});
            db.push_back(rule{ep.atom("stunt2"), {ep.atom("stunt3")}});
            db.push_back(rule{ep.atom("stunt3"), {ep.atom("stunt4")}});
            db.push_back(rule{ep.atom("stunt4"), {ep.atom("stunt5")}});
            db.push_back(rule{ep.atom("stunt5"), {ep.atom("stunt6")}});
            db.push_back(rule{ep.atom("stunt6"), {ep.atom("stunt7")}});
            db.push_back(rule{ep.atom("stunt7"), {ep.atom("stunt8")}});
            db.push_back(rule{ep.atom("stunt8"), {ep.atom("stunt9")}});
        }
        
        a01_goals goals;

        goals.push_back(ep.atom("root"));


        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        a01_decision_store avoidance;
        std::optional<a01_resolution_store> soln;

        bool result = solver.next_avoidance(avoidance, soln);

        assert(result == true);
        assert(!soln.has_value());

        // CRITICAL: MCTS finds the depth-4 needle.
        assert(avoidance.size() == 5);

        // CRITICAL: The first decision in the avoidance is root→rule0 (the needle branch).
        const goal_lineage* gl_root = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl_root = solver.lp.resolution(gl_root, 1);
        assert(avoidance.count(rl_root) == 1);
        const goal_lineage* gl_stuntX = solver.lp.goal(rl_root, 0);
        const resolution_lineage* rl_stuntX = solver.lp.resolution(gl_stuntX, 3);
        assert(avoidance.count(rl_stuntX) == 1);
        const goal_lineage* gl_stuntY = solver.lp.goal(rl_stuntX, 0);
        const resolution_lineage* rl_stuntY = solver.lp.resolution(gl_stuntY, 5);
        assert(avoidance.count(rl_stuntY) == 1);
        const goal_lineage* gl_stuntZ = solver.lp.goal(rl_stuntY, 0);
        const resolution_lineage* rl_stuntZ = solver.lp.resolution(gl_stuntZ, 7);
        assert(avoidance.count(rl_stuntZ) == 1);
        const goal_lineage* gl_stuntW = solver.lp.goal(rl_stuntZ, 0);
        const resolution_lineage* rl_stuntW = solver.lp.resolution(gl_stuntW, 9);
        assert(avoidance.count(rl_stuntW) == 1);
    }
}

void test_a01() {

    // Test 1: Budget = 0 — no iterations execute; operator() returns true with nullopt.
    // The solver makes no progress at all, so it cannot prove refutation or find a
    // solution. This validates the edge case of calling with zero budget.
    {
        trail t;
        t.push();
        expr_pool ep(t);
        bind_map bm(t);
        sequencer seq(t);

        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        std::optional<a01_resolution_store> soln;
        bool result = solver(0, soln);

        // CRITICAL: returns true (no refutation proved) and soln defaults to nullopt
        assert(result == true);
        assert(!soln.has_value());
        // The avoidance store is empty — no iterations ran, no avoidances recorded
        assert(solver.as.empty());
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

        a01_database db;
        db.push_back(rule{ep.atom("a"), {}});  // idx 0: a.

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        std::optional<a01_resolution_store> soln;
        bool result = solver(1, soln);

        assert(result == true);
        assert(soln.has_value());

        // CRITICAL: exactly one resolution (unit-prop of a with rule 0)
        assert(soln.value().size() == 1);
        const goal_lineage* gl0 = solver.lp.goal(nullptr, 0);
        const resolution_lineage* rl0 = solver.lp.resolution(gl0, 0);
        assert(soln.value().count(rl0) == 1);

        // CRITICAL: avoidance store has one entry — the empty decision set from the
        // unit-prop solution (no MCTS decision was needed)
        assert(solver.as.size() == 1);
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

        a01_database db;
        // idx 0: answer(42).
        db.push_back(rule{ep.cons(ep.atom("answer"), ep.atom("42")), {}});

        const expr* X = ep.var(seq());
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("answer"), X));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        std::optional<a01_resolution_store> soln;
        bool result = solver(1, soln);

        assert(result == true);
        assert(soln.has_value());

        // CRITICAL: one resolution, binding X to "42"
        assert(soln.value().size() == 1);
        assert(soln.value().count(solver.lp.resolution(solver.lp.goal(nullptr, 0), 0)) == 1);

        // CRITICAL: normalizer follows bm chain and returns atom("42")
        normalizer norm(ep, bm);
        const expr* X_val = norm(X);
        assert(std::holds_alternative<expr::atom>(X_val->content));
        assert(std::get<expr::atom>(X_val->content).value == "42");
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

        a01_database db;  // intentionally empty

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        std::optional<a01_resolution_store> soln;
        bool result = solver(1000, soln);

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

        a01_database db;
        db.push_back(rule{ep.atom("a"), {ep.atom("b")}});  // idx 0: a :- b.
        db.push_back(rule{ep.atom("a"), {ep.atom("c")}});  // idx 1: a :- c.

        a01_goals goals;
        goals.push_back(ep.atom("a"));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        std::optional<a01_resolution_store> soln;

        // Call 1: one depth-1 avoidance recorded
        bool result1 = solver(1, soln);
        assert(result1 == true);
        assert(!soln.has_value());
        assert(solver.as.size() == 1);

        // Call 2: CDCL + unit-prop leads to conflict with empty ds → refutation
        bool result2 = solver(1, soln);
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("is_a"), ep.atom("1")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("is_a"), ep.atom("2")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("is_b"), ep.atom("2")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.atom("is_b"), ep.atom("3")), {}});  // idx 3

        const expr* X = ep.var(seq());
        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("is_a"), X));  // goal 0: is_a(X)
        goals.push_back(ep.cons(ep.atom("is_b"), X));  // goal 1: is_b(X)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);
        std::optional<a01_resolution_store> soln;

        // Call 1: unique solution X=2
        bool result1 = solver(1000, soln);
        assert(result1 == true);
        assert(soln.has_value());

        // CRITICAL: bm binds X to "2"
        const expr* X_val = norm(X);
        assert(std::holds_alternative<expr::atom>(X_val->content));
        assert(std::get<expr::atom>(X_val->content).value == "2");

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
        bool result2 = solver(1000, soln);
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

        a01_database db;
        // parent(A, B) encoded as cons(cons(atom("parent"), A), B)
        db.push_back(rule{ep.cons(ep.cons(ep.atom("parent"), ep.atom("bob")),   ep.atom("alice")), {}});  // idx 0
        db.push_back(rule{ep.cons(ep.cons(ep.atom("parent"), ep.atom("carol")), ep.atom("alice")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.cons(ep.atom("parent"), ep.atom("dave")),  ep.atom("bob")),  {}});   // idx 2

        const expr* X = ep.var(seq());
        a01_goals goals;
        goals.push_back(ep.cons(ep.cons(ep.atom("parent"), X), ep.atom("alice")));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);
        std::optional<a01_resolution_store> soln;

        // First parent of alice
        bool result1 = solver(1000, soln);
        assert(result1 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        const expr* X_val1 = norm(X);
        assert(std::holds_alternative<expr::atom>(X_val1->content));
        std::string parent1 = std::get<expr::atom>(X_val1->content).value;
        assert(parent1 == "bob" || parent1 == "carol");

        // Second parent of alice — sim_one rolls back bm then unit-props the other rule
        bool result2 = solver(1000, soln);
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 1);

        const expr* X_val2 = norm(X);
        assert(std::holds_alternative<expr::atom>(X_val2->content));
        std::string parent2 = std::get<expr::atom>(X_val2->content).value;
        assert(parent2 == "bob" || parent2 == "carol");

        // CRITICAL: the two solutions bind X to different names
        assert(parent1 != parent2);

        // Test for refutation
        bool result3 = solver(1000, soln);
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("bool"), ep.atom("true")),  {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("bool"), ep.atom("false")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.cons(ep.atom("not"), ep.atom("true")),  ep.atom("false")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.cons(ep.atom("not"), ep.atom("false")), ep.atom("true")),  {}});  // idx 3
        db.push_back(rule{ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("true")),  ep.atom("true")),  ep.atom("true")),  {}});  // idx 4
        db.push_back(rule{ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("true")),  ep.atom("false")), ep.atom("true")),  {}});  // idx 5
        db.push_back(rule{ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("false")), ep.atom("true")),  ep.atom("true")),  {}});  // idx 6
        db.push_back(rule{ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("false")), ep.atom("false")), ep.atom("false")), {}});  // idx 7

        const expr* P  = ep.var(seq());
        const expr* Q  = ep.var(seq());
        const expr* NP = ep.var(seq());

        a01_goals goals;
        // goal 0: bool(P)
        goals.push_back(ep.cons(ep.atom("bool"), P));
        // goal 1: bool(Q)
        goals.push_back(ep.cons(ep.atom("bool"), Q));
        // goal 2: or(P, Q, true) — P ∨ Q = true
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("or"), P), Q), ep.atom("true")));
        // goal 3: not(P, NP) — compute ¬P
        goals.push_back(ep.cons(ep.cons(ep.atom("not"), P), NP));
        // goal 4: or(NP, Q, true) — ¬P ∨ Q = true
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("or"), NP), Q), ep.atom("true")));

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);
        std::optional<a01_resolution_store> soln;

        // First solution: P=true or P=false, but Q must be true
        bool result1 = solver(1000, soln);
        assert(result1 == true);
        assert(soln.has_value());

        const expr* Q_val1 = norm(Q);
        const expr* P_val1 = norm(P);
        assert(std::holds_alternative<expr::atom>(Q_val1->content));
        assert(std::holds_alternative<expr::atom>(P_val1->content));

        // CRITICAL: Q = true in every solution of (P∨Q)∧(¬P∨Q)
        assert(std::get<expr::atom>(Q_val1->content).value == "true");
        std::string P_str1 = std::get<expr::atom>(P_val1->content).value;
        assert(P_str1 == "true" || P_str1 == "false");

        // Second solution: CDCL eliminates first P choice; the other propagates
        bool result2 = solver(1000, soln);
        assert(result2 == true);
        assert(soln.has_value());

        const expr* Q_val2 = norm(Q);
        const expr* P_val2 = norm(P);
        assert(std::holds_alternative<expr::atom>(Q_val2->content));
        assert(std::holds_alternative<expr::atom>(P_val2->content));

        // CRITICAL: Q still true after finding the second solution
        assert(std::get<expr::atom>(Q_val2->content).value == "true");
        std::string P_str2 = std::get<expr::atom>(P_val2->content).value;

        // CRITICAL: the two solutions assign opposite values to P
        assert(P_str2 != P_str1);

        // Test for refutation
        bool result3 = solver(1000, soln);
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("red")),  {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("blue")), {}});  // idx 1
        // diff(X, Y) encoded as cons(cons(atom("diff"), X), Y)
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("red")),  ep.atom("blue")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("blue")), ep.atom("red")),  {}});  // idx 3

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("color"), A));                 // goal 0: color(A)
        goals.push_back(ep.cons(ep.atom("color"), B));                 // goal 1: color(B)
        goals.push_back(ep.cons(ep.atom("color"), C));                 // goal 2: color(C)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), A), B));      // goal 3: diff(A, B)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), B), C));      // goal 4: diff(B, C)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);
        std::optional<a01_resolution_store> soln;

        auto is_valid_color = [](const std::string& s) {
            return s == "red" || s == "blue";
        };

        // First valid 2-coloring of the path A-B-C
        bool result1 = solver(1000, soln);
        assert(result1 == true);
        assert(soln.has_value());

        // All 5 goals are resolved in every solution
        assert(soln.value().size() == 5);

        std::string A1 = std::get<expr::atom>(norm(A)->content).value;
        std::string B1 = std::get<expr::atom>(norm(B)->content).value;
        std::string C1 = std::get<expr::atom>(norm(C)->content).value;

        assert(is_valid_color(A1) && is_valid_color(B1) && is_valid_color(C1));
        // CRITICAL: adjacent nodes have different colors
        assert(A1 != B1);
        assert(B1 != C1);
        // CRITICAL: the path is symmetrically 2-colored — endpoints share a color
        assert(A1 == C1);

        // Second valid 2-coloring — bm is refreshed by the next sim_one's trail pop
        bool result2 = solver(1000, soln);
        assert(result2 == true);
        assert(soln.has_value());
        assert(soln.value().size() == 5);

        std::string A2 = std::get<expr::atom>(norm(A)->content).value;
        std::string B2 = std::get<expr::atom>(norm(B)->content).value;
        std::string C2 = std::get<expr::atom>(norm(C)->content).value;

        assert(is_valid_color(A2) && is_valid_color(B2) && is_valid_color(C2));
        assert(A2 != B2);
        assert(B2 != C2);
        assert(A2 == C2);

        // CRITICAL: the second coloring is the complement of the first
        assert(A2 != A1);

        // Test for refutation
        bool result3 = solver(1000, soln);
        assert(result3 == false);
    }

    // =========================================================================
    // Structured multi-solution enumeration helpers (Tests 10 and 11)
    // =========================================================================

    // solution = ordered vector of normalised const expr* values for the variables
    // of interest. Comparison uses pointer identity, which is correct because
    // expr_pool interns: same content → same pointer within the same pool.
    using solution = std::vector<const expr*>;

    // Enumerate all expected solutions in any order, skipping calls that return
    // the same variable bindings via a different resolution path (valid behaviour),
    // then assert that the solver refutes when the search space is exhausted.
    auto next_until_refuted = [](
        a01& solver,
        std::set<solution> expected,
        auto get_solution,
        size_t iterations = 1000
    ) {
        std::set<solution> visited;
        std::optional<a01_resolution_store> soln;
        while (!expected.empty()) {
            solution s;
            do {
                bool r = solver(iterations, soln);
                assert(r == true);
                s = get_solution();
            } while (visited.count(s));
            assert(expected.count(s) == 1);
            expected.erase(s);
            visited.insert(s);
        }
        // All solutions found — next call must refute
        bool r = solver(iterations, soln);
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("red")),   {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("green")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("blue")),  {}});  // idx 2
        // diff(X, Y) = cons(cons(atom("diff"), X), Y) — all 6 asymmetric pairs
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("red")),   ep.atom("green")), {}});  // idx 3
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("red")),   ep.atom("blue")),  {}});  // idx 4
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("green")), ep.atom("red")),   {}});  // idx 5
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("green")), ep.atom("blue")),  {}});  // idx 6
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("blue")),  ep.atom("red")),   {}});  // idx 7
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("blue")),  ep.atom("green")), {}});  // idx 8

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("color"), A));                 // goal 0: color(A)
        goals.push_back(ep.cons(ep.atom("color"), B));                 // goal 1: color(B)
        goals.push_back(ep.cons(ep.atom("color"), C));                 // goal 2: color(C)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), A), B));      // goal 3: diff(A,B)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), A), C));      // goal 4: diff(A,C)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), B), C));      // goal 5: diff(B,C)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);

        // Interned atoms from ep — same pointer as those embedded in the rules
        const expr* red   = ep.atom("red");
        const expr* green = ep.atom("green");
        const expr* blue  = ep.atom("blue");

        // Every permutation of {red, green, blue} assigned to {A, B, C}
        std::set<solution> expected = {
            {red,   green, blue },
            {red,   blue,  green},
            {green, red,   blue },
            {green, blue,  red  },
            {blue,  red,   green},
            {blue,  green, red  },
        };

        next_until_refuted(solver, expected, [&]() -> solution {
            return {norm(A), norm(B), norm(C)};
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("bool"), ep.atom("true")),  {}});  // idx 0: bool(true).
        db.push_back(rule{ep.cons(ep.atom("bool"), ep.atom("false")), {}});  // idx 1: bool(false).

        // or(true, X, true) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("true")), X), ep.atom("true")),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 2
        }
        // or(false, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("false")), X), X),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 3
        }
        // and(true, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("and"), ep.atom("true")), X), X),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 4
        }
        // and(false, X, false) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("and"), ep.atom("false")), X), ep.atom("false")),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 5
        }

        const expr* P  = ep.var(seq());
        const expr* Q  = ep.var(seq());
        const expr* R  = ep.var(seq());
        const expr* QR = ep.var(seq());

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("bool"), P));                                           // goal 0: bool(P)
        goals.push_back(ep.cons(ep.atom("bool"), Q));                                           // goal 1: bool(Q)
        goals.push_back(ep.cons(ep.atom("bool"), R));                                           // goal 2: bool(R)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("or"),  Q), R),  QR));                 // goal 3: or(Q,R,QR)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("and"), P), QR), ep.atom("true")));    // goal 4: and(P,QR,true)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);

        const expr* T_ = ep.atom("true");
        const expr* F_ = ep.atom("false");

        std::set<solution> expected = {
            {T_, T_, T_},   // P=T, Q=T, R=T
            {T_, T_, F_},   // P=T, Q=T, R=F
            {T_, F_, T_},   // P=T, Q=F, R=T
        };

        next_until_refuted(solver, expected, [&]() -> solution {
            return {norm(P), norm(Q), norm(R)};
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("red")),   {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("green")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.atom("color"), ep.atom("blue")),  {}});  // idx 2
        // All 6 ordered diff pairs among {red, green, blue}
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("red")),   ep.atom("green")), {}});  // idx 3
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("red")),   ep.atom("blue")),  {}});  // idx 4
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("green")), ep.atom("red")),   {}});  // idx 5
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("green")), ep.atom("blue")),  {}});  // idx 6
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("blue")),  ep.atom("red")),   {}});  // idx 7
        db.push_back(rule{ep.cons(ep.cons(ep.atom("diff"), ep.atom("blue")),  ep.atom("green")), {}});  // idx 8

        const expr* A = ep.var(seq());
        const expr* B = ep.var(seq());
        const expr* C = ep.var(seq());
        const expr* D = ep.var(seq());

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("color"), A));                 // goal 0: color(A)
        goals.push_back(ep.cons(ep.atom("color"), B));                 // goal 1: color(B)
        goals.push_back(ep.cons(ep.atom("color"), C));                 // goal 2: color(C)
        goals.push_back(ep.cons(ep.atom("color"), D));                 // goal 3: color(D)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), A), B));      // goal 4: diff(A,B)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), A), C));      // goal 5: diff(A,C)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), B), C));      // goal 6: diff(B,C)
        goals.push_back(ep.cons(ep.cons(ep.atom("diff"), A), D));      // goal 7: diff(A,D)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);

        const expr* R_ = ep.atom("red");
        const expr* G_ = ep.atom("green");
        const expr* B_ = ep.atom("blue");

        // 6 K3 colourings of (A,B,C), each with 2 choices for D (any colour ≠ A)
        std::set<solution> expected = {
            {R_, G_, B_, G_}, {R_, G_, B_, B_},
            {R_, B_, G_, G_}, {R_, B_, G_, B_},
            {G_, R_, B_, R_}, {G_, R_, B_, B_},
            {G_, B_, R_, R_}, {G_, B_, R_, B_},
            {B_, R_, G_, R_}, {B_, R_, G_, G_},
            {B_, G_, R_, R_}, {B_, G_, R_, G_},
        };

        next_until_refuted(solver, expected, [&]() -> solution {
            return {norm(A), norm(B), norm(C), norm(D)};
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

        a01_database db;
        db.push_back(rule{ep.cons(ep.atom("bool"), ep.atom("true")),  {}});  // idx 0
        db.push_back(rule{ep.cons(ep.atom("bool"), ep.atom("false")), {}});  // idx 1
        db.push_back(rule{ep.cons(ep.cons(ep.atom("not"), ep.atom("true")),  ep.atom("false")), {}});  // idx 2
        db.push_back(rule{ep.cons(ep.cons(ep.atom("not"), ep.atom("false")), ep.atom("true")),  {}});  // idx 3
        // or(true, X, true) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("true")), X), ep.atom("true")),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 4
        }
        // or(false, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("or"), ep.atom("false")), X), X),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 5
        }
        // and(true, X, X) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("and"), ep.atom("true")), X), X),
                {ep.cons(ep.atom("bool"), X)}
            });  // idx 6
        }
        // and(false, X, false) :- bool(X).
        {
            const expr* X = ep.var(seq());
            db.push_back(rule{
                ep.cons(ep.cons(ep.cons(ep.atom("and"), ep.atom("false")), X), ep.atom("false")),
                {ep.cons(ep.atom("bool"), X)}
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

        a01_goals goals;
        goals.push_back(ep.cons(ep.atom("bool"), P));                                               // goal  0: bool(P)
        goals.push_back(ep.cons(ep.atom("bool"), Q));                                               // goal  1: bool(Q)
        goals.push_back(ep.cons(ep.atom("bool"), R));                                               // goal  2: bool(R)
        goals.push_back(ep.cons(ep.atom("bool"), S));                                               // goal  3: bool(S)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("or"),  P),  Q),  PQ));                    // goal  4: or(P,Q,PQ)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("or"),  R),  S),  RS));                    // goal  5: or(R,S,RS)
        goals.push_back(ep.cons(ep.cons(ep.atom("not"), P), NP));                                   // goal  6: not(P,NP)
        goals.push_back(ep.cons(ep.cons(ep.atom("not"), R), NR));                                   // goal  7: not(R,NR)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("or"),  NP), NR), NPR));                   // goal  8: or(NP,NR,NPR)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("and"), PQ), RS),  PQ_RS));                // goal  9: and(PQ,RS,PQ_RS)
        goals.push_back(ep.cons(ep.cons(ep.cons(ep.atom("and"), PQ_RS), NPR), ep.atom("true")));   // goal 10: and(PQ_RS,NPR,true)

        std::mt19937 rng(42);
        a01 solver(db, goals, t, seq, bm, 1000, 1000, 1.414, rng);

        normalizer norm(ep, bm);

        const expr* T_ = ep.atom("true");
        const expr* F_ = ep.atom("false");

        // All 5 satisfying assignments to (P, Q, R, S)
        std::set<solution> expected = {
            {T_, T_, F_, T_},   // P=T, Q=T, R=F, S=T
            {T_, F_, F_, T_},   // P=T, Q=F, R=F, S=T
            {F_, T_, T_, T_},   // P=F, Q=T, R=T, S=T
            {F_, T_, T_, F_},   // P=F, Q=T, R=T, S=F
            {F_, T_, F_, T_},   // P=F, Q=T, R=F, S=T
        };

        next_until_refuted(solver, expected, [&]() -> solution {
            return {norm(P), norm(Q), norm(R), norm(S)};
        });
    }
}

void unit_test_main() {

    constexpr bool ENABLE_DEBUG_LOGS = true;

    // test cases
    TEST(test_trail_constructor);
    TEST(test_trail_push_pop);
    TEST(test_trail_log);
    TEST(test_atom_constructor);
    TEST(test_var_constructor);
    TEST(test_cons_constructor);
    TEST(test_expr_constructor);
    TEST(test_expr_pool_constructor);
    TEST(test_expr_pool_atom);
    TEST(test_expr_pool_var);
    TEST(test_expr_pool_cons);
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
    TEST(test_sequencer_constructor);
    TEST(test_sequencer);
    TEST(test_copier_constructor);
    TEST(test_copier);
    TEST(test_normalizer_constructor);
    TEST(test_normalizer);
    TEST(test_a01_goal_adder_constructor);
    TEST(test_a01_goal_adder);
    TEST(test_a01_goal_resolver_constructor);
    TEST(test_a01_goal_resolver);
    TEST(test_a01_head_elimination_detector_constructor);
    TEST(test_a01_head_elimination_detector);
    TEST(test_unit_propagation_detector_constructor);
    TEST(test_unit_propagation_detector);
    TEST(test_solution_detector_constructor);
    TEST(test_solution_detector);
    TEST(test_conflict_detector_constructor);
    TEST(test_conflict_detector);
    TEST(test_a01_cdcl_elimination_detector_constructor);
    TEST(test_a01_cdcl_elimination_detector);
    TEST(test_a01_decider_constructor);
    TEST(test_a01_decider_choose_goal);
    TEST(test_a01_decider_choose_candidate);
    TEST(test_a01_decider);
    TEST(test_a01_sim_constructor);
    TEST(test_a01_sim);
    TEST(test_a01_constructor_and_destructor);
    TEST(test_a01_sim_one);
    TEST(test_a01_next_avoidance);
    TEST(test_a01);
}

#ifdef DEBUG

int main() {
    unit_test_main();
    return 0;
}

#endif

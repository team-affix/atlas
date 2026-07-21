// pool_allocator: freelist acquire/release and emplace growth. Contract is storage
// ownership + pointer recycling; construction and clear stay with the caller.

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pool_allocator.hpp"

struct pooled_item {
    uint32_t id;
    auto operator<=>(const pooled_item&) const = default;
};

struct move_only_item {
    uint32_t id;
    move_only_item(uint32_t id);
    move_only_item(move_only_item&& other);
    move_only_item& operator=(move_only_item&& other);
    move_only_item(const move_only_item&) = delete;
    move_only_item& operator=(const move_only_item&) = delete;
};

move_only_item::move_only_item(uint32_t id) : id(id) {}
move_only_item::move_only_item(move_only_item&& other) : id(other.id) { other.id = 0; }
move_only_item& move_only_item::operator=(move_only_item&& other) {
    id = other.id;
    other.id = 0;
    return *this;
}

struct counted_item {
    static int live_count;
    uint32_t id;
    counted_item(uint32_t id);
    counted_item(counted_item&& other);
    counted_item& operator=(counted_item&& other);
    counted_item(const counted_item&) = delete;
    counted_item& operator=(const counted_item&) = delete;
    ~counted_item();
};

int counted_item::live_count = 0;

counted_item::counted_item(uint32_t id) : id(id) { ++live_count; }
counted_item::counted_item(counted_item&& other) : id(other.id) {
    ++live_count;
    other.id = 0;
}
counted_item& counted_item::operator=(counted_item&& other) {
    id = other.id;
    other.id = 0;
    return *this;
}
counted_item::~counted_item() { --live_count; }

struct PoolAllocatorTest : public ::testing::Test {
    pool_allocator<pooled_item> pool;
};

TEST_F(PoolAllocatorTest, AcquireOnEmptyReturnsNull) {
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, EmplaceReturnsLivePointerWithMovedValue) {
    pooled_item src{7};
    pooled_item* p = pool.emplace(std::move(src));
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->id, 7u);
}

TEST_F(PoolAllocatorTest, EmplaceDoesNotEnqueueOntoFreelist) {
    pool.emplace(pooled_item{1});
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, ReleaseThenAcquireReturnsSamePointer) {
    pooled_item* p = pool.emplace(pooled_item{3});
    pool.release(p);
    EXPECT_EQ(pool.acquire(), p);
}

TEST_F(PoolAllocatorTest, AcquireAfterReleaseLeavesFreelistEmptyAgain) {
    pooled_item* p = pool.emplace(pooled_item{4});
    pool.release(p);
    ASSERT_EQ(pool.acquire(), p);
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, ReleaseAcquireIsLifo) {
    pooled_item* a = pool.emplace(pooled_item{1});
    pooled_item* b = pool.emplace(pooled_item{2});
    pooled_item* c = pool.emplace(pooled_item{3});
    pool.release(a);
    pool.release(b);
    pool.release(c);
    EXPECT_EQ(pool.acquire(), c);
    EXPECT_EQ(pool.acquire(), b);
    EXPECT_EQ(pool.acquire(), a);
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, RepeatedEmplaceYieldsDistinctPointers) {
    std::vector<pooled_item*> ptrs;
    for (uint32_t i = 0; i < 32; ++i)
        ptrs.push_back(pool.emplace(pooled_item{i}));
    for (size_t i = 0; i < ptrs.size(); ++i) {
        ASSERT_NE(ptrs[i], nullptr);
        EXPECT_EQ(ptrs[i]->id, static_cast<uint32_t>(i));
        for (size_t j = i + 1; j < ptrs.size(); ++j)
            EXPECT_NE(ptrs[i], ptrs[j]);
    }
}

TEST_F(PoolAllocatorTest, EarlierPointersRemainValidAfterFurtherEmplaces) {
    pooled_item* first = pool.emplace(pooled_item{10});
    pooled_item* second = pool.emplace(pooled_item{20});
    for (uint32_t i = 0; i < 64; ++i)
        pool.emplace(pooled_item{100 + i});
    EXPECT_EQ(first->id, 10u);
    EXPECT_EQ(second->id, 20u);
    EXPECT_NE(first, second);
}

TEST_F(PoolAllocatorTest, InterleavedAcquireReleaseAndEmplace) {
    pooled_item* a = pool.emplace(pooled_item{1});
    pooled_item* b = pool.emplace(pooled_item{2});
    pool.release(a);
    pooled_item* recycled = pool.acquire();
    EXPECT_EQ(recycled, a);
    pooled_item* c = pool.emplace(pooled_item{3});
    EXPECT_NE(c, a);
    EXPECT_NE(c, b);
    pool.release(b);
    pool.release(c);
    EXPECT_EQ(pool.acquire(), c);
    EXPECT_EQ(pool.acquire(), b);
    EXPECT_EQ(pool.acquire(), nullptr);
    EXPECT_EQ(recycled->id, 1u);
}

TEST_F(PoolAllocatorTest, ReleasedEmplacedObjectCanBeReacquired) {
    pooled_item* p = pool.emplace(pooled_item{42});
    pool.release(p);
    pooled_item* again = pool.acquire();
    ASSERT_EQ(again, p);
    EXPECT_EQ(again->id, 42u);
}

TEST_F(PoolAllocatorTest, ReleaseNullThrowsInDebug) {
    EXPECT_THROW(pool.release(nullptr), std::logic_error);
}

TEST_F(PoolAllocatorTest, EmplacePreservesMovedInValue) {
    pooled_item src{99};
    pooled_item* p = pool.emplace(std::move(src));
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, pooled_item{99});
}

TEST_F(PoolAllocatorTest, IndependentAllocatorsDoNotShareFreelist) {
    pool_allocator<pooled_item> other;
    pooled_item* p = pool.emplace(pooled_item{1});
    pool.release(p);
    EXPECT_EQ(other.acquire(), nullptr);
    EXPECT_EQ(pool.acquire(), p);
}

TEST_F(PoolAllocatorTest, IndependentAllocatorsDoNotShareStorage) {
    pool_allocator<pooled_item> other;
    pooled_item* p0 = pool.emplace(pooled_item{1});
    pooled_item* p1 = other.emplace(pooled_item{2});
    EXPECT_NE(p0, p1);
    EXPECT_EQ(p0->id, 1u);
    EXPECT_EQ(p1->id, 2u);
    pool.release(p0);
    EXPECT_EQ(other.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, FreelistReuseDoesNotRequireNewEmplace) {
    pooled_item* p0 = pool.emplace(pooled_item{1});
    pooled_item* p1 = pool.emplace(pooled_item{2});
    pool.release(p0);
    pool.release(p1);
    pooled_item* r0 = pool.acquire();
    pooled_item* r1 = pool.acquire();
    EXPECT_EQ(r0, p1);
    EXPECT_EQ(r1, p0);
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, EmplaceAfterFullFreelistDrainGrowsAgain) {
    pooled_item* a = pool.emplace(pooled_item{1});
    pool.release(a);
    ASSERT_EQ(pool.acquire(), a);
    pooled_item* b = pool.emplace(pooled_item{2});
    EXPECT_NE(b, a);
    EXPECT_EQ(b->id, 2u);
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, CallerMutationsSurviveAcrossReleaseAcquire) {
    pooled_item* p = pool.emplace(pooled_item{0});
    p->id = 77;
    pool.release(p);
    pooled_item* again = pool.acquire();
    ASSERT_EQ(again, p);
    EXPECT_EQ(again->id, 77u);
}

TEST_F(PoolAllocatorTest, ManyReleaseAcquireCyclesStayStable) {
    pooled_item* p = pool.emplace(pooled_item{5});
    for (int i = 0; i < 100; ++i) {
        pool.release(p);
        pooled_item* again = pool.acquire();
        ASSERT_EQ(again, p);
        EXPECT_EQ(again->id, 5u);
    }
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST_F(PoolAllocatorTest, ReleaseMiddleLiveSlotLeavesOthersUnacquirable) {
    pooled_item* a = pool.emplace(pooled_item{1});
    pooled_item* b = pool.emplace(pooled_item{2});
    pooled_item* c = pool.emplace(pooled_item{3});
    pool.release(b);
    EXPECT_EQ(pool.acquire(), b);
    EXPECT_EQ(pool.acquire(), nullptr);
    EXPECT_EQ(a->id, 1u);
    EXPECT_EQ(c->id, 3u);
}

TEST_F(PoolAllocatorTest, StressAlternatingEmplaceReleaseAcquire) {
    std::vector<pooled_item*> live;
    for (uint32_t i = 0; i < 50; ++i) {
        live.push_back(pool.emplace(pooled_item{i}));
        if (i % 3 == 2) {
            pooled_item* recycled = live.back();
            live.pop_back();
            pool.release(recycled);
            pooled_item* again = pool.acquire();
            ASSERT_EQ(again, recycled);
            live.push_back(again);
        }
    }
    for (pooled_item* p : live)
        pool.release(p);
    for (size_t i = 0; i < live.size(); ++i)
        ASSERT_NE(pool.acquire(), nullptr);
    EXPECT_EQ(pool.acquire(), nullptr);
}

TEST(PoolAllocatorMoveOnlyTest, EmplaceAcquireReleaseWithMoveOnlyType) {
    pool_allocator<move_only_item> pool;
    move_only_item* p = pool.emplace(move_only_item{11});
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->id, 11u);
    EXPECT_EQ(pool.acquire(), nullptr);
    pool.release(p);
    move_only_item* again = pool.acquire();
    ASSERT_EQ(again, p);
    EXPECT_EQ(again->id, 11u);
}

TEST(PoolAllocatorLifetimeTest, DestroyingPoolDestroysAllStorageSlots) {
    counted_item::live_count = 0;
    {
        pool_allocator<counted_item> pool;
        counted_item* a = pool.emplace(counted_item{1});
        counted_item* b = pool.emplace(counted_item{2});
        counted_item* c = pool.emplace(counted_item{3});
        pool.release(a);
        pool.release(c);
        EXPECT_GT(counted_item::live_count, 0);
        (void)b;
    }
    EXPECT_EQ(counted_item::live_count, 0);
}

TEST(PoolAllocatorLifetimeTest, DestroyingPoolWithFullFreelistDestroysEverything) {
    counted_item::live_count = 0;
    {
        pool_allocator<counted_item> pool;
        for (uint32_t i = 0; i < 8; ++i) {
            counted_item* p = pool.emplace(counted_item{i});
            pool.release(p);
        }
        EXPECT_GT(counted_item::live_count, 0);
    }
    EXPECT_EQ(counted_item::live_count, 0);
}

TEST(PoolAllocatorLifetimeTest, DestroyingPoolWithNoReleasesDestroysEverything) {
    counted_item::live_count = 0;
    {
        pool_allocator<counted_item> pool;
        for (uint32_t i = 0; i < 5; ++i)
            pool.emplace(counted_item{i});
        EXPECT_EQ(pool.acquire(), nullptr);
        EXPECT_GT(counted_item::live_count, 0);
    }
    EXPECT_EQ(counted_item::live_count, 0);
}

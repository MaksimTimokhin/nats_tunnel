#include <gtest/gtest.h>
#include <tunnel/redirector/ConcurrentHashMap.hpp>
#include <random>
#include <unordered_set>

TEST(HashMap, Correctness) {
    ConcurrentHashMap<int, int> map;
    ASSERT_TRUE(map.Insert(3, 2));
    ASSERT_TRUE(map.Insert(2, 3));
    ASSERT_EQ(map.Get(3), 2);
    ASSERT_EQ(map.Get(2), 3);
    ASSERT_EQ(map.Get(1), std::nullopt);
    ASSERT_FALSE(map.Insert(3, 1));
    ASSERT_TRUE(map.Erase(3));
    ASSERT_TRUE(map.Insert(3, 1));
    ASSERT_EQ(map.Size(), 2);
    ASSERT_TRUE(map.Erase(2));
    ASSERT_EQ(map.Size(), 1);
    map.Clear();
    ASSERT_EQ(map.Size(), 0);
}

TEST(HashMap, Stripes) {
    int stripes_count = 10;
    int elements_count = 100000;
    std::unordered_set<int> keys;
    std::unordered_multiset<int> values;
    std::mt19937 gen(655212);
    std::uniform_int_distribution<int> dist(0, 5000000);
    ConcurrentHashMap<int, int> map(elements_count, 10);
    for (int i = 0; i < elements_count; ++i) {
        int key = dist(gen), value = dist(gen);
        keys.insert(key);
        if (map.Insert(key, value)) {
            values.insert(value);
        }
    }

    std::unordered_set<int> result_keys;
    std::unordered_multiset<int> result_values;
    for (int i = 0; i < stripes_count; ++i) {
        auto stripe_items = map.GetStripeItems(i);
        for (const auto& item : stripe_items) {
            result_keys.insert(item.first);
            result_values.insert(item.second);
        }
    }
    ASSERT_EQ(keys, result_keys);
    ASSERT_EQ(values, result_values);
    map.Clear();
    ASSERT_EQ(map.Size(), 0);
}
#pragma once

#include <atomic>
#include <cmath>
#include <list>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class ConcurrentHashMap {
private:
    using Item = std::pair<KeyType, ValueType>;
    using Bucket = std::list<Item>;

    constexpr static const int kUndefinedSize = -1;
    constexpr static const int kDefaultConcurrencyLevel = 8;
    constexpr static const double kMaxLoadFactor = 1.6;
    constexpr static const double kMinLoadFactor = 0.4;
    constexpr static const double kGrowthFactor = 2;

public:
    explicit ConcurrentHashMap(const Hash &hasher = Hash())
        : ConcurrentHashMap(kUndefinedSize, hasher) {
    }

    explicit ConcurrentHashMap(size_t expected_size, const Hash &hasher = Hash())
        : ConcurrentHashMap(expected_size, kDefaultConcurrencyLevel, hasher) {
    }

    ConcurrentHashMap(size_t expected_size, int expected_threads_count, const Hash &hasher = Hash())
        : hash_(hasher),
          concurrency_level_(expected_threads_count > 0 ? expected_threads_count
                                                        : kDefaultConcurrencyLevel),
          locks_(expected_threads_count),
          buckets_(expected_threads_count) {
        if (expected_size != kUndefinedSize) {
            buckets_.reserve(expected_size);
        }
    }

    size_t Size() const {
        return size_;
    }

    bool Insert(const KeyType &key, const ValueType &value) {
        size_t hash = hash_(key);
        auto guard = UniqueGuard(hash);
        Bucket &bucket = GetBucket(hash);
        if (std::find_if(bucket.begin(), bucket.end(),
                         [&key](const Item &item) { return item.first == key; }) != bucket.end()) {
            return false;
        }
        bucket.emplace_back(key, value);
        ++size_;

        if (MaxLoadFactorExceeded()) {
            size_t new_capacity = buckets_.size() * kGrowthFactor;
            guard.unlock();
            TryResizeTable(new_capacity, true);
        }
        return true;
    }

    bool Erase(const KeyType &key) {
        size_t hash = hash_(key);
        auto guard = UniqueGuard(hash);
        Bucket &bucket = GetBucket(hash);
        auto item_iter = std::find_if(bucket.begin(), bucket.end(),
                                      [&key](const Item &item) { return item.first == key; });
        if (item_iter == bucket.end()) {
            return false;
        }
        bucket.erase(item_iter);
        --size_;

        if (MinLoadFactorReached()) {
            size_t new_capacity = buckets_.size() / kGrowthFactor;
            guard.unlock();
            TryResizeTable(new_capacity, false);
        }
        return true;
    }

    void Clear() {
        auto guard = UniqueGuard(0);
        std::vector<std::unique_lock<std::shared_mutex>> guards(concurrency_level_ - 1);
        for (int index = 1; index < concurrency_level_; ++index) {
            guards[index - 1] = UniqueGuard(index);
        }
        for (auto &bucket : buckets_) {
            bucket.clear();
        }
        buckets_.resize(concurrency_level_);
        size_ = 0;
    }

    std::optional<ValueType> Get(const KeyType &key) const {
        size_t hash = hash_(key);
        auto guard = SharedGuard(hash);
        const Bucket &bucket = GetBucket(hash);
        auto item_iter = std::find_if(bucket.begin(), bucket.end(),
                                      [&key](const Item &item) { return item.first == key; });
        if (item_iter == bucket.end()) {
            return std::nullopt;
        }
        return item_iter->second;
    }

    std::vector<Item> GetStripeItems(size_t stripe_index) {
        auto guard = SharedGuard(stripe_index);
        std::vector<Item> stripe_items;
        stripe_items.reserve(std::llround(kMaxLoadFactor * buckets_.size() / concurrency_level_));
        for (size_t bucket_index = stripe_index; bucket_index < buckets_.size();
             bucket_index += concurrency_level_) {
            const Bucket &bucket = GetBucket(bucket_index);
            stripe_items.insert(stripe_items.end(), bucket.begin(), bucket.end());
        }
        return stripe_items;
    }

private:
    Hash hash_;
    const int concurrency_level_;
    mutable std::vector<std::shared_mutex> locks_;
    std::vector<Bucket> buckets_;
    std::atomic_size_t size_{0};

    size_t GetStripeIndex(const size_t hash) const {
        return hash % concurrency_level_;
    }

    size_t GetBucketIndex(const size_t hash) const {
        return hash % buckets_.size();
    }

    Bucket &GetBucket(const size_t hash) {
        return buckets_[GetBucketIndex(hash)];
    }

    const Bucket &GetBucket(const size_t hash) const {
        return buckets_[GetBucketIndex(hash)];
    }

    std::shared_lock<std::shared_mutex> SharedGuard(const size_t hash) const {
        return std::shared_lock(locks_[GetStripeIndex(hash)]);
    }

    std::unique_lock<std::shared_mutex> UniqueGuard(const size_t hash) const {
        return std::unique_lock(locks_[GetStripeIndex(hash)]);
    }

    bool MaxLoadFactorExceeded() const {
        return size_ > kMaxLoadFactor * buckets_.size();
    }

    bool MinLoadFactorReached() const {
        return size_ < kMinLoadFactor * buckets_.size();
    }

    void TryResizeTable(const size_t expected_buckets_count, bool expand) {
        auto guard = UniqueGuard(0);
        if ((expand && expected_buckets_count <= buckets_.size()) ||
            (!expand && expected_buckets_count >= buckets_.size())) {
            return;
        }

        std::vector<std::unique_lock<std::shared_mutex>> guards(concurrency_level_ - 1);
        for (int index = 1; index < concurrency_level_; ++index) {
            guards[index - 1] = UniqueGuard(index);
        }

        std::vector<Bucket> new_buckets(expected_buckets_count);
        for (Bucket &bucket : buckets_) {
            for (Item &item : bucket) {
                new_buckets[hash_(item.first) % expected_buckets_count].push_back(item);
            }
        }
        std::swap(buckets_, new_buckets);
    }
};

#pragma once

#include "utils/Compiler.h"
#include "utils/Assert.h"
#include "utils/containers/GrowthPolicy.h"

#include <cstdint>
#include <vector>

// implementation reference: https://github.com/Tessil/robin-map

namespace coust {
namespace container {
namespace detail {

using hash_type = uint16_t;

inline hash_type truncate(size_t origin) {
    return (hash_type) origin;
}

template <typename I>
concept is_pair = requires(I const& i) {
    { i.first };
    { i.second };
};

template <typename V>
class robin_bucket_entry {
public:
    robin_bucket_entry& operator=(robin_bucket_entry&&) = delete;
    robin_bucket_entry& operator=(robin_bucket_entry const&) = delete;

public:
    using value_type = V;
    using distance_type = uint8_t;
    static bool constexpr IS_PAIR = is_pair<value_type>;

public:
    robin_bucket_entry() noexcept : m_bytes() {}

    robin_bucket_entry(bool last) noexcept : m_last(last), m_bytes() {}

    robin_bucket_entry(robin_bucket_entry const& other) noexcept
        requires(std::copy_constructible<value_type>)
        : m_last(other.m_last), m_hash(other.m_hash), m_bytes() {
        if (!other.empty()) {
            std::construct_at(std::addressof(m_value), other.get_value());
            m_distance_from_home = other.m_distance_from_home;
        }
    }

    robin_bucket_entry(robin_bucket_entry&& other) noexcept
        requires(std::move_constructible<value_type>)
        : m_last(other.m_last), m_hash(other.m_hash), m_bytes() {
        if (!other.empty()) {
            std::construct_at(
                std::addressof(m_value), std::move(other.get_value()));
            m_distance_from_home = other.m_distance_from_home;
        }
    }

    ~robin_bucket_entry() { clear(); }

public:
    void set_last() noexcept { m_last = true; }

    template <typename... Args>
    void fill(distance_type dist, hash_type hash, Args&&... args) noexcept
        requires(std::constructible_from<value_type, Args...>)
    {
        COUST_ASSERT(dist >= 1, "Can't set the distance from home of a filled "
                                "bucket entry to empty");
        COUST_ASSERT(empty(), "Can't fill a filled bucket entry");
        std::construct_at(std::addressof(m_value), std::forward<Args>(args)...);
        m_distance_from_home = dist;
        m_hash = hash;
    }

    void swap(distance_type& dist, hash_type& hash, value_type& value) noexcept
        requires(std::swappable<value_type>)
    {
        COUST_ASSERT(!empty(), "Swap operation requried a filled bucket entry");
        std::swap(m_distance_from_home, dist);
        std::swap(m_hash, hash);
        std::swap(m_value, value);
    }

    void clear() noexcept {
        if (!empty()) {
            m_value.~value_type();
            m_distance_from_home = EMPTY_MARKER_DIST_FROM_HOME;
        }
    }

public:
    value_type& get_value() noexcept {
        COUST_ASSERT(!empty(), "Can't get value from an empty bucket entry");
        return *std::launder(std::addressof(m_value));
    }

    value_type const& get_value() const noexcept {
        COUST_ASSERT(!empty(), "Can't get value from an empty bucket entry");
        return *std::launder(std::addressof(m_value));
    }

    auto& get_key() noexcept {
        if constexpr (IS_PAIR) {
            return get_value().first;
        } else {
            return get_value();
        }
    }

    auto const& get_key() const noexcept {
        if constexpr (IS_PAIR) {
            return get_value().first;
        } else {
            return get_value();
        }
    }

    auto& get_mapped() noexcept
        requires(IS_PAIR)
    {
        return get_value().second;
    }

    auto const& get_mapped() const noexcept
        requires(IS_PAIR)
    {
        return get_value().second;
    }

    bool is_last() const noexcept { return m_last; }

    distance_type get_distance_from_home() const noexcept {
        return m_distance_from_home;
    }

    hash_type get_hash() const noexcept { return m_hash; }

    bool empty() const noexcept {
        return m_distance_from_home == EMPTY_MARKER_DIST_FROM_HOME;
    }

    // "home" is where a value should live in, which means the location
    // specified by its hash value

    // internal logic of the robin map is based on the concept of "richness" and
    // "poverty": the further an value locates from "home", the poorer it is.

    bool poorer_than_or_same_as(distance_type other_dist) const noexcept {
        return m_distance_from_home >= other_dist;
    }

    bool richer_than(distance_type other_dist) const noexcept {
        return m_distance_from_home < other_dist;
    }

    bool can_be_richer() const noexcept {
        return m_distance_from_home > IDEAL_DIST_FROM_HOME;
    }

public:
    static distance_type constexpr MAX_DIST_FROM_HOME = 32u;
    static distance_type constexpr IDEAL_DIST_FROM_HOME = 1u;

private:
    // here we stiuplate that if there's no value located in this entry, its
    // distance from home is zero. **so the smallest possible distance from
    // home is 1.**
    static distance_type constexpr EMPTY_MARKER_DIST_FROM_HOME = 0u;
    // packed, 4 bytes in total
    bool m_last = false;
    distance_type m_distance_from_home = EMPTY_MARKER_DIST_FROM_HOME;
    hash_type m_hash = 0u;
    union {
        alignas(value_type) char m_bytes[sizeof(value_type)];
        value_type m_value;
    };
};

template <typename Key, typename Mapped, typename Hash, typename Key_Equal,
    typename Alloc, detail::growth_policy Growth_Policy>
class robin_hash : private Hash,
                   private Key_Equal,
                   private Growth_Policy {
public:
    template <bool Constant>
    class robin_iterator;

    static bool constexpr HAS_MAPPED = !std::is_same_v<Mapped, void>;

    using key_type = Key;
    using mapped_type = Mapped;
    using value_type = std::conditional_t<HAS_MAPPED,
        std::pair<key_type, mapped_type>, key_type>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using hasher = Hash;
    using key_equal = Key_Equal;
    using allocator_type = Alloc;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = robin_iterator<false>;
    using const_iterator = robin_iterator<true>;

private:
    using bucket_entry = robin_bucket_entry<value_type>;
    using distance_type = typename bucket_entry::distance_type;
    // for consistency, we use std::allocator_traits here
    using bucket_allocator = typename std::allocator_traits<
        allocator_type>::template rebind_alloc<bucket_entry>;
    using bucket_container_type = std::vector<bucket_entry, bucket_allocator>;

public:
    template <bool Constant>
    class robin_iterator {
    private:
        friend class robin_hash;
        using bucket_entry_ptr = typename std::conditional_t<Constant,
            const bucket_entry*, bucket_entry*>;

        robin_iterator(bucket_entry_ptr bucket) noexcept : m_bucket(bucket) {}

    public:
        // it's a one direction iterator
        using itertor_category = std::forward_iterator_tag;
        using value_type = typename robin_hash::value_type const;
        using difference_type = ptrdiff_t;
        using reference = value_type&;
        using pointer = value_type*;

    public:
        robin_iterator() noexcept {}

        // construct from iterator (other) to const_iterator (this)
        robin_iterator(robin_iterator<false> const& other) noexcept
            requires(Constant)
            : m_bucket(other.m_bucket) {}

        robin_iterator(robin_iterator<false>&& other) noexcept
            requires(Constant)
            : m_bucket(other.m_bucket) {}

        robin_iterator(robin_iterator&& other) noexcept = default;
        robin_iterator(robin_iterator const& other) noexcept = default;
        robin_iterator& operator=(robin_iterator&& other) noexcept = default;
        robin_iterator& operator=(
            robin_iterator const& other) noexcept = default;

        // user isn't allowed to modify key
        typename robin_hash::key_type const& key() const noexcept {
            m_bucket->get_key();
        }

        auto const& mapped() const noexcept { return m_bucket->get_mapped(); }

        auto& mapped() const noexcept
            requires(!Constant)
        {
            return m_bucket->get_mapped();
        }

        reference operator*() const noexcept { return m_bucket->get_value(); }

        pointer operator->() const noexcept {
            return std::addressof(m_bucket->get_value());
        }

        WARNING_PUSH
        CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
        // proceed until reach last bucket location or find an filled bucket
        // location
        robin_iterator& operator++() {
            while (true) {
                if (m_bucket->is_last()) {
                    // consistent with the end of stl vector
                    // https://en.cppreference.com/w/cpp/container/vector/end
                    ++m_bucket;
                    return *this;
                }
                ++m_bucket;
                if (!m_bucket->empty())
                    return *this;
            }
        }
        WARNING_POP

        robin_iterator operator++(int) {
            robin_iterator tmp{*this};
            ++(*this);
            return tmp;
        }

        friend bool operator==(
            robin_iterator const& lhs, robin_iterator const& rhs) {
            return lhs.m_bucket == rhs.m_bucket;
        }

        friend bool operator!=(
            robin_iterator const& lhs, robin_iterator const& rhs) {
            return !(lhs == rhs);
        }

    private:
        bucket_entry_ptr m_bucket = nullptr;
    };

    // API reference:
    // https://en.cppreference.com/w/cpp/container/unordered_map
public:
    robin_hash(size_type bucket_count, hasher const& hash,
        key_equal const& equal, allocator_type const& allocator,
        float min_load_factor = MIN_LOAD_FACTOR_DEFAULT,
        float max_load_factor = MAX_LOAD_FACTOR_DEFAULT)
        : Hash(hash),
          Key_Equal(equal),
          Growth_Policy(bucket_count),
          m_buckets_container(bucket_count, allocator),
          m_buckets(m_buckets_container.empty() ? sentinel_bucket_ptr() :
                                                  m_buckets_container.data()),
          m_bucket_count(bucket_count),
          m_filled_bucket_count(0),
          m_grow_on_next_insert(false),
          m_try_shrink_on_next_insert(false) {
        COUST_ASSERT(bucket_count < max_bucket_count(),
            "the size of this robin map exceeds its limit");
        if (m_bucket_count > 0) {
            m_buckets_container.back().set_last();
        }

        this->min_load_factor(min_load_factor);
        this->max_load_factor(max_load_factor);
    }

    robin_hash(robin_hash const& other) noexcept
        : Hash(other),
          Key_Equal(other),
          Growth_Policy(other),
          m_buckets_container(other.m_buckets_container),
          m_buckets(m_buckets_container.empty() ? sentinel_bucket_ptr() :
                                                  m_buckets_container.data()),
          m_bucket_count(other.m_bucket_count),
          m_filled_bucket_count(other.m_filled_bucket_count),
          m_load_threshold(other.m_load_threshold),
          m_min_load_factor(other.m_min_load_factor),
          m_max_load_factor(other.m_max_load_factor),
          m_grow_on_next_insert(other.m_grow_on_next_insert),
          m_try_shrink_on_next_insert(other.m_try_shrink_on_next_insert) {}

    robin_hash(robin_hash&& other) noexcept
        : Hash(std::forward<Hash>(other)),
          Key_Equal(std::forward<Key_Equal>(other)),
          Growth_Policy(std::forward<Growth_Policy>(other)),
          m_buckets_container(std::move(other.m_buckets_container)),
          m_buckets(m_buckets_container.empty() ? sentinel_bucket_ptr() :
                                                  m_buckets_container.data()),
          m_bucket_count(other.m_bucket_count),
          m_filled_bucket_count(other.m_filled_bucket_count),
          m_load_threshold(other.m_load_threshold),
          m_min_load_factor(other.m_min_load_factor),
          m_max_load_factor(other.m_max_load_factor),
          m_grow_on_next_insert(other.m_grow_on_next_insert),
          m_try_shrink_on_next_insert(other.m_try_shrink_on_next_insert) {
        other.clear_and_shrink();
    }

    robin_hash& operator=(robin_hash const& other) {
        if (&other != this) {
            Hash::operator=(other);
            Key_Equal::operator=(other);
            Growth_Policy::operator=(other);
            m_buckets_container = other.m_buckets_container;
            m_buckets = m_buckets_container.empty() ?
                            sentinel_bucket_ptr() :
                            m_buckets_container.data();
            m_bucket_count = other.m_bucket_count;
            m_filled_bucket_count = other.m_filled_bucket_count;
            m_load_threshold = other.m_load_threshold;
            m_min_load_factor = other.m_min_load_factor;
            m_max_load_factor = other.m_max_load_factor;
            m_grow_on_next_insert = other.m_grow_on_next_insert;
            m_try_shrink_on_next_insert = other.m_try_shrink_on_next_insert;
        }
        return *this;
    }

    robin_hash& operator=(robin_hash&& other) {
        other.swap(*this);
        other.clear_and_shrink();
        return *this;
    }

    allocator_type get_allocator() const noexcept {
        return m_buckets_container.get_allocator();
    }

public:
    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
    /* Iterators */
    iterator begin() noexcept {
        size_t i = 0;
        for (; i < m_bucket_count && m_buckets[i].empty(); ++i) {}
        return iterator{m_buckets + i};
    }

    const_iterator begin() const noexcept { return cbegin(); }

    const_iterator cbegin() const noexcept {
        size_t i = 0;
        for (; i < m_bucket_count && m_buckets[i].empty(); ++i) {}
        return const_iterator{m_buckets + i};
    }

    iterator end() noexcept { return iterator{m_buckets + m_bucket_count}; }

    const_iterator end() const noexcept { return cend(); }

    const_iterator cend() const noexcept {
        return const_iterator{m_buckets + m_bucket_count};
    }
    /* Iterators */
    WARNING_POP

public:
    /* Capacity */
    bool empty() const noexcept { return m_filled_bucket_count == 0; }

    size_type size() const noexcept { return m_filled_bucket_count; }

    size_type max_size() const noexcept {
        return m_buckets_container.max_size();
    }
    /* Capacity */
public:
    /* Modifiers*/
    void clear() noexcept {
        if (m_min_load_factor > 0.0f) {
            clear_and_shrink();
        } else {
            std::ranges::for_each(
                m_buckets_container, [](auto& b) { b.clear(); });
            m_filled_bucket_count = 0;
            m_grow_on_next_insert = false;
        }
    }

    template <typename V>
    std::pair<iterator, bool> insert(V&& value) noexcept
        // stl unordered map allowed implicit constrution from V to value_type
        // I think that's ambiguous, so that overload is prohibited here
        requires(std::same_as<std::remove_cvref_t<V>, value_type>)
    {
        return insert_impl(extract_key(value), std::forward<V>(value));
    }

    template <typename V>
    iterator insert(const_iterator hint, V&& value) noexcept
        requires(std::same_as<std::remove_cvref_t<V>, value_type>)
    {
        if (hint != cend() &&
            compare_keys(extract_key(value), extract_key(*hint)))
            return mutable_cast(hint);
        return insert(std::forward<V>(value)).first;
    }

    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
    template <typename Iter>
    void insert(Iter first, Iter last) noexcept
        requires(requires(Iter l, Iter r) {
            { std::distance(l, r) };
            { ++l };
            { l != r } -> std::same_as<bool>;
            { *l };
        })
    {
        auto const insertion_count = std::distance(first, last);
        size_type const free_buckets_count =
            m_load_threshold - m_filled_bucket_count;
        if (size_type(insertion_count) > free_buckets_count)
            reserve(m_filled_bucket_count + size_type(insertion_count));
        for (auto iter = first; iter != last; ++iter) {
            insert(*iter);
        }
    }
    WARNING_POP

    template <typename K, typename M>
    std::pair<iterator, bool> insert_or_assign(K&& key, M&& mapped) noexcept
        requires(std::same_as<key_type, std::remove_cvref_t<K>> &&
                 std::is_assignable_v<mapped_type&, M &&>)
    {
        auto iter_success =
            try_emplace(std::forward<K>(key), std::forward<M>(mapped));
        if (!iter_success.second)
            iter_success.first.mapped() = std::forward<M>(mapped);
        return iter_success;
    }

    template <typename K, typename M>
    iterator insert_or_assign(const_iterator hint, K&& key, M&& mapped) noexcept
        requires(std::same_as<key_type, std::remove_cvref_t<K>> &&
                 std::is_assignable_v<mapped_type&, M &&>)
    {
        if (hint != cend() && compare_keys(key, extract_key(*hint))) {
            auto iter = mutable_cast(hint);
            iter.mapped() = std::forward<M>(mapped);
            return iter;
        }
        return insert_or_assign(std::forward<K>(key), std::forward<M>(mapped))
            .first;
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) noexcept
        requires(std::constructible_from<value_type, Args...>)
    {
        return insert(value_type{std::forward<Args>(args)...});
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args&&... args) noexcept
        requires(std::constructible_from<value_type, Args...>)
    {
        return insert(hint, value_type{std::forward<Args>(args)...});
    }

    template <typename K, typename... Args>
    std::pair<iterator, bool> try_emplace(
        K&& key, Args&&... mapped_args) noexcept
        requires(std::same_as<key_type, std::remove_cvref_t<K>> &&
                 std::constructible_from<mapped_type, Args...>)
    {
        return insert_impl(std::forward<K>(key), std::piecewise_construct,
            std::forward_as_tuple(std::forward<K>(key)),
            std::forward_as_tuple(std::forward<Args>(mapped_args)...));
    }

    template <typename K, typename... Args>
    iterator try_emplace(
        const_iterator hint, K&& key, Args&&... mapped_args) noexcept
        requires(std::same_as<key_type, std::remove_cvref_t<K>> &&
                 std::constructible_from<mapped_type, Args...>)
    {
        if (hint != cend() && compare_keys(key, extract_key(*hint))) {
            return mutable_cast(hint);
        }
        return try_emplace(
            std::forward<K>(key), std::forward<Args>(mapped_args)...)
            .first;
    }

    iterator erase(iterator pos) noexcept {
        erase_one_bucket_impl(pos);
        if (pos.m_bucket->empty()) {
            ++pos;
        }
        m_try_shrink_on_next_insert = true;
        return pos;
    }

    iterator erase(const_iterator pos) noexcept {
        return erase(mutable_cast(pos));
    }

    iterator erase(const_iterator first, const_iterator last) noexcept {
        return erase_ranges_impl(mutable_cast(first), mutable_cast(last));
    }

    iterator erase(iterator first, iterator last) noexcept {
        return erase_ranges_impl(first, last);
    }

    template <typename K>
    size_type erase(K const& key) noexcept {
        auto iter = find_impl(key, key_to_hash(key));
        if (iter != end()) {
            erase_one_bucket_impl(iter);
            m_try_shrink_on_next_insert = true;
            return 1u;
        } else {
            return 0u;
        }
    }

    void swap(robin_hash& other) noexcept {
        std::swap((Hash&) (*this), (Hash&) (other));
        std::swap((Key_Equal&) (*this), (Key_Equal&) (other));
        std::swap((Growth_Policy&) (*this), (Growth_Policy&) (other));
        std::swap(m_buckets_container, other.m_buckets_container);
        std::swap(m_buckets, other.m_buckets);
        std::swap(m_bucket_count, other.m_bucket_count);
        std::swap(m_filled_bucket_count, other.m_filled_bucket_count);
        std::swap(m_load_threshold, other.m_load_threshold);
        std::swap(m_min_load_factor, other.m_min_load_factor);
        std::swap(m_max_load_factor, other.m_max_load_factor);
        std::swap(m_grow_on_next_insert, other.m_grow_on_next_insert);
        std::swap(
            m_try_shrink_on_next_insert, other.m_try_shrink_on_next_insert);
    }
    /* Modifiers*/

public:
    /* Lookup*/
    template <typename K>
    auto& at(K const& key) noexcept {
        auto iter = find_impl(key, key_to_hash(key));
        COUST_PANIC_IF(iter == end(), "Can't find key");
        return iter.mapped();
    }

    template <typename K>
    auto const& at(K const& key) const noexcept {
        auto iter = find_impl(key, key_to_hash(key));
        COUST_PANIC_IF(iter == cend(), "Can't find key");
        return iter.mapped();
    }

    // the implicit behavior (default construction) of operator[] of stl
    // map/unordered_map is really ambiguous, I decided to not implement
    // that api here

    // count, equal_range are redundant for an hash map, so they are omitted

    template <typename K>
    iterator find(K const& key) noexcept {
        return find_impl(key, key_to_hash(key));
    }

    template <typename K>
    const_iterator find(K const& key) const noexcept {
        return find_impl(key, key_to_hash(key));
    }

    template <typename K>
    bool contains(K const& key) noexcept {
        return find_impl(key, key_to_hash(key)) != end();
    }

    template <typename K>
    bool contains(K const& key) const noexcept {
        return find_impl(key, key_to_hash(key)) != cend();
    }

    /* Lookup*/
public:
    /* Bucket Interface */
    size_type bucket_count() const noexcept { return m_bucket_count; }

    size_type max_bucket_count() const noexcept {
        return std::min(Growth_Policy::max(), m_buckets_container.max_size());
    }
    /* Bucket Interface */
public:
    /* Hash Policy */
    float load_factor() const noexcept {
        return m_bucket_count == 0 ?
                   0.0f :
                   float(m_filled_bucket_count) / float(m_bucket_count);
    }

    float min_load_factor() const noexcept { return m_min_load_factor; }

    float max_load_factor() const noexcept { return m_max_load_factor; }

    void min_load_factor(float factor) noexcept {
        m_min_load_factor = std::clamp(
            factor, MIN_LOAD_FACTOR_MINIMUM, MIN_LOAD_FACTOR_MAXIMUM);
    }

    void max_load_factor(float factor) noexcept {
        m_max_load_factor = std::clamp(
            factor, MAX_LOAD_FACTOR_MINIMUM, MAX_LOAD_FACTOR_MAXIMUM);
        m_load_threshold = size_type(float(bucket_count()) * m_max_load_factor);
    }

    void rehash(size_type new_count) noexcept {
        size_type const adjusted_count = std::max(new_count,
            (size_type) std::ceil(float(size()) / float(m_max_load_factor)));
        rehash_impl(adjusted_count);
    }

    void reserve(size_type new_capacity) noexcept {
        rehash(size_type(std::ceil(float(new_capacity) / m_max_load_factor)));
    }
    /* Hash Policy */
public:
    /* Observers */
    hasher hash_function() const noexcept { return (Hash const&) (*this); }

    key_equal key_eq() const noexcept { return (Key_Equal const&) (*this); }

    /* Observers */

private:
    size_t hash_to_index(hash_type hash) const noexcept {
        size_t const bucket_idx =
            Growth_Policy::hash_to_index((hash_type) hash);
        return bucket_idx;
    }

    size_t key_to_hash(key_type const& key) const noexcept {
        size_t ret = Hash::operator()(key);
        return ret;
    }

    bool compare_keys(key_type const& k1, key_type const& k2) const noexcept {
        return Key_Equal::operator()(k1, k2);
    }

    static key_type& extract_key(value_type& value) noexcept {
        if constexpr (HAS_MAPPED) {
            return value.first;
        } else {
            return value;
        }
    }

    static key_type const& extract_key(value_type const& value) noexcept {
        if constexpr (HAS_MAPPED) {
            return value.first;
        } else {
            return value;
        }
    }

    static iterator mutable_cast(const_iterator iter) noexcept {
        return iterator(const_cast<bucket_entry*>(iter.m_bucket));
    }

private:
    void clear_and_shrink() noexcept {
        Growth_Policy::clear();
        m_buckets_container.clear();
        m_buckets = sentinel_bucket_ptr();
        m_bucket_count = 0;
        m_filled_bucket_count = 0;
        m_load_threshold = 0;
        m_grow_on_next_insert = false;
        m_try_shrink_on_next_insert = false;
    }

    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
    void rehash_impl(size_type new_bucket_count) noexcept {
        robin_hash new_hash{new_bucket_count, (Hash&) (*this),
            (Key_Equal&) (*this), get_allocator(), m_min_load_factor,
            m_max_load_factor};
        auto const move_value_to = [this](auto& rh, size_t bucket_idx,
                                       distance_type dist_from_home,
                                       hash_type hash, value_type&& value) {
            auto& buckets_data = rh.m_buckets_container;
            while (true) {
                if (buckets_data[bucket_idx].richer_than(dist_from_home)) {
                    if (buckets_data[bucket_idx].empty()) {
                        buckets_data[bucket_idx].fill(
                            dist_from_home, hash, std::move(value));
                        return;
                    } else
                        buckets_data[bucket_idx].swap(
                            dist_from_home, hash, value);
                }
                ++dist_from_home;
                bucket_idx = Growth_Policy::next_idx(bucket_idx);
            }
        };
        for (auto& bucket : m_buckets_container) {
            if (bucket.empty())
                continue;
            hash_type const hash = bucket.get_hash();
            size_t const bucket_idx = new_hash.hash_to_index(hash);
            move_value_to(new_hash, bucket_idx,
                bucket_entry::IDEAL_DIST_FROM_HOME, hash,
                std::move(bucket.get_value()));
        }
        new_hash.m_bucket_count = new_hash.m_buckets_container.size();
        new_hash.m_filled_bucket_count = m_filled_bucket_count;
        new_hash.swap(*this);
    }

    template <typename K, typename... Args>
    std::pair<iterator, bool> insert_impl(
        K const& key, Args&&... value_args) noexcept
        requires(std::same_as<key_type, std::remove_cvref_t<K>>)
    {
        auto const insert_value_to_filled_bucket =
            [this](size_t bucket_idx, distance_type dist_from_home,
                hash_type hash, auto&& value) {
                COUST_ASSERT(
                    m_buckets[bucket_idx].richer_than(dist_from_home), "");
                m_buckets[bucket_idx].swap(dist_from_home, hash, value);
                bucket_idx = Growth_Policy::next_idx(bucket_idx);
                ++dist_from_home;
                while (!m_buckets[bucket_idx].empty()) {
                    if (m_buckets[bucket_idx].richer_than(dist_from_home)) {
                        m_grow_on_next_insert =
                            m_grow_on_next_insert ||
                            dist_from_home > bucket_entry::MAX_DIST_FROM_HOME;
                        m_buckets[bucket_idx].swap(dist_from_home, hash, value);
                    }
                    bucket_idx = Growth_Policy::next_idx(bucket_idx);
                    ++dist_from_home;
                }
                m_buckets[bucket_idx].fill(
                    dist_from_home, hash, std::move(value));
            };

        // the robin hash needs to rehash in the following scenarios:
        // 1. `m_grow_on_next_insert` is true (grow)
        // 2. the value we are gonna insert is too far from home (grow)
        // 3. current load factor exceeds maximum load factor (grow)
        // 4. `m_try_shrink_on_next_insert` is true and load factor is lower
        //    than minimum load factor (shrink)
        auto const grow_if_needed =
            [this](distance_type cur_dist_from_home) -> bool {
            bool const need_grow =
                m_grow_on_next_insert ||
                cur_dist_from_home > bucket_entry::MAX_DIST_FROM_HOME ||
                m_filled_bucket_count >= m_load_threshold;
            if (need_grow) {
                rehash_impl(Growth_Policy::grow());
            }
            m_grow_on_next_insert = false;
            return need_grow;
        };
        auto const shrink_if_needed = [this]() {
            bool const need_shrink = m_try_shrink_on_next_insert &&
                                     m_min_load_factor != 0.0f &&
                                     load_factor() < m_min_load_factor;
            if (need_shrink)
                reserve(m_filled_bucket_count + 2);
            m_try_shrink_on_next_insert = false;
            return need_shrink;
        };

        size_t const origin_hash = key_to_hash(key);
        hash_type const truncated_hash = truncate(origin_hash);
        size_t bucket_idx = hash_to_index(truncated_hash);
        distance_type dist_from_home = bucket_entry::IDEAL_DIST_FROM_HOME;
        // find bucket who's richer than us and get ready to "rob" it
        while (m_buckets[bucket_idx].poorer_than_or_same_as(dist_from_home)) {
            // if the bucket with same key already exists, return false
            if (m_buckets[bucket_idx].get_hash() == truncated_hash &&
                compare_keys(m_buckets[bucket_idx].get_key(), key)) {
                return std::make_pair(iterator(m_buckets + bucket_idx), false);
            }
            bucket_idx = Growth_Policy::next_idx(bucket_idx);
            ++dist_from_home;
        }
        // keep growing / shrinking if needed
        while (grow_if_needed(dist_from_home) || shrink_if_needed()) {
            // if the container changed, find another bucket to rob
            bucket_idx = hash_to_index(truncated_hash);
            dist_from_home = bucket_entry::IDEAL_DIST_FROM_HOME;
            while (
                m_buckets[bucket_idx].poorer_than_or_same_as(dist_from_home)) {
                bucket_idx = Growth_Policy::next_idx(bucket_idx);
                ++dist_from_home;
            }
        }
        if (m_buckets[bucket_idx].empty()) {
            m_buckets[bucket_idx].fill(dist_from_home, truncated_hash,
                std::forward<Args>(value_args)...);
        } else {
            value_type value{std::forward<Args>(value_args)...};
            insert_value_to_filled_bucket(
                bucket_idx, dist_from_home, truncated_hash, value);
        }
        ++m_filled_bucket_count;
        return std::make_pair(iterator(m_buckets + bucket_idx), true);
    }

    void erase_one_bucket_impl(iterator pos) noexcept {
        pos.m_bucket->clear();
        --m_filled_bucket_count;
        // try to move buckets closer to their home to make them overall richer
        for (size_t previous_idx = (size_t) (pos.m_bucket - m_buckets),
                    idx = Growth_Policy::next_idx(previous_idx);
             m_buckets[idx].can_be_richer();
             previous_idx = idx, idx = Growth_Policy::next_idx(idx)) {
            COUST_ASSERT(m_buckets[previous_idx].empty(), "");
            distance_type const new_dist =
                m_buckets[previous_idx].get_distance_from_home() - 1;
            m_buckets[previous_idx].fill(new_dist, m_buckets[idx].get_hash(),
                std::move(m_buckets[idx].get_value()));
            m_buckets[idx].clear();
        }
    }

    iterator erase_ranges_impl(iterator first, iterator last) {
        if (first == last) {
            return first;
        }

        m_try_shrink_on_next_insert = true;
        for (auto iter = first; iter != last; ++iter) {
            COUST_ASSERT(!iter.m_bucket->empty(), "");
            iter.m_bucket->clear();
            --m_filled_bucket_count;
        }
        // there're not filled buckets after cleared buckets
        if (last == cend()) {
            return end();
        }
        // there're still some filled buckets after cleared buckets,
        // shift them left
        auto const pick_destination = [this](size_t empty_buckets_begin,
                                          size_t bucket_moved_from) {
            size_t const dist_from_home =
                (size_t) m_buckets[bucket_moved_from].get_distance_from_home();
            size_t const moving_dist =
                std::min(bucket_moved_from - empty_buckets_begin,
                    dist_from_home - bucket_entry::IDEAL_DIST_FROM_HOME);
            size_t const new_bucket = bucket_moved_from - moving_dist;
            distance_type const new_dist_from_home =
                (distance_type) (dist_from_home - moving_dist);
            return std::make_pair(new_bucket, new_dist_from_home);
        };
        size_t empty_bucket_begin = (size_t) (first.m_bucket - m_buckets);
        size_t bucket_moved_from = (size_t) (last.m_bucket - m_buckets);
        size_t const return_bucket =
            pick_destination(empty_bucket_begin, bucket_moved_from).first;
        for (; bucket_moved_from < m_bucket_count &&
               m_buckets[bucket_moved_from].can_be_richer();
             ++bucket_moved_from) {
            COUST_ASSERT(m_buckets[empty_bucket_begin].empty(), "");
            auto const [new_bucket, new_dist_from_home] =
                pick_destination(empty_bucket_begin, bucket_moved_from);
            m_buckets[new_bucket].fill(new_dist_from_home,
                m_buckets[bucket_moved_from].get_hash(),
                std::move(m_buckets[bucket_moved_from].get_value()));
            m_buckets[bucket_moved_from].clear();
            empty_bucket_begin = new_bucket + 1;
        }
        return iterator(m_buckets + return_bucket);
    }

    template <typename K>
    iterator find_impl(K const& key, size_t hash) noexcept {
        return mutable_cast(((const robin_hash*) (this))->find_impl(key, hash));
    }

    template <typename K>
    const_iterator find_impl(K const& key, size_t hash) const noexcept {
        hash_type truncated_hash = truncate(hash);
        size_t bucket_idx = hash_to_index(truncated_hash);
        distance_type dist_from_home = bucket_entry::IDEAL_DIST_FROM_HOME;
        // our search begins from the home, so if the following buckets have
        // the same hash value, we should expect their `m_dist_from_home` be
        // not less than as our local var `dist_from_home`
        while (
            m_buckets[bucket_idx].get_distance_from_home() >= dist_from_home) {
            if (m_buckets[bucket_idx].get_hash() == truncated_hash &&
                compare_keys(m_buckets[bucket_idx].get_key(), key)) [[likely]]
                return const_iterator(m_buckets + bucket_idx);
            ++bucket_idx;
            ++dist_from_home;
        }
        return cend();
    }
    WARNING_POP

public:
    static size_type constexpr INIT_BUCKET_COUNT_DEFAULT = 0;

    static float constexpr MIN_LOAD_FACTOR_DEFAULT = 0.0f;
    static float constexpr MIN_LOAD_FACTOR_MINIMUM = 0.0f;
    static float constexpr MIN_LOAD_FACTOR_MAXIMUM = 0.15f;

    static float constexpr MAX_LOAD_FACTOR_DEFAULT = 0.5f;
    static float constexpr MAX_LOAD_FACTOR_MINIMUM = 0.2f;
    static float constexpr MAX_LOAD_FACTOR_MAXIMUM = 0.95f;

    static_assert(MIN_LOAD_FACTOR_DEFAULT < MAX_LOAD_FACTOR_DEFAULT, "");
    static_assert(MIN_LOAD_FACTOR_MINIMUM < MAX_LOAD_FACTOR_MINIMUM, "");
    static_assert(MIN_LOAD_FACTOR_MAXIMUM < MAX_LOAD_FACTOR_MAXIMUM, "");

private:
    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wexit-time-destructors")
    // a sentinel bucket pointed by `m_buckets` when there's no data in
    // `m_buckets_container`
    bucket_entry* sentinel_bucket_ptr() noexcept {
        static bucket_entry sentinel_bucket{true};
        return &sentinel_bucket;
    }
    WARNING_POP

private:
    bucket_container_type m_buckets_container;

    bucket_entry* RESTRICT m_buckets;

    size_type m_bucket_count;

    size_type m_filled_bucket_count;

    size_type m_load_threshold;

    // prevents container occupying too much memory
    float m_min_load_factor;
    // speeds up query and modification
    float m_max_load_factor;

    bool m_grow_on_next_insert;

    bool m_try_shrink_on_next_insert;
};

}  // namespace detail
}  // namespace container
}  // namespace coust

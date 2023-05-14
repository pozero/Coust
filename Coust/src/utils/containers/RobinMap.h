#pragma once

#include "utils/containers/RobinHash.h"

// implementation reference: https://github.com/Tessil/robin-map

namespace coust {
namespace container {

template <typename Key, typename T, typename Hash = std::hash<Key>,
    typename Key_Equal = std::equal_to<Key>,
    typename Alloc = std::allocator<std::pair<Key, T>>,
    detail::growth_policy Growth_Policy = detail::power_of_two_growth<2>>
class robin_map {
public:
    using rh =
        detail::robin_hash<Key, T, Hash, Key_Equal, Alloc, Growth_Policy>;

    using key_type = typename rh::key_type;
    using mapped_type = typename rh::mapped_type;
    using value_type = typename rh::value_type;
    using size_type = typename rh::size_type;
    using difference_type = typename rh::difference_type;
    using hasher = typename rh::hasher;
    using key_equal = typename rh::key_equal;
    using allocator_type = typename rh::allocator_type;
    using reference = typename rh::reference;
    using const_reference = typename rh::const_reference;
    using pointer = typename rh::pointer;
    using const_pointer = typename rh::const_pointer;
    using iterator = typename rh::iterator;
    using const_iterator = typename rh::const_iterator;

    // https://en.cppreference.com/w/cpp/container/unordered_map
public:
    /* Constructors */
    robin_map() noexcept : robin_map(rh::INIT_BUCKET_COUNT_DEFAULT) {}

    explicit robin_map(size_type bucket_count, Hash const& hash = Hash{},
        Key_Equal const& equal = Key_Equal{},
        Alloc const& alloc = Alloc{}) noexcept
        : m_rh(bucket_count, hash, equal, alloc) {}

    robin_map(size_type bucket_count, Alloc const& alloc) noexcept
        : robin_map(bucket_count, Hash{}, Key_Equal{}, alloc) {}

    robin_map(
        size_type bucket_count, Hash const& hash, Alloc const& alloc) noexcept
        : robin_map(bucket_count, hash, Key_Equal{}, alloc) {}

    explicit robin_map(Alloc const& alloc) noexcept
        : robin_map(rh::INIT_BUCKET_COUNT_DEFAULT, Hash{}, Key_Equal{}, alloc) {
    }

    template <typename Iter>
    robin_map(Iter first, Iter last,
        size_type bucket_count = rh::INIT_BUCKET_COUNT_DEFAULT,
        Hash const& hash = Hash{}, Key_Equal const& equal = Key_Equal{},
        Alloc const& alloc = Alloc{}) noexcept
        : robin_map(bucket_count, hash, equal, alloc) {
        m_rh.insert(first, last);
    }

    template <typename Iter>
    robin_map(Iter first, Iter last, size_type bucket_count,
        Alloc const& alloc) noexcept
        : robin_map(bucket_count, Hash{}, Key_Equal{}, alloc) {
        m_rh.insert(first, last);
    }

    template <typename Iter>
    robin_map(Iter first, Iter last, size_type bucket_count, Hash const& hash,
        Alloc const& alloc) noexcept
        : robin_map(bucket_count, hash, Key_Equal{}, alloc) {
        m_rh.insert(first, last);
    }

    robin_map(robin_map const& other) noexcept : m_rh(other.m_rh) {}

    robin_map(robin_map&& other) noexcept : m_rh(std::move(other.m_rh)) {}

    robin_map(std::initializer_list<value_type> init,
        size_type bucket_count = rh::INIT_BUCKET_COUNT_DEFAULT,
        Hash const& hash = Hash{}, Key_Equal const& equal = Key_Equal{},
        Alloc const& alloc = Alloc{}) noexcept
        : robin_map(bucket_count, hash, equal, alloc) {
        m_rh.insert(init.begin(), init.end());
    }

    robin_map(std::initializer_list<value_type> init, size_type bucket_count,
        Alloc const& alloc) noexcept
        : robin_map(bucket_count, Hash{}, Key_Equal{}, alloc) {
        m_rh.insert(init.begin(), init.end());
    }

    robin_map(std::initializer_list<value_type> init, size_type bucket_count,
        Hash const& hash, Alloc const& alloc) noexcept
        : robin_map(bucket_count, hash, Key_Equal{}, alloc) {
        m_rh.insert(init.begin(), init.end());
    }
    /* Constructors */
public:
    /* operator = */
    robin_map& operator=(robin_map const& other) noexcept {
        m_rh = other.m_rh;
        return *this;
    }

    robin_map& operator=(robin_map&& other) noexcept {
        m_rh = std::move(other.m_rh);
        return *this;
    }

    robin_map& operator=(std::initializer_list<value_type> list) noexcept {
        m_rh.clear();
        m_rh.reserve(list.size());
        m_rh.insert(list.begin(), list.end());
        return *this;
    }
    /* operator = */

public:
    allocator_type get_allocator() const noexcept {
        return m_rh.get_allocator();
    }

public:
    /* Iterators */
    iterator begin() noexcept { return m_rh.begin(); }

    const_iterator begin() const noexcept { return m_rh.begin(); }

    const_iterator cbegin() const noexcept { return m_rh.cbegin(); }

    iterator end() noexcept { return m_rh.end(); }

    const_iterator end() const noexcept { return m_rh.end(); }

    const_iterator cend() const noexcept { return m_rh.cend(); }
    /* Iterators */

public:
    /* capacity */
    bool empty() const noexcept { return m_rh.empty(); }

    size_type size() const noexcept { return m_rh.size(); }

    size_type max_size() const noexcept { return m_rh.max_size(); }
    /* capacity */
public:
    /* modifier */
    void clear() noexcept { m_rh.clear(); }

    std::pair<iterator, bool> insert(value_type const& value) noexcept {
        return m_rh.insert(value);
    }

    std::pair<iterator, bool> insert(value_type&& value) noexcept {
        return m_rh.insert(std::move(value));
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& value) noexcept
        requires(std::is_constructible_v<value_type, P &&>)
    {
        return m_rh.emplace(std::forward<P>(value));
    }

    iterator insert(const_iterator hint, value_type const& value) noexcept {
        return m_rh.insert(hint, value);
    }

    iterator insert(const_iterator hint, value_type&& value) noexcept {
        return m_rh.insert(hint, std::move(value));
    }

    template <typename P>
    iterator insert(const_iterator hint, P&& value) noexcept
        requires(std::is_constructible_v<value_type, P &&>)
    {
        return m_rh.emplace_hint(hint, std::forward<P>(value));
    }

    template <typename Iter>
    void insert(Iter first, Iter last) noexcept {
        m_rh.insert(first, last);
    }

    void insert(std::initializer_list<value_type> list) noexcept {
        m_rh.insert(list.begin(), list.end());
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(
        key_type const& key, M&& obj) noexcept {
        return m_rh.insert_or_assign(key, std::forward<M>(obj));
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(
        key_type&& key, M&& obj) noexcept {
        return m_rh.insert_or_assign(std::move(key), std::forward<M>(obj));
    }

    template <typename M>
    iterator insert_or_assign(
        const_iterator hint, key_type const& key, M&& obj) noexcept {
        return m_rh.insert_or_assign(hint, key, std::forward<M>(obj));
    }

    template <typename M>
    iterator insert_or_assign(
        const_iterator hint, key_type&& key, M&& obj) noexcept {
        return m_rh.insert_or_assign(
            hint, std::move(key), std::forward<M>(obj));
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) noexcept {
        return m_rh.emplace(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args&&... args) noexcept {
        return m_rh.emplace_hint(hint, std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(
        key_type const& key, Args&&... args) noexcept {
        return m_rh.try_emplace(key, std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(
        key_type&& key, Args&&... args) noexcept {
        return m_rh.try_emplace(std::move(key), std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator try_emplace(
        const_iterator hint, key_type const& key, Args&&... args) noexcept {
        return m_rh.try_emplace(hint, key, std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator try_emplace(
        const_iterator hint, key_type&& key, Args&&... args) noexcept {
        return m_rh.try_emplace(
            hint, std::move(key), std::forward<Args>(args)...);
    }

    iterator erase(iterator pos) noexcept { return m_rh.erase(pos); }

    iterator erase(const_iterator pos) noexcept { return m_rh.erase(pos); }

    iterator erase(const_iterator first, const_iterator last) noexcept {
        return m_rh.erase(first, last);
    }

    size_type erase(key_type const& key) noexcept { return m_rh.erase(key); }

    void swap(robin_map& other) noexcept { other.m_rh.swap(m_rh); }
    /* modifier */

public:
    /* lookup */
    T& at(key_type const& key) noexcept { return m_rh.at(key); }

    T const& at(key_type const& key) const noexcept { return m_rh.at(key); }

    iterator find(key_type const& key) noexcept { return m_rh.find(key); }

    const_iterator find(key_type const& key) const noexcept {
        return m_rh.find(key);
    }

    bool contains(key_type const& key) const noexcept {
        return m_rh.contains(key);
    }
    /* lookup */

public:
    /* bucket interface */
    size_type bucket_count() const noexcept { return m_rh.bucket_count(); }

    size_type max_bucket_count() const noexcept {
        return m_rh.max_bucket_count();
    }
    /* bucket interface */

public:
    /* hash policy */
    float load_factor() const noexcept { return m_rh.load_factor(); }

    float min_load_factor() const noexcept { return m_rh.min_load_factor(); }
    float max_load_factor() const noexcept { return m_rh.max_load_factor(); }

    void min_load_factor(float ml) const noexcept { m_rh.min_load_factor(ml); }
    void max_load_factor(float ml) const noexcept { m_rh.max_load_factor(ml); }

    void rehash(size_type new_count) noexcept { m_rh.rehash(new_count); }

    void reserve(size_type new_count) noexcept { m_rh.reserve(new_count); }
    /* hash policy */

public:
    /* Observers */
    Hash hash_function() const noexcept { return m_rh.hash_function(); }

    Key_Equal key_eq() const noexcept { return m_rh.key_eq(); }
    /* Observers */

private:
    rh m_rh;
};

}  // namespace container
}  // namespace coust

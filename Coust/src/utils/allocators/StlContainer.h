#pragma once

#include "utils/allocators/StlAdaptor.h"

#include <scoped_allocator>

#include <vector>
#include <string>
#include <deque>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace coust {
namespace memory {

// collection of template aliasings to save typing

template <typename T, detail::Allocator Alloc>
using vector = std::vector<T, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using vector_nested =
    std::vector<T, std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

template <detail::Allocator Alloc>
using string =
    std::basic_string<char, std::char_traits<char>, StdAllocator<char, Alloc>>;

template <typename T, detail::Allocator Alloc>
using deque = std::deque<T, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using deque_nested =
    std::deque<T, std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using map = std::map<Key, Val, std::less<Key>,
    StdAllocator<std::pair<Key const, Val>, Alloc>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using map_nested = std::map<Key, Val, std::less<Key>,
    std::scoped_allocator_adaptor<
        StdAllocator<std::pair<Key const, Val>, Alloc>>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using multimap = std::multimap<Key, Val, std::less<Key>,
    StdAllocator<std::pair<Key const, Val>, Alloc>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using multimap_nested = std::multimap<Key, Val, std::less<Key>,
    std::scoped_allocator_adaptor<
        StdAllocator<std::pair<Key const, Val>, Alloc>>>;

template <typename T, detail::Allocator Alloc>
using set = std::set<T, std::less<T>, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using set_nested = std::set<T, std::less<T>,
    std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

template <typename T, detail::Allocator Alloc>
using multiset = std::multiset<T, std::less<T>, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using multiset_nested = std::multiset<T, std::less<T>,
    std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

template <typename T, detail::Allocator Alloc>
using stack = std::stack<T, deque<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using stack_nested = std::stack<T, deque_nested<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using queue = std::queue<T, deque<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using queue_nested = std::queue<T, deque_nested<T, Alloc>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using unordered_map = std::unordered_map<Key, Val, std::less<Key>,
    StdAllocator<std::pair<Key const, Val>, Alloc>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using unordered_map_nested = std::unordered_map<Key, Val, std::less<Key>,
    std::scoped_allocator_adaptor<
        StdAllocator<std::pair<Key const, Val>, Alloc>>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using unordered_multimap = std::unordered_multimap<Key, Val, std::less<Key>,
    StdAllocator<std::pair<Key const, Val>, Alloc>>;

template <typename Key, typename Val, detail::Allocator Alloc>
using unordered_multimap_nested =
    std::unordered_multimap<Key, Val, std::less<Key>,
        std::scoped_allocator_adaptor<
            StdAllocator<std::pair<Key const, Val>, Alloc>>>;

template <typename T, detail::Allocator Alloc>
using unordered_set =
    std::unordered_set<T, std::less<T>, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using unordered_set_nested = std::unordered_set<T, std::less<T>,
    std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

template <typename T, detail::Allocator Alloc>
using unordered_multiset =
    std::unordered_multiset<T, std::less<T>, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using unordered_multiset_nested = std::unordered_multiset<T, std::less<T>,
    std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

template <typename T, detail::Allocator Alloc>
using list = std::list<T, StdAllocator<T, Alloc>>;

template <typename T, detail::Allocator Alloc>
using list_nested =
    std::list<T, std::scoped_allocator_adaptor<StdAllocator<T, Alloc>>>;

}  // namespace memory
}  // namespace coust

#pragma once

#include <utility>
#include <concepts>

namespace Coust
{
    namespace Hash
    {

        inline uint32_t murmur3(const uint32_t* key, std::size_t numBytes, uint32_t seed) noexcept
        {
            uint32_t h = seed;
            size_t i = numBytes;

            do {
                uint32_t k = *key++;
                k *= 0xcc9e2d51u;
                k = (k << 15u) | (k >> 17u);
                k *= 0x1b873593u;
                h ^= k;
                h = (h << 13u) | (h >> 19u);
                h = (h * 5u) + 0xe6546b64u;
            } while (--i);

            h ^= numBytes;
            h ^= h >> 16u;
            h *= 0x85ebca6bu;
            h ^= h >> 13u;
            h *= 0xc2b2ae35u;
            h ^= h >> 16u;
            return h;
        }
        
        // Murmur3 algorithm requires key struct with size of multiple of 4. 
        // And we also need to ensure the key comes in is a POD.
        template <typename T>
        concept Murmur3Hashable = (sizeof(T) & 3u) == 0 && std::is_pod<T>::value;
        
        template <typename T>
        concept StdHashable = requires (T a)
        {
            { std::hash<T>{}(a) } -> std::convertible_to<size_t>;
        };

        /**
         * @brief General hash function, it uses std::hash* if implemented. Otherwise it uses murmur3 hash function.
         * @tparam T 
         */
        template<typename T>
        struct HashFn
        {
            size_t operator()(const T& key) const noexcept
                requires StdHashable<T>
            {
                return std::hash<T>{}(key);
            }

            size_t operator()(const T& key) const noexcept
                requires Murmur3Hashable<T>
            {
                return (size_t) murmur3((const uint32_t*) &key, sizeof(T) / 4u, 0);
            }
        };

        template <typename T>
        concept Comparable = requires(const T& a, const T& b)
        {
            { a == b } -> std::same_as<bool>;
        };

        /**
         * @brief General comparison function, using operator==() if it's implemented.
         * @tparam T 
         */
        template<typename T>
        struct EqualFn 
        {
            bool operator()(const T& lhs, const T& rhs) const noexcept
                requires Comparable<T>
            {
                return lhs == rhs;
            }
            
            bool operator()(const T& lhs, const T& rhs) const noexcept
                requires Murmur3Hashable<T>
            {
                auto l = (const uint32_t*) &lhs;
                auto r = (const uint32_t*) &rhs;
                for (size_t i = 0; i < sizeof(T) / 4u; ++i, ++l, ++r)
                {
                    if (*l != *r)
                        return false;
                }
                return true;
            }
            
        };

        template <typename T>
        inline void Combine(size_t& seed, const T& value)
            requires StdHashable<T> || Murmur3Hashable<T>
        {
            HashFn<T> h;
            size_t hash = h(value);
            // From glm
            hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		    seed ^= hash;
        }
    }
}
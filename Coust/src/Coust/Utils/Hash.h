#pragma once

#include <unordered_map>
#include <utility>

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

        template<typename T>
        struct HahsFn
        {
            size_t operator()(const T& key) const noexcept
            {
                static_assert((sizeof(T) & 3u) == 0, "Hash requires key struct with size of multiple of 4");
                return (size_t) murmur3((const uint32_t*) &key, sizeof(T) / 4u, 0);
            }
        };

        template<typename T>
        struct EqualFn
        {
            bool operator()(const T& lhs, const T& rhs) const noexcept
            {
                static_assert((sizeof(T) & 3u) == 0, "Hash requires key struct with size of multiple of 4");
                
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
    }
}
#include "pch.h"

#include "test/Test.h"

#include "utils/containers/RobinHash.h"
#include "utils/containers/RobinSet.h"
#include "utils/containers/RobinMap.h"

TEST_CASE(
    "[Coust] [utils] [containers] RobinHash Entry" * doctest::skip(true)) {
    struct Obj {
    public:
        Obj() = delete;

        Obj(int* pcc, int* pdc) : construct_cnt(pcc), destruct_cnt(pdc) {
            (*construct_cnt)++;
        }

        Obj(Obj const& other)
            : construct_cnt(other.construct_cnt),
              destruct_cnt(other.destruct_cnt) {
            (*construct_cnt)++;
        }

        ~Obj() { (*destruct_cnt)++; }

    private:
        int* construct_cnt;
        int* destruct_cnt;
    };
    using namespace coust::container;

    size_t what_so_ever = 1u;
    detail::power_of_two_growth<2> what_ever{what_so_ever};

    SUBCASE("Empty entry") {
        int cc = 0, dc = 0;
        detail::robin_bucket_entry<Obj> e0{false};
        detail::robin_bucket_entry<Obj> e1{};
        CHECK(e0.empty());
        CHECK(e1.empty());
        e0.fill(1, 0, &cc, &dc);
        e1.fill(1, 0, &cc, &dc);
        CHECK(!e0.empty());
        CHECK(!e1.empty());
        CHECK(e0.poorer_than_or_same_as(1));
        CHECK(e0.poorer_than_or_same_as(0));
        CHECK(e0.richer_than(2));
        CHECK(e0.richer_than(3));
        CHECK(cc == 2);
        CHECK(dc == 0);
        e0.clear();
        e1.clear();
        CHECK(e0.empty());
        CHECK(e1.empty());
        CHECK(cc == 2);
        CHECK(dc == 2);
    }

    SUBCASE("Filled entry") {
        std::string s0{"Some texts"};
        std::string s1{"Some other texts"};
        detail::robin_bucket_entry<std::string> e0{};
        detail::robin_bucket_entry<std::string> e1{};
        e0.fill(1, 0, s0);
        e1.fill(1, 0, s1);
        CHECK(e0.get_value() == s0);
        CHECK(e1.get_value() == s1);
        decltype(e0)::distance_type d0 = 1, d1 = 1;
        detail::hash_type h0 = 0, h1 = 0;
        e0.swap(d0, h0, s1);
        e1.swap(d1, h1, s0);
        CHECK(e0.get_value() == s0);
        CHECK(e1.get_value() == s1);
    }
}

TEST_CASE("[Coust] [utils] [containers] RobinHash" * doctest::skip(true)) {
    using namespace coust;
    using test_key = std::string;
    using test_val = std::string;
    using test_hasher = std::hash<test_key>;
    using test_key_equal = std::equal_to<test_key>;
    using test_allocator = std::allocator<std::pair<test_key, test_val>>;
    using test_growth = container::detail::power_of_two_growth<2>;
    using test_hash = container::detail::robin_hash<test_key, test_val,
        test_hasher, test_key_equal, test_allocator, test_growth>;

    SUBCASE("Constructor and Iterator") {
        test_hash h0{0, test_hasher{}, test_key_equal{}, test_allocator{}};
        CHECK(h0.begin() == h0.end());
        CHECK(h0.cbegin() == h0.cend());
        CHECK(h0.bucket_count() == 0);
        test_hash h1{12, test_hasher{}, test_key_equal{}, test_allocator{}};
        CHECK(h1.begin() == h1.end());
        CHECK(h1.cbegin() == h1.cend());
        CHECK(h1.bucket_count() == 16);

        h0.swap(h1);
        CHECK(h0.begin() == h0.end());
        CHECK(h0.cbegin() == h0.cend());
        CHECK(h0.bucket_count() == 16);
        CHECK(h1.begin() == h1.end());
        CHECK(h1.cbegin() == h1.cend());
        CHECK(h1.bucket_count() == 0);
    }

    SUBCASE("Insertion") {
        SUBCASE("Simple insertion") {
            std::string const k0{"Key 0"};
            std::string const k1{"Key 1"};
            std::string const v0{"Val 0"};
            test_hash h0{0, test_hasher{}, test_key_equal{}, test_allocator{}};
            {
                auto [iter, success] = h0.insert(std::make_pair(k0, v0));
                CHECK(success);
                auto [pfirst, psecond] = *iter;
                CHECK(pfirst == k0);
                CHECK(psecond == v0);
                CHECK(!h0.empty());
                CHECK(h0.size() == 1);
                CHECK(h0.bucket_count() >= 2);
            }
            // insert again with the same key
            {
                auto [iter, success] = h0.insert(std::make_pair(k0, k0));
                CHECK(!success);
                auto [pfirst, psecond] = *iter;
                CHECK(pfirst == k0);
                CHECK(psecond == v0);
                CHECK(h0.size() == 1);
                CHECK(h0.bucket_count() >= 2);
            }
            {
                auto iter = h0.cbegin();
                auto [pfirst, psecond] = *iter;
                CHECK(pfirst == k0);
                CHECK(psecond == v0);
                ++iter;
                CHECK(iter == h0.cend());
            }
            {
                auto [iter, success] = h0.insert(std::make_pair(k1, v0));
                CHECK(success);
                auto [pfirst, psecond] = *iter;
                CHECK(pfirst == k1);
                CHECK(psecond == v0);
                CHECK(h0.size() == 2);
                CHECK(h0.bucket_count() >= 4);
            }
            // insert or assign with the same key
            {
                auto [iter, success] = h0.insert_or_assign(k0, k0);
                CHECK(!success);
                auto [pfirst, psecond] = *iter;
                CHECK(pfirst == k0);
                CHECK(psecond == k0);
            }
            std::string_view constexpr sv0{"sv0"};
            std::string_view constexpr sv1{"sv1"};
            {
                auto [iter, success] = h0.emplace(sv0, sv1);
                CHECK(success);
                auto [pfirst, psecond] = *iter;
                CHECK(pfirst == std::string{sv0});
                CHECK(psecond == std::string{sv1});
            }
            {
                auto [iter, success] = h0.try_emplace(std::string{sv0}, sv1);
                CHECK(!success);
            }
        }
        SUBCASE("Bulky Insertion") {
            size_t constexpr exp_cnt = 100;
            test_hash h0{0, test_hasher{}, test_key_equal{}, test_allocator{}};
            for (size_t i = 0; i < exp_cnt; ++i) {
                std::string const key{std::to_string(i)};
                std::string const mapped{std::to_string(i * 2)};
                auto [iter, success] = h0.insert(std::make_pair(key, mapped));
                CHECK(success);
                auto [first, second] = *iter;
                CHECK(first == key);
                CHECK(second == mapped);
            }
            CHECK(h0.size() == exp_cnt);
            CHECK(h0.bucket_count() >= 2 * exp_cnt);
            auto iter = h0.begin();
            for (size_t i = 0; i < exp_cnt; ++i, ++iter) {}
            CHECK(iter == h0.end());
        }
        SUBCASE("Range Insertion") {
            size_t constexpr exp_cnt = 100;
            std::array data = std::invoke([]() {
                std::array<std::pair<size_t, size_t>, exp_cnt> ret{};
                for (size_t i = 0; i < exp_cnt; ++i) {
                    ret[i] = std::make_pair(i, i * 2);
                }
                return ret;
            });
            container::detail::robin_hash<size_t, size_t, std::hash<size_t>,
                std::equal_to<size_t>,
                std::allocator<std::pair<size_t, size_t>>,
                container::detail::power_of_two_growth<2>>
                h0{0, std::hash<size_t>{}, std::equal_to<size_t>{},
                    std::allocator<std::pair<size_t, size_t>>{}};
            h0.insert(data.begin(), data.end());
            CHECK(h0.size() == data.size());
            for (auto iter = h0.cbegin(); iter != h0.cend(); ++iter) {
                CHECK(iter->first * 2 == iter->second);
            }
        }
    }

    SUBCASE("Rehash") {
        size_t constexpr exp_cnt = 10;
        test_hash h0{0, test_hasher{}, test_key_equal{}, test_allocator{}};
        for (size_t i = 0; i < exp_cnt; ++i) {
            std::string const key{std::to_string(i)};
            std::string const mapped{std::to_string(i * 2)};
            auto [iter, success] = h0.insert(std::make_pair(key, mapped));
            CHECK(success);
        }
        SUBCASE("Inflation") {
            h0.rehash(exp_cnt * 10);
            CHECK(h0.size() == exp_cnt);
            CHECK(h0.bucket_count() >= exp_cnt * 10);
            auto iter = h0.begin();
            for (size_t i = 0; i < exp_cnt; ++i, ++iter) {}
            CHECK(iter == h0.end());
        }
        SUBCASE("Shrinking") {
            h0.rehash(0);
            CHECK(h0.size() == exp_cnt);
            CHECK(h0.bucket_count() >= exp_cnt);
            auto iter = h0.begin();
            for (size_t i = 0; i < exp_cnt; ++i, ++iter) {}
            CHECK(iter == h0.end());
        }
    }

    SUBCASE("Erase") {
        size_t constexpr exp_cnt = 20;
        test_hash h0{0, test_hasher{}, test_key_equal{}, test_allocator{}};
        std::vector<std::pair<test_key, test_val>> h0_contents{};
        for (size_t i = 0; i < exp_cnt; ++i) {
            std::string const key{std::to_string(i)};
            std::string const mapped{std::to_string(i * 2)};
            auto [iter, success] = h0.insert(std::make_pair(key, mapped));
            CHECK(success);
            h0_contents.push_back(*iter);
        }
        SUBCASE("Single Element") {
            std::array constexpr element_to_erase_idx = {2, 4, 6, 8, 10};
            SUBCASE("Erase by iterator") {
                for (auto idx : element_to_erase_idx) {
                    std::string const key{std::to_string(idx)};
                    std::string const mapped{std::to_string(idx * 2)};
                    auto const iter_to_erase = std::invoke([&h0, &key]() {
                        for (auto iter = h0.begin(); iter != h0.end(); ++iter) {
                            auto const& [k, m] = *iter;
                            if (k == key) {
                                return iter;
                            }
                        }
                        ASSUME(0);
                    });
                    std::erase_if(h0_contents, [&key, &mapped](auto const& p) {
                        return p.first == key && p.second == mapped;
                    });
                    auto iter_after_erase = h0.erase(iter_to_erase);
                    CHECK(h0.contains(iter_after_erase.key()));
                    CHECK(h0.size() == h0_contents.size());
                    for (auto iter = h0.begin(); iter != h0.end(); ++iter) {
                        CHECK(std::ranges::any_of(
                            h0_contents, [&iter](auto const& p) {
                                return p.first == iter->first &&
                                       p.second == iter->second;
                            }));
                    }
                }
            }
            SUBCASE("Erase by key") {
                for (auto idx : element_to_erase_idx) {
                    std::string const key{std::to_string(idx)};
                    std::string const mapped{std::to_string(idx * 2)};
                    std::erase_if(h0_contents, [&key, &mapped](auto const& p) {
                        return p.first == key && p.second == mapped;
                    });
                    h0.erase(key);
                    CHECK(h0.size() == h0_contents.size());
                    for (auto iter = h0.begin(); iter != h0.end(); ++iter) {
                        CHECK(std::ranges::any_of(
                            h0_contents, [&iter](auto const& p) {
                                return p.first == iter->first &&
                                       p.second == iter->second;
                            }));
                    }
                }
            }
        }
        SUBCASE("Elements in range") {
            size_t constexpr begin_idx = 10;
            size_t constexpr end_idx = 16;
            auto const advance = [](auto iter, size_t idx) {
                for (; idx > 0; --idx, ++iter) {}
                return iter;
            };
            auto const range_begin = advance(h0.cbegin(), begin_idx);
            auto const range_end = advance(h0.cbegin(), end_idx);
            for (auto iter = range_begin; iter != range_end; ++iter) {
                std::erase_if(h0_contents, [iter](auto const& p) {
                    return p.first == iter->first && p.second == iter->second;
                });
            }
            auto iter_after_erase = h0.erase(range_begin, range_end);
            CHECK(iter_after_erase->first == range_end->first);
            CHECK(iter_after_erase->second == range_end->second);
            CHECK(h0.size() == h0_contents.size());
            for (auto iter = h0.begin(); iter != h0.end(); ++iter) {
                CHECK(std::ranges::any_of(h0_contents, [&iter](auto const& p) {
                    return p.first == iter->first && p.second == iter->second;
                }));
            }
        }
        SUBCASE("Erase by key, iterate by iterator") {
            std::vector<std::string> element_to_erase{
                std::to_string(1), std::to_string(3), std::to_string(5)};
            for (auto iter = h0.begin(); iter != h0.end();) {
                if (std::ranges::contains(element_to_erase, iter.key())) {
                    iter = h0.erase(iter);
                } else {
                    ++iter;
                }
            }
            CHECK(h0.size() == exp_cnt - element_to_erase.size());
            for (auto const& [key, mapped] : h0) {
                CHECK(!std::ranges::contains(element_to_erase, key));
            }
        }
    }

    SUBCASE("Find") {
        size_t constexpr exp_cnt = 20;
        test_hash h0{0, test_hasher{}, test_key_equal{}, test_allocator{}};
        for (size_t i = 0; i < exp_cnt; ++i) {
            std::string const key{std::to_string(i)};
            std::string const mapped{std::to_string(i * 2)};
            auto [iter, success] = h0.insert(std::make_pair(key, mapped));
            CHECK(success);
        }
        for (size_t i = 0; i < exp_cnt; ++i) {
            std::string const key{std::to_string(i)};
            std::string const mapped{std::to_string(i * 2)};
            auto& m = h0.at(key);
            CHECK(m == mapped);
        }
        for (size_t i = 0; i < exp_cnt; ++i) {
            std::string const key{std::to_string(i)};
            std::string const mapped{std::to_string(i * 2)};
            auto iter = h0.find(key);
            auto [k, m] = *iter;
            CHECK(k == key);
            CHECK(m == mapped);
        }
        for (size_t i = 0; i < exp_cnt; ++i) {
            std::string const key{std::to_string(i)};
            std::string const mapped{std::to_string(i * 2)};
            CHECK(h0.contains(key));
        }
    }
}

TEST_CASE("[Coust] [utils] [containers] Robin Set" * doctest::skip(true)) {
    SUBCASE("Bulky Insertion") {
        coust::container::robin_set<std::string> s0{};
        uint32_t constexpr exp_cnt = 100;
        for (uint32_t i = 0; i < exp_cnt; ++i) {
            s0.insert(std::to_string(i));
        }
        CHECK(s0.size() == 100);
        for (uint32_t i = 8; i < exp_cnt; ++i) {
            CHECK(s0.contains(std::to_string(i)));
        }
    }

    SUBCASE("Construction ans Assignment") {
        coust::container::robin_set<uint32_t> s0 = {1u, 2u, 3u, 3u, 2u, 1u, 0u};
        CHECK(s0.size() == 4);
        CHECK(s0.contains(0u));
        CHECK(s0.contains(1u));
        CHECK(s0.contains(2u));
        CHECK(s0.contains(3u));

        coust::container::robin_set<uint32_t> s1 = s0;
        CHECK(s1.size() == 4);
        CHECK(s1.contains(0u));
        CHECK(s1.contains(1u));
        CHECK(s1.contains(2u));
        CHECK(s1.contains(3u));

        coust::container::robin_set<uint32_t> s2 = std::move(s1);
        CHECK(s2.size() == 4);
        CHECK(s2.contains(0u));
        CHECK(s2.contains(1u));
        CHECK(s2.contains(2u));
        CHECK(s2.contains(3u));

        coust::container::robin_set<uint32_t> s3{s2};
        CHECK(s3.size() == 4);
        CHECK(s3.contains(0u));
        CHECK(s3.contains(1u));
        CHECK(s3.contains(2u));
        CHECK(s3.contains(3u));

        coust::container::robin_set<uint32_t> s4{std::move(s3)};
        CHECK(s4.size() == 4);
        CHECK(s4.contains(0u));
        CHECK(s4.contains(1u));
        CHECK(s4.contains(2u));
        CHECK(s4.contains(3u));

        std::array<uint32_t, 7> arr{1u, 2u, 3u, 3u, 2u, 1u, 0u};
        coust::container::robin_set<uint32_t> s5{arr.begin(), arr.end()};
        CHECK(s5.size() == 4);
        CHECK(s5.contains(0u));
        CHECK(s5.contains(1u));
        CHECK(s5.contains(2u));
        CHECK(s5.contains(3u));

        coust::container::robin_set<uint32_t> s6{1u, 2u, 3u, 3u, 2u, 1u, 0u};
        CHECK(s6.size() == 4);
        CHECK(s6.contains(0u));
        CHECK(s6.contains(1u));
        CHECK(s6.contains(2u));
        CHECK(s6.contains(3u));
        s6.clear();
        CHECK(s6.empty());
    }

    SUBCASE("Iterators") {
        coust::container::robin_set<uint32_t> s0{};
        CHECK(s0.begin() == s0.end());
        CHECK(s0.cbegin() == s0.cend());
        CHECK(s0.empty());

        coust::container::robin_set<uint32_t> s1{0u, 2u, 4u, 8u};
        auto iter = s1.begin();
        for (size_t i = 0; i < s1.size(); ++i) {
            iter++;
        }
        CHECK(iter == s1.end());
    }

    SUBCASE("Modifier") {
        coust::container::robin_set<std::string> s0{};
        {
            std::string_view constexpr str{"One"};
            auto [iter, success] = s0.insert(std::string{str});
            CHECK(success);
            CHECK(*iter == std::string{str});
        }
        {
            std::string_view constexpr str{"Two"};
            auto iter = s0.insert(s0.cbegin(), std::string{str});
            CHECK(*iter == std::string{str});
        }
        {
            std::array str_arr{std::string{"Three"}, std::string{"Four"}};
            s0.insert(str_arr.begin(), str_arr.end());
            CHECK(s0.size() == 4);
            CHECK(s0.contains(str_arr[0]));
            CHECK(s0.contains(str_arr[1]));
        }
        {
            s0.insert({std::string{"Five"}, std::string{"Six"}});
            CHECK(s0.size() == 6);
            CHECK(s0.contains(std::string{"Five"}));
            CHECK(s0.contains(std::string{"Six"}));
        }
        {
            auto [iter, success] = s0.emplace(std::string_view{"Seven"});
            CHECK(success);
            CHECK(*iter == std::string{"Seven"});
            CHECK(s0.size() == 7);
            CHECK(s0.contains(std::string{"Seven"}));
        }
        {
            auto ec0 = s0.erase(std::string{"One"});
            CHECK(ec0 == 1);
            CHECK(!s0.contains(std::string{"One"}));
            auto ec1 = s0.erase(std::string{"One"});
            CHECK(ec1 == 0);
            auto ec2 = s0.erase(std::string{"Two"});
            CHECK(ec2 == 1);
            CHECK(!s0.contains(std::string{"Two"}));
            auto iter = s0.begin();
            auto str_to_erase = *iter;
            s0.erase(iter);
            CHECK(!s0.contains(str_to_erase));
        }
    }

    SUBCASE("LookUp") {
        coust::container::robin_set<std::string> s0{"One", "Two", "Three"};
        {
            auto iter = s0.find(std::string{"One"});
            CHECK(iter != s0.end());
            CHECK(*iter == std::string{"One"});
        }
        {
            auto iter = s0.find(std::string{"Two"});
            CHECK(iter != s0.end());
            CHECK(*iter == std::string{"Two"});
        }
        {
            auto iter = s0.find(std::string{"Three"});
            CHECK(iter != s0.end());
            CHECK(*iter == std::string{"Three"});
        }
    }
}

TEST_CASE("[Coust] [utils] [containers] Robin Map" * doctest::skip(true)) {
    SUBCASE("Construction ans Assignment") {
        coust::container::robin_map<std::string, uint32_t> s0 = {
            std::make_pair(std::string{"One"}, 1u),
            std::make_pair(std::string{"Two"}, 2u),
            std::make_pair(std::string{"Three"}, 3u),
            std::make_pair(std::string{"Four"}, 4u),
            std::make_pair(std::string{"One"}, 1u),
        };
        CHECK(s0.size() == 4);
        CHECK(s0.at(std::string{"One"}) == 1u);
        CHECK(s0.at(std::string{"Two"}) == 2u);
        CHECK(s0.at(std::string{"Three"}) == 3u);
        CHECK(s0.at(std::string{"Four"}) == 4u);

        coust::container::robin_map<std::string, uint32_t> s1 = s0;
        CHECK(s1.size() == 4);
        CHECK(s1.at(std::string{"One"}) == 1u);
        CHECK(s1.at(std::string{"Two"}) == 2u);
        CHECK(s1.at(std::string{"Three"}) == 3u);
        CHECK(s1.at(std::string{"Four"}) == 4u);

        coust::container::robin_map<std::string, uint32_t> s2 = std::move(s1);
        CHECK(s2.size() == 4);
        CHECK(s2.at(std::string{"One"}) == 1u);
        CHECK(s2.at(std::string{"Two"}) == 2u);
        CHECK(s2.at(std::string{"Three"}) == 3u);
        CHECK(s2.at(std::string{"Four"}) == 4u);

        coust::container::robin_map<std::string, uint32_t> s3{s2};
        CHECK(s3.size() == 4);
        CHECK(s3.at(std::string{"One"}) == 1u);
        CHECK(s3.at(std::string{"Two"}) == 2u);
        CHECK(s3.at(std::string{"Three"}) == 3u);
        CHECK(s3.at(std::string{"Four"}) == 4u);

        coust::container::robin_map<std::string, uint32_t> s4{std::move(s3)};
        CHECK(s4.size() == 4);
        CHECK(s4.at(std::string{"One"}) == 1u);
        CHECK(s4.at(std::string{"Two"}) == 2u);
        CHECK(s4.at(std::string{"Three"}) == 3u);
        CHECK(s4.at(std::string{"Four"}) == 4u);

        std::array<std::pair<std::string, uint32_t>, 5> arr{
            std::make_pair(std::string{"One"}, 1u),
            std::make_pair(std::string{"Two"}, 2u),
            std::make_pair(std::string{"Three"}, 3u),
            std::make_pair(std::string{"Four"}, 4u),
            std::make_pair(std::string{"One"}, 1u),
        };
        coust::container::robin_map<std::string, uint32_t> s5{
            arr.begin(), arr.end()};
        CHECK(s5.size() == 4);
        CHECK(s5.at(std::string{"One"}) == 1u);
        CHECK(s5.at(std::string{"Two"}) == 2u);
        CHECK(s5.at(std::string{"Three"}) == 3u);
        CHECK(s5.at(std::string{"Four"}) == 4u);

        coust::container::robin_map<std::string, uint32_t> s6{
            std::make_pair(std::string{"One"}, 1u),
            std::make_pair(std::string{"Two"}, 2u),
            std::make_pair(std::string{"Three"}, 3u),
            std::make_pair(std::string{"Four"}, 4u),
            std::make_pair(std::string{"One"}, 1u),
        };
        CHECK(s6.size() == 4);
        CHECK(s6.at(std::string{"One"}) == 1u);
        CHECK(s6.at(std::string{"Two"}) == 2u);
        CHECK(s6.at(std::string{"Three"}) == 3u);
        CHECK(s6.at(std::string{"Four"}) == 4u);
        s6.clear();
        CHECK(s6.empty());
    }
    SUBCASE("Iterators") {
        coust::container::robin_map<uint32_t, std::string> s0{};
        CHECK(s0.begin() == s0.end());
        CHECK(s0.cbegin() == s0.cend());
        CHECK(s0.empty());

        coust::container::robin_map<uint32_t, std::string> s1{
            std::make_pair(0, std::string{"Zero"}),
            std::make_pair(1, std::string{"One"}),
            std::make_pair(2, std::string{"Two"}),
            std::make_pair(3, std::string{"Three"}),
        };
        auto iter = s1.begin();
        for (size_t i = 0; i < s1.size(); ++i) {
            CHECK(s1.contains(iter->first));
            iter++;
        }
        CHECK(iter == s1.end());
    }
    SUBCASE("Modifier") {
        coust::container::robin_map<std::string, uint32_t> s0{};
        std::array content{
            std::make_pair(std::string{"Zero"}, 0u),
            std::make_pair(std::string{"One"}, 1u),
            std::make_pair(std::string{"Two"}, 2u),
            std::make_pair(std::string{"Three"}, 3u),
            std::make_pair(std::string{"Four"}, 4u),
            std::make_pair(std::string{"Five"}, 5u),
            std::make_pair(std::string{"Six"}, 6u),
            std::make_pair(std::string{"Seven"}, 7u),
            std::make_pair(std::string{"Eight"}, 8u),
            std::make_pair(std::string{"Nine"}, 9u),
            std::make_pair(std::string{"Ten"}, 10u),
        };
        {
            auto [iter, success] = s0.insert(content[0]);
            CHECK(success);
            auto [k, m] = *iter;
            CHECK(s0.size() == 1);
            CHECK(content[0].first == k);
            CHECK(content[0].second == m);
        }
        {
            auto iter = s0.insert(s0.cend(), content[1]);
            auto [k, m] = *iter;
            CHECK(s0.size() == 2);
            CHECK(content[1].first == k);
            CHECK(content[1].second == m);
        }
        {
            s0.insert(content.begin() + 2, content.begin() + 4);
            CHECK(s0.size() == 4);
            CHECK(s0.contains(std::string{"Two"}));
            CHECK(s0.contains(std::string{"Three"}));
        }
        {
            s0.insert({content[4], content[5]});
            CHECK(s0.size() == 6);
            CHECK(s0.contains(std::string{"Four"}));
            CHECK(s0.contains(std::string{"Five"}));
        }
        {
            auto [iter, success] =
                s0.insert_or_assign(content[6].first, content[6].second);
            CHECK(success);
            CHECK(s0.size() == 7);
            CHECK(s0.at(std::string{"Six"}) == 6);
        }
        {
            auto [iter, success] = s0.insert_or_assign(content[0].first, 1u);
            CHECK(!success);
            CHECK(s0.size() == 7);
            CHECK(s0.at(content[0].first) == 1u);
        }
        {
            auto iter = s0.insert_or_assign(s0.cbegin(), content[0].first, 0u);
            CHECK(s0.size() == 7);
            auto [k, m] = *iter;
            CHECK(content[0].first == k);
            CHECK(0u == m);
        }
        {
            auto [iter, success] =
                s0.emplace(content[7].first, content[7].second);
            CHECK(success);
            CHECK(s0.size() == 8);
            auto [k, m] = *iter;
            CHECK(content[7].first == k);
            CHECK(content[7].second == m);
        }
        {
            auto iter =
                s0.emplace_hint(s0.cend(), content[8].first, content[8].second);
            CHECK(s0.size() == 9);
            auto [k, m] = *iter;
            CHECK(content[8].first == k);
            CHECK(content[8].second == m);
        }
        {
            auto [iter, success] =
                s0.try_emplace(content[9].first, content[9].second);
            CHECK(success);
            CHECK(s0.size() == 10);
            auto [k, m] = *iter;
            CHECK(content[9].first == k);
            CHECK(content[9].second == m);
        }
        {
            auto iter = s0.try_emplace(
                s0.cbegin(), content[10].first, content[10].second);
            CHECK(s0.size() == 11);
            auto [k, m] = *iter;
            CHECK(content[10].first == k);
            CHECK(content[10].second == m);
        }
        {
            s0.erase(content[0].first);
            CHECK(s0.size() == 10);
            CHECK(!s0.contains(content[0].first));
        }
        {
            s0.erase(s0.begin(), s0.end());
            CHECK(s0.empty());
        }
    }

    SUBCASE("LookUp") {
        std::array content{
            std::make_pair(std::string{"Zero"}, 0u),
            std::make_pair(std::string{"One"}, 1u),
            std::make_pair(std::string{"Two"}, 2u),
            std::make_pair(std::string{"Three"}, 3u),
            std::make_pair(std::string{"Four"}, 4u),
            std::make_pair(std::string{"Five"}, 5u),
            std::make_pair(std::string{"Six"}, 6u),
            std::make_pair(std::string{"Seven"}, 7u),
            std::make_pair(std::string{"Eight"}, 8u),
            std::make_pair(std::string{"Nine"}, 9u),
            std::make_pair(std::string{"Ten"}, 10u),
        };
        coust::container::robin_map<std::string, uint32_t> s0{
            content.begin(), content.end()};
        {
            auto iter = s0.find(content[0].first);
            auto [k, m] = *iter;
            CHECK(k == content[0].first);
            CHECK(m == content[0].second);
        }
        {
            auto iter = s0.find(content[1].first);
            auto [k, m] = *iter;
            CHECK(k == content[1].first);
            CHECK(m == content[1].second);
        }
        {
            auto iter = s0.find(content[10].first);
            auto [k, m] = *iter;
            CHECK(k == content[10].first);
            CHECK(m == content[10].second);
        }
    }
}

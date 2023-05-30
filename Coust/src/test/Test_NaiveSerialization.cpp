#include "test/Test.h"

#include "utils/math/BoundingBox.h"

#include "utils/filesystem/NaiveSerialization.h"

TEST_CASE(
    "[Coust] [utils] [filesystem] Naive Serialization" * doctest::skip(true)) {
    using namespace coust;
    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wfloat-equal")
    CLANG_DISABLE_WARNING("-Wunused-member-function")
    // type from https://github.com/fraillt/cpp_serializers_benchmark
    enum Color : uint8_t {
        Red,
        Green,
        Blue
    };
    struct Vec3 {
        float x;
        float y;
        float z;
        auto operator<=>(Vec3 const&) const noexcept = default;
    };
    struct Weapon {
        std::string name;
        int16_t damage;
        auto operator<=>(Weapon const&) const noexcept = default;
    };
    struct Monster {
        Vec3 pos;
        int16_t mana;
        int16_t hp;
        std::string name;
        std::vector<uint8_t> inventory;
        Color color;
        std::vector<Weapon> weapons;
        Weapon equipped;
        std::vector<Vec3> path;
        auto operator<=>(Monster const&) const noexcept = default;
    };

    struct NonAggregate {
        NonAggregate() = default;
        NonAggregate(int i) : i1(i), i2(i), i3(i) {}
        int i1 = 0;
        int i2 = 0;
        int i3 = 0;

        auto operator<=>(NonAggregate const&) const noexcept = default;
        using member_count = std::integral_constant<int, 3>;
    };
    WARNING_POP

    SUBCASE("Fundamental type & Enum") {
        int constexpr exp_cnt = 20;
        for (int i = 0; i < exp_cnt; ++i) {
            int origin = i * 2 + i * i;
            file::ByteArray byte_array = file::to_byte_array(origin);
            int from_bytes = file::from_byte_array<int>(byte_array);
            CHECK(origin == from_bytes);
        }
        {
            file::ByteArray byte_array = file::to_byte_array(Color::Red);
            Color c = file::from_byte_array<Color>(byte_array);
            CHECK(c == Color::Red);
        }
        {
            file::ByteArray byte_array = file::to_byte_array(Color::Green);
            Color c = file::from_byte_array<Color>(byte_array);
            CHECK(c == Color::Green);
        }
        {
            file::ByteArray byte_array = file::to_byte_array(Color::Blue);
            Color c = file::from_byte_array<Color>(byte_array);
            CHECK(c == Color::Blue);
        }
    }

    SUBCASE("Trivially accessible type") {
        int constexpr exp_cnt = 20;
        for (int i = 0; i < exp_cnt; ++i) {
            Vec3 v{float(i - 1), float(i * 3), 1.0f / float(i)};
            file::ByteArray byte_array = file::to_byte_array(v);
            Vec3 from_byte = file::from_byte_array<Vec3>(byte_array);
            CHECK(from_byte == v);
        }
        for (int i = 0; i < exp_cnt; ++i) {
            NonAggregate a{i * 22};
            file::ByteArray byte_array = file::to_byte_array(a);
            NonAggregate from_byte =
                file::from_byte_array<NonAggregate>(byte_array);
            CHECK(from_byte == a);
        }
    }

    SUBCASE("Contiguous range") {
        {
            std::array array{1, 3, 5, 7, 9, 2, 4, 6, 8, 10};
            file::ByteArray byte_array = file::to_byte_array(array);
            auto from_byte = file::from_byte_array<decltype(array)>(byte_array);
            CHECK(std::ranges::equal(array, from_byte));
        }
        {
            std::vector<int> vec{1, 3, 5, 7, 9, 2, 4, 6, 8, 10};
            file::ByteArray byte_array = file::to_byte_array(vec);
            auto from_byte =
                file::from_byte_array<std::vector<int>>(byte_array);
            CHECK(std::ranges::equal(vec, from_byte));
        }
        int constexpr exp_cnt = 20;
        {
            std::vector<Vec3> vecs{};
            vecs.reserve(exp_cnt);
            for (int i = 0; i < exp_cnt; ++i) {
                vecs.emplace_back(float(i - 1), float(i * 3), 1.0f / float(i));
            }
            file::ByteArray byte_array = file::to_byte_array(vecs);
            auto from_byte =
                file::from_byte_array<std::vector<Vec3>>(byte_array);
            CHECK(std::ranges::equal(vecs, from_byte));
        }
        {
            std::vector<NonAggregate> ags{};
            ags.reserve(20);
            for (int i = 0; i < exp_cnt; ++i) {
                ags.emplace_back(i * 22);
            }
            file::ByteArray byte_array = file::to_byte_array(ags);
            auto from_byte =
                file::from_byte_array<std::vector<NonAggregate>>(byte_array);
            CHECK(std::ranges::equal(ags, from_byte));
        }
    }

    SUBCASE("Complicate structs") {
        int16_t exp_cnt = 30;
        for (int16_t i = 0; i < exp_cnt; ++i) {
            Weapon w{std::to_string(i), i};
            auto byte_array = file::to_byte_array(w);
            auto from_byte = file::from_byte_array<Weapon>(byte_array);
            CHECK(w == from_byte);
        }
        for (int16_t i = 0; i < exp_cnt; ++i) {
            Monster m{
                .pos =
                    {
                          .x = (float) i - 12,
                          .y = (float) i + 22,
                          .z = (float) i * 9,
                          },
                .mana = i / 2,
                .hp = i * -10,
                .name = std::to_string(i + 4),
                .inventory =
                    {
                          (uint8_t) (i + 0),
                          (uint8_t) (i + 7),
                          (uint8_t) (i + 3),
                          (uint8_t) (i + 5),
                          },
                .color = Color::Red,
                .weapons =
                    {
                          {std::to_string(i * 8), i},
                          },
                .equipped = {std::to_string(i * 8), i},
                .path = {},
            };
            auto byte_array = file::to_byte_array(m);
            auto from_byte = file::from_byte_array<Monster>(byte_array);
            CHECK(m == from_byte);
        }
    }

    SUBCASE("Continuous container of complicate struct") {
        int16_t exp_cnt = 30;
        std::vector<Monster> monsters{};
        monsters.reserve(30);
        for (int16_t i = 0; i < exp_cnt; ++i) {
            Monster m{
                .pos =
                    {
                          .x = (float) i - 12,
                          .y = (float) i + 22,
                          .z = (float) i * 9,
                          },
                .mana = i / 2,
                .hp = i * -10,
                .name = std::to_string(i + 4),
                .inventory =
                    {
                          (uint8_t) (i + 0),
                          (uint8_t) (i + 7),
                          (uint8_t) (i + 3),
                          (uint8_t) (i + 5),
                          },
                .color = Color::Red,
                .weapons =
                    {
                          {std::to_string(i * 8), i},
                          },
                .equipped = {std::to_string(i * 8), i},
                .path = {},
            };
            monsters.push_back(std::move(m));
        }
        auto byte_array = file::to_byte_array(monsters);
        auto from_byte =
            file::from_byte_array<std::vector<Monster>>(byte_array);
        CHECK(std::ranges::equal(monsters, from_byte));
    }
}

TEST_CASE("[Coust] [utils] [filesystem] Naive Serialization for Bounding Box" *
          doctest::skip(true)) {
    using namespace coust;
    {
        BoundingBox b{};
        file::ByteArray byte_array = file::to_byte_array(b);
        BoundingBox from_bytes = file::from_byte_array<BoundingBox>(byte_array);
        CHECK(b == from_bytes);
    }
    {
        BoundingBox b{glm::vec3{1.0f}, glm::vec3{2.3f}};
        file::ByteArray byte_array = file::to_byte_array(b);
        BoundingBox from_bytes = file::from_byte_array<BoundingBox>(byte_array);
        CHECK(b == from_bytes);
    }
    {
        std::vector<glm::vec3> vs{};
        for (uint32_t i = 1; i <= 10; ++i) {
            vs.emplace_back(float(i));
        }
        BoundingBox b{
            {vs.begin(), vs.end()}
        };
        file::ByteArray byte_array = file::to_byte_array(b);
        BoundingBox from_bytes = file::from_byte_array<BoundingBox>(byte_array);
        CHECK(b == from_bytes);
    }
}

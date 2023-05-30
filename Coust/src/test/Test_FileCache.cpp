#include "pch.h"

#include "test/Test.h"

#include "utils/filesystem/FileCache.h"
#include "utils/filesystem/NaiveSerialization.h"

TEST_CASE("[Coust] [utils] [filesystem] File Cache" * doctest::skip(true)) {
    using namespace coust;
    std::filesystem::path headers_path =
        file::get_absolute_path_from(".coustcache", "coust_cache_header");
    {
        file::Caches cache{headers_path};
        {
            memory::string<DefaultAlloc> str0{"0", get_default_alloc()};
            cache.add_cache_data(
                "Some random string 0", 0, file::to_byte_array(str0), false);
            auto [bytes, status] =
                cache.get_cache_data("Some random string 0", 0);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str0);
        }
        {
            memory::string<DefaultAlloc> str1{"1", get_default_alloc()};
            cache.add_cache_data(
                "Some random string 1", 1, file::to_byte_array(str1), false);
            auto [bytes, status] =
                cache.get_cache_data("Some random string 1", 1);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str1);
        }
        {
            memory::string<DefaultAlloc> str2{"2", get_default_alloc()};
            cache.add_cache_data(
                "Some random string 2", 2, file::to_byte_array(str2), false);
            auto [bytes, status] =
                cache.get_cache_data("Some random string 2", 2);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str2);
        }
        {
            memory::string<DefaultAlloc> str3{"3", get_default_alloc()};
            cache.add_cache_data(
                "Some random string 3", 3, file::to_byte_array(str3), false);
            auto [bytes, status] =
                cache.get_cache_data("Some random string 3", 3);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str3);
        }
        {
            memory::string<DefaultAlloc> str4{"4", get_default_alloc()};
            cache.add_cache_data(
                "Some random string 4", 4, file::to_byte_array(str4), false);
            auto [bytes, status] =
                cache.get_cache_data("Some random string 4", 4);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str4);
        }
        {
            memory::string<DefaultAlloc> str5{"5", get_default_alloc()};
            cache.add_cache_data(
                "Some random string 5", 5, file::to_byte_array(str5), false);
            auto [bytes, status] =
                cache.get_cache_data("Some random string 5", 5);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str5);
        }
    }
    {
        file::Caches cache{headers_path};
        {
            memory::string<DefaultAlloc> str0{"0", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 0", 0);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str0);
        }
        {
            memory::string<DefaultAlloc> str1{"1", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 1", 1);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str1);
        }
        {
            memory::string<DefaultAlloc> str2{"2", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 2", 2);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str2);
        }
        {
            memory::string<DefaultAlloc> str3{"3", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 3", 3);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str3);
        }
        {
            memory::string<DefaultAlloc> str4{"4", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 4", 4);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str4);
        }
        {
            memory::string<DefaultAlloc> str5{"5", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 5", 5);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str5);
        }
    }
    {
        file::Caches cache{headers_path};
        {
            memory::string<DefaultAlloc> str0{"0", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 0", 0);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str0);
        }
        {
            memory::string<DefaultAlloc> str1{"1", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 1", 1);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str1);
        }
        {
            memory::string<DefaultAlloc> str2{"2", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 2", 2);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str2);
        }
        {
            memory::string<DefaultAlloc> str3{"3", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 3", 3);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str3);
        }
        {
            memory::string<DefaultAlloc> str4{"4", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 4", 4);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str4);
        }
        {
            memory::string<DefaultAlloc> str5{"5", get_default_alloc()};
            auto [bytes, status] =
                cache.get_cache_data("Some random string 5", 5);
            CHECK(status == file::Caches::Status::available);
            memory::string<DefaultAlloc> str{get_default_alloc()};
            file::from_byte_array(bytes, str);
            CHECK(str == str5);
        }
    }
    {
        file::Caches cache{headers_path};
        std::filesystem::path path = file::get_absolute_path_from(
            "Coust", "src", "test", "Test_FileCache.cpp");
        std::string path_str = path.string();
        cache.add_cache_data(path_str, 6, file::to_byte_array(path_str), true);
        auto [bytes, status] = cache.get_cache_data(path_str, 6);
        CHECK(status == file::Caches::Status::available);
        std::string from_bytes = file::from_byte_array<std::string>(bytes);
        CHECK(path_str == from_bytes);
    }
    {
        file::Caches cache{headers_path};
        std::filesystem::path path = file::get_absolute_path_from(
            "Coust", "src", "test", "Test_FileCache.cpp");
        std::string path_str = path.string();
        auto [bytes, status] = cache.get_cache_data(path_str, 6);
        CHECK(status == file::Caches::Status::available);
        std::string from_bytes = file::from_byte_array<std::string>(bytes);
        CHECK(path_str == from_bytes);
    }
    {
        std::filesystem::path orgin_path = file::get_absolute_path_from("Junk");
        std::string content{"Junk"};
        {
            file::ByteArray bytes = file::to_byte_array(content);
            file::write_file_whole(orgin_path, bytes, bytes.size());
        }
        {
            file::Caches cache{headers_path};
            cache.add_cache_data(
                orgin_path.string(), 7, file::to_byte_array(content), true);
        }
        {
            file::Caches cache{headers_path};
            auto [bytes, status] = cache.get_cache_data(orgin_path.string(), 7);
            CHECK(status == file::Caches::Status::available);
            CHECK(content == file::from_byte_array<std::string>(bytes));
        }
        std::filesystem::resize_file(orgin_path, content.size() * 2);
        {
            file::Caches cache{headers_path};
            auto [bytes, status] = cache.get_cache_data(orgin_path.string(), 7);
            CHECK(status == file::Caches::Status::out_of_date);
        }
        std::filesystem::remove(orgin_path);
    }
}

TEST_CASE("[Coust] [utils] [filesystem] Read text into byte array" *
          doctest::skip(false)) {
    using namespace coust;
    std::filesystem::path origin_path = file::get_absolute_path_from("Junk");
    std::string content{"Junk"};
    {
        std::ofstream file{origin_path};
        CHECK(file.is_open());
        file << content;
        file.close();
    }
    {
        file::ByteArray byte_array = file::read_file_whole(origin_path);
        std::string read_form_byte_array{byte_array.to_string_view()};
        CHECK(read_form_byte_array == content);
    }
    std::filesystem::remove(origin_path);
}

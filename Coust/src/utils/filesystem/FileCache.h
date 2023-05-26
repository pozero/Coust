#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/filesystem/FileIO.h"

namespace coust {
namespace file {

class Caches {
public:
    Caches() = delete;
    Caches(Caches &&) = delete;
    Caches(Caches const &) = delete;
    Caches &operator=(Caches &&) = delete;
    Caches &operator=(Caches const &) = delete;

public:
    // the first three statuses are meaningful only if the cache has
    // corresponding file. other caches are always available.
    enum class Status {
        not_found,    // the cache file doesn't exist
        out_of_date,  // our cache data is out of date since the corresponding
                      // file is modifed since last caching
        invalid,      // our cache data is corrupted
        available,    // cache data is available
    };

public:
    Caches(std::filesystem::path headers_path) noexcept;

    ~Caches() noexcept;

    // since std::filesystem::path can't use customized allocator, the api has
    // to use std::string

    // get cache data from memory or disk if exists
    std::pair<ByteArray, Status> get_cache_data(
        std::string origin_name, size_t tag) noexcept;

    // add cache data to memory, wait to be flushed to disk later by manually
    // calling `flush_cache_to_disk` or in destructor
    void add_cache_data(std::string origin_name, size_t tag, ByteArray &&data,
        bool crc32) noexcept;

    // force to flush cache data to disk if corresponding cache exists,
    // otherwise return false;
    bool flush_cache_to_disk(std::string origin_name, size_t tag) noexcept;

private:
    struct Header {
        // either full path of corresponding file or indicating its content if
        // does not have a corresponding file
        memory::string<DefaultAlloc> name{get_default_alloc()};
        // the following data filed is meaningful only when `created_from_file`
        // is true
        memory::string<DefaultAlloc> corresponding_file_last_modified{
            get_default_alloc()};
        size_t corresponding_file_size_in_byte;
        size_t cache_tag;
        size_t cache_size_in_byte;
        uint32_t crc32;
        // is this cache loaded from disk or just created by program
        bool is_new = false;
        // does the cache need the crc32 verification
        bool use_crc32 = false;
        // is the cache created from a file on disk (which means that it needs
        // additional verification related to origin file)
        bool created_from_file = false;
    };

    // the struct to be serialized to disk
    struct Headers {
        memory::string<DefaultAlloc> cache_folder_dir{get_default_alloc()};
        memory::vector_nested<Header, DefaultAlloc> m_headers{
            get_default_alloc()};
    };

private:
    // we always put the magic number in the front of cache data file to
    // help quickly identify any possible corruption in the file
    static uint32_t constexpr MAGIC_NUMBER = 0x13572468;

private:
    // check if the cache exists and if it's out of date compared to
    // corresponding file, if the cache file exists the function return
    // `available` however, that doesn't mean the cache is actually available
    // because it might be invalid. so we need another check.
    Status check_cache_header(Header const &header) const noexcept;

    // if the cache file exists and is up to date, check if it's valid
    Status check_cache_data(Header const &header, ByteArray const &data,
        decltype(MAGIC_NUMBER) magic_num) const noexcept;

    static void write_cache_data(
        std::filesystem::path path, ByteArray const &data) noexcept;

    static std::pair<ByteArray, decltype(MAGIC_NUMBER)> read_cache_data(
        std::filesystem::path path) noexcept;

private:
    Headers m_headers;
    // cache tag -> cache data
    memory::robin_map<size_t, ByteArray, DefaultAlloc> m_cache_data{
        get_default_alloc()};
    std::filesystem::path m_headers_path;
    std::filesystem::path m_cache_dir;
};

}  // namespace file
}  // namespace coust

#pragma once

#include "utils/Compiler.h"

#include <filesystem>
#include <string_view>

namespace coust {
namespace file {

class ByteArray {
public:
    ByteArray() = delete;
    ByteArray(ByteArray const&) = delete;
    ByteArray& operator=(ByteArray const&) = delete;

public:
    // the alignment is both the address alignment and actual size alignment
    ByteArray(size_t size, size_t alignment) noexcept;

    ByteArray(ByteArray&& other) noexcept;

    ByteArray& operator=(ByteArray&& other) noexcept;

    ~ByteArray() noexcept;

    size_t size() const noexcept;

    std::string_view to_string_view() const noexcept;

    template <typename T>
    T* get() noexcept {
        return (T*) m_bytes;
    }

    template <typename T>
    const T* get() const noexcept {
        return (T*) m_bytes;
    }

private:
    size_t m_size = 0;
    char* RESTRICT m_bytes = nullptr;
};

ByteArray read_file_whole(std::filesystem::path const& path,
    size_t alignment = alignof(char)) noexcept;

void write_file_whole(
    std::filesystem::path const& path, ByteArray const& data) noexcept;

template <typename... Args>
std::filesystem::path get_absolute_path_from(
    const char* first_dir, Args... args) noexcept
    requires(std::conjunction_v<std::is_same<const char*, Args>...>)
{
    std::filesystem::path ret{COUST_ROOT_PATH};
    ret.append(first_dir);
    if constexpr (sizeof...(args) > 0) {
        return get_absolute_path_from(ret.c_str(), args...);
    }
    return ret;
}

}  // namespace file
}  // namespace coust

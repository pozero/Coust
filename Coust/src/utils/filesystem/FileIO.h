#pragma once

#include "utils/Compiler.h"

#include <filesystem>
#include <string_view>

namespace coust {
namespace file {

class ByteArray {
public:
    ByteArray(ByteArray const&) = delete;
    ByteArray& operator=(ByteArray const&) = delete;

public:
    ByteArray() noexcept = default;

    // the alignment is both the address alignment and actual size alignment
    ByteArray(size_t size, size_t alignment) noexcept;

    ByteArray(const void* data, size_t size, size_t alignment) noexcept;

    ByteArray(ByteArray&& other) noexcept;

    ByteArray& operator=(ByteArray&& other) noexcept;

    ~ByteArray() noexcept;

    void grow_to(size_t new_size) noexcept;

    size_t size() const noexcept;

    void* data() noexcept;

    const void* data() const noexcept;

    std::string_view to_string_view() const noexcept;

    std::span<const char> to_span() const noexcept;

    ByteArray copy() const noexcept;

private:
    size_t m_size = 0;
    size_t m_alignment = 0;
    void* RESTRICT m_bytes = nullptr;
};

ByteArray read_file_whole(std::filesystem::path const& path,
    size_t alignment = alignof(char)) noexcept;

void write_file_whole(std::filesystem::path const& path, ByteArray const& data,
    size_t size) noexcept;

void write_file_whole(
    std::filesystem::path const& path, std::span<char> data) noexcept;

template <typename... Args>
std::filesystem::path get_absolute_path_from(Args&&... args) noexcept
    requires(
        std::conjunction_v<std::is_same<const char*, std::decay_t<Args>>...>)
{
    std::filesystem::path ret{COUST_ROOT_PATH};
    (ret.append(args), ...);
    return ret;
}

}  // namespace file
}  // namespace coust

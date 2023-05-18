#include "pch.h"

#include "core/Memory.h"
#include "utils/Assert.h"
#include "utils/filesystem/FileIO.h"
#include "utils/PtrMath.h"

namespace coust {
namespace file {

ByteArray::ByteArray(size_t size, size_t alignment) noexcept
    : m_size(ptr_math::round_up_to_alinged(size, alignment)),
      m_alignment(alignment),
      m_bytes(get_default_alloc().allocate(m_size, alignment)) {
    memset(m_bytes, 0, m_size);
}

ByteArray::ByteArray(ByteArray&& other) noexcept
    : m_size(other.m_size),
      m_alignment(other.m_alignment),
      m_bytes(other.m_bytes) {
    other.m_size = 0;
    other.m_bytes = nullptr;
}

ByteArray& ByteArray::operator=(ByteArray&& other) noexcept {
    std::swap(m_size, other.m_size);
    std::swap(m_alignment, other.m_alignment);
    std::swap(m_bytes, other.m_bytes);
    return *this;
}

ByteArray::~ByteArray() noexcept {
    get_default_alloc().deallocate(m_bytes, m_size);
}

void ByteArray::grow_to(size_t new_size) noexcept {
    COUST_ASSERT(new_size > m_size,
        "new_size {} is smaller than original size {}", new_size, m_size);
    ByteArray new_byte_array{new_size, m_alignment};
    memcpy(new_byte_array.data(), m_bytes, m_size);
    *this = std::move(new_byte_array);
}

size_t ByteArray::size() const noexcept {
    return m_size;
}

void* ByteArray::data() noexcept {
    return m_bytes;
}

const void* ByteArray::data() const noexcept {
    return m_bytes;
}

std::string_view ByteArray::to_string_view() const noexcept {
    std::string_view ret{(const char*) m_bytes, m_size};
    return ret;
}

ByteArray read_file_whole(
    std::filesystem::path const& path, size_t alignment) noexcept {
    std::ifstream file{path, std::ios::ate | std::ios::binary};
    COUST_PANIC_IF_NOT(file.is_open(), "Can't open file {} as binary in stream",
        path.string());
    auto const file_size = file.tellg();
    ByteArray ret{(size_t) file_size, alignment};
    file.seekg(0);
    file.read((char*) ret.data(), file_size);
    file.close();
    return ret;
}

void write_file_whole(std::filesystem::path const& path, ByteArray const& data,
    size_t size) noexcept {
    std::ofstream file{path, std::ios::binary};
    COUST_PANIC_IF_NOT(file.is_open(),
        "Can't open file {} as binary out stream", path.string());
    file.write((const char*) data.data(), (std::streamsize) size);
    file.close();
}

}  // namespace file
}  // namespace coust

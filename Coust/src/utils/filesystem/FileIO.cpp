#include "pch.h"

#include "core/Memory.h"
#include "utils/Assert.h"
#include "utils/filesystem/FileIO.h"
#include "utils/allocators/Allocator.h"

namespace coust {
namespace file {

ByteArray::ByteArray(size_t size, size_t alignment) noexcept
    : m_size(memory::ptr_math::round_up_to_alinged(size, alignment)),
      m_bytes((char*) get_default_alloc().allocate(m_size, alignment)) {
    memset(m_bytes, 0, m_size);
}

ByteArray::ByteArray(ByteArray&& other) noexcept
    : m_size(other.m_size), m_bytes(other.m_bytes) {
    other.m_size = 0;
    other.m_bytes = nullptr;
}

ByteArray& ByteArray::operator=(ByteArray&& other) noexcept {
    std::swap(m_size, other.m_size);
    std::swap(m_bytes, other.m_bytes);
    return *this;
}

ByteArray::~ByteArray() noexcept {
    get_default_alloc().deallocate(m_bytes, m_size);
}

size_t ByteArray::size() const noexcept {
    return m_size;
}

std::string_view ByteArray::to_string_view() const noexcept {
    std::string_view ret{m_bytes, m_size};
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
    file.read(ret.get<char>(), file_size);
    file.close();
    return ret;
}

void write_file_whole(
    std::filesystem::path const& path, ByteArray const& data) noexcept {
    std::ofstream file{path, std::ios::binary};
    COUST_PANIC_IF_NOT(file.is_open(),
        "Can't open file {} as binary out stream", path.string());
    file.write(data.get<char>(), (std::streamsize) data.size());
    file.close();
}

}  // namespace file
}  // namespace coust

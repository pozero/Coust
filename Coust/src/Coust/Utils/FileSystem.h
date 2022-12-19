#pragma once

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace Coust
{
	class FileSystem
	{
    public:
        static bool Init();

        static void Shut();

        template<typename T>
        static bool GetCache(const std::string& originName, std::vector<T>& out_buf);

        template<typename T>
        static void AddCache(const std::string& originName, std::vector<T>& cacheBytes, bool needCRC32);

    private:
        FileSystem() = default;
        ~FileSystem() = default;

        bool Initialize();

        void Shutdown();

        static FileSystem* s_Instance;

        static constexpr uint32_t MAGIC_NUMBER = 0x13572468;
        static std::filesystem::path s_CacheHeadersPath;

    public:
        enum class CacheStatus
        {
            NO_FOUND,           // cache file doesn't exist
            OUT_OF_DATE,        // cache file out of date
            INVALID,            // cache file up to date but corrupted
            AVAILABLE,          // cache file is valid and up to date, ready to use
        };

    private:
        // find originName in `m_CacheHeaders`, implicitly delete all invalid & out of date cache entries
        CacheStatus getCache(const std::string& originName, std::vector<char>& out_buf);

        // cachebytes will be *MOVED* into `m_Caches`
        void addCache(const std::string& originName, std::vector<char>& cacheContents, bool needCRC32);

    private:
        struct CacheHeader 
        {
            std::string originName;

            std::optional<size_t> originFileSizeInByte;
            std::optional<std::string> originFileLastModifiedTime;

            std::string cacheName;
            size_t cacheSizeInByte;
            std::optional<uint32_t> cacheCRC32;
        };

        struct CacheToWrite
        {
            std::string cacheName;
            std::vector<char> cache;
        };

        std::vector<CacheHeader> m_CacheHeaders;
        std::vector<CacheToWrite> m_Caches;

    public:
        static std::filesystem::path GetRootPath() { return std::filesystem::path{ CURRENT_DIRECTORY }; }

        static std::filesystem::path GetFullPathFrom(const std::initializer_list<const char*>& entries);

		static bool ReadWholeText(const std::filesystem::path& filePath, std::string& out_buf);

        template<typename T>
        static bool ReadWholeBinary(const std::filesystem::path& filePath, std::vector<T>& out_buf);

		static bool WriteWholeText(const std::filesystem::path& filePath, const char* text);

		static bool WriteWholeBinary(const std::filesystem::path& filePath, std::size_t sizeInByte, const char* data);
	};

    uint32_t CRC32FromBuf(const char *buf, size_t len);
}

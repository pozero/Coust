#pragma once

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <functional>

#include "Coust/Utils/Hash.h"

namespace Coust
{
	class FileSystem
	{
    public:
        static FileSystem* CreateFileSystem();
        
        static size_t GetCacheTag(const std::string& originName, std::optional<size_t> extraHash)
        {
            Hash::HashFn<std::string> hasher;
            size_t cacheTag = hasher(originName);
            // if extra hash is provided, combine it
            if (extraHash.has_value())
                Hash::Combine(cacheTag, extraHash);
            return cacheTag;
        }

        void Shutdown();

        /**
         * @brief Get cache
         * 
         * @tparam T 
         * @param originName    File path for a file, or user-defined name for other kinds of cache
         * @param extraHash     Optional. If different version of the same origin exists, the caller should provide extra hash to distinguish them
         * @param out_buf       Output
         * @return
         */
        template<typename T>
        bool GetCache(const std::string& originName, std::optional<size_t> extraHash, std::vector<T>& out_buf);

        /**
         * @brief Add cache
         * 
         * @tparam T 
         * @param originName    File path for a file, or user-defined name for other kinds of cache
         * @param extraHash     Optional. If different version of the same origin exists, the caller should provide extra hash to distinguish them
         * @param cacheBytes    Data to be cached
         * @param needCRC32     If needs further validation
         */
        template<typename T>
        void AddCache(const std::string& originName, std::optional<size_t> extraHash, std::vector<T>& cacheBytes, bool needCRC32);

    private:
        bool Initialize();

        static constexpr uint32_t MAGIC_NUMBER = 0x13572468;
        static std::filesystem::path s_CachePath;
        static std::string s_CacheHeaderFileName;

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
        CacheStatus GetCache_Impl(const std::string& originName, std::optional<size_t> extraHash, std::vector<char>& out_buf);

        // cachebytes will be *MOVED* into `m_Caches`
        void AddCache_Impl(const std::string& originName, std::optional<size_t> extraHash, std::vector<char>& cacheContents, bool needCRC32);

    private:
        /**
         * @brief struct that contains every meta information we need about this cache
         */
        struct CacheHeader 
        {
            std::optional<std::string> originFileLastModifiedTime;
            std::string originName;
            size_t cacheTag;    // comes from combine(hash(originName), extraTag), also used as cache file name
            std::optional<size_t> originFileSizeInByte;
            size_t cacheSizeInByte;
            std::optional<uint32_t> cacheCRC32;
            bool isNew = false;
        };

        struct CacheToWrite
        {
            size_t cacheTag;    // comes from combine(hash(originName), extraTag), also used as cache file name
            std::vector<char> cache;
        };

        std::vector<CacheHeader> m_CacheHeaders;
        std::vector<CacheToWrite> m_Caches;

    public:
        static std::filesystem::path GetRootPath();

        static std::filesystem::path GetFullPathFrom(const std::initializer_list<const char*>& entries);

		static bool ReadWholeText(const std::filesystem::path& filePath, std::string& out_buf);

        template<typename T>
        static bool ReadWholeBinary(const std::filesystem::path& filePath, std::vector<T>& out_buf);

		static bool WriteWholeText(const std::filesystem::path& filePath, const char* text);

		static bool WriteWholeBinary(const std::filesystem::path& filePath, std::size_t sizeInByte, const char* data);
	};

    uint32_t CRC32FromBuf(const char *buf, size_t len);
}

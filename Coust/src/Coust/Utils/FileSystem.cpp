#include "Coust/Core/Logger.h"
#include "pch.h"

#include "Coust/Utils/FileSystem.h"
#include "Coust/Utils/Hash.h"

#undef max
#include "rapidjson/error/error.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace Coust 
{
    // cache file always locate in `CURRENT_DIRECTORY\.coustcache\`

    std::filesystem::path FileSystem::s_CachePath = GetFullPathFrom({ ".coustcache" });
    std::string FileSystem::s_CacheHeaderFileName = "cache_headers.json";

    template
    bool FileSystem::GetCache<uint32_t>(const std::string& originName, size_t hash, std::vector<uint32_t>& out_buf);

    template<>
    bool FileSystem::GetCache<char>(const std::string& originName, size_t hash, std::vector<char>& out_buf)
    {
        return CacheStatus::AVAILABLE == GetCache_Impl(originName, hash, out_buf);
    }

    template<typename T>
    bool FileSystem::GetCache(const std::string& originName, size_t hash, std::vector<T>& out_buf)
    {
        std::vector<char> tmp_buf{};
        auto status = GetCache_Impl(originName, hash, tmp_buf);
        if (status != CacheStatus::AVAILABLE)
            return false;
        
        size_t fullVarCount = tmp_buf.size() / sizeof(T);
        size_t remainByteCount = tmp_buf.size() % sizeof(T);
        out_buf.resize(fullVarCount + (size_t) ((bool) remainByteCount));

        std::memcpy(out_buf.data(), tmp_buf.data(), tmp_buf.size());
        return true;
    }

    template
    void FileSystem::AddCache<uint32_t>(const std::string& originName, size_t hash, std::vector<uint32_t>& cacheBytes, bool needCRC32);

    template<>
    void FileSystem::AddCache<char>(const std::string& originName, size_t hash, std::vector<char>& cacheBytes, bool needCRC32)
    {
        return AddCache_Impl(originName, hash, cacheBytes, needCRC32);
    }

    template<typename T>
    void FileSystem::AddCache(const std::string& originName, size_t hash, std::vector<T>& cacheBytes, bool needCRC32)
    {
        std::vector<char> tmp_buf{};
        tmp_buf.resize(cacheBytes.size() * sizeof(T));
        std::memcpy(tmp_buf.data(), cacheBytes.data(), cacheBytes.size() * sizeof(T));

        AddCache_Impl(originName, hash, tmp_buf, needCRC32);
    }

    FileSystem* FileSystem::CreateFileSystem()
    {
        FileSystem* filesystem = new FileSystem();
        if (filesystem->Initialize())
            return filesystem;
        else
        {
            filesystem->Shutdown();
            delete filesystem;
            return nullptr;
        }
    }

    bool FileSystem::Initialize()
    {
        auto cacheHeadersPath = s_CachePath / s_CacheHeaderFileName;
        if (!std::filesystem::exists(s_CachePath))
        {
            std::filesystem::create_directory(s_CachePath);
        }
        if (!std::filesystem::exists(cacheHeadersPath))
        {
            std::ofstream headers{cacheHeadersPath};
            headers.close();
            return true;
        }
        
        if (std::string headerJson; ReadWholeText(cacheHeadersPath, headerJson))
        {
            rapidjson::Document doc{};
            doc.Parse(headerJson.c_str());
            if (!doc.HasParseError())
            {
                for (auto iter = doc.MemberBegin(); iter != doc.MemberEnd(); ++iter)
                {
                    CacheHeader h
                    {
                        .isNew = false,
                        .cacheTag = iter->value.FindMember("cache_tag")->value.GetUint64(),
                        .originName = iter->name.GetString(),
                        .cacheSizeInByte = iter->value.FindMember("cache_size_in_byte")->value.GetUint64(),
                    };

                    if (auto m = iter->value.FindMember("size_in_byte"); !m->value.IsNull())
                        h.originFileSizeInByte = m->value.GetUint64();
                    if (auto m = iter->value.FindMember("last_modified"); !m->value.IsNull())
                        h.originFileLastModifiedTime = m->value.GetString();
                    if (auto m = iter->value.FindMember("cache_crc32"); !m->value.IsNull())
                        h.cacheCRC32 = m->value.GetUint();

                    m_CacheHeaders.push_back(h);
                }
                return true;
            }
        }
        COUST_CORE_WARN("Failed to read cache header {}, it may not exist or corrupted...", cacheHeadersPath.string());
        return true;
    }

    void FileSystem::Shutdown()
    {
        // flush to cache files
        for (const auto& c : m_Caches)
        {
            // TODO: Task System
            auto path = s_CachePath;
            path /= std::to_string(c.cacheTag);

            std::ofstream file{ path, std::ios::binary };

            if (!file.is_open())
                return;

            file.write((const char*) &MAGIC_NUMBER, sizeof(uint32_t));
            file.write(c.cache.data(), c.cache.size());
            file.close();
        }

        // flush to cache header
        rapidjson::Document doc{};
        doc.SetObject();
        for (const auto& h : m_CacheHeaders)
        {
            rapidjson::Value cacheTag(h.cacheTag);
            rapidjson::Value cacheSizeInByte(h.cacheSizeInByte);

            rapidjson::Value originFileSizeInByte{};
            rapidjson::Value originFileLastModifiedTime{};
            rapidjson::Value cacheCRC32{};

            if (h.originFileSizeInByte.has_value())
                originFileSizeInByte.SetUint64(h.originFileSizeInByte.value());
            if (h.originFileLastModifiedTime.has_value())
                originFileLastModifiedTime.SetString(h.originFileLastModifiedTime->c_str(), (rapidjson::SizeType) h.originFileLastModifiedTime->length());
            if (h.cacheCRC32.has_value())
                cacheCRC32.SetUint(h.cacheCRC32.value());

            rapidjson::Value cacheEntry(rapidjson::kObjectType);
            cacheEntry.AddMember("cache_tag", cacheTag, doc.GetAllocator());
            cacheEntry.AddMember("cache_size_in_byte", cacheSizeInByte, doc.GetAllocator());
            cacheEntry.AddMember("size_in_byte", originFileSizeInByte, doc.GetAllocator());
            cacheEntry.AddMember("last_modified", originFileLastModifiedTime, doc.GetAllocator());
            cacheEntry.AddMember("cache_crc32", cacheCRC32, doc.GetAllocator());

            const char* entryName = h.originName.c_str();
            doc.AddMember(rapidjson::StringRef(entryName), cacheEntry, doc.GetAllocator());
        }

        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        doc.Accept(writer);
        WriteWholeText(s_CachePath / s_CacheHeaderFileName, buf.GetString());
    }

    FileSystem::CacheStatus FileSystem::GetCache_Impl(const std::string& originName, size_t hash, std::vector<char>& out_buf)
    {
        size_t cacheTag = hash;

        auto header = m_CacheHeaders.cbegin();
        for (; header != m_CacheHeaders.cend(); header++)
        {
            if (cacheTag == header->cacheTag)
                break;
        }
        if (header == m_CacheHeaders.cend())
        {
            COUST_CORE_TRACE("Cache of {} Not Found (doesn't exist)", originName);
            return CacheStatus::NO_FOUND;
        }

        // new cache header, not wrote to disk yet
        if (header->isNew)
        {
            COUST_CORE_TRACE("Cache of {} Not Found (not flushed to disk yet)", originName);
            return CacheStatus::NO_FOUND;
        }
        
        // origin file deleted, log warning
        if (!std::filesystem::exists(originName))
        {
            COUST_CORE_WARN("Origin file has been deleted!");
            return CacheStatus::NO_FOUND;
        }

        // origin file size changed
        if (header->originFileSizeInByte.has_value())
        {
            size_t originFileCurrentSize = std::filesystem::file_size(originName);
            if (originFileCurrentSize != header->originFileSizeInByte.value())
            {
                m_CacheHeaders.erase(header);
                COUST_CORE_TRACE("Cache of {} Out of Date (Origin File Size Changed)", originName);
                return CacheStatus::OUT_OF_DATE;
            }
        }

        // origin file last modified time changed
        if (header->originFileLastModifiedTime.has_value())
        {
            std::stringstream ss{};
            ss << std::filesystem::last_write_time(originName);
            if (header->originFileLastModifiedTime.value() != ss.str())
            {
                m_CacheHeaders.erase(header);
                COUST_CORE_TRACE("Cache of {} Out of Date (Origin File Modified)", originName);
                return CacheStatus::OUT_OF_DATE;
            }
        }

        std::ifstream cache{ GetFullPathFrom({".coustcache", std::to_string(header->cacheTag).c_str()}), std::ios::ate | std::ios::binary};
        // cache file doesn't exist or can't open
        if (!cache.is_open())
        {
            cache.close();
            m_CacheHeaders.erase(header);
            COUST_CORE_TRACE("Cache of {} Invalid (Can't Read Cache)", originName);
            return CacheStatus::INVALID;
        }

        // cache file size doesn't match, corrupted
        else if (size_t cacheSize = (size_t)cache.tellg(); cacheSize - sizeof(uint32_t) != header->cacheSizeInByte)
        {
            cache.close();
            m_CacheHeaders.erase(header);
            COUST_CORE_TRACE("Cache of {} Invalid (Cache Size Varied)", originName);
            return CacheStatus::INVALID;
        }

        cache.seekg(0);
        // cache magic number doesn't match
        {
            uint32_t headGuard;
            cache.read((char*) &headGuard, sizeof(headGuard));
            if (headGuard != MAGIC_NUMBER)
            {
                cache.close();
                COUST_CORE_TRACE("Cache of {} Invalid (Cache Corrupted)", originName);
                return CacheStatus::INVALID;
            }
        }

        out_buf.resize(header->cacheSizeInByte);
        cache.read(out_buf.data(), header->cacheSizeInByte);
        cache.close();

        // cache file crc32 hash doesn't match, corrupted
        if (header->cacheCRC32.has_value())
        {
            uint32_t currentCacheCRC32 = CRC32FromBuf(out_buf.data(), out_buf.size());
            if (currentCacheCRC32 != header->cacheCRC32.value())
            {
                cache.close();
                out_buf.clear();
                m_CacheHeaders.erase(header);
                COUST_CORE_TRACE("Cache of {} Invalid (CRC32 Hash Test Failed)", originName);
                return CacheStatus::INVALID;
            }
        }

        COUST_CORE_TRACE("Cache of {} Available", originName);
        return CacheStatus::AVAILABLE;
    }

    void FileSystem::AddCache_Impl(const std::string& originName, size_t hash, std::vector<char>& cacheBytes, bool needCRC32)
    {
        size_t cacheTag = hash;

        CacheHeader header;
        {
            header.isNew = true;
            header.originName = originName;
            header.cacheSizeInByte = cacheBytes.size();

            header.cacheTag = cacheTag;

            // if it's a file, add its last write time and size as validation
            if (std::filesystem::exists(originName))
            {
                std::stringstream ss{};
                ss << std::filesystem::last_write_time(originName);
                header.originFileLastModifiedTime = ss.str();

                header.originFileSizeInByte = std::filesystem::file_size(originName);
            }

            if (needCRC32)
            {
                uint32_t crc = CRC32FromBuf(cacheBytes.data(), header.cacheSizeInByte);
                header.cacheCRC32 = crc;
            }
        }

        for (auto iter = m_CacheHeaders.cbegin(); iter != m_CacheHeaders.cend(); iter++)
        {
            // We always use the latest cache
            // If older cache with same name exists, erase it
            if (iter->cacheTag == cacheTag)
            {
                m_CacheHeaders.erase(iter);
                break;
            }
        }

        m_CacheHeaders.push_back(header);
        m_Caches.push_back({cacheTag, std::move(cacheBytes)});
    }

    std::filesystem::path FileSystem::GetRootPath()
    { 
        return std::filesystem::path{ CURRENT_DIRECTORY }; 
    }

    std::filesystem::path FileSystem::GetFullPathFrom(const std::initializer_list<const char*>& entries)
    {
        std::filesystem::path fullPath = GetRootPath();
        for (const char* e : entries)
        {
            fullPath /= e;
        }

        return fullPath;
    }

    bool FileSystem::ReadWholeText(const std::filesystem::path& filePath, std::string& out_buf)
    {
        std::ifstream file{ filePath };
        if (!file.is_open())
            return false;

        std::stringstream ss{};
        ss << file.rdbuf();
        file.close();
    
        out_buf = ss.str();
        return true;
    }
    
    template<typename T>
    bool FileSystem::ReadWholeBinary(const std::filesystem::path& filePath, std::vector<T>& out_buf)
    {
        std::ifstream file{ filePath, std::ios::ate | std::ios::binary };
        if (!file.is_open())
            return false;
    
        size_t fileSize = (size_t) file.tellg();
        // alignment
        size_t blockCount = fileSize / sizeof(T) + (size_t) ((bool) (fileSize % sizeof(T)));
        out_buf.resize(blockCount);
    
        file.seekg(0);
        file.read((char*)out_buf.data(), fileSize);
        file.close();
    
        return true;
    }
    
    bool FileSystem::WriteWholeText(const std::filesystem::path& filePath, const char* text)
    {
        std::ofstream file{ filePath, std::ios::trunc };
        if (!file.is_open())
            return false;
    
        file << text;
        file.close();
        return true;
    }
    
    bool FileSystem::WriteWholeBinary(const std::filesystem::path& filePath, std::size_t sizeInByte, const char* data)
    {
        std::ofstream file{ filePath, std::ios::binary };
        if (!file.is_open())
            return false;
    
        file.write(data, sizeInByte);
        file.close();
        return true;
    }


    /* Copyright (C) 1986 Gary S. Brown.  You may use this program, or
    code or tables extracted from it, as desired without restriction.*/
    static uint32_t crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    
    #define UPDC32(octet,crc) (crc_32_tab[ ((crc) ^ ((uint8_t)octet)) & 0xff ] ^ ((crc) >> 8))

    uint32_t CRC32FromBuf(const char *buf, size_t len)
    {
       uint32_t oldcrc32 = 0xFFFFFFFF;
       for (; len > 0; --len, ++buf)
       {
          oldcrc32 = UPDC32(*buf, oldcrc32);
       }
       return ~oldcrc32;
    }
}


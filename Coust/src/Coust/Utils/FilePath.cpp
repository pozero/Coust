#include "pch.h"

#include "Coust/Utils/FilePath.h"

namespace Coust
{
    FilePath::FilePath()
    {
        const char* src = CURRENT_DIRECTORY;
        char* dst = (char*) m_Buf;
        std::size_t size = std::strlen(src) + 1;
        std::memcpy(dst, src, size);
    }

    FilePath::FilePath(const FilePath& other)
    {
        const char* src = (const char*) other.m_Buf;
        char* dst = m_Buf;
        std::size_t size = std::strlen(src) + 1;
        std::memcpy(dst, src, size);
    }

    FilePath& FilePath::operator=(const FilePath& other)
    {
        const char* src = (const char*) other.m_Buf;
        char* dst = m_Buf;
        std::size_t size = std::strlen(src) + 1;
        std::memcpy(dst, src, size);
        return *this;
    }

    FilePath::FilePath(const char* pathStr)
    {
        if (std::strlen(pathStr) < 2)
            return;
        if (!std::isupper(pathStr[0]) || pathStr[1] != ':' || (pathStr[2] != '/' && pathStr[2] != '\\'))
            return;

        const char* src = pathStr;
        char* dst = m_Buf;
        std::size_t size = std::strlen(src) + 1;
        std::memcpy(dst, src, size);
    }

    FilePath& FilePath::AddDirectory(const char* directoryName)
    {
        char* dst = GetFirstNil();
        *(dst++) = '\\';
        std::size_t size = std::strlen(directoryName) + 1;
        std::memcpy(dst, directoryName, size);
        return *this;
    }
    
    void FilePath::AddFile(const char* fileName)
    {
        char* dst = GetFirstNil();
        *(dst++) = '\\';
        std::size_t size = std::strlen(fileName) + 1;
        std::memcpy(dst, fileName, size);
    }
    
    const char* FilePath::Get() const
    {
        return m_Buf;
    }
    
    const char* FilePath::GetName() const
    {
        const char* lastSlash = GetLastSlash();
        return lastSlash + 1;
    }
    
    FilePath& FilePath::GoBack()
    {
        char* lastSlash = GetLastSlash();
        *lastSlash = '\0';
        return *this;
    }

    char* FilePath::GetLastSlash()
    {
        char* lastSlash = nullptr;
        for (char* p = (char*) m_Buf; *p != '\0'; ++p)
        {
            if (*p == '\\')
                lastSlash = p;
        }
        return lastSlash;
    }

    const char* FilePath::GetLastSlash() const
    {
        return (const char*) ((FilePath*) this)->GetLastSlash();
    }

    char* FilePath::GetFirstNil()
    {
        char* p = (char*) m_Buf;
        while (*p != '\0')
            ++p;
        return p;
    }

    bool FilePath::IsRelativePath(const char* path)
    {
        if (std::strlen(path) < 2)
            return true;

        // Windows Specific
        return path[1] != ':';
    }
}


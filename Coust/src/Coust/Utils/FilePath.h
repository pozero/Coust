#pragma once

#include <algorithm>
#include <string>

namespace Coust
{
    class FilePath
    {
        static constexpr int BUF_MAX_SIZE = 1024;
    public:
        FilePath();

        FilePath(const FilePath& other);
        FilePath& operator=(const FilePath& other);

        // Just simply check string comes in. If the path is invalid, bahavior is undefined
        FilePath(const char* pathStr);

        FilePath& AddDirectory(const char* directoryName);

        void AddFile(const char* fileName);

        const char* Get() const;

        const char* GetName() const;

        FilePath& GoBack();

        std::ostream& operator<<(std::ostream& os);

    private:
        const char* GetLastSlash() const;
        char* GetLastSlash();

        char* GetFirstNil();

    private:
        char m_Buf[BUF_MAX_SIZE]{0};

    public:
        static bool IsRelativePath(const char* path);
    };
}

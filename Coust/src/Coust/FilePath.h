#pragma once

#include <string>

namespace Coust
{
    class FilePath
    {
    public:
        FilePath()
            : m_Path(CURRENT_DIRECTORY) {}

        FilePath& AddDirectory(const char* directoryName)
        {
            m_Path = m_Path + "\\";
            m_Path = m_Path + directoryName;
            return *this;
        }

        void AddFile(const char* fileName)
        {
            m_Path = m_Path + "\\";
            m_Path = m_Path + fileName;
        }

        const char* Get() const
        {
            return m_Path.c_str();
        }
    private:
        std::string m_Path;
    };
}
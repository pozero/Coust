#include "pch.h"

#include "FileStream.h"

namespace Coust 
{
    std::string FileStream::ReadWholeText(const char* filePath)
    {
        std::ifstream file{ filePath };
        if (!file.is_open())
            return {};
    
        std::stringstream ss{};
    
        ss << file.rdbuf();
        file.close();
    
        return ss.str();
    }
    
    
    bool FileStream::WriteWholeText(const char* filePath, const char* text)
    {
        std::ofstream file{ filePath, std::ios::trunc };
        if (!file.is_open())
            return false;
    
        file << text;
        file.close();
        return true;
    }
    
    bool FileStream::WriteWholeBinary(const char* filePath, std::size_t sizeInByte, const char* data)
    {
        std::ofstream file{ filePath, std::ios::binary };
        if (!file.is_open())
            return false;
    
        file.write(data, sizeInByte);
        file.close();
        return true;
    }
}


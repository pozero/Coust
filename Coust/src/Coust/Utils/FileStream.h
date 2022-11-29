#pragma once

#include <fstream>
#include <vector>

namespace Coust
{
	class FileStream
	{
	public:
		static std::string ReadWholeText(const char* filePath);

        template<typename T>
        static std::vector<T> ReadWholeBinary(const char* filePath)
        {
            std::ifstream file{ filePath, std::ios::ate | std::ios::binary };
            if (!file.is_open())
                return {};
        
            size_t fileSize = (size_t) file.tellg();
            std::vector<T> buffer(fileSize / sizeof(T));
        
            file.seekg(0);
            file.read((char*)buffer.data(), fileSize);
            file.close();
        
            return buffer;
        }

		static bool WriteWholeText(const char* filePath, const char* text);

		static bool WriteWholeBinary(const char* filePath, std::size_t sizeInByte, const char* data);
	};
}

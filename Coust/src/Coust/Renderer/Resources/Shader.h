#pragma once

#include "Coust/Utils/FilePath.h"
#include "Coust/Utils/FileStream.h"
#include "Coust/Core/Logger.h"

#include <shaderc/shaderc.hpp>

#include <utility>
#include <vector>


namespace Coust
{
	class Shader
	{
	public:
		Shader() = delete;
		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;

		Shader(const FilePath& path, const std::initializer_list<std::pair<std::string, std::string>>& macroes);
		~Shader();

        void SaveFile();

	private:
		class Includer : public shaderc::CompileOptions::IncluderInterface
		{
        public:
			~Includer() override = default;

			shaderc_include_result *GetInclude( const char *requested_source, shaderc_include_type type, 
												const char *requesting_source, size_t include_depth ) override;

			void ReleaseInclude(shaderc_include_result *include_result) override;

        private:
            struct IncludedFileInfo
            {
                std::string fullFilePath;
                std::string fileContent;
            };
		};

	private:
	    shaderc::CompileOptions m_Options;
	    shaderc::Compiler m_Compiler;

        std::string m_Preprocessed{};
        std::string m_Assembly{};
		std::vector<uint32_t> m_ByteCode{};

        FilePath m_Path{};

	private:
		std::string GlslToPreprocessed(const char* filePath, const std::string& source, shaderc_shader_kind kind);

		std::string PreProcessedToAssembly(const char* filePath, const std::string& source, shaderc_shader_kind kind);

		std::vector<uint32_t> AssemblyToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind);

        std::vector<uint32_t> GlslToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind);
	};
}

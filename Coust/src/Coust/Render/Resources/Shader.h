#pragma once

#include "Coust/Utils/FileSystem.h"
#include "Coust/Core/Logger.h"

#include <filesystem>
#include <shaderc/shaderc.hpp>

#include <utility>
#include <vector>


namespace Coust::Render
{
	class Shader
	{
	public:
		enum class Type
		{
			UNDEFINED,
			VERTEX,
			FRAGMENT,
			TESSELLATION_CONTROL,
			TESSELLATION_EVALUATION,
			GEOMETRY,
			COMPUTE,
		};
	public:
		Shader() = delete;
		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;

		Shader(const std::filesystem::path& path, const std::vector<const char*>& macroes);
		~Shader();

		const std::vector<uint32_t>& GetByteCode() const { return m_ByteCode; }

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

	public:
		Type m_Type = Type::UNDEFINED;

		std::string m_SourceFile;
		bool m_FlushToCache = false;

	private:
	    shaderc::CompileOptions m_Options;
	    shaderc::Compiler m_Compiler;

        std::string m_Preprocessed{};
        std::string m_Assembly{};
		std::vector<uint32_t> m_ByteCode{};

        std::filesystem::path m_Path{};

	private:
		std::string GlslToPreprocessed(const char* filePath, const std::string& source, shaderc_shader_kind kind);

		std::string PreProcessedToAssembly(const char* filePath, const std::string& source, shaderc_shader_kind kind);

		std::vector<uint32_t> AssemblyToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind);

        std::vector<uint32_t> GlslToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind);
	};
}

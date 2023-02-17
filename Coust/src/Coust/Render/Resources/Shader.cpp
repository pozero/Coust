#include "Coust/Utils/FileSystem.h"
#include "pch.h"

#include "Coust/Render/Resources/Shader.h"
#include <filesystem>

namespace Coust::Render
{
	inline bool IsSpirV(const char* name)
	{
		const char* ext = nullptr;
		for (const char* p = name; *p != '\0'; ++p)
		{
			if (*p == '.')
				ext = p;
	    }
		if (ext)
			return std::strcmp(++ext, "spv") == 0;
		return false;
	}

    inline bool BeginWith(const char* str, const char* prefix)
    {
        while (*prefix != '\0')
        {
            if (*str != *prefix)
                return false;
            ++str; ++prefix;
        }
        return true;
    }

	inline std::pair<shaderc_shader_kind, Shader::Type> InferShaderKindFromExt(const char* ext)
	{
        if (!ext)
            return { shaderc_shader_kind::shaderc_glsl_infer_from_source, Shader::Type::UNDEFINED };
		++ext;
        if (BeginWith(ext, "frag"))
            return { shaderc_shader_kind::shaderc_glsl_default_fragment_shader, Shader::Type::FRAGMENT };
        else if (BeginWith(ext, "vert"))
            return { shaderc_shader_kind::shaderc_glsl_default_vertex_shader, Shader::Type::VERTEX };
        else if (BeginWith(ext, "geom"))
            return { shaderc_shader_kind::shaderc_glsl_default_geometry_shader, Shader::Type::GEOMETRY };
        else if (BeginWith(ext, "comp"))
            return { shaderc_shader_kind::shaderc_glsl_default_compute_shader, Shader::Type::COMPUTE };
        else if (BeginWith(ext, "tesc"))
            return { shaderc_shader_kind::shaderc_glsl_default_tess_control_shader, Shader::Type::TESSELLATION_CONTROL };
        else if (BeginWith(ext, "tese"))
            return { shaderc_shader_kind::shaderc_glsl_default_tess_evaluation_shader, Shader::Type::TESSELLATION_EVALUATION };
	    // else if (BeginWith(ext, "rchit"))
	    // 	return shaderc_shader_kind::shaderc_glsl_default_closesthit_shader;
	    // else if (BeginWith(ext, "rgen"))
	    // 	return shaderc_shader_kind::shaderc_glsl_default_raygen_shader;
	    // else if (BeginWith(ext, "rmiss"))
	    // 	return shaderc_shader_kind::shaderc_glsl_default_miss_shader;
	    // else if (BeginWith(ext, "rahit"))
	    // 	return shaderc_shader_kind::shaderc_glsl_default_anyhit_shader;
	    // else if (BeginWith(ext, "mesh"))
	    // 	return shaderc_shader_kind::shaderc_glsl_geometry_shader;
	    // else if (BeginWith(ext, "rcall"))
	    // 	return shaderc_shader_kind::shaderc_glsl_default_callable_shader;
        return { shaderc_shader_kind::shaderc_glsl_infer_from_source, Shader::Type::UNDEFINED };
	}

	Shader::Shader(const std::filesystem::path& path, const std::vector<const char*>& macroes)
        : m_SourceFile(path.string()), m_Path(path) 
	{
	    m_Options.SetIncluder(std::make_unique<Includer>());
	    m_Options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
	    m_Options.SetSourceLanguage(shaderc_source_language_glsl);
	    m_Options.SetOptimizationLevel(shaderc_optimization_level_performance);
	    m_Options.AddMacroDefinition("__VK_GLSL__", "1");

	    for (const char* macro : macroes)
	    {
            const char* p = macro;
            const char* name = macro, * value = nullptr;
            std::size_t nameSize = 0, valueSize = 0;

            while (*p != '\0' && !std::isblank(*p)) ++p;
            nameSize = p - name;

            while (*p != '\0')
            {
                ++p;
                if (std::isalnum(*p))
                {
                    value = p;
                    break;
                }
            }

            while (*p != '\0') ++p;
            if (value)
                valueSize = p - value;

            m_Options.AddMacroDefinition(name, nameSize, value, valueSize);
	    }

	    const auto kind = InferShaderKindFromExt(path.filename().extension().string().c_str());
        m_Type = kind.second;

        if (!GlobalContext::Get().GetFileSystem().GetCache(path.string(), m_ByteCode))
	    {
            m_FlushToCache = true;
            std::string source{};
	    	FileSystem::ReadWholeText(path, source);
            m_Preprocessed = GlslToPreprocessed(path.string().c_str(), source, kind.first);
            m_Assembly = PreProcessedToAssembly(path.string().c_str(), m_Preprocessed.c_str(), kind.first);
            m_ByteCode = AssemblyToByteCode(path.string().c_str(), m_Assembly.c_str(), kind.first);
	    }
	}

	Shader::~Shader()
	{
        if (m_FlushToCache)
        {
            GlobalContext::Get().GetFileSystem().AddCache(m_SourceFile, m_ByteCode, true);
        }
	}

	std::string Shader::GlslToPreprocessed(const char* filePath, const std::string& source, shaderc_shader_kind kind)
	{
        auto result = m_Compiler.PreprocessGlsl(source, kind, filePath, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR("GLSL Preprocessor: {}", result.GetErrorMessage().data());
            return std::string{};
        }

        return std::string{result.cbegin(), result.cend()};
	}

	std::string Shader::PreProcessedToAssembly(const char* filePath, const std::string& source, shaderc_shader_kind kind)
	{
        auto result = m_Compiler.CompileGlslToSpvAssembly(source, kind, filePath, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR("GLSL Compiler: {}", result.GetErrorMessage().data());
            return std::string{};
        }

        return std::string{result.cbegin(), result.cend()};
	}

	std::vector<uint32_t> Shader::AssemblyToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind)
	{
        auto result = m_Compiler.AssembleToSpv(source, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR("GLSL Assembler: {}", result.GetErrorMessage().data());
            return {};
        }
        return std::vector<uint32_t>{result.cbegin(), result.cend()};
	}

    std::vector<uint32_t> Shader::GlslToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind)
    {
        auto result = m_Compiler.CompileGlslToSpv(source, kind, filePath, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR(result.GetErrorMessage().data());
            return {};
        }
        return std::vector<uint32_t>{result.cbegin(), result.cend()};
    }

	shaderc_include_result* Shader::Includer::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth)
	{
        IncludedFileInfo* info = new IncludedFileInfo{};
        std::filesystem::path includePath{ requested_source };
        if (includePath.is_absolute())
            info->fullFilePath = includePath.string();
        else
        {
            includePath = std::filesystem::path{ requesting_source };
            includePath = includePath.parent_path();
            includePath /= requested_source;
            info->fullFilePath = includePath.string();
        }
        
        FileSystem::ReadWholeText(info->fullFilePath.c_str(), info->fileContent);
        
        shaderc_include_result* result = new shaderc_include_result{
            .source_name = info->fullFilePath.c_str(),
            .source_name_length = info->fullFilePath.length(),
            .content = info->fileContent.c_str(),
            .content_length = info->fileContent.length(),
            .user_data = (void*) info,
        };

        return result;
	}

	void Shader::Includer::ReleaseInclude(shaderc_include_result* include_result)
	{
        IncludedFileInfo* info = (IncludedFileInfo*) include_result->user_data;
        delete info;
	}
}

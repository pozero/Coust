#include "pch.h"

#include "Coust/Renderer/Resources/Shader.h"

namespace Coust
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

	inline shaderc_shader_kind InferShaderKindFromName(const char* name)
	{
		const char* ext = nullptr;
		for (const char* p = name; *p != '\0'; ++p)
		{
			if (*p == '.')
				ext = p;
		}
		if (!ext)
			return shaderc_shader_kind::shaderc_glsl_infer_from_source;

		++ext;
		if (std::strcmp(ext, "frag") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_fragment_shader;
		else if (std::strcmp(ext, "vert") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_vertex_shader;
		else if (std::strcmp(ext, "geom") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_geometry_shader;
		else if (std::strcmp(ext, "rchit") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_closesthit_shader;
		else if (std::strcmp(ext, "rgen") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_raygen_shader;
		else if (std::strcmp(ext, "rmiss") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_miss_shader;
		else if (std::strcmp(ext, "rahit") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_anyhit_shader;
		else if (std::strcmp(ext, "mesh") == 0)
			return shaderc_shader_kind::shaderc_glsl_geometry_shader;
		else if (std::strcmp(ext, "rcall") == 0)
			return shaderc_shader_kind::shaderc_glsl_default_callable_shader;
		return shaderc_glsl_infer_from_source;
	}

	Shader::Shader(const FilePath& path, const std::initializer_list<std::pair<std::string, std::string>>& macroes)
        : m_Path(path)
	{
	    m_Options.SetIncluder(std::make_unique<Includer>());
	    m_Options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
	    m_Options.SetSourceLanguage(shaderc_source_language_glsl);
	    m_Options.SetOptimizationLevel(shaderc_optimization_level_performance);
	    m_Options.AddMacroDefinition("__VK_GLSL__", "1");
	    for (const auto& p : macroes)
	    {
	    	m_Options.AddMacroDefinition(p.first, p.second);
	    }

	    if (IsSpirV(path.GetName()))
        {
            m_ByteCode = FileStream::ReadWholeBinary<uint32_t>(path.Get());
        }
	    else
	    {
	    	const auto kind = InferShaderKindFromName(path.GetName());
	    	const auto source = FileStream::ReadWholeText(path.Get());
            m_Preprocessed = GlslToPreprocessed(path.Get(), source, kind);
            m_Assembly = PreProcessedToAssembly(path.Get(), m_Preprocessed.c_str(), kind);
            m_ByteCode = AssemblyToByteCode(path.Get(), m_Assembly.c_str(), kind);
	    }
	}

	Shader::~Shader()
	{
	}

    void Shader::SaveFile()
    {
        FilePath saveDir{m_Path};
        saveDir.GoBack().AddDirectory("build");
        COUST_CORE_TRACE(saveDir.Get());
        std::string sourceFileName{m_Path.GetName()};

        if (m_Preprocessed.length() > 0)
        {
            std::string name = "Preprocessed" + sourceFileName;
            saveDir.AddFile(name.c_str());
            COUST_CORE_TRACE(saveDir.Get());
            FileStream::WriteWholeText(saveDir.Get(), m_Preprocessed.c_str());
            saveDir.GoBack();
        }

        if (m_Assembly.length() > 0)
        {
            std::string name = sourceFileName + ".asm";
            saveDir.AddFile(name.c_str());
            COUST_CORE_TRACE(saveDir.Get());
            FileStream::WriteWholeText(saveDir.Get(), m_Assembly.c_str());
            saveDir.GoBack();
        }

        if (m_ByteCode.size() > 0)
        {
            std::string name = sourceFileName + ".spv";
            saveDir.AddFile(name.c_str());
            COUST_CORE_TRACE(saveDir.Get());
            FileStream::WriteWholeBinary(saveDir.Get(), m_ByteCode.size() * sizeof(uint32_t), (const char*)m_ByteCode.data());
        }
    }

	std::string Shader::GlslToPreprocessed(const char* filePath, const std::string& source, shaderc_shader_kind kind)
	{
        auto result = m_Compiler.PreprocessGlsl(source, kind, filePath, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR(result.GetErrorMessage().data());
            return std::string{};
        }

        return std::string{result.cbegin(), result.cend()};
	}

	std::string Shader::PreProcessedToAssembly(const char* filePath, const std::string& source, shaderc_shader_kind kind)
	{
        auto result = m_Compiler.CompileGlslToSpvAssembly(source, kind, filePath, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR(result.GetErrorMessage().data());
            return std::string{};
        }

        return std::string{result.cbegin(), result.cend()};
	}

	std::vector<uint32_t> Shader::AssemblyToByteCode(const char* filePath, const std::string& source, shaderc_shader_kind kind)
	{
        auto result = m_Compiler.AssembleToSpv(source, m_Options);
        if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
        {
        	COUST_CORE_ERROR(result.GetErrorMessage().data());
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
        if (!FilePath::IsRelativePath(requested_source))
            info->fullFilePath = std::string{requested_source};
        else
        {
            FilePath f{requesting_source};
            f.GoBack().AddFile(requested_source);
            info->fullFilePath = std::string{f.Get()};
        }
        
        info->fileContent = FileStream::ReadWholeText(info->fullFilePath.c_str());
        
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

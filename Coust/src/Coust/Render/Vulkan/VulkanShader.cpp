#include "pch.h"

#include <optional>

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanShader.h"

#include "Coust/Utils/Hash.h"
#include "Coust/Utils/Macro.h"
#include "Coust/Utils/FileSystem.h"

namespace Coust::Render::VK 
{
	class Includer : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		~Includer() override = default;

		shaderc_include_result *GetInclude( const char *requested_source, shaderc_include_type type, 
											const char *requesting_source, size_t include_depth ) override
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

		void ReleaseInclude(shaderc_include_result *include_result) override
		{
		    IncludedFileInfo* info = (IncludedFileInfo*) include_result->user_data;
		    delete info;
		}

	private:
		struct IncludedFileInfo
		{
			std::string fullFilePath;
			std::string fileContent;
		};
	};

    size_t ShaderSource::GetHash() const noexcept
    {
        Hash::HashFn<std::string> sHasher{};
        size_t hash = sHasher(m_SourceFilePath.string());
        if (GetMacros().size() > 0)
        {
            for (const auto& p : GetMacros())
            {
                Hash::Combine(hash, p.first);
                Hash::Combine(hash, p.second);
            }
        }
        return hash;
    }

    const std::string& ShaderSource::GetCode()
    {
        if (m_SourceCode.empty())
            FileSystem::ReadWholeText(m_SourceFilePath, m_SourceCode);
        return m_SourceCode;
    }

    const std::filesystem::path& ShaderSource::GetPath() const noexcept { return m_SourceFilePath; }
    
    const std::unordered_map<std::string, std::string>& ShaderSource::GetMacros() const noexcept { return m_Macros; }
    
    const std::unordered_map<std::string, size_t>& ShaderSource::GetDesiredDynamicBufferSize() const noexcept { return m_DesiredDynamicBufferSize; }
    
    void ShaderSource::AddMacro(const std::string& name, const std::string& value) { m_Macros[name] = value; }
    
    void ShaderSource::DeleteMacroByName(const std::string& name)
    {
        const auto& iter = m_Macros.find(name);
        m_Macros.erase(iter);
    }
    
    void ShaderSource::SetDynamicBufferSize(std::string_view name, size_t size) 
    { 
        m_DesiredDynamicBufferSize[std::string{ name }] = size; 
    }
	
	ShaderByteCode::~ShaderByteCode()
	{
		if (!ByteCode.empty())
		{
			GlobalContext::Get().GetFileSystem().AddCache(SourcePath, CacheTag, ByteCode, true);
		}
	}
	
	inline ShaderResourceBaseType GetShaderResourceBastType(spirv_cross::SPIRType::BaseType baseType)
    {
        switch (baseType)
        {
            case spirv_cross::SPIRType::BaseType::Boolean:  return ShaderResourceBaseType::Bool;
            case spirv_cross::SPIRType::BaseType::SByte:    return ShaderResourceBaseType::Int8;
            case spirv_cross::SPIRType::BaseType::UByte:    return ShaderResourceBaseType::UInt8;
            case spirv_cross::SPIRType::BaseType::Short:    return ShaderResourceBaseType::Int16;
            case spirv_cross::SPIRType::BaseType::UShort:   return ShaderResourceBaseType::UInt16;
            case spirv_cross::SPIRType::BaseType::Int:      return ShaderResourceBaseType::Int32;
            case spirv_cross::SPIRType::BaseType::UInt:     return ShaderResourceBaseType::UInt32;
            case spirv_cross::SPIRType::BaseType::Int64:    return ShaderResourceBaseType::Int64;
            case spirv_cross::SPIRType::BaseType::UInt64:   return ShaderResourceBaseType::UInt64;
            case spirv_cross::SPIRType::BaseType::Half:     return ShaderResourceBaseType::Half;
            case spirv_cross::SPIRType::BaseType::Float:    return ShaderResourceBaseType::Float;
            case spirv_cross::SPIRType::BaseType::Double:   return ShaderResourceBaseType::Double;
            case spirv_cross::SPIRType::BaseType::Struct:   return ShaderResourceBaseType::Struct;
            default:                                        return ShaderResourceBaseType::All;
        }
    }

    inline uint32_t GetShaderResourceBaseTypeSize(ShaderResourceBaseType baseType)
    {
        switch (baseType)
        {
            case ShaderResourceBaseType::Int8:
            case ShaderResourceBaseType::UInt8:
                return 1;
            case ShaderResourceBaseType::Int16:
            case ShaderResourceBaseType::UInt16:
            case ShaderResourceBaseType::Half:
                return 2;
            case ShaderResourceBaseType::Bool:
            case ShaderResourceBaseType::Int32:
            case ShaderResourceBaseType::UInt32:
            case ShaderResourceBaseType::Float:
                return 4;
            case ShaderResourceBaseType::Int64:
            case ShaderResourceBaseType::UInt64:
            case ShaderResourceBaseType::Double:
                return 8;
            default:
                return 0;
        }
    }
    
    inline uint32_t GetShaderResourceBaseTypeSize(spirv_cross::SPIRType::BaseType baseType)
    {
        switch (baseType)
        {
            case spirv_cross::SPIRType::BaseType::SByte:   
            case spirv_cross::SPIRType::BaseType::UByte:   
                return 1;
            case spirv_cross::SPIRType::BaseType::Short:   
            case spirv_cross::SPIRType::BaseType::UShort:  
            case spirv_cross::SPIRType::BaseType::Half:    
                return 2;
            case spirv_cross::SPIRType::BaseType::Boolean: 
            case spirv_cross::SPIRType::BaseType::Int:     
            case spirv_cross::SPIRType::BaseType::UInt:    
            case spirv_cross::SPIRType::BaseType::Float: 
                return 4;
            case spirv_cross::SPIRType::BaseType::Int64:   
            case spirv_cross::SPIRType::BaseType::UInt64: 
            case spirv_cross::SPIRType::BaseType::Double:
                return 8;
            default:                                       
                return 0;
        }
    }
    
    ShaderResourceMember* ParseResourceMembers(const spirv_cross::CompilerGLSL& compiler, const spirv_cross::SPIRType& spirvType)
    {
        ShaderResourceMember* pFirstMember = nullptr;
        ShaderResourceMember* pPrevMember = nullptr;
        for (size_t i = 0u; i < spirvType.member_types.size(); ++i)
        {
            const auto& memType = compiler.get_type(spirvType.member_types[i]);
            
            ShaderResourceBaseType baseType = GetShaderResourceBastType(memType.basetype);
            // basetype isn't supported
            if (baseType == ShaderResourceBaseType::All)
                continue;
            
            ShaderResourceMember* mem = new ShaderResourceMember{};
            mem->Name = compiler.get_member_name(spirvType.self, uint32_t(i));
            mem->BaseType = baseType;
            mem->Offset = compiler.type_struct_member_offset(spirvType, uint32_t(i));
            mem->Size = compiler.get_declared_struct_member_size(spirvType, uint32_t(i));
            mem->VecSize = memType.vecsize;
            mem->Columns = memType.columns;
            mem->ArraySize = (memType.array.size() == 0) ? 1 : memType.array[0];
            
            if (!pFirstMember)
                pFirstMember = mem;
            if (pPrevMember)
                pPrevMember->pNextMember = mem;
            
            pPrevMember = mem;
            
            if (baseType == ShaderResourceBaseType::Struct)
                mem->pMembers = ParseResourceMembers(compiler, memType);
        }
        return pFirstMember;
    }
    
    inline void CleanResourceMembersInfo(ShaderResourceMember* pMemberInfo)
    {
        if (!pMemberInfo)
            return;
        
        std::queue<ShaderResourceMember*> queue{};
        queue.push(pMemberInfo);
    
        while (!queue.empty())
        {
            const auto& topMem = queue.front();
            if (topMem->pMembers)
                queue.push(topMem->pMembers);
            
            if (topMem->pNextMember)
                queue.push(topMem->pNextMember);
            
            delete topMem;
            queue.pop();
        }
    }
	
	// `basetype` is queried for `VertexInputAttribute.format`
    inline void ReadResourceBaseType(const spirv_cross::CompilerGLSL& compiler, 
									 const spirv_cross::Resource& resource, 
									 ShaderResource& out_ShaderResources)
    {
        const auto& spirvType = compiler.get_type_from_variable(resource.id);
        out_ShaderResources.BaseType = GetShaderResourceBastType(spirvType.basetype);
    }
    
    template <spv::Decoration decoration>
    inline void ReadResourceDecoration(const spirv_cross::CompilerGLSL& compiler, 
									   const spirv_cross::Resource& resource, 
									   ShaderResource& out_ShaderResources)
    {
		COUST_CORE_WARN("ReadResourceDecoration<{}>(*) Not Implemented!", ToString(decoration));
    }
    
    template <>
    inline void ReadResourceDecoration<spv::DecorationLocation>(const spirv_cross::CompilerGLSL& compiler, 
																const spirv_cross::Resource& resource, 
																ShaderResource& out_ShaderResources)
    {
        out_ShaderResources.Location = compiler.get_decoration(resource.id, spv::DecorationLocation);
    }
    
    template <>
    inline void ReadResourceDecoration<spv::DecorationDescriptorSet>(const spirv_cross::CompilerGLSL& compiler, 
																	 const spirv_cross::Resource& resource, 
																	 ShaderResource& out_ShaderResources)
    {
        out_ShaderResources.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    }
    
    template <>
    inline void ReadResourceDecoration<spv::DecorationBinding>(const spirv_cross::CompilerGLSL& compiler, 
															   const spirv_cross::Resource& resource, 
															   ShaderResource& out_ShaderResources)
    {
        out_ShaderResources.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
    }
    
    template <>
    inline void ReadResourceDecoration<spv::DecorationInputAttachmentIndex>(const spirv_cross::CompilerGLSL& compiler, 
																			const spirv_cross::Resource& resource, 
																			ShaderResource& out_ShaderResources)
    {
        out_ShaderResources.InputAttachmentIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
    }
    
    template <>
    inline void ReadResourceDecoration<spv::DecorationNonReadable>(const spirv_cross::CompilerGLSL& compiler, 
																   const spirv_cross::Resource& resource, 
																   ShaderResource& out_ShaderResources)
    {
        // Note: DecorationNonReadable / DecorationNonWritable should be obtained through `get_buffer_block_flags` instead of `get_decoration`
        auto flag = compiler.get_buffer_block_flags(resource.id);
        if (flag.get(spv::DecorationNonReadable))
            out_ShaderResources.Access &= ~VK_ACCESS_SHADER_READ_BIT;
    }
    
    template <>
    inline void ReadResourceDecoration<spv::DecorationNonWritable>(const spirv_cross::CompilerGLSL& compiler, 
																   const spirv_cross::Resource& resource, 
																   ShaderResource& out_ShaderResources)
    {
        // Note: DecorationNonReadable / DecorationNonWritable should be obtained through `get_buffer_block_flags` instead of `get_decoration`
        auto flag = compiler.get_buffer_block_flags(resource.id);
        if (flag.get(spv::DecorationNonWritable))
            out_ShaderResources.Access &= ~VK_ACCESS_SHADER_WRITE_BIT;
    }
    
    // `vecsize` is queried for `VertexInputAttribute.format`
    inline void ReadResourceVecSize(const spirv_cross::CompilerGLSL& compiler, 
									const spirv_cross::Resource& resource, 
									ShaderResource& out_ShaderResources)
    {
        const auto& spirvType = compiler.get_type_from_variable(resource.id);
        out_ShaderResources.VecSize = spirvType.vecsize;
        out_ShaderResources.Columns = spirvType.columns;
    }
    
    // `arraysize` is queried for `VkDescriptorSetLayoutBinding.descriptorCount`
    inline void ReadResourceArraySize(const spirv_cross::CompilerGLSL& compiler, 
									  const spirv_cross::Resource& resource, 
									  ShaderResource& out_ShaderResources)
    {
        const auto& spirvType = compiler.get_type_from_variable(resource.id);
        out_ShaderResources.ArraySize = spirvType.array.size() > 0 ? spirvType.array[0] : 1;
    }
	
	inline void ReadResourceSize(const spirv_cross::CompilerGLSL& compiler, 
								 const spirv_cross::Resource& resource, 
								 const std::unordered_map<std::string, size_t>& desiredRuntimeSize, 
								 ShaderResource& out_ShaderResources)
    {
        const auto& spirvType = compiler.get_type_from_variable(resource.id);
        size_t arraySize = 0;
        if (desiredRuntimeSize.count(resource.name) > 0)
            arraySize = desiredRuntimeSize.at(resource.name);
        out_ShaderResources.Size = ToU32(compiler.get_declared_struct_size_runtime_array(spirvType, arraySize));
    }

	inline void ReadConstantBaseTypeAndSize(const spirv_cross::CompilerGLSL& compiler, 
                                 const spirv_cross::SpecializationConstant& resource,
								 const std::unordered_map<std::string, size_t>& desiredRuntimeSize, 
								 ShaderResource& out_ShaderResources)
    {
        const auto& constant = compiler.get_constant(resource.id);
        const auto& spirvType = compiler.get_type(constant.constant_type);
        auto baseType = spirvType.basetype;
        out_ShaderResources.BaseType = GetShaderResourceBastType(baseType);
        out_ShaderResources.Size = GetShaderResourceBaseTypeSize(baseType);
    }
    
    template <ShaderResourceType Type>
    inline void ReadShaderResource(const spirv_cross::CompilerGLSL& compiler, 
								   const spirv_cross::ShaderResources& resources, 
								   VkShaderStageFlagBits stage, 
                                    const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
								   std::vector<ShaderResource>& out_ShaderResources)
    {
        COUST_CORE_WARN("ReadShaderResource<{}> Not implemented!", ToString(Type));
    }

 	template <>
    inline void ReadShaderResource<ShaderResourceType::Input>(const spirv_cross::CompilerGLSL& compiler, 
                                                              const spirv_cross::ShaderResources& resources, 
                                                              VkShaderStageFlagBits stage, 
                                                              const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                              std::vector<ShaderResource>& out_ShaderResources)
    {
        for (auto& res : resources.stage_inputs)
        {
            ShaderResource out_Res 
            {
                .Name = res.name,
                .Type = ShaderResourceType::Input,
            };
            out_Res.Stage |= stage;
            
            ReadResourceBaseType(compiler, res, out_Res);
            ReadResourceVecSize(compiler, res, out_Res);
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationLocation>(compiler, res, out_Res);
        
            out_ShaderResources.push_back(std::move(out_Res));
        }
    }

 	template <>
    inline void ReadShaderResource<ShaderResourceType::InputAttachment>(const spirv_cross::CompilerGLSL& compiler, 
                                                                        const spirv_cross::ShaderResources& resources, 
                                                                        VkShaderStageFlagBits stage, 
                                                                        const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                        std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.subpass_inputs)
       {
            ShaderResource out_Res 
            {
                 .Name = res.name,
                 .Type = ShaderResourceType::InputAttachment,
                 .Access = VK_ACCESS_SHADER_READ_BIT,
            };
            out_Res.Stage |= stage;
           
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationInputAttachmentIndex>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
            
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }

 	template <>
    inline void ReadShaderResource<ShaderResourceType::Output>(const spirv_cross::CompilerGLSL& compiler, 
                                                               const spirv_cross::ShaderResources& resources, 
                                                               VkShaderStageFlagBits stage, 
                                                               const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                               std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.stage_outputs)
       {
            ShaderResource out_Res 
            {
                .Name = res.name,
                .Type = ShaderResourceType::Output,
            };
            out_Res.Stage |= stage;
           
            ReadResourceBaseType(compiler, res, out_Res);
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceVecSize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationLocation>(compiler, res, out_Res);
            
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }

 	template <>
    inline void ReadShaderResource<ShaderResourceType::Image>(const spirv_cross::CompilerGLSL& compiler, 
                                                              const spirv_cross::ShaderResources& resources, 
                                                              VkShaderStageFlagBits stage, 
                                                              const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                              std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.separate_images)
       {
            ShaderResource out_Res 
            {
                .Name = res.name,
                .Type = ShaderResourceType::Image,
                .Access = VK_ACCESS_SHADER_READ_BIT,
            };
            out_Res.Stage |= stage;
           
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
            
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }
    
 	template <>
    inline void ReadShaderResource<ShaderResourceType::ImageAndSampler>(const spirv_cross::CompilerGLSL& compiler, 
                                                                        const spirv_cross::ShaderResources& resources, 
                                                                        VkShaderStageFlagBits stage, 
                                                                        const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                        std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.sampled_images)
       {
            ShaderResource out_Res
            {
                 .Name = res.name,
                 .Type = ShaderResourceType::ImageAndSampler,
                 .Access = VK_ACCESS_SHADER_READ_BIT,
            };
            out_Res.Stage |= stage;
           
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
            
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }
    
 	template <>
    inline void ReadShaderResource<ShaderResourceType::ImageStorage>(const spirv_cross::CompilerGLSL& compiler, 
                                                                     const spirv_cross::ShaderResources& resources, 
                                                                     VkShaderStageFlagBits stage, 
                                                                     const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                     std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.storage_images)
       {
            ShaderResource out_Res 
            {
                .Name = res.name,
                .Type = ShaderResourceType::ImageStorage,
                // Initialization for query later
                .Access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            };
            out_Res.Stage |= stage;
    
            ReadResourceDecoration<spv::DecorationNonReadable>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationNonWritable>(compiler, res, out_Res);
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
    
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }

 	template <>
    inline void ReadShaderResource<ShaderResourceType::Sampler>(const spirv_cross::CompilerGLSL& compiler, 
                                                                     const spirv_cross::ShaderResources& resources, 
                                                                     VkShaderStageFlagBits stage, 
                                                                     const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                     std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.separate_samplers)
       {
            ShaderResource out_Res 
            {
                .Name = res.name,
                .Type = ShaderResourceType::Sampler,
                .Access = VK_ACCESS_SHADER_READ_BIT,
            };
            out_Res.Stage |= stage;
           
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
    
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }
    
 	template <>
    inline void ReadShaderResource<ShaderResourceType::UniformBuffer>(const spirv_cross::CompilerGLSL& compiler, 
                                                                      const spirv_cross::ShaderResources& resources, 
                                                                      VkShaderStageFlagBits stage, 
                                                                      const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                      std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.uniform_buffers)
       {
            ShaderResource out_Res 
            {
                .Name = res.name,
                .Type = ShaderResourceType::UniformBuffer,
                .Access = VK_ACCESS_UNIFORM_READ_BIT,
            };
            out_Res.Stage |= stage;
           
            const auto& spirvType = compiler.get_type_from_variable(res.id);
            out_Res.pMembers = ParseResourceMembers(compiler, spirvType);
            
            ReadResourceSize(compiler, res, desiredDynamicBufferSize, out_Res);
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
    
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }
    
 	template <>
    inline void ReadShaderResource<ShaderResourceType::StorageBuffer>(const spirv_cross::CompilerGLSL& compiler, 
                                                                      const spirv_cross::ShaderResources& resources, 
                                                                      VkShaderStageFlagBits stage, 
                                                                      const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                      std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.storage_buffers)
       {
            ShaderResource out_Res 
            {
                .Name = res.name,
                // Initialization for query later
                .Type = ShaderResourceType::StorageBuffer,
                .Access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            };
            out_Res.Stage |= stage;
           
            const auto& spirvType = compiler.get_type_from_variable(res.id);
            out_Res.pMembers = ParseResourceMembers(compiler, spirvType);
            
            ReadResourceDecoration<spv::DecorationNonReadable>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationNonWritable>(compiler, res, out_Res);
            ReadResourceSize(compiler, res, desiredDynamicBufferSize, out_Res);
            ReadResourceArraySize(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationDescriptorSet>(compiler, res, out_Res);
            ReadResourceDecoration<spv::DecorationBinding>(compiler, res, out_Res);
    
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }
    

 	template <>
    inline void ReadShaderResource<ShaderResourceType::PushConstant>(const spirv_cross::CompilerGLSL& compiler, 
                                                                     const spirv_cross::ShaderResources& resources, 
                                                                     VkShaderStageFlagBits stage, 
                                                                     const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                     std::vector<ShaderResource>& out_ShaderResources)
    {
       for (auto& res : resources.push_constant_buffers)
       {
            const auto& spirvType = compiler.get_type_from_variable(res.id);
            uint32_t offset = std::numeric_limits<uint32_t>::max();
            // Get the start offset (offset of the whole struct in other words) of the push constant buffer
            for (size_t i = 0u; i < spirvType.member_types.size(); ++i)
            {
                uint32_t memberOffset = compiler.get_member_decoration(spirvType.self, uint32_t(i), spv::DecorationOffset);
                offset = std::min(offset, memberOffset);
            }
            
            ShaderResource out_Res
            {
                .Name = res.name,
                .Type = ShaderResourceType::PushConstant,
    
                .Offset = offset,
            };
            out_Res.Stage |= stage;
    
            ReadResourceSize(compiler, res, desiredDynamicBufferSize, out_Res);
            out_Res.Size -= out_Res.Offset;
            
            out_Res.pMembers = ParseResourceMembers(compiler, spirvType);
            
            out_ShaderResources.push_back(std::move(out_Res));
       }
    }
    
 	template <>
    inline void ReadShaderResource<ShaderResourceType::SpecializationConstant>(const spirv_cross::CompilerGLSL& compiler, 
                                                                     const spirv_cross::ShaderResources& resources, 
                                                                     VkShaderStageFlagBits stage, 
                                                                     const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                                                     std::vector<ShaderResource>& out_ShaderResources)
    {
        auto specializationConstant = compiler.get_specialization_constants();
        
        for (auto& res : specializationConstant)
        {
            ShaderResource out_Res 
            {
                .Name = compiler.get_name(res.id),
                .Type = ShaderResourceType::SpecializationConstant,
                .ConstantId = res.constant_id,
            };
            out_Res.Stage |= stage;
            ReadConstantBaseTypeAndSize(compiler, res, desiredDynamicBufferSize, out_Res);
            
            out_ShaderResources.push_back(std::move(out_Res));
        }
    }

    bool SPIRVReflectShaderResource(const std::vector<uint32_t>& spirv, 
                                    VkShaderStageFlagBits stage,
                                    const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                    std::vector<ShaderResource>& out_ShaderResource)
    {
        try
        {
            std::unique_ptr<spirv_cross::CompilerGLSL> compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv);
            auto options = compiler->get_common_options();
            compiler->set_common_options(options);
            
            spirv_cross::ShaderResources resources = compiler->get_shader_resources();

            ReadShaderResource<ShaderResourceType::Input>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);              
            ReadShaderResource<ShaderResourceType::InputAttachment>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);    
            ReadShaderResource<ShaderResourceType::Output>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);             
            ReadShaderResource<ShaderResourceType::Image>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);              
            ReadShaderResource<ShaderResourceType::Sampler>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);            
            ReadShaderResource<ShaderResourceType::ImageAndSampler>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);    
            ReadShaderResource<ShaderResourceType::ImageStorage>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);       
            ReadShaderResource<ShaderResourceType::UniformBuffer>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);      
            ReadShaderResource<ShaderResourceType::StorageBuffer>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);      
            ReadShaderResource<ShaderResourceType::PushConstant>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);       
            ReadShaderResource<ShaderResourceType::SpecializationConstant>(*compiler, resources, stage, desiredDynamicBufferSize, out_ShaderResource);       
        }
        catch (const std::exception& e)
        {
            COUST_CORE_ERROR("SPIRV Reflection Failed: {}", e.what());
            return false;
        }

        return true;
    }

    inline shaderc_env_version GetShadercEnvVersion(uint32_t vulkanVer)
    {
        switch (vulkanVer)
        {
            case VK_API_VERSION_1_0:        return shaderc_env_version_vulkan_1_0;
            case VK_API_VERSION_1_1:        return shaderc_env_version_vulkan_1_1;
            case VK_API_VERSION_1_2:        return shaderc_env_version_vulkan_1_2;
            case VK_API_VERSION_1_3:        return shaderc_env_version_vulkan_1_3;
            default:                        return shaderc_env_version_vulkan_1_3;
        }
    }
    
    inline shaderc_shader_kind GetShaderKind(VkShaderStageFlagBits stage)
    {
        switch(stage)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:                        return shaderc_glsl_default_vertex_shader;
            case VK_SHADER_STAGE_FRAGMENT_BIT:                      return shaderc_glsl_default_fragment_shader;
            case VK_SHADER_STAGE_GEOMETRY_BIT:                      return shaderc_glsl_default_geometry_shader;
            case VK_SHADER_STAGE_COMPUTE_BIT:                       return shaderc_glsl_default_compute_shader;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:          return shaderc_glsl_default_tess_control_shader;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:       return shaderc_glsl_default_tess_evaluation_shader;
            default:                                                return shaderc_glsl_infer_from_source;
        }
    }

    void ShaderModule::CollectShaderResources(const std::vector<ShaderModule*>& modules, 
                                              std::vector<ShaderResource>& out_AllShaderResources, 
                                              std::unordered_map<uint32_t, uint64_t>& out_SetToResourceIdxLookup)
    {
        for (const auto m : modules)
        {
            const auto& res = m->GetResource();
            for (const auto& r : res)
            {
                auto iter = std::find_if(out_AllShaderResources.begin(), out_AllShaderResources.end(), 
                    [&r](const ShaderResource& a) -> bool 
                    { 
                        const bool IsNotIOOrConstant = !(a.Type == ShaderResourceType::Input || a.Type == ShaderResourceType::Output || a.Type == ShaderResourceType::SpecializationConstant);
                        const bool HasSameName = a.Name == r.Name;
                        // seems that glsl just 
                        const bool IsSameType = a.Type == r.Type;
                        return IsNotIOOrConstant && HasSameName && IsSameType;
                    });
                // the same resource can be referenced by different shaders, just logical or the stage flag
                if (iter != out_AllShaderResources.end())
                    iter->Stage |= r.Stage;
                else
                    out_AllShaderResources.push_back(r);
            }
        }

        if (out_AllShaderResources.size() > sizeof(decltype(out_SetToResourceIdxLookup.at(0))))
        {
            COUST_CORE_ERROR("There're {} shader resources presenting in this group of shader modules. The value type of `out_SetToResourceIdxLookup` should upscale", out_AllShaderResources.size());
            return;
        }

        for (size_t i = 0; i < out_AllShaderResources.size(); ++ i)
        {
            const auto& r = out_AllShaderResources[i];
            // skip shader resource without bining point
            if (r.Type == ShaderResourceType::Input ||
                r.Type == ShaderResourceType::Output ||
                r.Type == ShaderResourceType::PushConstant ||
                r.Type == ShaderResourceType::SpecializationConstant)
                continue;
            
            if (auto iter = out_SetToResourceIdxLookup.find(r.Set); iter != out_SetToResourceIdxLookup.end())
                iter->second |= (1ll << i);
            else 
                out_SetToResourceIdxLookup[r.Set] = (1ll << i);
        }
    }

#define GENERATE_VK_FORMAT_1(baseType, baseTypeSize) CONCAT(VK_FORMAT_, R, baseTypeSize, _, baseType)
#define GENERATE_VK_FORMAT_2(baseType, baseTypeSize) CONCAT(VK_FORMAT_, R, baseTypeSize, G, baseTypeSize, _, baseType)
#define GENERATE_VK_FORMAT_3(baseType, baseTypeSize) CONCAT(VK_FORMAT_, R, baseTypeSize, G, baseTypeSize, B, baseTypeSize, _, baseType)
#define GENERATE_VK_FORMAT_4(baseType, baseTypeSize) CONCAT(VK_FORMAT_, R, baseTypeSize, G, baseTypeSize, B, baseTypeSize, A, baseTypeSize, _, baseType)
#pragma warning( push )
#pragma warning( disable : 4003 )

    inline VkFormat GetInputResourceFormat(uint32_t vecSize, ShaderResourceBaseType baseType)
    {
        if (vecSize == 4)
        {
            switch (baseType)
            {
                case ShaderResourceBaseType::Int8:      return GENERATE_VK_FORMAT_4(SINT,   8);
                case ShaderResourceBaseType::UInt8:     return GENERATE_VK_FORMAT_4(UINT,   8);
                case ShaderResourceBaseType::Int16:     return GENERATE_VK_FORMAT_4(SINT,   16);
                case ShaderResourceBaseType::UInt16:    return GENERATE_VK_FORMAT_4(UINT,   16);
                case ShaderResourceBaseType::Half:      return GENERATE_VK_FORMAT_4(SFLOAT, 16);
                case ShaderResourceBaseType::Bool:      return GENERATE_VK_FORMAT_4(UINT,   32);
                case ShaderResourceBaseType::Int32:     return GENERATE_VK_FORMAT_4(SINT,   32);
                case ShaderResourceBaseType::UInt32:    return GENERATE_VK_FORMAT_4(UINT,   32);
                case ShaderResourceBaseType::Float:     return GENERATE_VK_FORMAT_4(SFLOAT, 32);
                case ShaderResourceBaseType::Int64:     return GENERATE_VK_FORMAT_4(SINT,   64);
                case ShaderResourceBaseType::UInt64:    return GENERATE_VK_FORMAT_4(UINT,   64);
                case ShaderResourceBaseType::Double:    return GENERATE_VK_FORMAT_4(SFLOAT, 64);
                default:                                return VK_FORMAT_UNDEFINED;
            }
        }
        else if (vecSize == 3)
        {
            switch (baseType)
            {
                case ShaderResourceBaseType::Int8:      return GENERATE_VK_FORMAT_3(SINT,   8);
                case ShaderResourceBaseType::UInt8:     return GENERATE_VK_FORMAT_3(UINT,   8);
                case ShaderResourceBaseType::Int16:     return GENERATE_VK_FORMAT_3(SINT,   16);
                case ShaderResourceBaseType::UInt16:    return GENERATE_VK_FORMAT_3(UINT,   16);
                case ShaderResourceBaseType::Half:      return GENERATE_VK_FORMAT_3(SFLOAT, 16);
                case ShaderResourceBaseType::Bool:      return GENERATE_VK_FORMAT_3(UINT,   32);
                case ShaderResourceBaseType::Int32:     return GENERATE_VK_FORMAT_3(SINT,   32);
                case ShaderResourceBaseType::UInt32:    return GENERATE_VK_FORMAT_3(UINT,   32);
                case ShaderResourceBaseType::Float:     return GENERATE_VK_FORMAT_3(SFLOAT, 32);
                case ShaderResourceBaseType::Int64:     return GENERATE_VK_FORMAT_3(SINT,   64);
                case ShaderResourceBaseType::UInt64:    return GENERATE_VK_FORMAT_3(UINT,   64);
                case ShaderResourceBaseType::Double:    return GENERATE_VK_FORMAT_3(SFLOAT, 64);
                default:                                return VK_FORMAT_UNDEFINED;
            }
        }
        else if (vecSize == 2)
        {
            switch (baseType)
            {
                case ShaderResourceBaseType::Int8:      return GENERATE_VK_FORMAT_2(SINT,   8);
                case ShaderResourceBaseType::UInt8:     return GENERATE_VK_FORMAT_2(UINT,   8);
                case ShaderResourceBaseType::Int16:     return GENERATE_VK_FORMAT_2(SINT,   16);
                case ShaderResourceBaseType::UInt16:    return GENERATE_VK_FORMAT_2(UINT,   16);
                case ShaderResourceBaseType::Half:      return GENERATE_VK_FORMAT_2(SFLOAT, 16);
                case ShaderResourceBaseType::Bool:      return GENERATE_VK_FORMAT_2(UINT,   32);
                case ShaderResourceBaseType::Int32:     return GENERATE_VK_FORMAT_2(SINT,   32);
                case ShaderResourceBaseType::UInt32:    return GENERATE_VK_FORMAT_2(UINT,   32);
                case ShaderResourceBaseType::Float:     return GENERATE_VK_FORMAT_2(SFLOAT, 32);
                case ShaderResourceBaseType::Int64:     return GENERATE_VK_FORMAT_2(SINT,   64);
                case ShaderResourceBaseType::UInt64:    return GENERATE_VK_FORMAT_2(UINT,   64);
                case ShaderResourceBaseType::Double:    return GENERATE_VK_FORMAT_2(SFLOAT, 64);
                default:                                return VK_FORMAT_UNDEFINED;
            }
        }
        else // vecSize == 1
        {
            switch (baseType)
            {
                case ShaderResourceBaseType::Int8:      return GENERATE_VK_FORMAT_1(SINT,   8);
                case ShaderResourceBaseType::UInt8:     return GENERATE_VK_FORMAT_1(UINT,   8);
                case ShaderResourceBaseType::Int16:     return GENERATE_VK_FORMAT_1(SINT,   16);
                case ShaderResourceBaseType::UInt16:    return GENERATE_VK_FORMAT_1(UINT,   16);
                case ShaderResourceBaseType::Half:      return GENERATE_VK_FORMAT_1(SFLOAT, 16);
                case ShaderResourceBaseType::Bool:      return GENERATE_VK_FORMAT_1(UINT,   32);
                case ShaderResourceBaseType::Int32:     return GENERATE_VK_FORMAT_1(SINT,   32);
                case ShaderResourceBaseType::UInt32:    return GENERATE_VK_FORMAT_1(UINT,   32);
                case ShaderResourceBaseType::Float:     return GENERATE_VK_FORMAT_1(SFLOAT, 32);
                case ShaderResourceBaseType::Int64:     return GENERATE_VK_FORMAT_1(SINT,   64);
                case ShaderResourceBaseType::UInt64:    return GENERATE_VK_FORMAT_1(UINT,   64);
                case ShaderResourceBaseType::Double:    return GENERATE_VK_FORMAT_1(SFLOAT, 64);
                default:                                return VK_FORMAT_UNDEFINED;
            }
        }
    }

#undef GENERATE_COMPONENT_1
#undef GENERATE_COMPONENT_2
#undef GENERATE_COMPONENT_3
#undef GENERATE_COMPONENT_4
#undef GENERATE_VK_FORMAT_1
#undef GENERATE_VK_FORMAT_2
#undef GENERATE_VK_FORMAT_3
#undef GENERATE_VK_FORMAT_4
#pragma warning( pop )

    void ShaderModule::CollectShaderInputs(const std::vector<ShaderModule*>& modules,
        uint32_t perInstanceInputMask,  
        std::vector<VkVertexInputBindingDescription>& out_VertexBindingDescriptions,
        std::vector<VkVertexInputAttributeDescription>& out_VertexAttributeDescriptions)
    {
        // For question related to the "binding" of vertex input, the mannual for `vkCmdBindVertexBuffers` says:
        // The values taken from elements i of pBuffers and pOffsets replace the current state for the vertex
        // input binding firstBinding + i, for i in [0, bindingCount). The vertex input binding is updated to start
        // at the offset indicated by pOffsets[i] from the start of the buffer pBuffers[i]. All vertex input
        // attributes that use each of these bindings will use these updated addresses in their address
        // calculations for subsequent drawing commands.

        // And the spec also describes the old-fashion way of binding vertex buffer:
        // Applications can store multiple vertex input attributes interleaved in a single
        // buffer, and use a single vertex input binding to access those attributes.

        // So the binding is just how we gonna bind the vertex buffer. Here we will specify a **UNIQUE** binding for each individual location,
        // which means we will use multiple buffers to store these attribute. This design is very convenient for our caching system,
        // because then we can store these attribute sperately, and reuse them no matter how the vertex shader changes its input format.
        
        // Also, we must take the existence of **component** qualifier into consideration, the OpenGL wiki says:
        // https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Interface_components
        // There are limits on the locations of input and output variables (of all kinds). 
        // These limits are based on an implicit assumption in the API that the resources being passed around are done so as groups of 4-element vectors of data. 
        // This is why a mat2 takes up 2 locations, while a vec4 only takes up 1, even though they both are 4 floats in size. 
        // The mat2 is considered to be passed as two vec4s; the last two components of each vector simply go unused.
        // It is possible to reclaim some of this unused space. To do this, you declare two (or more) variables that use the same location, 
        // but use different components within that location.

        // GLSL code example:
        // layout(location = 0) out vec2 arr1[5];
        // layout(location = 0, component = 2) out vec2 arr2[4]; //Different sizes are fine.
        // layout(location = 4, component = 2) out float val;    //A non-array takes the last two fields from location 4.

        std::vector<ShaderResource> inputResource{};
        std::unordered_set<uint32_t> allLocations{};
        inputResource.reserve(sizeof(perInstanceInputMask));
        for (auto m : modules)
        {
            for (const auto& res : m->GetResource())
            {
                if (res.Type == ShaderResourceType::Input && res.Stage == VK_SHADER_STAGE_VERTEX_BIT)
                {
                    inputResource.push_back(res);
                    allLocations.insert(res.Location);
                }
            }
        }
        std::sort(inputResource.begin(), inputResource.end(), 
            [](const ShaderResource& left, const ShaderResource& right) -> bool
            {
                return left.Location < right.Location;
            });

        for (const auto location : allLocations)
        {
            auto begin = std::find_if(inputResource.begin(), inputResource.end(), 
                [location](decltype(inputResource[0]) r) -> bool
                {
                    return r.Location == location;
                });
            auto end = std::find_if_not(begin, inputResource.end(), 
                [location](decltype(inputResource[0]) r) -> bool
                {
                    return r.Location == location;
                });
            
            uint32_t totalSizeInLocation = 0;
            for (auto iter = begin; iter != end; ++iter)
            {
                const auto& res = *iter;
                out_VertexAttributeDescriptions.push_back(
                VkVertexInputAttributeDescription
                {
                    .location = location,
                    .binding = location,
                    // Here we assume all the type of 1 byte is integer, since we can't get information about whether the data is normalized through spir-v reflection
                    // because the normalized byte isn't part of the standard glsl.
                    .format = GetInputResourceFormat(res.VecSize, res.BaseType),
                    .offset = totalSizeInLocation,
                });
                uint32_t componentSize = res.ArraySize * res.Columns * res.VecSize * GetShaderResourceBaseTypeSize(res.BaseType);
                totalSizeInLocation += componentSize;
            }
            out_VertexBindingDescriptions.push_back(
            VkVertexInputBindingDescription
            {
                .binding = location,
                .stride = totalSizeInLocation,
                .inputRate = ((1u << location) & perInstanceInputMask) == 0 ?
                    VK_VERTEX_INPUT_RATE_VERTEX :
                    VK_VERTEX_INPUT_RATE_INSTANCE,
            });
        }
    }

    ShaderModule::ShaderModule(const ConstructParm& param)
        : Base(param.ctx, VK_NULL_HANDLE), Hashable(param.GetHash()), m_Stage(param.stage), m_Source(param.source)
    {
        Construct(param.ctx);
        if (GetByteCode().size() > 0 && GetResource().size() > 0)
        {
            VkShaderModuleCreateInfo ci
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = GetByteCode().size() * sizeof(uint32_t),
                .pCode = GetByteCode().data(),
            };
            bool succeeded = false;
            VK_REPORT(vkCreateShaderModule(param.ctx.Device, &ci, nullptr, &m_Handle), succeeded);
            if (succeeded)
            {
                if (param.dedicatedName)
                    SetDedicatedDebugName(param.dedicatedName);
                else if (param.scopeName)
                    SetDefaultDebugName(param.scopeName, ToString(param.stage));
                else
                    COUST_CORE_WARN("Shader module created without a debug name");
            }
            else
                m_Handle = VK_NULL_HANDLE;
        }
    }

    ShaderModule::ShaderModule(ShaderModule&& other) noexcept
        : Base(std::forward<Base>(other)),
          Hashable(std::forward<Hashable>(other)),
          m_Stage(other.m_Stage),
          m_Source(std::move(other.m_Source)),
          m_ByteCode(std::move(other.m_ByteCode)),
          m_Resources(std::move(other.m_Resources))
    {
    }
    
    ShaderModule::~ShaderModule()
    {
        for (const auto& r : m_Resources)
        {
            CleanResourceMembersInfo(r.pMembers);
        }
        vkDestroyShaderModule(m_Ctx.Device, m_Handle, nullptr);
    }

    void ShaderModule::Construct(const Context& ctx)
    {
        size_t cacheTag = m_Hash;

        // if we can get the cached byte code, use the byte code
        if (std::vector<uint32_t> byteCode{}; GlobalContext::Get().GetFileSystem().GetCache(m_Source.GetPath().string(), cacheTag, byteCode))
            m_ByteCode = ShaderByteCode{ m_Source.GetPath().string(), std::move(byteCode), false };
        // else compile from source
        else
        {
            shaderc::Compiler compiler{};
            shaderc::CompileOptions options{};
	        options.SetIncluder(std::make_unique<Includer>());
	        options.SetSourceLanguage(shaderc_source_language_glsl);
	        options.SetTargetEnvironment(shaderc_target_env_vulkan, GetShadercEnvVersion(VULKAN_API_VERSION));
	        options.SetOptimizationLevel(shaderc_optimization_level_zero);
	        options.AddMacroDefinition("__VK_GLSL__", "1");
            for (const auto& macro : m_Source.GetMacros())
            {
                options.AddMacroDefinition(macro.first, macro.second);
            }

            auto result = compiler.CompileGlslToSpv(m_Source.GetCode(), GetShaderKind(m_Stage), m_Source.GetPath().string().c_str(), options);
            if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
            {
            	COUST_CORE_ERROR(result.GetErrorMessage().data());
            }
            else
                m_ByteCode = ShaderByteCode{ m_Source.GetPath().string(), std::vector<uint32_t>{result.cbegin(), result.cend()}, true};
        }
        m_ByteCode.CacheTag = cacheTag;
        
        if (!SPIRVReflectShaderResource(m_ByteCode.ByteCode, m_Stage, m_Source.GetDesiredDynamicBufferSize(), m_Resources))
            COUST_CORE_ERROR("Can't reflect SPIR-V");
    }
    
    std::string ShaderModule::GetDisassembledSPIRV()
    {
        std::unique_ptr<spirv_cross::CompilerGLSL> compiler = std::make_unique<spirv_cross::CompilerGLSL>(m_ByteCode.ByteCode);
        return compiler->compile();
    }

    VkShaderStageFlagBits ShaderModule::GetStage() const noexcept { return m_Stage; }

    const std::vector<uint32_t>& ShaderModule::GetByteCode() const noexcept { return m_ByteCode.ByteCode; }

    const std::vector<ShaderResource>& ShaderModule::GetResource() const noexcept { return m_Resources; }
    
    bool ShaderModule::IsValid() const noexcept
    { 
        return m_ByteCode.ByteCode.size() > 0 && 
               m_Resources.size() > 0 && 
               m_Handle != VK_NULL_HANDLE; 
    }

    // Shader module would use the same hash as its source
    size_t ShaderModule::ConstructParm::GetHash() const
    {
        return source.GetHash();
    }

    const char* ToString(ShaderResourceType type)
    {
        switch (type) 
        {
            case ShaderResourceType::Input:                         return "Input";              
            case ShaderResourceType::InputAttachment:               return "InputAttachment";    
            case ShaderResourceType::Output:                        return "Output";             
            case ShaderResourceType::Image:                         return "Image";              
            case ShaderResourceType::Sampler:                       return "Sampler";            
            case ShaderResourceType::ImageAndSampler:               return "ImageAndSampler";    
            case ShaderResourceType::ImageStorage:                  return "ImageStorage";       
            case ShaderResourceType::UniformBuffer:                 return "UniformBuffer";      
            case ShaderResourceType::StorageBuffer:                 return "StorageBuffer";      
            case ShaderResourceType::PushConstant:                  return "PushConstant";       
            case ShaderResourceType::SpecializationConstant:        return "SpecializationConstant";       
            default:                                                return "All";
        }
    }
    
    const char* ToString(ShaderResourceBaseType type)
    {
        switch (type)
        {
            case ShaderResourceBaseType::Bool:      return "Bool";
            case ShaderResourceBaseType::Int8:      return "Int8";
		    case ShaderResourceBaseType::UInt8:     return "UInt8";
		    case ShaderResourceBaseType::Int16:     return "Int16";
		    case ShaderResourceBaseType::UInt16:    return "UInt16";
            case ShaderResourceBaseType::Int32:     return "Int32";
            case ShaderResourceBaseType::UInt32:    return "UInt32";
            case ShaderResourceBaseType::Int64:     return "Int64";
            case ShaderResourceBaseType::UInt64:    return "UInt64";
            case ShaderResourceBaseType::Half:      return "Half";
            case ShaderResourceBaseType::Float:     return "Float";
            case ShaderResourceBaseType::Double:    return "Double";
            case ShaderResourceBaseType::Struct:    return "Struct";
            default:                                return "All";
        }
    }
    
    std::string ToString(const ShaderResourceMember* pMem)
    {
        if (!pMem)
            return{};
    
        std::stringstream ss{};
        
        std::queue<const ShaderResourceMember*> memQueue{};
        std::queue<int> indentQueue{};
        memQueue.push(pMem);
        indentQueue.push(2);
    
        while (!memQueue.empty())
        {
            const auto topMem = memQueue.front();
            int topMemIndet = indentQueue.front();
    
            if (topMem->pMembers)
            {
                memQueue.push(topMem->pMembers);
                indentQueue.push(topMemIndet + 1);
            }
            
            if (topMem->pNextMember)
            {
                memQueue.push(topMem->pNextMember);
                indentQueue.push(topMemIndet);
            }
            
            std::string indent(topMemIndet, '\t');
            ss << indent
               << ToString(topMem->BaseType) << ' '
               << topMem->Name << ' '
               << "Offset: " << topMem->Offset << ' '
               << "Size: " << topMem->Size << ' '
               << "VecSize: " << topMem->VecSize << ' '
               << "Columns: " << topMem->Columns << ' '
               << "ArraySize: " << topMem->ArraySize << '\n';
            
            memQueue.pop();
            indentQueue.pop();
        }
        
        return ss.str();
    }
    
    std::string ToString(const ShaderResource& res)
    {
        std::stringstream ss{};
        std::string indent{ "\n\t" };
        
        ss << ToString(res.Type) << ' '
            << res.Name << indent
            << "Stage: " << ToString<VkShaderStageFlags, VkShaderStageFlagBits>(res.Stage);
    
        switch (res.Type)
        {
            case ShaderResourceType::Input:
            case ShaderResourceType::Output:
                ss << indent << "BaseType: " << ToString(res.BaseType)
                   << indent << "VecSize: " << res.VecSize
                   << indent << "Columns: " << res.Columns
                   << indent << "ArraySize: " << res.ArraySize
                   << indent << "Location: " << res.Location << '\n';
                break;
            case ShaderResourceType::InputAttachment:
                ss << indent << "ArraySize: " << res.ArraySize
                   << indent << "Access: " << ToString<VkAccessFlags, VkAccessFlagBits>(res.Access)
                   << indent << "InputAttachmentIndex: " << res.InputAttachmentIndex
                   << indent << "Set: " << res.Set
                   << indent << "Binding: " << res.Binding << '\n';
                break;
            case ShaderResourceType::Image:
            case ShaderResourceType::Sampler:
            case ShaderResourceType::ImageAndSampler:
            case ShaderResourceType::ImageStorage:
                ss << indent << "ArraySize: " << res.ArraySize
                   << indent << "Access: " << ToString<VkAccessFlags, VkAccessFlagBits>(res.Access)
                   << indent << "Set: " << res.Set
                   << indent << "Binding: " << res.Binding << '\n';
                break;
            case ShaderResourceType::UniformBuffer:
            case ShaderResourceType::StorageBuffer:
                ss << indent << "Size: " << res.Size
                   << indent << "Access: " << ToString<VkAccessFlags, VkAccessFlagBits>(res.Access)
                   << indent << "Array Size: " << res.ArraySize
                   << indent << "Set: " << res.Set
                   << indent << "Binding: " << res.Binding
                   << indent << "Members: " << '\n' << ToString(res.pMembers);
                break;
            case ShaderResourceType::PushConstant:
                ss << indent << "Offset: " << res.Offset
                   << indent << "Size: " << res.Offset
                   << indent << "Members: " << '\n' << ToString(res.pMembers);
                break;
            case ShaderResourceType::SpecializationConstant:
                ss << indent << "Constant ID: " << res.ConstantId
                   << indent << "BaseType: " << ToString(res.BaseType)
                   << indent << "Size: " << res.Size << '\n';
            default:
                break;
        }
        
        return ss.str();
    }

}
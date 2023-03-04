#include "pch.h"

#include <optional>

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanShader.h"

#include "Coust/Utils/FileSystem.h"
#include "Coust/Utils/Hash.h"

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

    size_t ShaderSource::GetHash() const
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
    
    void CleanResourceMembersInfo(ShaderResourceMember* pMemberInfo)
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

    ShaderModule::ShaderModule(ShaderModule::ConstructParm0 param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), m_Stage(param.stage), m_Source(param.source)
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
                SetDefaultDebugName(param.scopeName, ToString(param.stage));
            else
                m_Handle = VK_NULL_HANDLE;
        }
    }

    ShaderModule::ShaderModule(ShaderModule::ConstructParm1 param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), m_Stage(param.stage), m_Source(param.source)
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
                SetDedicatedDebugName(param.dedicatedName);
            else
                m_Handle = VK_NULL_HANDLE;
        }
    }
    
    ShaderModule::~ShaderModule()
    {
        for (const auto& r : m_Resources)
        {
            CleanResourceMembersInfo(r.pMembers);
        }
        vkDestroyShaderModule(m_Device, m_Handle, nullptr);
    }

    void ShaderModule::Construct(const Context& ctx)
    {
        size_t cacheTag = m_Source.GetHash();

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
        
        m_Hash = Hash::HashFn<std::string>{}(
            std::string{ reinterpret_cast<const char*>(m_ByteCode.ByteCode.data()), 
                         reinterpret_cast<const char*>(m_ByteCode.ByteCode.data() + m_ByteCode.ByteCode.size()) });
        m_ByteCode.CacheTag = cacheTag;
        
        if (!SPIRVReflectShaderResource(m_ByteCode.ByteCode, m_Stage, m_Source.GetDesiredDynamicBufferSize(), m_Resources))
            COUST_CORE_ERROR("Can't reflect SPIR-V");
    }
    
    std::string ShaderModule::GetDisassembledSPIRV()
    {
        std::unique_ptr<spirv_cross::CompilerGLSL> compiler = std::make_unique<spirv_cross::CompilerGLSL>(m_ByteCode.ByteCode);
        return compiler->compile();
    }

    void ShaderModule::SetShaderResourceUpdateMode(const std::string& resoureceName, ShaderResourceUpdateMode mode)
    {
        auto iter = std::find_if(m_Resources.begin(), m_Resources.end(), 
            [&](const ShaderResource& res) { return res.Name == resoureceName; });
        
        if (iter != m_Resources.end())
        {
            if (iter->Type == ShaderResourceType::StorageBuffer || iter->Type == ShaderResourceType::UniformBuffer)
                iter->UpdateMode = ShaderResourceUpdateMode::Dynamic;
            else
                COUST_CORE_ERROR("Resource {} in shader module {} is not a buffer", resoureceName, m_DebugName);
        }
        else
            COUST_CORE_ERROR("Can't find resource {} in shader module {}", resoureceName, m_DebugName);
    }

}
#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <queue>
#include <unordered_map>
#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace Coust::Render::VK 
{
    class ShaderSource;
    class ShaderByteCode;
    class ShaderModule;
    struct ShaderResource;
    
    enum class ShaderResourceType
    {
        Input,                      // stage_inputs
        InputAttachment,            // subpass_inputs
        Output,                     // stage_outputs
        Image,                      // separate_images
        Sampler,                    // separate_samplers
        ImageAndSampler,            // sampled_images
        ImageStorage,               // storage_images
        UniformBuffer,              // uniform_buffers
        StorageBuffer,              // storage_buffers
        PushConstant,               // push_constant_buffers
        SpecializationConstant,     // `get_specialization_constants()`
        All,
    };

    enum class ShaderResourceBaseType
    {
        Bool,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Half,
        Float,
        Double,
        Struct,
        All,
    };
    
    enum class ShaderResourceUpdateMode
    {
        Static,
        Dynamic,
        // TODO: Add support to VK_EXT_descriptor_indexing
    };
    
    /**
     * @brief Struct that contains information about members of shader resources, can be used as validation information
     * 
     */
    struct ShaderResourceMember
    {
        std::string Name;
        ShaderResourceBaseType BaseType;
        uint32_t Offset;
        size_t Size;
        uint32_t VecSize;
        uint32_t Columns;
        uint32_t ArraySize;
        
        ShaderResourceMember* pNextMember = nullptr;
        ShaderResourceMember* pMembers = nullptr;
    };
    
    
    /**
     * @brief Struct that contains information about shader resources, reserved for descriptor & pipeline creation
     */
    struct ShaderResource
    {
        // Fields used by all types of shader resource
        std::string Name;
        VkShaderStageFlags Stage = 0;
        ShaderResourceType Type = ShaderResourceType::All;
        
        // Fields of decoration
        ShaderResourceUpdateMode UpdateMode;
        VkAccessFlags Access = 0;
        ShaderResourceMember* pMembers = nullptr;
        ShaderResourceBaseType BaseType = ShaderResourceBaseType::All;
        uint32_t Set;
        uint32_t Binding;
        uint32_t Location;
        uint32_t InputAttachmentIndex;
        uint32_t VecSize;
        uint32_t Columns;
        uint32_t ArraySize;
        uint32_t Offset;
        uint32_t Size;
        uint32_t ConstantId;
    };
    
    /**
     * @brief Get information of all shader resources referenced in spirv
     * 
     * @param spirv 
     * @param stage 
     * @param desiredDynamicBufferSize   For buffer with dynamic size, SPIRV cross requires we provides desired size to calculate its actual size
     *                                   Buffers are tagged with name
     * @param out_ShaderResource 
     * @return false if `spirv_cross::GLSLCompiler` falied to read the spirv
     */
    bool SPIRVReflectShaderResource(const std::vector<uint32_t>& spirv, 
                                    VkShaderStageFlagBits stage,
                                    const std::unordered_map<std::string, size_t>& desiredDynamicBufferSize, 
                                    std::vector<ShaderResource>& out_ShaderResource);
    
    /**
     * @brief Helper class to manage source file path , shader macro and dynamic buffer size declaration (for reflection)
     *        Won't load source code until asked to
     */
    class ShaderSource 
    {
    public:
        ShaderSource(std::filesystem::path&& sourceFilePath)
            : m_SourceFilePath(sourceFilePath)
        {}
        
        ~ShaderSource() = default;
        
        ShaderSource() = delete;
        
        const std::filesystem::path& GetPath() const { return m_SourceFilePath; }

        const std::string& GetCode();
        
        const std::unordered_map<std::string, std::string>& GetMacros() const { return m_Macros; }
        
        const std::unordered_map<std::string, size_t>& GetDesiredDynamicBufferSize() const { return m_DesiredDynamicBufferSize; }
        
        void AddMacro(const std::string& name, const std::string& value) { m_Macros[name] = value; }
        
        void DeleteMacroByName(const std::string& name)
        {
            const auto& iter = m_Macros.find(name);
            m_Macros.erase(iter);
        }
        
        void SetDynamicBufferSize(const std::string& name, size_t size) { m_DesiredDynamicBufferSize[name] = size; }
        
        size_t GetHash() const;
        
    private:
        std::filesystem::path m_SourceFilePath;
        
        std::string m_SourceCode;
        
        // macro name -> macro value
        std::unordered_map<std::string, std::string> m_Macros;
        
        // dynamic buffer name -> desired dynamic buffer size
        std::unordered_map<std::string, size_t> m_DesiredDynamicBufferSize;
    };
    
    /**
     * @brief Helper class to flush compiled spirv byte code to file as cache.
     *        It can only be found inside `ShaderModule` class.
     */
    class ShaderByteCode 
    {
    private:
        friend class ShaderModule;

        ShaderByteCode(const std::string& sourcePath, std::vector<uint32_t>&& byteCode, bool shouldBeFlushed)
        	: SourcePath(sourcePath), ByteCode(byteCode), ShouldBeFlushed(shouldBeFlushed)
        {}
        
    public:
        /**
         * @brief Flush the byte code with its name to FileSystem if needs to
         */
        ~ShaderByteCode();

        ShaderByteCode() = default;
        ShaderByteCode(ShaderByteCode&&) = default;
        ShaderByteCode& operator=(ShaderByteCode&&) = default;

        ShaderByteCode(const ShaderByteCode&) = delete;
        ShaderByteCode& operator=(const ShaderByteCode&) = delete;
        
        // Only `ShaderModule` possesses this class as its private member, so there is no need to declare them as private member.
    public:
        size_t CacheTag;
        std::string SourcePath;

        std::vector<uint32_t> ByteCode;
        
        bool ShouldBeFlushed;
        
    
    };
    
    /**
     * @brief Wrapper class that contains source glsl code (maybe) and spir-v byte code.
     *        Note: shaderc always assumes the entry point for glsl is "main", see: https://github.com/google/shaderc/blob/main/libshaderc_util/include/libshaderc_util/compiler.h#L358
     *              however, user-defined entry point can still be done with macro
     */
    class ShaderModule : public Resource<VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE>
    {
    public:
        using Base = Resource<VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE>;
        
    public:
        struct ConstructParm0
        {
            const Context&              ctx;
            VkShaderStageFlagBits       stage;
            const ShaderSource&         source;
            const char*                 scopeName;
        };
        /**
         * @brief Constructor with default debug name
         * 
         * @param ctx 
         * @param stage 
         * @param source
         * @param scopeName 
         */
        ShaderModule(ConstructParm0 param);

        struct ConstructParm1
        {
            const Context&              ctx;
            VkShaderStageFlagBits       stage;
            const ShaderSource&         source;
            const char*                 dedicatedName;
        };
        /**
         * @brief Constructor with dedicated debug name
         * 
         * @param ctx 
         * @param stage 
         * @param source 
         * @param debugName 
         */
        ShaderModule(ConstructParm1 param);
        
        ~ShaderModule();
        
        ShaderModule(ShaderModule&& other) = default;
        ShaderModule& operator=(ShaderModule&& other) = default;
        
        ShaderModule() = delete;
        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule& other) = delete;
        
        VkShaderStageFlagBits GetStage() const { return m_Stage; }

        const std::vector<uint32_t>& GetByteCode() const { return m_ByteCode.ByteCode; }

        const std::vector<ShaderResource>& GetResource() const { return m_Resources; }
        
        bool IsValid() const { return m_ByteCode.ByteCode.size() > 0 && 
                                      m_Resources.size() > 0 && 
                                      m_Handle != VK_NULL_HANDLE; }
        
        // get disassembled glsl code, this might be useful if we want to check the including and optimization state.
        std::string GetDisassembledSPIRV();
        
        void SetShaderResourceUpdateMode(const std::string& resoureceName, ShaderResourceUpdateMode mode);
        
        size_t GetHash() const { return m_Hash; }
        
    private:
        /**
         * @brief Actual constructor, compilation & reflection happens here
         * @param ctx 
         */
        void Construct(const Context& ctx);

    private:
        VkShaderStageFlagBits m_Stage;
        
        ShaderSource m_Source;
        
        ShaderByteCode m_ByteCode;
        
        std::vector<ShaderResource> m_Resources;
        
        size_t m_Hash;
    };

    
    inline const char* ToString(ShaderResourceType type)
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
    
    inline const char* ToString(ShaderResourceBaseType type)
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
    
    inline std::string ToString(const ShaderResourceMember* pMem)
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
    
    inline std::string ToString(const ShaderResource& res)
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
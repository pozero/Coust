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
    
    // These shader resource update mode can't be configured in shader module class, their modification is deferred until the cosntruction of pipeline layout
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
        ShaderResourceUpdateMode UpdateMode = ShaderResourceUpdateMode::Static;
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
        explicit ShaderSource(std::filesystem::path&& sourceFilePath) noexcept
            : m_SourceFilePath(sourceFilePath)
        {}

        ShaderSource(const ShaderSource& other) = default;
        ShaderSource(ShaderSource&& other) = default;
        
        ~ShaderSource() = default;
        
        ShaderSource() = delete;
        
        size_t GetHash() const noexcept;

        const std::string& GetCode();

        const std::filesystem::path& GetPath() const noexcept;
        
        const std::unordered_map<std::string, std::string>& GetMacros() const noexcept;
        
        const std::unordered_map<std::string, size_t>& GetDesiredDynamicBufferSize() const noexcept;
        
        void AddMacro(const std::string& name, const std::string& value);
        
        void DeleteMacroByName(const std::string& name);
        
        void SetDynamicBufferSize(std::string_view name, size_t size);
        
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
        	: CacheTag(0), SourcePath(sourcePath), ByteCode(byteCode), ShouldBeFlushed(shouldBeFlushed)
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
    class ShaderModule : public Resource<VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE>,
                         public Hashable
    {
    public:
        using Base = Resource<VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE>;

    public:
        /**
         * @brief Collect all shader resources in the shader module group and classify them according to set (if the resource has set index)
         * 
         * @param modules 
         * @param out_AllShaderResources 
         * @param out_SetToResourceIdxLookup    set -> mask of indice in `out_AllShaderResources` of all resources in this set
         */
        static void CollectShaderResources(const std::vector<ShaderModule*>& modules, 
                                           std::vector<ShaderResource>& out_AllShaderResources, 
                                           std::unordered_map<uint32_t, uint64_t>& out_SetToResourceIdxLookup);
        
        static void CollectShaderInputs(const std::vector<ShaderModule*>& modules,
            uint32_t perInstanceInputMask,  // (1 << l) & perInstanceInputMask != 0 means the input rate of data in location l is per instance
                                            // Also, the maxVertexInputAttributes for 1050 is just 32, so a uint32_t mask is just fine
            std::vector<VkVertexInputBindingDescription>& out_VertexBindingDescriptions,
            std::vector<VkVertexInputAttributeDescription>& out_VertexAttributeDescriptions);
        
    public:
        struct ConstructParm
        {
            const Context&              ctx;
            VkShaderStageFlagBits       stage;
            const ShaderSource&         source;
            const char*                 scopeName = nullptr;
            const char*                 dedicatedName = nullptr;

            size_t GetHash() const;
        };
        explicit ShaderModule(const ConstructParm& param);

        ShaderModule(ShaderModule&& other) noexcept;
        
        ~ShaderModule();
        
        ShaderModule() = delete;
        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(ShaderModule&& other) = delete;
        ShaderModule& operator=(const ShaderModule& other) = delete;
        
        // get disassembled glsl code, this might be useful if we want to check the including and optimization state.
        std::string GetDisassembledSPIRV();

        VkShaderStageFlagBits GetStage() const noexcept;

        const std::vector<uint32_t>& GetByteCode() const noexcept;

        const std::vector<ShaderResource>& GetResource() const noexcept;
        
        bool IsValid() const noexcept;
        
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
    };

    const char* ToString(ShaderResourceType type);
    
    const char* ToString(ShaderResourceBaseType type);
    
    std::string ToString(const ShaderResourceMember* pMem);
    
    std::string ToString(const ShaderResource& res);
}
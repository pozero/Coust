#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <string>
#include <concepts>

#include "Coust/Utils/Random.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
	struct Context;
    template <typename VkHandle, VkObjectType ObjectType>
    class Resource;

	/**
	 * @brief Everything inside `VK::Context` only needs to be created once per run. 
	 * 		  There's no need to wrap them inside `VK::Resource` class.
	 */
   	struct Context
   	{
		VmaAllocator VmaAlloc = nullptr;

		VkInstance Instance = VK_NULL_HANDLE;

		VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT DebugReportCallback = VK_NULL_HANDLE;

		VkSurfaceKHR Surface = VK_NULL_HANDLE;

		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;

		VkDevice Device = VK_NULL_HANDLE;

		uint32_t PresentQueueFamilyIndex = (uint32_t) -1;
		uint32_t GraphicsQueueFamilyIndex = (uint32_t) - 1;

		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		VkQueue PresentQueue = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties PhysicalDevProps{};
		
		VkSampleCountFlags MSAASampleCount = VK_SAMPLE_COUNT_1_BIT;
   	};


	template<typename T, typename VkHanlde, VkObjectType ObjectType>
	concept IsVulkanResource = std::is_base_of<Resource<VkHanlde, ObjectType>, T>::value;

	template<typename T>
	concept ImplementedIsValid = requires (const T& a)
	{
		{ a.IsValid() } -> std::same_as<bool>;
	};

	/**
	 * @brief Most of the vulkan object will be wrapped in this class. 
	 *		  It's used for setting vulkan debug name.
	 *
	 * @tparam VkHandle 
	 * @tparam ObjectType 
	 */
    template <typename VkHandle, VkObjectType ObjectType>
    class Resource
    {
    public:
		/**
		 * @brief Construct a new Resource object
		 * 
		 * @param ctx 
		 * @param handle 
		 */
        Resource(VkDevice device, VkHandle handle)
            : m_Device(device), m_Handle(handle)
		{
		}
		
        Resource() = default;
		~Resource() = default;
		
		Resource(Resource&& other)
			: m_Device(other.m_Device), m_Handle(other.m_Handle), m_DebugName(other.m_DebugName)
		{
			other.m_Handle = VK_NULL_HANDLE;
		}

        Resource(const Resource& other) = delete;
		Resource& operator=(Resource&& other) = delete;
		Resource& operator=(const Resource& other) = delete;
		
		VkDevice GetDevice() const { return m_Device; }
        
        VkHandle GetHandle() const { return m_Handle; }

        const std::string& GetDebugName() const { return m_DebugName; }

		/**
		 * @brief Set Dedicated Debug Name
		 * 
		 * @param name 
		 * @return
		 */
		bool SetDedicatedDebugName(const char* dedicatedName)
		{
			m_DebugName = dedicatedName;
#ifndef COUST_FULL_RELEASE
			return RegisterDebugName(m_Device, ObjectType, m_Handle, dedicatedName);
#else
			return true;
#endif
		}
		
		/**
		 * @brief Set Default Debug Name
		 * 
         * @param scopeName     	C++ scope name, provided by the class that creates this object
         * @param categoryName     	Optional. Further distinction, like that between primary command buffer and secondary command buffer
		 * @return 
		 */
		bool SetDefaultDebugName(const char* scopeName, const char* categoryName)
		{
            std::string typeName{ ToString(ObjectType) };
            typeName += ' ';
			if (categoryName)
			{
				typeName += categoryName;
				typeName += ' ';
			}
            m_DebugName = typeName + scopeName;
#ifndef COUST_FULL_RELEASE
			return RegisterDebugName(m_Device, ObjectType, m_Handle, m_DebugName.c_str());
#else
			return true;
#endif
		}
        
        static VkObjectType GetObjectType() { return ObjectType; }
		
		/**
		 * @brief Helper function to check if a vulkan resource is valid. 
		 *		  In this ver the resource requires special validation, which implemented `IsValid()`
		 * 
		 * @tparam T 		T that implemented `bool IsValid()` function
		 * @param resource 
		 * @return Valid or not
		 */
		template<typename T>
		static bool CheckValidation(T& resource)
			requires ImplementedIsValid<T> && IsVulkanResource<T, VkHandle, ObjectType>
		{
			return resource.IsValid();
		}
		
		/**
		 * @brief Helper function to check if a vulkan resource is valid. 
		 *		  The resource didn't implement `bool IsValid()`, so we check if its `m_Handle` equals `VK_NULL_HANDLE`
		 * 
		 * @tparam T 		T that didn't implement `bool IsValid()` function
		 * @param resource 
		 * @return 			Valid or not
		 */
		template<typename T>
		static bool CheckValidation(T& resource)
			requires IsVulkanResource<T, VkHandle, ObjectType>
		{
			return resource.m_Handle != VK_NULL_HANDLE;
		}

		/**
		 * @brief Resource construction helper
		 * 
		 * @tparam T 				Resource type
		 * @tparam A 				Constructor parameter
		 * @param out_Resource 		Output.
		 * @param args 				Constructor parameter
		 * @return 					true if it created a valid resource, false otherwise
		 */
		template<typename T, typename ...A>
		static bool Create(std::unique_ptr<T>& out_Resource, const A&... args)
			requires IsVulkanResource<T, VkHandle, ObjectType>
		{
			T* ptr = nullptr;
			bool result =  Create(ptr, args...);
			out_Resource = std::unique_ptr<T>{ptr};
			return result;
		}

		/**
		 * @brief Resource construction helper
		 * 
		 * @tparam T 				Resource type
		 * @tparam A 				Constructor parameter
		 * @param out_Resource 		Output.
		 * @param args 				Constructor parameter
		 * @return 					true if it created a valid resource, false otherwise
		 */
		template<typename T, typename ...A>
		static bool Create(std::shared_ptr<T>& out_Resource, const A&... args)
			requires IsVulkanResource<T, VkHandle, ObjectType>
		{
			T* ptr = nullptr;
			bool result =  Create(ptr, args...);
			out_Resource = std::shared_ptr<T>{ptr};
			return result;
		}
	
		/**
		 * @brief Resource construction helper
		 * 
		 * @tparam T 				Resource type
		 * @tparam A 				Constructor parameter
		 * @param out_Resource 		Output.
		 * @param args 				Constructor parameter
		 * @return 					true if it created a valid resource, false otherwise
		 */
		template<typename T, typename ...A>
		static bool Create(T*& out_Resource, const A&... args)
			requires IsVulkanResource<T, VkHandle, ObjectType>
		{
			T* pResource = new T(args...);
			if (CheckValidation(*pResource))
			{
				out_Resource = pResource;
				return true;
			}
			else  
			{
				delete pResource;
				return false;
			}
		}

    private:

        bool RegisterDebugName(const VkDevice device, VkObjectType type, VkHandle handle, const char* name)
        {
			// In case we move from an empty resource
			if (m_Handle == VK_NULL_HANDLE)
				return false;
			// See https://github.com/KhronosGroup/Vulkan-Docs/issues/368 .
			// Dispatchable and non-dispatchable handle types are **NOT** necessarily binary-compatible!
			// Non-dispatchable handles might be only 32-bit long. This is because, on 32-bit machines, they might be a typedef to a 32-bit pointer.
			using Handle = typename std::conditional<sizeof(VkHandle) == sizeof(uint64_t), uint64_t, uint32_t>::type;
			uint64_t convertedHandle = static_cast<uint64_t>(reinterpret_cast<Handle>(handle));

        	VkDebugUtilsObjectNameInfoEXT info								
        	{																
        		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        		.objectType = type,											
        		.objectHandle = convertedHandle,
        		.pObjectName = name,										
        	};																
        	return vkSetDebugUtilsObjectNameEXT(device, &info) == VK_SUCCESS;
        }
    
    protected:
		VkDevice m_Device = VK_NULL_HANDLE;
        VkHandle m_Handle = VK_NULL_HANDLE;
        std::string m_DebugName{};
    };
	
	class Hashable 
	{
	public:
		Hashable(size_t originValue) : m_Hash(originValue) {}
		
		// To remind there's something to be initialized
		Hashable() = delete;

		size_t GetHash() const { return m_Hash; }
		
	protected: 
		size_t m_Hash;
	};
}
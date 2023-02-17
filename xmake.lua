package("GLFW")
    add_deps("cmake")
    set_sourcedir(path.join(os.scriptdir(), "Coust/third_party/GLFW"))
    set_policy("package.install_always", true)
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=".."OFF")
        table.insert(configs, "-DGLFW_BUILD_EXAMPLES=".."OFF")
        table.insert(configs, "-DGLFW_BUILD_TESTS=".."OFF")
        table.insert(configs, "-DGLFW_BUILD_DOCS=".."OFF")
        table.insert(configs, "-DGLFW_INSTALL=".."ON")
        table.insert(configs, "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=".."ON")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

add_rules("mode.debug", "mode.releasedbg", "mode.release")

if is_mode("debug") then
    add_defines("COUST_DEBUG")
    set_runtimes("MDd")
    add_requires("GLFW", { debug = true })
end

if is_mode("releasedbg") then
    add_defines("COUST_DBG_RELEASE")
    set_runtimes("MDd")
    add_requires("GLFW", { debug = true })
end

if is_mode("release") then
    add_defines("COUST_FULL_RELEASE")
    set_runtimes("MD")
    add_requires("GLFW", { debug = false })
end

set_allowedarchs("windows|x64")
set_languages("c++20")


target("Coust")
    set_kind("static")

    set_warnings("all", "error")

    add_syslinks("user32", "gdi32", "shell32", "opengl32")

    add_includedirs("Coust/src")
    set_pcxxheader("Coust/src/pch.h") -- it seems that xmake's precompiled header has some bugs that sometimes it can't find pch.h

    add_defines("CURRENT_DIRECTORY=\"$(shell python ./Tool/GetCurrentDirectoryPath.py)\"")

    -- source file
    add_files("Coust/src/*.cpp")
    add_files("Coust/src/Coust/Core/*.cpp")
    add_files("Coust/src/Coust/Utils/*.cpp")
    add_files("Coust/src/Coust/Event/*.cpp")
    add_files("Coust/src/Coust/Render/*.cpp")
    add_files("Coust/src/Coust/Render/Vulkan/*.cpp")
    -- add_files("Coust/src/Coust/Renderer/Vulkan/RenderLayer/*.cpp")
    add_files("Coust/src/Coust/Render/Resources/*.cpp")

    add_headerfiles("Coust/src/*.h")
    add_headerfiles("Coust/src/Coust/*.h")
    add_headerfiles("Coust/src/Coust/Core/*.h")
    add_headerfiles("Coust/src/Coust/Utils/*.h")
    add_headerfiles("Coust/src/Coust/Event/*.h")
    add_headerfiles("Coust/src/Coust/Render/*.h")
    add_headerfiles("Coust/src/Coust/Render/Vulkan/*.h")
    -- add_headerfiles("Coust/src/Coust/Renderer/Vulkan/RenderLayer/*.h")
    add_headerfiles("Coust/src/Coust/Render/Resources/*.h")
    -- source file

    -- third party 
    add_includedirs("$(env VK_SDK_PATH)/Include")

    add_linkdirs("$(env VK_SDK_PATH)/Lib")
    if (is_mode("release")) then
        add_links("shaderc_combined")
        add_links("shaderc")
    else
        add_links("shaderc_combinedd")
        add_links("shadercd")
    end

    add_includedirs("Coust/third_party/spdlog/include")

    add_includedirs("Coust/third_party/rapidjson/include")

    add_includedirs("Coust/third_party/glm")

    add_includedirs("Coust/third_party/volk")

    add_includedirs("Coust/third_party/vma/include")

    add_packages("GLFW")

    add_deps("Glad")
    add_includedirs("Coust/third_party/glad/include")

    add_deps("imgui")
    add_includedirs("Coust/third_party/imgui")
    -- third party 
target_end()

target("Coustol")
    set_kind("binary")

    set_warnings("all", "error")

    -- source file
    add_files("Coustol/*.cpp")
    add_headerfiles("Coustol/*.h")
    -- source file

    add_deps("Coust")
    add_includedirs("Coust/src")

    -- third party inlude
    add_includedirs("Coust/third_party/spdlog/include")
    add_includedirs("Coust/third_party/imgui")
    add_includedirs("Coust/third_party/glm")
    -- third party inlude
target_end()

-- target("GLFW")
    -- set_kind("static")
-- 
    -- add_defines("_GLFW_WIN32")
    -- add_defines("_CRT_SECURE_NO_WARNINGS")
-- 
    -- add_headerfiles("Coust/third_party/GLFW/include/GLFW/glfw3.h")
    -- add_headerfiles("Coust/third_party/GLFW/include/GLFW/glfw3native.h")
	-- add_headerfiles("Coust/third_party/GLFW/src/glfw_config.h")
-- 
	-- add_files("Coust/third_party/GLFW/src/context.c")
	-- add_files("Coust/third_party/GLFW/src/init.c")
	-- add_files("Coust/third_party/GLFW/src/null_init.c")
	-- add_files("Coust/third_party/GLFW/src/null_monitor.c")
	-- add_files("Coust/third_party/GLFW/src/null_window.c")
	-- add_files("Coust/third_party/GLFW/src/null_joystick.c")
	-- add_files("Coust/third_party/GLFW/src/input.c")
	-- add_files("Coust/third_party/GLFW/src/monitor.c")
	-- add_files("Coust/third_party/GLFW/src/vulkan.c")
	-- add_files("Coust/third_party/GLFW/src/window.c")
	-- add_files("Coust/third_party/GLFW/src/platform.c")
	-- add_files("Coust/third_party/GLFW/src/win32_init.c")
	-- add_files("Coust/third_party/GLFW/src/win32_module.c")
	-- add_files("Coust/third_party/GLFW/src/win32_joystick.c")
	-- add_files("Coust/third_party/GLFW/src/win32_monitor.c")
	-- add_files("Coust/third_party/GLFW/src/win32_time.c")
	-- add_files("Coust/third_party/GLFW/src/win32_thread.c")
	-- add_files("Coust/third_party/GLFW/src/win32_window.c")
	-- add_files("Coust/third_party/GLFW/src/wgl_context.c")
	-- add_files("Coust/third_party/GLFW/src/egl_context.c")
	-- add_files("Coust/third_party/GLFW/src/osmesa_context.c")
-- target_end()

target("Glad")
    set_kind("static")

    add_includedirs("Coust/third_party/glad/include")

    add_files("Coust/third_party/glad/src/glad.c")
target_end()

target("imgui")
    set_kind("static")

    add_includedirs("Coust/third_party/imgui")

    add_files("Coust/third_party/imgui/imgui.cpp")
    add_files("Coust/third_party/imgui/imgui_demo.cpp")
    add_files("Coust/third_party/imgui/imgui_draw.cpp")
    add_files("Coust/third_party/imgui/imgui_tables.cpp")
    add_files("Coust/third_party/imgui/imgui_widgets.cpp")

    -- Render Backends
    add_includedirs("Coust/third_party/GLFW/include")
    add_includedirs("$(env VK_SDK_PATH)/Include")

    add_headerfiles("Coust/third_party/imgui/backends/imgui_impl_glfw.h")
    add_headerfiles("Coust/third_party/imgui/backends/imgui_impl_opengl3.h")

    add_files("Coust/third_party/imgui/backends/imgui_impl_glfw.cpp")
    add_files("Coust/third_party/imgui/backends/imgui_impl_opengl3.cpp")
target_end()

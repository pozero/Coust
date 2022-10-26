add_rules("mode.debug", "mode.releasedbg", "mode.release")

if is_mode("debug") then
    add_defines("COUST_DEBUG")
    set_runtimes("MD")
end

if is_mode("releasedbg") then
    add_defines("COUST_DBG_RELEASE")
    set_runtimes("MD")
end

if is_mode("release") then
    add_defines("COUST_FULL_RELEASE")
    set_runtimes("MT")
end

set_allowedarchs("windows|x64")
set_languages("c++17")

target("GLFW")
    set_kind("static")

    add_defines("_GLFW_WIN32")
    add_defines("_CRT_SECURE_NO_WARNINGS")

    add_headerfiles("Coust/third_party/GLFW/include/GLFW/glfw3.h")
    add_headerfiles("Coust/third_party/GLFW/include/GLFW/glfw3native.h")
	add_headerfiles("Coust/third_party/GLFW/src/glfw_config.h")

	add_files("Coust/third_party/GLFW/src/context.c")
	add_files("Coust/third_party/GLFW/src/init.c")
	add_files("Coust/third_party/GLFW/src/null_init.c")
	add_files("Coust/third_party/GLFW/src/null_monitor.c")
	add_files("Coust/third_party/GLFW/src/null_window.c")
	add_files("Coust/third_party/GLFW/src/null_joystick.c")
	add_files("Coust/third_party/GLFW/src/input.c")
	add_files("Coust/third_party/GLFW/src/monitor.c")
	add_files("Coust/third_party/GLFW/src/vulkan.c")
	add_files("Coust/third_party/GLFW/src/window.c")
	add_files("Coust/third_party/GLFW/src/platform.c")
	add_files("Coust/third_party/GLFW/src/win32_init.c")
	add_files("Coust/third_party/GLFW/src/win32_module.c")
	add_files("Coust/third_party/GLFW/src/win32_joystick.c")
	add_files("Coust/third_party/GLFW/src/win32_monitor.c")
	add_files("Coust/third_party/GLFW/src/win32_time.c")
	add_files("Coust/third_party/GLFW/src/win32_thread.c")
	add_files("Coust/third_party/GLFW/src/win32_window.c")
	add_files("Coust/third_party/GLFW/src/wgl_context.c")
	add_files("Coust/third_party/GLFW/src/egl_context.c")
	add_files("Coust/third_party/GLFW/src/osmesa_context.c")

target("Glad")
    set_kind("static")

    add_includedirs("Coust/third_party/glad/include")

    add_headerfiles("Coust/third_party/glad/include/glad/glad.h")
    add_headerfiles("Coust/third_party/glad/include/KHR/khrplatform.h")

    add_files("Coust/third_party/glad/src/glad.c")

target("Coust")
    set_kind("static")

    set_warnings("all", "error")

    add_syslinks("user32", "gdi32", "shell32", "opengl32")

    add_includedirs("Coust/src")
    -- set_pcxxheader("Coust/src/pch.h")

    -- source file
    add_files("Coust/src/*.cpp")
    add_files("Coust/src/Coust/*.cpp")
    add_files("Coust/src/Coust/Event/*.cpp")

    add_headerfiles("Coust/src/*.h")
    add_headerfiles("Coust/src/Coust/*.h")
    add_headerfiles("Coust/src/Coust/Event/*.h")
    -- source file

    -- third party 
    add_includedirs("Coust/third_party/spdlog/include")

    add_deps("GLFW")
    add_includedirs("Coust/third_party/GLFW/include")

    add_deps("Glad")
    add_includedirs("Coust/third_party/glad/include")
    -- third party 

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
    -- third party inlude
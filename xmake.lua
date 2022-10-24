add_rules("mode.debug", "mode.releasedbg", "mode.release")

if is_mode("debug") then
    add_defines("COUST_DEBUG")
end

if is_mode("releasedbg") then
    add_defines("COUST_DBG_RELEASE")
end

if is_mode("release") then
    add_defines("COUST_FULL_RELEASE")
end

set_allowedarchs("windows|x64")
set_languages("c++17")
set_warnings("all", "error")
set_runtimes("MD")

target("Coust")
    set_kind("static")

    add_syslinks("user32", "gdi32", "shell32", "opengl32")

    add_includedirs("Coust/src")
    set_pcxxheader("Coust/src/pch.h")

    -- source file
    add_files("Coust/src/*.cpp")
    add_files("Coust/src/Coust/*.cpp")
    add_files("Coust/src/Coust/Event/*.cpp")

    add_headerfiles("Coust/src/*.h")
    add_headerfiles("Coust/src/Coust/*.h")
    add_headerfiles("Coust/src/Coust/Event/*.h")
    -- source file

    -- third party inlude
    add_includedirs("Coust/third_party/spdlog/include")
    add_includedirs("Coust/third_party/GLFW/include")
    -- third party inlude

    -- third party binary
    add_links("glfw3")
    add_linkdirs("Coust/third_party/GLFW/binary")
    -- third party binary

target("Coustol")
    set_kind("binary")
    
    -- source file
    add_files("Coustol/*.cpp")
    add_headerfiles("Coustol/*.h")
    -- source file

    add_deps("Coust")
    add_includedirs("Coust/src")

    -- third party inlude
    add_includedirs("Coust/third_party/spdlog/include")
    -- third party inlude
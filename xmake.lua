add_rules("mode.debug", "mode.release")

target("Coust")
    set_kind("shared")
    
    add_files("Coust/src/Coust/*.cpp")

    add_headerfiles("Coust/src/*.h")
    add_headerfiles("Coust/src/Coust/*.h")

    add_defines("COUST_DYNAMIC_BUILD")

target("Roast")
    set_kind("binary")

    add_syslinks("user32", "gdi32", "shell32")
    
    add_files("Roast/*.cpp")
    add_headerfiles("Roast/*.h")

    add_deps("Coust")

    add_includedirs("Coust/src")
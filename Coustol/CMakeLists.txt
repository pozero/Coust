add_executable(Coustol 
    ${PROJECT_SOURCE_DIR}/Coustol/Coustol.cpp
)

target_link_libraries(Coustol 
    PRIVATE 
        Coust
)

target_include_directories(Coustol 
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/Coust/src
        ${PROJECT_SOURCE_DIR}/Coust/third_party/fmt/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/doctest
        ${PROJECT_SOURCE_DIR}/Coust/third_party/volk
        $ENV{VULKAN_SDK}/Include
)
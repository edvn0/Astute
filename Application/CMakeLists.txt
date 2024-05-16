cmake_minimum_required(VERSION 3.23)

project(Application)

add_executable(App main.cpp AstuteApplication.cpp AstuteApplication.hpp Widgets/Widget.hpp Widgets/SceneWidget.hpp Widgets/SceneWidget.cpp)
target_link_libraries(App PRIVATE Core EnTT::EnTT Logging ShaderCompilation)
target_include_directories(App PRIVATE ${CMAKE_SOURCE_DIR}/Core ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(App PRIVATE pch/ApplicationPCH.hpp)

if(ASTUTE_BASE_PATH)
    target_compile_definitions(App PRIVATE ASTUTE_BASE_PATH="${ASTUTE_BASE_PATH}")
endif()

if("Clang" IN_LIST CMAKE_CXX_COMPILER_ID)
    target_compile_options(Core PRIVATE -Wno-unused-local-typedef)
endif()

target_compile_definitions(App PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

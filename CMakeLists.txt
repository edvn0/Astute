cmake_minimum_required(VERSION 3.23)

project(AstuteEngine CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
    message(STATUS "CCACHE is found")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else(CCACHE_FOUND)
    message(STATUS "CCACHE is NOT found")
endif(CCACHE_FOUND)

add_subdirectory(Modules)
add_subdirectory(Library)
add_subdirectory(Core)
add_subdirectory(Application)

target_compile_definitions(Core
    PRIVATE
    $<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)
target_compile_definitions(App
    PRIVATE
    $<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)

# target_compile_definitions(Core PRIVATE
# $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_VALIDATION_LAYERS}>>:ENABLE_VALIDATION_LAYERS>)
# target_compile_definitions(App PRIVATE
# $<$<AND:$<CONFIG:Debug>,$<BOOL:${ENABLE_VALIDATION_LAYERS}>>:ENABLE_VALIDATION_LAYERS>)
target_compile_options(Core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_compile_options(App PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
add_link_options(-fsanitize=address -fno-omit-frame-pointer)

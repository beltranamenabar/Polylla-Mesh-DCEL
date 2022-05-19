cmake_minimum_required(VERSION 3.18)

project(Polylla
    DESCRIPTION "Polygonal meshing algorithm based on terminal-edge regions"
    LANGUAGES CXX
)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()


add_subdirectory(src)

add_executable(Polylla main.cpp)

target_link_libraries(Polylla INTERFACE meshfiles)
target_include_directories(Polylla PUBLIC src)
set_target_properties(meshfiles PROPERTIES LINKER_LANGUAGE CXX)

cmake_minimum_required(VERSION 3.18)

project(Polylla
    DESCRIPTION "Polylla"
    LANGUAGES CXX
)

add_subdirectory(src)

add_executable(Polylla main.cpp)

target_link_libraries(Polylla INTERFACE meshfiles)
target_include_directories(Polylla PUBLIC src)
set_target_properties(meshfiles PROPERTIES LINKER_LANGUAGE CXX)

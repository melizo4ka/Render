cmake_minimum_required(VERSION 3.27)
project(Render)

set(CMAKE_CXX_STANDARD 17)
find_package(OpenMP REQUIRED)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(matplotplusplus
        GIT_REPOSITORY https://github.com/alandefreitas/matplotplusplus
        GIT_TAG origin/master) # or whatever tag you want

FetchContent_GetProperties(matplotplusplus)
if(NOT matplotplusplus_POPULATED)
    FetchContent_Populate(matplotplusplus)
    add_subdirectory(${matplotplusplus_SOURCE_DIR} ${matplotplusplus_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


add_executable(Render main.cpp)
target_link_libraries(Render
        PRIVATE sfml-graphics
        OpenMP::OpenMP_CXX
        PUBLIC matplot)
target_compile_features(Render PRIVATE cxx_std_17)

find_package(X11 REQUIRED)
target_link_libraries(Render PRIVATE ${X11_LIBRARIES})

install(TARGETS Render)

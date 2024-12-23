cmake_minimum_required(VERSION 3.29.0)
project(Ef VERSION 0.1.0 LANGUAGES CXX)

enable_testing()
include(GoogleTest)

find_package(Boost REQUIRED COMPONENTS algorithm locale)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
add_definitions(
    -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)

add_library(EfLib)
target_sources(EfLib
    PRIVATE
        ef.cpp
        Exception.cpp
        Lexxer.cpp
        Parser.cpp
        Text.cpp
        Token.cpp
)
target_link_libraries(EfLib
    PRIVATE
        Boost::algorithm
        Boost::locale
)

add_executable(Ef)
target_sources(Ef
    PRIVATE
        main.cpp
)
target_link_libraries(Ef
    PRIVATE
        EfLib
)

add_subdirectory(test)

find_package(GTest CONFIG REQUIRED)

add_executable(MyTest)
target_sources(MyTest
    PRIVATE
        main.cpp
        StdStringTestHelpers.cpp
)
target_link_libraries(MyTest
    PRIVATE
        EfLib
        GTest::gtest
)

gtest_discover_tests(MyTest)

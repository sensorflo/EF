cmake_minimum_required(VERSION 2.8.8)
project(ef-distribution)

enable_testing()

add_subdirectory(doc)
add_subdirectory(src/efc)

# clang format
# --------------------------------------------------
find_program(CLANG_FORMAT clang-format)
if (NOT DOXYGEN)
  message(WARNING "clang-format not found. With apt, you can install it with 'apt-get install clang-format'.")
else()
  add_custom_target(clang-format
    COMMAND find -iname "\\*.h" -o -iname "\\*.cpp" |
            grep -v "/build\\\\\\|/googletest/\\\\\\|/efc/test/tests/\\\\\\|/efc/gensrc/" |
            xargs ${CLANG_FORMAT} -i
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

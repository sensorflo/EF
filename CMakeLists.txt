cmake_minimum_required(VERSION 2.8.8)
project(ef-distribution)

enable_testing()

# experiment
# add_definitions(-Weverything -Wno-c++98-compat -Wno-covered-switch-default
#   -Wno-undef -Wreserved-id-macro -Wno-padded -Wno-weak-vtables
#   -Wno-global-constructors -Wno-exit-time-destructors
#   -Wno-documentation-unknown-command)
# Wno-unused-parameter is due to llvm headers
add_definitions(-std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Werror)


add_subdirectory(doc)
add_subdirectory(googletest/googletest)
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

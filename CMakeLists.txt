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


# clang tidy / clang modernize
# --------------------------------------------------
# TODO: clang-tidy, clang-format, clang-apply-replacements should only be
#       searched in PATH, because run-clang-tidy.py also only searches there.
find_program(CLANG_TIDY clang-tidy)
find_program(RUN_CLANG_TIDY run-clang-tidy.py)
find_program(CLANG_APPLY_REPLACEMENTS clang-apply-replacements)
if (NOT CLANG_TIDY)
  message(WARNING "clang-tidy not found. With apt, you can install it with "
    "'apt-get install clang-tidy'.")
elseif(NOT RUN_CLANG_TIDY)
  message(WARNING "run-clang-tidy not found. You can download it from here: "
    "https://github.com/llvm-mirror/clang-tools-extra/blob/master/clang-tidy/tool/run-clang-tidy.py")
elseif(NOT CLANG_APPLY_REPLACEMENTS)
  message(WARNING "clang-apply-replacements not found. You need to download the "
    "'Extra Clang Tools' sources (https://clang.llvm.org/extra/index.html) and build them.")
elsif(${CMAKE_EXPORT_COMPILE_COMMANDS} EQUAL ON)
  message(WARNING "CMAKE_EXPORT_COMPILE_COMMANDS must be set to ON in order for "
    "the clang-modernize target to become available")
else()
  add_custom_target(clang-modernize
    COMMAND run-clang-tidy.py "'-header-filter=.*/efc/(test/(testhelpers/)?)?\\w*\\.h'"
      -checks='-*,modernize-*' -fix -format
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()

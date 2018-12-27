cmake_minimum_required(VERSION 2.8.8)
project(ef-doc)

find_program(ASCIIDOC asciidoc)
if (NOT ASCIIDOC)
  message(FATAL_ERROR "asciidoc not found. With apt, you can install it with 'apt-get install asciidoc'")
endif()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ef.html
  DEPENDS ef.txt
  COMMAND ${ASCIIDOC} -o ${CMAKE_CURRENT_BINARY_DIR}/ef.html ${CMAKE_CURRENT_SOURCE_DIR}/ef.txt
  VERBATIM
  )

add_custom_target(
  ef-doc ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ef.html
  )

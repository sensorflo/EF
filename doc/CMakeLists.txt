cmake_minimum_required(VERSION 2.8.8)
project(ef-doc)

find_program(ASCIIDOC asciidoc)
if (NOT ASCIIDOC)
  message(WARNING "asciidoc not found. With apt, you can install it with 'apt-get install asciidoc'")

else()
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ef.html
    DEPENDS ef.asciidoc
    COMMAND ${ASCIIDOC} -o ${CMAKE_CURRENT_SOURCE_DIR}/ef.html ${CMAKE_CURRENT_SOURCE_DIR}/ef.asciidoc
    VERBATIM
  )

  add_custom_target(
    ef-doc ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ef.html
  )
endif()

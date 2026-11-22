cmake_minimum_required(VERSION 3.16)
project(ouro LANGUAGES C)

set(OURO_VERSION "0.1.0-dev" CACHE STRING "Version reported by ouroc0 --version")
option(OURO_STATIC "Link statically (used for Linux release builds)" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB OURO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_executable(ouroc0 ${OURO_SOURCES})
target_include_directories(ouroc0 PRIVATE src)
target_compile_definitions(ouroc0 PRIVATE OURO_VERSION="${OURO_VERSION}")
set_target_properties(ouroc0 PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

if(MSVC)
  target_compile_definitions(ouroc0 PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_options(ouroc0 PRIVATE /W3)
else()
  target_compile_options(ouroc0 PRIVATE -Wall -Wextra)
  if(OURO_STATIC)
    target_link_options(ouroc0 PRIVATE -static)
  endif()
endif()

enable_testing()
add_test(NAME ouroc0-tests
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh $<TARGET_FILE:ouroc0>)

install(TARGETS ouroc0 RUNTIME DESTINATION bin)

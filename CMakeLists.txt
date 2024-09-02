cmake_minimum_required(VERSION 3.5)

project(
  dmtx
  VERSION 0.7.7
  LANGUAGES C
)

# DMTX library
option(DMTX_SHARED "Build DMTX as shared library" ${BUILD_SHARED_LIBS})

if(DMTX_SHARED)
  add_library(${PROJECT_NAME} SHARED "dmtx.c")
else()
  add_library(${PROJECT_NAME} STATIC "dmtx.c")
endif()

# Compiler specific settings
if(MSVC)
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
  target_link_libraries(${PROJECT_NAME} PUBLIC -lm)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/dmtx.h")

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

install(TARGETS ${PROJECT_NAME} LIBRARY ARCHIVE RUNTIME PUBLIC_HEADER)

# Add tests if DMTX is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)

  if(BUILD_TESTING)
    add_subdirectory("test")
  endif()
endif()

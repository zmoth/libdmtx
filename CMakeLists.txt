cmake_minimum_required(VERSION 3.5)

set(PROJECT_DESCRIPTION "libdmtx is a software library that enables programs to read and write Data Matrix barcodes of the modern ECC200 variety.")
set(PROJECT_HOMEPAGE_URL "https://github.com/dmtx/libdmtx")

project(
  dmtx
  VERSION 0.7.7
  DESCRIPTION ${PROJECT_DESCRIPTION}
  HOMEPAGE_URL ${PROJECT_HOMEPAGE_URL}
  LANGUAGES C
)

# DMTX library
option(DMTX_SHARED "Build DMTX as shared library" ${BUILD_SHARED_LIBS})

if(MSVC)
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# 如果此项目作为其他项目的第三方库，以下的配置不生效
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # 设置输出路径
  if(UNIX)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
  else()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
  endif()
endif()

if(DMTX_SHARED)
  add_library(${PROJECT_NAME} SHARED "src/dmtx.c")
else()
  add_library(${PROJECT_NAME} STATIC "src/dmtx.c")
endif()

# 添加配置头文件，将CMake中的版本等信息转换为头文件
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h"
  @ONLY
)

# Compiler specific settings
if(MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
  target_link_libraries(${PROJECT_NAME} PUBLIC -lm)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/dmtx.h")

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# 设定工程（通常是库或可执行文件）的输出目录
set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Add tests if DMTX is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_definitions(-DDEBUG_CALLBACK)

  # EXAMPLES
  add_subdirectory(examples)

  include(CTest)

  if(BUILD_TESTING)
    add_subdirectory("test")
  endif()

  # INSTALL
  install(TARGETS ${PROJECT_NAME} LIBRARY ARCHIVE RUNTIME PUBLIC_HEADER)

  include(CMakePackageConfigHelpers)

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  export(TARGETS ${PROJECT_NAME}
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    DESTINATION cmake
  )

  install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION "./"
  )

  install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DESTINATION "./"
  )

  # 打包
  include(cmake/package.cmake)
endif()

cmake_minimum_required(VERSION 3.21)

project(
  cv
  LANGUAGES CXX
)

message(STATUS "[example] 项目名称 ${PROJECT_NAME}")

file(GLOB SRCS *.cpp)
file(GLOB INCS *.h)

find_package(OpenCV REQUIRED)

if(OpenCV_FOUND)
  message(STATUS "[${PROJECT_NAME}] OpenCV_VERSION: ${OpenCV_VERSION}, OpenCV_DIR: ${OpenCV_DIR}")
else()
  message(FATAL_ERROR "[${PROJECT_NAME}] OpenCV found failed")
endif()

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${SRCS} ${INCS})

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE $<BUILD_INTERFACE:${dmtx_SOURCE_DIR}>
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE dmtx
  PRIVATE ${OpenCV_LIBS}
)

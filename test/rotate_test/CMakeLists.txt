project(rotate_test)

message(STATUS "[example] 项目名称 ${PROJECT_NAME}")

file(GLOB SRCS *.c)
file(GLOB INCS *.h)

find_package(SDL2 REQUIRED)

if(SDL2_FOUND)
    message(STATUS "[${PROJECT_NAME}] SDL2_VERSION: ${SDL2_VERSION}, SDL2_DIR: ${SDL2_DIR}")
else()
    message(FATAL_ERROR "[${PROJECT_NAME}] SDL2 found failed")
endif()

include_directories(${SDL2_INCLUDE_DIRS})

# 查找OpenGL库
find_package(OpenGL REQUIRED)

# 如果find_package成功找到了OpenGL，它会设置OPENGL_FOUND变量，并且设置OpenGL_LIBRARIES变量
if(OPENGL_FOUND)
    message(STATUS "OpenGL_VERSION: ${OpenGL_VERSION}, OpenGL_DIR: ${OpenGL_DIR}")
else()
    message(FATAL_ERROR "OpenGL library not found.")
endif()

add_executable(${PROJECT_NAME} ${SRCS} ${INCS})

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE $<BUILD_INTERFACE:${dmtx_SOURCE_DIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE dmtx
    PRIVATE ${SDL2_LIBRARIES}
    PRIVATE ${OPENGL_LIBRARIES}
)
# 通过git获取版本宏
find_package(Git QUIET)

function(get_git_version version)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_TAG_VERSION
            ERROR_VARIABLE GIT_TAG_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            TIMEOUT 5
        )
    endif()

    # 如果有错误则设置版本号为0.0.0
    if(NOT DEFINED GIT_TAG_ERROR OR NOT "${GIT_TAG_ERROR}" STREQUAL "")
        set(GIT_TAG_VERSION "0.0.0")
    endif()

    # 获取并解析版本号
    string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+$" PROJECT_VERSION ${GIT_TAG_VERSION})

    if("${PROJECT_VERSION}" STREQUAL "")
        set(IS_RELEASE OFF)
    else()
        set(IS_RELEASE ON)
    endif()

    string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+(-[0-9]+)?" PROJECT_VERSION ${GIT_TAG_VERSION})
    string(REPLACE "-" "." VERSION_PARTS ${PROJECT_VERSION})
    string(REPLACE "." ";" VERSION_PARTS ${VERSION_PARTS})
    list(LENGTH VERSION_PARTS VERSION_PARTS_COUNT)

    # 分别给四个版本号部分赋值
    list(GET VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
    list(GET VERSION_PARTS 1 PROJECT_VERSION_MINOR)
    list(GET VERSION_PARTS 2 PROJECT_VERSION_PATCH)

    if(${VERSION_PARTS_COUNT} GREATER 3)
        list(GET VERSION_PARTS 3 PROJECT_VERSION_TWEAK)
    else()
        set(PROJECT_VERSION_TWEAK 0)
    endif()

    set(${version} "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}" PARENT_SCOPE)
endfunction()

# 打包
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT "Moth <QianMoth@qq.com>")
set(CPACK_PACKAGE_VENDOR "MOTH") # Company
set(CPACK_PACKAGE_HOMEPAGE_URL ${PROJECT_HOMEPAGE_URL})

# 系统信息
set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
string(TOLOWER ${CPACK_SYSTEM_NAME} CPACK_SYSTEM_NAME)

# 编译时间
string(TIMESTAMP COMPILE_TIME "%Y-%m-%d %H:%M:%S")

# 输出名称
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")

# 输出目录
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR}/package)

list(APPEND CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

message(STATUS "[cpack] CPack generators: ${CPACK_GENERATOR}, Time: ${COMPILE_TIME}, Name: ${CPACK_PACKAGE_FILE_NAME}")

include(CPack)

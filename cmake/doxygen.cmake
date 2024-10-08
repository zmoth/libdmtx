# 检查是否安装了Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "lib${PROJECT_NAME}")

  # set(DOXYGEN_PROJECT_LOGO "${CMAKE_SOURCE_DIR}/resources/icon/logo.png")
  set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

  # set(DOXYGEN_OUTPUT_LANGUAGE "Chinese")
  set(DOXYGEN_JAVADOC_AUTOBRIEF "YES")
  set(DOXYGEN_EXTRACT_ALL "YES")
  set(DOXYGEN_EXTRACT_PRIVATE "YES")
  set(DOXYGEN_EXTRACT_STATIC "YES")
  set(DOXYGEN_RECURSIVE "YES")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  set(DOXYGEN_SOURCE_BROWSER "YES")
  set(DOXYGEN_MARKDOWN_SUPPORT "YES")
  set(DOXYGEN_GENERATE_TREEVIEW "YES")
  set(DOXYGEN_USE_MATHJAX "YES")
  set(DOXYGEN_GENERATE_LATEX "NO")
  set(DOXYGEN_USE_PDFLATEX "NO")

  set(DOXYGEN_EXAMPLE_PATH "${CMAKE_SOURCE_DIR}/examples")
  set(DOXYGEN_EXAMPLE_RECURSIVE "YES")

  doxygen_add_docs(
    doxygen
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/LICENSE"
    ALL
    COMMENT "Generate Doxygen documents"
  )
endif()

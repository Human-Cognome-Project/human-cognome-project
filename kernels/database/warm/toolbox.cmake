include(${CMAKE_CURRENT_LIST_DIR}/values.cmake)
if(NOT TARGET warm_toolbox)
  find_package(PostgreSQL 16 REQUIRED)
  add_library(warm_toolbox STATIC
    ${CMAKE_CURRENT_LIST_DIR}/toolbox.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cold_reader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/shared_manager.cpp)
  target_link_libraries(warm_toolbox PUBLIC warm_values PostgreSQL::PostgreSQL)
  target_include_directories(warm_toolbox PUBLIC ${CMAKE_CURRENT_LIST_DIR})
  target_compile_options(warm_toolbox PRIVATE -Wall -Wextra -Wpedantic)
endif()

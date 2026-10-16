# Shared value boundary: no PostgreSQL, engine, or runtime dependency.
if(NOT TARGET warm_values)
  add_library(warm_values STATIC
    ${CMAKE_CURRENT_LIST_DIR}/values.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../codec/codec.cpp)
  target_compile_features(warm_values PUBLIC cxx_std_17)
  target_include_directories(warm_values PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
  target_compile_options(warm_values PRIVATE -Wall -Wextra -Wpedantic)
endif()

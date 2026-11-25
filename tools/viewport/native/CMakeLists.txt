add_library(viewport_snapshot STATIC snapshot.cpp completed_state.cpp prepared_owner_state.cpp prepared_letter_profile.cpp)
target_link_libraries(viewport_snapshot PUBLIC active_workbench prepared_study_owner)
target_include_directories(viewport_snapshot PUBLIC . ../vendor)
target_compile_options(viewport_snapshot PRIVATE -Wall -Wextra -Wpedantic)
add_executable(semantic_native_trace_test
  ../../../engine/tests/semantic_native_trace_test.cpp
  ../../../engine/tests/scoped_hierarchy_audit.cpp)
target_include_directories(semantic_native_trace_test PRIVATE ../../../engine/tests)
target_link_libraries(semantic_native_trace_test PRIVATE viewport_snapshot)
target_compile_options(semantic_native_trace_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME semantic_native_projection COMMAND semantic_native_trace_test --projection-only)
add_test(NAME semantic_native_trace COMMAND semantic_native_trace_test)
add_test(NAME semantic_character_rollup_bulk COMMAND semantic_native_trace_test --case character-rollup)
add_test(NAME semantic_character_rollup_direct COMMAND semantic_native_trace_test --case character-rollup --threads 2 --direct)
add_test(NAME semantic_parent_overlap COMMAND semantic_native_trace_test --case parent-overlap --threads 2 --direct)
add_test(NAME semantic_scoped_nibble COMMAND semantic_native_trace_test --case scoped-nibble --threads 2 --direct)
add_test(NAME semantic_repeated_rollup COMMAND semantic_native_trace_test --case repeated-rollup --threads 2 --direct)
add_test(NAME semantic_varied_rollup_bulk COMMAND semantic_native_trace_test --case varied-rollup)
add_test(NAME semantic_varied_rollup_direct COMMAND semantic_native_trace_test --case varied-rollup --threads 2 --direct)
set_tests_properties(semantic_native_projection semantic_native_trace
  semantic_character_rollup_bulk semantic_character_rollup_direct semantic_parent_overlap semantic_scoped_nibble
  semantic_repeated_rollup semantic_varied_rollup_bulk semantic_varied_rollup_direct PROPERTIES
  ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)
add_library(viewport_live_study_commands STATIC live_study_runner.cpp)
target_link_libraries(viewport_live_study_commands PUBLIC viewport_snapshot)
target_compile_options(viewport_live_study_commands PRIVATE -Wall -Wextra -Werror)
add_executable(viewport_live_study live_study.cpp)
target_include_directories(viewport_live_study PRIVATE ../../../engine/tests)
target_link_libraries(viewport_live_study PRIVATE viewport_live_study_commands)
target_compile_options(viewport_live_study PRIVATE -Wall -Wextra -Werror)
foreach(path bulk direct)
  add_test(NAME viewport_unicode_i_${path}
    COMMAND "${CMAKE_COMMAND}" "-DSTUDY=$<TARGET_FILE:viewport_live_study>" "-DHOST_PATH=${path}"
      "-DCOMMANDS=${CMAKE_CURRENT_SOURCE_DIR}/../examples/live-unicode.commands"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/check_live_unicode.cmake")
  set_tests_properties(viewport_unicode_i_${path} PROPERTIES
    ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
endforeach()
foreach(workers 1 2)
  foreach(path bulk direct)
    set(live_study_args --threads ${workers})
    if(path STREQUAL "direct")
      list(APPEND live_study_args --direct)
    endif()
    add_test(NAME viewport_live_study_${workers}_${path}
      COMMAND viewport_live_study ${live_study_args} --commands ${CMAKE_CURRENT_SOURCE_DIR}/../examples/live-study.commands)
    set_tests_properties(viewport_live_study_${workers}_${path} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
    if(workers EQUAL 2 AND path STREQUAL "direct")
      add_test(NAME viewport_whole_field_${workers}_${path}
        COMMAND "${CMAKE_COMMAND}"
          "-DSTUDY=$<TARGET_FILE:viewport_live_study>"
          "-DCOMMANDS=${CMAKE_CURRENT_SOURCE_DIR}/../examples/whole-field-study.commands"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/check_whole_field_response.cmake")
    else()
      add_test(NAME viewport_whole_field_${workers}_${path}
        COMMAND viewport_live_study ${live_study_args} --whole-field --commands ${CMAKE_CURRENT_SOURCE_DIR}/../examples/whole-field-study.commands)
    endif()
    set_tests_properties(viewport_whole_field_${workers}_${path} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 600)
  endforeach()
endforeach()
add_executable(viewport_options_test options_test.cpp)
target_include_directories(viewport_options_test PRIVATE .)
target_compile_options(viewport_options_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME viewport_options_test COMMAND viewport_options_test)
add_executable(viewport_signal_guard_test signal_guard_test.cpp)
target_include_directories(viewport_signal_guard_test PRIVATE .)
target_link_libraries(viewport_signal_guard_test PRIVATE Threads::Threads)
target_compile_options(viewport_signal_guard_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME viewport_signal_guard_test COMMAND viewport_signal_guard_test)
add_test(NAME semantic_scoped_nibble_no_unroll COMMAND semantic_native_trace_test
  --case scoped-nibble --threads 2 --direct --compiler-no-unroll)
add_test(NAME semantic_unicode_exchange_bulk COMMAND semantic_native_trace_test --case unicode-exchange)
add_test(NAME semantic_unicode_exchange_direct COMMAND semantic_native_trace_test --case unicode-exchange --threads 2 --direct)
set_tests_properties(semantic_scoped_nibble_no_unroll semantic_unicode_exchange_bulk
  semantic_unicode_exchange_direct PROPERTIES
  ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)
add_executable(viewport_snapshot_test snapshot_test.cpp)
target_link_libraries(viewport_snapshot_test PRIVATE viewport_snapshot)
target_include_directories(viewport_snapshot_test PRIVATE ../../../engine/tests)
add_test(NAME viewport_snapshot_test COMMAND viewport_snapshot_test)
set_tests_properties(viewport_snapshot_test PROPERTIES
  ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 180)
add_executable(viewport_prepared_owner_test prepared_owner_state_test.cpp)
target_link_libraries(viewport_prepared_owner_test PRIVATE viewport_snapshot)
target_include_directories(viewport_prepared_owner_test PRIVATE ../../../engine/tests)
foreach(workers 1 2)
  foreach(path bulk direct)
    add_test(NAME viewport_prepared_owner_${workers}_${path} COMMAND viewport_prepared_owner_test ${workers} ${path})
    set_tests_properties(viewport_prepared_owner_${workers}_${path} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
  endforeach()
endforeach()
add_executable(viewport_prepared_letter_profile_test prepared_letter_profile_test.cpp)
target_link_libraries(viewport_prepared_letter_profile_test PRIVATE viewport_snapshot)
target_include_directories(viewport_prepared_letter_profile_test PRIVATE ../../../engine/tests)
target_compile_options(viewport_prepared_letter_profile_test PRIVATE -Wall -Wextra -Werror)
foreach(workers 1 2)
  foreach(path bulk direct)
    add_test(NAME viewport_prepared_letter_profile_${workers}_${path}
      COMMAND viewport_prepared_letter_profile_test ${workers} ${path})
    set_tests_properties(viewport_prepared_letter_profile_${workers}_${path} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
  endforeach()
endforeach()
add_executable(viewport_prepared_retained_profile_test prepared_retained_profile_test.cpp)
target_link_libraries(viewport_prepared_retained_profile_test PRIVATE viewport_snapshot)
target_include_directories(viewport_prepared_retained_profile_test PRIVATE ../../../engine/tests)
target_compile_options(viewport_prepared_retained_profile_test PRIVATE -Wall -Wextra -Werror)
foreach(workers 1 2)
  foreach(path bulk direct)
    add_test(NAME viewport_prepared_retained_profile_${workers}_${path}
      COMMAND viewport_prepared_retained_profile_test ${workers} ${path})
    set_tests_properties(viewport_prepared_retained_profile_${workers}_${path} PROPERTIES
      ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 300)
  endforeach()
endforeach()
if(HCP_WITH_WARM_INTEGRATION)
  add_executable(viewport_retained_capture_check retained_capture_check.cpp)
  target_include_directories(viewport_retained_capture_check PRIVATE ../vendor)
  target_compile_options(viewport_retained_capture_check PRIVATE -Wall -Wextra -Werror)
  add_library(viewport_retained_letter_driver STATIC letter_capture.cpp)
  target_link_libraries(viewport_retained_letter_driver PRIVATE viewport_live_study_commands warm_toolbox Threads::Threads)
  target_compile_options(viewport_retained_letter_driver PRIVATE -Wall -Wextra -Wpedantic)
  add_executable(viewport_letter_capture letter_capture_main.cpp)
  target_link_libraries(viewport_letter_capture PRIVATE viewport_retained_letter_driver)
  target_link_libraries(viewport_live_study PRIVATE viewport_retained_letter_driver)
  target_compile_definitions(viewport_live_study PRIVATE HCP_HAS_RETAINED_LIVE_STUDY=1)
endif()

add_test(NAME semantic_repeated_rollup_no_unroll COMMAND semantic_native_trace_test
  --case repeated-rollup --threads 2 --direct --compiler-no-unroll)
set_tests_properties(semantic_repeated_rollup_no_unroll PROPERTIES
  ENVIRONMENT "TI_LIB_DIR=${ENGINE_TAICHI_RUNTIME_DIR}" TIMEOUT 900)

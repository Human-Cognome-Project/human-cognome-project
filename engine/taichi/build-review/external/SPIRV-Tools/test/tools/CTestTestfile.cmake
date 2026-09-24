# CMake generated Testfile for 
# Source directory: /opt/project/taichi/external/SPIRV-Tools/test/tools
# Build directory: /opt/project/taichi/build-review/external/SPIRV-Tools/test/tools
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(spirv-tools_expect_unittests "/opt/project/taichi/.venv/bin/python" "-m" "unittest" "expect_unittest.py")
set_tests_properties(spirv-tools_expect_unittests PROPERTIES  WORKING_DIRECTORY "/opt/project/taichi/external/SPIRV-Tools/test/tools" _BACKTRACE_TRIPLES "/opt/project/taichi/external/SPIRV-Tools/test/tools/CMakeLists.txt;15;add_test;/opt/project/taichi/external/SPIRV-Tools/test/tools/CMakeLists.txt;0;")
add_test(spirv-tools_spirv_test_framework_unittests "/opt/project/taichi/.venv/bin/python" "-m" "unittest" "spirv_test_framework_unittest.py")
set_tests_properties(spirv-tools_spirv_test_framework_unittests PROPERTIES  WORKING_DIRECTORY "/opt/project/taichi/external/SPIRV-Tools/test/tools" _BACKTRACE_TRIPLES "/opt/project/taichi/external/SPIRV-Tools/test/tools/CMakeLists.txt;18;add_test;/opt/project/taichi/external/SPIRV-Tools/test/tools/CMakeLists.txt;0;")
subdirs("opt")

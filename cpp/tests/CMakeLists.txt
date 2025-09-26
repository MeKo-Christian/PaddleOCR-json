# Test configuration for PaddleOCR-json

# Download and setup Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50e33e9fd7616c35ef2c85b8887894.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Test executable
add_executable(
  paddleocr_tests
  test_main.cpp
  test_base64.cpp
  test_args.cpp
  test_task.cpp
)

# Link test executable with gtest and project libraries
target_link_libraries(
  paddleocr_tests
  gtest_main
)

# Include directories for tests
target_include_directories(paddleocr_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Add test sources that need to be compiled
target_sources(paddleocr_tests PRIVATE
  ../src/base64.cpp
  ../src/args.cpp
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(paddleocr_tests)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

# Create test executable
# Automatically discover all test source files
file(GLOB TEST_SOURCES "*.cpp")

# Create test executable
add_executable(MyTests
  ${TEST_SOURCES}
)

# Link against gtest
target_link_libraries(MyTests
  GTest::gtest
  GTest::gtest_main
)

# Include directories (so tests can find your headers)
target_include_directories(MyTests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(MyTests)
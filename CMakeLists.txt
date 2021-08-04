cmake_minimum_required(VERSION 3.5)

project(CPPLispReader VERSION 0.1.0 DESCRIPTION "A Lisp Reader written in modern C++" LANGUAGES CXX)

# Options
option(ENABLE_TESTING "Enable compilation of files for testing the reader using the Catch2 submodule" OFF)

if(ENABLE_TESTING)
  message("Building test files")

  # Compile the Catch2 submodule
  add_subdirectory(ext/Catch2)

  # Add test files
  add_executable(reader_test src/test_reader.cpp)
  set_property(TARGET reader_test PROPERTY CXX_STANDARD 17)
  target_link_libraries(reader_test Catch2::Catch2)
  target_include_directories(reader_test PRIVATE include)
endif()

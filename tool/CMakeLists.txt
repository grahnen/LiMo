cmake_minimum_required(VERSION 3.30)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

project(oolin)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)


# Debugging compilation
#add_compile_options(-fsanitize=address -g)
#add_link_options(-fsanitize=address -g)

#add_compile_options(-O2 -g)
#add_link_options(-O2 -g)

add_compile_options(-O3 -march=native)
add_link_options(-O3 -march=native -latomic)

include_directories(include)

add_subdirectory(events)
add_subdirectory(monitor)

add_subdirectory(builder)
add_subdirectory(runner)
add_subdirectory(impl)
add_subdirectory(test)
add_subdirectory(logic)
add_subdirectory(linearize)
add_subdirectory(exhaustive)
add_subdirectory(automata_checker)
add_subdirectory(random)


file(GLOB IMPL_SRC
  RELATIVE "${PROJECT_SOURCE_DIR}/impl/"
  "${PROJECT_SOURCE_DIR}/impl/*.cpp")

foreach(IMPL ${IMPL_SRC})
  string(REGEX REPLACE ".cpp" "" IMPL_NAME ${IMPL}) #Get name from cpp

  add_executable("run_${IMPL_NAME}" "runner/src/null.cpp")
  target_compile_features("run_${IMPL_NAME}" PUBLIC cxx_std_20)
  target_link_libraries("run_${IMPL_NAME}" runner)
  target_link_libraries("run_${IMPL_NAME}" ${IMPL_NAME})
endforeach()

cmake_minimum_required(VERSION 3.10)

project(random)

find_package(MPI REQUIRED)


find_package(Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

#Executables
add_executable(generate "src/main.cpp")

# C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXECUTABLES "random" "random_bench" "build_lin")

foreach(EXEC ${EXECUTABLES})
  target_compile_features(${EXEC} PUBLIC cxx_std_20)
  target_compile_options(${EXEC} PUBLIC -fcoroutines)
  target_link_options(${EXEC} PUBLIC -fcoroutines)

  #Source directory
  target_include_directories(${EXEC} PUBLIC include PRIVATE include)

  #MPI
  target_include_directories(${EXEC} PUBLIC ${MPI_INCLUDE_PATH})

  #Link Libs
  target_link_libraries(${EXEC} LINK_PUBLIC ${Boost_LIBRARIES} events logic monitor MPI::MPI_CXX)
endforeach()

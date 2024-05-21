cmake_minimum_required(VERSION 3.10)

project(automata_checker)


find_package(MPI REQUIRED)

#Sources
file(GLOB SRC CONFIGURE_DEPENDS "src/main.cpp")

#Executables
add_executable(automata_checker ${SRC})

# C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_compile_features(automata_checker PUBLIC cxx_std_20)
target_compile_options(automata_checker PUBLIC -fcoroutines)
target_link_options(automata_checker PUBLIC -fcoroutines)


#Source directory
target_include_directories(automata_checker PUBLIC include PRIVATE include)


#MPI
target_include_directories(automata_checker PUBLIC ${MPI_INCLUDE_PATH})


#Boost
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

#Link Libs
target_link_libraries(automata_checker LINK_PUBLIC ${Boost_LIBRARIES} events logic monitor MPI::MPI_CXX)

cmake_minimum_required(VERSION 3.25)

project(runner)

#C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Sources
file(GLOB SRC CONFIGURE_DEPENDS "src/*.hpp" "src/*.cpp")

#Executables
add_library(runner ${SRC})

target_compile_features(runner PUBLIC cxx_std_20)

#PThreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(runner PRIVATE Threads::Threads)

#Source directory
target_include_directories(runner PUBLIC include PRIVATE src)

#Boost
find_package(Boost CONFIG COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

#Link Boost
target_link_libraries(runner LINK_PUBLIC ${Boost_LIBRARIES} events monitor atomic)

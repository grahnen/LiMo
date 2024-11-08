cmake_minimum_required(VERSION 3.30)

project(linearize)

#Sources
file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp")

#Executables
add_executable(linearize ${SRC})

# C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_compile_features(linearize PUBLIC cxx_std_20)

#Source directory
target_include_directories(linearize PUBLIC include PRIVATE include)

#Boost
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

#Link Libs
target_link_libraries(linearize LINK_PUBLIC ${Boost_LIBRARIES} events logic monitor)

cmake_minimum_required(VERSION 3.30)

project(builder)

#C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Sources
file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp")

#Executables
add_executable(builder ${SRC})

target_compile_features(builder PUBLIC cxx_std_20)

#Source directory
target_include_directories(builder PUBLIC include PRIVATE src)

#Boost
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

#Link Boost
target_link_libraries(builder LINK_PUBLIC ${Boost_LIBRARIES} events monitor)

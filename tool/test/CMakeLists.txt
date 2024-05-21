cmake_minimum_required(VERSION 3.10)

project(test)

#Sources
file( GLOB SRCS "src/*.cpp" )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

foreach(test ${SRCS})
  get_filename_component(name ${test} NAME_WLE)
  set(t test_${name})
  add_executable(${t} ${test})
  target_compile_features(${t} PUBLIC cxx_std_20)
  target_include_directories(${t} PUBLIC include PRIVATE include)
  target_link_libraries(${t} LINK_PUBLIC ${Boost_LIBRARIES} events logic)
  target_compile_options(${t} PUBLIC -fcoroutines)
  target_link_options(${t} PUBLIC -fcoroutines)
endforeach(test ${COND_SRCS})

# #Executables
# add_executable(test_propagation "src/propagation.cpp")
# add_executable(test_interval "src/interval.cpp")
# add_executable(test_logic "src/logic.cpp")
# add_executable(test_relation "src/relation.cpp")
# add_executable(test_restriction "src/restriction.cpp")
# add_executable(test_regex "src/regex.cpp")
# add_executable(test_subsumption "src/subsumption.cpp")
# add_executable(test_generation "src/generation.cpp")

# # C++ 20
# target_compile_features(test_interval PUBLIC cxx_std_20)
# target_compile_features(test_logic PUBLIC cxx_std_20)
# target_compile_features(test_relation PUBLIC cxx_std_20)
# target_compile_features(test_restriction PUBLIC cxx_std_20)
# target_compile_features(test_propagation PUBLIC cxx_std_20)
# target_compile_features(test_regex PUBLIC cxx_std_20)
# target_compile_features(test_subsumption PUBLIC cxx_std_20)
# target_compile_features(test_generation PUBLIC cxx_std_20)

# #Source directory
# target_include_directories(test_interval PUBLIC include PRIVATE include)
# target_include_directories(test_logic PUBLIC include PRIVATE include)
# target_include_directories(test_relation PUBLIC include PRIVATE include)
# target_include_directories(test_restriction PUBLIC include PRIVATE include)
# target_include_directories(test_regex PUBLIC include PRIVATE include)
# target_include_directories(test_generation PUBLIC include PRIVATE include)



#Boost
# find_package(Boost COMPONENTS program_options REQUIRED)
# include_directories( ${Boost_INCLUDE_DIR} )

# #Link Libs
# target_link_libraries(test_interval LINK_PUBLIC ${Boost_LIBRARIES} events)
# target_link_libraries(test_logic LINK_PUBLIC ${Boost_LIBRARIES} logic)
# target_link_libraries(test_logic LINK_PUBLIC ${Boost_LIBRARIES} events)
# target_link_libraries(test_relation LINK_PUBLIC ${Boost_LIBRARIES} logic)
# target_link_libraries(test_relation LINK_PUBLIC ${Boost_LIBRARIES} events)
# target_link_libraries(test_restriction LINK_PUBLIC ${Boost_LIBRARIES} events logic)
# target_link_libraries(test_propagation LINK_PUBLIC events logic)
# target_link_libraries(test_regex LINK_PUBLIC events logic)
# target_link_libraries(test_subsumption LINK_PUBLIC events logic)

# target_compile_options(test_generation PUBLIC -fcoroutines)
# target_link_options(test_generation PUBLIC -fcoroutines)

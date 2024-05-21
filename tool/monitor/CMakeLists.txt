file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp")
add_library(monitor ${SRC})
target_link_libraries(monitor LINK_PUBLIC ${Boost_LIBRARIES} events logic)

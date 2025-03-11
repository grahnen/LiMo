project(impl)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB IMPL_SRC RELATIVE "${PROJECT_SOURCE_DIR}" *.cpp)

foreach(IMPL ${IMPL_SRC})
  string(REGEX REPLACE ".cpp" "" IMPL_NAME ${IMPL})
  add_library(${IMPL_NAME} ${IMPL})
  target_include_directories(${IMPL_NAME} PUBLIC include
    PRIVATE "../run/src")
  target_include_directories(${IMPL_NAME} PUBLIC include
    PRIVATE "../oolin/src")
  target_compile_features(${IMPL_NAME} PUBLIC cxx_std_20)
  target_link_libraries(${IMPL_NAME} PUBLIC atomic)

endforeach()

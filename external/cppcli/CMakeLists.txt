cmake_minimum_required(VERSION 2.8)
project (cppcli)

# C++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(CMAKE_CXX_FLAGS_ANALYSIS "${CMAKE_CXX_FLAGS} --analyse")

### Include google test
if(NOT TARGET gtest)
  add_subdirectory(external/googletest)
endif()

### If boost was not already included elsewhere, get it here
if(NOT Boost_INCLUDE_DIR)
  find_package(Boost)
  message(STATUS "Using Boost headers from ${Boost_INCLUDE_DIR}")
endif()

### Find logging library
if(NOT CPPLOGGING_INCLUDE_DIR)
  find_package(cpplogging)
  if(NOT CPPLOGGING_FOUND)
    message(STATUS "Using built-in cpplogging")
    add_subdirectory(external/cpplogging)
    SET(CPPLOGGING_INCLUDE_DIR external/cpplogging/include)
  endif()
endif()

### Trick to add header files to projects in IDEs like Visual Studio and XCode
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS "include/**.h")
file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES "src/**.cpp")

add_custom_target(${PROJECT_NAME}_headers SOURCES ${${PROJECT_NAME}_HEADERS})

include_directories(include ${CPPLOGGING_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
add_library (${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
target_link_libraries(${PROJECT_NAME} cpplogging)

install (TARGETS ${PROJECT_NAME}
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib)
install (FILES ${${PROJECT_NAME}_HEADERS} DESTINATION include)

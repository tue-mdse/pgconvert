cmake_minimum_required(VERSION 2.6)
project(pgconvert)

find_package(Boost 1.44 REQUIRED COMPONENTS regex)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

set(MCRL2_LIB "" CACHE PATH "Path to mCRL2 lib dir.")
set(MCRL2_INCLUDE "" CACHE PATH "Path to mCRL2 include dir.")

link_directories(${MCRL2_LIB})
include_directories(include ${MCRL2_INCLUDE})
add_library(libmcrl2_utilities STATIC IMPORTED)

file(GLOB_RECURSE HEADERS include/*.h)
add_custom_target(${PROJECT_NAME}_headers
  SOURCES ${HEADERS}
)
add_executable(pgconvert
  src/pgconvert.cpp 
  src/equivalence.cpp 
)
add_executable(autscc
  src/autscc.cpp
)


target_link_libraries(pgconvert mcrl2_utilities ${Boost_LIBRARIES})
target_link_libraries(autscc mcrl2_utilities ${Boost_LIBRARIES})


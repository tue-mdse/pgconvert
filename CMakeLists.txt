cmake_minimum_required(VERSION 2.6)
project(pgconvert)

find_package(Boost 1.44 REQUIRED COMPONENTS regex)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

add_subdirectory(external/cppcli)
set(CPPCLI_INCLUDE_DIR external/cppcli/include)
set(CPPLOGGING_INCLUDE_DIR external/cppcli/external/cpplogging/include)

include_directories(include ${CPPLOGGING_INCLUDE_DIR} ${CPPCLI_INCLUDE_DIR})

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


target_link_libraries(pgconvert cpplogging cppcli ${Boost_LIBRARIES})
target_link_libraries(autscc cpplogging cppcli ${Boost_LIBRARIES})


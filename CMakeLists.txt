cmake_minimum_required(VERSION 2.6)
project (pgconvert)
set (MCRL2_LIB "" CACHE PATH "Path to mCRL2 lib dir.")
set (MCRL2_INCLUDE "" CACHE PATH "Path to mCRL2 include dir.")
link_directories (${MCRL2_LIB})
include_directories (include ${MCRL2_INCLUDE})
add_library (libmcrl2_core UNKNOWN IMPORTED)
add_library (libmcrl2_aterm UNKNOWN IMPORTED)
add_library (libmcrl2_utilities_command_line UNKNOWN IMPORTED)
add_executable (pgconvert 
  src/pgconvert.cpp 
  src/equivalence.cpp 
)
target_link_libraries (pgconvert mcrl2_core mcrl2_aterm mcrl2_utilities_command_line)


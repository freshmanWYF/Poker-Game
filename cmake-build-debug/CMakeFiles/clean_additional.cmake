# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\PokerServer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\PokerServer_autogen.dir\\ParseCache.txt"
  "PokerServer_autogen"
  )
endif()

cmake_minimum_required(VERSION 3.16.3)

set(PROGRAM haVoc)

project(haVoc
  VERSION 1.0.0
  DESCRIPTION "Hobby chess project"
  LANGUAGES CXX
)

###################################################################
# Options
###################################################################


###################################################################
# Source Files
###################################################################
set(SRC_FILES
  bitboards.cpp
  evaluate.cpp
  hashtable.cpp
  haVoc.cpp
  magics.cpp
  material.cpp
  order.cpp
  pawns.cpp
  pgn.cpp
  position.cpp
  threads.cpp
  uci.cpp
  zobrist.cpp
)

###################################################################
# Build Configuration
###################################################################
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build configuration not set, setting configuration to Release.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration type: Debug Release.")
endif()

###################################################################
# Compilation Options
###################################################################
if(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000")
  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /W3 /GR /EHsc /D_64BIT /D_CONSOLE /D_UNICODE /D_WIN32 /D_WIN64 /D_MSC_VER=1939 /std:c++17 /GS /GL /W3 /Gy /Zi /Gm- /O2 /Ob2 /Zc:inline /fp:precise /GT /WX- /Ot /FC /Oi /MD")
  elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /JMC /EHsc /GS /W3 /ZI /Gm- /Od /Zc:inline /fp:precise /WX- /RTC1 /Gd /MDd /FC /D_DEBUG /D_64BIT /D_CONSOLE /D_UNICODE /D_WIN32 /D_WIN64 /D_MSC_VER=1939 /std:c++17")
  endif()
else()
  set(DBG_OPTS "-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-O0;-g3;-ggdb")
  set(REL_OPTS "")
endif()


add_executable(${PROGRAM} ${SRC_FILES})



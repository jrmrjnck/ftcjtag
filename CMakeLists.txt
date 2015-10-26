cmake_minimum_required(VERSION 2.6)

project(ftcjtag)
add_library(ftcjtag SHARED FT2232c.cpp FT2232h.cpp FT2232hMpsseJtag.cpp FTCJTAG.cpp)
set_target_properties(ftcjtag PROPERTIES CXX_VISIBILITY_PRESET hidden)

set(FTD2XX_DIR "" CACHE PATH "Path to FTD2XX driver distributable directory, if not provided by system")
set(FTD2XX_INCLUDE_DIR ${FTD2XX_DIR} CACHE PATH "Path to FTD2XX headers, if not provided by system")
set(FTD2XX_LIB_DIR ${FTD2XX_DIR} CACHE PATH "Path to FTD2XX library, if not provided by system")
include_directories(${FTD2XX_INCLUDE_DIR})
find_library(FTD2XX_LIB NAMES ftd2xx ftd2xx64 PATHS ${FTD2XX_LIB_DIR})
target_link_libraries(ftcjtag ${FTD2XX_LIB})

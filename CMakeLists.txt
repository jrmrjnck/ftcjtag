cmake_minimum_required(VERSION 2.6)

project(ftcjtag)

set(FTD2XX_INCLUDE_DIR "" CACHE PATH "Path to external FTD2XX headers, if needed.")
include_directories(${FTD2XX_INCLUDE_DIR})

set(FTCJTAG_SOURCES FT2232c.cpp FT2232h.cpp FT2232hMpsseJtag.cpp FTCJTAG.cpp)
add_library(ftcjtag-static STATIC ${FTCJTAG_SOURCES})
add_library(ftcjtag SHARED ${FTCJTAG_SOURCES})
set_target_properties(ftcjtag-static PROPERTIES OUTPUT_NAME ftcjtag)
set_target_properties(ftcjtag PROPERTIES CXX_VISIBILITY_PRESET hidden)

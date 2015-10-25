cmake_minimum_required(VERSION 2.6)
project(ftcjtag)
add_library(ftcjtag SHARED FT2232c.cpp FT2232h.cpp FT2232hMpsseJtag.cpp FTCJTAG.cpp)

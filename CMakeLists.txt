cmake_minimum_required(VERSION 3.12)
project(noodlec)

option(NOODLEC_EXAMPLES "Enables building of examples" ON)

add_library(noodlec STATIC "Source/noodle.c")
target_include_directories(noodlec PUBLIC "Include")


if (NOODLEC_EXAMPLES)
    add_subdirectory("Examples")
endif()
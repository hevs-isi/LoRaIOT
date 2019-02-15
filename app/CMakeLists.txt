# Boilerplate code, which pulls in the Zephyr build system.
cmake_minimum_required(VERSION 3.13.1)
macro(set_conf_file)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(loraiot)
# Add all sources files from src/ recuresively
FILE(GLOB_RECURSE csources src/*.c)
target_sources(app PRIVATE ${csources})

# add inc/ to include dir
target_include_directories(app PRIVATE inc/)

#FILE(GLOB app_sources src/*.c)
#target_sources(app PRIVATE ${app_sources})


execute_process(COMMAND git submodule update --init --recursive)

add_library(date INTERFACE)
target_include_directories(date INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/date/include/)


add_library(json INTERFACE)
target_include_directories(json SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/include/)
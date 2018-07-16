file(GLOB HEADERS "include/enumivo/ram_plugin/*.hpp")
add_library( ram_plugin
             ram_plugin.cpp
             ${HEADERS} )

target_link_libraries( ram_plugin chain_plugin enumivo_chain appbase fc )
target_include_directories( ram_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

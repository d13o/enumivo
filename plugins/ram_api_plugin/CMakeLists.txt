file( GLOB HEADERS "include/enumivo/ram_api_plugin/*.hpp" )
add_library( ram_api_plugin
             ram_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( ram_api_plugin ram_plugin chain_plugin http_plugin appbase )
target_include_directories( ram_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

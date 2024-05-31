file(GLOB_RECURSE SOURCES "*.cpp" "*.h")

add_library(${SPARKS_SUBLIB_NAME} ${SOURCES})

list(APPEND SPARKS_LIBS ${SPARKS_SUBLIB_NAME})
set(SPARKS_LIBS ${SPARKS_LIBS} PARENT_SCOPE)

target_include_directories(${SPARKS_SUBLIB_NAME} PUBLIC ${SPARKS_INCLUDE_DIR})

target_link_libraries(${SPARKS_SUBLIB_NAME} PUBLIC LongMarch sparks_utils sparks_assets sparks_asset_manager sparks_scene sparks_renderer)

PACK_SHADER_CODE(${SPARKS_SUBLIB_NAME})

file(GLOB SOURCES "*.cpp" "*.h")

add_executable(sparks ${SOURCES})

target_include_directories(sparks PUBLIC ${SPARKS_INCLUDE_DIR})

file(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

foreach (subdir ${subdirs})
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
        set(SPARKS_SUBLIB_NAME sparks_${subdir})
        add_subdirectory(${subdir})
    endif ()
endforeach ()

message(STATUS "SPARKS_LIBS: ${SPARKS_LIBS}")

target_link_libraries(sparks ${SPARKS_LIBS})

target_link_libraries(sparks LongMarch)

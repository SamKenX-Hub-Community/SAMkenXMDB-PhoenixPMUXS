include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(maxavro maxavro.cc maxavro_schema.cc maxavro_record.cc maxavro_file.cc)
target_link_libraries(maxavro maxscale-common ${JANSSON_LIBRARIES})

add_executable(maxavrocheck maxavrocheck.cc)
target_link_libraries(maxavrocheck maxavro maxscale-common)
install_executable(maxavrocheck core)
add_subdirectory(test)

cmake_minimum_required(VERSION 3.10)

set(HIVEMIND_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/hivemind)
set(HIVEMIND_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/libhivemind.so)

add_executable(platform-console main.cpp)

target_include_directories(platform-console PRIVATE
    ${HIVEMIND_INCLUDE}
)

target_link_libraries(platform-console
    ${HIVEMIND_LIB}
    pthread
)

add_custom_command(TARGET platform-console POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${HIVEMIND_LIB}
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying libhivemind.so to build directory"
)

add_custom_command(TARGET platform-console POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        platform-console
        ${CMAKE_CURRENT_BINARY_DIR}/platform
    COMMENT "Creating symlink platform -> platform-console"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
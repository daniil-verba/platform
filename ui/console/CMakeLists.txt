cmake_minimum_required(VERSION 3.10)
project(Platform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(platform main.cpp)

# Путь к заголовкам Hivemind (на уровень выше в libs/hivemind)
target_include_directories(platform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs
)

# Путь к библиотеке (на уровень выше в libs)
target_link_directories(platform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs
)

# Линкуем Hivemind и libsodium (который нужен Hivemind)
target_link_libraries(platform
    hivemind
    sodium
    pthread
)

# Копируем .so в папку сборки (для удобства запуска)
add_custom_command(TARGET platform POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/libhivemind.so
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying libhivemind.so to build directory"
)

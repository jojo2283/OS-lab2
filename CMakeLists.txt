cmake_minimum_required(VERSION 3.10)

# Название проекта
project(block_cache LANGUAGES C)

# Включение отладочной информации и уровня предупреждений
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Установка пути для заголовочных файлов
include_directories(include)

# Установка пути для сборки динамической библиотеки
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Добавление библиотеки
add_library(block_cache SHARED src/cache.c)

# Пример использования
add_executable(example examples/example.c)
target_link_libraries(example block_cache)

# Тесты
enable_testing()
add_executable(test_cache tests/test_cache.c)
target_link_libraries(test_cache block_cache)
add_test(NAME CacheTest COMMAND test_cache)

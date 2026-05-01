cmake_minimum_required(VERSION 3.24..3.31)
project(FlexLayoutDesigner)

cmake_policy(SET CMP0091 NEW)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB SOURCE_FILES *.c)
file(GLOB HEADER_FILES *.h)
list(APPEND SOURCE_FILES flex/flex.c)
list(APPEND HEADER_FILES flex/flex.h)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        shlwapi.lib
        comctl32.lib
)

target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE /wd4996)
target_compile_options(${PROJECT_NAME} PRIVATE /UUNICODE /U_UNICODE)


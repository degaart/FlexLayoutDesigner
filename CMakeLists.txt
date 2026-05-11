cmake_minimum_required(VERSION 3.24..3.31)
project(FlexLayoutDesigner)

cmake_policy(SET CMP0091 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB SOURCE_FILES *.c)
file(GLOB HEADER_FILES *.h)

file(GLOB_RECURSE YOGA_SOURCES
    LIST_DIRECTORIES FALSE
    yoga/yoga/*.cpp)
set(YOGA_HEADERS yoga/yoga/Yoga.h)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${YOGA_SOURCES} ${YOGA_HEADERS})
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        shlwapi.lib
        comctl32.lib
)

target_include_directories(${PROJECT_NAME} PRIVATE yoga)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE /wd4996 /wd5105)
target_compile_options(${PROJECT_NAME} PRIVATE /UUNICODE /U_UNICODE)


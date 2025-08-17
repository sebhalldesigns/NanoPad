cmake_minimum_required(VERSION 3.10)

add_subdirectory(NanoKit)

message(STATUS "NKGEN_CMAKE: ${NKGEN_CMAKE}")

include(${NKGEN_CMAKE})

project(NanoPad)

if (NOT EMSCRIPTEN)
    set(EXE_NAME ${CMAKE_PROJECT_NAME})
else()
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(SHELL_FILE ${CMAKE_SOURCE_DIR}/shell.html)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
        -s USE_WEBGL2=1 \
        -s FULL_ES3=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s EXPORTED_FUNCTIONS=\"['_main', '_malloc', '_free', '_AsyncWebFileCallback', 'stringToUTF8']\" \
        -g \
        -gsource-map \
        --shell-file ${SHELL_FILE} \
    ")

    set(EXE_NAME "index")
endif()

add_executable(${EXE_NAME}
    src/main.c
)

target_link_libraries(${EXE_NAME} PUBLIC
    NanoKit
)

target_include_directories(${EXE_NAME} PUBLIC

)

generate_modules(${EXE_NAME}
    src/Window   
)

cmake_minimum_required(VERSION 3.20)
project(Animation LANGUAGES C)


# Check if Ninja is available
find_program(NINJA_COMMAND ninja PATHS "$ENV{PATH}")
if(NOT APPLE)
    if(NINJA_COMMAND)
        set(CMAKE_MAKE_PROGRAM "${NINJA_COMMAND}" CACHE INTERNAL "")
        set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "")
        message(STATUS "Using Ninja generator: ${NINJA_COMMAND}")
    endif()
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD     11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS   OFF)

# Place all executables under build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (NOT APPLE)
  set(GGML_VULKAN ON CACHE BOOL "Enable Vulkan GPU support" FORCE)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# ————————————————————————————————————————————————————————————————

# Collect all your .c files
file(GLOB_RECURSE SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/raylib"    # path to your clone
  "${CMAKE_CURRENT_BINARY_DIR}/raylib_build"     # where raylib’s objects go
)

add_executable(animation ${SOURCES})

# Collect all your test .c files
# 4) Tell it where to find headers
target_include_directories(animation
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"                       # your own headers
)
    
# 5) Link in the whisper library
target_link_libraries(animation
  PRIVATE
    raylib
)
target_compile_definitions(animation PRIVATE
    SOURCE_CODE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/\"
)


# ————————————————————————————————————————————————————————————————
# (Optional) any other libs you had in lib/
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/extern")
# e.g. target_link_libraries(client PRIVATE foo)
# ————————————————————————————————————————————————————————————————

add_compile_definitions(FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/extern/police/latinmodern-math-1959/otf/latinmodern-math.otf")
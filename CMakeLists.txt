cmake_minimum_required(VERSION 3.20)
project(Animation LANGUAGES C CXX)

# --- Optional: prefer Ninja when available (comme tu avais) ---
find_program(NINJA_COMMAND ninja PATHS "$ENV{PATH}")
if(NOT APPLE)
  if(NINJA_COMMAND)
    set(CMAKE_MAKE_PROGRAM "${NINJA_COMMAND}" CACHE INTERNAL "")
    set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "")
    message(STATUS "Using Ninja generator: ${NINJA_COMMAND}")
  endif()
  set(GGML_VULKAN ON CACHE BOOL "Enable Vulkan GPU support" FORCE)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)

# outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   # nouveau

# --- build extern libraries first (raylib) ---
add_subdirectory(
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/raylib" 
  "${CMAKE_CURRENT_BINARY_DIR}/raylib_build"
)

set(FFTW3_ROOT "C:/Program Files/Common Files/fftw3")
include_directories("${FFTW3_ROOT}/include")
link_directories("${FFTW3_ROOT}/lib")

# --- library commune (src) ---
add_subdirectory(src)

# --- executables (chaque sous-dossier video/* aura son propre CMakeLists) ---
add_subdirectory(video)
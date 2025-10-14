# src/CMakeLists.txt
file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
set(SRC_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/fft_wrapper.cpp")

# crée une librairie statique "myvisual"

add_library(myvisual STATIC ${SRC_FILES} ${SRC_CPP_FILES})
# headers publics
target_include_directories(myvisual
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include   # visual.h, etc.
)


# Si visual.c utilise raylib, link la ici pour que ce soit transitif
target_link_libraries(myvisual
  PUBLIC
    raylib
)
set_target_properties(myvisual PROPERTIES LINKER_LANGUAGE CXX)
# exemple : définir un macro publicement si tu veux
target_compile_definitions(myvisual PUBLIC SOURCE_CODE_PATH=\"${CMAKE_SOURCE_DIR}/\")

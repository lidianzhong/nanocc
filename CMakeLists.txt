cmake_minimum_required(VERSION 3.13)
project(compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# settings
# set to OFF to enable C mode
set(CPP_MODE ON)
if(CPP_MODE)
  set(FB_EXT ".cpp")
else()
  set(FB_EXT ".c")
endif()
message(STATUS "Flex/Bison generated source file extension: ${FB_EXT}")

# enable all warnings
if(MSVC)
  add_compile_options(/W3)
else()
  # disable warnings caused by old version of Flex
  add_compile_options(-Wall -Wno-register)
endif()

if(EMSCRIPTEN)
  set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/koopa/lib/emscripten" CACHE STRING "directory of libraries" FORCE)
else()
  set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/koopa/lib/x86" CACHE STRING "directory of libraries" FORCE)
endif()

message(STATUS "Library directory: ${LIB_DIR}")
message(STATUS "Include directory: ${INC_DIR}")

# find Flex/Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# generate lexer/parser
file(GLOB_RECURSE L_SOURCES "src/frontend/*.l")
file(GLOB_RECURSE Y_SOURCES "src/frontend/*.y")
if(NOT (L_SOURCES STREQUAL "" AND Y_SOURCES STREQUAL ""))
  string(REGEX REPLACE ".*/(.*)\\.l" "${CMAKE_CURRENT_BINARY_DIR}/\\1.lex${FB_EXT}" L_OUTPUTS "${L_SOURCES}")
  string(REGEX REPLACE ".*/(.*)\\.y" "${CMAKE_CURRENT_BINARY_DIR}/\\1.tab${FB_EXT}" Y_OUTPUTS "${Y_SOURCES}")
  flex_target(Lexer ${L_SOURCES} ${L_OUTPUTS})
  bison_target(Parser ${Y_SOURCES} ${Y_OUTPUTS})
  add_flex_bison_dependency(Lexer Parser)
endif()

# project link directories
link_directories(${LIB_DIR})

# project include directories
include_directories(third_party/koopa/include)
include_directories(src)
include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${INC_DIR})

# all of C/C++ source files
file(GLOB_RECURSE C_SOURCES "src/*.c")
file(GLOB_RECURSE CXX_SOURCES "src/*.cpp")
file(GLOB_RECURSE CC_SOURCES "src/*.cc")
file(GLOB_RECURSE H_SOURCES "src/*.h")
set(SOURCES ${C_SOURCES} ${CXX_SOURCES} ${CC_SOURCES} ${H_SOURCES}
      ${FLEX_Lexer_OUTPUTS} ${BISON_Parser_OUTPUT_SOURCE})

# Filter out main.cpp and wasm specific files from common sources
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*wasm_.*\\.cpp$")

if(EMSCRIPTEN)
  add_executable(nanocc_wasm wasm/nanocc_wasm_entry.cpp ${SOURCES})
  set_target_properties(nanocc_wasm PROPERTIES OUTPUT_NAME "nanocc" SUFFIX ".js")
  target_link_libraries(nanocc_wasm koopa)
  target_link_options(nanocc_wasm PRIVATE 
    "-lembind" 
    "-sMODULARIZE=1" 
    "-sEXPORT_NAME='NanoccCompiler'"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sEXIT_RUNTIME=0"
  )

  # Copy HTML and helper JS to output directory
  add_custom_command(TARGET nanocc_wasm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/wasm/main.html"
      "${CMAKE_CURRENT_SOURCE_DIR}/wasm/miniriscv.js"
      "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/wasm/examples"
      "${CMAKE_CURRENT_BINARY_DIR}/examples"
    COMMENT "Copying Wasm frontend resources and examples"
  )
else()
  # executable
  add_executable(compiler src/main.cpp ${SOURCES})
  set_target_properties(compiler PROPERTIES C_STANDARD 11 CXX_STANDARD 17)
  target_link_libraries(compiler koopa pthread dl)
endif()

# add clang-format target
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include -name "*.cpp" -o -name "*.h" -o -name "*.c" | xargs clang-format -i
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting source code"
)
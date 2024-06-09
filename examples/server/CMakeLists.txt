set(TARGET server)
option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
option(LLAMA_SERVER_SSL "Build SSL support for the server" OFF)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set(TARGET_SRCS
    server.cpp
    utils.hpp
    httplib.h
)
set(PUBLIC_ASSETS
    colorthemes.css
    style.css
    theme-beeninorder.css
    theme-ketivah.css
    theme-mangotango.css
    theme-playground.css
    theme-polarnight.css
    theme-snowstorm.css
    index.html
    index-new.html
    index.js
    completion.js
    system-prompts.js
    prompt-formats.js
    json-schema-to-grammar.mjs
)
foreach(asset ${PUBLIC_ASSETS})
    set(input "${CMAKE_CURRENT_SOURCE_DIR}/public/${asset}")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${asset}.hpp")
    list(APPEND TARGET_SRCS ${output})
    add_custom_command(
        DEPENDS "${input}"
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${input}" "-DOUTPUT=${output}" -P "${PROJECT_SOURCE_DIR}/scripts/xxd.cmake"
    )
endforeach()
add_executable(${TARGET} ${TARGET_SRCS})
install(TARGETS ${TARGET} RUNTIME)
target_compile_definitions(${TARGET} PRIVATE
    SERVER_VERBOSE=$<BOOL:${LLAMA_SERVER_VERBOSE}>
)
target_link_libraries(${TARGET} PRIVATE common ${CMAKE_THREAD_LIBS_INIT})
if (LLAMA_SERVER_SSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()
if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
target_compile_features(${TARGET} PRIVATE cxx_std_11)

set(TARGET rpcserver)
option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(${TARGET} SHARED server.cpp ../../common/json.hpp) # httplib.h rpcserver.h

set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

#set_target_properties(${TARGET} PROPERTIES RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ggml-metal.metal")

target_compile_definitions(${TARGET} PRIVATE LLAMA_SHARED LLAMA_BUILD JSONRPC)

install(TARGETS ${TARGET} LIBRARY PUBLIC_HEADER)

# llava - someday?

target_link_libraries(${TARGET} PRIVATE common llama ggml ${CMAKE_THREAD_LIBS_INIT} ${LLAMA_EXTRA_LIBS})
if (LLAMA_CLBLAST)
  target_link_libraries(ggml_shared PUBLIC Threads::Threads ${LLAMA_EXTRA_LIBS} OpenCL clblast)
endif()
if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
target_compile_features(${TARGET} PRIVATE cxx_std_11)

# Add -fno-exceptions option
target_compile_options(${TARGET} PRIVATE) # TODO -fno-exceptions)

if(TARGET BUILD_INFO)
  add_dependencies(${TARGET} BUILD_INFO)
endif()
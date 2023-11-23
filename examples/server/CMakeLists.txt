set(TARGET server)
option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(${TARGET} server.cpp json.hpp httplib.h)
install(TARGETS ${TARGET} RUNTIME)
target_compile_definitions(${TARGET} PRIVATE
    SERVER_VERBOSE=$<BOOL:${LLAMA_SERVER_VERBOSE}>
)
target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
target_compile_features(${TARGET} PRIVATE cxx_std_11)
if(TARGET BUILD_INFO)
  add_dependencies(${TARGET} BUILD_INFO)
endif()

set(TARGET server_oaicompat)
option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(${TARGET} server_oaicompat.cpp json.hpp httplib.h)
install(TARGETS ${TARGET} RUNTIME)
target_compile_definitions(${TARGET} PRIVATE
    SERVER_VERBOSE=$<BOOL:${LLAMA_SERVER_VERBOSE}>
)
target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
target_compile_features(${TARGET} PRIVATE cxx_std_11)
if(TARGET BUILD_INFO)
  add_dependencies(${TARGET} BUILD_INFO)
endif()

set(TARGET rpcserver)
option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(${TARGET} SHARED rpcserver.cpp json.hpp httplib.h rpcserver.h)
set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

#set_target_properties(${TARGET} PROPERTIES RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ggml-metal.metal")

target_compile_definitions(${TARGET} PRIVATE LLAMA_SHARED LLAMA_BUILD)
install(TARGETS ${TARGET} LIBRARY PUBLIC_HEADER)
target_link_libraries(${TARGET} PRIVATE common llama ggml ${CMAKE_THREAD_LIBS_INIT} ${LLAMA_EXTRA_LIBS})
if (LLAMA_CLBLAST)
  target_link_libraries(ggml_shared PUBLIC Threads::Threads ${LLAMA_EXTRA_LIBS} OpenCL clblast)
endif()
if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
target_compile_features(${TARGET} PRIVATE cxx_std_11)

# Add -fno-exceptions option
target_compile_options(${TARGET} PRIVATE -fno-exceptions)

if(TARGET BUILD_INFO)
  add_dependencies(${TARGET} BUILD_INFO)
endif()

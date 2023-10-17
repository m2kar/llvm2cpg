set (SOURCES
  CPGEmitter.cpp
  CPGProtoWriter.cpp
  CPGProtoAdapter.cpp
  CPGProtoBuilder.cpp
  CPGProtoNode.cpp
  CPGTypeEmitter.cpp
  )

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-unknown-pragmas -Wno-sign-compare)
endif()

add_library(CPGProtoWriter ${SOURCES})
target_compile_options(CPGProtoWriter PRIVATE ${LLVM2CPG_CXX_FLAGS})
target_link_libraries(CPGProtoWriter CPG CPGProto CPGLogger CPGDemangler LLVMExt protobuf zip)
target_include_directories(CPGProtoWriter SYSTEM PRIVATE /usr/local/include)


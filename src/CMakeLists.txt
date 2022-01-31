llvm_map_components_to_libnames(llvm_libs bitwriter core ipo irreader instcombine instrumentation target linker analysis scalaropts support )

add_executable(svf-server svf-server.cpp ConsoleInput.cpp)

target_link_libraries(svf-server ${SVF_LIB} ${LLVMCudd} ${llvm_libs})
target_link_libraries(svf-server ${Z3_DIR}/build/libz3.a )
set_target_properties( svf-server PROPERTIES
                       RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

add_library(svf-plugin SHARED svf-plugin.cpp)

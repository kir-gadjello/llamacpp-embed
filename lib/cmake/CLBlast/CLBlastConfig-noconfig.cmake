#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "clblast" for configuration ""
set_property(TARGET clblast APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(clblast PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.0.sdk/System/Library/Frameworks/OpenCL.framework"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libclblast.a"
  )

list(APPEND _cmake_import_check_targets clblast )
list(APPEND _cmake_import_check_files_for_clblast "${_IMPORT_PREFIX}/lib/libclblast.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

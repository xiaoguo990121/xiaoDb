# Read rocksdb version from version.h header file.

function(get_xiaodb_version version_var)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/xiaodb/version.h" version_header_file)
  foreach(component MAJOR MINOR PATCH)
    string(REGEX MATCH "#define XIAODB_${component} ([0-9]+)" _ ${version_header_file})
    set(XIAODB_VERSION_${component} ${CMAKE_MATCH_1})
  endforeach()
  set(${version_var} "${XIAODB_VERSION_MAJOR}.${XIAODB_VERSION_MINOR}.${XIAODB_VERSION_PATCH}" PARENT_SCOPE)
endfunction()

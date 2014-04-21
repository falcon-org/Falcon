# Utility function for building thrift services.

# Function for building a thrift file to cpp.
# if the thriftFile is my/thrift/file.thrift, the output cpp file will be in
# ${CMAKE_BINARY_DIR}/my/thrift/file.cpp
#
# Notes:
# - it is expected that the thrift file defines only one service.
# - This service must have the same name as the thrift file.
function(buildThriftCpp thriftFile)
  get_filename_component(fileName "${thriftFile}" NAME_WE)
  get_filename_component(filePath "${thriftFile}" PATH)
  set(OUTPUT_PREFIX ${CMAKE_BINARY_DIR}/${filePath}/gen-cpp/${fileName})
  set(OUTPUT_FILES
    ${OUTPUT_PREFIX}.cpp
    ${OUTPUT_PREFIX}_types.cpp
    ${OUTPUT_PREFIX}_constants.cpp
    ${OUTPUT_PREFIX}.h
    ${OUTPUT_PREFIX}_types.h
    ${OUTPUT_PREFIX}_constants.h)
  add_custom_command(
    OUTPUT ${OUTPUT_FILES}
    COMMAND ${thrift_COMPILER} --gen cpp -o ${CMAKE_BINARY_DIR}/${filePath} ${thriftFile}
    DEPENDS ${thriftFile}
    COMMENT Generating cpp thrift module for ${thriftFile}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  # Create output directory
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${filePath})
endfunction()

# Function for building a thrift file to python modules.
# If the thriftFile is my/thrift/file.thrift, the two generated files will be:
# - ${CMAKE_BINARY_DIR/my/thrift/file/file.py
# - ${CMAKE_BINARY_DIR/my/thrift/file/ttypes.py
#
# Notes:
# - it is expected that the thrift file defines only one service.
# - This service must have the same name as the thrift file.
# - The generated command will be added to the default build target.
function(buildThriftPy thriftFile)
  get_filename_component(fileName "${thriftFile}" NAME_WE)
  get_filename_component(filePath "${thriftFile}" PATH)
  set(OUTPUT_PATH ${CMAKE_BINARY_DIR}/${filePath}/gen-py/${fileName})
  set(OUTPUT_FILES ${OUTPUT_PATH}/${fileName}.py ${OUTPUT_PATH}/ttypes.py)
  add_custom_command(
    OUTPUT ${OUTPUT_FILES}
    COMMAND ${thrift_COMPILER} --gen py -o ${CMAKE_BINARY_DIR}/${filePath} ${thriftFile}
    DEPENDS ${thriftFile}
    COMMENT Generating python thrift module for ${thriftFile}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  # Create output directory
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${filePath})
  # Create a custom target
  add_custom_target(gen_thrift_py_${fileName} ALL DEPENDS ${OUTPUT_FILES})
endfunction()

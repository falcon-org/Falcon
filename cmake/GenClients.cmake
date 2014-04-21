# Helper functions for generating the clients


# Create target python_client for generating the python client.
# This will build the required thrift files and move everything together in
# ${CMAKE_BINARY_DIR}/clients/python/
function(genPythonClient)
  # Directory of the python client
  set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/clients/python")

  # Generate the command that builds the python thrift module.
  buildThriftPy(thrift/FalconService.thrift)

  set(FILES_TO_MOVE
    ${CMAKE_BINARY_DIR}/thrift/gen-py/FalconService/FalconService.py
    ${CMAKE_BINARY_DIR}/thrift/gen-py/FalconService/ttypes.py)

  set(FILES_MOVED "")

  foreach(FILE ${FILES_TO_MOVE})
    get_filename_component(fileName "${FILE}" NAME)
    get_filename_component(filePath "${FILE}" PATH)

    set(OUTPUT "${OUTPUT_DIR}/${fileName}")
    list(APPEND FILES_MOVED ${OUTPUT})

    add_custom_command(
      OUTPUT ${OUTPUT}
      COMMAND ${CMAKE_COMMAND} -E copy ${FILE} ${OUTPUT}
      DEPENDS ${FILE}
      COMMENT Copying ${FILE}
      )
  endforeach()

  # Create a custom target that
  add_custom_target(python_client ALL DEPENDS ${FILES_MOVED})
endfunction()

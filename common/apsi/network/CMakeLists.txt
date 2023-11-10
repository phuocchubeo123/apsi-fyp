# Source files in this directory
set(APSI_SOURCE_FILES ${APSI_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/sender_operation.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sender_operation_response.cpp
    ${CMAKE_CURRENT_LIST_DIR}/result_package.cpp
)

if(APSI_USE_ZMQ)
    add_subdirectory(zmq)
endif()

set(APSI_SOURCE_FILES ${APSI_SOURCE_FILES} PARENT_SCOPE)
# Source files in this directory
set(APSI_SOURCE_FILES ${APSI_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/sender_db.cpp
    ${CMAKE_CURRENT_LIST_DIR}/bin_bundle.cpp
)

add_subdirectory(util)

if(APSI_USE_ZMQ)
    add_subdirectory(zmq)
endif()
set(APSI_SOURCE_FILES ${APSI_SOURCE_FILES} PARENT_SCOPE)
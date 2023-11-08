set(APSI_SOURCE_FILES ${APSI_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/item.cpp
    ${CMAKE_CURRENT_LIST_DIR}/log.cpp
    ${CMAKE_CURRENT_LIST_DIR}/version.cpp
    ${CMAKE_CURRENT_LIST_DIR}/thread_pool_mgr.cpp
    ${CMAKE_CURRENT_LIST_DIR}/psi_params.cpp
)

add_subdirectory(network)
add_subdirectory(util)
add_subdirectory(oprf)
add_subdirectory(fourq)

set(APSI_SOURCE_FILES ${APSI_SOURCE_FILES} PARENT_SCOPE)
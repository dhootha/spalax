
set (SOURCES ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/quest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wmm_util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/state_estimate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/kalman.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/kalman_constants.cpp
        PARENT_SCOPE)

set (HEADERS ${HEADERS}
    PARENT_SCOPE)

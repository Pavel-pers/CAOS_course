if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(ARCHITECTURE x86_64)
elseif(
    CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR
    CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64"
)
    set(ARCHITECTURE aarch64)
else()
    message(FATAL_ERROR "Unknown target architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "Detected target architecture ${ARCHITECTURE}")

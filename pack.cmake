set(CPACK_PACKAGE_NAME "airmap-platform-sdk")
set(CPACK_PACKAGE_VENDOR "AirMap")
set(CPACK_VERBATIM_VARIABLES true)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "AirMap, Inc.")

include(DetermineTargetArchitecture)
determine_target_architecture(TARGET_ARCH)
message(STATUS "project name architecture: ${TARGET_ARCH}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${TARGET_ARCH}")

include(CPack)

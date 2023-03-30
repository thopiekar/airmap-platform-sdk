cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(GetGitRevisionDescription)

if(NOT DEFINED VERSION)
  get_git_head_revision(GIT_REFSPEC AIRMAPD_VERSION_GIT_REVISION)
  git_describe(VERSION --tags --abbrev=0)
  message(STATUS "Git -- refspec: ${GIT_REFSPEC} -- sha1: ${AIRMAPD_VERSION_GIT_REVISION}")
  message(STATUS "Building in git revision ${AIRMAPD_VERSION_GIT_REVISION}")
endif()
message(STATUS "Version: ${VERSION}")

string(TIMESTAMP AIRMAPD_VERSION_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%S.000000Z" UTC)
message(STATUS "Building in build timestamp ${AIRMAPD_VERSION_BUILD_TIMESTAMP}")

project(airmap
  VERSION "${VERSION}"
  LANGUAGES C CXX)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

option(AIRMAP_ENABLE_NETWORK_TESTS "enable tests requiring network access"       ON)
option(AIRMAP_ENABLE_GRPC          "Enable libraries/executables requiring gRPC" OFF)
option(AIRMAP_ENABLE_QT            "Enable libraries/executables requiring Qt5"  OFF)

if (AIRMAP_ENABLE_GRPC)
  add_definitions(-DAIRMAP_ENABLE_GRPC)
endif ()

# Detecting the platform at build time and exposing
# information on the platform to the platform
set(AIRMAP_PLATFORM "null")

if (UNIX)
  # We are treating OS X as a proper Linux-like operating system and store
  # configuration files according to the XDG directory standard.
  set(AIRMAP_PLATFORM "linux")
endif()

message(STATUS "Enabling platform ${AIRMAP_PLATFORM}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(external.cmake)
include(airmapd.cmake)
include(pack.cmake)

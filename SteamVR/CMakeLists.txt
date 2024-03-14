cmake_minimum_required(VERSION 3.7.1)

project(steamhmd)

# For your project, this might look something like:
# set(OPENVR_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/openvr)

# But we have the binaries we need in this repo, so link those:
get_filename_component(OPENVR_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}../../.. ABSOLUTE)

# If not set, determine the platform architecture
if(NOT PLATFORM)
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PLATFORM 64)
  else()
    set(PLATFORM 32)
  endif()
endif()
message(STATUS "Compilation set for ${PLATFORM}bits architectures.")

# OpenVR compatibility checking
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions(-DLINUX -DPOSIX)
  set(ARCH_TARGET linux64)

  if(${PLATFORM} MATCHES 32)
    message(WARNING "OpenVR x86 binaries not provided on GNU/Linux.")
  endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_MACOSX_RPATH 0)
  add_definitions(-DOSX -DPOSIX)
  set(ARCH_TARGET osx32)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions(-D_WIN32)
  set(ARCH_TARGET win${PLATFORM})

  # Binaries path for thirdparties are not generics so we try to guess their suffixes.
  set(WINDOWS_PATH_SUFFIXES win${PLATFORM} Win${PLATFORM} x${PLATFORM})
endif()

find_library(OPENVR_LIBRARIES
  NAMES
    openvr_api
  PATHS
    ${OPENVR_LIB_DIR}/bin
    ${OPENVR_LIB_DIR}/lib
  PATH_SUFFIXES
    osx${PLATFORM}
    linux${PLATFORM}
    win${PLATFORM}
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)

set(OPENVR_INCLUDE_DIR ${OPENVR_LIB_DIR}/headers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output/drivers")
add_library(steamhmd SHARED
        src/hmd_driver_factory.cpp
        src/device_provider.h
        src/device_provider.cpp
        src/hmd_device_driver.h
        src/hmd_device_driver.cpp
        )

# This is so we can build directly to "<binary_dir>/<target_name>/<platform>/<arch>/<driver_name>.<dll/so>"
set_target_properties(steamhmd PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}/bin/${ARCH_TARGET}>)

target_link_libraries(steamhmd PRIVATE ${OPENVR_LIBRARIES} util_driverlog util_vrmath)
target_include_directories(steamhmd PRIVATE ${OPENVR_INCLUDE_DIR})

# Copy driver assets to output folder
add_custom_command(
        TARGET steamhmd
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/SteamHMD
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SteamHMD)

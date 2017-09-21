cmake_minimum_required (VERSION 3.5.1)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)

#SET PROJECT NAME
SET(PROJECT_NAME_ORIG "dccomms_utils" )

#SET DEPENDENCIES DIRECTORY
SET(MODULES_DIR "modules" )

message("Building ${PROJECT_NAME_ORIG}...")

#BEGIN RESOLVE GIT REVISION
#https://bravenewmethod.com/2017/07/14/git-revision-as-compiler-definition-in-build-with-cmake/
message(STATUS "Resolving GIT Version...")
set(_build_version "unknown")
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE _build_version
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message( STATUS "GIT hash: ${_build_version}")
else()
  message(STATUS "GIT not found")
endif()
string(TIMESTAMP _time_stamp)
#END RESOLVE GIT REVISION

#http://stackoverflow.com/questions/7787823/cmake-how-to-get-the-name-of-all-subdirectories-of-a-directory
function(GETPROJECTS result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*/CMakeLists.txt)
  SET(dirlist "")
  FOREACH(file ${children})
        get_filename_component(subdir ${file} DIRECTORY)
        LIST(APPEND dirlist ${subdir})
  ENDFOREACH()
  SET(${result} ${dirlist} PARENT_SCOPE)
endfunction()

#DECLARE PROJECT
SET(PROJECT_NAME_GIT "${PROJECT_NAME_ORIG}_${_build_version}")
string(TOUPPER ${PROJECT_NAME_ORIG} TMP0)
SET(${TMP0}_GIT "${PROJECT_NAME_GIT}" PARENT_SCOPE)

if (TARGET ${PROJECT_NAME_GIT})
    message(WARNING "${PROJECT_NAME_GIT} has already been built!")
    return()
endif()

project (${PROJECT_NAME_GIT} CXX)
message(STATUS "PROJECT NAME: ${PROJECT_NAME}")

##################################
####### BUILD DEPENDENCIES #######
##################################

SET(${PROJECT_NAME}_DEPS_DIR ${MODULES_DIR})
SET(${PROJECT_NAME}_TMP0 ${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECT_NAME}_DEPS_DIR})
GETPROJECTS(${PROJECT_NAME}_DEPS ${${PROJECT_NAME}_TMP0})
message(STATUS "Dependencies found in ${${PROJECT_NAME}_TMP0}: ${${PROJECT_NAME}_DEPS}")
FOREACH(subdir ${${PROJECT_NAME}_DEPS})
add_subdirectory(${${PROJECT_NAME}_DEPS_DIR}/${subdir})
ENDFOREACH()

##################################
######### BUILD PROJECT ##########
##################################


## Add support for C++11, supported in ROS Kinetic and newer
add_definitions(-std=c++11)

## Specify additional locations of header files
## Your package locations should be listed before other locations
# include_directories(include)


######
SET(SRCPATH src/)
SET(${PROJECT_NAME}_INCLUDE_DIRS "${SRCPATH}lib/include")
SET(${PROJECT_NAME}_CPP_FILES
                        ${SRCPATH}lib/USBLStream.cpp
                        ${SRCPATH}lib/GironaStream.cpp
                        ${SRCPATH}lib/EvologicsBridge.cpp
                        ${SRCPATH}lib/EvologicsStream.cpp
                        ${SRCPATH}lib/WFSStream.cpp
                        ${SRCPATH}lib/S100Stream.cpp
	)
SET(${PROJECT_NAME}_CXX_COMPILE_FLAGS "-fPIC -std=c++0x")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${PROJECT_NAME}_CXX_COMPILE_FLAGS}")

include_directories(${catkin_INCLUDE_DIRS})

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_CPP_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${DCCOMMS_GIT} ${catkin_LIBRARIES})

add_executable(dccomms_girona_bridge ${SRCPATH}main/dccomms_girona_bridge/dccomms_girona_bridge.cpp)
target_link_libraries(dccomms_girona_bridge ${DCCOMMS_GIT} ${PROJECT_NAME})

add_executable(dccomms_usbl_bridge ${SRCPATH}main/dccomms_usbl_bridge/dccomms_usbl_bridge.cpp)
target_link_libraries(dccomms_usbl_bridge ${DCCOMMS_GIT} ${PROJECT_NAME})

add_executable(dccomms_s100_bridge ${SRCPATH}main/dccomms_s100_bridge/dccomms_s100_bridge.cpp)
target_link_libraries(dccomms_s100_bridge ${DCCOMMS_GIT} ${PROJECT_NAME})


install(TARGETS ${PROJECT_NAME} dccomms_girona_bridge dccomms_usbl_bridge
        LIBRARY DESTINATION "lib"
        RUNTIME DESTINATION "bin"
        )
install(DIRECTORY "src/lib/include/dccomms_utils" DESTINATION "include")

message("End ${PROJECT_NAME_ORIG}")

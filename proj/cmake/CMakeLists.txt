cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( TouchBroadcast )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cinder_master/" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )
get_filename_component( TUIO_PATH "${APP_PATH}/lib/TUIO11_CPP/" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

FILE(GLOB APP_SOURCES ${APP_PATH}/src/*.cpp)

if (UNIX)
	FILE(GLOB OSC_SOURCES ${TUIO_PATH}/oscpack/ip/*.cpp ${TUIO_PATH}/oscpack/ip/posix/*.cpp ${TUIO_PATH}/oscpack/osc/*.cpp)
endif ()

if (WIN32)
	FILE(GLOB OSC_SOURCES ${TUIO_PATH}/oscpack/ip/*.cpp ${TUIO_PATH}/oscpack/ip/win32/*.cpp ${TUIO_PATH}/oscpack/osc/*.cpp)
endif ()

FILE(GLOB TUIO_SOURCES ${TUIO_PATH}/TUIO/*.cpp ${OSC_SOURCES})

ci_make_app(
	SOURCES     ${APP_SOURCES} ${TUIO_SOURCES}
	INCLUDES		${TUIO_PATH} ${TUIO_PATH}/oscpack
	CINDER_PATH ${CINDER_PATH}
)

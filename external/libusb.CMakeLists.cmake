project(libusb VERSION 1.0.24 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
include(FindThreads)

include(FetchContent)
FetchContent_Declare(
  libusb
  GIT_REPOSITORY https://github.com/libusb/libusb.git
  GIT_TAG v1.0.24
)
if(NOT libusb_POPULATED)
  FetchContent_Populate(libusb)
endif()

set(PTHREADS_ENABLED FALSE)
if (CMAKE_USE_PTHREADS_INIT)
	set(PTHREADS_ENABLED TRUE)
endif()

if (WITHOUT_PTHREADS)
	set(PTHREADS_ENABLED FALSE)
endif()
set(THREADS_POSIX ${PTHREADS_ENABLED} CACHE INTERNAL "use pthreads" FORCE)

if (CMAKE_THREAD_LIBS_INIT)
	list(APPEND LIBUSB_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif()

add_library(usb-1.0
	${libusb_SOURCE_DIR}/libusb/core.c
	${libusb_SOURCE_DIR}/libusb/descriptor.c
	${libusb_SOURCE_DIR}/libusb/io.c
	${libusb_SOURCE_DIR}/libusb/sync.c
	${libusb_SOURCE_DIR}/libusb/libusb-1.0.rc
	${libusb_SOURCE_DIR}/libusb/libusb-1.0.def
	${libusb_SOURCE_DIR}/libusb/hotplug.c
)
add_library(libusb::usb-1.0 ALIAS usb-1.0)
if (DEFINED LIBUSB_LIBRARIES)
	message("Linking shared library against ${LIBUSB_LIBRARIES}")
	target_link_libraries(usb-1.0 PUBLIC
		${LIBUSB_LIBRARIES}
	)
endif()

if (WIN32 OR "${CMAKE_SYSTEM_NAME}" STREQUAL "CYGWIN")
	set(PLATFORM_WINDOWS 1 CACHE INTERNAL "controls config.h macro definition" FORCE)
	enable_language(RC)
	if ("${CMAKE_SYSTEM_NAME}" STREQUAL "CYGWIN")
		message(STATUS "Detected cygwin")
		set(PTHREADS_ENABLED TRUE)
		set(WITHOUT_POLL_H TRUE CACHE INTERNAL "Disable using poll.h even if it's available - use windows poll instead fo cygwin's" FORCE)
	endif()
	
	target_sources(usb-1.0 PUBLIC
  ${libusb_SOURCE_DIR}/libusb/os/events_windows.c
  ${libusb_SOURCE_DIR}/libusb/os/windows_winusb.c
  ${libusb_SOURCE_DIR}/libusb/os/windows_common.c
  ${libusb_SOURCE_DIR}/libusb/os/windows_usbdk.c
	)
	if (PTHREADS_ENABLED AND NOT WITHOUT_PTHREADS)
		target_sources(usb-1.0 PUBLIC ${libusb_SOURCE_DIR}/libusb/os/threads_posix.c)
	else()
		target_sources(usb-1.0 PUBLIC ${libusb_SOURCE_DIR}/libusb/os/threads_windows.c)
	endif()
elseif (UNIX)
	# Unix is for all *NIX systems including OSX
	set(PLATFORM_POSIX 1 CACHE INTERNAL "controls config.h macro definition" FORCE)

	target_sources(usb-1.0 PUBLIC
  ${libusb_SOURCE_DIR}/libusb/os/linux_usbfs.c
  ${libusb_SOURCE_DIR}/libusb/os/threads_posix.c
		${libusb_SOURCE_DIR}/libusb/os/events_posix.c
		${libusb_SOURCE_DIR}/libusb/os/linux_netlink.c
	)
	target_link_libraries(usb-1.0 PUBLIC rt)
else()
	message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}.  Currently only support Windows, OSX, & Linux.")
endif()

target_include_directories(usb-1.0 SYSTEM PUBLIC ${libusb_SOURCE_DIR}/libusb)
target_include_directories(usb-1.0 SYSTEM PUBLIC ${libusb_BINARY_DIR})
target_include_directories(usb-1.0 SYSTEM PRIVATE ${libusb_SOURCE_DIR}/libusb/os)
set_target_properties(usb-1.0 PROPERTIES
	PREFIX "lib"
	OUTPUT_NAME "usb-1.0"
	PUBLIC_HEADER libusb.h
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION}
)

include(libusb.config.cmake)

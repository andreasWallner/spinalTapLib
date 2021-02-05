project(libusb VERSION 1.0.24 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
include(FindThreads)

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
	libusb/libusb/core.c
	libusb/libusb/descriptor.c
	libusb/libusb/io.c
	libusb/libusb/sync.c
	libusb/libusb/libusb-1.0.rc
	libusb/libusb/libusb-1.0.def
	libusb/libusb/hotplug.c
)
add_library(libusb::usb-1.0 ALIAS usb-1.0)
if (DEFINED LIBUSB_LIBRARIES)
	message("Linking shared library against ${LIBUSB_LIBRARIES}")
	target_link_libraries(usb-1.0
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
		libusb/libusb/os/events_windows.c
		libusb/libusb/os/windows_winusb.c
		libusb/libusb/os/windows_common.c
		libusb/libusb/os/windows_usbdk.c
	)
	if (PTHREADS_ENABLED AND NOT WITHOUT_PTHREADS)
		target_sources(usb-1.0 PUBLIC libusb/libusb/os/threads_posix.c)
	else()
		target_sources(usb-1.0 PUBLIC libusb/libusb/os/threads_windows.c)
	endif()
elseif (UNIX)
	# Unix is for all *NIX systems including OSX
	set(PLATFORM_LINUX 1 CACHE INTERNAL "controls config.h macro definition" FORCE)

	target_sources(usb-1.0
		libusb/libusb/os/linux_usbfs.c
		libusb/libusb/os/threads_posix.c
		libusb/libusb/os/events_posix.c
	)
	target_link_libraries(usb-1.0 PUBLIC rt)
else()
	message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}.  Currently only support Windows, OSX, & Linux.")
endif()

target_include_directories(usb-1.0 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libusb/libusb)
target_include_directories(usb-1.0 PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(usb-1.0 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libusb/libusb/os)
set_target_properties(usb-1.0 PROPERTIES
	PREFIX "lib"
	OUTPUT_NAME "usb-1.0"
	PUBLIC_HEADER libusb.h
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION}
)

include(libusb.config.cmake)

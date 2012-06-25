cmake_minimum_required(VERSION 2.8)
project(dnl)


enable_testing()

IF(BUILD_TESTING)
	SET(BUILDNAME "${BUILDNAME}" CACHE STRING "spider")
	MARK_AS_ADVANCED(BUILDNAME)
ENDIF(BUILD_TESTING)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if (EXISTS "${CMAKE_BINARY_DIR}/settings.cmake")
	include(${CMAKE_BINARY_DIR}/settings.cmake)
endif()

find_package(Boost COMPONENTS regex filesystem system REQUIRED)
find_package(Event2 REQUIRED)
find_package(Berkeley REQUIRED)

if (NOT WIN32)
	add_definitions(-std=c++0x)
endif ()

include_directories(utils ${Berkeley_INCLUDE_PATH} ${Boost_INCLUDE_DIRS} ${Event2_INCLUDE_PATH})
link_directories(${Boost_LIBRARY_DIRS})

set(source 
	dnl.cpp dnl.h 
	links_extractor.h links_extractor.cpp
	links_db.h links_db.cpp
	utils/config.cpp utils/config.h
	utils/timer.cpp utils/timer.h
	utils/logger.c utils/logger.h
	)

add_library(dnl ${source})
add_executable(spider main.cpp)
target_link_libraries(spider dnl ${Berkeley_LIBRARY} ${Event2_LIBRARY} ${Boost_LIBRARIES})
add_executable(test_links_extractor test_links_extractor.cpp)
target_link_libraries(test_links_extractor dnl ${Berkeley_LIBRARY} ${Event2_LIBRARY} ${Boost_LIBRARIES})

add_executable(test_links_db test_links_db.cpp)
target_link_libraries(test_links_db dnl ${Berkeley_LIBRARY} ${Event2_LIBRARY} ${Boost_LIBRARIES})


add_test(links_db_queue ${CMAKE_BINARY_DIRECTORY}/test_links_db queue)
add_test(links_db_flags ${CMAKE_BINARY_DIRECTORY}/test_links_db flags)
add_test(links_db_converters ${CMAKE_BINARY_DIRECTORY}/test_links_db converters)
add_test(links_db_restore ${CMAKE_BINARY_DIRECTORY}/test_links_db restore)



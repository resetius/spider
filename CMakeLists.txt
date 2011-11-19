cmake_minimum_required(VERSION 2.8)
project(dnl)

find_package(Boost COMPONENTS regex filesystem REQUIRED)

set(source 
	dnl.cpp dnl.h 
	links_extractor.h links_extractor.cpp
	db/db.cpp db/db.h
	utils/config.cpp utils/config.h
	utils/timer.cpp utils/timer.h
	utils/logger.c utils/logger.h
	)

add_library(dnl ${source})
include_directories(utils /usr/include/db4 ${Boost_INCLUDE_DIRS})
add_definitions(-std=c++0x)
add_executable(spider main.cpp)
target_link_libraries(spider dnl db_cxx event ${Boost_LIBRARIES})
add_executable(test_links_extractor test_links_extractor.cpp)
target_link_libraries(test_links_extractor dnl db_cxx event ${Boost_LIBRARIES})


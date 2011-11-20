
if (Event2_FOUND)
	return ()
endif ()

if (NOT Event2_INCLUDE_PATH)
find_path (Event2_INCLUDE_PATH event2/http.h PATHS ENV INCLUDE)
endif ()



if (Event2_LIBRARY_DIR)
find_library(Event2_LIBRARY1 NAMES event libevent PATHS ${Event2_LIBRARY_DIR})
else ()
find_library(Event2_LIBRARY1 PATH_SUFFIXES event2 NAMES event event-2.0)
endif ()

if (NOT WIN32)
find_library(Event2_LIBRARY2 PATH_SUFFIXES event2 NAMES event_pthreads event_pthreads-2.0)
else ()
set(Event2_LIBRARY2 "fake")
endif ()

# for clock_gettime
if (NOT WIN32)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .so;.a)
endif ()
find_library(Event2_LIBRARY3 NAMES rt)

if (Event2_INCLUDE_PATH AND Event2_LIBRARY1 AND Event2_LIBRARY2)
  set (Event2_FOUND TRUE)
  if (NOT WIN32)
    set (Event2_LIBRARY ${Event2_LIBRARY3};${Event2_LIBRARY1};${Event2_LIBRARY2})
  else ()
    set (Event2_LIBRARY ${Event2_LIBRARY1})
  endif ()
  message (STATUS "Event2 include path: ${Event2_INCLUDE_PATH}")
  message (STATUS "Event2 library: ${Event2_LIBRARY}")
else ()
  if (Event2_FIND_REQUIRED)
    message(FATAL_ERROR "Event2 not found")
  endif ()
endif ()


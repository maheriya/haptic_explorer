cmake_minimum_required(VERSION 2.8)
project( hapticsExplorer )

INCLUDE(FindPkgConfig)

#Set CXX standard
ADD_DEFINITIONS(
      -std=c++11
)

include_directories(include ${PROJECT_BINARY_DIR})

## Find Bluz
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
find_package(Bluez REQUIRED)
include_directories (${BLUEZ_INCLUDE_DIRS})

# Find Glib
pkg_check_modules(GLIB_PKG glib-2.0 gobject-2.0 gio-2.0 REQUIRED)
if(GLIB_PKG_FOUND)
    include_directories(${GLIB_PKG_INCLUDE_DIRS})
endif()
# Find GTK
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(GDKMM REQUIRED gdkmm-3.0)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)
include_directories (${GTKMM_INCLUDE_DIRS} ${GDKMM_INCLUDE_DIRS} ${GLIBMM_INCLUDE_DIRS})
link_directories (${GTKMM_LIBRARY_DIRS} ${GDKMM_LIBRARY_DIRS} ${GLIBMM_LIBRARY_DIRS})
add_definitions(${GTKMM_CFLAGS_OTHER} ${GDKMM_CFLAGS_OTHER} ${GLIBMM_CFLAGS_OTHER})

find_package(LibBlepp REQUIRED)
if (LibBlepp_FOUND)
  message("-- Found LibBlepp")
endif()
#message("LIBBLEPP_LIBRARIES ${LIBBLEPP_LIBRARIES}")
#message("LIBBLEPP_INCLUDE_DIRS ${LIBBLEPP_INCLUDE_DIRS}")
#message("LIBBLEPP_LIBRARY_DIRS ${LIBBLEPP_LIBRARY_DIRS}")
include_directories (${LIBBLEPP_INCLUDE_DIRS})
link_directories (${LIBBLEPP_LIBRARY_DIRS})

# Add the source files
file(GLOB SOURCES_CPP "explorer/*.cpp")

# Add Executable
add_executable(explorer ${SOURCES_C} ${SOURCES_CPP})
target_link_libraries( explorer ${GLIB_PKG_LIBRARIES} ${GLIBMM_LIBRARIES} ${GTKMM_LIBRARIES} ${GDKMM_LIBRARIES} ${LIBBLEPP_LIBRARIES} ${BLUEZ_LIBRARIES} -lreadline -pthread )

# EOF

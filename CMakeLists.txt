cmake_minimum_required(VERSION 2.8.3)
project(gatt_server_ros)

find_package(catkin REQUIRED
  app_api roscpp
)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GOBJECT REQUIRED gobject-2.0)
include_directories(${GOBJECT_INCLUDE_DIRS})

pkg_check_modules(GIO REQUIRED gio-2.0)
include_directories(${GIO_INCLUDE_DIRS})

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 17)

catkin_package(
  INCLUDE_DIRS
    include
  LIBRARIES
    ${PROJECT_NAME}
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

file(GLOB_RECURSE LIBRARY_SOURCES "src/**.cpp" )
add_library(${PROJECT_NAME} SHARED
  ${LIBRARY_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
  ${catkin_LIBRARIES}
  ${GLIB_LDFLAGS}
  ${GOBJECT_LDFLAGS}
  ${GIO_LDFLAGS}
)

add_executable(standalone
  src/standalone.cxx
)

target_link_libraries(standalone ${PROJECT_NAME}
  ${catkin_LIBRARIES}
)

install(
  TARGETS ${PROJECT_NAME} standalone
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY include/
   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
   FILES_MATCHING PATTERN "*.h"
)

# - Find CURLPP library
# Once done this will define
#
#  CURLPP_FOUND - This defines if we found CURLPP
#  CURLPP_INCLUDE_DIR - CURLPP include directory
#  CURLPP_LIBS - CURLPP libraries
#  CURLPP_DEFINITIONS - Compiler switches required for CURLPP


# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
#FIND_PACKAGE(PkgConfig)
#PKG_SEARCH_MODULE(PC_LIBCURLPP libCURLPP)

SET(MYSQL_DEFINITIONS ${PC_MYSQL_CFLAGS_OTHER})

FIND_PATH(
    MYSQL_INCLUDE_DIR
    NAMES "mysql.h"
    PATH_SUFFIXES "mysql"
)

FIND_LIBRARY(MYSQL_LIBS NAMES mysqlclient
   HINTS
   ${PC_MYSQL_LIBDIR}
   ${PC_MYSQL_LIBRARY_DIRS}
)

SET(ENABLE_XORM ON)
SET(ENABLE_MYSQL ON)

if(ENABLE_XORM)
  add_definitions(-D_ENABLE_XORM_)
  if(ENABLE_MYSQL)
     add_definitions(-DXORM_ENABLE_MYSQL)
  endif()
  if(ENABLE_SQLITE)
    add_definitions(-DXORM_ENABLE_SQLITE)
  if(ENABLE_SQLITE_CODEC)
   add_definitions(-DSQLITE_HAS_CODEC)
  endif()
  endif()
endif()


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(mysql DEFAULT_MSG MYSQL_LIBS MYSQL_INCLUDE_DIR)

# - Find sqlpp11 library
# Once done this will define
#
#  SQLPP11_FOUND - This defines if we found sqlpp11
#  SQLPP11_INCLUDE_DIR - sqlpp11 include directory
#  SQLPP11_LIBS - sqlpp11 libraries
#  SQLPP11_DEFINITIONS - Compiler switches required for sqlpp11


SET(SQLPP11_DEFINITIONS ${PC_SQLPP11_CFLAGS_OTHER})

FIND_PATH(
    SQLPP11_INCLUDE_DIR
    NAMES "sqlpp11.h"
    PATH_SUFFIXES "sqlpp11"
)

FIND_LIBRARY(SQLPP11_LIBS NAMES sqlpp-mysql
   HINTS
   ${PC_SQLPP11_LIBDIR}
   ${PC_SQLPP11_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(sqlpp-mysql DEFAULT_MSG SQLPP11_LIBS SQLPP11_INCLUDE_DIR)

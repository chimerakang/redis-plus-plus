# - Find ultimate alpr library
# Once done this will define
#
#  ALPR_FOUND - This defines if we found hiredis
#  ALPR_INCLUDE_DIR - hiredis include directory
#  ALPR_LIBS - hiredis libraries
#  ALPR_DEFINITIONS - Compiler switches required for hiredis


# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
#FIND_PACKAGE(PkgConfig)
#PKG_SEARCH_MODULE(PC_LIBHIREDIS libhiredis)

SET(ALPR_DEFINITIONS ${PC_ALPR_CFLAGS_OTHER})

FIND_PATH(
    ALPR_INCLUDE_DIR
    NAMES "ultimateALPR-SDK-API-PUBLIC.h"
    PATH_SUFFIXES ""
)

FIND_LIBRARY(ALPR_LIBS NAMES ultimate_alpr-sdk
   HINTS
   ${PC_ALPR_LIBDIR}
   ${PC_ALPR_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ALPR DEFAULT_MSG ALPR_LIBS ALPR_INCLUDE_DIR)

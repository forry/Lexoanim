# This is part of the Findosg* suite used to find OpenSceneGraph components.
# Each component is separate and you must opt in to each module. You must
# also opt into OpenGL and OpenThreads (and Producer if needed) as these
# modules won't do it for you. This is to allow you control over your own
# system piece by piece in case you need to opt out of certain components
# or change the Find behavior for a particular module (perhaps because the
# default FindOpenGL.cmake module doesn't work with your system as an
# example).
# If you want to use a more convenient module that includes everything,
# use the FindOpenSceneGraph.cmake instead of the Findosg*.cmake modules.
#
# Locate osgViewer
# This module defines
# OSGVIEWER_LIBRARY
# OSGVIEWER_FOUND, if false, do not try to link to osgViewer
# OSGVIEWER_INCLUDE_DIR, where to find the headers
#
# $OSGDIR is an environment variable that would
# correspond to the ./configure --prefix=$OSGDIR
# used in building osg.
#
# Created by Eric Wing.

# Header files are presumed to be included like
# #include <osg/PositionAttitudeTransform>
# #include <osgViewer/Viewer>

# Search paths
IF (!WIN32)
SET(SEARCH_PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt)
ELSE (!WIN32)
SET(SEARCH_PATHS)
ENDIF (!WIN32)

# Try the user's environment request before anything else.
FIND_PATH(OSGVIEWER_INCLUDE_DIR osgViewer/Viewer
  HINTS
  $ENV{OSGVIEWER_DIR}
  $ENV{OSG_DIR}
  $ENV{OSGDIR}
  PATH_SUFFIXES include
  PATHS
    ${SEARCH_PATHS}
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OpenThreads_ROOT]
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]
)

FIND_LIBRARY(OSGVIEWER_LIBRARY_RELEASE
  NAMES osgViewer
  HINTS
  $ENV{OSGVIEWER_DIR}
  $ENV{OSG_DIR}
  $ENV{OSGDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
    ${SEARCH_PATHS}
)

FIND_LIBRARY(OSGVIEWER_LIBRARY_DEBUG
  NAMES osgViewerd
  HINTS
  $ENV{OSGVIEWER_DIR}
  $ENV{OSG_DIR}
  $ENV{OSGDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
    ${SEARCH_PATHS}
)

# set release to debug (if only debug found)
IF(NOT OSGVIEWER_LIBRARY_RELEASE AND OSGVIEWER_LIBRARY_DEBUG)
  SET(OSGVIEWER_LIBRARY_RELEASE ${OSGVIEWER_LIBRARY_DEBUG})
ENDIF(NOT OSGVIEWER_LIBRARY_RELEASE AND OSGVIEWER_LIBRARY_DEBUG)

# set debug to release (if only release found)
IF(NOT OSGVIEWER_LIBRARY_DEBUG AND OSGVIEWER_LIBRARY_RELEASE)
  SET(OSGVIEWER_LIBRARY_DEBUG ${OSGVIEWER_LIBRARY_RELEASE})
ENDIF(NOT OSGVIEWER_LIBRARY_DEBUG AND OSGVIEWER_LIBRARY_RELEASE)

# OSGVIEWER_LIBRARY
SET(OSGVIEWER_LIBRARY optimized ${OSGVIEWER_LIBRARY_RELEASE} debug ${OSGVIEWER_LIBRARY_DEBUG})

# OSGVIEWER_FOUND
SET(OSGVIEWER_FOUND "NO")
IF(OSGVIEWER_LIBRARY AND OSGVIEWER_INCLUDE_DIR)
  SET(OSGVIEWER_FOUND "YES")
ENDIF(OSGVIEWER_LIBRARY AND OSGVIEWER_INCLUDE_DIR)


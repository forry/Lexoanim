cmake_minimum_required( VERSION 2.6.0 )

project( "Lexolights" )


# lexolights version
set( LEXOLIGHTS_VERSION_MAJOR 2 )
set( LEXOLIGHTS_VERSION_MINOR 1a )

# OSG SOVERSIONS (used for Windows packaging)
set( OPENSCENEGRAPH_VERSION 3.1.0 )
set( OPENSCENEGRAPH_SOVERSION 90 )
set( OPENTHREADS_SOVERSION 12 )

# Coin version, FIXME: need to be detected
set( COIN_VERSION 3 )


# look for CMake modules in the project first
set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}" )

# find packages
find_package( osg REQUIRED )
find_package( osgDB REQUIRED )
find_package( osgUtil REQUIRED )
find_package( osgGA REQUIRED )
find_package( osgViewer REQUIRED )
find_package( osgQt REQUIRED )
find_package( osgShadow REQUIRED )
find_package( Qt4 4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED )
find_package( OpenThreads REQUIRED ) # this is required at least
         # in release code using osg::ref_ptr for its dependency
         # on OpenThreads::Atomic
find_package( ZLIB REQUIRED )

# init QT
include( "${QT_USE_FILE}" )

# customize various things
if( WIN32 )
   set( INSTALL_DESTINATION_DIR ./ )
   add_definitions( -D_CRT_SECURE_NO_DEPRECATE )
else( WIN32 )
   set( INSTALL_DESTINATION_DIR bin )
#   set( CMAKE_CXX_FLAGS "-O2" )
#   set( CMAKE_CXX_FLAGS "-g -ggdb -gstabs" )
endif( WIN32 )


#
# compile directory src
#
add_subdirectory( src )


#
# required libraries
#
if( WIN32 )

   # OpenSceneGraph libraries
   # note: OPENSCENEGRAPH_SOVERSION and OPENTHREADS_SOVERSION have
   #       to be set properly
   find_path( OSG_LIBRARY_DIR osg${OPENSCENEGRAPH_SOVERSION}-osg.dll
                              osg${OPENSCENEGRAPH_SOVERSION}-osgd.dll
      HINTS
      $ENV{OSG_DIR}
      $ENV{OSGDIR}
      PATH_SUFFIXES bin
      PATHS
         ${SEARCH_PATHS}
   )

   # Coin libraries
   find_path( COIN_LIBRARY_DIR coin4.dll coin4d.dll
                               coin3.dll coin3d.dll
                               coin2.dll coin2d.dll
      HINTS
      $ENV{COINDIR}
      $ENV{PATH}
   )

   # OSG 3rd party dir and libpng13.dll
   find_path( OSG_3RD_PARTY_DIR README_3rdParty.txt
                                bin/libpng13.dll
                                bin/zlib1.dll
      HINTS
      $ENV{PATH}
   )
   find_file( OSG_PRECOMPILED_LIBPNG libpng13.dll
      HINTS
      ${OSG_3RD_PARTY_DIR}
      ${PATH}
      PATH_SUFFIXES bin
   )
   if( NOT OSG_PRECOMPILED_LIBPNG )
      if( NOT OSG_3RD_PARTY_DIR )
         message( SEND_ERROR "Can not find OSG_3RD_PARTY_DIR or OSG_PRECOMPILED_LIBPNG." )
      else( NOT OSG_3RD_PARTY_DIR )
         message( SEND_ERROR "Can not find OSG_PRECOMPILED_LIBPNG." )
      endif( NOT OSG_3RD_PARTY_DIR )
   endif( NOT OSG_PRECOMPILED_LIBPNG )

   # OSG core libraries + accompanying libs
   # note: osgViewer requires osgText
   install( FILES
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osg.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgDB.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgUtil.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgGA.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgViewer.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgQt.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgText.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgShadow.dll"
      "${OSG_LIBRARY_DIR}/ot${OPENTHREADS_SOVERSION}-OpenThreads.dll"
      "${COIN_LIBRARY_DIR}/coin${COIN_VERSION}.dll"
      "${COIN_LIBRARY_DIR}/zlib1.dll"
      DESTINATION "${INSTALL_DESTINATION_DIR}"
      CONFIGURATIONS Release MinSizeRel RelWithDebInfo
   )
   install( FILES
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgd.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgDBd.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgUtild.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgGAd.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgViewerd.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgQtd.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgTextd.dll"
      "${OSG_LIBRARY_DIR}/osg${OPENSCENEGRAPH_SOVERSION}-osgShadowd.dll"
      "${OSG_LIBRARY_DIR}/ot${OPENTHREADS_SOVERSION}-OpenThreadsd.dll"
      "${COIN_LIBRARY_DIR}/coin${COIN_VERSION}d.dll"
      "${COIN_LIBRARY_DIR}/zlib1.dll"
      DESTINATION "${INSTALL_DESTINATION_DIR}"
      CONFIGURATIONS Debug
   )

   # OSG plugins
   install( DIRECTORY "${OSG_LIBRARY_DIR}/osgPlugins-${OPENSCENEGRAPH_VERSION}"
            DESTINATION "${INSTALL_DESTINATION_DIR}"
            USE_SOURCE_PERMISSIONS
            CONFIGURATIONS Release MinSizeRel RelWithDebInfo
            FILES_MATCHING PATTERN "*.dll"
                           PATTERN "*d.dll" EXCLUDE
                           PATTERN "osgdb_p3d.dll" )  # exception for p3d plugin that terminates with d
   install( DIRECTORY "${OSG_LIBRARY_DIR}/osgPlugins-${OPENSCENEGRAPH_VERSION}"
            DESTINATION "${INSTALL_DESTINATION_DIR}"
            USE_SOURCE_PERMISSIONS
            CONFIGURATIONS Release MinSizeRel RelWithDebInfo
            FILES_MATCHING PATTERN "osgdb_p3d.dll" )  # exception for p3d plugin that terminates with d
   install( DIRECTORY "${OSG_LIBRARY_DIR}/osgPlugins-${OPENSCENEGRAPH_VERSION}"
            DESTINATION "${INSTALL_DESTINATION_DIR}"
            USE_SOURCE_PERMISSIONS
            CONFIGURATIONS Debug
            FILES_MATCHING PATTERN "*d.dll"
                           PATTERN "osgdb_p3d.dll" EXCLUDE )  # exception for p3d plugin that terminates with d
   install( FILES "${OSG_3RD_PARTY_DIR}/bin/libpng13.dll"
            DESTINATION "${INSTALL_DESTINATION_DIR}"
            CONFIGURATIONS Release MinSizeRel RelWithDebInfo )
   install( FILES "${OSG_3RD_PARTY_DIR}/bin/libpng13d.dll"
            DESTINATION "${INSTALL_DESTINATION_DIR}"
            CONFIGURATIONS Debug )

   # Qt libraries
   install( FILES
      "${QT_LIBRARY_DIR}/QtCore${QT_VERSION_MAJOR}.dll"
      "${QT_LIBRARY_DIR}/QtGui${QT_VERSION_MAJOR}.dll"
      "${QT_LIBRARY_DIR}/QtOpenGL${QT_VERSION_MAJOR}.dll"
      "${QT_LIBRARY_DIR}/QtXml${QT_VERSION_MAJOR}.dll"
      DESTINATION ${INSTALL_DESTINATION_DIR}
      CONFIGURATIONS Release MinSizeRel RelWithDebInfo
   )
   install( FILES
      "${QT_LIBRARY_DIR}/QtCored${QT_VERSION_MAJOR}.dll"
      "${QT_LIBRARY_DIR}/QtGuid${QT_VERSION_MAJOR}.dll"
      "${QT_LIBRARY_DIR}/QtOpenGLd${QT_VERSION_MAJOR}.dll"
      "${QT_LIBRARY_DIR}/QtXmld${QT_VERSION_MAJOR}.dll"
      DESTINATION ${INSTALL_DESTINATION_DIR}
      CONFIGURATIONS Debug
   )

   # MSVC libraries
   set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1)
   include (InstallRequiredSystemLibraries)
   install (PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION ./
            CONFIGURATIONS Release MinSizeRel RelWithDebInfo)
   set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "")
   set (CMAKE_INSTALL_DEBUG_LIBRARIES 1)
   include (InstallRequiredSystemLibraries)
   install (PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION ./
            CONFIGURATIONS Debug)

endif( WIN32 )

#
# CPack installation
#
set( CPACK_PACKAGE_NAME "Lexolights" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lexolights is photorealistic viewer of 3D scenes" )
#set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Readme.txt" )
#set( CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/Readme.txt" )
#set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt" )
set( CPACK_PACKAGE_VERSION_MAJOR "${LEXOLIGHTS_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${LEXOLIGHTS_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH  )
set( CPACK_PACKAGE_VERSION "${LEXOLIGHTS_VERSION_MAJOR}.${LEXOLIGHTS_VERSION_MINOR}" )
set( CPACK_PACKAGE_INSTALL_DIRECTORY "Lexolights ${LEXOLIGHTS_VERSION_MAJOR}.${LEXOLIGHTS_VERSION_MINOR}" )
set( CPACK_PACKAGE_VENDOR "Cadwork" )
include (CPack)

project( "Lexolights" )

include_directories( ${OSG_INCLUDE_DIR};${ZLIB_INCLUDE_DIRS};${PROJECT_SOURCE_DIR};
                     ${PROJECT_SOURCE_DIR}/utils )

add_definitions( -DLEXOLIGHTS_VERSION_MAJOR="${LEXOLIGHTS_VERSION_MAJOR}"
                 -DLEXOLIGHTS_VERSION_MINOR="${LEXOLIGHTS_VERSION_MINOR}" )

QT4_WRAP_CPP( HDR_MOCCED
              Lexolights.h
			  LexoanimQtApp.h
              LexolightsDocument.h
              gui/MainWindow.h
			  gui/LexoanimMainWindow.h
              gui/LogWindow.h
			  gui/LexoanimLogWindow.h
              threading/WorkerThread.h
              threading/CustomizedProcess.h
              )

# executable size investigation:
# no resources:    105 859 bytes
# with resources:  178 705 bytes
# difference:       72 846 bytes
# files on disk:    69 788 bytes
# overhead:          3 058 bytes - very ok
QT4_ADD_RESOURCES( RES_FILES "../data/lexolights.qrc" )

# embed executable icon on Win32 platform
# (for details, including other platforms, see http://doc.trolltech.com/latest/appicon.html)
if( WIN32 )
set( RES_FILES ${RES_FILES} ../data/lexolights.rc )
endif( WIN32 )

add_executable( lexolights WIN32
                main.cpp
                ${HDR_MOCCED}
                ${RES_FILES}
                Lexolights.h Lexolights.cpp
				LexoanimQtApp.h LexoanimQtApp.cpp
                LexolightsDocument.h LexolightsDocument.cpp
                CadworkViewer.h CadworkViewer.cpp
                Options.h Options.cpp
                gui/MainWindow.h gui/MainWindow.cpp
				gui/LexoanimMainWindow.h gui/LexoanimMainWindow.cpp
                gui/OSGWidget.h gui/OSGWidget.cpp
                gui/LogWindow.h gui/LogWindow.cpp
				gui/LexoanimLogWindow.h gui/LexoanimLogWindow.cpp
                gui/CentralContainer.h gui/CentralContainer.cpp
                gui/CadworkOrbitManipulator.h gui/CadworkOrbitManipulator.cpp
                gui/CadworkFirstPersonManipulator.h gui/CadworkFirstPersonManipulator.cpp
                lighting/PerPixelLighting.h
                lighting/PerPixelLighting.cpp
                lighting/ShadowVolume.h
                lighting/ShadowVolume.cpp
                lighting/PhotorealismData.h
                lighting/PhotorealismData.cpp
                threading/MainThreadRoutine.h threading/MainThreadRoutine.cpp
                threading/WorkerThread.h threading/WorkerThread.cpp
                threading/ExternalApplicationWorker.h threading/ExternalApplicationWorker.cpp
                threading/CustomizedProcess.h threading/CustomizedProcess.cpp
                utils/Log.h utils/Log.cpp
                utils/CadworkReaderWriter.h
                utils/CadworkReaderWriter.cpp
                utils/StateSetVisitor.h utils/StateSetVisitor.cpp
                utils/SetAnisotropicFilteringVisitor.h utils/SetAnisotropicFilteringVisitor.cpp
                utils/TextureUnitsUsageVisitor.h utils/TextureUnitsUsageVisitor.cpp
                utils/TextureUnitMoverVisitor.h utils/TextureUnitMoverVisitor.cpp
                utils/SysInfo.h utils/SysInfo.cpp
                utils/ViewLoadSave.h utils/ViewLoadSave.cpp
                utils/WinRegistry.h utils/WinRegistry.cpp
                utils/minizip/unzip.h utils/minizip/unzip.c
                utils/minizip/ioapi.h utils/minizip/ioapi.c
                utils/minizip/crypt.h
                )

if( WIN32 )
set( EXTRA_LIBS opengl32 )
else( WIN32 )
set( EXTRA_LIBS Xxf86vm )
endif( WIN32 )

target_link_libraries( lexolights
                       ${OSG_LIBRARY}
                       ${OSGDB_LIBRARY}
                       ${OSGUTIL_LIBRARY}
                       ${OSGGA_LIBRARY}
                       ${OSGVIEWER_LIBRARY}
                       ${OSGQT_LIBRARY}
                       ${OSGSHADOW_LIBRARY}
                       ${QT_LIBRARIES}
                       ${OPENTHREADS_LIBRARY}
                       ${ZLIB_LIBRARIES}
                       ${EXTRA_LIBS}
                       )

set_target_properties( lexolights PROPERTIES PREFIX "../" )
set_target_properties( lexolights PROPERTIES DEBUG_POSTFIX "d" )
set_target_properties( lexolights PROPERTIES RELWITHDEBINFO_POSTFIX "rd" )
set_target_properties( lexolights PROPERTIES MINSIZEREL_POSTFIX "rms" )

if( WIN32 )
set_target_properties( lexolights PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"")
endif( WIN32 )

install( TARGETS lexolights
         RUNTIME
         DESTINATION ${INSTALL_DESTINATION_DIR} )

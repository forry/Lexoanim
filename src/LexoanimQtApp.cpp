#include "LexoanimQtApp.h"

#include "utils/CadworkReaderWriter.h"
#include "gui/MainWindow.h"


LexoanimQtApp::LexoanimQtApp( int &argc, char* argv[], bool initialize)
   : Lexolights(argc,argv, false)
{
   setApplicationName( "Lexolights" );

   // call init
   if(initialize)
      init();
}

LexoanimQtApp::~LexoanimQtApp(){}

void LexoanimQtApp::init()
{
   // protect against multiple initializations
   if( _initialized )
      return;
   _initialized = true;
   // register Cadwork Inventor aliases (ivx, ivl)
   CadworkReaderWriter::createAliases();
}
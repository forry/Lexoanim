/**
 * @file
 * LexoanimLogWindow class header.
 *
 * @author Tomas Starka
 */

#include "LogWindow.h"
#include <dtABC/application.h>

#ifndef LEXOANIM_LOG_WINDOW
#define LEXOANIM_LOG_WINDOW

class LexoanimLogWindow : public LogWindow
{
   Q_OBJECT

public:

   LexoanimLogWindow( QWidget *parent = 0, Qt::WFlags flags = 0 );
   ~LexoanimLogWindow();

   inline virtual void setDeltaApp(dtABC::Application *deltaApp){ _deltaApp = deltaApp;}
   inline virtual dtABC::Application *getDeltaApp(){ return _deltaApp; }

protected slots:
   virtual void printSomethingCB();

protected:
   
   dtABC::Application *_deltaApp;

   virtual void printOpenGLVersion();
   virtual void printOpenGLExtensions();
   virtual void printOpenGLLimits();
   virtual void printGLSLLimits();
   virtual void printGraphicsDriverInfo();
   virtual void printVideoMemoryInfo();
   //virtual void printScreenInfo();
};

#endif
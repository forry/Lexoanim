
/**
 * @file
 * Lexolights class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef LEXOLIGHTS_H
#define LEXOLIGHTS_H

#include <QApplication>
#include "Options.h"

namespace osg {
   class Node;
}
class MainWindow;
class LexolightsDocument;
class CadworkViewer;


/**
 * Lexolights class representing the application instance.
 */
class Lexolights : public QApplication
{
   Q_OBJECT

public:

   // constructor and destructor
   Lexolights( int &argc, char* argv[], bool initialize = true );
   virtual ~Lexolights();

   // initialize Lexolights object
   virtual void init();

   // realize window
   static void realize();

   // start slave Lexolights instance with elevated privileges
   static int startElevated( const QString& params );

   // global functions
   static inline LexolightsDocument* activeDocument()  { return g_activeDocument; }
   static void setActiveDocument( LexolightsDocument *doc );
   static inline CadworkViewer* viewer()  { return g_viewer; }
   static inline MainWindow* mainWindow()  { return g_mainWindow; }

   // Options (command line,...)
   static inline Options* options()  { return g_options; }

   // registry file associations
   static bool checkFileAssociations();
   static void registerFileAssociations();
   static void unregisterFileAssociations();

protected:
   static bool _initialized;
   static osg::ref_ptr< LexolightsDocument > g_activeDocument;
   static Options *g_options;

private:
   static MainWindow *g_mainWindow;
   static osg::ref_ptr< CadworkViewer > g_viewer;
};


#endif /* LEXOLIGHTS_H */


/**
 * @file
 * Options class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QString>
#include "lighting/PerPixelLighting.h"


/**
 * Options class representing command-line and user defined options.
 */
class Options
{
public:

   enum ExitTime { DONT_EXIT = 0, AFTER_PARSING_CMDLINE, BEFORE_GUI_CREATION };

   // Constructor and destructor
   Options( int &argc, char* argv[] );
   virtual ~Options();
   bool reportRemainingOptionsAsUnrecognized();

   osg::ArgumentParser *argumentParser;

   QString startUpModelName;
   ExitTime exitTime;

   bool no_conversion;
   bool no_shadows;
   bool no_threads;
   bool renderInPovray;
   bool recreateFileAssociations;
   bool removeFileAssociations;
   bool exportScene;
   PerPixelLighting::ShadowTechnique shadowTechnique;
   bool continuousUpdate;

   /** slaveElevatedProcess is set to true by some cmd-line parameters that tells the application
       that it was attempted to be started with administrative privileges, usually to perform some
       actions like recreating file associations on Windows Vista and 7. If application does not
       get administrative privileges for some reasons, this flag avoids recursive attepmts to get
       the administrative privileges.*/
   bool elevatedProcess;

};


#endif

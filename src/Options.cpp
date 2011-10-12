
/**
 * @file
 * Options class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osg/ArgumentParser>
#include <iostream>
#include "Options.h"

using namespace std;
using namespace osg;


/**
 * Constructor.
 */
Options::Options( int &argc, char* argv[] )
   : exitTime( DONT_EXIT )
{
   // use an ArgumentParser object to manage the program arguments
   argumentParser = new ArgumentParser( &argc, argv );
   ApplicationUsage &au = *argumentParser->getApplicationUsage();

   // application usage
   au.setApplicationName( argumentParser->getApplicationName() );
   au.setDescription( argumentParser->getApplicationName() +
         " is 3D model viewer aimed at photorealistic user experience.");
   au.setCommandLineUsage( argumentParser->getApplicationName() + " [options] filename");

   au.addCommandLineOption( "--no-conversion", "Disables the scene conversion "
         "for the close-to-photorealistic rendering." );
   au.addCommandLineOption( "--no-shadows", "Disables shadows." );
   au.addCommandLineOption( "--install", "Setup file associations." );
   au.addCommandLineOption( "--uninstall", "Remove file associations." );
   au.addCommandLineOption( "--povray", "Render the model using POV-Ray." );
   au.addCommandLineOption( "--export-scene", "Saves the visualized scene to scene.osg for debugging purposes." );
   au.addCommandLineOption( "--sv",  "Use ShadowVolume technique for shadows." );
   au.addCommandLineOption( "--sm",  "Use ShadowMap technique for shadows." );
   au.addCommandLineOption( "--ssm", "Use StandardShadowMap technique for shadows (default)." );
   au.addCommandLineOption( "--msm", "Use MinimalShadowMap technique for shadows." );
   au.addCommandLineOption( "--lspsmvb", "Use LightSpacePerspectiveShadowMapVB (View Bounds) technique for shadows." );
   au.addCommandLineOption( "--lspsmcb", "Use LightSpacePerspectiveShadowMapCB (Cull Bounds) technique for shadows." );
   au.addCommandLineOption( "--lspsmdb", "Use LightSpacePerspectiveShadowMapDB (Draw Bounds) technique for shadows." );
   au.addCommandLineOption( "--continuous-update", "Make screen updated on maximum FPS." );

   // print help
   unsigned int helpType = argumentParser->readHelpType();
   if( helpType != ApplicationUsage::NO_HELP )
   {
      argumentParser->getApplicationUsage()->write( std::cerr, helpType );
      exitTime = AFTER_PARSING_CMDLINE;
      return;
   }

   // default options
   no_conversion = false;
   no_shadows = false;
   no_threads = false;
   renderInPovray = false;
   recreateFileAssociations = false;
   removeFileAssociations = false;
   exportScene = false;
   elevatedProcess = false;
   shadowTechnique = PerPixelLighting::SHADOW_VOLUMES;
   continuousUpdate = false;

   // read options
   while( argumentParser->read( "--no-conversion" ) )
      no_conversion = true;
   while( argumentParser->read( "--no-shadows" ) )
      no_shadows = true;
   while( argumentParser->read( "--povray" ) )
      renderInPovray = true;
   while( argumentParser->read( "--export-scene" ) )
      exportScene = true;
   while( argumentParser->read( "--install" ) ) {
      recreateFileAssociations = true;
      exitTime = BEFORE_GUI_CREATION;
   }
   while( argumentParser->read( "--install-elevated" ) ) {
      recreateFileAssociations = true;
      elevatedProcess = true;
      exitTime = BEFORE_GUI_CREATION;
   }
   while( argumentParser->read( "--uninstall" ) ) {
      recreateFileAssociations = false;
      removeFileAssociations = true;
      exitTime = BEFORE_GUI_CREATION;
   }
   while( argumentParser->read( "--uninstall-elevated" ) ) {
      recreateFileAssociations = false;
      removeFileAssociations = true;
      elevatedProcess = true;
      exitTime = BEFORE_GUI_CREATION;
   }
   while( argumentParser->read( "--sv" ) )
      shadowTechnique = PerPixelLighting::SHADOW_VOLUMES;
   while( argumentParser->read( "--sm" ) )
      shadowTechnique = PerPixelLighting::SHADOW_MAPS;
   while( argumentParser->read( "--ssm" ) )
      shadowTechnique = PerPixelLighting::STANDARD_SHADOW_MAPS;
   while( argumentParser->read( "--msm" ) )
      shadowTechnique = PerPixelLighting::MINIMAL_SHADOW_MAPS;
   while( argumentParser->read( "--lspsmvb" ) )
      shadowTechnique = PerPixelLighting::LSP_SHADOW_MAP_VIEW_BOUNDS;
   while( argumentParser->read( "--lspsmcb" ) )
      shadowTechnique = PerPixelLighting::LSP_SHADOW_MAP_CULL_BOUNDS;
   while( argumentParser->read( "--lspsmdb" ) )
      shadowTechnique = PerPixelLighting::LSP_SHADOW_MAP_DRAW_BOUNDS;
   while( argumentParser->read( "--continuous-update" ) )
      continuousUpdate = true;
   while( argumentParser->read( "--run-continuous" ) ) // compatibility with osgviewer
      continuousUpdate = true;

   // get model name
   if( argc >= 2 )
      startUpModelName = argv[1];

   // report any errors if they have occurred when parsing the program arguments
   if( argumentParser->errors() )
   {
      argumentParser->writeErrorMessages( std::cerr );
      exitTime = AFTER_PARSING_CMDLINE;
      return;
   }
}


Options::~Options()
{
   delete argumentParser;
}


/** Marks remaining options as unrecognized.
 *
 *  Returns true upon finding unrecognized options,
 *  false if all options were recognized and parsed. */
bool Options::reportRemainingOptionsAsUnrecognized()
{
   // any option left unread are converted into errors to write out later
   argumentParser->reportRemainingOptionsAsUnrecognized();

   // report any errors if they have occurred when parsing the program arguments
   if( argumentParser->errors() )
   {
      argumentParser->writeErrorMessages( std::cout );
      return true;
   }

   return false;
}

/**
 * @file
 * PerPixelLighting class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Geode>
#include <osg/LightSource>
#include <osg/Notify>
#include <osg/Program>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/Timer>
#include <osg/ValueObject>
#include <osgDB/WriteFile> // for debugging purposes
#include <osgShadow/LightSpacePerspectiveShadowMap>
#include <osgShadow/ShadowMap>
#include <sstream>
#include <cassert>
#include "PerPixelLighting.h"
#include "ShadowVolume.h"
#include "PhotorealismData.h"
#include "utils/Log.h"

using namespace std;
using namespace osg;


// maxLights - maximum number of lights in OpenGL implementation
// note: we are using maxLights constant (8 + huge reserve)
// instead of glGetIntegerv( GL_MAX_LIGHTS, &c )
// because rendering context may not be active when using
// this visitor and we would get garbaged values
static const int maxLights = 32;

// forward declarations
std::ostream& operator<<( std::ostream& out,
          const PerPixelLighting::ShaderGenerator::VertexShaderParams& vsp );
std::ostream& operator<<( std::ostream& out,
          const PerPixelLighting::ShaderGenerator::FragmentShaderParams& fsp );



/**
 * Constructor.
 */
PerPixelLighting::PerPixelLighting()
{
}


/**
 * Destructor.
 */
PerPixelLighting::~PerPixelLighting()
{
}


/**
 * Constructor.
 */
PerPixelLighting::ConvertVisitor::ConvertVisitor()
      : NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN ),
        shadowTechnique( NO_SHADOWS )
{
   // there must be always state set in the state stack
   stateStack.push_back( new StateSet() );

   // default light index is 0 and shadowMapTexUnit 1
   mpData.lightBaseIndex = 0;
   mpData.shadowMapTexUnit = 1;
}


/**
 * Destructor.
 */
PerPixelLighting::ConvertVisitor::~ConvertVisitor()
{
   assert( stateStack.size() == 1 && "stateStack overflow or underflow");
   assert( cloneStack.size() == 0 && "cloneStack overflow or underflow");
}


static Node* createPassData( int passNum, Node *scene )
{
   // make sure the root is without state set
   if( scene->getStateSet() || scene->asGroup() == NULL ) {
      Group *newRoot = new Group();
      newRoot->addChild( scene );
      scene = newRoot;
   }

   // create per-pass state set
   StateSet *ss = new StateSet();

   if( passNum == 1 ) {
      ss->setRenderBinDetails( 1, "RenderBin" );
      Depth *depth = new Depth( Depth::LESS );
      ss->setAttributeAndModes( depth, StateAttribute::ON );
   }
   else {
      ss->setRenderBinDetails( passNum, "RenderBin" );
      BlendFunc *blend = new BlendFunc( BlendFunc::ONE, BlendFunc::ONE );
      ss->setAttributeAndModes( blend, StateAttribute::ON | StateAttribute::OVERRIDE );
      Depth *depth = new Depth( Depth::LEQUAL );
      depth->setWriteMask( false );
      ss->setAttributeAndModes( depth, StateAttribute::ON );
   }

   // append the state set
   scene->setStateSet( ss );

   // return (possibly new) scene
   return scene;
}


/**
 * Convert the scene to per-pixel lit scene.
 * The new scene can be retrieved by getScene() function
 * while original scene stays unchanged. Untouched nodes
 * may be shared between getScene() and original scene.
 *
 * Any geometry in the scene that contains shader code,
 * is ignored during the conversion.
 *
 * Parameter useShadowMaps indicates whether shadows should be
 * generated for the scene. osgShadows::ShadowMap is currently
 * used as shadow algorithm.
 */
void PerPixelLighting::convert( Node *scene, ShadowTechnique shadowTechnique )
{
   // start time
   Timer time;

   // notify
   std::string s;
   switch( shadowTechnique ) {
      case NO_SHADOWS:  s = "NO_SHADOWS"; break;
      case SHADOW_VOLUMES: s = "SHADOW_VOLUMES"; break;
      case SHADOW_MAPS: s = "SHADOW_MAPS"; break;
      case STANDARD_SHADOW_MAPS: s = "STANDARD_SHADOW_MAPS"; break;
      case MINIMAL_SHADOW_MAPS:  s = "MINIMAL_SHADOW_MAPS"; break;
      case LSP_SHADOW_MAP_VIEW_BOUNDS: s = "LSP_SHADOW_MAP_VIEW_BOUNDS"; break;
      case LSP_SHADOW_MAP_CULL_BOUNDS: s = "LSP_SHADOW_MAP_CULL_BOUNDS"; break;
      case LSP_SHADOW_MAP_DRAW_BOUNDS: s = "LSP_SHADOW_MAP_DRAW_BOUNDS"; break;
      default: s = "unknown shadow technique";
   };
   notify( NOTICE ) << "PerPixelLighting: Converting scene using " << s << "." << std::endl;

   // convert scene
   ref_ptr< ConvertVisitor > convertVisitor = this->createConvertVisitor();
   convertVisitor->setShadowTechnique( shadowTechnique );

   // use multipass
   convertVisitor->setMultipass( true );

   // collect all lights in the scene
   ref_ptr< CollectLightVisitor > clv = this->createCollectLightVisitor();
   scene->accept( *clv );

   // pass number and number of lights
   int passNum = 1;
   int numLights = 0;

   // ambient pass, if required
   ref_ptr< Node > ambientScene;
   if( clv->getNumLights() == 0 || shadowTechnique == SHADOW_VOLUMES ) {

      // set conversion parameters
      ConvertVisitor::MultipassData &mp = convertVisitor->getMultipassData();
      mp.activeLightSourcePath = NULL;
      mp.activeLight = NULL;
      mp.globalAmbient = true;

      // perform conversion with no lights activated
      scene->accept( *convertVisitor );
      ambientScene = convertVisitor->getScene();
   }

   if( clv->getNumLights() == 0 ) {

      // if no lights, use ambient scene
      newScene = ambientScene;

   } else {

      // create converted scene root
      Group *multipassRoot = new Group;
      multipassRoot->getOrCreateStateSet()->setBinNumber( 0 );
      newScene = multipassRoot;

      // if ambient pass was created, append it
      if( ambientScene ) {

         Node *ambientPass = createPassData( passNum, ambientScene );
         multipassRoot->addChild( ambientPass );
         passNum++;
      }

      // iterate through light sources
      CollectLightVisitor::LightSourceList &lsl = clv->getLightSourceList();
      for( CollectLightVisitor::LightSourceList::const_iterator lightIt = lsl.begin();
         lightIt != lsl.end();
         lightIt++ ) {

         // iterate through multi-parented occurences of the light source
         for( RefNodePathList::const_iterator it = lightIt->second.begin();
            it != lightIt->second.end();
            it++, passNum++, numLights++ ) {

            // select the light for multi-pass
            ConvertVisitor::MultipassData &mp = convertVisitor->getMultipassData();
            mp.activeLightSourcePath = it->get();
            LightSource *ls = dynamic_cast< LightSource* >( (*it)->back() );
            assert( ls->getLight() && "No light!" );
            mp.activeLight = ls->getLight();

            // setup multipass struct
            mp.globalAmbient = ambientScene.valid() ? false : passNum == 1;
            mp.newLight = NULL;

            // convert the scene
            scene->accept( *convertVisitor );
            ref_ptr< Node > renderPassRoot = convertVisitor->getScene();
            mp.activeLightSourcePath = NULL;
            mp.activeLight = NULL;

            // empty pass? => continue
            if( !renderPassRoot ) {
               passNum--;
               numLights--;
               continue;
            }

            if( shadowTechnique != NO_SHADOWS && mp.newLight ) {

               // setup shadows
               osgShadow::ShadowedScene *shadowedScene = new osgShadow::ShadowedScene();
               switch( shadowTechnique ) {

                  case SHADOW_VOLUMES: {

                     osgShadow::ShadowVolume *sv = new osgShadow::ShadowVolume();
                     sv->setLight( mp.newLight );
                     sv->disableAmbientPass( true ); // we already created ambient pass
                     if( passNum <=2 )
                        sv->setClearStencil( false ); // stencil is cleared on the beginning of the frame
                     sv->setMethod( osgShadow::ShadowVolumeGeometryGenerator::ZFAIL );
                     sv->setMode(osgShadow::ShadowVolumeGeometryGenerator::SILHOUETTES_ONLY);
                     sv->setStencilImplementation( osgShadow::ShadowVolume::STENCIL_TWO_SIDED );
                     sv->setShadowCastingFace( osgShadow::ShadowVolumeGeometryGenerator::BACK );
                     //sv->setFaceOrdering(osgShadow::ShadowVolumeGeometryGenerator::CW);
                     sv->setUpdateStrategy( osgShadow::ShadowVolume::MANUAL_INVALIDATE );
                     shadowedScene->setShadowTechnique( sv );
                     break;
                  }

                  case SHADOW_MAPS: {

                     // setup ShadowMap
                     osgShadow::ShadowMap *sm = new osgShadow::ShadowMap();
                     sm->setLight( mp.newLight );
                     sm->setTextureUnit( mp.shadowMapTexUnit );
                     sm->setTextureSize( Vec2s( 2048, 2048 ) );
                     shadowedScene->setShadowTechnique( sm );
                     break;
                  }

                  case STANDARD_SHADOW_MAPS:
                  case MINIMAL_SHADOW_MAPS:
                  case LSP_SHADOW_MAP_VIEW_BOUNDS:
                  case LSP_SHADOW_MAP_CULL_BOUNDS:
                  case LSP_SHADOW_MAP_DRAW_BOUNDS: {

                     osgShadow::StandardShadowMap *sm = NULL;
                     switch( shadowTechnique ) {
                        case STANDARD_SHADOW_MAPS:       sm = new osgShadow::StandardShadowMap(); break;
                        case MINIMAL_SHADOW_MAPS:        sm = new osgShadow::MinimalShadowMap(); break;
                        case LSP_SHADOW_MAP_VIEW_BOUNDS: sm = new osgShadow::LightSpacePerspectiveShadowMapVB(); break;
                        case LSP_SHADOW_MAP_CULL_BOUNDS: sm = new osgShadow::LightSpacePerspectiveShadowMapCB(); break;
                        case LSP_SHADOW_MAP_DRAW_BOUNDS: sm = new osgShadow::LightSpacePerspectiveShadowMapDB(); break;
                        default: assert( false );
                     }
                     // setup shadow map
                     sm->setLight( mp.newLight );
                     sm->setBaseTextureUnit( 0 );
                     sm->setBaseTextureCoordIndex( 0 );
                     sm->setShadowTextureUnit( mp.shadowMapTexUnit );
                     sm->setShadowTextureCoordIndex( mp.shadowMapTexUnit );
                     sm->setTextureSize( Vec2s( 2048, 2048 ) );
                     if( mp.newLightCubeShadowMap ) {
                        //sm->setCubeMap( true );
                        //sm->setDebugDraw( true );
                     }
                     osgShadow::MinimalShadowMap *msm = dynamic_cast< osgShadow::MinimalShadowMap* >( sm );
                     if( msm ) {
                        //msm->setMinLightMargin( 1000.f );
                        //msm->setMaxFarPlane( 10.f );
                     }
                     shadowedScene->setShadowTechnique( sm );
                     break;
                  }

               }

               // append shadows
               shadowedScene->addChild( renderPassRoot );
               renderPassRoot = shadowedScene;

            }

            // create pass data (blending, renderBinDetails, depth test,...)
            renderPassRoot = createPassData( passNum, renderPassRoot );

            // append the pass to the scene
            multipassRoot->addChild( renderPassRoot );

   #if 1 // this is temporary workaround for light index
         // until a solution is developed for handling the same indices in PositionalStateContainer.
         // Limitations: only 8 lights supported.
         // PCJohn-2010-04-12
            mp.lightBaseIndex++;
            mp.shadowMapTexUnit++;
   #endif
         }

      }
   }

#if 0 // debug: write converted scene to file
   osgDB::writeNodeFile( *newScene.get(), "PerPixelLighting.osg" );
#endif

   Log::notice() << QString( "PerPixelLighting: Converted %1 lights. Operation completed "
                             "in %2ms.").arg( numLights ).arg( time.time_m(), 0, 'f', 2 ) << Log::endm;
}


/**
 * Processes a node in the scene graph.
 */
void PerPixelLighting::ConvertVisitor::apply( Node &node )
{
   // process node's state set
   processState( node.getStateSet() );

   // traverse children
   traverse( node );

   // unprocess node's state set
   unprocessState();
}


/**
 * Processes a geode in the scene graph.
 * Appends per pixel lighting shaders to appropriate places of the scene graph.
 */
void PerPixelLighting::ConvertVisitor::apply( Geode &geode )
{
   // process geode's state set
   Geode *newGeode = dynamic_cast< Geode* >( processState( geode.getStateSet() ) );

   // traverse drawables
   for( unsigned int i=0, c=geode.getNumDrawables(); i<c; i++ ) {

      // process drawable
      Drawable *d = geode.getDrawable( i );
      Drawable *newD = dynamic_cast< Drawable* >( processState( d->getStateSet(), d ) );
      Drawable *latestD = newD ? newD : d;
      StateSet *cumulatedStateSet = getCumulatedStateSet();

      // process transparent drawables in ambient pass only
      if( cumulatedStateSet->getRenderingHint() == StateSet::TRANSPARENT_BIN &&
          !mpData.globalAmbient )
      {
         // clone geode, if not already
         if( !newGeode )
            newGeode = dynamic_cast< Geode* >( cloneCurrentPath() );

         // remove drawable
         newGeode->removeDrawable( latestD );

         // process next drawable
         unprocessState();
         continue;
      }

      // process REPLACE textures in ambient pass only
      TexEnv *e =  dynamic_cast< TexEnv* >( cumulatedStateSet->getTextureAttribute( 0, StateAttribute::TEXENV ));
      if( !mpData.globalAmbient && e && e->getMode() == TexEnv::REPLACE ) {

         // clone geode, if not already
         if( !newGeode )
            newGeode = dynamic_cast< Geode* >( cloneCurrentPath() );

         // remove drawable
         newGeode->removeDrawable( latestD );

         // process next drawable
         unprocessState();
         continue;
      }

#if 0 // debug code for printing texture info

         Texture *t = dynamic_cast< Texture* >( cumulatedStateSet->getTextureAttribute( 0, StateAttribute::TEXTURE ));
         Image *i = ( t ? t->getImage( 0 ) : NULL );
         TexEnv *e =  dynamic_cast< TexEnv* >( cumulatedStateSet->getTextureAttribute( 0, StateAttribute::TEXENV ));
         notify( INFO ) << "TEXTURE: "
               << ( cumulatedStateSet->getTextureMode( 0, GL_TEXTURE_2D ) ? "y" : "n" )
               << "  t: " << ( t ? "y" : "NULL" )
               << "  i: " << ( i ? "y" : "NULL" )
               << "  name: " << ( i ? i->getFileName() : "< none >" )
               << "  e: ";
         if( e ) notify( INFO ) << e->getMode() << endl;
         else notify( INFO ) << "NULL" << endl;
#endif

      //
      // insert per-pixel-lighting shader
      //

      StateSet *newS;

      if( latestD->getStateSet() ) {

         //
         //  Insert shader to Drawable's StateSet
         //

         // make sure that we have cloned drawable
         if( !newD ) {

            // clone geode
            if( !newGeode )
               newGeode = dynamic_cast< Geode* >( cloneCurrentPath() );

            // clone current drawable
            newD = dynamic_cast< Drawable* >(
                  d->clone( CopyOp::SHALLOW_COPY ) );
            newGeode->replaceDrawable( d, newD );
         }

         // clone state set if not cloned already
         StateSet *oldS = d->getStateSet();
         newS = newD->getStateSet();
         if( newS == oldS ) {
            newS = (StateSet*)oldS->clone( CopyOp::SHALLOW_COPY );
            newD->setStateSet( newS );
         }

      } else {

         //
         //  Insert shader to StateSet of a Node above in the scene graph
         //

         // find the node to insert state set
         int i = getLastStateSetPathIndex();
         if( i == -1 )
            i = 0;
         Node *oldNode = getNodePath()[i];
         Node *newNode = clonePathUpToIndex( i );

         // clone state set, if not cloned already
         StateSet *oldS = oldNode->getStateSet();
         newS = newNode->getOrCreateStateSet();
         if( newS == oldS ) {
            newS = (StateSet*)oldS->clone( CopyOp::SHALLOW_COPY );
            newNode->setStateSet( newS );
         }

      }

      ref_ptr< StateSet > ss = cumulatedStateSet;

      if( multipassActive ) {

         // create multipass compatible state set
         StateSet *compatibleSs = adjustStateSet( ss, true, true, false );
         if( compatibleSs )
            ss = compatibleSs;
         else
            if( cumulatedStateSet )
               ss = (StateSet*)cumulatedStateSet->clone( CopyOp::SHALLOW_COPY );
            else
               ss = new StateSet();

         // turn on one light
         // with one exception when renderig transparent drawables;
         // transparent drawables are rendered with all lights off (no lighting now
         // until more sophisticated algorithms for transparent drawables are developed)
         if( mpData.activeLight.get() != NULL &&
             cumulatedStateSet->getRenderingHint() != StateSet::TRANSPARENT_BIN )
         {
            ss->setMode( GL_LIGHT0 + mpData.lightBaseIndex, StateAttribute::ON );
         }
      }

      // need cube map?
      mpData.newLightCubeShadowMap = false;
      /*if( shadowTechnique != PerPixelLighting::NO_SHADOWS &&
          shadowTechnique != PerPixelLighting::SHADOW_VOLUMES )
      {
         const Light *l = mpData.activeLight.get();
         if( l != NULL )
            if( l->getPosition()[3] != 0 )
               if( l->getSpotCutoff() > 45 )
                  mpData.newLightCubeShadowMap = true;
      }*/

      // shadow map texture unit
      int shadowMapTexUnit;
      if( shadowTechnique == PerPixelLighting::NO_SHADOWS ||
          shadowTechnique == PerPixelLighting::SHADOW_VOLUMES )
         shadowMapTexUnit = -1;
      else
         shadowMapTexUnit = mpData.shadowMapTexUnit;

      // insert shader program
      Program *glProgram = this->createShaderProgram( ss, shadowMapTexUnit,
            mpData.newLightCubeShadowMap, mpData.globalAmbient );
      newS->setAttribute( glProgram, StateAttribute::ON );

      // unprocess drawable
      unprocessState();
   }

   // traverse node
   traverse( geode );

   // unprocess geode
   unprocessState();
}


/** The method attempts to remove empty groups from the scene graph.
 *
 *  When the method is called, it expects NULL or parent clone
 *  at the top of cloneStack. If the clone exists, its children are
 *  checked whether they are cloned as well. The cloned children that
 *  are Geodes and does not have any Drawables are removed. The cloned
 *  children that are Groups with no child nodes are removed as well.
 *
 *  Original (not cloned) graph is kept intact. */
void PerPixelLighting::ConvertVisitor::purgeEmptyNodes( Group &parent )
{
   // do anything only if parent (given as parameter) has its clone
   // (e.g. do not modify original scene graph)
   Node *clonedNode = cloneStack.back();
   if( clonedNode ) {

      // iterate through all children of cloned parent
      Group *clonedParent = clonedNode->asGroup();
      unsigned int i = 0, c = clonedParent->getNumChildren();
      while( i<c )
      {
         Node *child = clonedParent->getChild( i );

         // skip children that seems to be not cloned
         // (original scene graph must not be modified)
         if( parent.containsNode( child ) ) {
            i++;
            continue;
         }

         // remove empty geodes
         Geode *childGeode = child->asGeode();
         if( childGeode && childGeode->getNumDrawables() == 0 ) {
            clonedParent->removeChild( i );
            c--;
            continue;
         }

         // remove empty groups
         Group *childGroup = child->asGroup();
         if( childGroup && childGroup->getNumChildren() == 0 ) {
            clonedParent->removeChild( i );
            c--;
            continue;
         }

         i++;
      }
   }
}


void PerPixelLighting::ConvertVisitor::apply( Group &group )
{
   // process group's state set
   processState( group.getStateSet() );

   // traverse children
   traverse( group );

   // remove empty groups and geodes in the subgraph
   purgeEmptyNodes( group );

   // unprocess group's state set
   unprocessState();
}


static string getUserData( const string &domain, const string &name,
                           const Object *obj1, const Object *obj2 = NULL, const Object *obj3 = NULL )
{
   string str, value;
   if( obj1 )
   {
      obj1->getUserValue< string >( domain, str );
      value = PhotorealismData::getValue( str, name );
      if( !value.empty() )
         return value;
   }

   if( obj2 )
   {
      obj2->getUserValue< string >( domain, str );
      value = PhotorealismData::getValue( str, name );
      if( !value.empty() )
         return value;
   }

   if( obj3 )
   {
      obj3->getUserValue< string >( domain, str );
      value = PhotorealismData::getValue( str, name );
      if( !value.empty() )
         return value;
   }

   return "";
}


void PerPixelLighting::ConvertVisitor::apply( LightSource &lightSource )
{
   // process node's state set
   ref_ptr< LightSource > newLightSource =
         dynamic_cast< LightSource*>( processState( lightSource.getStateSet() ) );
   LightSource *latestLS = newLightSource.get() ? newLightSource.get() : &lightSource;

   if( multipassActive ) {

      if( mpData.activeLight == lightSource.getLight() &&
          *mpData.activeLightSourcePath.get() == getNodePath() ) {

         // process active light

         assert( latestLS->getLight() && "No light." );

         // get LightSource.beamWidthAngle and LightSource.concentrationExponent from user data
         string beamWidthString = ::getUserData( "Photorealism", "LightSource.beamWidthAngle", latestLS->getLight(), latestLS->getStateSet(), latestLS );
         string concentrationExponentString = ::getUserData( "Photorealism", "LightSource.concentrationExponent", latestLS->getLight(), latestLS->getStateSet(), latestLS );

         // clone if required
         // (there are two reasons for cloning: setting of beamWidthAngle and
         //  when latestLS->getLight()->getLightNum() != mpData.lightBaseIndex.
         //  When cloning did not happen (in old code), we used to set mpData.newLight = latestLS->getLight().)

         // clone LightSource
         LightSource *ls = dynamic_cast< LightSource* >( cloneCurrentPath() );

         // clone Light
         mpData.newLight = dynamic_cast< Light* > ( ls->getLight()->clone( CopyOp::SHALLOW_COPY ) );
         ls->setLight( mpData.newLight );

         // set light num
         mpData.newLight->setLightNum( mpData.lightBaseIndex );

         // compute beamWidthAngle if not set
         // (do not use -1. as it would activate OpenGL-style spotlight and
         // we prefer DirectX style spotlight)
         double beamWidthAngleCos;
         if( beamWidthString.empty() )
            //beamWidthAngleCos = ( 1. + cos( mpData.newLight->getSpotCutoff() / 180. * PI ) ) / 2.;
            beamWidthAngleCos = cos( mpData.newLight->getSpotCutoff() / 180. * PI / 2. );
         else
            beamWidthAngleCos = cos( atof( beamWidthString.c_str() ) );

         // set beamWidthAngle (use specular alpha as a hack because
         // there is no beamWidthAngle variable in light structure
         Vec4 specular = mpData.newLight->getSpecular();
         specular.w() = beamWidthAngleCos;
         mpData.newLight->setSpecular( specular );

         // set concentrationExponent
         if( concentrationExponentString.empty() )
         {
            // set exponent to 1. if using DirectX spotlight and concentrationExponent/beamWidthString was not given
            if( beamWidthString.empty() )
               mpData.newLight->setSpotExponent( 1. );
         }
         else
            mpData.newLight->setSpotExponent( atof( concentrationExponentString.c_str() ) );

      } else {

         // process not active lights
         // by replacing LightSources by Groups

         // clone all nodes on the path
         Group *clonedParent = dynamic_cast< Group* >( cloneCurrentPathUpToParent() );
         assert( clonedParent && "cloneCurrentPathUpToParent did not returned Group." );

         // replace this light source by a group
         Group *newNode = new Group( *latestLS, CopyOp::SHALLOW_COPY );
         cloneStack.back() = newNode;
         clonedParent->replaceChild( latestLS, newNode );
      }
   }

   // traverse children
   traverse( lightSource );

   // remove empty groups and geodes in the subgraph
   purgeEmptyNodes( lightSource );

   // unprocess node's state set
   unprocessState();
}


/**
 * The method adjusts content of StateSet according to given requirements.
 *
 * If removeLightAttribs is true, all light attributes are removed.
 * The same is done for removeLightModes.
 * If adjustTransparency is true and the StateSet contains renderHint to
 * TRANSPARENT_BIN, transparency related settings are applied to the StateSet.
 *
 * The function does not modify original StateSet. Instead, if any updates to
 * the StateSet are required, its copy is created and updated copy is returned.
 * If no changes to the original StateSet are required and no copy was made,
 * NULL is returned. */
StateSet* PerPixelLighting::ConvertVisitor::adjustStateSet( const StateSet *ss, bool removeLightAttribs,
                                                            bool removeLightModes, bool adjustTransparency ) const
{
   // if NULL, nothing to adjust
   if( ss == NULL )
      return NULL;

   // returned state set defaults to NULL
   StateSet *newSs = NULL;

   if( removeLightAttribs )
   {
      // iterate through all OpenGL lights
      for( int i=0; i<maxLights; i++ )
      {
         // get light attribute
         const Light *l = dynamic_cast< const Light* >( ss->getAttribute( StateAttribute::LIGHT, i ));

         // if no light attribute, continue
         if( !l )
            continue;

         // if state set was not cloned yet, do it now
         if( !newSs )
            newSs = new StateSet( *ss );

         // remove light attribute
         newSs->removeAttribute( StateAttribute::LIGHT, i );
      }
   }

   if( removeLightModes )
   {
      // iterate through all OpenGL light modes
      for( int i=0; i<maxLights; i++ )
      {
         // get light mode
         StateAttribute::GLModeValue v = ss->getMode( GL_LIGHT0 + i );

         // if no mode in this state set, continue
         if( (v & StateAttribute::INHERIT) != 0 )
            continue;

         // if state set was not cloned yet, do it now
         if( !newSs )
            newSs = new StateSet( *ss );

         // remove light mode
         newSs->removeMode( GL_LIGHT0 + i );
      }
   }

   if( adjustTransparency )
   {
      // transparent state sets need to set setNestRenderBins to false
      if( ss->getRenderingHint() == StateSet::TRANSPARENT_BIN )
      {
         // if state set was not cloned yet, do it now
         if( !newSs )
            newSs = new StateSet( *ss );

         newSs->setNestRenderBins( false );
         newSs->setAttributeAndModes( new Depth( Depth::LEQUAL, 0., 1., false ) );
         newSs->setAttributeAndModes( new BlendFunc );
      }
   }

   return newSs;
}


/**
 * Helper method for updating the scene traversal state
 * requred for proper creation of per pixel lighting.
 * The method is expected to be called when entering a node or a drawable
 * during the traversal.
 *
 * Based on various ConvertVisitor settings, this method may
 * be required to modify the StateSet of the current node or drawable.
 * This result in cloning the scene graph while cloned node or drawable
 * is returned. The new node or drawable contains the new StateSet.
 */
Object* PerPixelLighting::ConvertVisitor::processState( StateSet *s, Drawable *d )
{
   // push NULL (NULL may be replaced later when node is cloned)
   cloneStack.push_back( NULL );

   if( !s ) {

      // no state set -> push NULL
      stateStack.push_back( NULL );

   }
   else
   {

      // combine with previous state set
      StateSet *sc = new StateSet( *getCumulatedStateSet(), CopyOp::SHALLOW_COPY );
      sc->merge( *s );
      stateStack.push_back( sc );

      // multipass may modify light settings
      if( multipassActive ) {

         // adjust state set, if required
         StateSet *ms = adjustStateSet( s, true, true, true );
         if( ms ) {

            // if new state set was generated, clone scene nodes on the current path
            Node *clonedNode = cloneCurrentPath();

            // replace state set
            if( d ) {

               // if it is drawable's state set,
               // clone it and replace StateSet in the clone
               Drawable *newD = dynamic_cast< Drawable* >( d->clone( CopyOp::SHALLOW_COPY ) );
               newD->setStateSet( ms );
               Geode *clonedGeode = dynamic_cast< Geode* >( clonedNode );
               assert( clonedGeode != NULL &&
                       "processStateSet's Drawable is set while parent is not Geode!" );
               clonedGeode->replaceDrawable( d, newD );

               // return new drawable
               return newD;
            }
            else {

               // replace node's state set
               clonedNode->setStateSet( ms );

               // return new node
               return clonedNode;
            }
         }
      }
   }

   return NULL;
}


/**
 * Helper function for updating the scene traversal state
 * requred for proper creation of per pixel lighting.
 * The method is expected to be called when leaving a node or a drawable
 * during the traversal.
 */
void PerPixelLighting::ConvertVisitor::unprocessState()
{
   stateStack.pop_back();
   cloneStack.pop_back();
}


/**
 * Returns the StateSet that cumulates all the StateSets
 * on the visitor path.
 */
StateSet* PerPixelLighting::ConvertVisitor::getCumulatedStateSet()
{
   StateStack::reverse_iterator i = stateStack.rbegin();
   while (i != stateStack.rend())
      if (*i != NULL)  return *i;
      else  i++;
   return NULL;
}


/**
 * Returns the index to the visitor's node path of the last node
 * containing state set (the most recent state set affecting drawable or node).
 * If there is no state set on the path, it returns -1.
 */
int PerPixelLighting::ConvertVisitor::getLastStateSetPathIndex()
{
   StateStack::reverse_iterator iter = stateStack.rbegin();
   int i = getNodePath().size();
   while( i > 0 && iter != stateStack.rend() ) {
      i--;
      if (*iter != NULL) {
         assert( getNodePath()[i]->getStateSet() != NULL && "Wrong indexing." );
         return i;
      }
      iter++;
   }
   return -1;
}


/**
 * Clones all the nodes from the root to the current node
 * on the visitor's path. It returns the clone of the current node.
 *
 * FIXME: Application at the moment when getNodePath() returns
 * empty path is not handled yet.
*/
Node* PerPixelLighting::ConvertVisitor::cloneCurrentPath()
{
   return clonePathUpToIndex( getNodePath().size()-1 );
}


/**
 * Clones all the nodes on the visitor's path, starting
 * at the root node and finishing at the parent of the current node.
 * It returns the clone of the parent node.
 *
 * FIXME: Application at the moment when getNodePath() returns
 * empty path or path of single node is not handled yet.
 */
Node* PerPixelLighting::ConvertVisitor::cloneCurrentPathUpToParent()
{
   return clonePathUpToIndex( getNodePath().size()-2 );
}


/**
 * Clones all the nodes from the root to the node on index i
 * on the visitor's path. It returns the clone of the node on the index i.
 */
Node* PerPixelLighting::ConvertVisitor::clonePathUpToIndex( int i )
{
   const NodePath &path = getNodePath();
   CloneStack::iterator iter = cloneStack.begin();
   CloneStack::iterator prev = iter;
   assert( i < int(path.size()) && "Index is bigger than path size." );
   assert( i >= 0 && "Index is negative. Implement it by adding empty Group" );

   // iterate from 0 to i
   for( int x=0; x<=i; x++, prev=iter, iter++ ) {
      assert( iter != cloneStack.end() && "Data integrity error." );

      // skip already cloned items
      if( *iter != NULL )
         continue;

      if( x == 0 ) {

         // clone node and set newScene
         *iter = (Node*) path[x]->clone( CopyOp::SHALLOW_COPY );
         newScene = *iter;

      } else {

         // clone node
         Node *oldNode = path[x];
         Node *newNode = (Node*) oldNode->clone( CopyOp::SHALLOW_COPY );
         *iter = newNode;

         // update parent
         Group *parent = (*prev)->asGroup();
         assert(parent && "Non-group node with children!");
         parent->replaceChild( oldNode, newNode );

      }
   }

   return *prev;
}


/**
 * Creates ConvertVisitor. Overriding of this method enables using of
 * the user-defined convert visitors.
 */
PerPixelLighting::ConvertVisitor* PerPixelLighting::createConvertVisitor() const
{
   return new ConvertVisitor();
}


PerPixelLighting::CollectLightVisitor* PerPixelLighting::createCollectLightVisitor() const
{
   return new CollectLightVisitor();
}


void PerPixelLighting::ConvertVisitor::recreateShaderGenerator()
{
   shaderGenerator = new ShaderGenerator();
}


/**
 * Virtual method that creates the shader program for per-pixel lighting.
 * The function just calls PixePixelLighting::createShaderProgram function
 * unless overriden in derived class.
 */
class Program* PerPixelLighting::ConvertVisitor::createShaderProgram( const StateSet *s,
           int shadowMapTextureUnit, bool cubeShadowMap, bool globalAmbient )
{
   if( !shaderGenerator )
      recreateShaderGenerator();

   return shaderGenerator->getProgram( s, shadowMapTextureUnit, cubeShadowMap, this->shadowTechnique, globalAmbient );
}


void PerPixelLighting::ConvertVisitor::setShadowTechnique( ShadowTechnique shadowTechnique )
{
   this->shadowTechnique = shadowTechnique;
}



//
//
//       Implementation of
//
//  Program* PerPixelLighting::createShaderProgram( const StateSet *s )
//
//     and comments about shaders and shader builder architecture.
//
//

// Shader sources and tutorials:
// http://www.lighthouse3d.com/opengl/glsl/index.php
// http://www.ozone3d.net/tutorials/glsl_lighting_phong.php
// http://www.opengl.org/sdk/docs/tutorials/ClockworkCoders/lighting.php
//
// Bump-mapping (investigate):
// http://www.paulsprojects.net/opengl/bumpatten/bumpatten.html


// Comments on implementation completeness, bugs, limitations,...:
//
// missing:
// - GL_COLOR_MATERIAL (one material component from gl_Color?)
// - back materials
// - face culling
// - all lights
// - all textures
// - directional light (be aware of attenuation)
// - spot light
// - two sided lighting
// - back side color computation
// - separate speculat color (including color sum)
//
// easy to implement, but no testing data:
// - 4D vertex coordinates (eyeDir should be vec4)
// - light position with 4D coordinates (related to previous point)
// - alpha when processing textures (RGBA textures, transp. mat.)
//
// non-compatible improvements:
// - per-pixel lighting
// - specular computed by reflection vector
//   (OpenGL 2.1 spec uses half vector that is less precise)
//
// not supported:
// - indexed mode
// - LIGHT_MODEL_LOCAL_VIEWER set to false
//   (always using local viewer because it provides more realism)
// - texture coord matrices



/**
 * The method creates the vertex shader for per-pixel lighting.
 *
 * Notes to the content of vsp parameter:
 * texCoords indicates whether texture coordinates for texturing unit 0
 * should be passed to fragment shader.
 * shadowMapTexCoords parameter determines whether texture
 * coordinates for shadow mapping are generated on texturing unit 1
 * based on texgen setup of texture unit 1.
 */
Shader* PerPixelLighting::ShaderGenerator::createVertexShader(
          const VertexShaderParams& vsp )
{
   notify( INFO )
             << "PerPixelLighting::ShaderGenerator: "
             << "Creating VERTEX shader with params:" << endl
             << vsp << endl;


   // Number of varying variables
   //
   // Please note that there is hardware limit on number of varying
   // variables. For instance, the limit is exhausted on Mobile Radeon
   // X1300M when trying to append anything (float or vector) over
   // 3x vec3, 3x vec4, 1x float. And it seems that any of these
   // consume one space - for example, float can be replaced by
   // vec4 (one float occupies the same place as four-component vec4).
   // This was seen on Linux ATI drivers.

   stringstream code;
   code << "varying vec3 vertex;" << endl;  // one per vertex, linear interpolation
   code << "varying vec3 normal;" << endl;  // one per vertex, linear interp. - big
                                            // angle not precise, rather circ. interp.
   code << "varying vec4 ambient;" << endl;  // one per vertex, linear interp.
   code << "varying vec4 diffuse;" << endl;  // one per vertex, linear interp.
   code << "varying vec4 specular;" << endl; // one per vertex, linear interp.
   code << "varying float shininess;" << endl; // one per vertex, probably linear interp.
#if 0 // note to emissive component:
   // there is not enough "slots" for all the varying variables
   // on older graphics cards (for example Mobile Radeon X1300),
   // so we are going to use gl_FrontColor and gl_BackColor for it
   // while we will get it as gl_Color in the fragment shader
   varying vec4 emissive; // one per vertex, linear interp.
#endif
   code << endl;

   code << "void main()" << endl;
   code << "{" << endl;

   // vertex position in eye coordinates
   code << "   vec4 vertex4 = gl_ModelViewMatrix * gl_Vertex;" << endl;
   code << "   vertex = vec3( vertex4 );" << endl;

   // vertex normal
   code << "   normal = gl_NormalMatrix * gl_Normal;" << endl;

   // material
   // note on diffuse component: we use gl_Color instead of
   //                            gl_FrontMaterial.diffuse as it seems that
   //                            GL_COLOR_MATERIAL is enabled in OSG
   //                            and diffuse is passed in by glColor
   // note on emission component: emissive variable is replaced by
   //                             gl_FrontColor and gl_BackColor (see above)
   code << "   ambient = gl_FrontMaterial.ambient;" << endl;
   code << "   diffuse = gl_Color;" << endl;
   code << "   specular = gl_FrontMaterial.specular;" << endl;
   code << "   gl_FrontColor = gl_FrontMaterial.emission;" << endl;
   code << "   gl_BackColor = gl_FrontMaterial.emission;" << endl;
   code << "   shininess = gl_FrontMaterial.shininess;" << endl;

   // texture coordinates
   if( vsp.texCoords )
      code << "   gl_TexCoord[0] = gl_MultiTexCoord0;" << endl;
   if( vsp.shadowMapTexUnit != -1 ) {
      code << "   gl_TexCoord[" << vsp.shadowMapTexUnit << "].s = "
              "dot( vertex4, gl_EyePlaneS[" << vsp.shadowMapTexUnit << "] );" << endl;
      code << "   gl_TexCoord[" << vsp.shadowMapTexUnit << "].t = "
              "dot( vertex4, gl_EyePlaneT[" << vsp.shadowMapTexUnit << "] );" << endl;
      code << "   gl_TexCoord[" << vsp.shadowMapTexUnit << "].p = "
              "dot( vertex4, gl_EyePlaneR[" << vsp.shadowMapTexUnit << "] );" << endl;
      code << "   gl_TexCoord[" << vsp.shadowMapTexUnit << "].q = "
              "dot( vertex4, gl_EyePlaneQ[" << vsp.shadowMapTexUnit << "] );" << endl;
   }

   // ftransform provides invariance with standard OpenGL
   // pipeline (it is deprecated, but this shader is intended to
   // run on old graphics cards - shading language 1.1)
   // new command can be: gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
   code << "   gl_Position = ftransform();" << endl;
   code << "}" << endl;

   // vertex shader object instance
   Shader *vertexShader = new Shader( Shader::VERTEX );
   vertexShader->setShaderSource( code.str().c_str() );
   return vertexShader;
}


/**
 * The function creates the fragment shader's header.
 * It is composed of variable declarations used in the shader.
 *
 * The function's parameter fs is stringstream to which the shader's
 * header will be appended.
 * baseTextureUniform specifies the base texture of the geometry.
 * It is the name of the texture's uniform. If the geometry
 * does not have a texture, empty string should be passed in.
 * shadowTextureUniform is a shadow texture. It is a name of
 * an uniform holding the shadow texture.
 */
static void createFragmentShaderHeader( stringstream &fs,
          const string& baseTextureUniform = "",
          const vector< string >& shadowTextureUniforms = vector< string >(),
          bool cubeShadowMap = false )
{
   // declaration of input variables
   // note on emissive component:
   //    varying vec4 emissive is not declared because emissive component is passed
   //    using gl_Color (using gl_FrontColor and gl_BackColor in vertex shader).
   //    The reason is in limited number of available interpolators.
   //    Details are explained in vertex shader generator function.
   fs << "varying vec3 vertex;" << endl;
   fs << "varying vec3 normal;" << endl;
   fs << "varying vec4 ambient;" << endl;
   fs << "varying vec4 diffuse;" << endl;
   fs << "varying vec4 specular;" << endl;
   fs << "varying float shininess;" << endl;
   fs << endl;

   // textures
   bool appendEndl = false;
   if( !baseTextureUniform.empty() ) {
      fs << "uniform sampler2D " << baseTextureUniform << ";" << endl;
      appendEndl = true;
   }
   for( unsigned int i=0; i<shadowTextureUniforms.size(); i++ )
      if( !shadowTextureUniforms[i].empty() ) {
         fs << "uniform sampler" << (cubeShadowMap ? "Cube" : "2D") << "Shadow " << shadowTextureUniforms[i] << ";" << endl;
         appendEndl = true;
      }
   if( appendEndl )
      fs << endl;
}


/**
 * The method creates the shader code for one light computation.
 * The result is either a piece of inline code or a function for
 * processing of one light.
 *
 * The parameter fs is stringstream to which the generated code is appended.
 * makeFunction when set to true tells the method to generate the code
 * in the form of function. Otherwise, inline code is generated.
 * destVariable specifies the name of a variable that receives the result
 * of the light computation. This is useful especially when generating inline code.
 * In that case, the variable has to be already declared in the previous code
 * as vec4 while its content is going to be replaced by the result of the light
 * calculation. When generating the function, the variable is declared automatically.
 * compatibilityParams switches whether the function parameter list
 * will contain light as gl_LightSourceParameters, or whether all required
 * light parameters are passed into the generated function as separate arguments.
 * The first option turned out to make problems some older graphics drivers.
 * The second seems to work always but it expands parameter list
 * quite much. FIXME: performance may be investigated here.
 * paramPrefix makes it possible to change the light for which lighting code
 * should be generated. It is implemented by replacing standard "lightSource."
 * (followed by the light parameter like ambient, diffuse,...) by other text
 * specified by the parameter, for example, gl_LightSource[1].
 * As a result, lightSource.diffuse will be translated to gl_LightSource[1].diffuse,
 * making the computation of OpenGL light 1. The parameter is in the effect only
 * when generating inline code.
 */
static void createFragmentShaderLightingCode( stringstream &fs,
          bool makeFunction = true, const string& destVariable = "",
          bool compatibilityParams = true, const string& paramPrefix = "" )
{
   // set the name for shader variable that will receive the computed color
   string destVar( "color" );
   if( !destVariable.empty() )
      destVar = destVariable;

   // Set the variable name prefix that may be used instead of "lightSource.".
   // The real replacement is done bellow after the shader code is created.
   string prefix( "lightSource." );
   if( compatibilityParams && makeFunction )
      prefix = "ls_"; // force prefix to ls_
   if( !makeFunction )
      if( !paramPrefix.empty() ) // empty prefix not allowed
         prefix = paramPrefix;


   if( makeFunction ) {
      // vec4 processLight(const gl_LightSourceParameters lightSource,
      //                   vec3 v, vec3 n)
      // parameters: li - light index" << endl;
      //             v  - normalized eye to vertex vector" << endl;
      //             n  - normalized normal" << endl;
      // note on compatibility issue: see comment bellow compatibility code
      if( !compatibilityParams ) {

         fs << "vec4 processLight( const gl_LightSourceParameters lightSource," << endl;
         fs << "                   vec3 v, vec3 n, bool twoSidedLighting )" << endl;

      } else {

         fs << "vec4 processLight( vec4 ls_ambient, vec4 ls_diffuse, vec4 ls_specular," << endl;
         fs << "                   vec4 ls_position, vec3 ls_spotDirection," << endl;
         fs << "                   float ls_spotExponent, float ls_spotCosOuterAngle," << endl;
         fs << "                   float ls_spotCosInnerAngle," << endl;
         fs << "                   float ls_constantAttenuation, float ls_linearAttenuation, " << endl;
         fs << "                   float ls_quadraticAttenuation, " << endl;
         fs << "                   vec3 v, vec3 n, bool twoSidedLighting )" << endl;

         // note: Old drivers does not accept gl_LightSourceParameters type as a parameter.
         //       It was necessary to split it to its components (ambient, diffuse, ...)
         //       This was see on ATI Mobile Radeon X1300, driver 8.261, release date 23.5.2006.
         //
         // Simple example of code that failed to work:
         //
         //    fs << "vec4 processLight( gl_LightSourceParameters lightSource )" << endl;
         //    fs << "{" << endl;
         //    fs << "   return vec4(1.,1.,1.,1.);" << endl;
         //    fs << "}" << endl;
         //
         //    // Call of the function
         //    fs << "   gl_FragColor += processLight( gl_LightSource[1] );" << endl;
         //
         // Example that worked:
         //
         //    fs << "vec4 processLight( vec4 color )" << endl;
         //    fs << "{" << endl;
         //    fs << "   return color;" << endl;
         //    fs << "}" << endl;
         //
         //    // Call of the function
         //    fs << "   gl_FragColor += processLight( gl_LightSource["
         //       << i << "].diffuse );" << endl;
         //
      }
      fs << "{" << endl;
   }

   // remember stream position for the case of replacing "lightSource." by
   // the content of prefix
   // note: pos_type is converted to size_type (can produce sign/unsign mismatch)
   iostream::pos_type pos = fs.tellp();
   string::size_type replaceStart = pos;

   // compute variables:
   //   lv - light vector (vertex to light)
   //   ld - light distance
   //   l  - normalized light vector
   fs << "   vec3 lv = vec3( lightSource.position );" << endl;
   fs << "   if( lightSource.position.w != 0. )" << endl;
   fs << "      lv -= vertex;" << endl;
   fs << "   float ld = length( lv );" << endl;
   fs << "   vec3 l = lv / ld;" << endl;
   fs << endl;

   // result variable
   fs << "   " << (makeFunction ? "vec4 " : "") << destVar
      << " = vec4( 0. );" << endl;

   // Diffuse component: Lambertian reflection
   fs << "   float lambertTerm = dot( n, l );" << endl;
   fs << endl;
   fs << "   if( twoSidedLighting )" << endl;
   fs << "      if( lambertTerm < 0. ) {" << endl;
   fs << "         n = -n;" << endl;
   fs << "         lambertTerm = dot( n, l );" << endl;
   fs << "      }" << endl;
   fs << endl;
   fs << "   if( lambertTerm > 0. ) {" << endl;
   fs << endl;

   // Spot Light Term
   fs << "      float spotTerm;" << endl;
   fs << "      if( lightSource.spotCosOuterAngle == -1. )" << endl;
   fs << "         spotTerm = 1.;" << endl;
   fs << "      else {" << endl;
   fs << "         spotTerm = dot( -l, normalize( lightSource.spotDirection ));" << endl;
   fs << "         if( spotTerm < lightSource.spotCosOuterAngle )" << endl;
   fs << "            spotTerm = 0.;" << endl;
   fs << "         else" << endl;
   fs << "            if( lightSource.spotCosInnerAngle == -1. ) // OpenGL spotlight" << endl;
   fs << "               spotTerm = pow( spotTerm, lightSource.spotExponent );" << endl;
   fs << "            else // DX style spotlight" << endl;
   fs << "               spotTerm = pow( clamp( (spotTerm - lightSource.spotCosOuterAngle) /" << endl;
   fs << "                                      (lightSource.spotCosInnerAngle - "
                                                "lightSource.spotCosOuterAngle), 0., 1. )," << endl;
   fs << "                               lightSource.spotExponent );" << endl;
   fs << "      }" << endl;
   fs << endl;
   fs << "      if( spotTerm != 0. ) {" << endl;
   fs << endl;

   // Specular component: Phong reflection
   // variables: r - reflected vector
   fs << "         float specularTerm;" << endl;
   fs << "         vec3 r = reflect( -l, n );" << endl;
   fs << "         float rDotMV = dot( r, normalize( -v ) );" << endl;
   fs << "         if( rDotMV > 0. )" << endl;
   fs << "            specularTerm = pow( rDotMV, shininess );" << endl;
   fs << "         else" << endl;
   fs << "            specularTerm = 0.;" << endl;
   fs << endl;

   // attenuation
   fs << "         float attenuation = 1.;" << endl;
   fs << "         if (lightSource.position.w != 0.)" << endl;
   fs << "            attenuation /=" << endl;
   fs << "                  lightSource.constantAttenuation +" << endl;
   fs << "                  lightSource.linearAttenuation * ld +" << endl;
   fs << "                  lightSource.quadraticAttenuation * ld * ld;" << endl;
   fs << endl;

   // color sum
   fs << "         " << destVar << " =" << endl;
   fs << "               (ambient  * lightSource.ambient  +" << endl;
   fs << "                diffuse  * lightSource.diffuse  * lambertTerm +" << endl;
   fs << "                specular * lightSource.specular * specularTerm) *" << endl;
   fs << "                   spotTerm * attenuation;" << endl;
   fs << "      }" << endl;
   fs << "   }" << endl;
   fs << endl;

   // function footer
   if( makeFunction ) {
      fs << "   return " << destVar << ";" << endl;
      fs << "}" << endl;
      fs << endl;
   }

   if( prefix != "lightSource." ) {
      string::size_type newLength = prefix.size();
      string::size_type oldLength = string( "lightSource." ).size();
      string s = fs.str();
      if( replaceStart < s.size() ) {
         while( true ) {
            replaceStart = s.find( "lightSource.", replaceStart );
            if( replaceStart == string::npos )
               break;
            if( oldLength >= newLength ) {
               s.replace( replaceStart, newLength, prefix );
               s.erase( replaceStart+newLength, oldLength-newLength );
            } else {
               s.replace( replaceStart, oldLength, prefix );
               s.insert( replaceStart+oldLength, prefix, oldLength, newLength-oldLength );
            }
            replaceStart += newLength;
         }
      }
      fs.str( s );
      fs.seekp( s.size() );
   }
}


/**
 * The function creates the fragment shader main function.
 *
 * The function's parameter fs is stringstream to which the shader's
 * body will be appended.
 * baseTextureUniform specifies the base texture of the geometry.
 * It is the name of the texture's uniform. If the geometry
 * does not have a texture, empty string should be passed in.
 * baseTextureMode is texture mode of base texture, such as MODULATE, REPLACE,
 * DECAL, or BLEND.
 * lights is vector of bools that specifies whether particular OpenGL light
 * is on or off. There is no code generated for the lights switched off.
 * shadowTextureUniform is vector of strings. Each string corresponds with
 * one OpenGL light and it specifies uniform name of the shadow texture
 * assigned to the particular light. If the uniform name is empty,
 * the light does not have shadow texture assigned. If the light is off,
 * the uniform name of the light is ignored.
 * appendGlobalAmbient set to false disables appending of global ambient light
 * to the particular fragment. This is useful, for example, for multipass
 * algorithms to avoid multiple use of the ambient light.
 * compatibilityParams determines whether safe way of passing parameters
 * to the light computation function should be used. See details in comments
 * of createFragmentShaderLightingCode function.
 */
static void createFragmentShaderBody( stringstream &fs,
          const string& baseTextureUniform = "",
          TexEnv::Mode baseTextureMode = TexEnv::MODULATE,
          const vector< bool >& lights = vector< bool >(),
          int shadowMapTexUnit = -1,
          bool cubeShadowMap = false,
          const vector< string >& shadowTextureUniform = vector< string >(),
//          const vector< int >& shadowTexCoordIndex = vector< int >(),
          bool appendGlobalAmbient = true,
          bool compatibilityParams = true )
{
   // Floating numbers issue
   //
   // .f is not allowed in GLSL 1.1!!!
   // and is optional from 1.2
   // If we want to support GLSL 1.1, we must not use .f!


   // Array Indexing Problem
   //
   // Indexing to an array by non-constant integer does not work
   // on some possibly old ATI cards. It seems that the indexing
   // is not implemented in hardware in R500 and pre-R500 cards.
   // Some hints indicate that old Catalyst drivers on a new ATI
   // card can probably cause the same problem (need to be investigated).
   //
   // Example code:
   //   int i =2;
   //   const float[5] a = float[5](0.1,0.2,0.3,0.4,0.5);
   //   float zzz = a[i];
   //
   // This was seen on Mobile Radeon X1300M on Linux. People were
   // reporting the same problem on internet with Mobile Radeon
   // X1600 and Radeon HD4850 (Catalyst 9.2 and previous).
   // More info at:
   // http://www.gamedev.net/community/forums/topic.asp?topic_id=476879&whichpage=1&#3126230
   // http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=254240
   // http://www.gamedev.net/community/forums/topic.asp?topic_id=206242

   // has active lights?
   bool hasLights = false;
   for( unsigned int i=0; i<lights.size(); i++ ) {
      if( lights[i] == true ) {
         hasLights = true;
         break;
      }
   }

   // main function
   fs << "void main()" << endl;
   fs << "{" << endl;
   if( hasLights ) {
      fs << "   vec3 v = normalize( vertex.xyz );" << endl;
      fs << "   vec3 n = normalize( normal );" << endl;
      fs << endl;
   }

   // global ambient light
   if( appendGlobalAmbient ) {
      fs << "   gl_FragColor = vec4( gl_Color.rgb + // emission material component" << endl;
      fs << "                        (ambient.rgb * gl_LightModel.ambient.rgb), // global amb." << endl;
      fs << "                        diffuse.a ); // alpha" << endl;
   } else
      fs << "   gl_FragColor = vec4( 0., 0., 0., diffuse.a );" << endl;
   fs << endl;

   // lights and shadows
   for( unsigned int i=0; i<lights.size(); i++ ) {

      // ignore lights that are switched off
      if( lights[i] == false )
         continue;

      // has the light shadowMap?
      const string& shadowTexture = ( i < shadowTextureUniform.size() ?
                                      shadowTextureUniform[i] : "" );

      // do shadowMap lookup
      string shadowVar( shadowTexture + "Color" );
      if( !shadowTexture.empty() ) {
         assert( shadowMapTexUnit != -1 && "shadowMapTexUnit is -1 while there are shadowMap texture uniforms." );
         if( !cubeShadowMap )
         {
         fs << "   vec4 " << shadowVar << " = shadow" << (cubeShadowMap ? "Cube" : "2D") << "Proj( " << shadowTexture << ", ";
         if( cubeShadowMap )  fs << "vertex.xyz - gl_LightSource[" << i << "].position.xyz";
         else  fs << "gl_TexCoord[" << shadowMapTexUnit << "]";
         fs << " );" << endl;
         }
         else fs<<"   vec4 " << shadowVar << " = vec4(0,0,0,1);" << endl;
      }

      // lit fragment by the light source i
      //
      // Warning: it seems that normals are sometimes inverted for unknown
      // reasons although model seems correct. PCJohn-2009-12-17
      //
      // note on compatibilityParams: some old drivers has problems with
      // gl_LightSource as a parameter. For more details, see comments in
      // createFragmentShaderLightingCode function.
      //
      if( !compatibilityParams )
         fs << "   gl_FragColor.rgb += processLight( gl_LightSource[" << i
            << "], v, n, false ).rgb";
      else
         fs << "   gl_FragColor.rgb += processLight(" << endl
            << "          gl_LightSource[" << i << "].ambient," << endl
            << "          gl_LightSource[" << i << "].diffuse," << endl
            << "          vec4(gl_LightSource[" << i << "].specular.xyz, gl_LightSource[" << i << "].diffuse.w)," << endl
            << "          gl_LightSource[" << i << "].position," << endl
            << "          gl_LightSource[" << i << "].spotDirection," << endl
#if 0 // Switch between OpenGL spotlight (first code) and DirectX spotlight (second code).
            << "          gl_LightSource[" << i << "].spotExponent," << endl
            << "          gl_LightSource[" << i << "].spotCosCutoff," << endl
            << "          -1," << endl
#else
            << "          gl_LightSource[" << i << "].spotExponent," << endl
            << "          gl_LightSource[" << i << "].spotCosCutoff, // outer cone" << endl // Outer cone radius
            << "          gl_LightSource[" << i << "].specular.w, // inner cone" << endl // Inner cone radius
#endif
            << "          gl_LightSource[" << i << "].constantAttenuation," << endl
            << "          gl_LightSource[" << i << "].linearAttenuation," << endl
            << "          gl_LightSource[" << i << "].quadraticAttenuation," << endl
            << "          v, n, false ).rgb";

      // apply shadow
      if( !shadowTexture.empty() )
         fs << " * " << shadowVar << ";" << endl;
      else
         fs << ";" << endl;
   }

   if( !baseTextureUniform.empty() ) {

      // look up texture
      fs << "   vec4 texColor = texture2D( " << baseTextureUniform
         << ", gl_TexCoord[0].st );" << endl;

      // apply texture environment
      switch( baseTextureMode ) {

      case TexEnv::MODULATE:
         fs << "   gl_FragColor *= texColor; // modulate" << endl;
         break;

      case TexEnv::REPLACE:
         fs << "   gl_FragColor = texColor; // replace" << endl;
         break;

      case TexEnv::DECAL:
         fs << "   gl_FragColor = texColor;" << endl;
         break;

      case TexEnv::BLEND:
         fs << "   gl_FragColor *= texColor;" << endl; // FIXME: blend mode is not
                                                       // implemented properly
         break;

      default:
         notify( WARN ) << "PerPixelLighting::ShaderGenerator warning:" << endl
                             << "Unknown Texture Environment, ignoring texture" << endl;
      }
   }
   fs << "}" << endl;
}


/**
 * The function creates the fragment shader main function.
 *
 * The function's parameter fs is stringstream to which the shader's
 * body will be appended.
 * baseTextureUniform specifies the base texture of the geometry.
 * It is the name of the texture's uniform. If the geometry
 * does not have a texture, empty string should be passed in.
 * baseTextureMode is texture mode of base texture, such as MODULATE, REPLACE,
 * DECAL, or BLEND.
 * lights is vector of bools that specifies whether particular OpenGL light
 * is on or off. There is no code generated for the lights switched off.
 * shadowTextureUniform is vector of strings. Each string corresponds with
 * one OpenGL light and it specifies uniform name of the shadow texture
 * assigned to the particular light. If the uniform name is empty,
 * the light does not have shadow texture assigned. If the light is off,
 * the uniform name of the light is ignored.
 * appendGlobalAmbient set to false disables appending of global ambient light
 * to the particular fragment. This is useful, for example, for multipass
 * algorithms to avoid multiple use of the ambient light.
 * compatibilityParams determines whether safe way of passing parameters
 * to the light computation function should be used. See details in comments
 * of createFragmentShaderLightingCode function.
 */
Shader* PerPixelLighting::ShaderGenerator::createFragmentShader(
          const FragmentShaderParams &fsp )
{
   notify( INFO )
             << "PerPixelLighting::ShaderGenerator: "
             << "Creating FRAGMENT shader with params:" << endl
             << fsp << endl;


   stringstream code;

   createFragmentShaderHeader( code,
          fsp.baseTextureUniform,
          fsp.shadowTextureUniforms,
          fsp.cubeShadowMap );

#if 1
   if( fsp.lights.size() > 0 )
      createFragmentShaderLightingCode( code,
            true, // makeFunction
            "",   // destVariable
            fsp.compatibilityParams,
            "" ); // paramPrefix
#else
#endif

   createFragmentShaderBody( code,
          fsp.baseTextureUniform,
          fsp.baseTextureMode,
          fsp.lights,
          fsp.shadowMapTexUnit,
          fsp.cubeShadowMap,
          fsp.shadowTextureUniforms,
          fsp.appendGlobalAmbient,
          fsp.compatibilityParams );

   // fragment shader object instance
   Shader *fragmentShader = new Shader( Shader::FRAGMENT );
   fragmentShader->setShaderSource( code.str().c_str() );
   return fragmentShader;
}




bool PerPixelLighting::ShaderGenerator::hasTexture2D( const StateSet *s,
                                                      unsigned int unit )
{
   return s->getTextureMode( unit, GL_TEXTURE_2D ) != 0 &&
          s->getTextureAttribute( unit, StateAttribute::TEXTURE ) != NULL;
}


// FIXME: this is temporary hack until we move from OSG 2.8.2 to a new version
class TextureShadowComparisonHack : public Texture {
public: bool getShadowComparison() const { return _use_shadow_comparison; }
};

bool PerPixelLighting::ShaderGenerator::isShadowTexture(
       const StateSet *s, unsigned int unit )
{
   const Texture *t = dynamic_cast< const Texture* >(
         s->getTextureAttribute( unit, StateAttribute::TEXTURE ) );
   return ((const TextureShadowComparisonHack*)t)->getShadowComparison();
}


TexEnv::Mode PerPixelLighting::ShaderGenerator::getTexEnv(
       const StateSet *s, unsigned int unit )
{
   const TexEnv *te = dynamic_cast< const TexEnv* >(
         s->getTextureAttribute( unit, StateAttribute::TEXENV ) );
   return te ? te->getMode() : TexEnv::Mode( 0 );
}


PerPixelLighting::ShaderGenerator::VertexShaderParams::VertexShaderParams(
          bool texCoords, int shadowMapTextureUnit )
{
   this->texCoords = texCoords;
   this->shadowMapTexUnit = shadowMapTextureUnit;
}


PerPixelLighting::ShaderGenerator::VertexShaderParams::VertexShaderParams(
          const StateSet *s, int shadowMapTextureUnit )
{
   texCoords = PerPixelLighting::ShaderGenerator::hasTexture2D( s, 0 );
   shadowMapTexUnit = shadowMapTextureUnit;
}


std::ostream& operator<<( std::ostream& out,
          const PerPixelLighting::ShaderGenerator::VertexShaderParams& vsp )
{
   out << "      TexCoords: " << ( vsp.texCoords ? "y" : "n" ) << endl
       << "      ShadowMapTexUnit: ";
   if( vsp.shadowMapTexUnit == -1 )  out << "none";
   else  out << vsp.shadowMapTexUnit;
   return out;
}


Shader* PerPixelLighting::ShaderGenerator::getVertexShader(
          const VertexShaderParams& vsp )
{
   // try to re-use one of existing vertex shaders
   // that were already created by this ShaderGenerator
   std::map< VertexShaderParams, ref_ptr< Shader > >::iterator it;
   it = vertexShaders.find( vsp );
   if( it != vertexShaders.end() ) {
      notify( INFO )
                << "PerPixelLighting::ShaderGenerator: "
                << "Reusing VERTEX shader." << endl;
      return it->second;
   }

   // create new vertex shader
   Shader *vs = createVertexShader( vsp );
   vertexShaders[ vsp ] = vs;
   return vs;
}



PerPixelLighting::ShaderGenerator::FragmentShaderParams::FragmentShaderParams( const StateSet *s,
          int shadowMapTextureUnit, bool cubeMap, ShadowTechnique shadowTechnique, bool globalAmbient )
{
   bool hasTexture2D_0 = PerPixelLighting::ShaderGenerator::hasTexture2D( s, 0 );

   // base texture: detected from StateSet
   if( !hasTexture2D_0 )
      baseTextureUniform = "";
   else
   {
      if( shadowTechnique != SHADOW_MAPS )
         baseTextureUniform = "baseTexture";
      else
         baseTextureUniform = "osgShadow_baseTexture";
   }
   baseTextureMode = PerPixelLighting::ShaderGenerator::getTexEnv( s, 0 );

   // lights: detected from StateSet
   int lastLight = -1;
   for( int i=0; i<maxLights; i++ ) {

      // get mode
      StateAttribute::GLModeValue mode = s->getMode( GL_LIGHT0 + i );
      bool lightOn = ( mode & StateAttribute::ON );
      lights.push_back( lightOn );
      if( lightOn )
         lastLight = i;

   }

   // purge all switched off lights from the end of the list
   lights.resize( lastLight + 1 );

   // shadow map texture unit
   shadowMapTexUnit = shadowMapTextureUnit;

   // cube map
   cubeShadowMap = cubeMap;

   // set shadow map uniforms
   if( shadowMapTexUnit != -1 )
#if 0 // for multiple light support (does not work with official version of OSG yet)
      for( unsigned int i=0; i<lights.size(); i++ )
         shadowTextureUniforms.push_back(
               lights[i] ? std::string("shadowMap")+char('0'+i) : "" );
#else // compatibility code
      for( unsigned int i=0; i<lights.size(); i++ )
         if( !lights[i] )
            shadowTextureUniforms.push_back( "" );
         else {
            // use only the first light found
            if( shadowTechnique == SHADOW_MAPS )
               shadowTextureUniforms.push_back( "osgShadow_shadowTexture" );
            else
               shadowTextureUniforms.push_back( "shadowTexture" );
            break;
         }
#endif

   // global ambient
   appendGlobalAmbient = globalAmbient;

   // compatibility params
   compatibilityParams = true;
}


std::ostream& operator<<( std::ostream& out,
          const PerPixelLighting::ShaderGenerator::FragmentShaderParams& fsp )
{
   // baseTextureUniform
   out << "      BaseTextureUniform: \"" << fsp.baseTextureUniform << "\"" << endl;

   // baseTextureMode
   out << "      BaseTextureMode: ";
   switch( fsp.baseTextureMode ) {
   case TexEnv::MODULATE: out << "MODULATE"; break;
   case TexEnv::REPLACE:  out << "REPLACE"; break;
   case TexEnv::DECAL:    out << "DECAL"; break;
   case TexEnv::BLEND:    out << "BLEND"; break;
   default: out << "Not set";
   }
   out << endl;

   // lights
   out << "      Active Lights IDs: ";
   bool first = true;
   unsigned int i;
   for( i=0; i<fsp.lights.size(); i++ )
      if( fsp.lights[i] )
         if( first ) {
            first = false;
            out << i;
         } else
            out << "," << i;
   if( first )
      out << "none";
   out << endl;

   // shadowMapTextureUnit
   out << "      Active ShadowMap Texture Units: ";
   if( fsp.shadowMapTexUnit == -1 )  out << "none";
   else  out << fsp.shadowMapTexUnit;
   out << endl;

   // shadowMapTextureUnit
   out << "      Use cube map: ";
   if( fsp.cubeShadowMap )  out << "y";
   else  out << "n";
   out << endl;

   // shadowTextureUniforms
   out << "      Active Shadow Textures Uniforms: ";
   first = true;
   for( i=0; i<fsp.shadowTextureUniforms.size(); i++ )
      if( !fsp.shadowTextureUniforms[i].empty() )
         if( first ) {
            first = false;
            out << endl;
            out << "         " << i << ": " << fsp.shadowTextureUniforms[i] << endl;
         } else
            out << "         " << i << ": " << fsp.shadowTextureUniforms[i] << endl;
   if( first )
      out << "none" << endl;

   // appendGlobalAmbient
   out << "      AppendGlobalAmbient: " << ( fsp.appendGlobalAmbient ? "y" : "n" ) << endl;

   // compatibilityParams
   out << "      CompatibilityParams: " << ( fsp.compatibilityParams ? "y" : "n" );

   return out;
}


Shader* PerPixelLighting::ShaderGenerator::getFragmentShader(
          const FragmentShaderParams& fsp )
{
   // try to re-use one of existing vertex shaders
   // that were already created by this ShaderGenerator
   std::map< FragmentShaderParams, ref_ptr< Shader > >::iterator it;
   it = fragmentShaders.find( fsp );
   if( it != fragmentShaders.end() ) {
      notify( INFO )
                << "PerPixelLighting::ShaderGenerator: "
                << "Reusing FRAGMENT shader." << endl;
      return it->second;
   }

   // create new vertex shader
   Shader *fs = createFragmentShader( fsp );
   fragmentShaders[ fsp ] = fs;
   return fs;
}


bool PerPixelLighting::ShaderGenerator::FragmentShaderParams::operator<(
          const FragmentShaderParams &other ) const
{
   // comparisons are sorted according to the expectation
   // of the most frequently changed variables
   if( this->lights < other.lights )
      return true;
   if( this->lights > other.lights )
      return false;
   if( this->appendGlobalAmbient < other.appendGlobalAmbient )
      return true;
   if( this->appendGlobalAmbient > other.appendGlobalAmbient )
      return false;
   if( this->shadowTextureUniforms < other.shadowTextureUniforms )
      return true;
   if( this->shadowTextureUniforms > other.shadowTextureUniforms )
      return false;
   if( this->cubeShadowMap < other.cubeShadowMap )
      return true;
   if( this->cubeShadowMap > other.cubeShadowMap )
      return false;
   if( this->baseTextureUniform < other.baseTextureUniform )
      return true;
   if( this->baseTextureUniform > other.baseTextureUniform )
      return false;
   if( this->baseTextureMode < other.baseTextureMode )
      return true;
   if( this->baseTextureMode > other.baseTextureMode )
      return false;
   return this->compatibilityParams < other.compatibilityParams;
}


/**
 * The method creates the shader program for per-pixel lighting based on
 * The shader is created according to the StateSet passed as the parameter.
 */
Program* PerPixelLighting::ShaderGenerator::createProgram(
           const VertexShaderParams& vsp, const FragmentShaderParams& fsp )
{
   Shader *vs = createVertexShader( vsp );
   Shader *fs = createFragmentShader( fsp );
   Program *pr = new Program();
   pr->addShader( vs );
   pr->addShader( fs );
   return pr;
}


Program* PerPixelLighting::ShaderGenerator::createProgram( const StateSet *s,
           int shadowMapTextureUnit, bool cubeShadowMap, ShadowTechnique shadowTechnique, bool globalAmbient )
{
   return createProgram(
         PerPixelLighting::ShaderGenerator::VertexShaderParams( s, shadowMapTextureUnit ),
         PerPixelLighting::ShaderGenerator::FragmentShaderParams( s, shadowMapTextureUnit, cubeShadowMap,
                                                                  shadowTechnique, globalAmbient ) );
}


Program* PerPixelLighting::ShaderGenerator::getProgram(
           const VertexShaderParams& vsp, const FragmentShaderParams& fsp )
{
   Shader *vs = getVertexShader( vsp );
   Shader *fs = getFragmentShader( fsp );

   // try to re-use existing shader program
   std::map< ProgramShaders, ref_ptr< Program > >::iterator it;
   it = shaderPrograms.find( ProgramShaders( vs, fs ) );
   if( it != shaderPrograms.end() )
      return it->second;

   // create new shader program
   Program *pr = new Program();
   pr->addShader( vs );
   pr->addShader( fs );
   shaderPrograms[ ProgramShaders( vs, fs ) ] = pr;
   return pr;
}


/**
 * The method creates the shader program for per-pixel lighting based on
 * The shader is created according to the StateSet passed as the parameter.
 */
Program* PerPixelLighting::ShaderGenerator::getProgram( const StateSet *s,
           int shadowMapTextureUnit, bool cubeShadowMap, ShadowTechnique shadowTechnique, bool globalAmbient )
{
   return getProgram(
         PerPixelLighting::ShaderGenerator::VertexShaderParams( s, shadowMapTextureUnit ),
         PerPixelLighting::ShaderGenerator::FragmentShaderParams( s, shadowMapTextureUnit, cubeShadowMap,
                                                                  shadowTechnique, globalAmbient ) );
}


PerPixelLighting::CollectLightVisitor::CollectLightVisitor()
      : NodeVisitor( NodeVisitor::TRAVERSE_ALL_CHILDREN )
{
}


void PerPixelLighting::CollectLightVisitor::reset()
{
   lightSourceList.clear();
}


void PerPixelLighting::CollectLightVisitor::apply( LightSource &lightSource )
{
   // get attached light
   const Light *l = lightSource.getLight();
   if( l ) {

      // append light to the list
      RefNodePathList &list = lightSourceList[ &lightSource ];
      list.push_back( new ::RefNodePath( this->getNodePath() ) );

   }

   // traverse children
   traverse( lightSource );
}


int PerPixelLighting::CollectLightVisitor::getNumLights() const
{
   int num = 0;

   for( LightSourceList::const_iterator lightSIt = lightSourceList.begin();
        lightSIt != lightSourceList.end();
        lightSIt++ )
      num += lightSIt->second.size();

   return num;
}

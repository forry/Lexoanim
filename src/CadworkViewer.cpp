
/**
 * @file
 * CadworkViewer class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */

#include <osgViewer/ViewerEventHandlers>
#include "CadworkViewer.h"
#include "gui/CadworkOrbitManipulator.h"
#include "gui/CadworkFirstPersonManipulator.h"
#include "lighting/ShadowVolume.h"

using namespace osg;


/** Camera draw callback performed before the camera children rendering. */
struct MyInitialDrawCallback : public Camera::DrawCallback
{
   /// Functor method called by the rendering thread.
   virtual void operator () (osg::RenderInfo& renderInfo) const;

   /// Time of the last frame rendering start.
   mutable osg::Timer _frameStartTime;

   /// Callbacks called one time.
   struct OneTimeCallback {
      void (*func)( void* );
      void* data;
      OneTimeCallback( void (*func)( void* ), void* data )  { this->func = func; this->data = data; }
   };
   typedef std::list< OneTimeCallback > OneTimeCallbacks;
   mutable OneTimeCallbacks oneTimeCallbacks;

   /// Munipulating routines for callbacks
   static OpenThreads::Mutex mutex;
   void appendOneTimeOpenGLCallback( void (*func)( void* data ), void* data = NULL );
};

/** Camera draw callback performed after the camera children rendering. */
struct MyFinalDrawCallback : public Camera::DrawCallback
{
   MyFinalDrawCallback( MyInitialDrawCallback *initialCallback ) : _initialCallback( initialCallback ) {}

   /// Functor method called by the rendering thread.
   virtual void operator () (osg::RenderInfo& renderInfo) const;

   /// Pointer on MyInitialDrawCallback. Used for the access to MyInitialDrawCallback::frameStartTime.
   ref_ptr< MyInitialDrawCallback > _initialCallback;
};

OpenThreads::Mutex MyInitialDrawCallback::mutex;



CadworkViewer::CadworkViewer()
   : inherited()
{
   commonConstructor();
}


CadworkViewer::CadworkViewer( osg::ArgumentParser &arguments )
   : inherited( arguments )
{
   commonConstructor();
}


void CadworkViewer::commonConstructor()
{
   // append initial and final camera callbacks
   // note: these objects are accessed from other threads
   // (remove and access them carefully)
   MyInitialDrawCallback *initialCallback = new MyInitialDrawCallback;
   getCamera()->setInitialDrawCallback( initialCallback );
   getCamera()->setFinalDrawCallback( new MyFinalDrawCallback( initialCallback ) );

   setLightingMode( osg::View::HEADLIGHT );

   // initialize manipulators
   orbitManipulator = new CadworkOrbitManipulator();
   orbitManipulator->setAutoComputeHomePosition( false );
   orbitManipulator->setAnimationTime( 0.2 );
   firstPersonManipulator = new CadworkFirstPersonManipulator();
   firstPersonManipulator->setAutoComputeHomePosition( false );
   currentManipulator = orbitManipulator;

   setCameraManipulator( currentManipulator, false ); // non-virtual method of osgViewer::View

   // initialize shadow volumes
   osgShadow::ShadowVolume::setupDisplaySettings( this );

   // StatsHandler
#if 1
   osgViewer::StatsHandler *eh = new osgViewer::StatsHandler();
   this->addEventHandler( eh );

   // show stats handler in debug
#if 0 //ifndef NDEBUG
   osg::ref_ptr< osgGA::GUIEventAdapter > keyEvent = this->getEventQueue()->createEvent();
   keyEvent->setEventType( osgGA::GUIEventAdapter::KEYDOWN );
   keyEvent->setKey( 's' );
   eh->handle( *keyEvent.get(), *this );
   eh->setStatsType( osgViewer::StatsHandler::VIEWER_STATS );
#endif
#endif
}


CadworkViewer::~CadworkViewer()
{
}


void CadworkViewer::setSceneData( osg::Node *scene, bool resetCameraPosition )
{
   // save camera transformation
   osg::Matrix savedMatrix;
   double savedDistance;
   if( !resetCameraPosition )
   {
      // save matrix
      savedMatrix = getCameraManipulator()->getMatrix();

      // save distance for OrbitManipulators
      osgGA::OrbitManipulator *orbitManipulator = dynamic_cast< osgGA::OrbitManipulator* >( getCameraManipulator() );
      if( orbitManipulator != NULL )
         savedDistance = orbitManipulator->getDistance();
   }

   // set scene data
   // note: this changes or ?resets? the camera position as a side effect
   inherited::setSceneData( scene );

   // restore saved camera transformation
   if( !resetCameraPosition )
   {
      // restore distance for OrbitManipulators
      osgGA::OrbitManipulator *orbitManipulator = dynamic_cast< osgGA::OrbitManipulator* >( getCameraManipulator() );
      if( orbitManipulator != NULL )
         orbitManipulator->setDistance( savedDistance );

      // restore matrix
      getCameraManipulator()->setByMatrix( savedMatrix );
   }

   // set new scene for manipulators
   // (maybe, this is not necessary)
   orbitManipulator->setNode( scene );
   firstPersonManipulator->setNode( scene );

   // new home positions
   orbitManipulator->computeHomePosition( getSceneWithCamera(), true );
   firstPersonManipulator->computeHomePosition( getSceneWithCamera(), true );

   // either reset to home position or restore original position
   if( resetCameraPosition )
      currentManipulator->home( 0. );
}


/**
 * The method creates the default orbit manipulator.
 * Then, it attaches viewer's camera to the manipulator.
 */
void CadworkViewer::setOrbitManipulator()
{
   if( currentManipulator == orbitManipulator )
      return;

   // reset manipulator
   osg::ref_ptr< osgGA::GUIEventAdapter > dummy = this->getEventQueue()->createEvent();
   orbitManipulator->init( *dummy.get(), *this );

   // copy transformation
   osg::Vec3d eye;
   osg::Quat rotation;
   currentManipulator->getTransformation( eye, rotation );
   orbitManipulator->setTransformation( eye, rotation );

   // set current manipulator
   currentManipulator = orbitManipulator;
   setCameraManipulator( currentManipulator, false );  // non-virtual method of osgViewer::View
}


/**
 * The method creates the default first person manipulator
 * (osgGA::FlightManipulator is used).
 * Then, it attaches viewer's camera to the manipulator.
 */
void CadworkViewer::setFirstPersonManipulator()
{
   if( currentManipulator == firstPersonManipulator )
      return;

   // reset manipulator
   osg::ref_ptr< osgGA::GUIEventAdapter > dummy = this->getEventQueue()->createEvent();
   firstPersonManipulator->init( *dummy.get(), *this );

   // copy transformation
   osg::Vec3d eye;
   osg::Quat rotation;
   currentManipulator->getTransformation( eye, rotation );
   firstPersonManipulator->setTransformation( eye, rotation );

   // set current manipulator
   currentManipulator = firstPersonManipulator;
   setCameraManipulator( currentManipulator, false );  // non-virtual method of osgViewer::View
}


/**
 * Sets background color of the scene.
 */
void CadworkViewer::setBackgroundColor( const osg::Vec4 &color )
{
   getCamera()->setClearColor( color );
}


/**
 * Sets background color of the scene.
 */
void CadworkViewer::setBackgroundColor( const float &red, const float &green,
                                    const float &blue, const float &alpha)
{
   setBackgroundColor( osg::Vec4( red, green, blue, alpha ));
}


void MyInitialDrawCallback::operator () (osg::RenderInfo& renderInfo) const
{
   // get frame number
   unsigned int frameNumber = renderInfo.getState()->getFrameStamp()->getFrameNumber();

   // put message to log for first few frames
   if( frameNumber <= 3)
   {
      OSG_NOTICE << "Frame " << frameNumber << " rendering started." << std::endl;
   }

#if 0  // debug: print camera setup to log - make it each 100 frames
   osg::Vec3 center, pos, up;
   getCamera()->getViewMatrixAsLookAt( pos, center, up,
            getCameraManipulator()->getFusionDistanceValue() );
            // note: FusionDistanceValue is probably just a hack to get "distance"
   if( frameNumber % 100 == 3 )
      Log::always() << QString( "Camera: %1,%2,%3,  %4,%5,%6."
                                ).arg( center.x() ).arg( center.y() ).arg( center.z() )
                                 .arg( pos.x() ).arg( pos.y() ).arg( pos.z() ) << Log::endm;
#endif

   // record the start frame time
   _frameStartTime.setStartTick();

   // make a copy of one-time callbacks
   // note: we need a copy as callbacks may schedule new callbacks for the next frame
   OneTimeCallbacks callbacks;
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mutex);
      callbacks = oneTimeCallbacks;
      oneTimeCallbacks.clear();
   }

   // call callbacks
   for( OneTimeCallbacks::iterator it = callbacks.begin(); it != callbacks.end(); it++ )
      (*it->func)( it->data );
}


void MyFinalDrawCallback::operator () (osg::RenderInfo& renderInfo) const
{
   // get frame number
   unsigned int frameNumber = renderInfo.getState()->getFrameStamp()->getFrameNumber();

   // put message to log for first few frames
   if( frameNumber <= 3)
   {
      if( _initialCallback.valid() )
      {
         double delta = _initialCallback->_frameStartTime.time_m();
         OSG_NOTICE << "Frame " << frameNumber << " rendering completed in "
                    << int( delta + 0.5 ) << "ms." << std::endl;
      }
      else
      {
         OSG_NOTICE << "Frame " << frameNumber << " rendering finished." << std::endl;
      }
   }
}


void MyInitialDrawCallback::appendOneTimeOpenGLCallback( void (*func)( void* data ), void* data )
{
   {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mutex);
      oneTimeCallbacks.push_back( MyInitialDrawCallback::OneTimeCallback( func, data ) );
   }
}


void CadworkViewer::appendOneTimeOpenGLCallback( void (*func)( void* data ), void* data )
{
   Camera *camera = getCamera();
   if( !camera || !camera->getInitialDrawCallback() )
   {
      OSG_WARN << "CadworkViewer::appendOneTimeOpenGLCallback(): Failed to append callback\n"
                  "   No camera or camera's initialDrawCallback attached." << std::endl;
      return;
   }

   ((MyInitialDrawCallback*)camera->getInitialDrawCallback())->appendOneTimeOpenGLCallback( func, data );
   requestRedraw();
}

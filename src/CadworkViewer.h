
/**
 * @file
 * CadworkViewer class header.
 *
 * @author PCJohn (Jan Pečiva)
 */

#ifndef CADWORK_VIEWER_H
#define CADWORK_VIEWER_H

#include <osgViewer/Viewer>

namespace osgGA {
   class StandardManipulator;
   class OrbitManipulator;
   class FirstPersonManipulator;
};


/**
 * CadworkViewer is view and scene managing class derived from osgViewer::Viewer.
 */
class CadworkViewer : public osgViewer::Viewer
{
   typedef osgViewer::Viewer inherited;

public:

   // constructor and destructor
   CadworkViewer();
   CadworkViewer( osg::ArgumentParser &arguments );
   virtual ~CadworkViewer();


   // scene data methods
   virtual void setSceneData( osg::Node *scene, bool resetCameraPosition );
   //osg::Node* getSceneData(); - implemented in osgViewer::View
   inline osg::Camera* getSceneWithCamera();
   inline const osg::Camera* getSceneWithCamera() const;
   float getModelRadius() const;

   // camera manipulator
   // notes on related important derived functions:
   //   setCameraManipulator is implemented by osgViewer::View as non-virtual method
   //   getCameraManipulator is implemented by osgViewer::View as const and non-const non-virtual method
   void setOrbitManipulator();
   void setFirstPersonManipulator();
   inline bool isOrbitManipulatorActive() const;
   inline bool isFirstPersonManipulatorActive() const;

   // background color
   void setBackgroundColor(const osg::Vec4 &color);
   void setBackgroundColor(const float &red, const float &green,
                           const float &blue, const float &alpha = 1.f);

   void appendOneTimeOpenGLCallback( void (*func)( void* data ), void* data = NULL );

protected:

   void commonConstructor();

   osgGA::StandardManipulator* currentManipulator;
   osg::ref_ptr< osgGA::OrbitManipulator > orbitManipulator;
   osg::ref_ptr< osgGA::FirstPersonManipulator > firstPersonManipulator;

};


//
//  inline methods
//

inline osg::Camera* CadworkViewer::getSceneWithCamera()  { return this->getCamera(); }
inline const osg::Camera* CadworkViewer::getSceneWithCamera() const  { return this->getCamera(); }
inline bool CadworkViewer::isOrbitManipulatorActive() const  { return getCameraManipulator() == (const osgGA::CameraManipulator*)orbitManipulator.get(); }
inline bool CadworkViewer::isFirstPersonManipulatorActive() const  { return getCameraManipulator() == (const osgGA::CameraManipulator*)firstPersonManipulator.get(); }


#endif /* CADWORK_VIEWER_H */

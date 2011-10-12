#ifndef CADWORK_MOTION_MODEL_INTERFACE
#define CADWORK_MOTION_MODEL_INTERFACE

class CadworkMotionModelInterface
{
public:
   /** for setting View (loaded from .ivv files) into Motion models corectly. */
   virtual void SetViewPosition(/*osg::Matrix*/osg::Vec3 , osg::Vec3 )=0;
};

#endif //CADWORK_MOTION_MODEL_INTERFACE
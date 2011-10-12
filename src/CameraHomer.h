/**
 * file: CameraHomer.h
 */

#ifndef CAMERA_HOMER
#define CAMERA_HOMER

#include <osg/Referenced>
#include <osg/Vec3d>

namespace dtCore{

   class Camera;

   /**
    * This class provides a functionality for computing camera
    * homing position from scene bounding box for use in 
    * motion model classes. Code is from OSG.
    */
   class CameraHomer
   {
   public:
      CameraHomer();

      virtual void ComputeHomePosition(dtCore::Camera *camera, bool useBoundingBox = true);
      virtual void GoToHomePosition() =0;

      inline virtual bool GetAutoComputeHomePosition(){ return mAutoComputeHomePosition; }
      inline virtual void SetAutoComputeHomePosition(bool v = true){  mAutoComputeHomePosition = v; }

      ~CameraHomer();

   protected:
      /** Camera home position like in OSG OrbitManipulator */
      osg::Vec3d mEye;
      osg::Vec3d mCenter;
      osg::Vec3d mUp;

      bool mAutoComputeHomePosition;
   };
}

#endif //CAMERA_HOMER
/**
 * file: CameraHomer.cpp
 */

#include "CameraHomer.h"
#include <dtCore/camera.h>
#include <osgViewer/View>
#include <osg/ComputeBoundsVisitor>

namespace dtCore{

   CameraHomer::CameraHomer()
      : mAutoComputeHomePosition(true)
   {}

   CameraHomer::~CameraHomer()
   {
   }

   void CameraHomer::ComputeHomePosition(Camera *camera, bool useBoundingBox)
   {
      osgViewer::View *view = dynamic_cast<osgViewer::View *> (camera->GetOSGCamera()->getView());
      if(view)
      {
         osg::Node *sceneData = view->getSceneData();
         if(sceneData)
         {
            //now we can compute bounding box
            osg::BoundingSphere boundingSphere;

           //Log::notice()<<" CameraManipulator::computeHomePosition("<<camera<<", "<<mUseBoundingBox<<")"<<std::endl;

           if (useBoundingBox)
           {
               // compute bounding box
               // (bounding box computes model center more precisely than bounding sphere)
               osg::ComputeBoundsVisitor cbVisitor;
               sceneData->accept(cbVisitor);
               osg::BoundingBox &bb = cbVisitor.getBoundingBox();

               if (bb.valid()) boundingSphere.expandBy(bb);
               else boundingSphere = sceneData->getBound();
           }
           else
           {
               // compute bounding sphere
               boundingSphere = sceneData->getBound();
           }

           //Log::notice()<<"    boundingSphere.center() = ("<<boundingSphere.center()<<")"<<std::endl;
           //Log::notice()<<"    boundingSphere.radius() = "<<boundingSphere.radius()<<std::endl;

           // set dist to default
           double dist = 3.5f * boundingSphere.radius();

           if (camera)
           {

               // try to compute dist from frustrum
               double left,right,bottom,top,zNear,zFar;
               if (camera->GetFrustum(left,right,bottom,top,zNear,zFar))
               {
                   double vertical2 = fabs(right - left) / zNear / 2.;
                   double horizontal2 = fabs(top - bottom) / zNear / 2.;
                   double dim = horizontal2 < vertical2 ? horizontal2 : vertical2;
                   double viewAngle = atan2(dim,1.);
                   dist = boundingSphere.radius() / sin(viewAngle);
               }
               else
               {
                   // try to compute dist from ortho
                   if (camera->GetOrtho(left,right,bottom,top,zNear,zFar))
                   {
                       dist = fabs(zFar - zNear) / 2.;
                   }
               }
           }

           mEye = boundingSphere.center() + osg::Vec3d(0.0,-dist,0.0f);
           mCenter = boundingSphere.center();
           mUp = osg::Vec3d(0.0f,0.0f,1.0f);
         }
      }
   }

}
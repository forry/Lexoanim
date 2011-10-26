#include <prefix/dtcoreprefix.h>
#include <dtCore/keyboard.h>
#include <dtCore/logicalinputdevice.h>
#include <dtCore/camera.h>
#include <dtCore/deltawin.h>
#include <dtCore/system.h>
#include "cadworkorbitmotionmodel.h"
#include <dtCore/transformable.h>
#include <dtCore/transform.h>
#include <dtUtil/matrixutil.h>

#include <osg/Vec3>
#include <osg/Matrix>
#include <osgViewer/View>
#include <osg/ComputeBoundsVisitor>
#include <osg/Camera>

#include "Log.h"
#include <osg/io_utils>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>

namespace dtCore
{
   
IMPLEMENT_MANAGEMENT_LAYER(CadworkOrbitMotionModel)

   CadworkOrbitMotionModel::CadworkOrbitMotionModel(Keyboard* keyboard,
                                   Mouse* mouse, Scene* scene,
                                   bool mAutoComputeHomePosition,
                                   bool mUseBoundingBox)
   : dtCore::OrbitMotionModel(keyboard, mouse)
   , mMouse(mouse)
   , mKeyboard(keyboard)
   , mLineIntersector(new osgUtil::LineSegmentIntersector(osg::Vec3(0,0,0),osg::Vec3(0,0,0)))
   , mScene(scene)
   , mLinearRate(10.0f)
{
   RegisterInstance(this);
   AddSender(&dtCore::System::GetInstance());
   mIntersectionVisitor = new osgUtil::IntersectionVisitor(mLineIntersector.get());

   if (keyboard != NULL && mouse != NULL)
   {
      SetDefaultMappings(keyboard, mouse);
   }
}

/**
 * Destructor.
 */
CadworkOrbitMotionModel::~CadworkOrbitMotionModel()
{
   DeregisterInstance(this);
}

void CadworkOrbitMotionModel::SetDefaultMappings(Keyboard* keyboard, Mouse* mouse)
{
   if (!mDefaultInputDevice.valid())
   {
      mDefaultInputDevice = new LogicalInputDevice;

      mLeftButtonUpDownMapping = new ButtonAxisToAxis(mouse->GetButton(Mouse::LeftButton),
                                                      mouse->GetAxis(1));

      mLeftButtonLeftRightMapping = new ButtonAxisToAxis(mouse->GetButton(Mouse::LeftButton),
                                                         mouse->GetAxis(0));
      //mDefaultAzimuthAxis = mDefaultInputDevice->AddAxis("left mouse button left/right",
      //                                                   mLeftButtonLeftRightMapping.get());

      mMiddleButtonUpDownMapping = new ButtonAxisToAxis(mouse->GetButton(Mouse::MiddleButton),
                                                        mouse->GetAxis(1));
      mMiddleButtonLeftRightMapping = new ButtonAxisToAxis(mouse->GetButton(Mouse::MiddleButton),
                                                        mouse->GetAxis(0));
      //mDefaultDistanceAxis = mDefaultInputDevice->AddAxis("middle mouse button up/down",
      //                                                    mMiddleButtonUpDownMapping.get());

      mRightButtonUpDownMapping = new ButtonAxisToAxis(mouse->GetButton(Mouse::RightButton),
                                                       mouse->GetAxis(1));
      /*mDefaultUpDownTranslationAxis = mDefaultInputDevice->AddAxis("right mouse button up/down",
                                                                   mRightButtonUpDownMapping.get());
*/
      mRightButtonLeftRightMapping = new ButtonAxisToAxis(mouse->GetButton(Mouse::RightButton),
                                                          mouse->GetAxis(0));
     /* mDefaultLeftRightTranslationAxis = mDefaultInputDevice->AddAxis("right mouse button left/right",
                                                                      mRightButtonLeftRightMapping.get());*/
      mMouseWheelUpDownMapping = new AxisToAxis(mouse->GetAxis(2),0.05,0.0);
   }
   else
   {
      mLeftButtonUpDownMapping->SetSourceButton(mouse->GetButton(Mouse::LeftButton));
      mLeftButtonUpDownMapping->SetSourceAxis(mouse->GetAxis(1));

      mLeftButtonLeftRightMapping->SetSourceButton(mouse->GetButton(Mouse::LeftButton));
      mLeftButtonLeftRightMapping->SetSourceAxis(mouse->GetAxis(0));

      mRightButtonUpDownMapping->SetSourceButton(mouse->GetButton(Mouse::RightButton));
      mRightButtonUpDownMapping->SetSourceAxis(mouse->GetAxis(1));

      mRightButtonLeftRightMapping->SetSourceButton(mouse->GetButton(Mouse::RightButton));
      mRightButtonLeftRightMapping->SetSourceAxis(mouse->GetAxis(0));

      mMiddleButtonUpDownMapping->SetSourceButton(mouse->GetButton(Mouse::MiddleButton));
      mMiddleButtonUpDownMapping->SetSourceAxis(mouse->GetAxis(1));

      mMiddleButtonLeftRightMapping->SetSourceButton(mouse->GetButton(Mouse::MiddleButton));
      mMiddleButtonLeftRightMapping->SetSourceAxis(mouse->GetAxis(1));

      mMouseWheelUpDownMapping->SetSourceAxis(mouse->GetAxis(2));
      mMouseWheelUpDownMapping->SetTransformationParameters(0.05,0.0);
   }

   SetDistanceAxis(mDefaultInputDevice->AddAxis("mouse wheel camera zoom",mMouseWheelUpDownMapping.get()));
   SetElevationAxis(mDefaultInputDevice->AddAxis("left/right mouse button + up/down",
         new dtCore::AxesToAxis(
            mDefaultInputDevice->AddAxis(
               "left mouse button + up/down",
               new dtCore::ButtonAxisToAxis(
                  mouse->GetButton(Mouse::LeftButton),
                  mouse->GetAxis(1)
               )
            ),
            mDefaultInputDevice->AddAxis(
               "right mouse button + up/down",
               new dtCore::ButtonAxisToAxis(
                  mouse->GetButton(Mouse::RightButton),
                  mouse->GetAxis(1)
               )
            )
         )
   ));
   SetAzimuthAxis(mDefaultInputDevice->AddAxis("left/right mouse button + left/right",
         new dtCore::AxesToAxis(
            mDefaultInputDevice->AddAxis(
               "left mouse button + left/right",
               new dtCore::ButtonAxisToAxis(
                  mouse->GetButton(Mouse::LeftButton),
                  mouse->GetAxis(0)
               )
            ),
            mDefaultInputDevice->AddAxis(
               "right mouse button + left/right",
               new dtCore::ButtonAxisToAxis(
                  mouse->GetButton(Mouse::RightButton),
                  mouse->GetAxis(0)
               )
            )
         )
   ));
   SetLeftRightTranslationAxis(mDefaultInputDevice->AddAxis("middle mouse button + left/right",mMiddleButtonLeftRightMapping.get()));

   SetUpDownTranslationAxis(mDefaultInputDevice->AddAxis("middle mouse button + up/down",mMiddleButtonUpDownMapping.get()));

   /** Seting up the OSG functionality: When space is pressed the camera resets to homing position (computed as in OSG) */
   SetHomingButton(mDefaultInputDevice->AddButton("Homing button", keyboard->GetButton(' '), ' '));
   SetLookAtCenterButton(mDefaultInputDevice->AddButton("Points camera to center of the scene", keyboard->GetButton('l'), 'l'));
}

/**
 * Sets the target of this motion model. Then if target is a camera
 * and the computeHomePos is true, it coumputes and sets new position
 * and orientation for the camera.
 *
 * @param target the new target
 */
void  CadworkOrbitMotionModel::SetTarget(Transformable* target, bool computeHomePos)
{
   OrbitMotionModel::SetTarget(target);
   if(target != NULL) // we're back in control, should recompute distance from current position
   {
      //when new target has been provided, it tries to pick a new focal point from center of the screen
      SSPick(0.0,0.0);
      if(mLineIntersector->containsIntersections())//we picked something
      {
         osg::Vec3 hitpoint;
         Transform trans;
         GetTarget()->GetTransform(trans);
         hitpoint = mLineIntersector->getFirstIntersection().getWorldIntersectPoint();
         SetDistance((trans.GetTranslation() - hitpoint).length());

      }
   }
   dtCore::Camera *cam = dynamic_cast<dtCore::Camera *> (target);
   if(cam && computeHomePos)
   { 
      ComputeHomePosition(cam);
      GoToHomePosition();
   }
}

void CadworkOrbitMotionModel::GoToHomePosition()
{
   dtCore::Camera *cam = dynamic_cast<dtCore::Camera *> (GetTarget());
   if(cam)
   {
      
      double dist = (mCenter - mEye).length();
      Transform trans;
      trans.SetTranslation(mEye);
      SetDistance(dist);
      SetFocalPoint(mCenter);
      GetTarget()->SetTransform(trans);

   }
}

void CadworkOrbitMotionModel::SetHomingButton(dtCore::Button *butt){
   if (mHomingButton.valid())
   {
      mHomingButton->RemoveButtonHandler(this);
   }

   mHomingButton = butt;

   if (mHomingButton.valid())
   {
      mHomingButton->AddButtonHandler(this);
   }
}

void CadworkOrbitMotionModel::SetLookAtCenterButton(dtCore::Button *butt){
   if (mLookAtCenterButton.valid())
   {
      mLookAtCenterButton->RemoveButtonHandler(this);
   }

   mLookAtCenterButton = butt;

   if (mLookAtCenterButton.valid())
   {
      mLookAtCenterButton->AddButtonHandler(this);
   }
}

float CadworkOrbitMotionModel::CMMI_GetDistance() 
{
   return OrbitMotionModel::GetDistance();
}

void CadworkOrbitMotionModel::CMMI_SetDistance(float distance)
{ 
   SetDistance(distance);
}
/**
 * Sets ne camera center via look at. It doesn't move the camera but
 * instead it turns it around its axis.
 * @param lookAt new point to look at.
 */
void CadworkOrbitMotionModel::SetCenterPoint(osg::Vec3& lookAt)
{
   Transform trans;
   GetTarget()->GetTransform(trans);
   trans.Set(trans.GetTranslation(), lookAt, osg::Vec3(0.0,0.0,1.0)/*trans.GetUpVector()*/);
   GetTarget()->SetTransform(trans);
}

/**
 * Get what would be a distance from focal point after zoom.
 * It's used as a target distance for smooth interpolation
 * when zooming (by mouse wheel).
 *
 * @param delta - delta parametr from HandleAxisStateChanged.
 */
float CadworkOrbitMotionModel::GetDistanceAfterZoom(double delta)
{
   delta = delta * (double)GetMouseSensitivity();
   float dist = GetDistance();
   float distDelta = -float(delta * dist * mLinearRate);
   if(delta < 0) // backward motion
   {
      //distDelta += (mHomePosition.eye - mHomePosition.center).length()/20.0;
      //distDelta *= 1.2;
      distDelta = -float(delta * (dist/(1-(-delta* mLinearRate))) * mLinearRate);
   }
   if (dist + distDelta < MIN_DISTANCE)
   {
      distDelta = MIN_DISTANCE - dist;
   }
   return GetDistance()+distDelta;
}

bool CadworkOrbitMotionModel::HandleAxisStateChanged(const Axis* axis,
                                        double oldState,
                                        double newState,
                                        double delta)
{
   if (GetTarget() != 0 && IsEnabled())
   {
      
      if (axis == GetDistanceAxis())
      {


         dtCore::Camera *camera = dynamic_cast<dtCore::Camera *> (GetTarget());
         if(camera)
         {
            if(delta>0) //zoom in we want to focus on point uder cursor
            {
               osg::Vec3 nearPoint,farPoint,hitPoint,startingFocal; //hitpoint will be our new center
               //mouse position
               float x=0, y=0;
               //float win_x, win_y;
               GetMouse()->GetPosition(x,y);
               //camera->GetWindow()->CalcPixelCoords(x,y,win_x, win_y);

               //animation start data
               osg::Matrix rotmat;
               osg::Vec3 xyz; // for computing new distance from new center point got by picking
               Transform trans;
               camera->GetTransform(trans);
               trans.GetRotation(rotmat);
               trans.GetTranslation(xyz);
               mAnimData._fromRotation = rotmat.getRotate(); //get starting rotation
               startingFocal = GetFocalPoint(); //storing focal point for later restore

               mAnimData._fromCursor.set(x,y);
               mAnimData._toCursor.set(0.0,0.0);

               //actual picking
               
               SSPick(x,y);
               if(mLineIntersector->containsIntersections())
               {
                  hitPoint = mLineIntersector->getFirstIntersection().getWorldIntersectPoint();
                  SetCenterPoint(hitPoint);
                  mNewCenter = hitPoint;
                  mDistanceShouldChange = true;
                  camera->GetTransform(trans);
                  trans.GetRotation(rotmat);
                  mAnimData._toRotation = rotmat.getRotate();//get ending rotation
                  mAnimData._isRotating = true; //set zooming flag
               


                  /** Now the hitpoint is our new center so we need to recalculate current distance
                      for zooming and other purposses. */
                  SetDistance( (xyz - hitPoint).length());
                  mAnimData._fromDist = GetDistance();
                  mAnimData._toDist = GetDistanceAfterZoom(delta);
                  mAnimData._isZooming = true;

                  mAnimData._startTime = System::GetInstance().GetSimulationTime();

                  SetCenterPoint(startingFocal);//reset position to starting one
                  //centerMousePointer();
               }
               else
               { /** No point hit by ray, so we have our cursor pointing outside
                     the mode, but we still want to look and zoom in that direction. */

                  mDistanceShouldChange = false;
                  mStoredDistance = GetDistance();
                  osg::Matrix VPW =camera->GetOSGCamera()->getViewMatrix() * 
                  camera->GetOSGCamera()->getProjectionMatrix() * 
                  camera->GetOSGCamera()->getViewport()->computeWindowMatrix();
                  osg::Matrix inverseVPW;
                  inverseVPW.invert(VPW);
                  float win_x,win_y;
                  camera->GetWindow()->CalcPixelCoords(x,y,win_x, win_y);
                  farPoint.set(win_x,win_y, 0.0f);
                  farPoint=farPoint*inverseVPW;
                  Log::notice()<<"farpoint : "<<farPoint<<Log::endm;
                  //farPoint.normalize();
                  SetCenterPoint(farPoint);
                  mNewCenter = GetFocalPoint();
                  camera->GetTransform(trans);
                  trans.GetRotation(rotmat);
                  mAnimData._toRotation = rotmat.getRotate();//get ending rotation
                  mAnimData._isRotating = true; //set zooming flag

                  mAnimData._fromDist = GetDistance();
                  mAnimData._toDist = GetDistanceAfterZoom(delta);

                  mAnimData._isZooming = true;
                  mAnimData._startTime = System::GetInstance().GetSimulationTime();

                  /*trans.SetRotation(mAnimData._fromRotation);
                  camera->SetTransform(trans);*/
                  SetCenterPoint(startingFocal);
               }
            }
            else // zooming out no rotations or change of focus point
            {
               mDistanceShouldChange = true;
               mAnimData._fromDist = GetDistance();
               mAnimData._toDist = GetDistanceAfterZoom(delta);
               mAnimData._isZooming = true;
               mAnimData._startTime = System::GetInstance().GetSimulationTime();
            }
         }
         return false;
      }
      return OrbitMotionModel::HandleAxisStateChanged(axis, oldState, newState, delta);
   }
   return false;
}

bool CadworkOrbitMotionModel::HandleButtonStateChanged(const Button* button, bool oldState, bool newState)
{
   if (GetTarget() != 0 && IsEnabled())
   {
      if(button == mHomingButton.get())
      {
         GoToHomePosition();
      }
      else if(button == mLookAtCenterButton)
      {
         SetCenterPoint(mCenter);
      }
      return true;
   }
   return false;
}

void CadworkOrbitMotionModel::OnMessage(MessageData *data)
{
   Transformable *target = GetTarget();

   //we are only interesting i pre frame messages
   if (  target == NULL
      || !IsEnabled()
      || (data->message != dtCore::System::MESSAGE_POST_EVENT_TRAVERSAL)
      )
   {
      return;
   }

   /**
    * Now some serious work. Below begins the 'animation' of the smooth camera
    * movement while zooming (by mouse wheel). First part is rotation when refocusing
    * and the second part is changing distance. Default animation duration is 0.4s.
    */
   if(mAnimData.isAnimating())
   {
      double f = (System::GetInstance().GetSimulationTime() - mAnimData._startTime) / mAnimData._motion->getDuration();
      f = f<0 ? 0 : f > 1 ? 1 : f; //clamp f into a <0,1> range
      float t;
      mAnimData._motion->getValueInNormalizedRange(f,t); //gets normalized <0,1> linear value f and turns it into non-lineary interpolated value t also normalized (the function is defined in MathMotionTemplate constructor in AnimationData definition)
      
      /** handle amooth zooming */
      if(mAnimData._isZooming)
      {
         float newdist;
         newdist = t * (mAnimData._toDist - mAnimData._fromDist) + mAnimData._fromDist;

         Transform trans;
         target->GetTransform(trans);
         if(mAnimData._isRotating) //rotating while zooming
         {
            osg::Vec3 xyz,hpr,oldhpr;
            trans.Get(xyz,oldhpr);
            trans.Set(xyz,mAnimData._toRotation);
            trans.Get(xyz,hpr);


            float deltaDist = GetDistance() - newdist;
            /*if (newdist < MIN_DISTANCE && (mAnimData._toDist < mAnimData._fromDist))
            {
               deltaDist = MIN_DISTANCE - GetDistance();
            }*/
            osg::Vec3 translation (0.0f, deltaDist, 0.0f);
            osg::Matrix mat;
            dtUtil::MatrixUtil::HprToMatrix(mat, hpr);
            translation = osg::Matrix::transform3x3(translation, mat);
            xyz += translation;
            //if(mDistanceShouldChange)
               SetDistance((mNewCenter-xyz).length());
            trans.Set(xyz,oldhpr);
         }
         else{ //zooming only
            osg::Vec3 xyz,hpr;
            trans.Get(xyz,hpr);
            float deltaDist = GetDistance() - newdist;
            osg::Vec3 translation (0.0f, deltaDist, 0.0f);
            osg::Matrix mat;
            dtUtil::MatrixUtil::HprToMatrix(mat, hpr);
            translation = osg::Matrix::transform3x3(translation, mat);
            xyz += translation;
            //if(mDistanceShouldChange)
               SetDistance(newdist);
            trans.Set(xyz,hpr);
         }
         target->SetTransform(trans);
      }
         
      if(mAnimData._isRotating)
      {
         osg::Quat newRotation;
         newRotation.slerp(f, mAnimData._fromRotation, mAnimData._toRotation);
         if(f>=1.)
         {
            newRotation = mAnimData._toRotation;
         }

         Transform trans;
         target->GetTransform(trans);
         osg::Vec3 xyz;
         trans.GetTranslation(xyz);
         trans.Set(xyz,newRotation);
         target->SetTransform(trans);

         /** cursor interpolation */
         if(mAnimData._interpolateCursor){
               float newx, newy;
               float x,y;
               mMouse->GetPosition(x,y); //get current position in case the user is moving the mouse
               float oldx, oldy;
               oldx = mAnimData._previousPhase * (mAnimData._toCursor.x() - mAnimData._fromCursor.x());
               oldy = mAnimData._previousPhase * (mAnimData._toCursor.y() - mAnimData._fromCursor.y());
               newx = f * (mAnimData._toCursor.x() - mAnimData._fromCursor.x());// + x/*mAnimData._fromCursor.x()*/;
               newy = f * (mAnimData._toCursor.y() - mAnimData._fromCursor.y());// + y/*mAnimData._fromCursor.y()*/;
               float truex, truey;
               truex = newx - oldx + x;
               truey = newy - oldy + y;

               mMouse->SetPosition(truex,truey);
         }
      }

      mAnimData._previousPhase = f;

      if(f>=1.)
      {
         /*Transform trans;
         target->GetTransform(trans);
         trans.Set(trans.GetTranslation(), mAnimData._toRotation);
         target->SetTransform(trans);*/
         mAnimData.reset(); //clear all data, set all flags on false => not animating anymore
         if(mDistanceShouldChange == false)
         {
            SetDistance(mStoredDistance);            
         }
      }
   }
}


void CadworkOrbitMotionModel::SSPick(float x, float y)
{
   //osg::Vec3 nearPoint,farPoint; //hitpoint will be our new center
   Camera *camera = dynamic_cast<Camera *>(GetTarget());
   if(!camera) return;

   //mouse position
   float win_x, win_y;
   GetMouse()->GetPosition(x,y);
   camera->GetWindow()->CalcPixelCoords(x,y,win_x, win_y);

   //computing pick line
   /*osg::Matrix VPW =camera->GetOSGCamera()->getViewMatrix() * camera->GetOSGCamera()->getProjectionMatrix() * camera->GetOSGCamera()->getViewport()->computeWindowMatrix();
   osg::Matrix inverseVPW;
   inverseVPW.invert(VPW);
   nearPoint.set(win_x,win_y, 0.0f);
   farPoint.set(win_x,win_y, 1.0f);
   nearPoint=nearPoint*inverseVPW;
   farPoint=farPoint*inverseVPW;*/


   //actual picking
   //mLineIntersector->reset();
   osg::Viewport *vp = camera->GetOSGCamera()->getViewport();
   osgUtil::LineSegmentIntersector::CoordinateFrame cf;
   if( vp ) {
        cf = osgUtil::Intersector::WINDOW;
        x *= vp->width();
        y *= vp->height();
    } else
        cf = osgUtil::Intersector::PROJECTION;
   mLineIntersector = new osgUtil::LineSegmentIntersector(cf, win_x, win_y);
   mIntersectionVisitor->setIntersector(mLineIntersector);
   //mLineIntersector->setStart(nearPoint);
   //mLineIntersector->setEnd(farPoint);
   
   //run intersection visitor
   camera->GetOSGCamera()->accept(*mIntersectionVisitor);
}

/***********CadworkMotionModelInterface**************/

/** for setting View (loaded from .ivv files) into Motion models corectly. */
void CadworkOrbitMotionModel::SetViewPosition(/*osg::Matrix modelView*/ osg::Vec3 eye, osg::Vec3 center)
{
   Transform trans;
   //osg::Vec3 eye,center, up;
   //modelView.getLookAt(eye,center,up);
   SetDistance((eye-center).length());
   //trans.SetTranslation(eye);
   trans.Set(eye,center, osg::Vec3(0.0,0.0,1.0));
   GetTarget()->SetTransform(trans);
   //SetCenterPoint(eye);
}

/***********CadworkMotionModelInterface END **************/

}//namespace dtCore
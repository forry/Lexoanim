// CadworkFlyMotionModel.cpp: Implementation of the CadworkFlyMotionModel class.
//
//////////////////////////////////////////////////////////////////////
#include <prefix/dtcoreprefix.h>
#include "cadworkflymotionmodel.h"

#include <osg/Vec3>
#include <osg/Matrix>

#include <dtCore/system.h>
#include <dtCore/logicalinputdevice.h>
#include <dtCore/mouse.h>
#include <dtCore/keyboard.h>
#include <dtCore/keyboard.h>
#include <dtCore/transform.h>

#include <dtUtil/bits.h>
#include <dtUtil/mathdefines.h>
#include <dtUtil/matrixutil.h>
#include <dtUtil/log.h>

#include <dtCore/camera.h>
#include <dtCore/deltawin.h>
#include <dtCore/button.h>

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>


using namespace dtCore;

const float CadworkFlyMotionModel::MIN_DISTANCE = 0.01f;

IMPLEMENT_MANAGEMENT_LAYER(CadworkFlyMotionModel)

//////////////////////////////////////////////////////////////////////////
CadworkFlyMotionModel::CadworkFlyMotionModel(Keyboard* keyboard, Mouse* mouse, unsigned int options)
   : MotionModel("CadworkFlyMotionModel")
   , mLeftButtonUpDownMapping(NULL)
   , mLeftButtonLeftRightMapping(NULL)
   , mRightButtonUpDownMapping(NULL)
   , mRightButtonLeftRightMapping(NULL)
   , mArrowKeysUpDownMapping(NULL)
   , mArrowKeysLeftRightMapping(NULL)
   , mWSKeysUpDownMapping(NULL)
   , mWSKeysUpDownMappingCaps(NULL)
   , mADKeysLeftRightMapping(NULL)
   , mADKeysLeftRightMappingCaps(NULL)
   , mQEKeysUpDownMapping(NULL)
   , mQEKeysUpDownMappingCaps(NULL)
   , mDefaultFlyForwardBackwardAxis(NULL)
   , mDefaultFlyLeftRightAxis(NULL)
   , mDefaultFlyUpDownAxis(NULL)
   , mDefaultTurnLeftRightAxis(NULL)
   , mDefaultTurnUpDownAxis(NULL)
   , mFlyForwardBackwardAxis(NULL)
   , mFlyLeftRightAxis(NULL)
   , mFlyUpDownAxis(NULL)
   , mTurnLeftRightAxis(NULL)
   , mTurnUpDownAxis(NULL)
   , mMaximumFlySpeed(100.0f)
   , mMaximumTurnSpeed(90.0f)
   , mMouse(mouse)
   , mOptions(options)
   , mMouseGrabbed(true)
   , mStartRotatingButton(NULL)
   , mDistanceAxis(NULL)
   , mLookAtCenterButton(NULL)
   , mRotationLRStartState(-1.0f)
   , mRotationUDStartState(-1.0f)
   , mLinearRate(10.0f)
   , mTmpPrevDistance(100.0f)
   , mLineIntersector(new osgUtil::LineSegmentIntersector(osg::Vec3(0,0,0),osg::Vec3(0,0,0)))
{

   RegisterInstance(this);
   mIntersectionVisitor = new osgUtil::IntersectionVisitor(mLineIntersector.get());

   if (keyboard != NULL && mouse != NULL)
   {
      SetDefaultMappings(keyboard, mouse);
   }

   AddSender(&System::GetInstance());
}

/**
 * Destructor.
 */
CadworkFlyMotionModel::~CadworkFlyMotionModel()
{
   RemoveSender(&System::GetInstance());

   DeregisterInstance(this);
}

/**
 * Sets the input axes to a set of default mappings for mouse
 * and keyboard.
 *
 * @param keyboard the keyboard instance
 * @param mouse the mouse instance
 */
void CadworkFlyMotionModel::SetDefaultMappings(Keyboard* keyboard, Mouse* mouse)
{
   if (!mDefaultInputDevice.valid())
   {
      mDefaultInputDevice = new LogicalInputDevice;

      Axis *leftButtonUpAndDown, *leftButtonLeftAndRight;

      //if (HasOption(OPTION_REQUIRE_MOUSE_DOWN))
      //{
      //   leftButtonUpAndDown = mDefaultInputDevice->AddAxis(
      //      "left mouse button up/down",
      //      mLeftButtonUpDownMapping = new ButtonAxisToAxis(
      //         mouse->GetButton(Mouse::LeftButton),
      //         mouse->GetAxis(1))
      //   );

      //   leftButtonLeftAndRight = mDefaultInputDevice->AddAxis(
      //      "left mouse button left/right",
      //      mLeftButtonLeftRightMapping = new ButtonAxisToAxis(
      //         mouse->GetButton(Mouse::LeftButton),
      //         mouse->GetAxis(0)
      //      )
      //   );
      //   /*leftButtonUpAndDown = mDefaultInputDevice->AddAxis(
      //      "left mouse movement up/down",
      //      new AxisToAxis(mouse->GetAxis(1))
      //   );

      //   leftButtonLeftAndRight = mDefaultInputDevice->AddAxis(
      //      "left mouse movement left/right",
      //      new AxisToAxis(mouse->GetAxis(0))
      //   );*/
      //}
      //else
      //{
      //   leftButtonUpAndDown = mDefaultInputDevice->AddAxis(
      //      "left mouse movement up/down",
      //      new AxisToAxis(mouse->GetAxis(1))
      //   );

      //   leftButtonLeftAndRight = mDefaultInputDevice->AddAxis(
      //      "left mouse movement left/right",
      //      new AxisToAxis(mouse->GetAxis(0))
      //   );
      //}

      leftButtonUpAndDown = mDefaultInputDevice->AddAxis(
         "left mouse movement up/down",
         new AxisToAxis(mouse->GetAxis(1))
      );

      leftButtonLeftAndRight = mDefaultInputDevice->AddAxis(
         "left mouse movement left/right",
         new AxisToAxis(mouse->GetAxis(0))
      );

      Axis* rightButtonUpAndDown = mDefaultInputDevice->AddAxis(
         "right mouse button up/down",
         mRightButtonUpDownMapping = new ButtonAxisToAxis(
            mouse->GetButton(Mouse::RightButton),
            mouse->GetAxis(1)
         )
      );

      Axis* rightButtonLeftAndRight = mDefaultInputDevice->AddAxis(
         "right mouse button left/right",
         mRightButtonLeftRightMapping = new ButtonAxisToAxis(
            mouse->GetButton(Mouse::RightButton),
            mouse->GetAxis(0)
         )
      );

      if (HasOption(OPTION_USE_CURSOR_KEYS))
      {
         Axis* arrowKeysUpAndDown = mDefaultInputDevice->AddAxis(
            "arrow keys up/down",
            mArrowKeysUpDownMapping = new ButtonsToAxis(
            keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Down),
            keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Up)
            )
            );

         Axis* arrowKeysLeftAndRight = mDefaultInputDevice->AddAxis(
            "arrow keys left/right",
            mArrowKeysLeftRightMapping = new ButtonsToAxis(
            keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Left),
            keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Right)
            )
            );

         mDefaultTurnLeftRightAxis = mDefaultInputDevice->AddAxis(
            "default turn left/right",
            new AxesToAxis(arrowKeysLeftAndRight, leftButtonLeftAndRight)
            );

         mDefaultTurnUpDownAxis = mDefaultInputDevice->AddAxis(
            "default turn up/down",
            new AxesToAxis(arrowKeysUpAndDown, leftButtonUpAndDown)
            );
      }
      else
      {
         mDefaultTurnLeftRightAxis = mDefaultInputDevice->AddAxis(
            "default turn left/right",
             leftButtonLeftAndRight
            );

         mDefaultTurnUpDownAxis = mDefaultInputDevice->AddAxis(
            "default turn up/down",
            leftButtonUpAndDown
            );
      }

      
      Axis* wsKeysUpAndDown = mDefaultInputDevice->AddAxis(
         "w/s keys stafe forward/back",
         mWSKeysUpDownMapping = new ButtonsToAxis(
            keyboard->GetButton('s'),
            keyboard->GetButton('w')
         )
      );

      Axis* wsKeysUpAndDownCaps = mDefaultInputDevice->AddAxis(
         "w/s keys stafe forward/back",
         mWSKeysUpDownMappingCaps = new ButtonsToAxis(
            keyboard->GetButton('S'),
            keyboard->GetButton('W')
         )
      );

      Axis* adKeysStrafeLeftAndRight = mDefaultInputDevice->AddAxis(
         "a/d keys strafe left/right",
         mADKeysLeftRightMapping = new ButtonsToAxis(
            keyboard->GetButton('a'),
            keyboard->GetButton('d')
         )
      );

      Axis* adKeysStrafeLeftAndRightCaps = mDefaultInputDevice->AddAxis(
         "a/d keys strafe left/right",
         mADKeysLeftRightMappingCaps = new ButtonsToAxis(
            keyboard->GetButton('A'),
            keyboard->GetButton('D')
         )
      );

      Axis* qeKeysFlyUpAndDown = mDefaultInputDevice->AddAxis(
         "q/e keys fly up/down",
         mQEKeysUpDownMapping = new ButtonsToAxis(
            keyboard->GetButton('q'),
            keyboard->GetButton('e')
         )
      );

      Axis* qeKeysFlyUpAndDownCaps = mDefaultInputDevice->AddAxis(
         "q/e keys fly up/down",
         mQEKeysUpDownMappingCaps = new ButtonsToAxis(
            keyboard->GetButton('Q'),
            keyboard->GetButton('E')
         )
      );

      dtCore::AxesToAxis* axesMapping = new AxesToAxis(wsKeysUpAndDown, wsKeysUpAndDownCaps);
      axesMapping->AddSourceAxis(rightButtonUpAndDown);
      mDefaultFlyForwardBackwardAxis = mDefaultInputDevice->AddAxis(
         "default fly forward/backward", axesMapping);

      axesMapping = new AxesToAxis(adKeysStrafeLeftAndRight, adKeysStrafeLeftAndRightCaps);
      axesMapping->AddSourceAxis(rightButtonLeftAndRight);
      mDefaultFlyLeftRightAxis = mDefaultInputDevice->AddAxis(
         "default fly left/right", axesMapping);

      mDefaultFlyUpDownAxis = mDefaultInputDevice->AddAxis(
         "default fly up/down",
         new AxesToAxis(qeKeysFlyUpAndDown, qeKeysFlyUpAndDownCaps)
      );

      mHomingButton = keyboard->GetButton(' ');

      mCursorReleaseButtonMapping = new ButtonsToButton(
         keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Control_L),
         keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Control_R)
         );

      /*mCursorGrabButtonMapping = new ButtonsToButton(
         mouse->GetButton(Mouse::LeftButton),
         mouse->GetButton(Mouse::RightButton)
         );*/

      mStartRotatingButtonMapping = new ButtonsToButton(
         mouse->GetButton(Mouse::LeftButton),
         mouse->GetButton(Mouse::RightButton),
         ButtonsToButton::SINGLE_BUTTON
         );

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

      mArrowKeysUpDownMapping->SetSourceButtons(
         keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Down),
         keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Up)
      );

      mArrowKeysLeftRightMapping->SetSourceButtons(
         keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Left),
         keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Right)
      );

      mWSKeysUpDownMapping->SetSourceButtons(
         keyboard->GetButton('s'),
         keyboard->GetButton('w')
      );

      mWSKeysUpDownMappingCaps->SetSourceButtons(
         keyboard->GetButton('S'),
         keyboard->GetButton('W')
      );

      mADKeysLeftRightMapping->SetSourceButtons(
         keyboard->GetButton('a'),
         keyboard->GetButton('d')
      );

      mADKeysLeftRightMappingCaps->SetSourceButtons(
         keyboard->GetButton('A'),
         keyboard->GetButton('D')
      );

      mQEKeysUpDownMapping->SetSourceButtons(
         keyboard->GetButton('q'),
         keyboard->GetButton('e')
      );

      mQEKeysUpDownMappingCaps->SetSourceButtons(
         keyboard->GetButton('Q'),
         keyboard->GetButton('E')
      );
      
      mHomingButton = keyboard->GetButton(' ');

      mCursorReleaseButtonMapping->SetFirstButton(keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Control_L));
      mCursorReleaseButtonMapping->SetSecondButton(keyboard->GetButton(osgGA::GUIEventAdapter::KEY_Control_R));

      /*mCursorGrabButtonMapping->SetFirstButton(mouse->GetButton(Mouse::LeftButton));
      mCursorGrabButtonMapping->SetSecondButton(mouse->GetButton(Mouse::RightButton));*/

      mStartRotatingButtonMapping->SetFirstButton(mouse->GetButton(Mouse::LeftButton));
      mStartRotatingButtonMapping->SetSecondButton(mouse->GetButton(Mouse::RightButton));

      mMouseWheelUpDownMapping->SetSourceAxis(mouse->GetAxis(2));
      mMouseWheelUpDownMapping->SetTransformationParameters(0.05,0.0);
   }

   SetFlyForwardBackwardAxis(mDefaultFlyForwardBackwardAxis);

   SetFlyLeftRightAxis(mDefaultFlyLeftRightAxis);

   SetFlyUpDownAxis(mDefaultFlyUpDownAxis);

   SetTurnLeftRightAxis(mDefaultTurnLeftRightAxis);

   SetTurnUpDownAxis(mDefaultTurnUpDownAxis);

   mDefaultInputDevice->AddButton("Homing button", mHomingButton, ' ');

   SetCursorReleaseButton(mDefaultInputDevice->AddButton("Cursor release button", osgGA::GUIEventAdapter::KEY_Control_L, mCursorReleaseButtonMapping));
   
   //SetCursorGrabButton(mDefaultInputDevice->AddButton("Cursor grab button", Mouse::LeftButton, mCursorGrabButtonMapping));

   SetStartRotatingButton(mDefaultInputDevice->AddButton("Rotation start", Mouse::LeftButton, mStartRotatingButtonMapping));

   SetDistanceAxis(mDefaultInputDevice->AddAxis("mouse wheel camera zoom",mMouseWheelUpDownMapping));
   
   SetLookAtCenterButton(mDefaultInputDevice->AddButton("Points camera to center of the scene", keyboard->GetButton('l'), 'l'));
}

/**
 * Sets the axis that moves the target forwards (for positive
 * values) or backwards (for negative values).
 *
 * @param flyForwardBackwardAxis the new forward/backward axis
 */
void CadworkFlyMotionModel::SetFlyForwardBackwardAxis(Axis* flyForwardBackwardAxis)
{
   mFlyForwardBackwardAxis = flyForwardBackwardAxis;
}

/**
 * Returns the axis that moves the target forwards (for positive
 * values) or backwards (for negative values).
 *
 * @return the current forward/backward axis
 */
Axis* CadworkFlyMotionModel::GetFlyForwardBackwardAxis()
{
   return mFlyForwardBackwardAxis;
}

const Axis* CadworkFlyMotionModel::GetFlyForwardBackwardAxis() const
{
   return mFlyForwardBackwardAxis;
}

////////////////////////////////////////////////////////////////////////////
void CadworkFlyMotionModel::SetFlyLeftRightAxis(Axis* flyLeftRightAxis)
{
   mFlyLeftRightAxis = flyLeftRightAxis;
}

////////////////////////////////////////////////////////////////////////////
Axis* CadworkFlyMotionModel::GetFlyLeftRightAxis()
{
   return mFlyLeftRightAxis;
}

////////////////////////////////////////////////////////////////////////////
const Axis* CadworkFlyMotionModel::GetFlyLeftRightAxis() const
{
   return mFlyLeftRightAxis;
}

////////////////////////////////////////////////////////////////////////////
void CadworkFlyMotionModel::SetFlyUpDownAxis(Axis* flyUpDownAxis)
{
   mFlyUpDownAxis = flyUpDownAxis;
}

////////////////////////////////////////////////////////////////////////////
Axis* CadworkFlyMotionModel::GetFlyUpDownAxis()
{
   return mFlyUpDownAxis;
}

////////////////////////////////////////////////////////////////////////////
const Axis* CadworkFlyMotionModel::GetFlyUpDownAxis() const
{
   return mFlyUpDownAxis;
}

////////////////////////////////////////////////////////////////////////////

/**
 * Sets the axis that turns the target left (for negative values)
 * or right (for positive values).
 *
 * @param turnLeftRightAxis the new turn left/right axis
 */
void CadworkFlyMotionModel::SetTurnLeftRightAxis(Axis* turnLeftRightAxis)
{
   if (mTurnLeftRightAxis)
   {
      mTurnLeftRightAxis->RemoveAxisHandler(this);
   }
   mTurnLeftRightAxis = turnLeftRightAxis;
   if (mTurnLeftRightAxis)
   {
      mTurnLeftRightAxis->AddAxisHandler(this);
   }
}

/**
 * Returns the axis that turns the target left (for negative values)
 * or right (for positive values).
 *
 * @return the current turn left/right axis
 */
Axis* CadworkFlyMotionModel::GetTurnLeftRightAxis()
{
   return mTurnLeftRightAxis;
}

const Axis* CadworkFlyMotionModel::GetTurnLeftRightAxis() const
{
   return mTurnLeftRightAxis;
}

/**
 * Sets the axis that turns the target up (for positive values)
 * or down (for negative values).
 *
 * @param turnUpDownAxis the new turn up/down axis
 */
void CadworkFlyMotionModel::SetTurnUpDownAxis(Axis* turnUpDownAxis)
{
   if (mTurnUpDownAxis)
   {
      mTurnUpDownAxis->RemoveAxisHandler(this);
   }
   mTurnUpDownAxis = turnUpDownAxis;
   if (mTurnUpDownAxis)
   {
      mTurnUpDownAxis->AddAxisHandler(this);
   }
}

void CadworkFlyMotionModel::SetStartRotatingButton( LogicalButton *b)
{ 
   if(mStartRotatingButton)
   {
      mStartRotatingButton->RemoveButtonHandler(this);
   }
   mStartRotatingButton = b;
   if(mStartRotatingButton)
   {
      mStartRotatingButton->AddButtonHandler(this);
   }
}

void CadworkFlyMotionModel::SetDistanceAxis(LogicalAxis* distance)
{
   if(mDistanceAxis)
   {
      mDistanceAxis->RemoveAxisHandler(this);
   }
   mDistanceAxis = distance;
   if(mDistanceAxis)
   {
      mDistanceAxis->AddAxisHandler(this);
   }
}

/**
 * Returns the axis that turns the target up (for positive values)
 * or down (for negative values).
 *
 * @return the current turn up/down axis
 */
Axis* CadworkFlyMotionModel::GetTurnUpDownAxis()
{
   return mTurnUpDownAxis;
}

const Axis* CadworkFlyMotionModel::GetTurnUpDownAxis() const
{
   return mTurnUpDownAxis;
}

/**
 * Sets the maximum fly speed (meters per second).
 *
 * @param maximumFlySpeed the new maximum fly speed
 */
void CadworkFlyMotionModel::SetMaximumFlySpeed(float maximumFlySpeed)
{
   mMaximumFlySpeed = maximumFlySpeed;
}

/**
 * Returns the maximum fly speed (meters per second).
 *
 * @return the current maximum fly speed
 */
float CadworkFlyMotionModel::GetMaximumFlySpeed() const
{
   return mMaximumFlySpeed;
}

/**
 * Sets the maximum turn speed (degrees per second).
 *
 * @param maximumTurnSpeed the new maximum turn speed
 */
void CadworkFlyMotionModel::SetMaximumTurnSpeed(float maximumTurnSpeed)
{
   mMaximumTurnSpeed = maximumTurnSpeed;
}

/**
 * Returns the maximum turn speed (degrees per second).
 *
 * @return the current maximum turn speed
 */
float CadworkFlyMotionModel::GetMaximumTurnSpeed() const
{
   return mMaximumTurnSpeed;
}

void CadworkFlyMotionModel::SetUseSimTimeForSpeed(bool useSimTimeForSpeed)
{
   if (useSimTimeForSpeed)
   {
      mOptions = dtUtil::Bits::Add(mOptions, OPTION_USE_SIMTIME_FOR_SPEED);
   }
   else
   {
      mOptions = dtUtil::Bits::Remove(mOptions, OPTION_USE_SIMTIME_FOR_SPEED);
   }
}

void CadworkFlyMotionModel::ShowCursor(bool v)
{
   Camera *cam = dynamic_cast<Camera *>(GetTarget());
   if(cam)
   {
      cam->GetWindow()->ShowCursor(v);
   }
}

void CadworkFlyMotionModel::SetLookAtCenterButton(dtCore::Button *butt){
   if (mLookAtCenterButton)
   {
      mLookAtCenterButton->RemoveButtonHandler(this);
   }

   mLookAtCenterButton = butt;

   if (mLookAtCenterButton)
   {
      mLookAtCenterButton->AddButtonHandler(this);
   }
}

/**
 * Sets the target of this motion model. Then if target is a camera
 * and the computeHomePos is true, it coumputes and sets new position
 * and orientation for the camera.
 *
 * @param target the new target
 */
void  CadworkFlyMotionModel::SetTarget(Transformable* target, bool computeHomePos)
{
   MotionModel::SetTarget(target);
   dtCore::Camera *cam = dynamic_cast<dtCore::Camera *> (target);
   if(cam && computeHomePos)
   { 
      ComputeHomePosition(cam);
      GoToHomePosition();
   }
}

/**
 * Sets ne camera center via look at. It doesn't move the camera but
 * instead it turns it around its axis.
 * @param lookAt new point to look at.
 */
void CadworkFlyMotionModel::SetCenterPoint(osg::Vec3& lookAt)
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
float CadworkFlyMotionModel::GetDistanceAfterZoom(double delta)
{
   //float mLinearRate = 1.0; //maybe later should be added as a member
   delta = delta;
   float dist = mAnimData._fromDist;
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
   return mAnimData._fromDist+distDelta;
}

/**
 * Called when an axis' state has changed.
 *
 * @param axis the changed axis
 * @param oldState the old state of the axis
 * @param newState the new state of the axis
 * @param delta a delta value indicating stateless motion
 * @return If the
 */
bool CadworkFlyMotionModel::HandleAxisStateChanged(const dtCore::Axis* axis,
                                       double oldState,
                                       double newState,
                                       double delta)
{
   if(!IsEnabled()) return false;
   if(!HasOption(OPTION_REQUIRE_MOUSE_DOWN)) return false;

   double deltaState = newState - oldState;
   Transform trans;
   osg::Vec3 hpr;
   GetTarget()->GetTransform(trans);
   trans.GetRotation(hpr);

   if(mStartRotatingButton->GetState() == true)
   {
      if(axis == mDefaultTurnLeftRightAxis)
      {
         // rotation    
         hpr[0] -= float(mMaximumTurnSpeed * deltaState);              
      }
      if(axis == mDefaultTurnUpDownAxis)
      {
         float rotateTo = hpr[1] + float(mMaximumTurnSpeed * deltaState);

         if (rotateTo < -89.5f)
         {
            hpr[1] = -89.5f;
         }
         else if (rotateTo > 89.5f)
         {
            hpr[1] = 89.5f;
         }
         else
         {
            hpr[1] = rotateTo;
         }
      }
   }
   if(axis == mDistanceAxis)
   {
      dtCore::Camera *camera = dynamic_cast<dtCore::Camera *> (GetTarget());
      if(camera)
      {
         if(delta>0) //zoom in we want to focus on point uder cursor
         {
            osg::Matrix VPW =camera->GetOSGCamera()->getViewMatrix() * 
               camera->GetOSGCamera()->getProjectionMatrix() * 
               camera->GetOSGCamera()->getViewport()->computeWindowMatrix();
            osg::Matrix inverseVPW;
            inverseVPW.invert(VPW);
            float win_x,win_y;

            osg::Vec3 xyz,nearPoint,farPoint,hitPoint;
            float x=0, y=0;
            //float win_x, win_y;
            GetMouse()->GetPosition(x,y);
            osg::Quat quat;
            trans.GetRotation(quat);
            trans.GetTranslation(xyz);
            mAnimData._fromRotation = quat;
            mAnimData._fromCursor.set(x,y);
            mAnimData._toCursor.set(0.0,0.0);

            SSPick(x,y);
            if(mLineIntersector->containsIntersections())
            {
               mDistanceShouldChange = true;
               hitPoint = mLineIntersector->getFirstIntersection().getWorldIntersectPoint();
               SetCenterPoint(hitPoint);
               camera->GetTransform(trans);
               trans.GetRotation(quat);
               mAnimData._toRotation = quat;//get ending rotation
               mAnimData._isRotating = true; //set zooming flag
               


               /** Now the hitpoint is our new center so we need to recalculate current distance
                     for zooming and other purposses. */
               //SetDistance( (xyz - hitPoint).length());
               mAnimData._fromDist = (xyz - hitPoint).length();
               mTmpPrevDistance = mAnimData._fromDist;
               mAnimData._toDist = GetDistanceAfterZoom(delta);
               mAnimData._isZooming = true;

               mAnimData._startTime = System::GetInstance().GetSimulationTime();
               trans.SetRotation(mAnimData._fromRotation);
               camera->SetTransform(trans);
            }
            else
            { /** No point hit by ray, so we have our cursor pointing outside
                  the mode, but we still want to look and zoom in that direction. */
               mDistanceShouldChange = false;
               osg::Matrix VPW =camera->GetOSGCamera()->getViewMatrix() * 
                  camera->GetOSGCamera()->getProjectionMatrix() * 
                  camera->GetOSGCamera()->getViewport()->computeWindowMatrix();
               osg::Matrix inverseVPW;
               inverseVPW.invert(VPW);
               float win_x,win_y;
               camera->GetWindow()->CalcPixelCoords(x,y,win_x, win_y);
               farPoint.set(win_x,win_y, 0.0f);
               farPoint=farPoint*inverseVPW;
               //farPoint.normalize();
               SetCenterPoint(farPoint);

               camera->GetTransform(trans);
               trans.GetRotation(quat);
               mAnimData._toRotation = quat;//get ending rotation
               mAnimData._isRotating = true; //set zooming flag

               mAnimData._fromDist = (mEye - mCenter).length()/20; //magical constant derived from the model size
               mTmpPrevDistance = mAnimData._fromDist;
               mAnimData._toDist = GetDistanceAfterZoom(delta);

               mAnimData._isZooming = true;
               mAnimData._startTime = System::GetInstance().GetSimulationTime();
               
               //SetCenterPoint(oldCenter);
            }
         }
         else // zooming out no rotations or change of focus point
         {
            mDistanceShouldChange = true;
            mAnimData._fromDist = mTmpPrevDistance; //trying to adjust zoom speed to previous known distance, works fine with homing position initialization
            //mTmpPrevDistance = mAnimData._fromDist;//init
            mAnimData._toDist = GetDistanceAfterZoom(delta);
            mAnimData._isZooming = true;
            mAnimData._startTime = System::GetInstance().GetSimulationTime();
         }
      }
      return true;
   }
   trans.SetRotation(hpr);
   GetTarget()->SetTransform(trans);

   return false;
}

bool CadworkFlyMotionModel::HandleButtonStateChanged(const Button* button, bool oldState, bool newState)
{
   if (GetTarget() != 0 && IsEnabled())
   {
      if(button == mStartRotatingButton)
      {
         if(newState == false)
         {
            mRotationLRStartState = -1.0f;
            mRotationUDStartState = -1.0f;         
         }
      }
      else if(button == mLookAtCenterButton)
      {
         SetCenterPoint(mCenter);
         return true;
      }
   }
   return false;
}

/**
 * Message handler callback.
 *
 * @param data the message data
 */
void CadworkFlyMotionModel::OnMessage(MessageData* data)
{
   Transformable *target = GetTarget();
   if (target != NULL && IsEnabled() &&
      (data->message == dtCore::System::MESSAGE_POST_EVENT_TRAVERSAL/*MESSAGE_PRE_FRAME*/) &&
      // don't move if paused & using simtime for speed (since simtime will be 0 if paused)
      (!HasOption(OPTION_USE_SIMTIME_FOR_SPEED) || !System::GetInstance().GetPause()))
   {
         if(IsMouseGrabbed())
         {
         // Get the time change (sim time or real time)
         double delta = GetTimeDelta(data);

         Transform transform;

         GetTarget()->GetTransform(transform);

         osg::Vec3 xyz, hpr;

         transform.Get(xyz, hpr);

         // rotation

         bool rotationChanged = false;
         if(!HasOption(OPTION_REQUIRE_MOUSE_DOWN))
         {
            hpr = Rotate(hpr, delta, rotationChanged);
            if (rotationChanged)
            {
               transform.SetRotation(hpr);
            }
         }

         // translation

         bool translationChanged = false;
         xyz = Translate(xyz, delta, translationChanged);
         if (translationChanged)
         {
            transform.SetTranslation(xyz);
         }


         // homing

         if(mHomingButton->GetState())
         {
            GoToHomePosition();
         }


         // finalize changes

         if (rotationChanged || translationChanged)
         {
            GetTarget()->SetTransform(transform);
         }
      }
      // release cursor
      //if(mCursorReleaseButton->GetState())
      //{
      //   ReleaseMouse();
      //}
      ////grab cursor
      //if(mCursorGrabButton->GetState())
      //{
      //   GrabMouse();
      //}
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


               float deltaDist = mTmpPrevDistance - newdist;
               /*if (newdist < MIN_DISTANCE && (mAnimData._toDist < mAnimData._fromDist))
               {
                  deltaDist = MIN_DISTANCE - GetDistance();
               }*/
               osg::Vec3 translation (0.0f, deltaDist, 0.0f);
               osg::Matrix mat;
               dtUtil::MatrixUtil::HprToMatrix(mat, hpr);
               translation = osg::Matrix::transform3x3(translation, mat);
               xyz += translation;
               if(mDistanceShouldChange)                  
                  mTmpPrevDistance = newdist;//(mNewCenter-xyz).length();
               trans.Set(xyz,oldhpr);
            }
            else{ //zooming only
               osg::Vec3 xyz,hpr;
               trans.Get(xyz,hpr);
               float deltaDist = mTmpPrevDistance - newdist;
               osg::Vec3 translation (0.0f, deltaDist, 0.0f);
               osg::Matrix mat;
               dtUtil::MatrixUtil::HprToMatrix(mat, hpr);
               translation = osg::Matrix::transform3x3(translation, mat);
               xyz += translation;
               if(mDistanceShouldChange)
                  mTmpPrevDistance = newdist;
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
         }
      }
   }
}

double CadworkFlyMotionModel::GetTimeDelta(const MessageData* data) const
{
   // Get the time change (sim time or real time)
   double delta;
   double* timeChange = (double*)data->userData;
   //if (data->message == dtCore::System::MESSAGE_PAUSE) // paused and !useSimTime
   // Note - the if in OnMessage() prevents getting here if paused & OPTION_USE_SIMTIME_FOR_SPEED is set
   if (System::GetInstance().GetPause())
   {
      delta = timeChange[1]; // 0 is real time when paused
   }
   else if (HasOption(OPTION_USE_SIMTIME_FOR_SPEED))
   {
      delta = timeChange[0]; // 0 is sim time
   }
   else
   {
      delta = timeChange[1]; // 1 is real time
   }

   return delta;
}

osg::Vec3 CadworkFlyMotionModel::Rotate(const osg::Vec3& hpr, double delta, bool& changed) const
{
   osg::Vec3 out = hpr;

   changed = false;

   if (mTurnLeftRightAxis != NULL && mTurnLeftRightAxis->GetState() != 0.0)
   {
      out[0] -= float(mTurnLeftRightAxis->GetState() * mMaximumTurnSpeed * delta);
      changed = true;
   }

   if (mTurnUpDownAxis != NULL && mTurnUpDownAxis->GetState() != 0.0)
   {
      float rotateTo = out[1] + float(mTurnUpDownAxis->GetState() * mMaximumTurnSpeed * delta);

      if (rotateTo < -89.5f)
      {
         out[1] = -89.5f;
      }
      else if (rotateTo > 89.5f)
      {
         out[1] = 89.5f;
      }
      else
      {
         out[1] = rotateTo;
      }
      changed = true;
   }


   if (HasOption(OPTION_RESET_MOUSE_CURSOR))
   {
      // fix to avoid camera drift
      mTurnUpDownAxis->SetState(0.0f); // necessary to stop camera drifting down
      mTurnLeftRightAxis->SetState(0.0f); // necessary to stop camera drifting left

      mMouse->SetPosition(0.0f,0.0f); // keeps cursor at center of screen
   }

   // allow for one degree of error.  Assigning it to 1 or -1 as opposed
   // to 0.0, keeps the camera from jerking when it corrects.
   // One degree of being off isn't very noticeable, and it keeps the accumulated error
   // of getting and setting the transform down.
   if (out[2] > 1.0 )
   {
      out[2] = 1.0f;
      changed = true;
   }
   else if (out[2] < -1.0)
   {
      out[2] = -1.0f;
      changed = true;
   }

   if (!changed)
   {
      return hpr;
   }

   return out;
}

osg::Vec3 CadworkFlyMotionModel::Translate(const osg::Vec3& xyz, double delta, bool& changed) const
{
   osg::Vec3 translation;

   changed = false;

   if (mFlyForwardBackwardAxis != NULL && mFlyForwardBackwardAxis->GetState() != 0.0)
   {
      translation[1] = float(mFlyForwardBackwardAxis->GetState() * mMaximumFlySpeed * delta);
      changed = true;
   }

   if (mFlyLeftRightAxis != NULL && mFlyLeftRightAxis->GetState() != 0.0)
   {
      translation[0] = float(mFlyLeftRightAxis->GetState() * mMaximumFlySpeed * delta);
      changed = true;
   }

   if (mFlyUpDownAxis != NULL && mFlyUpDownAxis->GetState() != 0.0)
   {
      translation[2] = float(mFlyUpDownAxis->GetState() * mMaximumFlySpeed * delta);
      changed = true;
   }

   if (!changed)
   {
      return xyz;
   }

   // rotate
   {
      Transform transform;
      GetTarget()->GetTransform(transform);
      osg::Matrix mat;

      transform.GetRotation(mat);

      translation = osg::Matrix::transform3x3(translation, mat);
   }

   osg::Vec3 out = xyz + translation;

   return out;
}

void CadworkFlyMotionModel::GoToHomePosition()
{
   dtCore::Camera *cam = dynamic_cast<dtCore::Camera *> (GetTarget());
   if(cam)
   {
      
      double dist = (mCenter - mEye).length();
      mTmpPrevDistance = dist;
      Transform trans;
      trans.Set(mEye,mCenter,mUp);
      cam->SetTransform(trans);

   }
}

void CadworkFlyMotionModel::ReleaseMouse()
{
   if(HasOption(OPTION_RESET_MOUSE_CURSOR))
   {
      SetOptions(mOptions & ~OPTION_RESET_MOUSE_CURSOR);
      mMouseGrabbed = false;
   }
   if(HasOption(OPTION_HIDE_CURSOR))
      ShowCursor(true);
}

void CadworkFlyMotionModel::GrabMouse()
{
   if(HasOption(OPTION_RESET_MOUSE_CURSOR))
   {
      SetOptions(mOptions & ~OPTION_RESET_MOUSE_CURSOR);
      mMouse->SetPosition(0.0,0.0);
      mMouseGrabbed = true;
   }
   if(HasOption(OPTION_HIDE_CURSOR))
      ShowCursor(false);
}

void CadworkFlyMotionModel::SSPick(float x, float y)
{
   //osg::Vec3 nearPoint,farPoint; //hitpoint will be our new center
   Camera *camera = dynamic_cast<Camera *>(GetTarget());
   if(!camera) return;

   //mouse position
   float win_x, win_y;
   mMouse->GetPosition(x,y);
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
void CadworkFlyMotionModel::SetViewPosition(/*osg::Matrix modelView*/osg::Vec3 eye, osg::Vec3 center)
{
   dtCore::Transform trans;
   //trans.Set(modelView);
   trans.Set(eye,center, osg::Vec3 (0.0,0.0,1.0));
   GetTarget()->SetTransform(trans);
}

void CadworkFlyMotionModel::CMMI_SetDistance(float distance){ mTmpPrevDistance = distance;}
float CadworkFlyMotionModel::CMMI_GetDistance() { return mTmpPrevDistance;}

/***********CadworkMotionModelInterface END **************/

//////////////////////////////////////////////////////////////////////////
bool CadworkFlyMotionModel::HasOption(unsigned int option) const
{
   return dtUtil::Bits::Has(mOptions, option);
}

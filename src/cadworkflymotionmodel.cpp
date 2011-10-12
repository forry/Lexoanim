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
#include <dtUtil/log.h>

#include <dtCore/camera.h>
#include <dtCore/deltawin.h>
#include <dtCore/button.h>

using namespace dtCore;

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
{

   RegisterInstance(this);

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

      if (HasOption(OPTION_REQUIRE_MOUSE_DOWN))
      {
         leftButtonUpAndDown = mDefaultInputDevice->AddAxis(
            "left mouse button up/down",
            mLeftButtonUpDownMapping = new ButtonAxisToAxis(
               mouse->GetButton(Mouse::LeftButton),
               mouse->GetAxis(1))
         );

         leftButtonLeftAndRight = mDefaultInputDevice->AddAxis(
            "left mouse button left/right",
            mLeftButtonLeftRightMapping = new ButtonAxisToAxis(
               mouse->GetButton(Mouse::LeftButton),
               mouse->GetAxis(0)
            )
         );
      }
      else
      {
         leftButtonUpAndDown = mDefaultInputDevice->AddAxis(
            "left mouse movement up/down",
            new AxisToAxis(mouse->GetAxis(1))
         );

         leftButtonLeftAndRight = mDefaultInputDevice->AddAxis(
            "left mouse movement left/right",
            new AxisToAxis(mouse->GetAxis(0))
         );
      }

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
            new AxisToAxis(mouse->GetAxis(0))
            );

         mDefaultTurnUpDownAxis = mDefaultInputDevice->AddAxis(
            "default turn up/down",
            new AxisToAxis(mouse->GetAxis(1))
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

      mCursorGrabButtonMapping = new ButtonsToButton(
         mouse->GetButton(Mouse::LeftButton),
         mouse->GetButton(Mouse::RightButton)
         );
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

      mCursorGrabButtonMapping->SetFirstButton(mouse->GetButton(Mouse::LeftButton));
      mCursorGrabButtonMapping->SetSecondButton(mouse->GetButton(Mouse::RightButton));
   }

   SetFlyForwardBackwardAxis(mDefaultFlyForwardBackwardAxis);

   SetFlyLeftRightAxis(mDefaultFlyLeftRightAxis);

   SetFlyUpDownAxis(mDefaultFlyUpDownAxis);

   SetTurnLeftRightAxis(mDefaultTurnLeftRightAxis);

   SetTurnUpDownAxis(mDefaultTurnUpDownAxis);

   mDefaultInputDevice->AddButton("Homing button", mHomingButton, ' ');

   SetCursorReleaseButton(mDefaultInputDevice->AddButton("Cursor release button", osgGA::GUIEventAdapter::KEY_Control_L, mCursorReleaseButtonMapping));
   
   SetCursorGrabButton(mDefaultInputDevice->AddButton("Cursor grab button", Mouse::LeftButton, mCursorGrabButtonMapping));
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
   mTurnLeftRightAxis = turnLeftRightAxis;
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
   mTurnUpDownAxis = turnUpDownAxis;
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
 * Message handler callback.
 *
 * @param data the message data
 */
void CadworkFlyMotionModel::OnMessage(MessageData* data)
{
   if (GetTarget() != NULL && IsEnabled() &&
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
         hpr = Rotate(hpr, delta, rotationChanged);
         if (rotationChanged)
         {
            transform.SetRotation(hpr);
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
      if(mCursorReleaseButton->GetState())
      {
         ReleaseMouse();
      }
      //grab cursor
      if(mCursorGrabButton->GetState())
      {
         GrabMouse();
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
      Transform trans;
      trans.Set(mEye,mCenter,mUp);
      cam->SetTransform(trans);

   }
}

void CadworkFlyMotionModel::ReleaseMouse()
{
   SetOptions(mOptions & ~OPTION_RESET_MOUSE_CURSOR);
   mMouseGrabbed = false;
   ShowCursor(true);
}

void CadworkFlyMotionModel::GrabMouse()
{
   SetOptions(mOptions| OPTION_RESET_MOUSE_CURSOR);
   mMouse->SetPosition(0.0,0.0);
   mMouseGrabbed = true;
   ShowCursor(false);
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

/***********CadworkMotionModelInterface END **************/

//////////////////////////////////////////////////////////////////////////
bool CadworkFlyMotionModel::HasOption(unsigned int option) const
{
   return dtUtil::Bits::Has(mOptions, option);
}

/*
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2004-2005 MOVES Institute
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef CADWORK_FLYMOTIONMODEL
#define CADWORK_FLYMOTIONMODEL

// flymotionmodel.h: Declaration of the FlyMotionModel class.
//
////////////////////////////////////////////////////////////////////////////////

#include <dtCore/motionmodel.h>
#include <dtCore/axishandler.h>
#include <dtCore/buttonhandler.h>
#include "CameraHomer.h"
#include "CadworkMotionModelInterface.h"

////////////////////////////////////////////////////////////////////////////////

namespace osgUtil
{
   class LineSegmentIntersector;
   class IntersectionVisitor;
}

namespace dtCore
{
   class Axis;
   class AxisToAxis;
   class ButtonAxisToAxis;
   class ButtonsToAxis;
   class Keyboard;
   class LogicalAxis;
   class LogicalButton;
   class LogicalInputDevice;
   class Mouse;
   class Button;
   class ButtonsToButton;

   /**
    * A motion model that simulates the action of flying.
    */
   class CadworkFlyMotionModel : public MotionModel, public AxisHandler, public ButtonHandler, public CameraHomer, virtual public CadworkMotionModelInterface
   {
      DECLARE_MANAGEMENT_LAYER(CadworkFlyMotionModel)

      public:

         static const float MIN_DISTANCE;

         ///Set of options to configure how FlyMotionModel behaves.
         enum BehaviorOptions
         {
            OPTION_NONE                  = 0x0,

            /** Indicates whether the FlyMotionModel will use Sim Time
             * or not (aka Real Time) for speed of movement and turn rotation
             */
            OPTION_USE_SIMTIME_FOR_SPEED = 0x1,

            /** Indicates whether the FlyMotionModel will require the mouse
             * button(s) depressed in order to control turn rotation
             */
            OPTION_REQUIRE_MOUSE_DOWN    = 0x2,

            /** Indicates whether the FlyMotionModel will reset the mouse
             * cursor coordinates to the center of the screen each frame
             */
            OPTION_RESET_MOUSE_CURSOR    = 0x4,

            /** Indicates whether the FlyMotionModel will move
             * in response to pressing the cursor keys.
             */
            OPTION_USE_CURSOR_KEYS       = 0x8,

            /**
             * Indicates whether this motion model will hide cursor when
             * it's grabed (when using it to fly).
             */
            OPTION_HIDE_CURSOR           = 0x10,

            /** Default setup of options (OPTION_USE_SIMTIME_FOR_SPEED,
             *   OPTION_USE_CURSOR_KEYS, OPTION_REQUIRE_MOUSE_DOWN).
             */
            OPTION_DEFAULT               = OPTION_USE_SIMTIME_FOR_SPEED |
                                           OPTION_USE_CURSOR_KEYS       |
                                           OPTION_REQUIRE_MOUSE_DOWN
         };

         /**
          * Constructor.
          *
          * @param keyboard the keyboard instance, or NULL to
          * avoid creating default input mappings
          * @param mouse the mouse instance, or NULL to avoid
          * creating default input mappings
          * @param options : bitwise or of possible FlyMotionModel::BehaviorOptions.
          */
         CadworkFlyMotionModel(Keyboard* keyboard = NULL, Mouse* mouse = NULL, unsigned int options = OPTION_DEFAULT);

      protected:

         /**
          * Destructor.
          */
         virtual ~CadworkFlyMotionModel();

      public:

         /**
          * Returns whether the motion model has been created with a
          * specific behavior option
          *
          * @param option the option to query
          */
         bool HasOption(unsigned int option) const;

         /**
          * Sets the input axes to a set of default mappings for mouse
          * and keyboard.
          *
          * @param keyboard the keyboard instance
          * @param mouse the mouse instance
          */
         void SetDefaultMappings(Keyboard* keyboard, Mouse* mouse);

         /**
          * Sets the axis that moves the target forwards (for positive
          * values) or backwards (for negative values).
          *
          * @param flyForwardBackwardAxis the new forward/backward axis
          */
         void SetFlyForwardBackwardAxis(Axis* flyForwardBackwardAxis);

         /**
          * Returns the axis that moves the target forwards (for positive
          * values) or backwards (for negative values).
          *
          * @return the current forward/backward axis
          */
         Axis* GetFlyForwardBackwardAxis();
         const Axis* GetFlyForwardBackwardAxis() const;

         /**
          * Sets the axis that moves the target left (for positive
          * values) or right (for negative values).
          *
          * @param flyLeftRightAxis the new left/right axis
          */
         void SetFlyLeftRightAxis(Axis* flyLeftRightAxis);

         /**
          * Returns the axis that moves the target left (for positive
          * values) or right (for negative values).
          *
          * @return the current left/right axis
          */
         Axis* GetFlyLeftRightAxis();
         const Axis* GetFlyLeftRightAxis() const;

         /**
          * Sets the axis that moves the target target-relative up
          * (for positive values) or target-relative down (for
          * negative values).
          *
          * @param flyUpDownAxis the new up/down axis
          */
         void SetFlyUpDownAxis(Axis* flyUpDownAxis);

         /**
          * Returns the axis that moves the target target-relative up
          * (for positive values) or target-relative down (for negative
          * values).
          *
          * @return the current up/down axis
          */
         Axis* GetFlyUpDownAxis();
         const Axis* GetFlyUpDownAxis() const;

         /**
          * Sets the axis that turns the target left (for negative values)
          * or right (for positive values).
          *
          * @param turnLeftRightAxis the new turn left/right axis
          */
         void SetTurnLeftRightAxis(Axis* turnLeftRightAxis);

         /**
          * Returns the axis that turns the target left (for negative values)
          * or right (for positive values).
          *
          * @return the current turn left/right axis
          */
         Axis* GetTurnLeftRightAxis();
         const Axis* GetTurnLeftRightAxis() const;

         /**
          * Sets the axis that turns the target up (for positive values)
          * or down (for negative values).
          *
          * @param turnUpDownAxis the new turn up/down axis
          */
         void SetTurnUpDownAxis(Axis* turnUpDownAxis);

         /**
          * Returns the axis that turns the target up (for positive values)
          * or down (for negative values).
          *
          * @return the current turn up/down axis
          */
         Axis* GetTurnUpDownAxis();
         const Axis* GetTurnUpDownAxis() const;

         /**
          * Sets the maximum fly speed (meters per second).
          *
          * @param maximumFlySpeed the new maximum fly speed
          */
         void SetMaximumFlySpeed(float maximumFlySpeed);

         /**
          * Returns the maximum fly speed (meters per second).
          *
          * @return the current maximum fly speed
          */
         float GetMaximumFlySpeed() const;

         /**
          * Sets the maximum turn speed (degrees per second).
          *
          * @param maximumTurnSpeed the new maximum turn speed
          */
         void SetMaximumTurnSpeed(float maximumTurnSpeed);

         /**
          * Returns the maximum turn speed (degrees per second).
          *
          * @return the current maximum turn speed
          */
         float GetMaximumTurnSpeed() const;

         /**
          * Sets whether we use sim time or not (aka real time)
          * for the speed of movement and rotation.
          */
         void SetUseSimTimeForSpeed(bool useSimTimeForSpeed);

         void ShowCursor(bool v = true);

         inline virtual void SetCursorReleaseButton( LogicalButton *b){mCursorReleaseButton = b;}
         inline virtual LogicalButton *GetCursorReleaseButton(){ return mCursorReleaseButton;}

         inline virtual void SetCursorGrabButton( LogicalButton *b){ mCursorGrabButton = b;}
         inline virtual LogicalButton *GetCursorGrabButton(){ return mCursorGrabButton;}

         virtual void SetStartRotatingButton( LogicalButton *b);
         inline virtual LogicalButton *GetStartRotatingButton(){ return mStartRotatingButton;}

         virtual void SetDistanceAxis(LogicalAxis* distance);
         inline virtual LogicalAxis* GetDistanceAxis(){ return mDistanceAxis;}

         inline virtual void SetOptions (unsigned int opt) { mOptions = opt; }

         //inline virtual void SetMouseGrabbed(bool v=true) {mMouseGrabbed = v; }
         inline virtual bool IsMouseGrabbed() { return mMouseGrabbed;}

         /** @return Reterns mouse device supplied in constructor */
         inline virtual Mouse *GetMouse(){ return mMouse;}

         virtual void ReleaseMouse();
         virtual void GrabMouse();

         /**
          * Sets the target of this motion model. Then if target is a camera
          * and the computeHomePos is true, it coumputes and sets new position
          * and orientation for the camera.
          *
          * @param target the new target
          */
         virtual void SetTarget(Transformable* target, bool computeHomePos = false);

         /**
          * Sets ne camera center via look at. It doesn't move the camera but
          * instead it turns it around its axis.
          * @param lookAt new point to look at.
          */
         virtual void SetCenterPoint(osg::Vec3& lookAt);

         /**
          * Called when an axis' state has changed.
          *
          * @param axis the changed axis
          * @param oldState the old state of the axis
          * @param newState the new state of the axis
          * @param delta a delta value indicating stateless motion
          * @return If the
          */
         virtual bool HandleAxisStateChanged(const dtCore::Axis* axis,
                                       double oldState,
                                       double newState,
                                       double delta);

         /**
          * Called when a button's state has changed.
          *
          * @param button the origin of the event
          * @param oldState the old state of the button
          * @param newState the new state of the button
          * @return whether this handler handled the state change or not
          */
         virtual bool HandleButtonStateChanged(const Button* button, bool oldState, bool newState);

         /**
          * Message handler callback.
          *
          * @param data the message data
          */
         virtual void OnMessage(MessageData* data);

         ///CameraHomer

         virtual void GoToHomePosition();

         ///CadworkMotionModelInterface

         /** for setting View (loaded from .ivv files) into Motion models corectly. */
         //virtual void SetViewPosition(osg::Matrix modelView);
         virtual void SetViewPosition(osg::Vec3 eye, osg::Vec3 center);

      protected:
         double GetTimeDelta(const MessageData* data) const;
         osg::Vec3 Rotate(const osg::Vec3& hpr, double delta, bool& changed) const;
         osg::Vec3 Translate(const osg::Vec3& xyz, double delta, bool& changed) const;
         virtual float GetDistanceAfterZoom(double delta);
         /**
          * Screen Space Pick
          */
         virtual void SSPick(float x, float y);


      private:

         /**
          * The default input device.
          */
         RefPtr<LogicalInputDevice> mDefaultInputDevice;

         /**
          * The left button up/down mapping.
          */
         ButtonAxisToAxis* mLeftButtonUpDownMapping;

         /**
          * The left button right/left mapping.
          */
         ButtonAxisToAxis* mLeftButtonLeftRightMapping;

         /**
          * The right button up/down mapping.
          */
         ButtonAxisToAxis* mRightButtonUpDownMapping;

         /**
          * The right button left/right mapping.
          */
         ButtonAxisToAxis* mRightButtonLeftRightMapping;

         /**
          * The arrow key up/down mapping.
          */
         ButtonsToAxis* mArrowKeysUpDownMapping;

         /**
          * The arrow key left/right mapping.
          */
         ButtonsToAxis* mArrowKeysLeftRightMapping;

         /**
          * The w/s key forward/backward mapping.
          */
         ButtonsToAxis* mWSKeysUpDownMapping;
         ButtonsToAxis* mWSKeysUpDownMappingCaps;

         /**
          * The a/d key strafe left/right mapping.
          */
         ButtonsToAxis* mADKeysLeftRightMapping;
         ButtonsToAxis* mADKeysLeftRightMappingCaps;

         /**
          * The q/e key fly up/down mapping.
          */
         ButtonsToAxis* mQEKeysUpDownMapping;
         ButtonsToAxis* mQEKeysUpDownMappingCaps;

         /**
          * The mouse wheel up/down scroll mapping.
          */
         AxisToAxis* mMouseWheelUpDownMapping;

         /**
          * The space key homing button
          */
         Button *mHomingButton;

         /**
          * Release mouse cursor button - for giving the cursor control bac to the user.
          */
         ButtonsToButton *mCursorReleaseButtonMapping;
         LogicalButton *mCursorReleaseButton;

         /**
          * Grab mouse cursor button - for retrieving the cursor back.
          */
         ButtonsToButton *mCursorGrabButtonMapping;
         LogicalButton *mCursorGrabButton;

         /**
          * Start rotating
          */
         ButtonsToButton *mStartRotatingButtonMapping;
         LogicalButton *mStartRotatingButton;
         float mRotationLRStartState,mRotationUDStartState;

         /**
          * The default fly forward/backward axis.
          */
         LogicalAxis* mDefaultFlyForwardBackwardAxis;

         /**
          * The default fly left/right axis.
          */
         LogicalAxis* mDefaultFlyLeftRightAxis;

         /**
          * The default fly up/down axis.
          */
         LogicalAxis* mDefaultFlyUpDownAxis;

         /**
          * The default turn left/right axis.
          */
         LogicalAxis* mDefaultTurnLeftRightAxis;

         /**
          * The default turn up/down axis.
          */
         LogicalAxis* mDefaultTurnUpDownAxis;

         LogicalAxis* mDistanceAxis;

         /**
          * The axis that moves the target forwards or backwards.
          */
         Axis* mFlyForwardBackwardAxis;

         /**
          * The axis that moves the target left or right.
          */
         Axis* mFlyLeftRightAxis;

         /**
          * The axis that moves the target up or down.
          */
         Axis* mFlyUpDownAxis;

         /**
          * The axis that turns the target left or right.
          */
         Axis* mTurnLeftRightAxis;

         /**
          * The axis that turns the target up or down.
          */
         Axis* mTurnUpDownAxis;

         /**
          * The maximum fly speed (meters per second).
          */
         float mMaximumFlySpeed;

         /**
          * The maximum turn speed (degrees per second).
          */
         float mMaximumTurnSpeed;

         Mouse* mMouse;

         /**
          * The bitwise combination of the types of behavior this
          * motion model will enforce
          */
         unsigned int mOptions;
          /**
           * When mouse is grabbed by motion model, we should be able to
           * manipulate the target. When the mouse is free the motion model 
           * only waits for clicking into scene, so he could grab it again.
           */
         bool mMouseGrabbed;

         /** Ray intersector for picking (on distance axis state change) */
         RefPtr<osgUtil::LineSegmentIntersector> mLineIntersector;
         RefPtr<osgUtil::IntersectionVisitor> mIntersectionVisitor;

         float mTmpPrevDistance;
         bool  mDistanceShouldChange;
         float mLinearRate;

         AnimationData mAnimData;

   };
} // namespace dtCore

////////////////////////////////////////////////////////////////////////////////

#endif // CADWORK_FLYMOTIONMODEL

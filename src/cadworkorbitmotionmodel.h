#ifndef CADWORK_ORBIT_MOTION_MODEL
#define CADWORK_ORBIT_MOTION_MODEL

#include <dtCore/axishandler.h>
#include <dtCore/buttonhandler.h>
#include <dtCore/orbitmotionmodel.h>
//#include <dtCore/isector.h>
//#include "isector.h"
#include <dtCore/scene.h>
#include <dtCore/mouse.h>

#include <osgAnimation/EaseMotion>
#include <CameraHomer.h>
#include "CadworkMotionModelInterface.h"

namespace osgUtil
{
   class LineSegmentIntersector;
   class IntersectionVisitor;
}

namespace dtCore
{
   class ButtonAxisToAxis;
   class ButtonsToAxis;
   class AxisToAxis;
   class Keyboard;
   class Mouse;
   class LogicalAxis;
   class LogicalInputDevice;
   class Isector;

   class AnimationData;

   struct HomePosition
   {
      osg::Vec3d eye;
      osg::Vec3d center;
      osg::Vec3d up;
   };

   class CadworkOrbitMotionModel : public dtCore::OrbitMotionModel, public dtCore::ButtonHandler, public CameraHomer, virtual public CadworkMotionModelInterface
   {
      DECLARE_MANAGEMENT_LAYER(CadworkOrbitMotionModel)

   public:
      /**
       * Constructor.
       */
      CadworkOrbitMotionModel(Keyboard* keyboard = NULL, Mouse* mouse = NULL, Scene* scene = NULL, 
                              bool mAutoComputeHomePosition = true, bool mUseBoundingBox = true);

      /**
       * Sets the input axes to a set of default mappings for mouse
       * and keyboard.
       *
       * @param keyboard the keyboard instance
       * @param mouse the mouse instance
       */
      void SetDefaultMappings(Keyboard* keyboard, Mouse* mouse);

      /**
       * Sets the target of this motion model. Then if target is a camera
       * and the mComputeHomePosition is true, it coumputes and sets new position
       * and orientation for the camera.
       *
       * @param target the new target
       */
      virtual void  SetTarget(Transformable* target, bool computeHomePos = false);

      /**
       * Computes and sets the homing position for camera.
       */
      //virtual void ComputeAndSetHomePosition();

      /**
       * Sets ne camera center via look at. It doesn't move the camera but
       * instead it turns it around its axis.
       * @param lookAt new point to look at.
       */
      virtual void SetCenterPoint(osg::Vec3& lookAt);

      /** 
       * Manually set the home position, and set the automatic compute of home position.
       */
      //virtual void setHomePosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up, bool autoComputeHomePosition=false);

      ///CameraHomer interface
      virtual void GoToHomePosition();

      //~CameraHomer interface


      //inline virtual bool GetAutoComputeHomePosition(){ return mAutoComputeHomePosition;}

      //inline virtual void SetAutoComputeHomePosition(bool v=true){ mAutoComputeHomePosition=v;}

      inline virtual bool GetUseBoundingBox(){ return mUseBoundingBox;}

      inline virtual void SetUseBoundingBox(bool v=true){ mUseBoundingBox=v;}

      inline virtual void SetScene(Scene * scene){ mScene = scene; }
      inline virtual Scene *GetScene(){ return mScene.get(); }

      inline virtual void centerMousePointer(){ if(mMouse) mMouse->SetPosition(0.0f, 0.0f);}

      /** @return Reterns mouse device supplied in constructor */
      inline virtual Mouse *GetMouse(){ return mMouse.get();}

      /** @return Reterns keyboard device supplied in constructor */
      inline virtual Keyboard *GetKeyboard(){ return mKeyboard.get();}

      virtual void SetHomingButton(dtCore::Button *butt);

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

      ///CadworkMotionModelInterface

      /** for setting View (loaded from .ivv files) into Motion models corectly. */
      virtual void SetViewPosition(/*osg::Matrix modelView*/ osg::Vec3 eye, osg::Vec3 center);

      //~CadworkMotionModelInterface

   protected:
      /**
       * Destructor.
       */
      virtual ~CadworkOrbitMotionModel();

      LogicalInputDevice* GetDefaultLogicalInputDevice() { return mDefaultInputDevice.get(); }
      const LogicalInputDevice* GetDefaultLogicalInputDevice() const { return mDefaultInputDevice.get(); }

      virtual float GetDistanceAfterZoom(double delta);

      virtual void OnMessage(MessageData *data);

      //picking util
      /**
       * Screen Space Pick
       */
      virtual void SSPick(float x, float y);

      /**
       * The default input device.
       */
      RefPtr<LogicalInputDevice> mDefaultInputDevice;

      /**
       * The left button up/down mapping.
       */
      RefPtr<ButtonAxisToAxis> mLeftButtonUpDownMapping;

      /**
       * The left button right/left mapping.
       */
      RefPtr<ButtonAxisToAxis> mLeftButtonLeftRightMapping;

      /**
       * The right button up/down mapping.
       */
      RefPtr<ButtonAxisToAxis> mRightButtonUpDownMapping;

      /**
       * The right button left/right mapping.
       */
      RefPtr<ButtonAxisToAxis> mRightButtonLeftRightMapping;

      /**
       * The middle button up/down mapping.
       */
      RefPtr<ButtonAxisToAxis> mMiddleButtonUpDownMapping;
      /**
       * The middle button up/down mapping.
       */
      RefPtr<ButtonAxisToAxis> mMiddleButtonLeftRightMapping;
      /**
       * The mouse wheel up/down scroll mapping.
       */
      RefPtr<AxisToAxis> mMouseWheelUpDownMapping;
      /**
       * The axis that affects the azimuth of the orbit.
       */
      RefPtr<Axis> mAzimuthAxis;

      /**
       * The axis that affects the elevation of the orbit.
       */
      RefPtr<Axis> mElevationAxis;

      /**
       * The axis that affects the distance of the orbit.
       */
      RefPtr<Axis> mDistanceAxis;

      /**
       * The axis that affects the left/right translation of the orbit.
       */
      RefPtr<Axis> mLeftRightTranslationAxis;

      /**
       * The axis that affects the up/down translation of the orbit.
       */
      RefPtr<Axis> mUpDownTranslationAxis;
      /**
       * The button that returns camera to it's original (homing) position.
       */
      RefPtr<Button> mHomingButton;
      /**
       * The linear rate (ratio between axis units and linear movement).
       */
      float mLinearRate;


      /**
       * If true, then the SetTarget method computes and sets a suitable
       * home position for camera target. If the target is not camera,
       * then it is ignored. Default is true.
       */
      //bool mAutoComputeHomePosition;

      bool mUseBoundingBox;

      //HomePosition mHomePosition;

      /** Mouse and keyboard, used for picking when camera is a target. */
      RefPtr<Mouse> mMouse;
      RefPtr<Keyboard> mKeyboard;

      /** Ray intersector for picking (on distance axis state change) */
      //RefPtr<Isector> mRay;
      RefPtr<osgUtil::LineSegmentIntersector> mLineIntersector;
      RefPtr<osgUtil::IntersectionVisitor> mIntersectionVisitor;

      /** Scene for picking */
      RefPtr<Scene> mScene;

      /** when zooming and rotating. We want to zoom along the straight line not a curve.*/
      osg::Vec3 mNewCenter;

      /** when we are zooming while cursor is pointing outside the model, we sould not change distance */
      bool mDistanceShouldChange;

      /**
         * Storing focal point for distance recomputation. Used when target
         * is moved by another motion model and then the target is returned
         * back here.
         */
      osg::Vec3 mStoredFocalPoint;


      

      AnimationData mAnimData;
   };


}

#endif
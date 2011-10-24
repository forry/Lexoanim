#ifndef CADWORK_MOTION_MODEL_INTERFACE
#define CADWORK_MOTION_MODEL_INTERFACE

#include <osgAnimation/EaseMotion>

class CadworkMotionModelInterface
{
public:
   /** for setting View (loaded from .ivv files) into Motion models corectly. */
   virtual void SetViewPosition(/*osg::Matrix*/osg::Vec3 , osg::Vec3 )=0;

   class AnimationData {
        public:

            AnimationData():_isRotating(false), _isZooming(false), _startTime(0.0), _previousPhase(0.0), _interpolateCursor(true),
               _fromCursor(osg::Vec2(0.0,0.0)), _toCursor(osg::Vec2(0.0,0.0)),
               _motion(new osgAnimation::MathMotionTemplate<osgAnimation::OutQuartFunction>(0,0.4,1,osgAnimation::Motion::CLAMP)){}

            AnimationData(double durationTime):
               _isRotating(false), _isZooming(false), _startTime(0.0), _previousPhase(0.0), _interpolateCursor(true),
               _fromCursor(osg::Vec2(0.0,0.0)), _toCursor(osg::Vec2(0.0,0.0)),
               _motion(new osgAnimation::MathMotionTemplate<osgAnimation::OutQuartFunction>(0,durationTime,1,osgAnimation::Motion::CLAMP)){}


            //virtual void startZooming( const double startTime );
            //virtual void startRotating( const double startTime );
            inline virtual bool isAnimating(){ return (_isRotating || _isZooming); }

            virtual void reset()
            {
               _isRotating = _isZooming = false;
               _startTime = 0.0;
               _previousPhase = 0.0;
               _fromRotation = _toRotation;
               _fromDist = _toDist;
               _fromCursor = _toCursor;
            }

            /**
             * Create new MathMotionTemplate object and sets the duration as duration. All animations
             * should be stopped before you do that. The reset method is called at the end.
             */
            virtual void setDuration(double duration)
            {
               _motion = new osgAnimation::MathMotionTemplate<osgAnimation::OutQuartFunction>(0,duration,1,osgAnimation::Motion::CLAMP);
               reset();
            }

            /** 
             * Time of the animation start (can be simulation time System::GetSimulationTime().
             * The duration is stored in MathMotionTemplate _motion.getDuration(), default is
             * 0.4s.
             */
            double _startTime;
            double _previousPhase; //previous step of/value of interpolator

            osg::Quat _fromRotation,_toRotation;
            bool _isRotating;

            double _fromDist, _toDist;
            bool _isZooming;

            osg::ref_ptr<osgAnimation::Motion> _motion;
            osg::Vec2f _fromCursor, _toCursor;
            bool _interpolateCursor;
            
        };
};

#endif //CADWORK_MOTION_MODEL_INTERFACE
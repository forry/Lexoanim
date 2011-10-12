
/**
 * @file
 * CadworkOrbitManipulator class implementation.
 *
 * @author PCJohn (Jan Pečiva)
 */


#include "CadworkOrbitManipulator.h"
#include "utils/Log.h"

using namespace osg;

static const double homeRotation = 0.7;
static const double homeElevation = 0.61;


/// Constructor.
CadworkOrbitManipulator::CadworkOrbitManipulator( int flags )
   : inherited( flags )
{
   setWheelZoomFactor( -0.1 );
}


/// Constructor.
CadworkOrbitManipulator::CadworkOrbitManipulator( const CadworkOrbitManipulator& com, const CopyOp& copyOp )
   : inherited( com, copyOp )
{
}


bool CadworkOrbitManipulator::performMovementRightMouseButton( const double eventTimeDelta, const double dx, const double dy )
{
   // handle it in the same way as the left button
   return inherited::performMovementLeftMouseButton( eventTimeDelta, dx, dy );
}


void CadworkOrbitManipulator::computeHomePosition( const Camera *camera, bool useBoundingBox )
{
   inherited::computeHomePosition( camera, useBoundingBox );

   // update the home position to "Cadwork-style" one
   setHomePosition( _homeCenter + ( Quat( homeRotation, Vec3d( 0., 0., 1. )) * Quat( homeElevation, Vec3d( -1., 0., 0. ) ) * ( _homeEye - _homeCenter ) ),
                    _homeCenter,
                    Quat( homeElevation, Vec3d( -1., 0., 0. ) ) * _homeUp,
                    _autoComputeHomePosition);
}

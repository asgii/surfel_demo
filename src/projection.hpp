#pragma once

#include <cmath> //for tan, atan

#include "../lib/glad/include/glad/glad.h"

#define GEOM_CPP
#include "../lib/geom/geom.h"

class frustum
{
protected:
   geom::vec3 pos; //Position
   geom::vec3 dirZ, dirY; //Z and Y axes (rotation)
   float horFov, verFov; //Fields of view
   float nearDZ, planesDZ; //distance from position to near plane;
			   //from near plane to far plane

   float getFarDZ() const;
   geom::vec3 getDirX() const;

public:
   frustum(geom::vec3 nuPos, geom::vec3 nuDirZ, geom::vec3 nuDirY,
	   float nuHorFov, float aspRatio,
	   float nuNearDZ, float nuPlanesDZ)
      : pos(nuPos)
      , dirZ (nuDirZ), dirY (nuDirY)
      ,	horFov (radians(nuHorFov / 2.f))
      , verFov (atan(tan(horFov) / aspRatio))
      ,	nearDZ (nuNearDZ), planesDZ (nuPlanesDZ)
   {}

   /*
     Get a matrix factoring in the position and rotation of the camera.
     This must be inverse because you're not actually using it on the
     camera, but on all other objects; the camera can remain at
     origin. That way movement of the camera will be registered visibly
     in the movement of the other objects. (It's done this way because
     it's simpler to render things with the viewpoint at origin. For one
     thing you only need this one transform matrix rather than a
     camera-relative transform for every object.)
   */
   geom::mat4 getInverseTransformMatrix() const;

   //Get a matrix factoring in the properties of the 'lens' and the
   //application of perspective (ie dividing x and y by z).
   geom::mat4 getPerspectiveMatrix() const;

   geom::vec3 getPos() const;
   void setPos(geom::vec3 nuPos);

   geom::vec3 getZ() const;
   geom::vec3 getY() const { return dirY; } //TODO remove

   void setAspectRatio(float aspRatio);
};

class camera : public frustum
{
protected:
   GLint transformLoc;

   void rotate(geom::axisAngle aa);
   
public:
   camera(GLint transfLoc,
	  geom::vec3 nuPos, geom::vec3 nuDirZ, geom::vec3 nuDirY,
	  float nuHorFov, float aspRatio,
	  float nuNearDZ, float nuPlanesDZ)
      : frustum(nuPos, nuDirZ, nuDirY,
		nuHorFov, aspRatio,
		nuNearDZ, nuPlanesDZ)
      , transformLoc (transfLoc)
   {}
   
   void pushTransformMatrix();

   //Rotate around Y or X axes
   //(Difference in unsigned/signed to simplify interface with SDL;
   //better would be to have middle interface between the two)
   void rotateY(uint32_t dt, bool ccw);
   void rotateX(int dx);

   void moveZ(uint32_t dt, bool forward);
   void moveX(uint32_t dt, bool right);
};

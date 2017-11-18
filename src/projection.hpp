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

   geom::mat4 getInverseTransformMatrix() const;
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
   void rotateY(uint32_t dt, bool ccw);
   void rotateX(int dx);
   void moveZ(uint32_t dt, bool forward);
};

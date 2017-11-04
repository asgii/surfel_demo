#pragma once

#include <cmath> //for tan, atan

#include "../lib/glad/include/glad/glad.h"

#define GEOM_CPP
#include "../lib/geom/geom.h"

using namespace geom;

class frustum
{
protected:
   vec3 pos; //Position
   vec3 dirZ, dirY; //Z and Y axes (rotation)
   float horFov, verFov; //Fields of view
   float nearDZ, planesDZ; //distance from position to near plane;
			   //from near plane to far plane

   float getFarDZ() const;
   vec3 getDirX() const;

public:
   frustum(vec3 nuPos, vec3 nuDirZ, vec3 nuDirY,
	   float nuHorFov, float aspRatio,
	   float nuNearDZ, float nuPlanesDZ)
      : pos(nuPos)
      , dirZ (nuDirZ), dirY (nuDirY)
      ,	horFov (radians(nuHorFov / 2.f))
      , verFov (atan(tan(horFov) / aspRatio))
      ,	nearDZ (nuNearDZ), planesDZ (nuPlanesDZ)
   {}

   mat4 getInverseTransformMatrix() const;
   mat4 getPerspectiveMatrix() const;

   vec3 getPos() const;
   void setPos(vec3 nuPos);

   vec3 getZ() const;
   vec3 getY() const { return dirY; } //TODO remove

   void setAspectRatio(float aspRatio);
};

class camera : public frustum
{
protected:
   GLint transformLoc;
   
public:
   camera(GLint transfLoc, vec3 nuPos, vec3 nuDirZ, vec3 nuDirY,
	  float nuHorFov, float aspRatio,
	  float nuNearDZ, float nuPlanesDZ)
      : frustum(nuPos, nuDirZ, nuDirY,
		nuHorFov, aspRatio,
		nuNearDZ, nuPlanesDZ)
      , transformLoc (transfLoc)
   {}
   
   void pushTransformMatrix();

   void rotate(axisAngle aa);
};

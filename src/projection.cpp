#include "../include/projection.hpp"

#include <cmath>

float frustum::getFarDZ() const
{
   return nearDZ + planesDZ;
}

vec3 frustum::getDirX() const
{
   //By right-hand rule
   return cross(dirZ, dirY);
}

mat4 frustum::getInverseTransformMatrix() const
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
{
   vec3 inversePos = pos.inverse();

   //Don't inverse the directions. But you do need X
   //(You would inverse direction if the camera stored a transitive
   //transform (eg a quat) of an original camera direction; as is, I store the
   //actual current direction in world coords.)
   vec3 dirX = getDirX();

   /*
     I've inlined a matrix mult of the frustum's direction and
     position here. They must be multiplied (rather than simply
     putting the position in the translation slots of the matrix)
     because the rotation is done in camera space, not in world space
     (i.e. around the world origin).

     It's equivalent to this:
   */
#if 0
      return (mat4(dirX[0], dirX[1], dirX[2], 0.f,
		   dirY[0], dirY[1], dirY[2], 0.f,
		   dirZ[0], dirZ[1], dirZ[2], 0.f,
		   0.f, 0.f, 0.f, 1.f)
	   *
	   mat4(1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		inversePos[0], inversePos[1], inversePos[2], 1.f));
#endif

   //You can ignore the 4th row/column of these matrices since they're
   //empty anyway.

   //These are the translation elements of the finished matrix.
   float nuPosX = (dirX[0] * inversePos[0] +
		   dirY[0] * inversePos[1] +
		   dirZ[0] * inversePos[2]);

   float nuPosY = (dirX[1] * inversePos[0] +
		   dirY[1] * inversePos[1] +
		   dirZ[1] * inversePos[2]);

   float nuPosZ = (dirX[2] * inversePos[0] +
		   dirY[2] * inversePos[1] +
		   dirZ[2] * inversePos[2]);

   return mat4(dirX[0], dirY[0], dirZ[0], 0.f,
	       dirX[1], dirY[1], dirZ[1], 0.f,
	       dirX[2], dirY[2], dirZ[2], 0.f,
	       nuPosX, nuPosY, nuPosZ, 1.f);
}

mat4 frustum::getPerspectiveMatrix() const
{
   //Conversion to radians is in constructor now
   //Aspect ratio should be implicit
   float posX = tan(horFov);
   float posY = tan(verFov);

   float zCol = 2 * nearDZ / planesDZ + 1;
   float wCol = -2.f * getFarDZ() * nearDZ / planesDZ;

   mat4 matrix = mat4(1.f / posX, 0.f, 0.f, 0.f,
		      0.f, 1.f / posY, 0.f, 0.f,
		      0.f, 0.f, zCol, 1.f,
		      0.f, 0.f, wCol, 0.f);
   
   return matrix;
}

vec3 frustum::getPos() const
{
   return pos;
}

void frustum::setPos(vec3 nuPos)
{
   pos = nuPos;
}

vec3 frustum::getZ() const
{
   return dirZ;
}

void frustum::setAspectRatio(float aspRatio)
{
   //Compute new verFov.
   verFov = atan(tan(horFov) / aspRatio);
}

void camera::pushTransformMatrix()
{
   mat4 transf = getPerspectiveMatrix() * getInverseTransformMatrix();

   glUniformMatrix4fv(transformLoc,
		      1,
		      false,
		      (GLfloat*) &transf);
}

void camera::rotate(axisAngle aa)
{
   if (aa.angle() == 0.f) { return; }

   quaternion qu = quaternion(aa);

   dirY = qu.rotate(dirY);
   dirZ = qu.rotate(dirZ);
}

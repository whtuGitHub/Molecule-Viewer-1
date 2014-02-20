#ifndef OBJECT3D_H
#define OBJECT3D_H
#include <GL/glew.h>
#include "math/Vec3.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Euler.h"

class Object3D{
private:
	Vec3* position;
	Euler* rotation;
	Quaternion* quaternion;
	Vec3* scale;
	Mat4<GLfloat> * modelMatrix;
public:
	Object3D();
	virtual ~Object3D();
	Vec3* getPosition();
	Euler* getRotation();
	Vec3* getScale();
	void setPosition(Vec3* position);
	void setRotation(Euler* rotation);
	void setScale(Vec3* scale);
	Mat4<GLfloat> * getModelMatrix();
	void updateModelMatrix();
	void setQuaternion(Quaternion* quaternion);
	Quaternion* getQuaternion();
};

#endif
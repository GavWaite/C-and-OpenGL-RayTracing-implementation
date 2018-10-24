#pragma once

#include "Ray.h"

//Holds material information of a particular object
class Material
{
public:
	Material();
	//Material values used for local illumination equations
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float specularExponent;

	// Coefficients for global illumination
	float Klocal; // From local illumination
	float Kreflectivity; // From reflection
	//float Ktransmission; // From refraction - not implemented
};

//Interface for an object in the scene
class Object
{
public:
	//Constructor
	//@transform The transformation matrix for the object
	//@material The material properties of the object
	Object(const glm::mat4 &transform = glm::mat4(1.0f), const Material &material = Material());

	//Test whether a ray intersects the object
	//@ray The ray that we are testing for intersection
	//@info Object containing information on the intersection between the ray and the object(if any)
	virtual bool Intersect(const Ray &ray, IntersectInfo &info)  { return true; }

	//Retrun the position of the object, according to its transformation matrix
	glm::vec3 Position() const { return glm::vec3(_transform[3][0], _transform[3][1], _transform[3][2]); }

	//Get a pointer to the objects material properties
	const Material *MaterialPtr() const { return &_material; }

protected:
	glm::mat4 _transform;
	Material _material;
};

// A sphere object with radius and centre coordinate attributes
class Sphere : public Object {
public:
	float radius; // The radius of the sphere
	float r_2; // The radius squared - saves doing the calculation multiple times
	glm::vec3 centre; // The center coordinates of the sphere

	Sphere(float r, glm::vec3 c, Material &material) : Object() {
		radius = r;
		r_2 = (r * r); // done to speed up calculations
		centre = c;
		_material = material;
	}
	bool Intersect(const Ray &ray, IntersectInfo &info);
};

// A plane defined by a point that lies on the plane and the normal vector
class Plane : public Object {
public:
	
	glm::vec3 p0; // A plane can be defined as a point on the plane p0
	glm::vec3 n; // and the normal vector n

	Plane(glm::vec3 _p0, glm::vec3 _n, Material &material) : Object() {
		p0 = _p0;
		n = glm::normalize(_n); // make sure that the normla is indeed normal
		_material = material;
	}
	bool Intersect(const Ray &ray, IntersectInfo &info);
};

// A triangle can be defined as three points in 3D space
class Triangle : public Object {
public:
	// Vertices of a triangle ABC
	glm::vec3 A;
	glm::vec3 B;
	glm::vec3 C;

	glm::vec3 normal; 

	Triangle(glm::vec3 _a, glm::vec3 _b, glm::vec3 _c, Material &material) : Object() {
		A = _a;
		B = _b;
		C = _c;

		// Doesn't matter if normal is pointing 180 degrees the wrong way as we treat triangles
		// as double-sided
		normal = glm::normalize(glm::cross((B - A), (C - A)));

		_material = material;
	}
	bool Intersect(const Ray &ray, IntersectInfo &info);
};

// An axis-aligned box can be defined by two points in 3D space representing the corners
// This implementation simply takes the min and max X,Y,Z cooridnates of the two points and uses
// them as the boundaries
class AxisAlignedBox : public Object {
public:

	glm::vec3 p1; // one corner
	glm::vec3 p2; // the opposite corner

	AxisAlignedBox(glm::vec3 _p1, glm::vec3 _p2, Material &material) : Object() {
		p1 = _p1;
		p2 = _p2;
		_material = material;
	}

	bool Intersect(const Ray &ray, IntersectInfo &info);
};



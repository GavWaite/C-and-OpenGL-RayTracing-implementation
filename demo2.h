#ifndef _rt_H
#define _rt_H

#include "header.h"

//Utility functions

//An output stream, useful for debugging.
//Allows you to stream glm::vec3 objects to the terminal.
ostream & operator << (ostream & stream, const glm::vec3 & obj)
{
	stream << obj.x << ' ' << obj.y << ' ' << obj.z << ' ';
	return stream;
}

//Utilities
//fmax returns the maximum of 3 floats
// float fmax(float f1,float f2, float f3) {
// 	float f = f1;

// 	if (f < f2) f = f2;
// 	if (f < f3) f = f3;

// 	return f;
// }

//fmin returns the minimum of 3 floats.
float fmin(float f1, float f2, float f3) {
	float f = f1;

	if (f > f2) f = f2;
	if (f > f3) f = f3;

	return f;
}



//Comparison operator for vectors
//Returns true if lhs is greater than rhs
bool operator >(const glm::vec3 &lhs, const glm::vec3 &rhs)
{
	if ( lhs.x > rhs.x && lhs.y > rhs.y && lhs.z > rhs.z ) {
		return true;
	}
	return false;
}
//Returns true if lhs is less than rhs
bool operator <(const glm::vec3 &lhs, const glm::vec3 &rhs)
{
	if ( lhs.x < rhs.x && lhs.y < rhs.y && lhs.z < rhs.z ) {
		return true;
	}
	return false;
}

#endif //_rt_H

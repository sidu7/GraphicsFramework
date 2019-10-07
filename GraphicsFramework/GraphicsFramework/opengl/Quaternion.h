#pragma once
#include "glm/glm.hpp"

class Quaternion
{
public:
	Quaternion() : mQuat(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) {}
	//vector (x,y,z) and scalar s
	Quaternion(float x, float y, float z, float s);
	//vec3 axis and angle
	Quaternion(glm::vec3 axis, float angle);
	//rotation matrix
	Quaternion(glm::mat4 rotation);

	Quaternion& operator = (Quaternion const& value);
	Quaternion& operator * (float const value);
	friend Quaternion& operator * (float const value, Quaternion const& quat);
	Quaternion& operator / (float const value);

	Quaternion& operator + (Quaternion const& value);
	Quaternion& operator * (Quaternion const& value);

	inline glm::vec3 v() const { return glm::vec3(mQuat.x, mQuat.y, mQuat.z); }
	inline float s() const { return mQuat.w; }

	float operator[](unsigned int index);

	float dot(Quaternion const& value) const;
	float length() const;
	void normalize();
	Quaternion conjugate();
	Quaternion inverse();

	//interpolate between two quaternions by a factor
	static Quaternion interpolate(const Quaternion& start, const Quaternion& end, float factor);

	//rotation matrix form of quaternion
	glm::mat4 matrix();

	//returns rotated vector by this quaternion
	glm::vec3 rotate(glm::vec3 vector);
private:
	glm::vec4 mQuat;
};
#include "Quaternion.h"

Quaternion::Quaternion(float x, float y, float z, float s) : mQuat(glm::vec4(x,y,z,s))
{
	//glm::normalize(mQuat);
}

Quaternion::Quaternion(glm::vec3 axis, float angle)
{
	mQuat = glm::vec4(glm::sin(glm::radians(angle)/2) * axis, glm::cos(glm::radians(angle)/2));
}

Quaternion::Quaternion(glm::mat4 rotation)
{
	float s = sqrt(rotation[0][0] + rotation[1][1] + rotation[2][2]) / 2;
	float x = (rotation[1][2] - rotation[2][1]) / (4 * s);
	float y = (rotation[2][0] - rotation[0][2]) / (4 * s);
	float z = (rotation[0][1] - rotation[1][0]) / (4 * s);
	mQuat = glm::vec4(x, y, z, s);
}

Quaternion& Quaternion::operator=(Quaternion const& value)
{
	mQuat = value.mQuat;
	return *this;
}

Quaternion& Quaternion::operator*(float const value)
{
	mQuat *= value;
	return *this;
}

Quaternion& Quaternion::operator/(float const value)
{
	mQuat /= value;
	return *this;
}

Quaternion& Quaternion::operator+(Quaternion const& value)
{
	mQuat += value.mQuat;
	return *this;
}

Quaternion& Quaternion::operator*(Quaternion const& value)
{
	//(vector) s1v2 + s2v1 + v1×v2
	glm::vec3 res = s() * value.v() + value.s() * v() + glm::cross(v(), value.v());
	mQuat.x = res.x;
	mQuat.y = res.y;
	mQuat.z = res.z;

	//(scalar) s1s2 - v1.v2
	mQuat.w = s() * value.s() - glm::dot(v(), value.v());

	return *this;
}

float Quaternion::operator[](unsigned int index)
{
	return mQuat[index];
}

float Quaternion::dot(Quaternion const& value) const
{
	return glm::dot(value.mQuat, mQuat);
}

float Quaternion::length() const
{
	return glm::length(mQuat);
}

void Quaternion::normalize()
{
	glm::normalize(mQuat);
}

Quaternion Quaternion::conjugate()
{
	return Quaternion(-mQuat.x,-mQuat.y,-mQuat.z,mQuat.w);
}

Quaternion Quaternion::inverse()
{
	float l = length();
	return conjugate() / (l * l);
}

Quaternion Quaternion::interpolate(const Quaternion& start, const Quaternion& end, float factor)
{
	float cos = glm::dot(start.mQuat, end.mQuat);

	Quaternion pend = end;
	if (cos < 0.0f)
	{
		cos = -cos;
		pend.mQuat = -pend.mQuat;
	}

	float sclp, sclq;
	if ((1.0f - cos) > 0.0001f) // epsillon
	{
		float omega, sinom;
		omega = std::acos(cos);
		sinom = std::sin(omega);
		sclp = std::sin((1.0f - factor) * omega) / sinom;
		sclq = std::sin(factor * omega) / sinom;
	}
	else
	{
		sclp = 1.0f - factor;
		sclq = factor;
	}

	Quaternion result;
	result.mQuat = sclp * start.mQuat + sclq * pend.mQuat;
	return result;
}

glm::mat4 Quaternion::matrix()
{
	glm::mat4 result = glm::mat4(1.0f);
	result[0][0] = 1 - 2 * (mQuat.y * mQuat.y + mQuat.z * mQuat.z);
	result[1][1] = 1 - 2 * (mQuat.x * mQuat.x + mQuat.z * mQuat.z);
	result[2][2] = 1 - 2 * (mQuat.y * mQuat.y + mQuat.x * mQuat.x);

	result[0][1] = 2 * (mQuat.x * mQuat.y - mQuat.w * mQuat.z);
	result[0][2] = 2 * (mQuat.x * mQuat.z + mQuat.w * mQuat.y);
	result[1][0] = 2 * (mQuat.x * mQuat.y + mQuat.w * mQuat.z);
	result[1][2] = 2 * (mQuat.y * mQuat.z - mQuat.w * mQuat.x);
	result[2][0] = 2 * (mQuat.x * mQuat.z - mQuat.w * mQuat.y);
	result[2][1] = 2 * (mQuat.y * mQuat.z + mQuat.w * mQuat.x);

	return glm::transpose(result);
}

glm::vec3 Quaternion::rotate(glm::vec3 vector)
{
	return glm::vec3((s() * s() - glm::dot(v(), v())) * vector + 2 * (glm::dot(v(), vector)) * v() + 2 * s() * (glm::cross(v(), vector)));
}

Quaternion& operator * (float const value, Quaternion& quat)
{
	return quat * value;
}
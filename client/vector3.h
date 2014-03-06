#pragma once

class VBVector3
{
public:
	VBVector3();
	VBVector3(float x, float y, float z);
	VBVector3(const float* xyz);

public:
	float x, y, z;
};

inline VBVector3::VBVector3()
: x(0), y(0), z(0)
{
}

inline VBVector3::VBVector3(float X, float Y, float Z)
: x(X), y(Y), z(Z)
{
}

inline VBVector3::VBVector3(const float* xyz)
: x(*xyz), y(*(xyz + 1)), z(*(xyz + 2))
{
}

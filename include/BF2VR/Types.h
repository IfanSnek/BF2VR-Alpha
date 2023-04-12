#pragma once
#include <format>

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3 operator+(Vec3 v2)
    {
        v2.x += x;
        v2.y += y;
        v2.z += z;

        return v2;
    }

    Vec3 operator*(Vec3 v2)
    {
        v2.x *= x;
        v2.y *= y;
        v2.z *= z;

        return v2;
    }

    Vec3 operator-(Vec3 v2)
    {
        Vec3 v1 = Vec3(x, y, z);
        v1.x -= v2.x;
        v1.y -= v2.y;
        v1.z -= v2.z;

        return v1;
    }

    std::string toString()
    {
        return std::format("X={:.4f} Y={:.4f} Z={:.4f}", x, y, z);
    }
};

struct Vec4 {
    float x;
    float y;
    float z;
    float w;

    Vec4 operator*(Vec4 v2)
    {
        Vec4 v1 = Vec4(x, y, z, w);
        v1.x *= v2.x;
        v1.y *= v2.y;
        v1.z *= v2.z;
        v1.w *= v2.w;

        return v1;
    }

    Vec4 operator/(Vec4 v2)
    {
        Vec4 v1 = Vec4(x, y, z, w);
        v1.x /= v2.x;
        v1.y /= v2.y;
        v1.z /= v2.z;
        v1.w /= v2.w;

        return v1;
    }

    Vec4 operator+(Vec4 v2)
    {
        v2.x += x;
        v2.y += y;
        v2.z += z;
        v2.w += w;

        return v2;
    }

    Vec3 dropW()
    {
        return Vec3(x, y, z);
    }
};

static inline Vec3 rotateAround(Vec3 point, Vec3 pivot, float rad)
{
    Vec3 out = Vec3(point);

    out.x = pivot.x + (cos(rad) * (point.x - pivot.x)) - (sin(rad) * (point.z - pivot.z));
    out.z = pivot.z + (sin(rad) * (point.x - pivot.x)) + (cos(rad) * (point.z - pivot.z));

    return out;
}

static inline Vec4 appendW(Vec3 v2, float w = 0.f)
{
    Vec4 v1;
    v1.x = v2.x;
    v1.y = v2.y;
    v1.z = v2.z;
    v1.w = w;
    return v1;
}

struct Matrix4 {
    Vec4 x;
    Vec4 y;
    Vec4 z;
    Vec4 o;
};
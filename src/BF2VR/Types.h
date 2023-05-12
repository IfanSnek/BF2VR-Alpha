// Types.h - Basic types for Frostbyte
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

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

    Vec4 rotateByEuler(float eulerX, float eulerY, float eulerZ) {
        // Convert Euler angles to quaternion
        Vec4 eulerQuat;
        float cosX = cos(eulerX / 2);
        float sinX = sin(eulerX / 2);
        float cosY = cos(eulerY / 2);
        float sinY = sin(eulerY / 2);
        float cosZ = cos(eulerZ / 2);
        float sinZ = sin(eulerZ / 2);
        eulerQuat.w = cosX * cosY * cosZ + sinX * sinY * sinZ;
        eulerQuat.x = sinX * cosY * cosZ - cosX * sinY * sinZ;
        eulerQuat.y = cosX * sinY * cosZ + sinX * cosY * sinZ;
        eulerQuat.z = cosX * cosY * sinZ - sinX * sinY * cosZ;

        // Multiply original quaternion by Euler angle quaternion to apply rotation
        Vec4 rotatedQuat;
        rotatedQuat.w = w * eulerQuat.w - x * eulerQuat.x - y * eulerQuat.y - z * eulerQuat.z;
        rotatedQuat.x = w * eulerQuat.x + x * eulerQuat.w + y * eulerQuat.z - z * eulerQuat.y;
        rotatedQuat.y = w * eulerQuat.y - x * eulerQuat.z + y * eulerQuat.w + z * eulerQuat.x;
        rotatedQuat.z = w * eulerQuat.z + x * eulerQuat.y - y * eulerQuat.x + z * eulerQuat.w;

        return rotatedQuat;
    }

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

    std::string toString()
    {
        return std::format("X={:.4f} Y={:.4f} Z={:.4f} W={:.4f}", x, y, z, w);
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
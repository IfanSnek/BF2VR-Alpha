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

    Vec3 normalize() {
        float length = sqrt(x * x + y * y + z * z);
        return Vec3{ x / length, y / length, z / length };
    }

    Vec3 cross(Vec3& v2) {
        return Vec3{ y * v2.z - z * v2.y,
                    z * v2.x - x * v2.z,
                    x * v2.y - y * v2.x };
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

    Vec4 relative(Vec4 q2)
    {
        return Vec4(x / q2.x, y / q2.y, z / q2.z, w / q2.w);
    }

    Vec4 conjugate()
    {
        return Vec4(-x, -y, -z, w);
    }

    Vec4 rotate(Vec4 quat)
    {
        return Vec4(x, y, z, w) * quat;
        Vec4 result;
        result.w = -x * quat.x - y * quat.y - z * quat.z + w * quat.w;
        result.x = x * quat.w + y * quat.z - z * quat.y + w * quat.x;
        result.y = -x * quat.z + y * quat.w + z * quat.x + w * quat.y;
        result.z = x * quat.y - y * quat.x + z * quat.w + w * quat.z;
        return result;
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

    void decompose(Vec3& loc, Vec4& rot)
    {
        float s;
        float tr = x.x + y.y + z.z;
        if (tr > 0) {
            s = sqrt(tr + 1.0f) * 2;
            rot.w = 0.25f * s;
            rot.x = (y.z - z.y) / s;
            rot.y = (z.x - x.z) / s;
            rot.z = (x.y - y.x) / s;
        }
        else if ((x.x > y.y) && (x.x > z.z)) {
            s = sqrt(1.0f + x.x - y.y - z.z) * 2;
            rot.w = (y.z - z.y) / s;
            rot.x = 0.25f * s;
            rot.y = (y.x + x.y) / s;
            rot.z = (z.x + x.z) / s;
        }
        else if (y.y > z.z) {
            s = sqrt(1.0f + y.y - x.x - z.z) * 2;
            rot.w = (z.x - x.z) / s;
            rot.x = (y.x + x.y) / s;
            rot.y = 0.25f * s;
            rot.z = (z.y + y.z) / s;
        }
        else {
            s = sqrt(1.0f + z.z - x.x - y.y) * 2;
            rot.w = (x.y - y.x) / s;
            rot.x = (z.x + x.z) / s;
            rot.y = (z.y + y.z) / s;
            rot.z = 0.25f * s;
        }
        loc = { o.x, o.y, o.z };
    }
};
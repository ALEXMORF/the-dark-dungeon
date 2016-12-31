#include "math.h"

inline v2
make_v2(real32 x, real32 y)
{
    return {x, y};
}

inline v2i
make_v2i(int32 x, int32 y)
{
    v2i result = {x, y};
    return result;
}

inline v2i
v2i_zero()
{
    v2i result = {0, 0};
    return result;
}

inline v2
cast_to_v2(v2i v)
{
    v2 result = {(real32)v.x, (real32)v.y};
    return result;
}

inline bool32
operator==(v2i a, v2i b)
{
    if (a.x == b.x && a.y == b.y)
    {
        return true;
    }
    return false;
}

inline v2
normalize(v2 a)
{
    real32 length = sqrtf(a.x*a.x + a.y*a.y);

    if (a.x)
    {
        a.x /= length;
    }
    if (a.y)
    {
        a.y /= length;
    }
    return a;
}

inline v2
v2_from_angle(real32 angle, real32 len)
{
    v2 result = {};
    result.x = cosf(angle)*len;
    result.y = sinf(angle)*len;
    return result;
}

inline real32
det(v2 a, v2 b)
{
    real32 result = a.x*b.y - a.y*b.x;
    return result;
}

inline real32
dot(v2 a, v2 b)
{
    real32 result = a.x*b.x + a.y*b.y;
    return result;
}

inline v2
operator+(v2 a, v2 b)
{
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline v2
operator-(v2 a, v2 b)
{
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline v2
operator*(v2 a, real32 scale)
{
    v2 result;
    result.x = a.x * scale;
    result.y = a.y * scale;
    return result;
}

inline v2
operator/(v2 a, real32 scale)
{
    v2 result;
    result.x = a.x / scale;
    result.y = a.y / scale;
    return result;
}

inline void
operator-=(v2 &a, v2 b)
{
    a.x -= b.x;
    a.y -= b.y;
}

inline void
operator+=(v2 &a, v2 b)
{
    a.x += b.x;
    a.y += b.y;
}

inline void
operator/=(v2 &a, real32 scale)
{
    a.x /= scale;
    a.y /= scale;
}

inline void
operator*=(v2 &a, real32 scale)
{
    a.x *= scale;
    a.y *= scale;
}

inline v2i
operator+(v2i a, v2i b)
{
    v2i result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline v2i
operator-(v2i a, v2i b)
{
    v2i result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline v2i
operator*(v2i a, int32 scale)
{
    v2i result;
    result.x = a.x * scale;
    result.y = a.y * scale;
    return result;
}

inline v2i
operator/(v2i a, int32 scale)
{
    v2i result;
    result.x = a.x / scale;
    result.y = a.y / scale;
    return result;
}

inline void
operator-=(v2i &a, v2i b)
{
    a.x -= b.x;
    a.y -= b.y;
}

inline void
operator+=(v2i &a, v2i b)
{
    a.x += b.x;
    a.y += b.y;
}

inline void
operator/=(v2i &a, int32 scale)
{
    a.x /= scale;
    a.y /= scale;
}

inline void
operator*=(v2i &a, int32 scale)
{
    a.x *= scale;
    a.y *= scale;
}

inline real32
degree_to_radian(real32 degree)
{
    real32 result = degree;
    result /= 180.0f;
    result *= pi32;
    return result;
}

inline real32
radian_to_degree(real32 radian)
{
    real32 result = radian;
    result /= pi32;
    result *= 180.0f;
    return result;
}

inline void
recanonicalize_angle(real32 *angle)
{
    while (*angle >= pi32*2.0f)
    {
        *angle -= pi32*2.0f;
    }
    while (*angle < 0.0f)
    {
        *angle += pi32*2.0f;
    }
}

inline real32
get_angle(v2 a)
{
    return atan2f(a.y, a.x);
}

inline real32
get_angle_diff(real32 a, real32 b)
{
    real32 result = 0.0f;
    v2 unit_a = v2_from_angle(a, 1.0f);
    v2 unit_b = v2_from_angle(b, 1.0f);

    real32 dot_value = dot(unit_a, unit_b);
    real32 det_value = det(unit_a, unit_b);

    result = atan2f(det_value, dot_value);
    return result;
}

inline real32
lerp(real32 start, real32 end, real32 t)
{
    real32 result = start * (1.0f - t) + end * t;
    return result;
}

inline v2
lerp(v2 start, v2 end, real32 t)
{
    v2 result = {};
    result.x = lerp(start.x, end.x, t);
    result.y = lerp(start.y, end.y, t);
    return result;
}

inline real32
approach(real32 start, real32 end, real32 t)
{
    real32 diff = end - start;

    if (diff > t)
        return start + t;
    if (diff < -t)
        return start - t;

    return end;
}

inline v2
approach(v2 start, v2 end, real32 t)
{
    return {approach(start.x, end.x, t), approach(start.y, end.y, t)};
}

inline real32
len(v2 a)
{
    return sqrtf(a.x*a.x + a.y*a.y);
}

inline real32
len_squared(v2 a)
{
    return (a.x*a.x + a.y*a.y);
}

inline real32
reduce(real32 a, real32 da)
{
    return (a > da? (a - da): 0.0f);
}

inline real32
clamp(real32 value, real32 min, real32 max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}


#include "math.h"

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

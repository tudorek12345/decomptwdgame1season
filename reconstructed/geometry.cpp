#include "geometry.hpp"
namespace twd {
static float dot(const Vec3& a, const Vec3& b) {
    // Original computes x/y products and their sum before adding z, then stores float.
    return static_cast<float>((double(a.x)*b.x + double(a.y)*b.y) + double(a.z)*b.z);
}
Vec3* closest_point_on_segment(Vec3* out, const Vec3* start, const Vec3* end, const Vec3* point) {
    const Vec3 a=*start, b=*end, p=*point;
    const Vec3 d{b.x-a.x,b.y-a.y,b.z-a.z};
    const float dp=dot(d,p), da=dot(d,a);
    const float numerator=dp-da, denominator=dot(d,d);
    if(numerator<0.0f) { *out=a; return out; }
    if(denominator<numerator) { *out=b; return out; }
    const float t=numerator/denominator;
    const Vec3 scaled{d.x*t,d.y*t,d.z*t};
    *out={a.x+scaled.x,a.y+scaled.y,a.z+scaled.z};
    return out;
}
}

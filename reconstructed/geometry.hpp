#pragma once
namespace twd {
struct Vec3 { float x, y, z; };
// Reconstructed from WalkingDead101.exe RVA 0x000cbfc0.
// Inferred semantic name; original name is unknown. Degenerate segments yield NaNs.
Vec3* closest_point_on_segment(Vec3* out, const Vec3* start, const Vec3* end, const Vec3* point);
}

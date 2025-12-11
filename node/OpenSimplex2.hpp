#ifndef OPENSIMPLEX2_HPP
#define OPENSIMPLEX2_HPP

#include <cstdint>

/**
 * OpenSimplex 2 (Fast)
 * 
 * Successor to OpenSimplex Noise.
 * Based on K.jpg's OpenSimplex 2 (January 2022 version).
 * Translated from Rust implementation.
 */
class OpenSimplex2 {
public:
    // 2D Noise
    static float noise2(int64_t seed, double x, double y);
    static float noise2_ImproveX(int64_t seed, double x, double y);

    // 3D Noise
    static float noise3_ImproveXY(int64_t seed, double x, double y, double z);
    static float noise3_ImproveXZ(int64_t seed, double x, double y, double z);
    static float noise3_Fallback(int64_t seed, double x, double y, double z);

    // 4D Noise
    static float noise4_ImproveXYZ_ImproveXY(int64_t seed, double x, double y, double z, double w);
    static float noise4_ImproveXYZ_ImproveXZ(int64_t seed, double x, double y, double z, double w);
    static float noise4_ImproveXYZ(int64_t seed, double x, double y, double z, double w);
    static float noise4_ImproveXY_ImproveZW(int64_t seed, double x, double y, double z, double w);
    static float noise4_Fallback(int64_t seed, double x, double y, double z, double w);

private:
    static float noise2_UnskewedBase(int64_t seed, double xs, double ys);
    static float noise3_UnrotatedBase(int64_t seed, double xr, double yr, double zr);
    static float noise4_UnskewedBase(int64_t seed, double xs, double ys, double zs, double ws);
};

/**
 * OpenSimplex 2S (Smooth)
 * 
 * Smoother variant of OpenSimplex 2.
 * Based on K.jpg's OpenSimplex 2S (January 2022 version).
 * Translated from Rust implementation.
 */
class OpenSimplex2S {
public:
    // 2D Noise
    static float noise2(int64_t seed, double x, double y);
    static float noise2_ImproveX(int64_t seed, double x, double y);

    // 3D Noise
    static float noise3_ImproveXY(int64_t seed, double x, double y, double z);
    static float noise3_ImproveXZ(int64_t seed, double x, double y, double z);
    static float noise3_Fallback(int64_t seed, double x, double y, double z);

    // 4D Noise
    static float noise4_ImproveXYZ_ImproveXY(int64_t seed, double x, double y, double z, double w);
    static float noise4_ImproveXYZ_ImproveXZ(int64_t seed, double x, double y, double z, double w);
    static float noise4_ImproveXYZ(int64_t seed, double x, double y, double z, double w);
    static float noise4_ImproveXY_ImproveZW(int64_t seed, double x, double y, double z, double w);
    static float noise4_Fallback(int64_t seed, double x, double y, double z, double w);

private:
    static float noise2_UnskewedBase(int64_t seed, double xs, double ys);
    static float noise3_UnrotatedBase(int64_t seed, double xr, double yr, double zr);
    static float noise4_UnskewedBase(int64_t seed, double xs, double ys, double zs, double ws);
};

#endif // OPENSIMPLEX2_HPP

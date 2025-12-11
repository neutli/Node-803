#include "OpenSimplex2.hpp"
#include <cmath>
#include <vector>
#include <mutex>

namespace {

    // Common Constants
    const int64_t PRIME_X = 0x5205402B9270C86F;
    const int64_t PRIME_Y = 0x598CD327003817B5;
    const int64_t PRIME_Z = 0x5BCC226E9FA0BACB;
    const int64_t PRIME_W = 0x56CC5227E58F554B;
    const int64_t HASH_MULTIPLIER = 0x53A3F72DEEC546F5;
    
    // Fast Variant Constants
    namespace Fast {
        const int64_t SEED_FLIP_3D = -0x52D547B2E96ED629; 
        const int64_t SEED_OFFSET_4D = 0xE83DC3E0DA7164D;
        
        const double ROOT2OVER2 = 0.7071067811865476;
        const double SKEW_2D = 0.366025403784439;
        const double UNSKEW_2D = -0.21132486540518713;
        
        const double ROOT3OVER3 = 0.577350269189626;
        const double FALLBACK_ROTATE_3D = 2.0 / 3.0; // Note: specific to Fast
        const double ROTATE_3D_ORTHOGONALIZER = UNSKEW_2D;
        
        const float SKEW_4D = -0.138196601125011f;
        const float UNSKEW_4D = 0.309016994374947f;
        const float LATTICE_STEP_4D = 0.2f;

        const int N_GRADS_2D_EXPONENT = 7;
        const int N_GRADS_3D_EXPONENT = 8;
        const int N_GRADS_4D_EXPONENT = 9;
        const int N_GRADS_2D = 1 << N_GRADS_2D_EXPONENT;
        const int N_GRADS_3D = 1 << N_GRADS_3D_EXPONENT;
        const int N_GRADS_4D = 1 << N_GRADS_4D_EXPONENT;

        const double NORMALIZER_2D = 0.01001634121365712;
        const double NORMALIZER_3D = 0.07969837668935331;
        const double NORMALIZER_4D = 0.0220065933241897;

        const float RSQUARED_2D = 0.5f;
        const float RSQUARED_3D = 0.6f;
        const float RSQUARED_4D = 0.6f;

        // Gradient Data
        struct Gradients {
            std::vector<float> gradients2D;
            std::vector<float> gradients3D;
            std::vector<float> gradients4D;
        };

        const double GRAD2_SRC[] = {
             0.38268343236509,   0.923879532511287,
             0.923879532511287,  0.38268343236509,
             0.923879532511287, -0.38268343236509,
             0.38268343236509,  -0.923879532511287,
            -0.38268343236509,  -0.923879532511287,
            -0.923879532511287, -0.38268343236509,
            -0.923879532511287,  0.38268343236509,
            -0.38268343236509,   0.923879532511287,
             0.130526192220052,  0.99144486137381,
             0.608761429008721,  0.793353340291235,
             0.793353340291235,  0.608761429008721,
             0.99144486137381,   0.130526192220051,
             0.99144486137381,  -0.130526192220051,
             0.793353340291235, -0.60876142900872,
             0.608761429008721, -0.793353340291235,
             0.130526192220052, -0.99144486137381,
            -0.130526192220052, -0.99144486137381,
            -0.608761429008721, -0.793353340291235,
            -0.793353340291235, -0.608761429008721,
            -0.99144486137381,  -0.130526192220052,
            -0.99144486137381,   0.130526192220051,
            -0.793353340291235,  0.608761429008721,
            -0.608761429008721,  0.793353340291235,
            -0.130526192220052,  0.99144486137381,
        };

        const double GRAD3_SRC[] = {
             2.22474487139,       2.22474487139,      -1.0,                 0.0,
             2.22474487139,       2.22474487139,       1.0,                 0.0,
             3.0862664687972017,  1.1721513422464978,  0.0,                 0.0,
             1.1721513422464978,  3.0862664687972017,  0.0,                 0.0,
            -2.22474487139,       2.22474487139,      -1.0,                 0.0,
            -2.22474487139,       2.22474487139,       1.0,                 0.0,
            -1.1721513422464978,  3.0862664687972017,  0.0,                 0.0,
            -3.0862664687972017,  1.1721513422464978,  0.0,                 0.0,
            -1.0,                -2.22474487139,      -2.22474487139,       0.0,
             1.0,                -2.22474487139,      -2.22474487139,       0.0,
             0.0,                -3.0862664687972017, -1.1721513422464978,  0.0,
             0.0,                -1.1721513422464978, -3.0862664687972017,  0.0,
            -1.0,                -2.22474487139,       2.22474487139,       0.0,
             1.0,                -2.22474487139,       2.22474487139,       0.0,
             0.0,                -1.1721513422464978,  3.0862664687972017,  0.0,
             0.0,                -3.0862664687972017,  1.1721513422464978,  0.0,
            -2.22474487139,      -2.22474487139,      -1.0,                 0.0,
            -2.22474487139,      -2.22474487139,       1.0,                 0.0,
            -3.0862664687972017, -1.1721513422464978,  0.0,                 0.0,
            -1.1721513422464978, -3.0862664687972017,  0.0,                 0.0,
            -2.22474487139,      -1.0,                -2.22474487139,       0.0,
            -2.22474487139,       1.0,                -2.22474487139,       0.0,
            -1.1721513422464978,  0.0,                -3.0862664687972017,  0.0,
            -3.0862664687972017,  0.0,                -1.1721513422464978,  0.0,
            -2.22474487139,      -1.0,                 2.22474487139,       0.0,
            -2.22474487139,       1.0,                 2.22474487139,       0.0,
            -3.0862664687972017,  0.0,                 1.1721513422464978,  0.0,
            -1.1721513422464978,  0.0,                 3.0862664687972017,  0.0,
            -1.0,                 2.22474487139,      -2.22474487139,       0.0,
             1.0,                 2.22474487139,      -2.22474487139,       0.0,
             0.0,                 1.1721513422464978, -3.0862664687972017,  0.0,
             0.0,                 3.0862664687972017, -1.1721513422464978,  0.0,
            -1.0,                 2.22474487139,       2.22474487139,       0.0,
             1.0,                 2.22474487139,       2.22474487139,       0.0,
             0.0,                 3.0862664687972017,  1.1721513422464978,  0.0,
             0.0,                 1.1721513422464978,  3.0862664687972017,  0.0,
             2.22474487139,      -2.22474487139,      -1.0,                 0.0,
             2.22474487139,      -2.22474487139,       1.0,                 0.0,
             1.1721513422464978, -3.0862664687972017,  0.0,                 0.0,
             3.0862664687972017, -1.1721513422464978,  0.0,                 0.0,
             2.22474487139,      -1.0,                -2.22474487139,       0.0,
             2.22474487139,       1.0,                -2.22474487139,       0.0,
             3.0862664687972017,  0.0,                -1.1721513422464978,  0.0,
             1.1721513422464978,  0.0,                -3.0862664687972017,  0.0,
             2.22474487139,      -1.0,                 2.22474487139,       0.0,
             2.22474487139,       1.0,                 2.22474487139,       0.0,
             1.1721513422464978,  0.0,                 3.0862664687972017,  0.0,
             3.0862664687972017,  0.0,                 1.1721513422464978,  0.0,
        };

        const double GRAD4_SRC[] = {
            -0.6740059517812944,   -0.3239847771997537,   -0.3239847771997537,    0.5794684678643381,
            -0.7504883828755602,   -0.4004672082940195,    0.15296486218853164,   0.5029860367700724,
            -0.7504883828755602,    0.15296486218853164,  -0.4004672082940195,    0.5029860367700724,
            -0.8828161875373585,    0.08164729285680945,   0.08164729285680945,   0.4553054119602712,
            -0.4553054119602712,   -0.08164729285680945,  -0.08164729285680945,   0.8828161875373585,
            -0.5029860367700724,   -0.15296486218853164,   0.4004672082940195,    0.7504883828755602,
            -0.5029860367700724,    0.4004672082940195,   -0.15296486218853164,   0.7504883828755602,
            -0.5794684678643381,    0.3239847771997537,    0.3239847771997537,    0.6740059517812944,
            -0.6740059517812944,   -0.3239847771997537,    0.5794684678643381,   -0.3239847771997537,
            -0.7504883828755602,   -0.4004672082940195,    0.5029860367700724,    0.15296486218853164,
            -0.7504883828755602,    0.15296486218853164,   0.5029860367700724,   -0.4004672082940195,
            -0.8828161875373585,    0.08164729285680945,   0.4553054119602712,    0.08164729285680945,
            -0.4553054119602712,   -0.08164729285680945,   0.8828161875373585,   -0.08164729285680945,
            -0.5029860367700724,   -0.15296486218853164,   0.7504883828755602,    0.4004672082940195,
            -0.5029860367700724,    0.4004672082940195,    0.7504883828755602,   -0.15296486218853164,
            -0.5794684678643381,    0.3239847771997537,    0.6740059517812944,    0.3239847771997537,
            -0.6740059517812944,    0.5794684678643381,   -0.3239847771997537,   -0.3239847771997537,
            -0.7504883828755602,    0.5029860367700724,   -0.4004672082940195,    0.15296486218853164,
            -0.7504883828755602,    0.5029860367700724,    0.15296486218853164,  -0.4004672082940195,
            -0.8828161875373585,    0.4553054119602712,    0.08164729285680945,   0.08164729285680945,
            -0.4553054119602712,    0.8828161875373585,   -0.08164729285680945,  -0.08164729285680945,
            -0.5029860367700724,    0.7504883828755602,   -0.15296486218853164,   0.4004672082940195,
            -0.5029860367700724,    0.7504883828755602,    0.4004672082940195,   -0.15296486218853164,
            -0.5794684678643381,    0.6740059517812944,    0.3239847771997537,    0.3239847771997537,
             0.5794684678643381,   -0.6740059517812944,   -0.3239847771997537,   -0.3239847771997537,
             0.5029860367700724,   -0.7504883828755602,   -0.4004672082940195,    0.15296486218853164,
             0.5029860367700724,   -0.7504883828755602,    0.15296486218853164,  -0.4004672082940195,
             0.4553054119602712,   -0.8828161875373585,    0.08164729285680945,   0.08164729285680945,
             0.8828161875373585,   -0.4553054119602712,   -0.08164729285680945,  -0.08164729285680945,
             0.7504883828755602,   -0.5029860367700724,   -0.15296486218853164,   0.4004672082940195,
             0.7504883828755602,   -0.5029860367700724,    0.4004672082940195,   -0.15296486218853164,
             0.6740059517812944,   -0.5794684678643381,    0.3239847771997537,    0.3239847771997537,
            -0.753341017856078,    -0.37968289875261624,  -0.37968289875261624,  -0.37968289875261624,
            -0.7821684431180708,   -0.4321472685365301,   -0.4321472685365301,    0.12128480194602098,
            -0.7821684431180708,   -0.4321472685365301,    0.12128480194602098,  -0.4321472685365301,
            -0.7821684431180708,    0.12128480194602098,  -0.4321472685365301,   -0.4321472685365301,
            -0.8586508742123365,   -0.508629699630796,     0.044802370851755174,  0.044802370851755174,
            -0.8586508742123365,    0.044802370851755174, -0.508629699630796,     0.044802370851755174,
            -0.8586508742123365,    0.044802370851755174,  0.044802370851755174, -0.508629699630796,
            -0.9982828964265062,   -0.03381941603233842,  -0.03381941603233842,  -0.03381941603233842,
            -0.37968289875261624,  -0.753341017856078,    -0.37968289875261624,  -0.37968289875261624,
            -0.4321472685365301,   -0.7821684431180708,   -0.4321472685365301,    0.12128480194602098,
            -0.4321472685365301,   -0.7821684431180708,    0.12128480194602098,  -0.4321472685365301,
             0.12128480194602098,  -0.7821684431180708,   -0.4321472685365301,   -0.4321472685365301,
            -0.508629699630796,    -0.8586508742123365,    0.044802370851755174,  0.044802370851755174,
             0.044802370851755174, -0.8586508742123365,   -0.508629699630796,     0.044802370851755174,
             0.044802370851755174, -0.8586508742123365,    0.044802370851755174, -0.508629699630796,
            -0.03381941603233842,  -0.9982828964265062,   -0.03381941603233842,  -0.03381941603233842,
            -0.37968289875261624,  -0.37968289875261624,  -0.753341017856078,    -0.37968289875261624,
            -0.4321472685365301,   -0.4321472685365301,   -0.7821684431180708,    0.12128480194602098,
            -0.4321472685365301,    0.12128480194602098,  -0.7821684431180708,   -0.4321472685365301,
             0.12128480194602098,  -0.4321472685365301,   -0.7821684431180708,   -0.4321472685365301,
            -0.508629699630796,     0.044802370851755174, -0.8586508742123365,    0.044802370851755174,
             0.044802370851755174, -0.508629699630796,    -0.8586508742123365,    0.044802370851755174,
             0.044802370851755174,  0.044802370851755174, -0.8586508742123365,   -0.508629699630796,
            -0.03381941603233842,  -0.03381941603233842,  -0.9982828964265062,   -0.03381941603233842,
            -0.3239847771997537,   -0.6740059517812944,   -0.3239847771997537,    0.5794684678643381,
            -0.4004672082940195,   -0.7504883828755602,    0.15296486218853164,   0.5029860367700724,
             0.15296486218853164,  -0.7504883828755602,   -0.4004672082940195,    0.5029860367700724,
             0.08164729285680945,  -0.8828161875373585,    0.08164729285680945,   0.4553054119602712,
            -0.08164729285680945,  -0.4553054119602712,   -0.08164729285680945,   0.8828161875373585,
            -0.15296486218853164,  -0.5029860367700724,    0.4004672082940195,    0.7504883828755602,
             0.4004672082940195,   -0.5029860367700724,   -0.15296486218853164,   0.7504883828755602,
             0.3239847771997537,   -0.5794684678643381,    0.3239847771997537,    0.6740059517812944,
            -0.3239847771997537,   -0.3239847771997537,   -0.6740059517812944,    0.5794684678643381,
            -0.4004672082940195,    0.15296486218853164,  -0.7504883828755602,    0.5029860367700724,
             0.15296486218853164,  -0.4004672082940195,   -0.7504883828755602,    0.5029860367700724,
             0.08164729285680945,   0.08164729285680945,  -0.8828161875373585,    0.4553054119602712,
            -0.08164729285680945,  -0.08164729285680945,  -0.4553054119602712,    0.8828161875373585,
            -0.15296486218853164,   0.4004672082940195,   -0.5029860367700724,    0.7504883828755602,
             0.4004672082940195,   -0.15296486218853164,  -0.5029860367700724,    0.7504883828755602,
             0.3239847771997537,    0.3239847771997537,   -0.5794684678643381,    0.6740059517812944,
            -0.3239847771997537,   -0.6740059517812944,    0.5794684678643381,   -0.3239847771997537,
            -0.4004672082940195,   -0.7504883828755602,    0.5029860367700724,    0.15296486218853164,
        };
        
        static std::once_flag init_flag;
        static Gradients gradients;

        void initGradients() {
            // Allocate arrays 
            // 2D
            gradients.gradients2D.resize(N_GRADS_2D * 2);
            for (int i = 0; i < N_GRADS_2D * 2; ++i) {
                int srcIdx = i % (sizeof(GRAD2_SRC)/sizeof(double));
                gradients.gradients2D[i] = (float)(GRAD2_SRC[srcIdx] / NORMALIZER_2D);
            }
            
            // 3D
            gradients.gradients3D.resize(N_GRADS_3D * 4); 
            for (int i = 0; i < N_GRADS_3D * 4; ++i) {
                int srcIdx = i % (sizeof(GRAD3_SRC)/sizeof(double));
                 gradients.gradients3D[i] = (float)(GRAD3_SRC[srcIdx] / NORMALIZER_3D);
            }
            
            // 4D
             gradients.gradients4D.resize(N_GRADS_4D * 4);
             for (int i = 0; i < N_GRADS_4D * 4; ++i) {
                int srcIdx = i % (sizeof(GRAD4_SRC)/sizeof(double));
                 gradients.gradients4D[i] = (float)(GRAD4_SRC[srcIdx] / NORMALIZER_4D);
            }
        }

        const Gradients& getGradients() {
            std::call_once(init_flag, initGradients);
            return gradients;
        }

        inline int fastFloor(double x) {
            int xi = (int)x;
            return x < xi ? xi - 1 : xi;
        }
        
        inline int fastRound(double x) {
            return (x < 0) ? (int)(x - 0.5) : (int)(x + 0.5);
        }
    }
}

// ============================================================================
// OpenSimplex2 Implementation
// ============================================================================

namespace {
    using namespace Fast;

    inline float grad2(int64_t seed, int64_t xsvp, int64_t ysvp, float dx, float dy) {
        int64_t hash = seed ^ xsvp ^ ysvp;
        hash = (int64_t)((uint64_t)hash * (uint64_t)HASH_MULTIPLIER);
        hash ^= hash >> (64 - N_GRADS_2D_EXPONENT + 1);
        int gi = (int)(hash & ((N_GRADS_2D - 1) << 1));
        const auto& grads = getGradients().gradients2D;
        return grads[gi] * dx + grads[gi + 1] * dy;
    }

    inline float grad3(int64_t seed, int64_t xrvp, int64_t yrvp, int64_t zrvp, float dx, float dy, float dz) {
        int64_t hash = (seed ^ xrvp) ^ (yrvp ^ zrvp);
        hash = (int64_t)((uint64_t)hash * (uint64_t)HASH_MULTIPLIER);
        hash ^= hash >> (64 - N_GRADS_3D_EXPONENT + 2);
        int gi = (int)(hash & ((N_GRADS_3D - 1) << 2));
        const auto& grads = getGradients().gradients3D;
        return grads[gi] * dx + grads[gi + 1] * dy + grads[gi + 2] * dz;
    }
    
    inline float grad4(int64_t seed, int64_t xsvp, int64_t ysvp, int64_t zsvp, int64_t wsvp, float dx, float dy, float dz, float dw) {
        int64_t hash = seed ^ (xsvp ^ ysvp) ^ (zsvp ^ wsvp);
        hash = (int64_t)((uint64_t)hash * (uint64_t)HASH_MULTIPLIER);
        hash ^= hash >> (64 - N_GRADS_4D_EXPONENT + 2);
        int gi = (int)(hash & ((N_GRADS_4D - 1) << 2));
        const auto& grads = getGradients().gradients4D;
        return grads[gi] * dx + grads[gi + 1] * dy + grads[gi + 2] * dz + grads[gi + 3] * dw;
    }
}

float OpenSimplex2::noise2_UnskewedBase(int64_t seed, double xs, double ys) {
    int xsb = fastFloor(xs);
    int ysb = fastFloor(ys);
    float xi = (float)(xs - xsb);
    float yi = (float)(ys - ysb);

    int64_t xsbp = (int64_t)((uint64_t)xsb * (uint64_t)PRIME_X);
    int64_t ysbp = (int64_t)((uint64_t)ysb * (uint64_t)PRIME_Y);

    float t = (xi + yi) * (float)UNSKEW_2D;
    float dx0 = xi + t;
    float dy0 = yi + t;

    float value = 0.0f;
    float a0 = RSQUARED_2D - dx0 * dx0 - dy0 * dy0;
    if (a0 > 0.0f) {
        value = (a0 * a0) * (a0 * a0) * grad2(seed, xsbp, ysbp, dx0, dy0);
    }
    
    float a1 = (float)(2.0 * (1.0 + 2.0 * UNSKEW_2D) * (1.0 / UNSKEW_2D + 2.0)) * t
               + (float)((-2.0 * (1.0 + 2.0 * UNSKEW_2D) * (1.0 + 2.0 * UNSKEW_2D)) + a0);
               
    if (a1 > 0.0f) {
        float dx1 = dx0 - (float)(1.0 + 2.0 * UNSKEW_2D);
        float dy1 = dy0 - (float)(1.0 + 2.0 * UNSKEW_2D);
        value += (a1 * a1) * (a1 * a1) * grad2(seed, (int64_t)((uint64_t)xsbp + (uint64_t)PRIME_X), (int64_t)((uint64_t)ysbp + (uint64_t)PRIME_Y), dx1, dy1);
    }
    
    if (dy0 > dx0) {
        float dx2 = dx0 - (float)UNSKEW_2D;
        float dy2 = dy0 - (float)(UNSKEW_2D + 1.0);
        float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
        if (a2 > 0.0f) {
            value += (a2 * a2) * (a2 * a2) * grad2(seed, xsbp, (int64_t)((uint64_t)ysbp + (uint64_t)PRIME_Y), dx2, dy2);
        }
    } else {
        float dx2 = dx0 - (float)(UNSKEW_2D + 1.0);
        float dy2 = dy0 - (float)UNSKEW_2D;
        float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
        if (a2 > 0.0f) {
            value += (a2 * a2) * (a2 * a2) * grad2(seed, (int64_t)((uint64_t)xsbp + (uint64_t)PRIME_X), ysbp, dx2, dy2);
        }
    }
    return value;
}

float OpenSimplex2::noise2(int64_t seed, double x, double y) {
    double s = SKEW_2D * (x + y);
    return noise2_UnskewedBase(seed, x + s, y + s);
}

float OpenSimplex2::noise2_ImproveX(int64_t seed, double x, double y) {
    double xx = x * ROOT2OVER2;
    double yy = y * (ROOT2OVER2 * (1.0 + 2.0 * SKEW_2D));
    return noise2_UnskewedBase(seed, yy + xx, yy - xx);
}

float OpenSimplex2::noise3_UnrotatedBase(int64_t seed_arg, double xr, double yr, double zr) {
    int64_t seed = seed_arg;
    int xrb = fastRound(xr);
    int yrb = fastRound(yr);
    int zrb = fastRound(zr);
    float xri = (float)(xr - xrb);
    float yri = (float)(yr - yrb);
    float zri = (float)(zr - zrb);

    int xNSign = (int)(-1.0f - xri) | 1;
    int yNSign = (int)(-1.0f - yri) | 1;
    int zNSign = (int)(-1.0f - zri) | 1;

    float ax0 = (float)xNSign * -xri;
    float ay0 = (float)yNSign * -yri;
    float az0 = (float)zNSign * -zri;

    int64_t xrbp = (int64_t)((uint64_t)xrb * (uint64_t)PRIME_X);
    int64_t yrbp = (int64_t)((uint64_t)yrb * (uint64_t)PRIME_Y);
    int64_t zrbp = (int64_t)((uint64_t)zrb * (uint64_t)PRIME_Z);

    float value = 0.0f;
    float a = (RSQUARED_3D - xri * xri) - (yri * yri + zri * zri);
    
    for (int l = 0; ; l++) {
        if (a > 0.0f) {
            value += (a * a) * (a * a) * grad3(seed, xrbp, yrbp, zrbp, xri, yri, zri);
        }

        if (ax0 >= ay0 && ax0 >= az0) {
            float b = a + ax0 + ax0;
            if (b > 1.0f) {
                b -= 1.0f;
                // Note: explicit casting to handle operator precedence and signed/unsigned mix
                value += (b * b) * (b * b) * grad3(seed, xrbp - (int64_t)xNSign * PRIME_X, yrbp, zrbp, xri + (float)xNSign, yri, zri);
            }
        } else if (ay0 > ax0 && ay0 >= az0) {
            float b = a + ay0 + ay0;
            if (b > 1.0f) {
                b -= 1.0f;
                value += (b * b) * (b * b) * grad3(seed, xrbp, yrbp - (int64_t)yNSign * PRIME_Y, zrbp, xri, yri + (float)yNSign, zri);
            }
        } else {
            float b = a + az0 + az0;
            if (b > 1.0f) {
                b -= 1.0f;
                value += (b * b) * (b * b) * grad3(seed, xrbp, yrbp, zrbp - (int64_t)zNSign * PRIME_Z, xri, yri, zri + (float)zNSign);
            }
        }

        if (l == 1) break;

        ax0 = 0.5f - ax0;
        ay0 = 0.5f - ay0;
        az0 = 0.5f - az0;

        xri = (float)xNSign * ax0;
        yri = (float)yNSign * ay0;
        zri = (float)zNSign * az0;
        
        a += (0.75f - ax0) - (ay0 + az0);
        
        xrbp += (xNSign >> 1) & PRIME_X;
        yrbp += (yNSign >> 1) & PRIME_Y;
        zrbp += (zNSign >> 1) & PRIME_Z;
        
        xNSign = -xNSign;
        yNSign = -yNSign;
        zNSign = -zNSign;
        
        seed ^= SEED_FLIP_3D;
    }

    return value;
}

float OpenSimplex2::noise3_ImproveXY(int64_t seed, double x, double y, double z) {
    double xy = x + y;
    double s2 = xy * ROTATE_3D_ORTHOGONALIZER;
    double zz = z * ROOT3OVER3;
    double xr = x + s2 + zz;
    double yr = y + s2 + zz;
    double zr = xy * -ROOT3OVER3 + zz;
    return noise3_UnrotatedBase(seed, xr, yr, zr);
}

float OpenSimplex2::noise3_ImproveXZ(int64_t seed, double x, double y, double z) {
    double xz = x + z;
    double s2 = xz * ROTATE_3D_ORTHOGONALIZER;
    double yy = y * ROOT3OVER3;
    double xr = x + s2 + yy;
    double zr = z + s2 + yy;
    double yr = xz * -ROOT3OVER3 + yy;
    return noise3_UnrotatedBase(seed, xr, yr, zr);
}

float OpenSimplex2::noise3_Fallback(int64_t seed, double x, double y, double z) {
    double r = FALLBACK_ROTATE_3D * (x + y + z);
    return noise3_UnrotatedBase(seed, r - x, r - y, r - z);
}

float OpenSimplex2::noise4_UnskewedBase(int64_t seed_arg, double xs, double ys, double zs, double ws) {
    int64_t seed = seed_arg;
    int xsb = fastFloor(xs);
    int ysb = fastFloor(ys);
    int zsb = fastFloor(zs);
    int wsb = fastFloor(ws);
    
    float xsi = (float)(xs - xsb);
    float ysi = (float)(ys - ysb);
    float zsi = (float)(zs - zsb);
    float wsi = (float)(ws - wsb);
    
    float siSum = (xsi + ysi) + (zsi + wsi);
    int startingLattice = (int)(siSum * 1.25f);
    
    seed += (int64_t)startingLattice * SEED_OFFSET_4D;
    
    float startingLatticeOffset = (float)startingLattice * -LATTICE_STEP_4D;
    xsi += startingLatticeOffset;
    ysi += startingLatticeOffset;
    zsi += startingLatticeOffset;
    wsi += startingLatticeOffset;
    
    float ssi = (siSum + startingLatticeOffset * 4.0f) * UNSKEW_4D;
    
    int64_t xsvp = (int64_t)((uint64_t)xsb * (uint64_t)PRIME_X);
    int64_t ysvp = (int64_t)((uint64_t)ysb * (uint64_t)PRIME_Y);
    int64_t zsvp = (int64_t)((uint64_t)zsb * (uint64_t)PRIME_Z);
    int64_t wsvp = (int64_t)((uint64_t)wsb * (uint64_t)PRIME_W);
    
    float value = 0.0f;
    for (int i = 0; ; i++) {
        float score0 = 1.0f + ssi * (-1.0f / UNSKEW_4D);
        if (xsi >= ysi && xsi >= zsi && xsi >= wsi && xsi >= score0) {
            xsvp += PRIME_X;
            xsi -= 1.0f;
            ssi -= UNSKEW_4D;
        } else if (ysi > xsi && ysi >= zsi && ysi >= wsi && ysi >= score0) {
            ysvp += PRIME_Y;
            ysi -= 1.0f;
            ssi -= UNSKEW_4D;
        } else if (zsi > xsi && zsi > ysi && zsi >= wsi && zsi >= score0) {
            zsvp += PRIME_Z;
            zsi -= 1.0f;
            ssi -= UNSKEW_4D;
        } else if (wsi > xsi && wsi > ysi && wsi > zsi && wsi >= score0) {
            wsvp += PRIME_W;
            wsi -= 1.0f;
            ssi -= UNSKEW_4D;
        }
        
        float dx = xsi + ssi;
        float dy = ysi + ssi;
        float dz = zsi + ssi;
        float dw = wsi + ssi;
        float a = (dx * dx + dy * dy) + (dz * dz + dw * dw);
        if (a < RSQUARED_4D) {
            a -= RSQUARED_4D;
            a *= a;
            value += a * a * grad4(seed, xsvp, ysvp, zsvp, wsvp, dx, dy, dz, dw);
        }
        
        if (i == 4) break;
        
        xsi += LATTICE_STEP_4D;
        ysi += LATTICE_STEP_4D;
        zsi += LATTICE_STEP_4D;
        wsi += LATTICE_STEP_4D;
        ssi += LATTICE_STEP_4D * 4.0f * UNSKEW_4D;
        seed -= SEED_OFFSET_4D;
        
        if (i == startingLattice) {
            xsvp -= PRIME_X;
            ysvp -= PRIME_Y;
            zsvp -= PRIME_Z;
            wsvp -= PRIME_W;
            seed += SEED_OFFSET_4D * 5;
        }
    }
    return value;
}

float OpenSimplex2::noise4_ImproveXYZ_ImproveXY(int64_t seed, double x, double y, double z, double w) {
    double xy = x + y;
    double s2 = xy * -0.21132486540518699998;
    double zz = z * 0.28867513459481294226;
    double ww = w * 0.2236067977499788;
    double xr = x + (zz + ww + s2);
    double yr = y + (zz + ww + s2);
    double zr = xy * -0.57735026918962599998 + (zz + ww);
    double wr = z * -0.866025403784439 + ww;
    return noise4_UnskewedBase(seed, xr, yr, zr, wr);
}

float OpenSimplex2::noise4_ImproveXYZ_ImproveXZ(int64_t seed, double x, double y, double z, double w) {
    double xz = x + z;
    double s2 = xz * -0.21132486540518699998;
    double yy = y * 0.28867513459481294226;
    double ww = w * 0.2236067977499788;
    double xr = x + (yy + ww + s2);
    double zr = z + (yy + ww + s2);
    double yr = xz * -0.57735026918962599998 + (yy + ww);
    double wr = y * -0.866025403784439 + ww;
    return noise4_UnskewedBase(seed, xr, yr, zr, wr);
}

float OpenSimplex2::noise4_ImproveXYZ(int64_t seed, double x, double y, double z, double w) {
    double xyz = x + y + z;
    double ww = w * 0.2236067977499788;
    double s2 = xyz * -0.16666666666666666 + ww;
    double xs = x + s2;
    double ys = y + s2;
    double zs = z + s2;
    double ws = -0.5 * xyz + ww;
    return noise4_UnskewedBase(seed, xs, ys, zs, ws);
}

float OpenSimplex2::noise4_ImproveXY_ImproveZW(int64_t seed, double x, double y, double z, double w) {
    double s2 = (x + y) * -0.178275657951399372 + (z + w) * 0.215623393288842828;
    double t2 = (z + w) * -0.403949762580207112 + (x + y) * -0.375199083010075342;
    double xs = x + s2;
    double ys = y + s2;
    double zs = z + t2;
    double ws = w + t2;
    return noise4_UnskewedBase(seed, xs, ys, zs, ws);
}

float OpenSimplex2::noise4_Fallback(int64_t seed, double x, double y, double z, double w) {
    double s = SKEW_4D * (x + y + z + w);
    return noise4_UnskewedBase(seed, x + s, y + s, z + s, w + s);
}

#ifndef NOISE_H
#define NOISE_H

#include <QVector>
#include <cmath>
#include <random>

// フラクタルタイプ (ノイズの重ね合わせアルゴリズム)
enum class FractalType {
    None,
    FBM,
    Multifractal,
    HybridMultifractal,
    HeteroTerrain,
    RidgedMultifractal,
    Division,
    LinearLight
};

// ノイズの種類 (Basis)
enum class NoiseType {
    OpenSimplex2S,  // OpenSimplex2 Smooth (default)
    OpenSimplex2F,  // OpenSimplex2 Fast (SuperSimplex)
    Perlin,
    Simplex,
    RidgedMultifractal, // Legacy
    White,
    Ridged, // Legacy
    Gabor,
    Everling // Everling Noise (Integrated Gaussian)
};

// Perlinノイズ生成クラス
class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 0);
    
    // 2Dノイズ生成 (0.0 ~ 1.0)
    double noise(double x, double y) const;
    
    // 3Dノイズ生成 (0.0 ~ 1.0)
    double noise(double x, double y, double z) const;
    
    // オクターブノイズ - 複数の周波数を重ね合わせる
    double octaveNoise(double x, double y, int octaves, double persistence = 0.5) const;
    double octaveNoise(double x, double y, double z, int octaves, double persistence = 0.5) const;
    
    // フラクタルブラウン運動 (fBm)
    double fbm(double x, double y, double z, int octaves, double lacunarity = 2.0, double gain = 0.5) const;
    
    // Simplexノイズ (3D)
    double simplexNoise(double x, double y, double z) const;
    
    // OpenSimplex2S (Smooth, default)
    double openSimplex2S(double x, double y, double z) const;
    
    // OpenSimplex2F (Fast/SuperSimplex)
    double openSimplex2F(double x, double y, double z) const;
    
    // Everling Noise (Procedural Texture via Integration)
    // mean: Gaussian Mean (controls bias/tendency)
    // stddev: Gaussian Standard Deviation (controls ruggedness)
    double everlingNoise(double x, double y, double z, double mean, double stddev) const;

    // Ridged Multifractal
    double ridgedMultifractal(double x, double y, double z, int octaves, double lacunarity = 2.0, double gain = 0.5, double offset = 1.0) const;
    
    // White Noise (ランダム)
    double whiteNoise(double x, double y, double z) const;
    
    // Gabor Noise (Anisotropic)
    // anisotropy: 0.0 (isotropic) to 1.0 (highly anisotropic)
    // orientation: 0.0 to 1.0 (rotation)
    double gaborNoise(double x, double y, double z, double frequency, double anisotropy, double orientation) const;



private:
    // パーミュテーションテーブル
    QVector<int> p;
    mutable std::mt19937 m_rng;
    
    // 補間関数
    static double fade(double t);
    static double lerp(double t, double a, double b);
    static double grad(int hash, double x, double y, double z);
    
    // Simplex用
    static double dot(const int g[], double x, double y, double z);
    static int fastfloor(double x);

    // OpenSimplex2F/2S (3D BCC) Lookup
    // Internal seed for OpenSimplex2
    int64_t m_seed64;

    // Everling Noise Cache
    mutable QVector<double> m_everlingBuffer;
    mutable double m_cachedMean = -9999.0;
    mutable double m_cachedStdDev = -9999.0;
    static const int EVERLING_SIZE = 64; // Limit buffer size for performance
    void regenerateEverling(double mean, double stddev) const;
};

#endif // NOISE_H


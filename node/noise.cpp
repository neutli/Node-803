#include "noise.h"
#include "OpenSimplex2.hpp"

PerlinNoise::PerlinNoise(unsigned int seed) {
    // パーミュテーションテーブルの初期化
    p.resize(512);
    
    // 0-255の配列を作成
    QVector<int> permutation(256);
    for (int i = 0; i < 256; i++) {
        permutation[i] = i;
    }
    
    // シャッフル
    std::mt19937 rng(seed);
    for (int i = 255; i > 0; i--) {
        int j = rng() % (i + 1);
        std::swap(permutation[i], permutation[j]);
    }
    
    // テーブルを2回繰り返す（オーバーフロー防止）
    for (int i = 0; i < 256; i++) {
        p[i] = permutation[i];
        p[256 + i] = permutation[i];
    }
    for (int i = 0; i < 256; i++) {
        p[i] = permutation[i];
        p[256 + i] = permutation[i];
    }
    m_seed64 = static_cast<int64_t>(seed);
}



double PerlinNoise::fade(double t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double PerlinNoise::noise(double x, double y, double z) const {
    // グリッドセルの座標を求める
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    // セル内の相対座標
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    // フェード曲線
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);
    
    // ハッシュ座標
    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;
    
    // 8つの角の勾配を補間
    double res = lerp(w,
        lerp(v,
            lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
            lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))
        ),
        lerp(v,
            lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))
        )
    );
    
    // -1.0 ~ 1.0 を 0.0 ~ 1.0 に変換
    return (res + 1.0) / 2.0;
}

double PerlinNoise::noise(double x, double y) const {
    return noise(x, y, 0.0);
}

double PerlinNoise::octaveNoise(double x, double y, int octaves, double persistence) const {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }
    
    return total / maxValue;
}

double PerlinNoise::octaveNoise(double x, double y, double z, int octaves, double persistence) const {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }
    
    return total / maxValue;
}

double PerlinNoise::fbm(double x, double y, double z, int octaves, double lacunarity, double gain) const {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
    
    return total;
}

// Simplex Noise (3D) - 簡易実装
double PerlinNoise::simplexNoise(double x, double y, double z) const {
    // Simplexノイズは複雑なので、ここでは通常のPerlinノイズをベースにした簡易版を使用
    // より滑らかな結果を得るために、少し異なるスケーリングを適用
    double n = noise(x * 0.8, y * 0.8, z * 0.8);
    return n * n * (3.0 - 2.0 * n); // スムーズステップで滑らかに
}

// OpenSimplex2S (Smooth) - 3D Implementation (8-point BCC)
double PerlinNoise::openSimplex2S(double x, double y, double z) const {
    // Call integrated OpenSimplex2S library
    // Map -1..1 to 0..1
    return OpenSimplex2S::noise3_ImproveXZ(m_seed64, x, y, z) * 0.5 + 0.5;
}

// OpenSimplex2F (Fast/SuperSimplex) - 3D Implementation (Rotated BCC)
double PerlinNoise::openSimplex2F(double x, double y, double z) const {
    // Call integrated OpenSimplex2 library (SuperSimplex)
    // Map -1..1 to 0..1
    return OpenSimplex2::noise3_ImproveXZ(m_seed64, x, y, z) * 0.5 + 0.5;
}


// Ridged Multifractal
double PerlinNoise::ridgedMultifractal(double x, double y, double z, int octaves, double lacunarity, double gain, double offset) const {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    
    for (int i = 0; i < octaves; i++) {
        double n = noise(x * frequency, y * frequency, z * frequency);
        // リッジを作るために絶対値を取り、反転
        n = offset - std::abs(n * 2.0 - 1.0);
        n = n * n; // 鋭いリッジを作る
        total += n * amplitude;
        
        frequency *= lacunarity;
        amplitude *= gain;
    }
    
    return total / octaves;
}

// White Noise
double PerlinNoise::whiteNoise(double x, double y, double z) const {
    // 座標をハッシュしてランダムな値を生成
    int ix = static_cast<int>(std::floor(x * 1000.0));
    int iy = static_cast<int>(std::floor(y * 1000.0));
    int iz = static_cast<int>(std::floor(z * 1000.0));
    
    int hash = p[(p[(p[ix & 255] + iy) & 255] + iz) & 255];
    return static_cast<double>(hash) / 255.0;
}

// Gabor Noise (Anisotropic) - Full Complex Result
PerlinNoise::GaborResult PerlinNoise::gaborNoise(double x, double y, double z, double frequency, double anisotropy, const QVector3D& orientation) const {
    double totalReal = 0.0;
    double totalImag = 0.0;
    double omega = 2.0 * M_PI * frequency;
    
    // 3D Orientation - normalize the vector
    QVector3D dir = orientation.normalized();
    if (dir.isNull()) dir = QVector3D(1, 0, 0); // Default direction
    
    // Build rotation matrix to align with direction
    // We need to rotate so that the wave travels along 'dir'
    // For simplicity, we compute the projection onto the direction
    
    // Anisotropy parameters
    // anisotropy 0 -> isotropic (spherical Gaussian)
    // anisotropy 1 -> thin streaks (elongated along dir)
    double bandwidth = 1.0;
    double alpha = bandwidth * bandwidth; // Width along wave direction
    double beta = alpha / (1.0 + anisotropy * 9.0); // Width perpendicular
    
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int cellX = ix + dx;
                int cellY = iy + dy;
                int cellZ = iz + dz;
                
                int hash = p[(p[(p[cellX & 255] + cellY) & 255] + cellZ) & 255];
                int hash2 = p[(hash + 10) & 255];
                
                // Jittered position
                double px = cellX + static_cast<double>(hash) / 255.0;
                double py = cellY + static_cast<double>(p[(hash + 1) & 255]) / 255.0;
                double pz = cellZ + static_cast<double>(p[(hash + 2) & 255]) / 255.0;
                
                // Vector from impulse to point
                QVector3D v(x - px, y - py, z - pz);
                
                // Project onto direction and perpendicular
                double parallel = QVector3D::dotProduct(v, dir);
                QVector3D perpVec = v - dir * parallel;
                double perpSq = perpVec.lengthSquared();
                
                // Anisotropic Gaussian envelope
                double distSq = alpha * parallel * parallel + beta * perpSq;
                
                if (distSq > 4.0) continue;
                
                double envelope = std::exp(-M_PI * distSq);
                
                // Complex Gabor kernel
                double phase = static_cast<double>(hash2) / 255.0 * 2.0 * M_PI;
                double arg = omega * parallel + phase;
                
                totalReal += envelope * std::cos(arg);
                totalImag += envelope * std::sin(arg);
            }
        }
    }
    
    // Compute results
    GaborResult result;
    result.value = (totalReal * 0.5) + 0.5; // Normalized to 0..1
    result.intensity = std::sqrt(totalReal * totalReal + totalImag * totalImag);
    result.phase = std::atan2(totalImag, totalReal) / (2.0 * M_PI) + 0.5; // Normalized to 0..1
    
    return result;
}

// Gabor Noise (Anisotropic) - Legacy wrapper
double PerlinNoise::gaborNoise(double x, double y, double z, double frequency, double anisotropy, double orientation) const {
    // Map orientation scalar to 3D direction (rotation around Z)
    double angle = orientation * 2.0 * M_PI;
    QVector3D dir(std::cos(angle), std::sin(angle), 0.0);
    return gaborNoise(x, y, z, frequency, anisotropy, dir).value;
}



int PerlinNoise::fastfloor(double x) {
    return x > 0 ? static_cast<int>(x) : static_cast<int>(x) - 1;
}

double PerlinNoise::dot(const int g[], double x, double y, double z) {
    return g[0] * x + g[1] * y + g[2] * z;
}

// Everling Noise (Integrated Gaussian)
// Based on "Everling Noise: A Linear-Time Noise Algorithm"
// Implements a buffered 3D noise generated by integrating Gaussian steps.
void PerlinNoise::regenerateEverling(double mean, double stddev, EverlingAccessMethod accessMethod) const {
    int size = EVERLING_SIZE; // 64
    int totalSize = size * size * size;
    if (m_everlingBuffer.size() != totalSize) {
        m_everlingBuffer.resize(totalSize);
    }
    m_everlingBuffer.fill(0.0);
    
    QVector<bool> visited(totalSize, false);
    QVector<int> frontier;
    frontier.reserve(totalSize);
    
    // Start at origin
    int startIdx = 0;
    visited[startIdx] = true;
    frontier.push_back(startIdx);
    m_everlingBuffer[startIdx] = 0.0;
    
    std::normal_distribution<double> dist(mean, stddev);
    std::normal_distribution<double> gaussianAccess(0.0, 0.3); // For Gaussian access method
    
    while (!frontier.isEmpty()) {
        int fIdx;
        
        // Select index based on access method
        switch (accessMethod) {
            case EverlingAccessMethod::Stack:
                // DFS-like: Always pick from end of frontier (creates fractal veins)
                fIdx = frontier.size() - 1;
                break;
                
            case EverlingAccessMethod::Random:
                // Pure random: Creates radial/erosion patterns
                {
                    std::uniform_int_distribution<int> rDist(0, (int)frontier.size() - 1);
                    fIdx = rDist(m_rng);
                }
                break;
                
            case EverlingAccessMethod::Gaussian:
                // Gaussian-weighted towards recent entries: Clustered/cloudy patterns
                {
                    double g = gaussianAccess(m_rng);
                    // Map Gaussian to index (centered on last entry)
                    int offset = static_cast<int>(g * frontier.size());
                    fIdx = std::clamp((int)frontier.size() - 1 + offset, 0, (int)frontier.size() - 1);
                }
                break;
                
            case EverlingAccessMethod::Mixed:
            default:
                // 50% Stack / 50% Random (default behavior)
                if (m_rng() % 2 == 0) {
                    fIdx = frontier.size() - 1;
                } else {
                    std::uniform_int_distribution<int> rDist(0, (int)frontier.size() - 1);
                    fIdx = rDist(m_rng);
                }
                break;
        }
        
        int currentIdx = frontier[fIdx];
        
        // Remove from frontier (Swap with last and removeLast for O(1))
        if (fIdx != frontier.size() - 1) {
            frontier[fIdx] = frontier.last();
        }
        frontier.removeLast();
        
        // Expand to neighbors
        int cy = (currentIdx / size) % size;
        int cx = currentIdx % size;
        int cz = currentIdx / (size * size);
        
        // Neighbors
        int neighbors[6];
        int count = 0;
        
        if (cx + 1 < size) neighbors[count++] = currentIdx + 1;
        if (cx - 1 >= 0)   neighbors[count++] = currentIdx - 1;
        if (cy + 1 < size) neighbors[count++] = currentIdx + size;
        if (cy - 1 >= 0)   neighbors[count++] = currentIdx - size;
        if (cz + 1 < size) neighbors[count++] = currentIdx + size*size;
        if (cz - 1 >= 0)   neighbors[count++] = currentIdx - size*size;
        
        for (int i=0; i<count; ++i) {
            int nIdx = neighbors[i];
            if (!visited[nIdx]) {
                visited[nIdx] = true;
                double step = dist(m_rng);
                m_everlingBuffer[nIdx] = m_everlingBuffer[currentIdx] + step;
                frontier.push_back(nIdx);
            }
        }
    }
    
    // Normalize to -1.0 to 1.0 (Essential for noise texture)
    double minVal = m_everlingBuffer[0];
    double maxVal = m_everlingBuffer[0];
    for (double v : m_everlingBuffer) {
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }
    
    double range = maxVal - minVal;
    if (range < 0.0001) range = 1.0;
    
    for (int i = 0; i < totalSize; ++i) {
        m_everlingBuffer[i] = (m_everlingBuffer[i] - minVal) / range;
    }
    
    m_cachedMean = mean;
    m_cachedStdDev = stddev;
    m_cachedAccessMethod = accessMethod;
}

double PerlinNoise::everlingNoise(double x, double y, double z, double mean, double stddev, 
                                   EverlingAccessMethod accessMethod) const {
    if (m_everlingBuffer.isEmpty() || 
        std::abs(mean - m_cachedMean) > 0.001 || 
        std::abs(stddev - m_cachedStdDev) > 0.001 ||
        accessMethod != m_cachedAccessMethod) {
        regenerateEverling(mean, stddev, accessMethod);
    }
    
    int size = EVERLING_SIZE;
    
    auto wrap = [&](double val) -> double {
        double v = val - std::floor(val); 
        return v * size; 
    };
    
    double X = wrap(x);
    double Y = wrap(y);
    double Z = wrap(z);
    
    int x0 = (int)X;
    int y0 = (int)Y;
    int z0 = (int)Z;
    
    int x1 = (x0 + 1) % size;
    int y1 = (y0 + 1) % size;
    int z1 = (z0 + 1) % size;
    
    double fx = X - x0;
    double fy = Y - y0;
    double fz = Z - z0;
    
    auto idx = [&](int ix, int iy, int iz) {
        return iz * size * size + iy * size + ix;
    };
    
    double c000 = m_everlingBuffer[idx(x0, y0, z0)];
    double c100 = m_everlingBuffer[idx(x1, y0, z0)];
    double c010 = m_everlingBuffer[idx(x0, y1, z0)];
    double c110 = m_everlingBuffer[idx(x1, y1, z0)];
    double c001 = m_everlingBuffer[idx(x0, y0, z1)];
    double c101 = m_everlingBuffer[idx(x1, y0, z1)];
    double c011 = m_everlingBuffer[idx(x0, y1, z1)];
    double c111 = m_everlingBuffer[idx(x1, y1, z1)];
    
    double lx0 = lerp(fx, c000, c100);
    double lx1 = lerp(fx, c010, c110);
    double lx2 = lerp(fx, c001, c101);
    double lx3 = lerp(fx, c011, c111);
    
    double ly0 = lerp(fy, lx0, lx1);
    double ly1 = lerp(fy, lx2, lx3);
    
    return lerp(fz, ly0, ly1); 
}

void PerlinNoise::clearEverlingCache() const {
    m_everlingBuffer.clear();
    m_cachedMean = -9999.0;
}

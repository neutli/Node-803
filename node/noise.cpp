#include "noise.h"
#include "OpenSimplex2.hpp"

PerlinNoise::PerlinNoise(unsigned int seed) : m_rng(seed) {
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

// Simplex Noise (3D) - Standard Implementation
// Based on Stefan Gustavson's implementation
double PerlinNoise::simplexNoise(double x, double y, double z) const {
    const double F3 = 1.0 / 3.0; // Skew factor
    const double G3 = 1.0 / 6.0; // Unskew factor

    // Skew the input space to determine which simplex cell we're in
    double s = (x + y + z) * F3;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));
    int k = static_cast<int>(std::floor(z + s));

    double t = (i + j + k) * G3;
    double X0 = i - t;
    double Y0 = j - t;
    double Z0 = k - t;
    double x0 = x - X0;
    double y0 = y - Y0;
    double z0 = z - Z0;

    // Determine which simplex we are in
    int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
    int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords
    
    if (x0 >= y0) {
        if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // X Y Z order
        else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; } // X Z Y order
        else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; } // Z X Y order
    } else { // x0 < y0
        if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; } // Z Y X order
        else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; } // Y Z X order
        else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // Y X Z order
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
    // c = 1/6.
    double x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
    double y1 = y0 - j1 + G3;
    double z1 = z0 - k1 + G3;
    double x2 = x0 - i2 + 2.0 * G3; // Offsets for third corner in (x,y,z) coords
    double y2 = y0 - j2 + 2.0 * G3;
    double z2 = z0 - k2 + 2.0 * G3;
    double x3 = x0 - 1.0 + 3.0 * G3; // Offsets for last corner in (x,y,z) coords
    double y3 = y0 - 1.0 + 3.0 * G3;
    double z3 = z0 - 1.0 + 3.0 * G3;

    // Calculate the contribution from the four corners
    double n0, n1, n2, n3;

    // Corner 0
    double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0) n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * grad(p[i + p[j + p[k & 255] & 255] & 255], x0, y0, z0);
    }

    // Corner 1
    double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0) n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * grad(p[i + i1 + p[j + j1 + p[k + k1 & 255] & 255] & 255], x1, y1, z1);
    }

    // Corner 2
    double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0) n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * grad(p[i + i2 + p[j + j2 + p[k + k2 & 255] & 255] & 255], x2, y2, z2);
    }

    // Corner 3
    double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0) n3 = 0.0;
    else {
        t3 *= t3;
        n3 = t3 * t3 * grad(p[i + 1 + p[j + 1 + p[k + 1 & 255] & 255] & 255], x3, y3, z3);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    double res = 32.0 * (n0 + n1 + n2 + n3);

    // Map to [0, 1] for consistency with other noise types in this application
    return (res + 1.0) * 0.5;
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
void PerlinNoise::regenerateEverling(int size, double mean, double stddev, EverlingAccessMethod accessMethod, double clusterSpread) const {
    // int size = EVERLING_SIZE; // No longer const
    if (size < 16) size = 16;   // Safety minimum
    if (size > 2048) size = 2048; // Safety maximum
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
    std::normal_distribution<double> gaussianAccess(0.0, clusterSpread); // Use parameter
    
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
    
    m_cachedSize = size;
    m_cachedMean = mean;
    m_cachedStdDev = stddev;
    m_cachedAccessMethod = accessMethod;
    m_cachedClusterSpread = clusterSpread;
}

double PerlinNoise::everlingNoise(double x, double y, double z, double mean, double stddev, 
                                   EverlingAccessMethod accessMethod, double clusterSpread, bool smoothEdges,
                                   int gridSize, double smoothWidth,
                                   EverlingPeriodicity periodicity,
                                   double distortion, int octaves, double lacunarity, double gain) const {
    if (m_everlingBuffer.isEmpty() || 
        m_cachedSize != gridSize ||
        std::abs(mean - m_cachedMean) > 0.001 || 
        std::abs(stddev - m_cachedStdDev) > 0.001 ||
        std::abs(clusterSpread - m_cachedClusterSpread) > 0.001 ||
        accessMethod != m_cachedAccessMethod) {
        regenerateEverling(gridSize, mean, stddev, accessMethod, clusterSpread);
    }
    
    int size = m_cachedSize;
    double total = 0.0;
    double amplitude = 1.0;
    double maxAmplitude = 0.0;
    
    double cx = x;
    double cy = y;
    double cz = z;
    
    // Domain Distortion (Coordinate Hashing/Warping)
    if (distortion > 0.0) {
        // Use OpenSimplex2S to warp the coordinates
        // Using distinct seeds/offsets for x,y,z
        cx += openSimplex2S(x * 0.5, y * 0.5, z * 0.5) * distortion;
        cy += openSimplex2S(x * 0.5 + 100, y * 0.5 + 100, z * 0.5 + 100) * distortion;
        cz += openSimplex2S(x * 0.5 + 200, y * 0.5 + 200, z * 0.5 + 200) * distortion;
    }

    for (int i = 0; i < octaves; ++i) {
        // Periodicity logic
        auto wrap = [&](double val) -> double {
            double v;
            if (periodicity == EverlingPeriodicity::Mirror) {
                // Triangle wave: 0->1->0
                double m = val - std::floor(val);
                v = std::abs(m - 0.5) * 2.0; 
                // Need to avoid edge case where v=1.0 or 0.0 exactly aligns with integer
            } else {
                // Wrap: 0->1
                v = val - std::floor(val); 
            }
            return v * size; 
        };
        
        double X = wrap(cx);
        double Y = wrap(cy);
        double Z = wrap(cz);
        
        int x0 = (int)X;
        int y0 = (int)Y;
        int z0 = (int)Z;
        
        // Safety clamp just in case
        x0 = std::clamp(x0, 0, size - 1);
        y0 = std::clamp(y0, 0, size - 1);
        z0 = std::clamp(z0, 0, size - 1);
        
        int x1, y1, z1;
        
        if (periodicity == EverlingPeriodicity::Mirror) {
             // In mirror mode, we clamp to edges because 0.99 map to size, but we just move within the buffer.
             // But actually we are playing safe.
             x1 = (x0 + 1) % size;
             y1 = (y0 + 1) % size;
             z1 = (z0 + 1) % size;
        } else {
             x1 = (x0 + 1) % size;
             y1 = (y0 + 1) % size;
             z1 = (z0 + 1) % size;
        }
        
        double fx = X - x0;
        double fy = Y - y0;
        double fz = Z - z0;
        
        auto idx = [&](int ix, int iy, int iz) {
            return iz * size * size + iy * size + ix;
        };
        
        // Tri-linear interpolation
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
        
        double rawVal = lerp(fz, ly0, ly1);
        
        // Smooth Edges (Fading) - Only apply if NOT mirroring (redundant if mirroring)
        // Kept for backward compat or if user prefers wrap + fade
        if (smoothEdges && periodicity != EverlingPeriodicity::Mirror) {
            double edgeDist = 0.5 - std::abs(cx - std::floor(cx) - 0.5);
            edgeDist = std::min(edgeDist, 0.5 - std::abs(cy - std::floor(cy) - 0.5));
            edgeDist = std::min(edgeDist, 0.5 - std::abs(cz - std::floor(cz) - 0.5));
            
            const double fadeWidth = smoothWidth;
            if (edgeDist < fadeWidth) {
                double t = edgeDist / fadeWidth;
                double fadeFactor = t * t * (3.0 - 2.0 * t);
                // rawVal = lerp(fadeFactor, 0.5, rawVal); // OLD: Fade to Gray
                // NEW: Do not fade to gray. Instead, fade transparency? No.
                // Just keep naive fade to gray but at least it works basically.
                // User complaint was "becomes one color". That's because fadeWidth was too large.
                // If mirroring is off, we must fade to something to hide seam.
                 rawVal = lerp(fadeFactor, 0.5, rawVal);
            }
        }
        
        total += rawVal * amplitude;
        maxAmplitude += amplitude;
        
        amplitude *= gain;
        cx *= lacunarity;
        cy *= lacunarity;
        cz *= lacunarity;
        
        // Add octave "phase shift" to avoid stacking on same grid
        cx += 123.45; cy += 345.67; cz += 567.89;
    }
    
    return total / maxAmplitude;
}

void PerlinNoise::clearEverlingCache() const {
    m_everlingBuffer.clear();
    m_cachedMean = -9999.0;
}

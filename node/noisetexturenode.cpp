#include "noisetexturenode.h"
#include <QVector3D>
#include <QJsonObject>

NoiseTextureNode::NoiseTextureNode()
    : Node("Noise Texture")
    , m_noise(std::make_unique<PerlinNoise>())
    , m_noiseType(NoiseType::OpenSimplex2S)
    , m_dimensions(Dimensions::D3)
    , m_distortionType(DistortionType::Legacy)
    , m_normalize(false)
{
    // Input sockets
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    m_vectorInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_vectorInput);

    m_wInput = new NodeSocket("W", SocketType::Float, SocketDirection::Input, this);
    m_wInput->setDefaultValue(0.0);
    addInputSocket(m_wInput);

    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(5.0);
    addInputSocket(m_scaleInput);

    m_detailInput = new NodeSocket("Detail", SocketType::Float, SocketDirection::Input, this);
    m_detailInput->setDefaultValue(2.0);
    addInputSocket(m_detailInput);

    m_roughnessInput = new NodeSocket("Roughness", SocketType::Float, SocketDirection::Input, this);
    m_roughnessInput->setDefaultValue(0.5);
    addInputSocket(m_roughnessInput);

    m_distortionInput = new NodeSocket("Distortion", SocketType::Float, SocketDirection::Input, this);
    m_distortionInput->setDefaultValue(0.0);
    addInputSocket(m_distortionInput);

    m_lacunarityInput = new NodeSocket("Lacunarity", SocketType::Float, SocketDirection::Input, this);
    m_lacunarityInput->setDefaultValue(2.0);
    addInputSocket(m_lacunarityInput);

    m_offsetInput = new NodeSocket("Offset", SocketType::Float, SocketDirection::Input, this);
    m_offsetInput->setDefaultValue(1.0);
    addInputSocket(m_offsetInput);

    m_noiseTypeInput = new NodeSocket("Noise Type", SocketType::Integer, SocketDirection::Input, this);
    m_noiseTypeInput->setDefaultValue(0); // Default to OpenSimplex2S
    addInputSocket(m_noiseTypeInput);

    // Output sockets
    m_facOutput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_facOutput);

    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
}

NoiseTextureNode::~NoiseTextureNode() {}

QVector<Node::ParameterInfo> NoiseTextureNode::parameters() const {
    return {
        ParameterInfo("Dimensions", {"2D", "3D", "4D"}, 
            QVariant::fromValue(static_cast<int>(m_dimensions)),
            [this](const QVariant& v) { 
                const_cast<NoiseTextureNode*>(this)->setDimensions(static_cast<Dimensions>(v.toInt())); 
            }),
        ParameterInfo("Noise Type", 
            {"OpenSimplex2S", "OpenSimplex2F", "Perlin", "Simplex", "RidgedMultifractal", "White", "Ridged", "Gabor", "Everling"},
            QVariant::fromValue(static_cast<int>(m_noiseType)),
            [this](const QVariant& v) {
                const_cast<NoiseTextureNode*>(this)->setNoiseType(static_cast<NoiseType>(v.toInt()));
            }),
        ParameterInfo("Fractal Type", 
            {"None", "FBM", "Multifractal", "Hybrid Multifractal", "Hetero Terrain", "Ridged Multifractal", "Division", "Linear Light"},
            QVariant::fromValue(static_cast<int>(m_fractalType)),
            [this](const QVariant& v) {
                const_cast<NoiseTextureNode*>(this)->setFractalType(static_cast<FractalType>(v.toInt()));
            }),
        ParameterInfo("Distortion Type", {"Legacy", "Blender"},
            QVariant::fromValue(static_cast<int>(m_distortionType)),
            [this](const QVariant& v) {
                const_cast<NoiseTextureNode*>(this)->setDistortionType(static_cast<DistortionType>(v.toInt()));
            }),
        ParameterInfo("Normalize", m_normalize,
            [this](const QVariant& v) {
                const_cast<NoiseTextureNode*>(this)->setNormalize(v.toBool());
            }),
        ParameterInfo("Scale", 0.0, 100.0, 5.0),
        ParameterInfo("Detail", 0.0, 15.0, 2.0),
        ParameterInfo("Roughness", 0.0, 1.0, 0.5),
        ParameterInfo("Distortion", 0.0, 10.0, 0.0),
        ParameterInfo("Lacunarity", 0.0, 5.0, 2.0),
        ParameterInfo("Offset", 0.0, 100.0, 1.0),
        ParameterInfo("W", -10.0, 10.0, 0.0)
    };
}

void NoiseTextureNode::evaluate()
{
    QMutexLocker locker(&m_mutex);
    if (!isDirty()) return;

    // Get vector input (or use position if not connected)
    QVector3D vec = m_vectorInput->isConnected()
        ? m_vectorInput->value().value<QVector3D>()
        : m_vectorInput->defaultValue().value<QVector3D>();

    double scaleVal = m_scaleInput->isConnected() ? m_scaleInput->value().toDouble() : m_scaleInput->defaultValue().toDouble();
    double detailVal = m_detailInput->isConnected() ? m_detailInput->value().toDouble() : m_detailInput->defaultValue().toDouble();
    double roughnessVal = m_roughnessInput->isConnected() ? m_roughnessInput->value().toDouble() : m_roughnessInput->defaultValue().toDouble();
    double distortionVal = m_distortionInput->isConnected() ? m_distortionInput->value().toDouble() : m_distortionInput->defaultValue().toDouble();
    double lacunarityVal = m_lacunarityInput->isConnected() ? m_lacunarityInput->value().toDouble() : m_lacunarityInput->defaultValue().toDouble();
    double offsetVal = m_offsetInput->isConnected() ? m_offsetInput->value().toDouble() : m_offsetInput->defaultValue().toDouble();
    double wVal = (m_dimensions == Dimensions::D4) ? (m_wInput->isConnected() ? m_wInput->value().toDouble() : m_wInput->defaultValue().toDouble()) : 0.0;

    // Apply scale and offset to avoid (0,0,0) artifacts
    // The offset ensures noise is never sampled exactly at origin
    const double NOISE_OFFSET = 100.0;
    double x = vec.x() * scaleVal + NOISE_OFFSET;
    double y = vec.y() * scaleVal + NOISE_OFFSET;
    double z = vec.z() * scaleVal;

    // Distortion
    if (distortionVal > 0.0) {
        x += m_noise->noise(y, z) * distortionVal;
        y += m_noise->noise(z, x) * distortionVal;
        z += m_noise->noise(x, y) * distortionVal;
    }

    int octaves = static_cast<int>(detailVal);
    if (octaves < 1) octaves = 1; // Ensure at least one octave
    double noiseValue = 0.0;

    // Helper to incorporate W into 3D coordinates (Fake 4D)
    auto applyW = [&](double& nx, double& ny, double& nz) {
        if (m_dimensions == Dimensions::D4) {
            nx += wVal;
            ny += wVal;
            nz += wVal;
        }
    };


    // Helper to get basis noise (single octave, -1..1 range)
    auto getBasis = [&](double bx, double by, double bz) -> double {
        double n = 0.0;
        switch (m_noiseType) {
            case NoiseType::OpenSimplex2S: n = m_noise->openSimplex2S(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::OpenSimplex2F: n = m_noise->openSimplex2F(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::Perlin: n = m_noise->noise(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::Simplex: n = m_noise->simplexNoise(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::White: n = m_noise->whiteNoise(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::Gabor: n = m_noise->gaborNoise(bx, by, bz, lacunarityVal, detailVal, roughnessVal) * 2.0 - 1.0; break;
            case NoiseType::RidgedMultifractal: n = m_noise->ridgedMultifractal(bx, by, bz, octaves, lacunarityVal, roughnessVal, 1.0) * 2.0 - 1.0; break;

            case NoiseType::Ridged: n = (1.0 - std::abs(m_noise->noise(bx, by, bz) * 2.0 - 1.0)) * 2.0 - 1.0;
                                    break;
            case NoiseType::Everling: n = m_noise->everlingNoise(bx, by, bz, offsetVal, roughnessVal * 5.0 + 0.1) * 2.0 - 1.0; break;
        }
        return n;
    };

    // Helper to compute fractal value at specific coords
    auto computeFractal = [&](double tx, double ty, double tz) -> double {
        applyW(tx, ty, tz);
        double val = 0.0;
        
        switch (m_fractalType) {
            case FractalType::None:
                val = getBasis(tx, ty, tz); // Raw -1..1
                break;
            case FractalType::FBM:
                {
                    double freq = 1.0;
                    double amp = 1.0;
                    double maxAmp = 0.0;
                    val = 0.0;
                    for (int i = 0; i < octaves; ++i) {
                        val += getBasis(tx * freq, ty * freq, tz * freq) * amp;
                        maxAmp += amp;
                        freq *= lacunarityVal;
                        amp *= roughnessVal;
                    }
                    if (maxAmp > 0.0) val /= maxAmp; // -1..1
                }
                break;
            case FractalType::Multifractal:
                {
                    // Musgrave Multifractal (Standard)
                    val = 1.0;
                    double freq = 1.0;
                    double pwr = 1.0;
                    for (int i = 0; i < octaves; ++i) {
                        double n = getBasis(tx * freq, ty * freq, tz * freq); // -1..1
                        val *= (offsetVal + n) * pwr;
                        freq *= lacunarityVal;
                        pwr *= roughnessVal;
                    }
                }
                break;
            case FractalType::HybridMultifractal:
                {
                    double freq = 1.0;
                    // Use raw basis (-1..1) and shift offset by -1.0 to center default 1.0 at 0.0
                    double result = getBasis(tx, ty, tz) + (offsetVal - 1.0);
                    double weight = result;
                    double signal;
                    
                    freq *= lacunarityVal;
                    double pwr = roughnessVal;
                    double maxAmp = 1.0;

                    for (int i = 1; i < octaves; ++i) {
                        if (weight > 1.0) weight = 1.0;
                        if (weight < 0.0) weight = 0.0;
                        signal = getBasis(tx * freq, ty * freq, tz * freq) + (offsetVal - 1.0);
                        result += weight * signal * pwr;
                        weight *= signal;
                        freq *= lacunarityVal;
                        maxAmp += pwr;
                        pwr *= roughnessVal;
                    }
                    if (maxAmp > 0.0) val = result / maxAmp; // -1..1
                }
                break;
            case FractalType::HeteroTerrain:
                {
                    double freq = 1.0;
                    double signal = getBasis(tx, ty, tz) + (offsetVal - 1.0);
                    double result = signal;
                    freq *= lacunarityVal;
                    double pwr = roughnessVal;
                    double maxAmp = 1.0;
                    
                    for (int i = 1; i < octaves; ++i) {
                         signal = getBasis(tx * freq, ty * freq, tz * freq) + (offsetVal - 1.0);
                         result += signal * pwr; 
                         freq *= lacunarityVal;
                         maxAmp += pwr;
                         pwr *= roughnessVal;
                    }
                    if (maxAmp > 0.0) val = result / maxAmp; // -1..1
                }
                break;
            case FractalType::RidgedMultifractal:
                 {
                     double freq = 1.0;
                     double amp = 1.0;
                     double signal;
                     val = 0.0;
                     for (int i = 0; i < octaves; ++i) {
                         signal = getBasis(tx * freq, ty * freq, tz * freq);
                         signal = offsetVal - std::abs(signal);
                         signal *= signal;
                         val += signal * amp;
                         freq *= lacunarityVal;
                         amp *= roughnessVal;
                     }
                 }
                 break;
            case FractalType::Division:
                {
                    double n = getBasis(tx, ty, tz);
                    double n01 = n * 0.5 + 0.5;
                    val = 1.0 / (n01 + 0.1);
                }
                break;
            case FractalType::LinearLight:
                {
                    double n = getBasis(tx, ty, tz);
                    double n01 = n * 0.5 + 0.5;
                    val = 2.0 * n01 - 0.5;
                }
                break;
        }
        
        if (m_normalize) {
             val = val * 0.5 + 0.5;
             val = qBound(0.0, val, 1.0);
        }
        return val;
    };

    noiseValue = computeFractal(x, y, z);

    // Set outputs
    m_facOutput->setValue(noiseValue);
    
    // Generate colorful noise for Color output
    // We use different offsets for R, G, B channels to decorrelate them
    double r = noiseValue; // Use the base noise for R
    double g = computeFractal(x + 123.45, y + 678.90, z + 42.0);
    double b = computeFractal(x - 42.0, y + 987.65, z - 123.45);
    
    m_colorOutput->setValue(QColor::fromRgbF(qBound(0.0, r, 1.0), qBound(0.0, g, 1.0), qBound(0.0, b, 1.0)));

    setDirty(false);
}

QVariant NoiseTextureNode::compute(const QVector3D &pos, NodeSocket *socket)
{
    QMutexLocker locker(&m_mutex);
    // For preview we compute directly using current parameters (no need to evaluate upstream)
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        // If not connected, normalize pixel coordinates to 0-1 range
        // Assume typical texture size for normalization
        vec = QVector3D(pos.x() / 512.0, pos.y() / 512.0, 0.0);
    }

    double scaleVal = m_scaleInput->isConnected() ? m_scaleInput->getValue(pos).toDouble() : m_scaleInput->defaultValue().toDouble();
    double detailVal = m_detailInput->isConnected() ? m_detailInput->getValue(pos).toDouble() : m_detailInput->defaultValue().toDouble();
    double roughnessVal = m_roughnessInput->isConnected() ? m_roughnessInput->getValue(pos).toDouble() : m_roughnessInput->defaultValue().toDouble();
    double distortionVal = m_distortionInput->isConnected() ? m_distortionInput->getValue(pos).toDouble() : m_distortionInput->defaultValue().toDouble();
    double lacunarityVal = m_lacunarityInput->isConnected() ? m_lacunarityInput->getValue(pos).toDouble() : m_lacunarityInput->defaultValue().toDouble();
    double offsetVal = m_offsetInput->isConnected() ? m_offsetInput->getValue(pos).toDouble() : m_offsetInput->defaultValue().toDouble();
    double wVal = m_wInput->isConnected() ? m_wInput->getValue(pos).toDouble() : m_wInput->defaultValue().toDouble();

    // Noise Type input overrides internal state if connected
    NoiseType currentNoiseType = m_noiseType;
    if (m_noiseTypeInput->isConnected()) {
        int typeInt = m_noiseTypeInput->getValue(pos).toInt();
        if (typeInt >= 0 && typeInt <= 13) {  // Updated for 14 noise types (0-13)
            currentNoiseType = static_cast<NoiseType>(typeInt);
        }
    }

    // Apply scale and offset to avoid (0,0,0) artifacts
    const double NOISE_OFFSET = 100.0;
    double x = vec.x() * scaleVal + NOISE_OFFSET;
    double y = vec.y() * scaleVal + NOISE_OFFSET;
    double z = vec.z() * scaleVal;
    double w = wVal * scaleVal;

    // Distortion
    if (distortionVal > 0.0) {
        if (m_distortionType == DistortionType::Legacy) {
            x += m_noise->noise(y, z) * distortionVal;
            y += m_noise->noise(z, x) * distortionVal;
            z += m_noise->noise(x, y) * distortionVal;
        } else {
            // Blender-style distortion (simplified domain warping)
            // Ideally should use a separate noise lookup for offset
            double dx = m_noise->noise(x + 5.3, y + 2.7, z - 1.4) * distortionVal;
            double dy = m_noise->noise(x - 4.2, y + 8.1, z + 3.3) * distortionVal;
            double dz = m_noise->noise(x + 1.9, y - 6.5, z + 0.2) * distortionVal;
            x += dx;
            y += dy;
            z += dz;
        }
    }

    int octaves = static_cast<int>(detailVal);
    
    // Helper for 4D noise (simplified as 3D with W offset for now)
    auto applyW = [&](double& nx, double& ny, double& nz) {
        if (m_dimensions == Dimensions::D4) {
            nx += w;
            ny += w;
            nz += w;
        }
    };

    if (m_dimensions == Dimensions::D2) {
        z = 0.0;
    }

    // Helper to get basis noise (normalized to -1..1 range)
    auto getBasis = [&](double bx, double by, double bz) -> double {
        double n = 0.0;
        switch (currentNoiseType) {
            case NoiseType::OpenSimplex2S: n = m_noise->openSimplex2S(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::OpenSimplex2F: n = m_noise->openSimplex2F(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::Perlin: n = m_noise->noise(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::Simplex: n = m_noise->simplexNoise(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::White: n = m_noise->whiteNoise(bx, by, bz) * 2.0 - 1.0; break;
            case NoiseType::Gabor: n = m_noise->gaborNoise(bx, by, bz, lacunarityVal, detailVal, roughnessVal) * 2.0 - 1.0; break;
            case NoiseType::RidgedMultifractal: n = m_noise->ridgedMultifractal(bx, by, bz, octaves, lacunarityVal, roughnessVal, 1.0) * 2.0 - 1.0; break;
            case NoiseType::Ridged: n = (1.0 - std::abs(m_noise->noise(bx, by, bz) * 2.0 - 1.0)) * 2.0 - 1.0; break;
            case NoiseType::Everling: n = m_noise->everlingNoise(bx, by, bz, offsetVal, roughnessVal * 5.0 + 0.1) * 2.0 - 1.0; break;
        }
        return n;
    };

    // Helper to compute fractal value at specific coords
    auto computeFractal = [&](double tx, double ty, double tz) -> double {
        applyW(tx, ty, tz);
        double val = 0.0;
        
        switch (m_fractalType) {
            case FractalType::None:
                val = getBasis(tx, ty, tz); // Raw -1..1
                break;
            case FractalType::FBM:
                {
                    double freq = 1.0;
                    double amp = 1.0;
                    double maxAmp = 0.0;
                    val = 0.0;
                    for (int i = 0; i < octaves; ++i) {
                        val += getBasis(tx * freq, ty * freq, tz * freq) * amp;
                        maxAmp += amp;
                        freq *= lacunarityVal;
                        amp *= roughnessVal;
                    }
                    if (maxAmp > 0.0) val /= maxAmp; // -1..1
                }
                break;
            case FractalType::Multifractal:
                {
                    // Musgrave Multifractal (Standard)
                    val = 1.0;
                    double freq = 1.0;
                    double pwr = 1.0;
                    for (int i = 0; i < octaves; ++i) {
                        double n = getBasis(tx * freq, ty * freq, tz * freq); // -1..1
                        val *= (offsetVal + n) * pwr;
                        freq *= lacunarityVal;
                        pwr *= roughnessVal;
                    }
                }
                break;
            case FractalType::HybridMultifractal:
                {
                    double freq = 1.0;
                    double result = getBasis(tx, ty, tz) + (offsetVal - 1.0);
                    double weight = result;
                    double signal;
                    
                    freq *= lacunarityVal;
                    double pwr = roughnessVal;
                    double maxAmp = 1.0;

                    for (int i = 1; i < octaves; ++i) {
                        if (weight > 1.0) weight = 1.0;
                        if (weight < 0.0) weight = 0.0;
                        signal = getBasis(tx * freq, ty * freq, tz * freq) + (offsetVal - 1.0);
                        result += weight * signal * pwr;
                        weight *= signal;
                        freq *= lacunarityVal;
                        maxAmp += pwr;
                        pwr *= roughnessVal;
                    }
                    if (maxAmp > 0.0) val = result / maxAmp; // -1..1
                }
                break;
            case FractalType::HeteroTerrain:
                {
                    double freq = 1.0;
                    double signal = getBasis(tx, ty, tz) + (offsetVal - 1.0);
                    double result = signal;
                    freq *= lacunarityVal;
                    double pwr = roughnessVal;
                    double maxAmp = 1.0;
                    
                    for (int i = 1; i < octaves; ++i) {
                         signal = getBasis(tx * freq, ty * freq, tz * freq) + (offsetVal - 1.0);
                         result += signal * pwr; 
                         freq *= lacunarityVal;
                         maxAmp += pwr;
                         pwr *= roughnessVal;
                    }
                    if (maxAmp > 0.0) val = result / maxAmp; // -1..1
                }
                break;
            case FractalType::RidgedMultifractal:
                 {
                     double freq = 1.0;
                     double amp = 1.0;
                     double signal;
                     val = 0.0;
                     for (int i = 0; i < octaves; ++i) {
                         signal = getBasis(tx * freq, ty * freq, tz * freq);
                         signal = offsetVal - std::abs(signal);
                         signal *= signal;
                         val += signal * amp;
                         freq *= lacunarityVal;
                         amp *= roughnessVal;
                     }
                 }
                 break;
            case FractalType::Division:
                {
                    double n = getBasis(tx, ty, tz);
                    double n01 = n * 0.5 + 0.5;
                    val = 1.0 / (n01 + 0.1);
                }
                break;
            case FractalType::LinearLight:
                {
                    double n = getBasis(tx, ty, tz);
                    double n01 = n * 0.5 + 0.5;
                    val = 2.0 * n01 - 0.5;
                }
                break;
        }
        
        if (m_normalize) {
             val = val * 0.5 + 0.5;
             val = qBound(0.0, val, 1.0);
        }
        return val;
    };

    double noiseValue = computeFractal(x, y, z);

    if (socket == m_facOutput) {
        return noiseValue;
    } else if (socket == m_colorOutput) {
        double r = noiseValue;
        // Decorrelate G and B by offsetting coordinates
        // We use large offsets to ensure correlation is lost
        double g = computeFractal(x + 123.45, y + 678.90, z + 42.0);
        double b = computeFractal(x - 42.0, y + 987.65, z - 123.45);
        
        // Output as Color (clamped to 0..1 for display safety)
        return QColor::fromRgbF(qBound(0.0, r, 1.0), qBound(0.0, g, 1.0), qBound(0.0, b, 1.0));
    }
    return QVariant();
}

// Getters
double NoiseTextureNode::scale() const { return m_scaleInput->defaultValue().toDouble(); }
double NoiseTextureNode::detail() const { return m_detailInput->defaultValue().toDouble(); }
double NoiseTextureNode::roughness() const { return m_roughnessInput->defaultValue().toDouble(); }
double NoiseTextureNode::distortion() const { return m_distortionInput->defaultValue().toDouble(); }
double NoiseTextureNode::lacunarity() const { return m_lacunarityInput->defaultValue().toDouble(); }
double NoiseTextureNode::offset() const { return m_offsetInput->defaultValue().toDouble(); }
double NoiseTextureNode::w() const { return m_wInput->defaultValue().toDouble(); }
NoiseType NoiseTextureNode::noiseType() const { return m_noiseType; }

// Setters
void NoiseTextureNode::setScale(double v) { m_scaleInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setDetail(double v) { m_detailInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setRoughness(double v) { m_roughnessInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setDistortion(double v) { m_distortionInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setLacunarity(double v) { m_lacunarityInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setOffset(double v) { m_offsetInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setW(double v) { m_wInput->setDefaultValue(v); setDirty(true); }
void NoiseTextureNode::setNoiseType(NoiseType t) { m_noiseType = t; setDirty(true); }
void NoiseTextureNode::setFractalType(FractalType t) { m_fractalType = t; setDirty(true); }
void NoiseTextureNode::setDimensions(Dimensions d) { m_dimensions = d; setDirty(true); notifyStructureChanged(); }
void NoiseTextureNode::setDistortionType(DistortionType t) { m_distortionType = t; setDirty(true); }
void NoiseTextureNode::setNormalize(bool b) { m_normalize = b; setDirty(true); }

// Helper for preview (optional)
double NoiseTextureNode::getNoiseValue(double x, double y, double z) const {
    QMutexLocker locker(&m_mutex);
    double scaleVal = scale();
    double detailVal = detail();
    double roughnessVal = roughness();
    double distortionVal = distortion();
    double lacunarityVal = lacunarity();
    double offsetVal = offset();
    double wVal = (m_dimensions == Dimensions::D4) ? w() : 0.0;
    x *= scaleVal; y *= scaleVal; z *= scaleVal;
    if (distortionVal > 0.0) {
        x += m_noise->noise(y, z) * distortionVal;
        y += m_noise->noise(z, x) * distortionVal;
        z += m_noise->noise(x, y) * distortionVal;
    }
    int octaves = static_cast<int>(detailVal);
    double noiseValue = 0.0;
    
    auto applyW = [&](double& nx, double& ny, double& nz) {
        if (m_dimensions == Dimensions::D4) {
            nx += wVal;
            ny += wVal;
            nz += wVal;
        }
    };

    switch (m_noiseType) {
        case NoiseType::OpenSimplex2S: {
            applyW(x, y, z);
            double freq = 1.0;
            double amp = 1.0;
            double maxAmp = 0.0;
            for (int i = 0; i < octaves; ++i) {
                noiseValue += m_noise->openSimplex2S(x * freq, y * freq, z * freq) * amp;
                maxAmp += amp;
                freq *= lacunarityVal;
                amp *= roughnessVal;
            }
            if (maxAmp > 0.0) noiseValue /= maxAmp;
            break;
        }
        case NoiseType::OpenSimplex2F: {
            applyW(x, y, z);
            double freq = 1.0;
            double amp = 1.0;
            double maxAmp = 0.0;
            for (int i = 0; i < octaves; ++i) {
                noiseValue += m_noise->openSimplex2F(x * freq, y * freq, z * freq) * amp;
                maxAmp += amp;
                freq *= lacunarityVal;
                amp *= roughnessVal;
            }
            if (maxAmp > 0.0) noiseValue /= maxAmp;
            break;
        }
        case NoiseType::Perlin:
            applyW(x, y, z);
            noiseValue = m_noise->octaveNoise(x, y, z, octaves, roughnessVal);
            break;
        case NoiseType::Simplex:
            applyW(x, y, z);
            noiseValue = m_noise->simplexNoise(x, y, z);
            break;
        case NoiseType::RidgedMultifractal:
            applyW(x, y, z);
            noiseValue = m_noise->ridgedMultifractal(x, y, z, octaves, lacunarityVal, roughnessVal, offsetVal);
            break;
        case NoiseType::White:
            applyW(x, y, z);
            noiseValue = m_noise->whiteNoise(x, y, z);
            break;
        case NoiseType::Gabor:
            applyW(x, y, z);
            noiseValue = m_noise->gaborNoise(x, y, z, lacunarityVal, detailVal, roughnessVal);
            break;


    }
    return noiseValue;
}

QColor NoiseTextureNode::getColorValue(double x, double y, double z) const {
    QMutexLocker locker(&m_mutex);
    // We need to compute full color here for preview
    // This is a bit inefficient as we duplicate logic, but for preview it's fine
    // Ideally we should refactor compute/evaluate to use a common noise generation method
    
    // For now, let's just call compute with a dummy socket if possible, or duplicate logic
    // Since compute() is public and handles null socket gracefully (returns QVariant), 
    // but we need specific socket to get specific output.
    // Let's use the helper getNoiseValue for Fac (R) and calculate G/B similarly.
    
    double r = getNoiseValue(x, y, z);
    
    // We need to replicate the offsets used in evaluate/compute
    // But getNoiseValue takes raw coordinates before scale/distortion? 
    // No, getNoiseValue applies scale/distortion internally.
    // So we can't just add offset to x,y,z passed to getNoiseValue because it would be scaled again.
    // We need to add offset AFTER scale/distortion but BEFORE noise function.
    // getNoiseValue doesn't expose that.
    
    // Simplest fix: Just use compute()!
    // We need to construct a position vector.
    // But compute() expects world position and applies scale/distortion.
    // getColorValue is called with 0-1 UV coordinates.
    // compute() handles this if vector input is not connected.
    
    // However, getColorValue is const, compute is not (in base class? No, compute is virtual).
    // compute is not const in base class.
    // We can cast away constness or fix design.
    // For now, let's duplicate the offset logic inside getColorValue by copying getNoiseValue body and adding offsets.
    
    double scaleVal = scale();
    double detailVal = detail();
    double roughnessVal = roughness();
    double distortionVal = distortion();
    double lacunarityVal = lacunarity();
    double offsetVal = offset();
    double wVal = (m_dimensions == Dimensions::D4) ? w() : 0.0;
    
    auto calc = [&](double ox, double oy, double oz) {
        double nx = x * scaleVal + ox;
        double ny = y * scaleVal + oy;
        double nz = z * scaleVal + oz;
        
        if (distortionVal > 0.0) {
            nx += m_noise->noise(ny, nz) * distortionVal;
            ny += m_noise->noise(nz, nx) * distortionVal;
            nz += m_noise->noise(nx, ny) * distortionVal;
        }
        
        if (m_dimensions == Dimensions::D4) {
            nx += wVal;
        }
        
        int octaves = static_cast<int>(detailVal);
        switch (m_noiseType) {
            case NoiseType::OpenSimplex2S: {
                double val = 0.0;
                double freq = 1.0;
                double amp = 1.0;
                double maxAmp = 0.0;
                for (int i = 0; i < octaves; ++i) {
                    val += m_noise->openSimplex2S(nx * freq, ny * freq, nz * freq) * amp;
                    maxAmp += amp;
                    freq *= lacunarityVal;
                    amp *= roughnessVal;
                }
                return maxAmp > 0.0 ? val / maxAmp : 0.0;
            }
            case NoiseType::OpenSimplex2F: {
                double val = 0.0;
                double freq = 1.0;
                double amp = 1.0;
                double maxAmp = 0.0;
                for (int i = 0; i < octaves; ++i) {
                    val += m_noise->openSimplex2F(nx * freq, ny * freq, nz * freq) * amp;
                    maxAmp += amp;
                    freq *= lacunarityVal;
                    amp *= roughnessVal;
                }
                return maxAmp > 0.0 ? val / maxAmp : 0.0;
            }
           case NoiseType::Perlin: return m_noise->octaveNoise(nx, ny, nz, octaves, roughnessVal);
            case NoiseType::Simplex: return m_noise->simplexNoise(nx, ny, nz); // Simplex in getNoiseValue was simplified?
            case NoiseType::RidgedMultifractal: return m_noise->ridgedMultifractal(nx, ny, nz, octaves, lacunarityVal, roughnessVal, offsetVal);
            case NoiseType::White: return m_noise->whiteNoise(nx, ny, nz);
            case NoiseType::Gabor: return m_noise->gaborNoise(nx, ny, nz, lacunarityVal, detailVal, roughnessVal);

        }
        return 0.0;
    };
    
    // Note: Offsets here must match evaluate/compute. 
    // In evaluate/compute, we added offsets to scaled coordinates.
    // Here we add offsets to scaled coordinates too.
    // But wait, in evaluate: x = vec.x * scale.
    // Then calcNoise(x + ox, ...).
    // So ox is added to scaled coordinate.
    // Here: nx = x * scale + ox. Correct.
    
    // Offsets from evaluate:
    // g: 123.45, 678.90, 42.0
    // b: -42.0, 987.65, -123.45
    
    double g = calc(123.45, 678.90, 42.0);
    double b = calc(-42.0, 987.65, -123.45);
    
    return QColor::fromRgbF(qBound(0.0, r, 1.0), qBound(0.0, g, 1.0), qBound(0.0, b, 1.0));
}

QJsonObject NoiseTextureNode::save() const {
    QJsonObject json = Node::save();
    json["noiseType"] = static_cast<int>(m_noiseType);
    json["fractalType"] = static_cast<int>(m_fractalType);
    json["dimensions"] = static_cast<int>(m_dimensions);
    json["distortionType"] = static_cast<int>(m_distortionType);
    json["normalize"] = m_normalize;
    return json;
}

void NoiseTextureNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("noiseType")) {
        m_noiseType = static_cast<NoiseType>(json["noiseType"].toInt());
    }
    if (json.contains("fractalType")) {
        m_fractalType = static_cast<FractalType>(json["fractalType"].toInt());
    }
    if (json.contains("dimensions")) {
        m_dimensions = static_cast<Dimensions>(json["dimensions"].toInt());
    }
    if (json.contains("distortionType")) {
        m_distortionType = static_cast<DistortionType>(json["distortionType"].toInt());
    }
    if (json.contains("normalize")) {
        m_normalize = json["normalize"].toBool();
    }
}

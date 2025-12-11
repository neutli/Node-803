#include "voronoinode.h"
#include <cmath>
#include <QVector3D>
#include <QColor>
#include <random>
#include <algorithm>

VoronoiNode::VoronoiNode() 
    : Node("Voronoi Texture")
    , m_dimensions(Dimensions::D3)
    , m_metric(Metric::Euclidean)
    , m_feature(Feature::F1)
    , m_normalize(false)
{
    // Inputs
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vectorInput);

    m_wInput = new NodeSocket("W", SocketType::Float, SocketDirection::Input, this);
    m_wInput->setDefaultValue(0.0);
    addInputSocket(m_wInput);

    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(5.0);
    addInputSocket(m_scaleInput);

    m_detailInput = new NodeSocket("Detail", SocketType::Float, SocketDirection::Input, this);
    m_detailInput->setDefaultValue(0.0); // Default 0 for standard Voronoi
    addInputSocket(m_detailInput);

    m_roughnessInput = new NodeSocket("Roughness", SocketType::Float, SocketDirection::Input, this);
    m_roughnessInput->setDefaultValue(0.5);
    addInputSocket(m_roughnessInput);

    m_lacunarityInput = new NodeSocket("Lacunarity", SocketType::Float, SocketDirection::Input, this);
    m_lacunarityInput->setDefaultValue(2.0);
    addInputSocket(m_lacunarityInput);

    m_randomnessInput = new NodeSocket("Randomness", SocketType::Float, SocketDirection::Input, this);
    m_randomnessInput->setDefaultValue(1.0);
    addInputSocket(m_randomnessInput);

    // Outputs
    m_distanceOutput = new NodeSocket("Distance", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_distanceOutput);

    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
    
    m_positionOutput = new NodeSocket("Position", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_positionOutput);

    m_wOutput = new NodeSocket("W", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_wOutput);

    m_radiusOutput = new NodeSocket("Radius", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_radiusOutput);
}

QVector<Node::ParameterInfo> VoronoiNode::parameters() const {
    return {
        ParameterInfo("Dimensions", {"2D", "3D", "4D"}, 
            QVariant::fromValue(static_cast<int>(m_dimensions)),
            [this](const QVariant& v) { 
                const_cast<VoronoiNode*>(this)->setDimensions(static_cast<Dimensions>(v.toInt())); 
            }),
        ParameterInfo("Feature", 
            {"F1", "F2", "Smooth F1", "Distance to Edge", "N-Sphere Radius"},
            QVariant::fromValue(static_cast<int>(m_feature)),
            [this](const QVariant& v) {
                const_cast<VoronoiNode*>(this)->setFeature(static_cast<Feature>(v.toInt()));
            }),
        ParameterInfo("Metric", 
            {"Euclidean", "Manhattan", "Chebyshev", "Minkowski"},
            QVariant::fromValue(static_cast<int>(m_metric)),
            [this](const QVariant& v) {
                const_cast<VoronoiNode*>(this)->setMetric(static_cast<Metric>(v.toInt()));
            }),
        ParameterInfo("Normalize", m_normalize,
            [this](const QVariant& v) {
                const_cast<VoronoiNode*>(this)->setNormalize(v.toBool());
            }),
        ParameterInfo("Scale", 0.0, 100.0, 5.0),
        ParameterInfo("Randomness", 0.0, 1.0, 1.0),
        ParameterInfo("Detail", 0.0, 15.0, 0.0),
        ParameterInfo("Roughness", 0.0, 1.0, 0.5),
        ParameterInfo("Lacunarity", 0.0, 5.0, 2.0),
        ParameterInfo("W", -10.0, 10.0, 0.0)
    };
}

void VoronoiNode::evaluate() {
    // Stateless
}

// Helper for hashing
static QVector3D hash3(QVector3D p) {
    p = QVector3D(
        fmod(p.x() * 127.1 + p.y() * 311.7 + p.z() * 74.7, 289.0),
        fmod(p.x() * 269.5 + p.y() * 183.3 + p.z() * 246.1, 289.0),
        fmod(p.x() * 113.5 + p.y() * 271.9 + p.z() * 124.6, 289.0)
    );
    
    p = QVector3D(
        sin(p.x()) * 43758.5453123,
        sin(p.y()) * 43758.5453123,
        sin(p.z()) * 43758.5453123
    );
    
    return QVector3D(
        fmod(std::abs(p.x()), 1.0),
        fmod(std::abs(p.y()), 1.0),
        fmod(std::abs(p.z()), 1.0)
    );
}

QVariant VoronoiNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D inputPos = pos;
    if (m_vectorInput->isConnected()) {
        inputPos = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        inputPos = QVector3D(pos.x() / 512.0, pos.y() / 512.0, 0.0);
    }

    double scaleVal = m_scaleInput->isConnected() ? m_scaleInput->getValue(pos).toDouble() : m_scaleInput->defaultValue().toDouble();
    double randomnessVal = m_randomnessInput->isConnected() ? m_randomnessInput->getValue(pos).toDouble() : m_randomnessInput->defaultValue().toDouble();
    double wVal = m_wInput->isConnected() ? m_wInput->getValue(pos).toDouble() : m_wInput->defaultValue().toDouble();
    double detailVal = m_detailInput->isConnected() ? m_detailInput->getValue(pos).toDouble() : m_detailInput->defaultValue().toDouble();
    double roughnessVal = m_roughnessInput->isConnected() ? m_roughnessInput->getValue(pos).toDouble() : m_roughnessInput->defaultValue().toDouble();
    double lacunarityVal = m_lacunarityInput->isConnected() ? m_lacunarityInput->getValue(pos).toDouble() : m_lacunarityInput->defaultValue().toDouble();

    int octaves = qBound(0, static_cast<int>(detailVal), 15);
    double freq = scaleVal;
    double amp = 1.0;
    double currentW = wVal * scaleVal;
    
    double finalDist = 0.0;
    QVector3D finalColor;
    QVector3D finalPos;
    double totalAmp = 0.0;

    // Loop for fractal detail (octaves)
    // Note: For F2/SmoothF1/etc, fractal usually just adds to the distance.
    // We'll compute the base layer fully, and add weighted distances for subsequent layers.
    
    for (int i = 0; i <= octaves; ++i) {
        QVector3D p = inputPos * freq;
        double w = currentW; // W scales with frequency? Usually W is time, so maybe not. Let's keep W constant relative to scale? 
        // Blender: W is W. It doesn't scale with frequency in 4D noise usually, but for consistency let's scale it if it's treated as 4th dimension.
        // If W is time, it should probably not scale. If W is 4th dimension, it should scale.
        // Let's assume 4th dimension.
        if (i > 0) w *= lacunarityVal;

        // Handle dimensions
        int zStart = -1, zEnd = 1;
        if (m_dimensions == Dimensions::D2) {
            p.setZ(0.0);
            zStart = 0; zEnd = 0;
        } else if (m_dimensions == Dimensions::D4) {
            p += QVector3D(w, w, w); 
        }

        QVector3D integerPart(std::floor(p.x()), std::floor(p.y()), std::floor(p.z()));
        QVector3D fractionalPart = p - integerPart;

        // Optimization: Track top 2 neighbors only
        struct NeighborInfo {
            double dist;
            QVector3D color;
            QVector3D pos;
        };
        
        NeighborInfo n1 = {1e9, QVector3D(), QVector3D()};
        NeighborInfo n2 = {1e9, QVector3D(), QVector3D()};

        for (int z = zStart; z <= zEnd; z++) {
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    QVector3D neighbor(x, y, z);
                    QVector3D point = hash3(integerPart + neighbor);
                    
                    // Jitter
                    point = point * randomnessVal; 
                    QVector3D featurePoint = neighbor + point;
                    QVector3D diff = featurePoint - fractionalPart;
                    
                    double dist = 0.0;
                    // Optimization: Squared distance for Euclidean comparison
                    if (m_metric == Metric::Euclidean) {
                        dist = diff.lengthSquared();
                    } else {
                        switch (m_metric) {
                            case Metric::Manhattan: dist = std::abs(diff.x()) + std::abs(diff.y()) + std::abs(diff.z()); break;
                            case Metric::Chebyshev: dist = std::max({std::abs(diff.x()), std::abs(diff.y()), std::abs(diff.z())}); break;
                            case Metric::Minkowski: {
                                double exponent = 0.5; 
                                // Optimization: Avoid pow if possible, but for 0.5 it's sqrt
                                // dist = ( |x|^0.5 + |y|^0.5 + |z|^0.5 ) ^ (1/0.5)
                                // dist = ( sqrt(|x|) + sqrt(|y|) + sqrt(|z|) ) ^ 2
                                double sum = std::sqrt(std::abs(diff.x())) + std::sqrt(std::abs(diff.y())) + std::sqrt(std::abs(diff.z()));
                                dist = sum * sum;
                                break;
                            }
                            default: break;
                        }
                    }

                    if (dist < n1.dist) {
                        n2 = n1;
                        n1 = {dist, hash3(integerPart + neighbor), featurePoint};
                    } else if (dist < n2.dist) {
                        n2 = {dist, hash3(integerPart + neighbor), featurePoint};
                    }
                }
            }
        }

        // Sqrt if Euclidean
        if (m_metric == Metric::Euclidean) {
            n1.dist = std::sqrt(n1.dist);
            n2.dist = std::sqrt(n2.dist);
        }

        double layerDist = 0.0;
        if (m_feature == Feature::F1) {
            layerDist = n1.dist;
        } else if (m_feature == Feature::F2) {
            layerDist = n2.dist;
        } else if (m_feature == Feature::SmoothF1) {
            // Simplified Smooth F1
            double h = qBound(0.0, 0.5 + 0.5 * (n2.dist - n1.dist) / 0.1, 1.0); // 0.1 is smoothness factor (fixed for now)
            layerDist = n1.dist * h + n2.dist * (1.0 - h) - 0.1 * h * (1.0 - h);
        } else if (m_feature == Feature::DistanceToEdge) {
            layerDist = n2.dist - n1.dist;
        } else if (m_feature == Feature::NSphereRadius) {
            layerDist = n1.dist;
        }

        if (i == 0) {
            finalDist = layerDist;
            finalColor = n1.color;
            finalPos = n1.pos;
            totalAmp = 1.0;
        } else {
            finalDist += layerDist * amp;
            totalAmp += amp;
        }

        freq *= lacunarityVal;
        amp *= roughnessVal;
        currentW *= lacunarityVal;
    }

    if (m_normalize) {
        // Normalize based on accumulated amplitude? 
        // Or just clamp? Voronoi distance is unbounded theoretically but usually < 1.
        // With fractal, it can grow.
        // Let's just clamp for now.
        finalDist = qBound(0.0, finalDist, 1.0);
    }

    if (socket == m_distanceOutput) {
        return finalDist;
    } else if (socket == m_colorOutput) {
        return QColor::fromRgbF(qBound(0.0, finalColor.x(), 1.0), qBound(0.0, finalColor.y(), 1.0), qBound(0.0, finalColor.z(), 1.0));
    } else if (socket == m_positionOutput) {
        return QVariant::fromValue(finalPos); 
    } else if (socket == m_wOutput) {
        return finalColor.x(); 
    } else if (socket == m_radiusOutput) {
        return finalDist; 
    }

    return QVariant();
}

// Getters
double VoronoiNode::scale() const { return m_scaleInput->defaultValue().toDouble(); }
double VoronoiNode::randomness() const { return m_randomnessInput->defaultValue().toDouble(); }
double VoronoiNode::detail() const { return m_detailInput->defaultValue().toDouble(); }
double VoronoiNode::roughness() const { return m_roughnessInput->defaultValue().toDouble(); }
double VoronoiNode::lacunarity() const { return m_lacunarityInput->defaultValue().toDouble(); }
double VoronoiNode::w() const { return m_wInput->defaultValue().toDouble(); }

// Setters
void VoronoiNode::setScale(double v) { m_scaleInput->setDefaultValue(v); setDirty(true); }
void VoronoiNode::setRandomness(double v) { m_randomnessInput->setDefaultValue(v); setDirty(true); }
void VoronoiNode::setDetail(double v) { m_detailInput->setDefaultValue(v); setDirty(true); }
void VoronoiNode::setRoughness(double v) { m_roughnessInput->setDefaultValue(v); setDirty(true); }
void VoronoiNode::setLacunarity(double v) { m_lacunarityInput->setDefaultValue(v); setDirty(true); }
void VoronoiNode::setW(double v) { m_wInput->setDefaultValue(v); setDirty(true); }
void VoronoiNode::setDimensions(Dimensions d) { m_dimensions = d; setDirty(true); notifyStructureChanged(); }
void VoronoiNode::setMetric(Metric m) { m_metric = m; setDirty(true); }
void VoronoiNode::setFeature(Feature f) { m_feature = f; setDirty(true); }
void VoronoiNode::setNormalize(bool b) { m_normalize = b; setDirty(true); }

QJsonObject VoronoiNode::save() const {
    QJsonObject json = Node::save();
    json["dimensions"] = static_cast<int>(m_dimensions);
    json["metric"] = static_cast<int>(m_metric);
    json["feature"] = static_cast<int>(m_feature);
    json["normalize"] = m_normalize;
    return json;
}

void VoronoiNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("dimensions")) m_dimensions = static_cast<Dimensions>(json["dimensions"].toInt());
    if (json.contains("metric")) m_metric = static_cast<Metric>(json["metric"].toInt());
    if (json.contains("feature")) m_feature = static_cast<Feature>(json["feature"].toInt());
    if (json.contains("normalize")) m_normalize = json["normalize"].toBool();
}

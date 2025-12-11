#include "watersourcenode.h"
#include "appsettings.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <QColor>
#include <QVector2D>
#include <QJsonArray>

WaterSourceNode::WaterSourceNode() : Node("Water Source") {
    m_noise = std::make_unique<PerlinNoise>();

    // === Inputs ===
    
    // Vector input (texture coordinates)
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vectorInput);

    // Position X - Lake center X offset
    m_positionXInput = new NodeSocket("Position X", SocketType::Float, SocketDirection::Input, this);
    m_positionXInput->setDefaultValue(0.0);
    addInputSocket(m_positionXInput);

    // Position Y - Lake center Y offset
    m_positionYInput = new NodeSocket("Position Y", SocketType::Float, SocketDirection::Input, this);
    m_positionYInput->setDefaultValue(0.0);
    addInputSocket(m_positionYInput);

    // Distortion strength
    m_mixFactorInput = new NodeSocket("Distortion", SocketType::Float, SocketDirection::Input, this);
    m_mixFactorInput->setDefaultValue(0.5);
    addInputSocket(m_mixFactorInput);

    // Noise Scale
    m_scaleInput = new NodeSocket("Noise Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(1.0);
    addInputSocket(m_scaleInput);

    // Detail (Octaves)
    m_detailInput = new NodeSocket("Detail", SocketType::Float, SocketDirection::Input, this);
    m_detailInput->setDefaultValue(15.0);
    addInputSocket(m_detailInput);

    // Roughness
    m_roughnessInput = new NodeSocket("Roughness", SocketType::Float, SocketDirection::Input, this);
    m_roughnessInput->setDefaultValue(0.736);
    addInputSocket(m_roughnessInput);

    // Lacunarity
    m_lacunarityInput = new NodeSocket("Lacunarity", SocketType::Float, SocketDirection::Input, this);
    m_lacunarityInput->setDefaultValue(2.0);
    addInputSocket(m_lacunarityInput);

    // Seed
    m_seedInput = new NodeSocket("Seed", SocketType::Float, SocketDirection::Input, this);
    m_seedInput->setDefaultValue(137.3);
    addInputSocket(m_seedInput);

    // === Outputs ===
    m_facOutput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Output, this);
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);

    addOutputSocket(m_facOutput);
    addOutputSocket(m_colorOutput);

    // === Default Color Ramp (like Blender's setup) ===
    addStop(0.0, Qt::black);
    addStop(0.486, Qt::black);
    addStop(0.645, Qt::white);
    addStop(1.0, Qt::white);
}

WaterSourceNode::~WaterSourceNode() {}

// === Color Ramp Methods ===

void WaterSourceNode::clearStops() {
    m_stops.clear();
    setDirty(true);
}

void WaterSourceNode::addStop(double pos, const QColor& color) {
    m_stops.append({pos, color});
    std::sort(m_stops.begin(), m_stops.end(), [](const Stop& a, const Stop& b) {
        return a.position < b.position;
    });
    setDirty(true);
}

void WaterSourceNode::removeStop(int index) {
    if (index >= 0 && index < m_stops.size() && m_stops.size() > 1) {
        m_stops.remove(index);
        setDirty(true);
    }
}

void WaterSourceNode::setStopPosition(int index, double pos) {
    if (index >= 0 && index < m_stops.size()) {
        m_stops[index].position = std::max(0.0, std::min(1.0, pos));
        std::sort(m_stops.begin(), m_stops.end(), [](const Stop& a, const Stop& b) {
            return a.position < b.position;
        });
        setDirty(true);
    }
}

void WaterSourceNode::setStopColor(int index, const QColor& color) {
    if (index >= 0 && index < m_stops.size()) {
        m_stops[index].color = color;
        setDirty(true);
    }
}

QColor WaterSourceNode::evaluateRamp(double t) {
    t = std::max(0.0, std::min(1.0, t));

    if (m_stops.isEmpty()) return Qt::black;
    if (m_stops.size() == 1) return m_stops[0].color;

    for (int i = 0; i < m_stops.size() - 1; ++i) {
        if (t >= m_stops[i].position && t <= m_stops[i+1].position) {
            double range = m_stops[i+1].position - m_stops[i].position;
            if (range < 0.0001) return m_stops[i].color;
            
            double localT = (t - m_stops[i].position) / range;
            
            double r = m_stops[i].color.redF() * (1.0 - localT) + m_stops[i+1].color.redF() * localT;
            double g = m_stops[i].color.greenF() * (1.0 - localT) + m_stops[i+1].color.greenF() * localT;
            double b = m_stops[i].color.blueF() * (1.0 - localT) + m_stops[i+1].color.blueF() * localT;
            double a = m_stops[i].color.alphaF() * (1.0 - localT) + m_stops[i+1].color.alphaF() * localT;
            
            return QColor::fromRgbF(r, g, b, a);
        }
    }

    if (t < m_stops.first().position) return m_stops.first().color;
    if (t > m_stops.last().position) return m_stops.last().color;

    return Qt::black;
}

QVector<Node::ParameterInfo> WaterSourceNode::parameters() const {
    return {
        ParameterInfo("Position X", -1.0, 1.0, 0.0, 0.01, "Lake center X position"),
        ParameterInfo("Position Y", -1.0, 1.0, 0.0, 0.01, "Lake center Y position"),
        ParameterInfo("Distortion", 0.0, 1.0, 0.5, 0.01, "Noise distortion strength"),
        ParameterInfo("Noise Scale", 0.1, 10.0, 1.0, 0.1, "Noise frequency"),
        ParameterInfo("Detail", 1.0, 15.0, 15.0, 1.0, "Noise octaves"),
        ParameterInfo("Roughness", 0.0, 1.0, 0.736, 0.01, "Noise roughness"),
        ParameterInfo("Lacunarity", 1.0, 4.0, 2.0, 0.1, "Noise lacunarity"),
        ParameterInfo("Seed", 0.0, 1000.0, 137.3, 1.0, "Random seed (W value)")
    };
}

void WaterSourceNode::evaluate() {
    // Stateless
}

QVariant WaterSourceNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // === 1. Get Input Coordinates ===
    QVector3D p;
    if (m_vectorInput->isConnected()) {
        p = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        // Default: Object-like coordinates centered at (0, 0)
        int w = AppSettings::instance().renderWidth();
        int h = AppSettings::instance().renderHeight();
        double u = (pos.x() + 0.5) / static_cast<double>(w);
        double v = (pos.y() + 0.5) / static_cast<double>(h);
        p = QVector3D(u - 0.5, v - 0.5, 0.0);
    }

    // === 2. Apply Position Offset (X and Y separately) ===
    double posX = m_positionXInput->isConnected() 
        ? m_positionXInput->getValue(pos).toDouble()
        : m_positionXInput->defaultValue().toDouble();
    double posY = m_positionYInput->isConnected() 
        ? m_positionYInput->getValue(pos).toDouble()
        : m_positionYInput->defaultValue().toDouble();
    p.setX(p.x() - posX);
    p.setY(p.y() - posY);

    // === 3. Get Parameters ===
    double distortion = m_mixFactorInput->isConnected() 
        ? m_mixFactorInput->getValue(pos).toDouble() 
        : m_mixFactorInput->defaultValue().toDouble();
    double noiseScale = m_scaleInput->isConnected() 
        ? m_scaleInput->getValue(pos).toDouble() 
        : m_scaleInput->defaultValue().toDouble();
    double detail = m_detailInput->isConnected() 
        ? m_detailInput->getValue(pos).toDouble() 
        : m_detailInput->defaultValue().toDouble();
    double roughness = m_roughnessInput->isConnected() 
        ? m_roughnessInput->getValue(pos).toDouble() 
        : m_roughnessInput->defaultValue().toDouble();
    double lacunarity = m_lacunarityInput->isConnected() 
        ? m_lacunarityInput->getValue(pos).toDouble() 
        : m_lacunarityInput->defaultValue().toDouble();
    double seed = m_seedInput->isConnected() 
        ? m_seedInput->getValue(pos).toDouble() 
        : m_seedInput->defaultValue().toDouble();

    // === 4. Calculate Distance from Center ===
    double centerDist = p.length();

    // === 5. Generate Noise (FBM) ===
    int octaves = static_cast<int>(detail);
    if (octaves < 1) octaves = 1;
    
    // Normalize noise to 0..1 range to prevent lake shrinkage
    double maxAmp = 0.0;
    double amp = 1.0;
    for(int i=0; i<octaves; ++i) {
        maxAmp += amp;
        amp *= roughness;
    }
    if (maxAmp <= 0.0) maxAmp = 1.0;

    // Use large offset to avoid (0,0) singularity in noise
    const double NOISE_OFFSET = 100.0;
    
    double rawNoiseX = m_noise->fbm(
        p.x() * noiseScale + NOISE_OFFSET, 
        p.y() * noiseScale + NOISE_OFFSET, 
        seed,
        octaves, lacunarity, roughness
    );
    double noiseX = (rawNoiseX / maxAmp) - 0.5;
    
    double rawNoiseY = m_noise->fbm(
        p.x() * noiseScale + NOISE_OFFSET + 123.456, 
        p.y() * noiseScale + NOISE_OFFSET + 789.012, 
        seed,
        octaves, lacunarity, roughness
    );
    double noiseY = (rawNoiseY / maxAmp) - 0.5;

    // === 6. Apply Distortion (Distance-based only, no tangential) ===
    // This avoids the left-tilt issue by using noise to modulate distance directly
    double dampRadius = 0.5;
    double damping = std::min(centerDist / dampRadius, 1.0);
    damping = damping * damping; // Quadratic falloff
    
    // Use average noise to modulate distance (symmetrical)
    // Increased strength coefficient from 0.3 to 1.5 to make distortion more visible
    double noiseAvg = (noiseX + noiseY) * 0.5;
    double distortedDist = centerDist + noiseAvg * distortion * damping * 1.5;

    // === 7. Spherical Gradient ===
    double gradient = 1.0 - (distortedDist * 2.0);
    gradient = std::clamp(gradient, 0.0, 1.0);

    // === 8. Apply Built-in Color Ramp ===
    QColor rampColor = evaluateRamp(gradient);
    double fac = 0.299 * rampColor.redF() + 0.587 * rampColor.greenF() + 0.114 * rampColor.blueF();

    // === 9. Output ===
    if (socket == m_colorOutput) {
        return rampColor;
    }
    
    return fac;
}

// === Save/Restore ===

QJsonObject WaterSourceNode::save() const {
    QJsonObject json = Node::save();
    
    QJsonArray stopsArray;
    for (const auto& stop : m_stops) {
        QJsonObject stopObj;
        stopObj["position"] = stop.position;
        stopObj["color"] = stop.color.name(QColor::HexArgb);
        stopsArray.append(stopObj);
    }
    json["colorRampStops"] = stopsArray;
    
    return json;
}

void WaterSourceNode::restore(const QJsonObject& json) {
    Node::restore(json);
    
    if (json.contains("colorRampStops")) {
        m_stops.clear();
        QJsonArray stopsArray = json["colorRampStops"].toArray();
        for (const auto& val : stopsArray) {
            QJsonObject stopObj = val.toObject();
            double pos = stopObj["position"].toDouble();
            QColor color(stopObj["color"].toString());
            m_stops.append({pos, color});
        }
        std::sort(m_stops.begin(), m_stops.end(), [](const Stop& a, const Stop& b) {
            return a.position < b.position;
        });
    }
}

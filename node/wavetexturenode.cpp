#include "wavetexturenode.h"
#include <cmath>
#include <QColor>

WaveTextureNode::WaveTextureNode() : Node("Wave Texture") {
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    
    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(5.0);
    
    m_distortionInput = new NodeSocket("Distortion", SocketType::Float, SocketDirection::Input, this);
    m_distortionInput->setDefaultValue(0.0);
    
    m_detailInput = new NodeSocket("Detail", SocketType::Float, SocketDirection::Input, this);
    m_detailInput->setDefaultValue(2.0);
    
    m_detailScaleInput = new NodeSocket("Detail Scale", SocketType::Float, SocketDirection::Input, this);
    m_detailScaleInput->setDefaultValue(1.0);
    
    m_detailRoughnessInput = new NodeSocket("Detail Roughness", SocketType::Float, SocketDirection::Input, this);
    m_detailRoughnessInput->setDefaultValue(0.5);
    
    m_phaseOffsetInput = new NodeSocket("Phase Offset", SocketType::Float, SocketDirection::Input, this);
    m_phaseOffsetInput->setDefaultValue(0.0);

    addInputSocket(m_vectorInput);
    addInputSocket(m_scaleInput);
    addInputSocket(m_distortionInput);
    addInputSocket(m_detailInput);
    addInputSocket(m_detailScaleInput);
    addInputSocket(m_detailRoughnessInput);
    addInputSocket(m_phaseOffsetInput);

    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    m_facOutput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Output, this);

    addOutputSocket(m_colorOutput);
    addOutputSocket(m_facOutput);
}

WaveTextureNode::~WaveTextureNode() {}

void WaveTextureNode::evaluate() {
    // Stateless
}

QVector<Node::ParameterInfo> WaveTextureNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Wave Type
    QStringList types = {"Bands", "Rings"};
    params.append(ParameterInfo("Wave Type", types, 
        QVariant::fromValue(static_cast<int>(m_waveType)),
        [this](const QVariant& v) { const_cast<WaveTextureNode*>(this)->setWaveType(static_cast<WaveType>(v.toInt())); }
    ));

    // Direction
    QStringList dirs = {"X", "Y", "Z", "Diagonal"};
    params.append(ParameterInfo("Direction", dirs, 
        QVariant::fromValue(static_cast<int>(m_waveDirection)),
        [this](const QVariant& v) { const_cast<WaveTextureNode*>(this)->setWaveDirection(static_cast<WaveDirection>(v.toInt())); }
    ));

    // Profile
    QStringList profiles = {"Sin", "Saw", "Tri"};
    params.append(ParameterInfo("Profile", profiles, 
        QVariant::fromValue(static_cast<int>(m_waveProfile)),
        [this](const QVariant& v) { const_cast<WaveTextureNode*>(this)->setWaveProfile(static_cast<WaveProfile>(v.toInt())); }
    ));

    params.append(ParameterInfo("Scale", 0.0, 100.0, 5.0));
    params.append(ParameterInfo("Distortion", 0.0, 100.0, 0.0));
    params.append(ParameterInfo("Detail", 0.0, 15.0, 2.0));
    params.append(ParameterInfo("Detail Scale", 0.0, 10.0, 1.0));
    params.append(ParameterInfo("Detail Roughness", 0.0, 1.0, 0.5));
    params.append(ParameterInfo("Phase Offset", -100.0, 100.0, 0.0));
    
    return params;
}

void WaveTextureNode::setWaveType(WaveType t) { m_waveType = t; setDirty(true); }
void WaveTextureNode::setWaveProfile(WaveProfile p) { m_waveProfile = p; setDirty(true); }
void WaveTextureNode::setWaveDirection(WaveDirection d) { m_waveDirection = d; setDirty(true); }

QVariant WaveTextureNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D p = pos;
    if (m_vectorInput->isConnected()) {
        p = m_vectorInput->getValue(pos).value<QVector3D>();
    }

    double scale = m_scaleInput->getValue(pos).toDouble();
    double distortion = m_distortionInput->getValue(pos).toDouble();
    double phase = m_phaseOffsetInput->getValue(pos).toDouble();

    // Simplified Wave Logic (ignoring detail/distortion for now to keep it simple, or add basic distortion)
    // To implement distortion, we'd need a noise function.
    
    double n = 0.0;
    if (m_waveType == WaveType::Bands) {
        if (m_waveDirection == WaveDirection::X) n = p.x() * scale;
        else if (m_waveDirection == WaveDirection::Y) n = p.y() * scale;
        else if (m_waveDirection == WaveDirection::Z) n = p.z() * scale;
        else n = (p.x() + p.y() + p.z()) * scale / 3.0;
    } else { // Rings
        QVector3D center(0,0,0); // Or offset?
        double dist = p.length(); 
        // Rings usually centered at origin or specific point. 
        // Blender rings are spherical from origin.
        n = dist * scale;
    }

    n += phase;

    // Apply Profile
    double val = 0.0;
    // Map n to 0-1 cycle
    // Sin: 0.5 + 0.5 * sin(n)
    // Saw: n - floor(n)
    // Tri: abs(n - floor(n + 0.5)) * 2
    
    // Blender Wave Texture uses 2*PI for Sin?
    // "Scale" in Blender usually means frequency.
    // Let's assume n is the argument.
    
    if (m_waveProfile == WaveProfile::Sin) {
        val = 0.5 + 0.5 * std::sin(n * 2.0 * M_PI); // Assuming scale 1 = 1 cycle per unit
    } else if (m_waveProfile == WaveProfile::Saw) {
        val = n - std::floor(n);
    } else if (m_waveProfile == WaveProfile::Tri) {
        val = std::abs(n - std::floor(n + 0.5)) * 2.0;
    }

    if (socket == m_facOutput) return val;
    if (socket == m_colorOutput) {
        int gray = static_cast<int>(val * 255);
        return QColor(gray, gray, gray);
    }
    return 0.0;
}

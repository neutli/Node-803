#include "everlingtexturenode.h"
#include <cmath>

EverlingTextureNode::EverlingTextureNode() 
    : Node("Everling Texture") 
    , m_noise(std::make_unique<PerlinNoise>(m_seed))
{
    // Input sockets
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    m_vectorInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_vectorInput);
    
    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(m_scale);
    addInputSocket(m_scaleInput);
    
    m_meanInput = new NodeSocket("Mean", SocketType::Float, SocketDirection::Input, this);
    m_meanInput->setDefaultValue(m_mean);
    addInputSocket(m_meanInput);
    
    m_stddevInput = new NodeSocket("Std Dev", SocketType::Float, SocketDirection::Input, this);
    m_stddevInput->setDefaultValue(m_stddev);
    addInputSocket(m_stddevInput);
    
    // Output sockets
    m_valueOutput = new NodeSocket("Value", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);
    
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
}

EverlingTextureNode::~EverlingTextureNode() {}

void EverlingTextureNode::evaluate() {
    // Stateless - computation happens in compute()
}

QVector<Node::ParameterInfo> EverlingTextureNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Access Method enum with setter
    params.append(ParameterInfo(
        "Access Method", 
        {"Stack", "Random", "Gaussian", "Mixed"},
        QVariant::fromValue(m_accessMethod),
        [this](const QVariant& v) {
            const_cast<EverlingTextureNode*>(this)->m_accessMethod = v.toInt();
            const_cast<EverlingTextureNode*>(this)->setDirty(true);
        },
        "Traversal Strategy:\nStack = Fractal veins\nRandom = Erosion patterns\nGaussian = Cloudy clusters\nMixed = Balanced"
    ));
    
    // Seed as integer with setter - regenerates noise
    ParameterInfo seedInfo;
    seedInfo.type = ParameterInfo::Int;
    seedInfo.name = "Seed";
    seedInfo.min = 0;
    seedInfo.max = 9999;
    seedInfo.defaultValue = m_seed;
    seedInfo.step = 1;
    seedInfo.tooltip = "Random seed (changes pattern)";
    seedInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_seed = v.toInt();
        self->m_noise = std::make_unique<PerlinNoise>(self->m_seed);
        // Force buffer regeneration by clearing cached values
        self->m_noise->clearEverlingCache();
        self->setDirty(true);
    };
    params.append(seedInfo);
    
    // Scale with setter
    ParameterInfo scaleInfo;
    scaleInfo.type = ParameterInfo::Float;
    scaleInfo.name = "Scale";
    scaleInfo.min = 0.01;
    scaleInfo.max = 100.0;
    scaleInfo.defaultValue = m_scale;
    scaleInfo.step = 0.1;
    scaleInfo.tooltip = "Texture scale";
    scaleInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_scale = v.toDouble();
        if (self->m_scaleInput) self->m_scaleInput->setDefaultValue(self->m_scale);
        self->setDirty(true);
    };
    params.append(scaleInfo);
    
    // Mean with setter
    ParameterInfo meanInfo;
    meanInfo.type = ParameterInfo::Float;
    meanInfo.name = "Mean";
    meanInfo.min = -5.0;
    meanInfo.max = 5.0;
    meanInfo.defaultValue = m_mean;
    meanInfo.step = 0.1;
    meanInfo.tooltip = "Gaussian mean (negative=valleys, positive=mountains)";
    meanInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_mean = v.toDouble();
        if (self->m_meanInput) self->m_meanInput->setDefaultValue(self->m_mean);
        self->setDirty(true);
    };
    params.append(meanInfo);
    
    // StdDev with setter
    ParameterInfo stddevInfo;
    stddevInfo.type = ParameterInfo::Float;
    stddevInfo.name = "Std Dev";
    stddevInfo.min = 0.1;
    stddevInfo.max = 10.0;
    stddevInfo.defaultValue = m_stddev;
    stddevInfo.step = 0.1;
    stddevInfo.tooltip = "Standard deviation (higher=more rugged)";
    stddevInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_stddev = v.toDouble();
        if (self->m_stddevInput) self->m_stddevInput->setDefaultValue(self->m_stddev);
        self->setDirty(true);
    };
    params.append(stddevInfo);
    
    return params;
}

QVariant EverlingTextureNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    // Get input coordinates
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        // Normalize pixel coordinates to 0-1 range
        vec = QVector3D(pos.x() / 512.0, pos.y() / 512.0, 0.0);
    }
    
    // Get parameters
    double scaleVal = m_scaleInput->isConnected() ? m_scaleInput->getValue(pos).toDouble() : m_scaleInput->defaultValue().toDouble();
    double meanVal = m_meanInput->isConnected() ? m_meanInput->getValue(pos).toDouble() : m_meanInput->defaultValue().toDouble();
    double stddevVal = m_stddevInput->isConnected() ? m_stddevInput->getValue(pos).toDouble() : m_stddevInput->defaultValue().toDouble();
    
    // Apply scale - NO OFFSET to avoid tiling at scale boundaries
    double bx = vec.x() * scaleVal;
    double by = vec.y() * scaleVal;
    double bz = vec.z() * scaleVal;
    
    // Generate noise with selected access method
    EverlingAccessMethod accessMethod = static_cast<EverlingAccessMethod>(m_accessMethod);
    double value = m_noise->everlingNoise(bx, by, bz, meanVal, stddevVal, accessMethod);
    
    // Return based on socket
    if (socket == m_valueOutput) {
        return value;
    } else if (socket == m_colorOutput) {
        float v = static_cast<float>(std::clamp(value, 0.0, 1.0));
        return QVariant::fromValue(QVector4D(v, v, v, 1.0f));
    }
    
    return value;
}

QJsonObject EverlingTextureNode::save() const {
    QJsonObject obj = Node::save();
    obj["type"] = "Everling Texture";
    obj["scale"] = m_scale;
    obj["mean"] = m_mean;
    obj["stddev"] = m_stddev;
    obj["accessMethod"] = m_accessMethod;
    obj["seed"] = m_seed;
    return obj;
}

void EverlingTextureNode::restore(const QJsonObject& data) {
    Node::restore(data);
    
    if (data.contains("scale")) m_scale = data["scale"].toDouble();
    if (data.contains("mean")) m_mean = data["mean"].toDouble();
    if (data.contains("stddev")) m_stddev = data["stddev"].toDouble();
    if (data.contains("accessMethod")) m_accessMethod = data["accessMethod"].toInt();
    if (data.contains("seed")) {
        m_seed = data["seed"].toInt();
        m_noise = std::make_unique<PerlinNoise>(m_seed);
    }
    
    // Update socket defaults
    if (m_scaleInput) m_scaleInput->setDefaultValue(m_scale);
    if (m_meanInput) m_meanInput->setDefaultValue(m_mean);
    if (m_stddevInput) m_stddevInput->setDefaultValue(m_stddev);
}

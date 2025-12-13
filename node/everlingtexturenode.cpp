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

    m_clusterSpreadInput = new NodeSocket("Spread", SocketType::Float, SocketDirection::Input, this);
    m_clusterSpreadInput->setDefaultValue(m_clusterSpread);
    addInputSocket(m_clusterSpreadInput);
    
    m_distortionInput = new NodeSocket("Distortion", SocketType::Float, SocketDirection::Input, this);
    m_distortionInput->setDefaultValue(m_distortion);
    addInputSocket(m_distortionInput);
    
    m_detailInput = new NodeSocket("Detail", SocketType::Float, SocketDirection::Input, this);
    m_detailInput->setDefaultValue(2.0); // Maps to lacunarity/octaves loosely or just use for Octaves input
    // Actually standard is "Detail" = Octaves.
    m_detailInput->setDefaultValue((float)m_octaves); // 1.0
    addInputSocket(m_detailInput);
    
    // Use Value socket (0 or 1) although typical UI is checkbox, socket allows driving it.
    // However, bool params are usually not sockets. User asked for "smooth edges item".
    // I will NOT add a socket for Smooth Edge to keep it simple, only a parameter.
    // Wait, the plan said "Additional parameters". 
    // Let's stick to Parameter-only for Smooth Edges as boolean sockets are rare.
    // But Spread should be a socket.
    
    // Output sockets
    
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
    
    // Cluster Spread (Gaussian Mode only)
    ParameterInfo spreadInfo;
    spreadInfo.type = ParameterInfo::Float;
    spreadInfo.name = "Cluster Spread";
    spreadInfo.min = 0.05;
    spreadInfo.max = 2.0;
    spreadInfo.defaultValue = m_clusterSpread;
    spreadInfo.step = 0.05;
    spreadInfo.tooltip = "Cluster spread (Gaussian mode only)";
    spreadInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_clusterSpread = v.toDouble();
        if (self->m_clusterSpreadInput) self->m_clusterSpreadInput->setDefaultValue(self->m_clusterSpread);
        self->setDirty(true);
    };
    params.append(spreadInfo);

    // Smooth Edges (Checkbox)
    ParameterInfo smoothInfo;
    smoothInfo.type = ParameterInfo::Bool;
    smoothInfo.name = "Smooth Edges";
    smoothInfo.defaultValue = m_smoothEdges;
    smoothInfo.tooltip = "Fade edges to prevent hard cuts at tile boundaries";
    smoothInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_smoothEdges = v.toBool();
        self->setDirty(true);
    };
    params.append(smoothInfo);

    // Smooth Width (Float, only if Smooth Edges is on ideally, but we show always)
    ParameterInfo smoothWidthInfo;
    smoothWidthInfo.type = ParameterInfo::Float;
    smoothWidthInfo.name = "Smooth Width";
    smoothWidthInfo.min = 0.01;
    smoothWidthInfo.max = 0.5;
    smoothWidthInfo.defaultValue = m_smoothWidth;
    smoothWidthInfo.step = 0.01;
    smoothWidthInfo.tooltip = "Width of the edge transition (0.0 - 0.5)";
    smoothWidthInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_smoothWidth = v.toDouble();
        self->setDirty(true);
    };
    params.append(smoothWidthInfo);

    // Grid Size (Int)
    ParameterInfo gridInfo;
    gridInfo.type = ParameterInfo::Int;
    gridInfo.name = "Tile Resolution";
    gridInfo.min = 16;
    gridInfo.max = 1024; // Limit to prevent freezing
    gridInfo.defaultValue = m_gridSize;
    gridInfo.step = 16;
    gridInfo.tooltip = "Internal simulation grid size. Higher = Larger non-repeating area but slower generation.";
    gridInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_gridSize = v.toInt();
        self->setDirty(true);
    };
    params.append(gridInfo);
    
    // Periodicity
    ParameterInfo periodInfo;
    periodInfo.type = ParameterInfo::Combo;
    periodInfo.name = "Tiling Mode";
    periodInfo.options = {"Repeat (Hard Edge)", "Mirror (Seamless)"};
    periodInfo.defaultValue = QVariant::fromValue(m_periodicity);
    periodInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_periodicity = v.toInt();
        self->setDirty(true);
    };
    params.append(periodInfo);

    // Distortion
    ParameterInfo distInfo;
    distInfo.type = ParameterInfo::Float;
    distInfo.name = "Distortion";
    distInfo.min = 0.0; distInfo.max = 10.0;
    distInfo.defaultValue = m_distortion;
    distInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_distortion = v.toDouble();
        if (self->m_distortionInput) self->m_distortionInput->setDefaultValue(self->m_distortion);
        self->setDirty(true);
    };
    params.append(distInfo);
    
    // Detail (Octaves)
    ParameterInfo octaveInfo;
    octaveInfo.type = ParameterInfo::Int;
    octaveInfo.name = "Detail";
    octaveInfo.min = 1; octaveInfo.max = 10;
    octaveInfo.defaultValue = m_octaves;
    octaveInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_octaves = v.toInt();
        if (self->m_detailInput) self->m_detailInput->setDefaultValue(self->m_octaves);
        self->setDirty(true);
    };
    params.append(octaveInfo);
    
    // Gain (Roughness)
    ParameterInfo gainInfo;
    gainInfo.type = ParameterInfo::Float;
    gainInfo.name = "Roughness";
    gainInfo.min = 0.0; gainInfo.max = 1.0;
    gainInfo.defaultValue = m_gain;
    gainInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<EverlingTextureNode*>(this);
        self->m_gain = v.toDouble();
        self->setDirty(true);
    };
    params.append(gainInfo);

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
    double spreadVal = m_clusterSpreadInput->isConnected() ? m_clusterSpreadInput->getValue(pos).toDouble() : m_clusterSpreadInput->defaultValue().toDouble();
    double distVal = m_distortionInput->isConnected() ? m_distortionInput->getValue(pos).toDouble() : m_distortionInput->defaultValue().toDouble();
    double detailVal = m_detailInput->isConnected() ? m_detailInput->getValue(pos).toDouble() : m_detailInput->defaultValue().toDouble();
    int octaves = std::clamp((int)detailVal, 1, 15);
    
    // Apply scale - NO OFFSET to avoid tiling at scale boundaries
    double bx = vec.x() * scaleVal;
    double by = vec.y() * scaleVal;
    double bz = vec.z() * scaleVal;
    
    // Generate noise with selected access method
    EverlingAccessMethod accessMethod = static_cast<EverlingAccessMethod>(m_accessMethod);
    EverlingPeriodicity periodicity = static_cast<EverlingPeriodicity>(m_periodicity);
    
    // Force Mirror if legacy smoothEdges is ON and we are in default Wrap mode (Backward compat)
    if (m_smoothEdges && periodicity == EverlingPeriodicity::Wrap) {
       // Ideally we migrate, but for now let's just use user pref
       // Actually user hated smoothEdges logic, so let's stick to periodicity enum primarily.
    }
    
    double value = m_noise->everlingNoise(bx, by, bz, meanVal, stddevVal, accessMethod, 
                                          spreadVal, m_smoothEdges, m_gridSize, m_smoothWidth,
                                          periodicity, distVal, octaves, m_lacunarity, m_gain);
    
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
    obj["clusterSpread"] = m_clusterSpread;
    obj["smoothEdges"] = m_smoothEdges;
    obj["smoothWidth"] = m_smoothWidth;
    obj["gridSize"] = m_gridSize;
    obj["accessMethod"] = m_accessMethod;
    obj["seed"] = m_seed;
    obj["periodicity"] = m_periodicity;
    obj["distortion"] = m_distortion;
    obj["octaves"] = m_octaves;
    obj["gain"] = m_gain;
    return obj;
}

void EverlingTextureNode::restore(const QJsonObject& data) {
    Node::restore(data);
    
    if (data.contains("scale")) m_scale = data["scale"].toDouble();
    if (data.contains("mean")) m_mean = data["mean"].toDouble();
    if (data.contains("stddev")) m_stddev = data["stddev"].toDouble();
    if (data.contains("clusterSpread")) m_clusterSpread = data["clusterSpread"].toDouble();
    if (data.contains("smoothEdges")) m_smoothEdges = data["smoothEdges"].toBool();
    if (data.contains("smoothWidth")) m_smoothWidth = data["smoothWidth"].toDouble();
    if (data.contains("gridSize")) m_gridSize = data["gridSize"].toInt();
    if (data.contains("periodicity")) m_periodicity = data["periodicity"].toInt();
    if (data.contains("distortion")) m_distortion = data["distortion"].toDouble();
    if (data.contains("octaves")) m_octaves = data["octaves"].toInt();
    if (data.contains("gain")) m_gain = data["gain"].toDouble();
    
    if (data.contains("accessMethod")) m_accessMethod = data["accessMethod"].toInt();
    if (data.contains("seed")) {
        m_seed = data["seed"].toInt();
        m_noise = std::make_unique<PerlinNoise>(m_seed);
    }
    
    // Update socket defaults
    if (m_scaleInput) m_scaleInput->setDefaultValue(m_scale);
    if (m_meanInput) m_meanInput->setDefaultValue(m_mean);
    if (m_stddevInput) m_stddevInput->setDefaultValue(m_stddev);
    if (m_clusterSpreadInput) m_clusterSpreadInput->setDefaultValue(m_clusterSpread);
    if (m_distortionInput) m_distortionInput->setDefaultValue(m_distortion);
    if (m_detailInput) m_detailInput->setDefaultValue(m_octaves);
}

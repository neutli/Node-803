#include "colorkeynode.h"
#include <cmath>

ColorKeyNode::ColorKeyNode()
    : Node("Color Key")
{
    // Input socket - the image/color to process
    m_colorInput = new NodeSocket("Color", SocketType::Color, SocketDirection::Input, this);
    m_colorInput->setDefaultValue(QVector4D(1, 1, 1, 1)); // White default
    addInputSocket(m_colorInput);
    
    // Output sockets
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
    
    m_alphaOutput = new NodeSocket("Alpha", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_alphaOutput);
}

ColorKeyNode::~ColorKeyNode() {}

void ColorKeyNode::evaluate() {
    // Stateless
}

double ColorKeyNode::colorDistance(const QVector4D& c1, const QVector4D& c2) const {
    // Euclidean distance in RGB space (normalized 0-1)
    double dr = c1.x() - c2.x();
    double dg = c1.y() - c2.y();
    double db = c1.z() - c2.z();
    return std::sqrt(dr * dr + dg * dg + db * db);
}

QVariant ColorKeyNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    // Get input color - MUST be connected for this node to work
    if (!m_colorInput->isConnected()) {
        // Not connected - return fully opaque white
        if (socket == m_alphaOutput) {
            return 1.0;
        }
        return QVariant::fromValue(QVector4D(1, 1, 1, 1));
    }
    
    // Get input color value
    QVector4D inputColor(1, 1, 1, 1);
    QVariant val = m_colorInput->getValue(pos);
    if (val.canConvert<QVector4D>()) {
        inputColor = val.value<QVector4D>();
    } else if (val.canConvert<QColor>()) {
        QColor c = val.value<QColor>();
        inputColor = QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    } else {
        // Float value - treat as grayscale
        double v = std::clamp(val.toDouble(), 0.0, 1.0);
        inputColor = QVector4D(v, v, v, 1.0);
    }
    
    // Key color - the color we want to make transparent
    QVector4D keyVec(m_keyColor.redF(), m_keyColor.greenF(), m_keyColor.blueF(), 1.0);
    
    // Compute color distance
    double dist = colorDistance(inputColor, keyVec);
    
    // Maximum possible distance in RGB cube is sqrt(3) ≈ 1.732
    // But we use normalized tolerance (0-1 = 0% to 100% of max distance)
    double maxDist = 1.732; // sqrt(3)
    double normalizedDist = dist / maxDist;
    
    // Compute alpha: 
    // - If distance <= tolerance: alpha = 0 (transparent, color matches key)
    // - If distance > tolerance + falloff: alpha = 1 (opaque, color doesn't match)
    // - In between: smooth transition
    double alpha;
    if (normalizedDist <= m_tolerance) {
        alpha = 0.0; // Fully transparent - this color should be removed
    } else if (normalizedDist <= m_tolerance + m_falloff) {
        // Smooth transition
        alpha = (normalizedDist - m_tolerance) / std::max(0.001, m_falloff);
    } else {
        alpha = 1.0; // Fully opaque - keep this color
    }
    
    // Invert if requested (selected color becomes opaque, others transparent)
    if (m_invert) {
        alpha = 1.0 - alpha;
    }
    
    // Alpha output
    if (socket == m_alphaOutput) {
        return alpha;
    }
    
    // Color output - RGB stays the same, only alpha changes
    // This allows compositing: transparent areas show through to background
    QVector4D outputColor(inputColor.x(), inputColor.y(), inputColor.z(), alpha);
    return QVariant::fromValue(outputColor);
}

QVector<Node::ParameterInfo> ColorKeyNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Key Color - the color to remove/make transparent
    ParameterInfo keyInfo;
    keyInfo.type = ParameterInfo::Color;
    keyInfo.name = "Key Color";
    keyInfo.defaultValue = m_keyColor;
    keyInfo.tooltip = "Color to make transparent (click to pick)";
    keyInfo.setter = [this](const QVariant& v) {
        auto* self = const_cast<ColorKeyNode*>(this);
        if (v.canConvert<QColor>()) {
            self->m_keyColor = v.value<QColor>();
        }
        self->setDirty(true);
    };
    params.append(keyInfo);
    
    // Tolerance - how close colors need to be to match
    ParameterInfo tolInfo;
    tolInfo.type = ParameterInfo::Float;
    tolInfo.name = "Tolerance";
    tolInfo.min = 0.0;
    tolInfo.max = 1.0;
    tolInfo.defaultValue = m_tolerance;
    tolInfo.step = 0.01;
    tolInfo.tooltip = "Color matching range (0=exact match only, 1=all colors)";
    tolInfo.setter = [this](const QVariant& v) {
        const_cast<ColorKeyNode*>(this)->m_tolerance = v.toDouble();
        const_cast<ColorKeyNode*>(this)->setDirty(true);
    };
    params.append(tolInfo);
    
    // Falloff - edge softness
    ParameterInfo fallInfo;
    fallInfo.type = ParameterInfo::Float;
    fallInfo.name = "Falloff";
    fallInfo.min = 0.0;
    fallInfo.max = 0.5;
    fallInfo.defaultValue = m_falloff;
    fallInfo.step = 0.01;
    fallInfo.tooltip = "Edge softness for smooth transitions";
    fallInfo.setter = [this](const QVariant& v) {
        const_cast<ColorKeyNode*>(this)->m_falloff = v.toDouble();
        const_cast<ColorKeyNode*>(this)->setDirty(true);
    };
    params.append(fallInfo);
    
    // Invert
    params.append(ParameterInfo("Invert", m_invert,
        [this](const QVariant& v) {
            const_cast<ColorKeyNode*>(this)->m_invert = v.toBool();
            const_cast<ColorKeyNode*>(this)->setDirty(true);
        },
        "Invert: make key color opaque, others transparent"));
    
    return params;
}

QJsonObject ColorKeyNode::save() const {
    QJsonObject json = Node::save();
    json["type"] = "Color Key";
    json["keyColorR"] = m_keyColor.redF();
    json["keyColorG"] = m_keyColor.greenF();
    json["keyColorB"] = m_keyColor.blueF();
    json["tolerance"] = m_tolerance;
    json["falloff"] = m_falloff;
    json["invert"] = m_invert;
    return json;
}

void ColorKeyNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("keyColorR") && json.contains("keyColorG") && json.contains("keyColorB")) {
        m_keyColor = QColor::fromRgbF(
            json["keyColorR"].toDouble(),
            json["keyColorG"].toDouble(),
            json["keyColorB"].toDouble()
        );
    }
    if (json.contains("tolerance")) m_tolerance = json["tolerance"].toDouble();
    if (json.contains("falloff")) m_falloff = json["falloff"].toDouble();
    if (json.contains("invert")) m_invert = json["invert"].toBool();
}

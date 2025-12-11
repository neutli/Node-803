#include "texturecoordinatenode.h"
#include "rendercontext.h"
#include "appsettings.h"

TextureCoordinateNode::TextureCoordinateNode()
    : Node("Texture Coordinate"), m_coordinateType(CoordinateType::UV)
{
    // Coordinate type selector (0=Generated, 1=Object, 2=UV, etc.)
    m_typeInput = new NodeSocket("Type", SocketType::Integer, SocketDirection::Input, this);
    m_typeInput->setDefaultValue(2);  // Default to UV mode
    addInputSocket(m_typeInput);
    
    m_output = new NodeSocket("UV", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_output);
}

TextureCoordinateNode::~TextureCoordinateNode() {}

QVector<Node::ParameterInfo> TextureCoordinateNode::parameters() const {
    QVector<ParameterInfo> params;
    
    QStringList types = {"Generated", "Object", "UV", "Camera", "Window", "Normal", "Reflection"};
    // Use the member getter/setter logic
    // But note that coordinateType() reads from m_typeInput->value()
    // and setCoordinateType() sets m_typeInput->setDefaultValue().
    // This is because older implementation used the socket to store the state.
    
    params.append(ParameterInfo("Coordinate", types, 
        QVariant::fromValue(static_cast<int>(coordinateType())),
        [this](const QVariant& v) { const_cast<TextureCoordinateNode*>(this)->setCoordinateType(static_cast<CoordinateType>(v.toInt())); }
    ));
    
    return params;
}

void TextureCoordinateNode::evaluate() {
    // Stateless
}

QVariant TextureCoordinateNode::compute(const QVector3D& pixelPos, NodeSocket* socket) {
    // Get resolution and viewport range from global AppSettings
    int w = AppSettings::instance().renderWidth();
    int h = AppSettings::instance().renderHeight();
    
    // Normalize pixel coordinates to 0..1
    double normU = (pixelPos.x() + 0.5) / (double)w;
    double normV = (pixelPos.y() + 0.5) / (double)h;
    
    // Map to viewport range
    double minU = AppSettings::instance().viewportMinU();
    double minV = AppSettings::instance().viewportMinV();
    double maxU = AppSettings::instance().viewportMaxU();
    double maxV = AppSettings::instance().viewportMaxV();
    
    double u = minU + normU * (maxU - minU);
    double v = minV + normV * (maxV - minV);
    
    // Apply coordinate type transformation
    int type = 1; // Default to Object mode
    if (m_typeInput) {
        type = m_typeInput->value().toInt();
    }
    
    switch (type) {
        case 0: // Generated (0..1)
            // Already in 0..1 range, no change
            break;
            
        case 1: // Object (-1..1, centered)
            u = (u - 0.5) * 2.0;
            v = (v - 0.5) * 2.0;
            break;
            
        case 2: // UV (0..1, same as Generated for now)
            break;
            
        case 3: // Camera (not implemented, fallback to Generated)
        case 4: // Window (not implemented, fallback to Generated)
        case 5: // Normal (not implemented, fallback to Generated)
        case 6: // Reflection (not implemented, fallback to Generated)
        default:
            break;
    }
    
    return QVector3D(u, v, 0.0);
}

TextureCoordinateNode::CoordinateType TextureCoordinateNode::coordinateType() const {
    if (m_typeInput) {
        return static_cast<CoordinateType>(m_typeInput->value().toInt());
    }
    return CoordinateType::Object; // Default
}

void TextureCoordinateNode::setCoordinateType(CoordinateType type) {
    if (m_typeInput) {
        m_typeInput->setDefaultValue(static_cast<int>(type));
    }
    m_coordinateType = type;
    setDirty(true);
}

#include "polygonnode.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PolygonNode::PolygonNode()
    : Node("Polygon")
{
    // Input sockets - only Vector for coordinates
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    m_vectorInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_vectorInput);
    
    // Output sockets
    m_valueOutput = new NodeSocket("Value", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);
    
    m_distanceOutput = new NodeSocket("Distance", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_distanceOutput);
}

PolygonNode::~PolygonNode() {}

void PolygonNode::evaluate() {
    // Stateless - computation in compute()
}

// Signed distance function for regular polygon
double PolygonNode::polygonSDF(double x, double y, int sides, double radius, double rotation) const {
    if (sides < 3) sides = 3;
    
    // Apply rotation
    double rotRad = rotation * M_PI / 180.0;
    double cosR = std::cos(rotRad);
    double sinR = std::sin(rotRad);
    double rx = x * cosR - y * sinR;
    double ry = x * sinR + y * cosR;
    
    // Convert to polar coordinates
    double angle = std::atan2(ry, rx);
    double dist = std::sqrt(rx * rx + ry * ry);
    
    // Angle per side
    double sideAngle = 2.0 * M_PI / sides;
    
    // Map angle to one sector
    double sectorAngle = std::fmod(angle + M_PI, sideAngle) - sideAngle * 0.5;
    
    // Distance to polygon edge in this sector
    double edgeDist = radius * std::cos(sideAngle * 0.5);
    double projDist = dist * std::cos(sectorAngle);
    
    // Signed distance (negative inside, positive outside)
    return projDist - edgeDist;
}

QVariant PolygonNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    // Get input coordinates
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        // Normalize to -0.5 to 0.5 (centered)
        vec = QVector3D(pos.x() / 512.0 - 0.5, pos.y() / 512.0 - 0.5, 0.0);
    }
    
    // Use member parameters
    double sides = qBound(3.0, m_sides, 32.0);
    double radius = qBound(0.01, m_radius, 1.0);
    double rotation = m_rotation;
    
    // Intepolate between floor(sides) and ceil(sides) for smooth transition
    int sidesFloored = static_cast<int>(std::floor(sides));
    int sidesCeiled = static_cast<int>(std::ceil(sides));
    double fract = sides - sidesFloored;
    
    double sdf;
    
    // Regular polygon (Seed == 0) - Use analytical formula (faster/exact)
    if (m_seed == 0) {
        if (sidesFloored == sidesCeiled) {
            sdf = polygonSDF(vec.x(), vec.y(), sidesFloored, radius, rotation);
        } else {
            double d1 = polygonSDF(vec.x(), vec.y(), sidesFloored, radius, rotation);
            double d2 = polygonSDF(vec.x(), vec.y(), sidesCeiled, radius, rotation);
            sdf = d1 * (1.0 - fract) + d2 * fract;
        }
    } else {
        // Irregular polygon (Seed != 0) - Generate vertices
        QVector2D p(vec.x(), vec.y());
        
        if (sidesFloored == sidesCeiled) {
            QList<QVector2D> v = generateVertices(sidesFloored, radius, rotation, m_seed);
            sdf = sdArbitraryPolygon(v, p);
        } else {
            QList<QVector2D> v1 = generateVertices(sidesFloored, radius, rotation, m_seed);
            QList<QVector2D> v2 = generateVertices(sidesCeiled, radius, rotation, m_seed);
            double d1 = sdArbitraryPolygon(v1, p);
            double d2 = sdArbitraryPolygon(v2, p);
            sdf = d1 * (1.0 - fract) + d2 * fract;
        }
    }
    
    if (socket == m_distanceOutput) {
        // Return normalized distance (0 at edge, positive outside, negative inside)
        return sdf;
    }
    
    // Value output
    if (m_fill) {
        // Fill: 1 inside, 0 outside
        return sdf <= 0.0 ? 1.0 : 0.0;
    } else {
        // Edge only: 1 on edge, 0 elsewhere
        double edge = std::abs(sdf) < m_edgeWidth ? 1.0 : 0.0;
        return edge;
    }
}

QVector<Node::ParameterInfo> PolygonNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Sides with setter (Float now)
    ParameterInfo sidesInfo;
    sidesInfo.type = ParameterInfo::Float; // Changed from Int
    sidesInfo.name = "Sides";
    sidesInfo.min = 3.0;
    sidesInfo.max = 32.0;
    sidesInfo.defaultValue = m_sides;
    sidesInfo.step = 0.1; // Finer step for smooth transition
    sidesInfo.tooltip = "Number of sides (fractional supported)";
    sidesInfo.setter = [this](const QVariant& v) {
        const_cast<PolygonNode*>(this)->setSides(v.toDouble());
    };
    params.append(sidesInfo);
    
    // Radius with setter
    ParameterInfo radiusInfo;
    radiusInfo.type = ParameterInfo::Float;
    radiusInfo.name = "Radius";
    radiusInfo.min = 0.01;
    radiusInfo.max = 1.0;
    radiusInfo.defaultValue = m_radius;
    radiusInfo.step = 0.01;
    radiusInfo.tooltip = "Polygon radius";
    radiusInfo.setter = [this](const QVariant& v) {
        const_cast<PolygonNode*>(this)->setRadius(v.toDouble());
    };
    params.append(radiusInfo);
    
    // Rotation with setter
    ParameterInfo rotInfo;
    rotInfo.type = ParameterInfo::Float;
    rotInfo.name = "Rotation";
    rotInfo.min = 0.0;
    rotInfo.max = 360.0;
    rotInfo.defaultValue = m_rotation;
    rotInfo.step = 1.0;
    rotInfo.tooltip = "Rotation in degrees";
    rotInfo.setter = [this](const QVariant& v) {
        const_cast<PolygonNode*>(this)->setRotation(v.toDouble());
    };
    params.append(rotInfo);
    
    params.append(ParameterInfo("Fill", m_fill,
        [this](const QVariant& v) {
            const_cast<PolygonNode*>(this)->setFill(v.toBool());
        },
        "Fill interior (off = edge only)"));
    
    // Edge Width with setter
    ParameterInfo edgeInfo;
    edgeInfo.type = ParameterInfo::Float;
    edgeInfo.name = "Edge Width";
    edgeInfo.min = 0.001;
    edgeInfo.max = 0.1;
    edgeInfo.defaultValue = m_edgeWidth;
    edgeInfo.step = 0.001;
    edgeInfo.tooltip = "Edge line width";
    edgeInfo.setter = [this](const QVariant& v) {
        const_cast<PolygonNode*>(this)->setEdgeWidth(v.toDouble());
    };
    params.append(edgeInfo);
    
    // Seed parameter
    ParameterInfo seedInfo;
    seedInfo.type = ParameterInfo::Int;
    seedInfo.name = "Seed";
    seedInfo.min = 0;
    seedInfo.max = 10000;
    seedInfo.defaultValue = m_seed;
    seedInfo.tooltip = "Random seed (0 = regular polygon)";
    seedInfo.setter = [this](const QVariant& v) {
        const_cast<PolygonNode*>(this)->setSeed(v.toInt());
    };
    params.append(seedInfo);
    
    return params;
}

void PolygonNode::setSides(double v) { m_sides = v; setDirty(true); }
void PolygonNode::setRadius(double v) { m_radius = v; setDirty(true); }
void PolygonNode::setRotation(double v) { m_rotation = v; setDirty(true); }
void PolygonNode::setFill(bool v) { m_fill = v; setDirty(true); }
void PolygonNode::setEdgeWidth(double v) { m_edgeWidth = v; setDirty(true); }
QJsonObject PolygonNode::save() const {
    QJsonObject json = Node::save();
    json["type"] = "Polygon";
    json["sides"] = m_sides;
    json["radius"] = m_radius;
    json["rotation"] = m_rotation;
    json["fill"] = m_fill;
    json["edgeWidth"] = m_edgeWidth;
    json["seed"] = m_seed;
    return json;
}

void PolygonNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("sides")) m_sides = json["sides"].toDouble();
    if (json.contains("radius")) m_radius = json["radius"].toDouble();
    if (json.contains("rotation")) m_rotation = json["rotation"].toDouble();
    if (json.contains("fill")) m_fill = json["fill"].toBool();
    if (json.contains("edgeWidth")) m_edgeWidth = json["edgeWidth"].toDouble();
    if (json.contains("seed")) m_seed = json["seed"].toInt();
}

void PolygonNode::setSeed(int v) { m_seed = v; setDirty(true); }

double PolygonNode::sdArbitraryPolygon(const QList<QVector2D>& v, const QVector2D& p) const {
    double d = QVector2D::dotProduct(p - v[0], p - v[0]);
    double s = 1.0;
    for (int i = 0, j = v.size() - 1; i < v.size(); j = i, i++) {
        QVector2D e = v[j] - v[i];
        QVector2D w = p - v[i];
        QVector2D b = w - e * qBound(0.0, QVector2D::dotProduct(w, e) / QVector2D::dotProduct(e, e), 1.0);
        d = std::min(d, static_cast<double>(QVector2D::dotProduct(b, b)));
        
        bool cond1 = p.y() >= v[i].y();
        bool cond2 = p.y() < v[j].y();
        bool cond3 = e.x() * w.y() > e.y() * w.x();
        if ((cond1 && cond2 && cond3) || (!cond1 && !cond2 && !cond3)) s *= -1.0;
    }
    return s * std::sqrt(d);
}

QList<QVector2D> PolygonNode::generateVertices(int sides, double radius, double rotation, int seed) const {
    QList<QVector2D> v;
    if (sides < 3) sides = 3;
    double rotRad = rotation * M_PI / 180.0;
    
    for (int i = 0; i < sides; ++i) {
        double angle = 2.0 * M_PI * i / sides + rotRad;
        double currentRadius = radius;
        
        if (seed != 0) {
             // Simple hash-based randomization
             double h = std::sin(seed * 12.9898 + i * 78.233) * 43758.5453;
             h = h - std::floor(h);
             // Vary radius by 0.5x to 1.5x
             currentRadius *= (0.5 + 1.0 * h); 
        }
        
        v.append(QVector2D(std::cos(angle) * currentRadius, std::sin(angle) * currentRadius));
    }
    return v;
}

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
double PolygonNode::polygonSDF(double x, double y, double sides, double radius, double rotation) const {
    // Determine effective sides. 
    // If sides < 2.0, clamp to 2 (digon/line) to avoid division by zero or negative.
    // User wants 2.5 to be star. Standard formula works for fractional.
    if (sides < 1.0) sides = 1.0;
    
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

// Helper logic for fractional sides implemented in compute/generate
// sdStar removed to rely on robust sdArbitraryPolygon with reordered vertices


QVariant PolygonNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        vec = QVector3D(pos.x() / 512.0 - 0.5, pos.y() / 512.0 - 0.5, 0.0);
    }
    
    double sides = qBound(2.0, m_sides, 32.0);
    double radius = qBound(0.01, m_radius, 1.0);
    double rotation = m_rotation;
    
    double sdf;
    
    if (m_seed == 0) {
        // Star Detection: Check if sides is fractional P/Q
        int bestP = -1, bestQ = -1;
        double minErr = 0.01;
        
        // Only check for stars if fraction is significant
        if (std::abs(sides - std::round(sides)) > 0.01) {
            for (int q = 2; q <= 5; ++q) {
                double pFloat = sides * q;
                int pInt = static_cast<int>(std::round(pFloat));
                if (std::abs(pFloat - pInt) < minErr) {
                    bestP = pInt;
                    bestQ = q;
                    break;
                }
            }
        }
        
        if (bestP != -1) {
            // Found fractional star (e.g. 2.5 -> 5/2)
            // Generate P vertices on the circle
            QList<QVector2D> polyVerts = generateVertices(bestP, radius, rotation, 0);
            
            // Reorder vertices based on stride Q
            // e.g. 5/2: 0 -> 2 -> 4 -> 1 -> 3 -> (0)
            QList<QVector2D> starVerts;
            int currentIdx = 0;
            // We need to visit all vertices. 
            // For standard star polygons (P, Q coprime), this loops P times.
            bool *visited = new bool[bestP]; // Simple safety
            for(int i=0; i<bestP; ++i) visited[i] = false;
            
            for (int i = 0; i < bestP; ++i) {
                starVerts.append(polyVerts[currentIdx]);
                visited[currentIdx] = true;
                currentIdx = (currentIdx + bestQ) % bestP;
            }
            delete[] visited;
            
            // Render using arbitrary polygon logic which handles self-intersection (Odd-Even rule)
            sdf = sdArbitraryPolygon(starVerts, QVector2D(vec.x(), vec.y()));
            
        } else {
            // Regular Polygon
            sdf = polygonSDF(vec.x(), vec.y(), sides, radius, rotation);
        }
    } else {
        // Irregular polygon (Seed != 0)
        int sidesInt = static_cast<int>(std::round(sides));
        if (sidesInt < 3) sidesInt = 3;
        
        QVector2D p(vec.x(), vec.y());
        QList<QVector2D> v = generateVertices(sidesInt, radius, rotation, m_seed);
        sdf = sdArbitraryPolygon(v, p);
    }
    
    if (socket == m_distanceOutput) return sdf;
    
    if (m_fill) {
        return sdf <= 0.0 ? 1.0 : 0.0;
    } else {
        return std::abs(sdf) < m_edgeWidth ? 1.0 : 0.0;
    }
}

QVector<Node::ParameterInfo> PolygonNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // Sides with setter (Float now)
    ParameterInfo sidesInfo;
    sidesInfo.type = ParameterInfo::Float; // Changed from Int
    sidesInfo.name = "Sides";
    sidesInfo.min = 2.0;
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
    int winding = 0;
    
    for (int i = 0, j = v.size() - 1; i < v.size(); j = i, i++) {
        QVector2D e = v[j] - v[i];
        QVector2D w = p - v[i];
        QVector2D b = w - e * std::clamp(QVector2D::dotProduct(w, e) / QVector2D::dotProduct(e, e), 0.0f, 1.0f);
        
        // Exact distance to segment
        d = std::min(d, static_cast<double>(QVector2D::dotProduct(b, b)));
        
        // Winding Number Calculation
        bool cond1 = p.y() >= v[i].y();
        bool cond2 = p.y() < v[j].y();
        
        if (cond1 && cond2) {
             // Upward crossing
             // Check if P is to the left of edge
             if (e.x() * w.y() - e.y() * w.x() > 0) winding++;
        } else if (!cond1 && !cond2) {
             // Downward crossing (v[j].y <= p.y < v[i].y)
             // Check if P is to the right? No, standard algorithm:
             // if (v[i].y > p.y && v[j].y <= p.y)
             if (e.x() * w.y() - e.y() * w.x() < 0) winding--;
        }
    }
    
    // Determine sign based on winding number (Non-Zero Rule)
    // If winding != 0, we are inside.
    if (winding != 0) s = -1.0;
    
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

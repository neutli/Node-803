#include "pointcreatenode.h"
#include <cmath>
#include <algorithm>

PointCreateNode::PointCreateNode()
    : Node("Point Create")
{
    // Input sockets
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    m_vectorInput->setDefaultValue(QVector3D(0, 0, 0));
    addInputSocket(m_vectorInput);
    
    // Output sockets
    m_pointsOutput = new NodeSocket("Distance", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_pointsOutput);
    
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
}

PointCreateNode::~PointCreateNode() {}

void PointCreateNode::evaluate() {
    // Regenerate points if needed
    regeneratePoints();
}

void PointCreateNode::regeneratePoints() const {
    int totalCount = (m_distribution == Distribution::Grid) ? (m_countX * m_countY) : m_count;
    
    if (m_cachedSeed == m_seed && m_cachedDist == m_distribution && m_cachedCount == totalCount && !m_points.isEmpty()) {
        return; // No regeneration needed
    }
    
    m_points.clear();
    std::mt19937 rng(m_seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    switch (m_distribution) {
        case Distribution::Grid: {
            for (int y = 0; y < m_countY; ++y) {
                for (int x = 0; x < m_countX; ++x) {
                    double px = (x + 0.5) / m_countX;
                    double py = (y + 0.5) / m_countY;
                    
                    // Apply jitter
                    if (m_jitter > 0) {
                        px += (dist(rng) - 0.5) * m_jitter / m_countX;
                        py += (dist(rng) - 0.5) * m_jitter / m_countY;
                        px = std::clamp(px, 0.0, 1.0);
                        py = std::clamp(py, 0.0, 1.0);
                    }
                    
                    m_points.append(QVector2D(px, py));
                }
            }
            break;
        }
        
        case Distribution::Random: {
            for (int i = 0; i < m_count; ++i) {
                m_points.append(QVector2D(dist(rng), dist(rng)));
            }
            break;
        }
        
        case Distribution::Poisson: {
            // Simple Poisson disk sampling approximation
            double minDist = 1.0 / std::sqrt((double)m_count * 2.0);
            int maxAttempts = 30;
            
            // Start with one random point
            m_points.append(QVector2D(dist(rng), dist(rng)));
            
            QVector<int> activeList;
            activeList.append(0);
            
            while (!activeList.isEmpty() && m_points.size() < m_count) {
                int idx = std::uniform_int_distribution<int>(0, activeList.size() - 1)(rng);
                QVector2D point = m_points[activeList[idx]];
                
                bool foundValid = false;
                for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                    double angle = dist(rng) * 2.0 * 3.14159265;
                    double r = minDist * (1.0 + dist(rng));
                    
                    double nx = point.x() + r * std::cos(angle);
                    double ny = point.y() + r * std::sin(angle);
                    
                    if (nx < 0 || nx > 1 || ny < 0 || ny > 1) continue;
                    
                    // Check distance to all existing points
                    bool valid = true;
                    for (const QVector2D& p : m_points) {
                        double d = std::sqrt((p.x() - nx) * (p.x() - nx) + (p.y() - ny) * (p.y() - ny));
                        if (d < minDist) {
                            valid = false;
                            break;
                        }
                    }
                    
                    if (valid) {
                        m_points.append(QVector2D(nx, ny));
                        activeList.append(m_points.size() - 1);
                        foundValid = true;
                        break;
                    }
                }
                
                if (!foundValid) {
                    activeList.remove(idx);
                }
            }
            break;
        }
    }
    
    m_cachedSeed = m_seed;
    m_cachedDist = m_distribution;
    m_cachedCount = totalCount;
}

double PointCreateNode::findNearestDistance(double x, double y) const {
    double minDist = 10.0;
    for (const QVector2D& p : m_points) {
        double dx = p.x() - x;
        double dy = p.y() - y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < minDist) minDist = d;
    }
    return minDist;
}

int PointCreateNode::findNearestIndex(double x, double y) const {
    double minDist = 10.0;
    int nearestIdx = 0;
    for (int i = 0; i < m_points.size(); ++i) {
        double dx = m_points[i].x() - x;
        double dy = m_points[i].y() - y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < minDist) {
            minDist = d;
            nearestIdx = i;
        }
    }
    return nearestIdx;
}

QVariant PointCreateNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QMutexLocker locker(&m_mutex);
    
    regeneratePoints();
    
    // Get input coordinates
    QVector3D vec;
    if (m_vectorInput->isConnected()) {
        vec = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        vec = QVector3D(pos.x() / 512.0, pos.y() / 512.0, 0.0);
    }
    
    double x = vec.x();
    double y = vec.y();
    
    if (socket == m_pointsOutput) {
        // Return distance to nearest point (normalized)
        double dist = findNearestDistance(x, y);
        return std::clamp(dist * 5.0, 0.0, 1.0); // Scale for visibility
    } else if (socket == m_colorOutput) {
        // Return color based on nearest point index
        int idx = findNearestIndex(x, y);
        // Generate pseudo-random color from index
        std::mt19937 rng(idx * 12345 + m_seed);
        std::uniform_real_distribution<float> cdist(0.2f, 1.0f);
        float r = cdist(rng);
        float g = cdist(rng);
        float b = cdist(rng);
        return QVariant::fromValue(QVector4D(r, g, b, 1.0f));
    }
    
    return 0.0;
}

QVector<Node::ParameterInfo> PointCreateNode::parameters() const {
    QVector<ParameterInfo> params;
    
    params.append(ParameterInfo(
        "Distribution", 
        {"Grid", "Random", "Poisson"},
        QVariant::fromValue(static_cast<int>(m_distribution)),
        [this](const QVariant& v) {
            const_cast<PointCreateNode*>(this)->m_distribution = static_cast<Distribution>(v.toInt());
            const_cast<PointCreateNode*>(this)->setDirty(true);
        },
        "Point distribution type"));
    
    params.append(ParameterInfo("Count X", 1.0, 20.0, (double)m_countX, 1.0, "Grid columns"));
    params.append(ParameterInfo("Count Y", 1.0, 20.0, (double)m_countY, 1.0, "Grid rows"));
    params.append(ParameterInfo("Count", 1.0, 500.0, (double)m_count, 1.0, "Total points (Random/Poisson)"));
    params.append(ParameterInfo("Jitter", 0.0, 1.0, m_jitter, 0.01, "Random offset for Grid"));
    
    ParameterInfo seedInfo;
    seedInfo.type = ParameterInfo::Int;
    seedInfo.name = "Seed";
    seedInfo.min = 0;
    seedInfo.max = 9999;
    seedInfo.defaultValue = m_seed;
    seedInfo.step = 1;
    seedInfo.tooltip = "Random seed";
    seedInfo.setter = [this](const QVariant& v) {
        const_cast<PointCreateNode*>(this)->m_seed = v.toInt();
        const_cast<PointCreateNode*>(this)->setDirty(true);
    };
    params.append(seedInfo);
    
    return params;
}

QJsonObject PointCreateNode::save() const {
    QJsonObject json = Node::save();
    json["type"] = "Point Create";
    json["distribution"] = static_cast<int>(m_distribution);
    json["countX"] = m_countX;
    json["countY"] = m_countY;
    json["count"] = m_count;
    json["jitter"] = m_jitter;
    json["seed"] = m_seed;
    return json;
}

void PointCreateNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("distribution")) m_distribution = static_cast<Distribution>(json["distribution"].toInt());
    if (json.contains("countX")) m_countX = json["countX"].toInt();
    if (json.contains("countY")) m_countY = json["countY"].toInt();
    if (json.contains("count")) m_count = json["count"].toInt();
    if (json.contains("jitter")) m_jitter = json["jitter"].toDouble();
    if (json.contains("seed")) m_seed = json["seed"].toInt();
}

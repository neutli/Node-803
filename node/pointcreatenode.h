#ifndef POINTCREATENODE_H
#define POINTCREATENODE_H

#include "node.h"
#include <QRecursiveMutex>
#include <QVector2D>
#include <random>

// Point Create Node - Generates point distribution patterns
class PointCreateNode : public Node {
public:
    PointCreateNode();
    ~PointCreateNode() override;
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;
    
    enum class Distribution {
        Grid,
        Random,
        Poisson
    };
    
private:
    // Input sockets
    NodeSocket* m_vectorInput;
    
    // Output sockets
    NodeSocket* m_pointsOutput;    // Distance to nearest point
    NodeSocket* m_colorOutput;     // Point ID as color
    
    // Parameters
    int m_countX = 5;              // Points in X (Grid mode)
    int m_countY = 5;              // Points in Y (Grid mode)
    int m_count = 25;              // Total points (Random/Poisson mode)
    Distribution m_distribution = Distribution::Grid;
    int m_seed = 0;
    double m_jitter = 0.0;         // Random offset for Grid mode (0-1)
    
    // Cached points
    mutable QVector<QVector2D> m_points;
    mutable int m_cachedSeed = -1;
    mutable Distribution m_cachedDist = Distribution::Grid;
    mutable int m_cachedCount = -1;
    
    mutable QRecursiveMutex m_mutex;
    
    void regeneratePoints() const;
    double findNearestDistance(double x, double y) const;
    int findNearestIndex(double x, double y) const;
};

#endif // POINTCREATENODE_H

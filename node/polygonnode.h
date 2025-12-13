#ifndef POLYGONNODE_H
#define POLYGONNODE_H

#include "node.h"
#include <QRecursiveMutex>

// Polygon Node - Generates regular polygon shapes
class PolygonNode : public Node {
public:
    PolygonNode();
    ~PolygonNode() override;
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;
    
private:
    // Input sockets (only Vector for coordinates)
    NodeSocket* m_vectorInput;
    
    // Output sockets
    NodeSocket* m_valueOutput;
    NodeSocket* m_distanceOutput;
    
    // Parameters (controlled via UI only, not sockets)
    double m_sides = 6.0;      // Number of sides (3.0-32.0)
    double m_radius = 0.4;     // Radius (0-1)
    double m_rotation = 0.0;   // Rotation in degrees
    bool m_fill = true;        // Fill interior
    double m_edgeWidth = 0.02; // Edge line width
    int m_seed = 0;            // Random seed (0 = regular)
    
    // Helpers
    void setSides(double v);
    void setRadius(double v);
    void setRotation(double v);
    void setFill(bool v);
    void setEdgeWidth(double v);
    void setSeed(int v);
    
    mutable QRecursiveMutex m_mutex;
    
    // Helper: compute signed distance to polygon edge
    double polygonSDF(double x, double y, double sides, double radius, double rotation) const;
    
    // Helper: compute signed distance to arbitrary polygon
    double sdArbitraryPolygon(const QList<QVector2D>& v, const QVector2D& p) const;
    
    // Helper: generate vertices with optional randomization
    QList<QVector2D> generateVertices(int sides, double radius, double rotation, int seed) const;
};

#endif // POLYGONNODE_H

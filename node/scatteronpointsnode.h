#ifndef SCATTERONPOINTSNODE_H
#define SCATTERONPOINTSNODE_H

#include "node.h"
#include <QRecursiveMutex>

// Scatter on Points Node - Places texture instances at point locations
class ScatterOnPointsNode : public Node {
public:
    ScatterOnPointsNode();
    ~ScatterOnPointsNode() override;
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;
    
private:
    // Input sockets
    NodeSocket* m_vectorInput;
    NodeSocket* m_textureInput;   // Texture to scatter
    NodeSocket* m_densityInput;   // Point density/mask
    
    // Output sockets
    NodeSocket* m_colorOutput;
    NodeSocket* m_valueOutput;
    
    // Parameters
    double m_scale = 0.2;           // Instance scale
    double m_scaleVariation = 0.0;  // Random scale variation
    double m_rotation = 0.0;        // Base rotation
    double m_rotationVariation = 0.0; // Random rotation variation
    int m_seed = 0;
    int m_pointsX = 5;              // Grid points X
    int m_pointsY = 5;              // Grid points Y
    
    mutable QRecursiveMutex m_mutex;
};

#endif // SCATTERONPOINTSNODE_H

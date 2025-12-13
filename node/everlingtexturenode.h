#ifndef EVERLINGTEXTURENODE_H
#define EVERLINGTEXTURENODE_H

#include "node.h"
#include "noise.h"
#include <memory>
#include <QRecursiveMutex>

// Everling Texture Node - Dedicated node for Everling Noise
// Based on "Everling Noise: A Linear-Time Noise Algorithm for Multi-Dimensional Procedural Terrain Generation"
class EverlingTextureNode : public Node {
public:
    EverlingTextureNode();
    ~EverlingTextureNode() override;
    
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    void evaluate() override;
    
    // Unified Parameter API
    QVector<ParameterInfo> parameters() const override;
    QJsonObject save() const override;
    void restore(const QJsonObject& data) override;
    
private:
    NodeSocket* m_vectorInput;
    NodeSocket* m_scaleInput;
    NodeSocket* m_meanInput;
    NodeSocket* m_stddevInput;
    
    NodeSocket* m_valueOutput;
    NodeSocket* m_colorOutput;
    
    // Parameters (stored for UI defaults, actual values come from sockets)
    double m_scale = 5.0;        // Texture scale
    double m_mean = 0.0;         // Gaussian mean (terrain bias)
    double m_stddev = 1.0;       // Gaussian stddev (ruggedness)
    int m_accessMethod = 3;      // 0=Stack, 1=Random, 2=Gaussian, 3=Mixed
    int m_seed = 0;              // Random seed
    
    // Noise generator
    std::unique_ptr<PerlinNoise> m_noise;
    mutable QRecursiveMutex m_mutex;
};

#endif // EVERLINGTEXTURENODE_H

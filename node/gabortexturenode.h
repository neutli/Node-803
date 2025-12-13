#ifndef GABORTEXTURENODE_H
#define GABORTEXTURENODE_H

#include "node.h"
#include "noise.h"
#include <memory>
#include <QRecursiveMutex>

class GaborTextureNode : public Node {
public:
    GaborTextureNode();
    ~GaborTextureNode() override;
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;
    
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;
    
private:
    std::unique_ptr<PerlinNoise> m_noise;
    mutable QRecursiveMutex m_mutex;
    
    // Input sockets
    NodeSocket* m_vectorInput;
    NodeSocket* m_scaleInput;
    NodeSocket* m_frequencyInput;   // Gabor: Wave frequency
    NodeSocket* m_anisotropyInput;  // Gabor: 0=isotropic, 1=directional
    NodeSocket* m_orientationInput; // Gabor: Direction vector (3D)
    NodeSocket* m_distortionInput;  // Domain warping
    
    // Output sockets
    NodeSocket* m_valueOutput;
    NodeSocket* m_phaseOutput;
    NodeSocket* m_intensityOutput;
};

#endif // GABORTEXTURENODE_H

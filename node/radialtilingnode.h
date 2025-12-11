#ifndef RADIALTILINGNODE_H
#define RADIALTILINGNODE_H

#include "node.h"

class RadialTilingNode : public Node {
public:
    RadialTilingNode();
    ~RadialTilingNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

private:
    NodeSocket* m_vectorInput;
    NodeSocket* m_sidesInput;
    NodeSocket* m_roundnessInput;
    NodeSocket* m_output;
};

#endif // RADIALTILINGNODE_H

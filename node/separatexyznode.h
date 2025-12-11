#ifndef SEPARATEXYZNODE_H
#define SEPARATEXYZNODE_H

#include "node.h"

class SeparateXYZNode : public Node {
public:
    SeparateXYZNode();
    ~SeparateXYZNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

private:
    NodeSocket* m_vectorInput;
    NodeSocket* m_xOutput;
    NodeSocket* m_yOutput;
    NodeSocket* m_zOutput;
};

#endif // SEPARATEXYZNODE_H

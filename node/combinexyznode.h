#ifndef COMBINEXYZNODE_H
#define COMBINEXYZNODE_H

#include "node.h"

class CombineXYZNode : public Node {
public:
    CombineXYZNode();
    ~CombineXYZNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

private:
    NodeSocket* m_xInput;
    NodeSocket* m_yInput;
    NodeSocket* m_zInput;
    NodeSocket* m_vectorOutput;
};

#endif // COMBINEXYZNODE_H

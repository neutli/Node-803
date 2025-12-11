#ifndef CLAMPNODE_H
#define CLAMPNODE_H

#include "node.h"

class ClampNode : public Node {
public:
    ClampNode();
    ~ClampNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

private:
    NodeSocket* m_valueInput;
    NodeSocket* m_minInput;
    NodeSocket* m_maxInput;
    NodeSocket* m_output;
};

#endif // CLAMPNODE_H

#ifndef INVERTNODE_H
#define INVERTNODE_H

#include "node.h"

class InvertNode : public Node {
public:
    InvertNode();
    ~InvertNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

    double fac() const;
    void setFac(double v);

private:
    NodeSocket* m_colorInput;
    NodeSocket* m_facInput;
    NodeSocket* m_colorOutput;
};

#endif // INVERTNODE_H

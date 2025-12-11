#ifndef BUMPNODE_H
#define BUMPNODE_H

#include "node.h"

class BumpNode : public Node {
public:
    BumpNode();
    ~BumpNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;

    bool invert() const { return m_invert; }
    void setInvert(bool inv);

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    NodeSocket* m_strengthInput;
    NodeSocket* m_distanceInput;
    NodeSocket* m_heightInput;
    NodeSocket* m_normalInput;
    NodeSocket* m_normalOutput;
    
    bool m_invert;
};

#endif // BUMPNODE_H

#ifndef PRINCIPLEDBSDFNODE_H
#define PRINCIPLEDBSDFNODE_H

#include "node.h"

class PrincipledBSDFNode : public Node {
public:
    PrincipledBSDFNode();
    ~PrincipledBSDFNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;

private:
    NodeSocket* m_baseColorInput;
    NodeSocket* m_metallicInput;
    NodeSocket* m_roughnessInput;
    NodeSocket* m_iorInput;
    NodeSocket* m_alphaInput;
    NodeSocket* m_normalInput;
    NodeSocket* m_bsdfOutput;
};

#endif // PRINCIPLEDBSDFNODE_H

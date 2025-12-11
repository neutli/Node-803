#ifndef MIXSHADERNODE_H
#define MIXSHADERNODE_H

#include "node.h"

class MixShaderNode : public Node {
public:
    MixShaderNode();
    ~MixShaderNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;

private:
    NodeSocket* m_facInput;
    NodeSocket* m_shader1Input;
    NodeSocket* m_shader2Input;
    NodeSocket* m_shaderOutput;
};

#endif // MIXSHADERNODE_H

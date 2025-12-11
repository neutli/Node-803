#ifndef VECTORMATHNODE_H
#define VECTORMATHNODE_H

#include "node.h"

enum class VectorMathOperation {
    Add, Subtract, Multiply, Divide,
    Cross, Dot,
    Distance, Length,
    Scale, Normalize,
    Absolute, Minimum, Maximum,
    Floor, Ceil, Fraction, Modulo, Wrap, Snap,
    Sine, Cosine, Tangent,
    Reflect, Refract, Faceforward
};

class VectorMathNode : public Node {
public:
    VectorMathNode();
    ~VectorMathNode() override = default;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

    void setOperation(VectorMathOperation op);

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    VectorMathOperation m_operation;

    NodeSocket* m_vector1Input;
    NodeSocket* m_vector2Input;
    NodeSocket* m_vector3Input; // For Faceforward/Refract/Wrap etc.
    NodeSocket* m_scaleInput; // For Scale operation
    
    NodeSocket* m_vectorOutput;
    NodeSocket* m_valueOutput;
};

#endif // VECTORMATHNODE_H

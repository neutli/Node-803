#ifndef MATHNODE_H
#define MATHNODE_H

#include "node.h"

enum class MathOperation {
    Add, Subtract, Multiply, Divide, MultiplyAdd,
    Logarithm, Sqrt, InverseSqrt, Absolute, Exponent,
    Minimum, Maximum, LessThan, GreaterThan, Sign, Compare,
    SmoothMin, SmoothMax,
    Round, Ceil, Floor, Fraction, Modulo, FlooredModulo, Wrap, Snap, PingPong,
    Sine, Cosine, Tangent, Arcsine, Arccosine, Arctangent, Arctangent2,
    Sinh, Cosh, Tanh,
    ToRadians, ToDegrees
};

class MathNode : public Node {
public:
    MathNode();
    ~MathNode() override = default;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

    void setOperation(MathOperation op);
    void setUseClamp(bool v);

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    MathOperation m_operation;
    bool m_useClamp;

    NodeSocket* m_value1Input;
    NodeSocket* m_value2Input;
    NodeSocket* m_value3Input;
    
    NodeSocket* m_valueOutput;
};

#endif // MATHNODE_H

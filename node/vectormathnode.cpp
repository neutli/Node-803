#include "vectormathnode.h"
#include <cmath>
#include <algorithm>
#include <QtMath>

VectorMathNode::VectorMathNode() 
    : Node("Vector Math")
    , m_operation(VectorMathOperation::Add)
{
    // Inputs
    m_vector1Input = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vector1Input);

    m_vector2Input = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vector2Input);
    
    m_vector3Input = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vector3Input); // Used for some ops

    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(1.0);
    addInputSocket(m_scaleInput);

    // Outputs
    m_vectorOutput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Output, this);
    addOutputSocket(m_vectorOutput);
    
    m_valueOutput = new NodeSocket("Value", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);
}

QVector<Node::ParameterInfo> VectorMathNode::parameters() const {
    return {
        ParameterInfo("Scale", -10000.0, 10000.0, 1.0)
    };
}

void VectorMathNode::evaluate() {
    // Stateless
}

QVariant VectorMathNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D v1 = m_vector1Input->getValue(pos).value<QVector3D>();
    QVector3D v2 = m_vector2Input->getValue(pos).value<QVector3D>();
    QVector3D v3 = m_vector3Input->getValue(pos).value<QVector3D>();
    double s = m_scaleInput->getValue(pos).toDouble();
    
    QVector3D resVec;
    double resVal = 0.0;
    
    switch (m_operation) {
        case VectorMathOperation::Add: resVec = v1 + v2; break;
        case VectorMathOperation::Subtract: resVec = v1 - v2; break;
        case VectorMathOperation::Multiply: resVec = v1 * v2; break;
        case VectorMathOperation::Divide: 
            resVec = QVector3D(
                (v2.x() != 0) ? v1.x() / v2.x() : 0,
                (v2.y() != 0) ? v1.y() / v2.y() : 0,
                (v2.z() != 0) ? v1.z() / v2.z() : 0
            );
            break;
            
        case VectorMathOperation::Cross: resVec = QVector3D::crossProduct(v1, v2); break;
        case VectorMathOperation::Dot: resVal = QVector3D::dotProduct(v1, v2); break;
        
        case VectorMathOperation::Distance: resVal = v1.distanceToPoint(v2); break;
        case VectorMathOperation::Length: resVal = v1.length(); break;
        
        case VectorMathOperation::Scale: resVec = v1 * s; break;
        case VectorMathOperation::Normalize: resVec = v1.normalized(); break;
        
        case VectorMathOperation::Absolute: 
            resVec = QVector3D(std::abs(v1.x()), std::abs(v1.y()), std::abs(v1.z())); 
            break;
        case VectorMathOperation::Minimum: 
            resVec = QVector3D(std::min(v1.x(), v2.x()), std::min(v1.y(), v2.y()), std::min(v1.z(), v2.z())); 
            break;
        case VectorMathOperation::Maximum: 
            resVec = QVector3D(std::max(v1.x(), v2.x()), std::max(v1.y(), v2.y()), std::max(v1.z(), v2.z())); 
            break;
            
        case VectorMathOperation::Floor:
            resVec = QVector3D(std::floor(v1.x()), std::floor(v1.y()), std::floor(v1.z()));
            break;
        case VectorMathOperation::Ceil:
            resVec = QVector3D(std::ceil(v1.x()), std::ceil(v1.y()), std::ceil(v1.z()));
            break;
        case VectorMathOperation::Fraction:
            resVec = QVector3D(v1.x() - std::floor(v1.x()), v1.y() - std::floor(v1.y()), v1.z() - std::floor(v1.z()));
            break;
        case VectorMathOperation::Modulo:
            resVec = QVector3D(
                (v2.x() != 0) ? std::fmod(v1.x(), v2.x()) : 0,
                (v2.y() != 0) ? std::fmod(v1.y(), v2.y()) : 0,
                (v2.z() != 0) ? std::fmod(v1.z(), v2.z()) : 0
            );
            break;
        case VectorMathOperation::Wrap: {
            // Wrap v1 between v2 and v3
            auto wrap = [](double val, double min, double max) {
                double range = max - min;
                if (range == 0) return min;
                return min + (val - min) - range * std::floor((val - min) / range);
            };
            resVec = QVector3D(
                wrap(v1.x(), v2.x(), v3.x()),
                wrap(v1.y(), v2.y(), v3.y()),
                wrap(v1.z(), v2.z(), v3.z())
            );
            break;
        }
        case VectorMathOperation::Snap: {
            auto snap = [](double val, double step) {
                if (step == 0) return val;
                return std::floor(val / step + 0.5) * step;
            };
            resVec = QVector3D(
                snap(v1.x(), v2.x()),
                snap(v1.y(), v2.y()),
                snap(v1.z(), v2.z())
            );
            break;
        }
        
        case VectorMathOperation::Sine:
            resVec = QVector3D(std::sin(v1.x()), std::sin(v1.y()), std::sin(v1.z()));
            break;
        case VectorMathOperation::Cosine:
            resVec = QVector3D(std::cos(v1.x()), std::cos(v1.y()), std::cos(v1.z()));
            break;
        case VectorMathOperation::Tangent:
            resVec = QVector3D(std::tan(v1.x()), std::tan(v1.y()), std::tan(v1.z()));
            break;
            
        case VectorMathOperation::Reflect:
            // I - 2.0 * dot(N, I) * N
            // v1 = I, v2 = N (should be normalized)
            {
                QVector3D n = v2.normalized();
                resVec = v1 - 2.0 * QVector3D::dotProduct(n, v1) * n;
            }
            break;
        case VectorMathOperation::Refract:
            // v1 = I, v2 = N, s = eta
            // k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
            // if k < 0, return 0
            // else return eta * I - (eta * dot(N, I) + sqrt(k)) * N
            {
                QVector3D i = v1.normalized();
                QVector3D n = v2.normalized();
                double eta = s;
                double dotNI = QVector3D::dotProduct(n, i);
                double k = 1.0 - eta * eta * (1.0 - dotNI * dotNI);
                if (k < 0.0) resVec = QVector3D(0, 0, 0);
                else resVec = eta * i - (eta * dotNI + std::sqrt(k)) * n;
            }
            break;
        case VectorMathOperation::Faceforward:
            // v1 = N, v2 = I, v3 = Nref
            // if dot(Nref, I) < 0 return N else return -N
            {
                if (QVector3D::dotProduct(v3, v2) < 0.0) resVec = v1;
                else resVec = -v1;
            }
            break;
    }
    
    if (socket == m_vectorOutput) {
        return resVec;
    } else if (socket == m_valueOutput) {
        return resVal;
    }
    
    return QVariant();
}

void VectorMathNode::setOperation(VectorMathOperation op) {
    m_operation = op;
    setDirty(true);
}

QJsonObject VectorMathNode::save() const {
    QJsonObject json = Node::save();
    json["operation"] = static_cast<int>(m_operation);
    return json;
}

void VectorMathNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("operation")) {
        m_operation = static_cast<VectorMathOperation>(json["operation"].toInt());
    }
}

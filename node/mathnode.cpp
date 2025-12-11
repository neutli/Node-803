#include "mathnode.h"
#include <cmath>
#include <algorithm>
#include <QtMath>

MathNode::MathNode() 
    : Node("Math")
    , m_operation(MathOperation::Add)
    , m_useClamp(false)
{
    // Inputs
    m_value1Input = new NodeSocket("Value", SocketType::Float, SocketDirection::Input, this);
    m_value1Input->setDefaultValue(0.5);
    addInputSocket(m_value1Input);

    m_value2Input = new NodeSocket("Value", SocketType::Float, SocketDirection::Input, this);
    m_value2Input->setDefaultValue(0.5);
    addInputSocket(m_value2Input);
    
    m_value3Input = new NodeSocket("Value", SocketType::Float, SocketDirection::Input, this);
    m_value3Input->setDefaultValue(0.0);
    addInputSocket(m_value3Input); // Only used for Multiply Add

    // Output
    m_valueOutput = new NodeSocket("Value", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);
}

QVector<Node::ParameterInfo> MathNode::parameters() const {
    return {
        ParameterInfo("Value", -10000.0, 10000.0, 0.5)
    };
}

void MathNode::evaluate() {
    // Stateless
}

QVariant MathNode::compute(const QVector3D& pos, NodeSocket* socket) {
    double v1 = m_value1Input->getValue(pos).toDouble();
    double v2 = m_value2Input->getValue(pos).toDouble();
    double v3 = m_value3Input->getValue(pos).toDouble();
    
    double result = 0.0;
    
    switch (m_operation) {
        case MathOperation::Add: result = v1 + v2; break;
        case MathOperation::Subtract: result = v1 - v2; break;
        case MathOperation::Multiply: result = v1 * v2; break;
        case MathOperation::Divide: result = (v2 != 0.0) ? v1 / v2 : 0.0; break;
        case MathOperation::MultiplyAdd: result = v1 * v2 + v3; break;
        
        case MathOperation::Logarithm: result = (v1 > 0.0 && v2 > 0.0 && v2 != 1.0) ? std::log(v1) / std::log(v2) : 0.0; break;
        case MathOperation::Sqrt: result = (v1 >= 0.0) ? std::sqrt(v1) : 0.0; break;
        case MathOperation::InverseSqrt: result = (v1 > 0.0) ? 1.0 / std::sqrt(v1) : 0.0; break;
        case MathOperation::Absolute: result = std::abs(v1); break;
        case MathOperation::Exponent: result = std::pow(v1, v2); break;
        
        case MathOperation::Minimum: result = std::min(v1, v2); break;
        case MathOperation::Maximum: result = std::max(v1, v2); break;
        case MathOperation::LessThan: result = (v1 < v2) ? 1.0 : 0.0; break;
        case MathOperation::GreaterThan: result = (v1 > v2) ? 1.0 : 0.0; break;
        case MathOperation::Sign: result = (v1 > 0.0) ? 1.0 : ((v1 < 0.0) ? -1.0 : 0.0); break;
        case MathOperation::Compare: result = (std::abs(v1 - v2) <= 0.00001) ? 1.0 : 0.0; break; // Epsilon check? Blender uses epsilon parameter usually
        
        case MathOperation::SmoothMin: {
            // Smooth Min (Polynomial smin)
            // k = v3? No, usually v3 is not used for this in standard nodes unless exposed.
            // Let's assume v3 is not used or we need a parameter. 
            // Blender Math node doesn't use v3 for smooth min, it uses 'Distance' (v2?).
            // If v2 is distance/smoothness:
            double h = std::max(0.0, std::min(1.0, (v2 - v1 + v3) / (2.0 * v3))); // Wait, formula needs check.
            // Standard: h = max( k-abs(a-b), 0 ) / k; res = min(a,b) - h*h*k*(1/4);
            // Blender uses: d = max(0, min(1, (b - a + c) / (2 * c))); return a * d + b * (1 - d) - c * d * (1 - d);
            // where c is distance.
            // Let's use v1, v2 as inputs and v3 as distance? Or v2 as distance?
            // Usually Math node has 2 inputs. Smooth Min has 2 inputs + Distance.
            // Let's use v3 as distance/smoothness for SmoothMin/Max.
            double c = (v3 != 0.0) ? v3 : 0.0001;
            double h_val = std::max(0.0, std::min(1.0, (v2 - v1 + c) / (2.0 * c)));
            result = v1 * (1.0 - h_val) + v2 * h_val - c * h_val * (1.0 - h_val);
            break;
        }
        case MathOperation::SmoothMax: {
            double c = (v3 != 0.0) ? v3 : 0.0001;
            double h_val = std::max(0.0, std::min(1.0, (v1 - v2 + c) / (2.0 * c)));
            result = v1 * h_val + v2 * (1.0 - h_val) + c * h_val * (1.0 - h_val);
            break;
        }
        
        case MathOperation::Round: result = std::round(v1); break;
        case MathOperation::Ceil: result = std::ceil(v1); break;
        case MathOperation::Floor: result = std::floor(v1); break;
        case MathOperation::Fraction: result = v1 - std::floor(v1); break;
        case MathOperation::Modulo: result = (v2 != 0.0) ? std::fmod(v1, v2) : 0.0; break;
        case MathOperation::FlooredModulo: {
             if (v2 == 0.0) result = 0.0;
             else {
                 result = v1 - std::floor(v1 / v2) * v2;
             }
             break;
        }
        case MathOperation::Wrap: {
            // Wrap v1 between v2 and v3
            double min = v2;
            double max = v3;
            double range = max - min;
            if (range == 0.0) result = min;
            else result = min + (v1 - min) - range * std::floor((v1 - min) / range);
            break;
        }
        case MathOperation::Snap: {
            if (v2 == 0.0) result = v1;
            else result = std::floor(v1 / v2 + 0.5) * v2;
            break;
        }
        case MathOperation::PingPong: {
            // PingPong v1 with scale v2
            if (v2 == 0.0) result = 0.0;
            else {
                double range = v2 * 2.0;
                double val = std::fmod(v1, range);
                if (val < 0) val += range;
                result = (val > v2) ? range - val : val;
            }
            break;
        }
        
        case MathOperation::Sine: result = std::sin(v1); break;
        case MathOperation::Cosine: result = std::cos(v1); break;
        case MathOperation::Tangent: result = std::tan(v1); break;
        case MathOperation::Arcsine: result = (v1 >= -1.0 && v1 <= 1.0) ? std::asin(v1) : 0.0; break;
        case MathOperation::Arccosine: result = (v1 >= -1.0 && v1 <= 1.0) ? std::acos(v1) : 0.0; break;
        case MathOperation::Arctangent: result = std::atan(v1); break;
        case MathOperation::Arctangent2: result = std::atan2(v1, v2); break;
        
        case MathOperation::Sinh: result = std::sinh(v1); break;
        case MathOperation::Cosh: result = std::cosh(v1); break;
        case MathOperation::Tanh: result = std::tanh(v1); break;
        
        case MathOperation::ToRadians: result = qDegreesToRadians(v1); break;
        case MathOperation::ToDegrees: result = qRadiansToDegrees(v1); break;
    }
    
    if (m_useClamp) {
        result = qBound(0.0, result, 1.0);
    }
    
    return result;
}

void MathNode::setOperation(MathOperation op) {
    m_operation = op;
    setDirty(true);
}

void MathNode::setUseClamp(bool v) {
    m_useClamp = v;
    setDirty(true);
}

QJsonObject MathNode::save() const {
    QJsonObject json = Node::save();
    json["operation"] = static_cast<int>(m_operation);
    json["useClamp"] = m_useClamp;
    return json;
}

void MathNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("operation")) {
        m_operation = static_cast<MathOperation>(json["operation"].toInt());
    }
    if (json.contains("useClamp")) {
        m_useClamp = json["useClamp"].toBool();
    }
}

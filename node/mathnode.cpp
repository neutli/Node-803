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
    m_value1Input = new NodeSocket("Value A", SocketType::Float, SocketDirection::Input, this);
    m_value1Input->setDefaultValue(0.5);
    addInputSocket(m_value1Input);

    m_value2Input = new NodeSocket("Value B", SocketType::Float, SocketDirection::Input, this);
    m_value2Input->setDefaultValue(0.5);
    addInputSocket(m_value2Input);
    
    m_value3Input = new NodeSocket("Value C", SocketType::Float, SocketDirection::Input, this);
    m_value3Input->setDefaultValue(0.0);
    addInputSocket(m_value3Input); // Only used for Multiply Add

    // Output
    m_valueOutput = new NodeSocket("Result", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_valueOutput);

    // Initialize visibility
    setOperation(m_operation);
}

QVector<Node::ParameterInfo> MathNode::parameters() const {
    QVector<ParameterInfo> params;

    // Operation Enum
    ParameterInfo opInfo;
    opInfo.type = ParameterInfo::Combo;
    opInfo.name = "Operation";
    opInfo.options = { 
        "Add", "Subtract", "Multiply", "Divide", "Multiply Add",
        "Logarithm", "Sqrt", "Inverse Sqrt", "Absolute", "Exponent",
        "Minimum", "Maximum", "Less Than", "Greater Than", "Sign", "Compare",
        "Smooth Min", "Smooth Max",
        "Round", "Ceil", "Floor", "Fraction", "Modulo", "Floored Modulo", "Wrap", "Snap", "Ping Pong",
        "Sine", "Cosine", "Tangent", "Arcsine", "Arccosine", "Arctangent", "Arctangent2",
        "Sinh", "Cosh", "Tanh",
        "To Radians", "To Degrees"
    };
    opInfo.enumNames = opInfo.options; // Populate enumNames for Generic UI support
    opInfo.defaultValue = static_cast<int>(m_operation);
    opInfo.setter = [this](const QVariant& v) {
        const_cast<MathNode*>(this)->setOperation(static_cast<MathOperation>(v.toInt()));
    };
    params.append(opInfo);

    // Clamp Bool
    ParameterInfo clampInfo;
    clampInfo.type = ParameterInfo::Bool;
    clampInfo.name = "Clamp";
    clampInfo.defaultValue = m_useClamp;
    clampInfo.tooltip = "Clamp result to [0, 1]";
    clampInfo.setter = [this](const QVariant& v) {
        const_cast<MathNode*>(this)->setUseClamp(v.toBool());
    };
    params.append(clampInfo);

    // Value Inputs (Floating point sliders for unconnected inputs)
    // These correspond to the 3 input sockets.
    
    // Value A
    ParameterInfo v1Info;
    v1Info.type = ParameterInfo::Float;
    v1Info.name = "Value A"; 
    v1Info.min = -10000.0;
    v1Info.max = 10000.0;
    v1Info.step = 0.01;
    if (m_value1Input) {
        v1Info.defaultValue = m_value1Input->defaultValue();
        v1Info.setter = [this](const QVariant& v) {
            if (m_value1Input) m_value1Input->setDefaultValue(v);
        };
        params.append(v1Info);
    }

    // Value B
    ParameterInfo v2Info;
    v2Info.type = ParameterInfo::Float;
    v2Info.name = "Value B";
    v2Info.min = -10000.0;
    v2Info.max = 10000.0;
    v2Info.step = 0.01;
    if (m_value2Input && m_value2Input->isVisible()) {
        v2Info.defaultValue = m_value2Input->defaultValue();
        v2Info.setter = [this](const QVariant& v) {
            if (m_value2Input) m_value2Input->setDefaultValue(v);
        };
        params.append(v2Info);
    }
    
    // Value C
    ParameterInfo v3Info;
    v3Info.type = ParameterInfo::Float;
    v3Info.name = "Value C";
    v3Info.min = -10000.0;
    v3Info.max = 10000.0;
    v3Info.step = 0.01;
    if (m_value3Input && m_value3Input->isVisible()) {
        v3Info.defaultValue = m_value3Input->defaultValue();
        v3Info.setter = [this](const QVariant& v) {
            if (m_value3Input) m_value3Input->setDefaultValue(v);
        };
        params.append(v3Info);
    }

    return params;
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

    // Determine required inputs
    bool show2 = false;
    bool show3 = false;

    switch (op) {
        // 3 Inputs
        case MathOperation::MultiplyAdd:
        case MathOperation::SmoothMin:
        case MathOperation::SmoothMax:
        case MathOperation::Wrap:
            show2 = true;
            show3 = true;
            break;

        // 2 Inputs
        case MathOperation::Add:
        case MathOperation::Subtract:
        case MathOperation::Multiply:
        case MathOperation::Divide:
        case MathOperation::Logarithm:
        case MathOperation::Exponent:
        case MathOperation::Minimum:
        case MathOperation::Maximum:
        case MathOperation::LessThan:
        case MathOperation::GreaterThan:
        case MathOperation::Compare:
        case MathOperation::Modulo:
        case MathOperation::FlooredModulo:
        case MathOperation::Snap:
        case MathOperation::PingPong:
        case MathOperation::Arctangent2:
            show2 = true;
            show3 = false;
            break;

        // 1 Input (Unary)
        default:
            show2 = false;
            show3 = false;
            break;
    }

    if (m_value2Input) m_value2Input->setVisible(show2);
    if (m_value3Input) m_value3Input->setVisible(show3);

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
        setOperation(static_cast<MathOperation>(json["operation"].toInt()));
    }
    if (json.contains("useClamp")) {
        m_useClamp = json["useClamp"].toBool();
    }
}

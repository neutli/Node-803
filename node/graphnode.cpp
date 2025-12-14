#include "graphnode.h"
#include <cmath>

#include "graphnode.h"
#include <cmath>

GraphNode::GraphNode() : Node("Graph") {
    // Inputs
    addInputSocket(new NodeSocket("UV", SocketType::Vector, SocketDirection::Input, this)); // 0

    // Outputs
    addOutputSocket(new NodeSocket("Plot", SocketType::Float, SocketDirection::Output, this)); // 0: Visual distance field / stroke
    addOutputSocket(new NodeSocket("Y Value", SocketType::Float, SocketDirection::Output, this)); // 1: Raw function value

    // Parameters
    m_functionType = 0; // Linear
    m_coeffA = 1.0f;
    m_coeffB = 0.0f;
    m_coeffC = 1.0f;
    m_coeffD = 1.0f;
    m_thickness = 0.02f;
    m_fillBelow = false;
}

QVector<Node::ParameterInfo> GraphNode::parameters() const {
    return {
        ParameterInfo("Function", { 
            "Linear (mx+b)", 
            "Quadratic (ax^2+bx+c)", 
            "Cubic (ax^3+bx^2+cx+d)", 
            "Inverse (a/x)", 
            "Square Root (sqrt(x))", 
            "Exponential (a^x)", 
            "Logarithm (log_a(x))",
            "Sine (a*sin(bx+c)+d)", 
            "Cosine (a*cos(bx+c)+d)",
            "Tangent (a*tan(bx+c)+d)",
            "Absolute (|x|)",
            "Step/Floor",
            "Circle (Radius A)"
        }, "Linear", [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_functionType = v.toInt(); 
            node->setDirty(true); 
        }),
        
        ParameterInfo("A", -10.0f, 10.0f, 1.0f, 0.1f, "", [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffA = v.toFloat(); 
            node->setDirty(true); 
        }),
        ParameterInfo("B", -10.0f, 10.0f, 0.0f, 0.1f, "", [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffB = v.toFloat(); 
            node->setDirty(true); 
        }),
        ParameterInfo("C", -10.0f, 10.0f, 0.0f, 0.1f, "", [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffC = v.toFloat(); 
            node->setDirty(true); 
        }),
        ParameterInfo("D", -10.0f, 10.0f, 0.0f, 0.1f, "", [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffD = v.toFloat(); 
            node->setDirty(true); 
        }),
        ParameterInfo("Thickness", 0.001f, 0.5f, 0.02f, 0.001f, "Line Width", [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_thickness = v.toFloat(); 
            node->setDirty(true); 
        }),
        ParameterInfo("Fill Below", false, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_fillBelow = v.toBool(); 
            node->setDirty(true); 
        })
    };
}

void GraphNode::evaluate() {
    // No pre-computation needed for per-pixel graph
    setDirty(false);
}

QVariant GraphNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // Get UV input
    QVector3D uv = pos;
    if (m_inputSockets[0]->isConnected()) {
        uv = m_inputSockets[0]->getValue(pos).value<QVector3D>();
    }

    float x = uv.x();
    float y = uv.y();
    float fx = 0.0f;
    float dfx = 0.0f; // Derivative of f(x) for anti-aliasing estimation
    bool defined = true;

    switch (m_functionType) {
    case 0: // Linear: y = Ax + B
        fx = m_coeffA * x + m_coeffB;
        dfx = m_coeffA;
        break;
    case 1: // Quadratic: y = Ax^2 + Bx + C
        fx = m_coeffA * x * x + m_coeffB * x + m_coeffC;
        dfx = 2.0f * m_coeffA * x + m_coeffB;
        break;
    case 2: // Cubic: y = Ax^3 + Bx^2 + Cx + D
        fx = m_coeffA * x * x * x + m_coeffB * x * x + m_coeffC * x + m_coeffD;
        dfx = 3.0f * m_coeffA * x * x + 2.0f * m_coeffB * x + m_coeffC;
        break;
    case 3: // Inverse: y = A / x
        if (std::abs(x) < 0.0001f) defined = false;
        else {
            fx = m_coeffA / x;
            dfx = -m_coeffA / (x * x);
        }
        break;
    case 4: // Sqrt: y = A * sqrt(x)
        if (x < 0) defined = false;
        else {
            fx = m_coeffA * std::sqrt(x);
            dfx = (x > 0) ? (m_coeffA / (2.0f * std::sqrt(x))) : 1000.0f;
        }
        break;
    case 5: // Exp: y = A^x
        if (m_coeffA <= 0) defined = false;
        else {
            fx = std::pow(m_coeffA, x);
            dfx = fx * std::log(m_coeffA);
        }
        break;
    case 6: // Log: y = log_A(x) = ln(x) / ln(A)
        if (x <= 0 || m_coeffA <= 0 || m_coeffA == 1.0f) defined = false;
        else {
            fx = std::log(x) / std::log(m_coeffA);
            dfx = 1.0f / (x * std::log(m_coeffA));
        }
        break;
    case 7: // Sine: y = A * sin(Bx + C) + D
        fx = m_coeffA * std::sin(m_coeffB * x + m_coeffC) + m_coeffD;
        dfx = m_coeffA * m_coeffB * std::cos(m_coeffB * x + m_coeffC);
        break;
    case 8: // Cosine: y = A * cos(Bx + C) + D
        fx = m_coeffA * std::cos(m_coeffB * x + m_coeffC) + m_coeffD;
        dfx = -m_coeffA * m_coeffB * std::sin(m_coeffB * x + m_coeffC);
        break;
    case 9: // Tangent: y = A * tan(Bx + C) + D
    {
        float theta = m_coeffB * x + m_coeffC;
        fx = m_coeffA * std::tan(theta) + m_coeffD;
        float sec = 1.0f / std::cos(theta);
        dfx = m_coeffA * m_coeffB * sec * sec;
    }
        break;
    case 10: // Absolute: y = A * |x|
        fx = m_coeffA * std::abs(x);
        dfx = (x > 0) ? m_coeffA : (x < 0) ? -m_coeffA : 0.0f;
        break;
    case 11: // Floor/Step: y = floor(x)
        fx = std::floor(x);
        dfx = 0; 
        break;
    case 12: // Circle (Implicit): x^2 + y^2 = A^2
        defined = false; 
        break;
    }

    // Determine output type
    if (socket == m_outputSockets[1]) { // Y Value
        if (m_functionType == 12) {
             return x*x + y*y; // Return r^2 for Circle
        } else {
             return defined ? fx : 0.0f;
        }
    } else { // Plot (Visual)
        float intensity = 0.0f;
        
        if (m_functionType == 12) {
            // Circle special case
            float cx = x - 0.5f;
            float cy = y - 0.5f;
            float dist = std::sqrt(cx*cx + cy*cy);
            float d = std::abs(dist - m_coeffA); 
            float aaWidth = 0.01f;
            
            // smoothstep implementation
            auto smoothstep = [](float edge0, float edge1, float x) {
                x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
                return x * x * (3.0f - 2.0f * x);
            };

            intensity = 1.0f - smoothstep(m_thickness - aaWidth, m_thickness + aaWidth, d);
        }
        else if (defined) {
            float dist = std::abs(y - fx);
            float grad = std::sqrt(1.0f + dfx * dfx);
            float dEstim = dist / grad;

            float aaWidth = 0.005f;
            
            auto smoothstep = [](float edge0, float edge1, float x) {
                x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
                return x * x * (3.0f - 2.0f * x);
            };

            intensity = 1.0f - smoothstep(m_thickness - aaWidth, m_thickness + aaWidth, dEstim);
            
            if (m_fillBelow && y < fx) {
                intensity = std::max(intensity, 0.2f); 
            }
        }
        return intensity;
    }
}

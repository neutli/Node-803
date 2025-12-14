#include "graphnode.h"
#include <cmath>
#include <stack>
#include <QRegularExpression>

GraphNode::GraphNode() : Node("Graph") {
    // Inputs
    addInputSocket(new NodeSocket("UV", SocketType::Vector, SocketDirection::Input, this)); // 0
    // Inputs for A, B, C, D
    // Inputs for A, B, C, D
    addInputSocket(new NodeSocket("A", SocketType::Float, SocketDirection::Input, this)); // 1
    addInputSocket(new NodeSocket("B", SocketType::Float, SocketDirection::Input, this)); // 2
    addInputSocket(new NodeSocket("C", SocketType::Float, SocketDirection::Input, this)); // 3
    addInputSocket(new NodeSocket("D", SocketType::Float, SocketDirection::Input, this)); // 4

    // Inputs for Thickness and View Range (Matching Parameters)
    addInputSocket(new NodeSocket("Thickness", SocketType::Float, SocketDirection::Input, this)); // 5
    addInputSocket(new NodeSocket("X Min", SocketType::Float, SocketDirection::Input, this)); // 6
    addInputSocket(new NodeSocket("X Max", SocketType::Float, SocketDirection::Input, this)); // 7
    addInputSocket(new NodeSocket("Y Min", SocketType::Float, SocketDirection::Input, this)); // 8
    addInputSocket(new NodeSocket("Y Max", SocketType::Float, SocketDirection::Input, this)); // 9

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
    
    // View Range
    m_xMin = -1.0f;
    m_xMax = 1.0f;
    m_yMin = -1.0f;
    m_yMax = 1.0f;
    
    m_showAxes = true;
    
    m_equationStr = "sin(x)";
    compileEquation();
}

QVector<Node::ParameterInfo> GraphNode::parameters() const {
    using PI = Node::ParameterInfo;
    return {
        // Coefficients (Match Input Sockets 1-4)
        PI(PI::Float, "A", m_coeffA, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffA = v.toFloat(); 
            node->setDirty(true); 
        }).range(-10.0, 10.0),

        PI(PI::Float, "B", m_coeffB, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffB = v.toFloat(); 
            node->setDirty(true); 
        }).range(-10.0, 10.0),

        PI(PI::Float, "C", m_coeffC, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffC = v.toFloat(); 
            node->setDirty(true); 
        }).range(-10.0, 10.0),

        PI(PI::Float, "D", m_coeffD, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_coeffD = v.toFloat(); 
            node->setDirty(true); 
        }).range(-10.0, 10.0),

        // Thickness (Match Socket 5)
        PI(PI::Float, "Thickness", m_thickness, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_thickness = v.toFloat(); 
            node->setDirty(true); 
        }, "Curve Width").range(0.001, 0.5),
        
        // View Range Parameters (Match Sockets 6-9)
        PI(PI::Float, "X Min", m_xMin, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_xMin = v.toFloat(); 
            node->setDirty(true); 
        }).range(-100.0, 100.0),
        
        PI(PI::Float, "X Max", m_xMax, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_xMax = v.toFloat(); 
            node->setDirty(true); 
        }).range(-100.0, 100.0),

        PI(PI::Float, "Y Min", m_yMin, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_yMin = v.toFloat(); 
            node->setDirty(true); 
        }).range(-100.0, 100.0),

        PI(PI::Float, "Y Max", m_yMax, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_yMax = v.toFloat(); 
            node->setDirty(true); 
        }).range(-100.0, 100.0),

        // Function Selection
        PI(PI::Combo, "Function", m_functionType, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_functionType = v.toInt(); 
            node->setDirty(true); 
        }, "").withOptions({ 
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
            "Circle (Radius A)",
            "Custom (Equation)"
        }),

        // Equation Input
        PI(PI::String, "Equation", m_equationStr, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_equationStr = v.toString();
            if (node->m_functionType != 13) node->m_functionType = 13;
            node->compileEquation(); 
            node->setDirty(true); 
        }, "e.g. sin(x) * x"),

        // Options
        PI(PI::Bool, "Fill Below", m_fillBelow, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_fillBelow = v.toBool(); 
            node->setDirty(true); 
        }),
        
        PI(PI::Bool, "Show Axes", m_showAxes, [this](const QVariant& v) { 
            auto* node = const_cast<GraphNode*>(this);
            node->m_showAxes = v.toBool(); 
            node->setDirty(true); 
        })
    };
}

void GraphNode::evaluate() {
    setDirty(false);
}

// Simple expression parser
void GraphNode::compileEquation() {
    m_rpn.clear();
    QString eq = m_equationStr.toLower().remove(" ").remove("y="); // Normalize
    
    // Shunting-yard algorithm
    QVector<QString> outputQueue;
    std::stack<QString> operatorStack;
    
    auto precedence = [](const QString& op) {
        if (op == "+" || op == "-") return 1;
        if (op == "*" || op == "/") return 2;
        if (op == "^") return 3;
        return 0;
    };
    
    auto isFunc = [](const QString& s) {
        return s=="sin"||s=="cos"||s=="tan"||s=="abs"||s=="sqrt"||s=="log"||s=="exp";
    };

    int i = 0;
    while (i < eq.length()) {
        QChar c = eq[i];
        
        if (c.isDigit() || c == '.') {
            int start = i;
            while (i < eq.length() && (eq[i].isDigit() || eq[i] == '.')) i++;
            outputQueue.append(eq.mid(start, i - start));
            continue;
        }
        
        if (c.isLetter()) {
            int start = i;
            while (i < eq.length() && eq[i].isLetter()) i++;
            QString ident = eq.mid(start, i - start);
            if (isFunc(ident)) {
                operatorStack.push(ident);
            } else { // Variable (x, or constants e, pi)
                outputQueue.append(ident);
            }
            continue;
        }
        
        if (QString("+-*/^").contains(c)) {
            QString op = QString(c);
            while (!operatorStack.empty()) {
                QString top = operatorStack.top();
                if (top == "(") break;
                if (precedence(top) >= precedence(op)) {
                    outputQueue.append(operatorStack.top());
                    operatorStack.pop();
                } else break;
            }
            operatorStack.push(op);
            i++;
            continue;
        }
        
        if (c == '(') {
            operatorStack.push("(");
            i++;
            continue;
        }
        
        if (c == ')') {
            while (!operatorStack.empty() && operatorStack.top() != "(") {
                outputQueue.append(operatorStack.top());
                operatorStack.pop();
            }
            if (!operatorStack.empty()) operatorStack.pop(); // Pop '('
            if (!operatorStack.empty() && isFunc(operatorStack.top())) {
                outputQueue.append(operatorStack.top());
                operatorStack.pop();
            }
            i++;
            continue;
        }
        
        i++; // Skip unknowns
    }
    
    while (!operatorStack.empty()) {
        outputQueue.append(operatorStack.top());
        operatorStack.pop();
    }
    
    // Convert to Tokens
    for (const QString& s : outputQueue) {
        if (s[0].isDigit() || s[0] == '.') {
            m_rpn.append({Number, s.toDouble(), ""});
        } else if (isFunc(s)) {
            m_rpn.append({Func, 0, s});
        } else if (QString("+-*/^").contains(s)) {
            m_rpn.append({Op, 0, s});
        } else {
             // Variable or constant
             if (s == "pi") m_rpn.append({Number, 3.14159265, ""});
             else if (s == "e") m_rpn.append({Number, 2.71828, ""});
             else m_rpn.append({Variable, 0, s}); // treat as x
        }
    }
}

QVariant GraphNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // Get UV input
    QVector3D uv = pos;
    if (m_inputSockets[0]->isConnected()) {
        uv = m_inputSockets[0]->getValue(pos).value<QVector3D>();
    }

    // Resolve View Range (with Input Sockets 6-9 overriding)
    float xMin = m_xMin;
    if (m_inputSockets[6]->isConnected()) xMin = m_inputSockets[6]->getValue(pos).toFloat();

    float xMax = m_xMax;
    if (m_inputSockets[7]->isConnected()) xMax = m_inputSockets[7]->getValue(pos).toFloat();

    float yMin = m_yMin;
    if (m_inputSockets[8]->isConnected()) yMin = m_inputSockets[8]->getValue(pos).toFloat();

    float yMax = m_yMax;
    if (m_inputSockets[9]->isConnected()) yMax = m_inputSockets[9]->getValue(pos).toFloat();

    // Map UV (0-1) to View Range
    float u = uv.x();
    float v = uv.y();
    
    float x = xMin + u * (xMax - xMin);
    float y = yMin + v * (yMax - yMin);
    
    // Resolve Coefficients (1-4)
    float A = m_coeffA;
    if (m_inputSockets[1]->isConnected()) A = m_inputSockets[1]->getValue(pos).toFloat();
    
    float B = m_coeffB;
    if (m_inputSockets[2]->isConnected()) B = m_inputSockets[2]->getValue(pos).toFloat();
    
    float C = m_coeffC;
    if (m_inputSockets[3]->isConnected()) C = m_inputSockets[3]->getValue(pos).toFloat();

    float D = m_coeffD;
    if (m_inputSockets[4]->isConnected()) D = m_inputSockets[4]->getValue(pos).toFloat();

    // Resolve Thickness (5)
    float thickness = m_thickness;
    if (m_inputSockets[5]->isConnected()) thickness = m_inputSockets[5]->getValue(pos).toFloat();
    // Clamp thickness to sane values if driven by input
    thickness = std::max(0.001f, std::abs(thickness));

    float fx = 0.0f;
    float dfx = 0.0f; 
    bool defined = true;

    switch (m_functionType) {
    case 0: // Linear: y = Ax + B
        fx = A * x + B;
        dfx = A;
        break;
    case 1: // Quadratic: y = Ax^2 + Bx + C
        fx = A * x * x + B * x + C;
        dfx = 2.0f * A * x + B;
        break;
    case 2: // Cubic: y = Ax^3 + Bx^2 + Cx + D
        fx = A * x * x * x + B * x * x + C * x + D;
        dfx = 3.0f * A * x * x + 2.0f * B * x + C;
        break;
    case 3: // Inverse: y = A / x
        if (std::abs(x) < 0.0001f) defined = false;
        else {
            fx = A / x;
            dfx = -A / (x * x);
        }
        break;
    case 4: // Sqrt: y = A * sqrt(x)
        if (x < 0) defined = false;
        else {
            fx = A * std::sqrt(x);
            dfx = (x > 0) ? (A / (2.0f * std::sqrt(x))) : 1000.0f;
        }
        break;
    case 5: // Exp: y = A^x
        if (A <= 0.0001f) defined = false;
        else {
            fx = std::pow(A, x);
            dfx = fx * std::log(A);
        }
        break;
    case 6: // Log: y = log_A(x) = ln(x) / ln(A)
        if (x <= 0 || A <= 0 || std::abs(A - 1.0f) < 0.001f) defined = false;
        else {
            fx = std::log(x) / std::log(A);
            dfx = 1.0f / (x * std::log(A));
        }
        break;
    case 7: // Sine: y = A * sin(Bx + C) + D
        fx = A * std::sin(B * x + C) + D;
        dfx = A * B * std::cos(B * x + C);
        break;
    case 8: // Cosine: y = A * cos(Bx + C) + D
        fx = A * std::cos(B * x + C) + D;
        dfx = -A * B * std::sin(B * x + C);
        break;
    case 9: // Tangent: y = A * tan(Bx + C) + D
    {
        float theta = B * x + C;
        fx = A * std::tan(theta) + D;
        float sec = 1.0f / std::cos(theta);
        dfx = A * B * sec * sec;
    }
        break;
    case 10: // Absolute: y = A * |x|
        fx = A * std::abs(x);
        dfx = (x > 0) ? A : (x < 0) ? -A : 0.0f;
        break;
    case 11: // Floor/Step: y = floor(x)
        fx = std::floor(x);
        dfx = 0; 
        break;
    case 12: // Circle (Implicit): x^2 + y^2 = A^2
        defined = false; 
        break;
    case 13: // Custom Equation
    {
        // Evaluate RPN
        std::stack<double> stack;
        for (const auto& tok : m_rpn) {
            if (tok.type == Number) stack.push(tok.val);
            else if (tok.type == Variable) stack.push(x);
            else if (tok.type == Op) {
                if (stack.size() < 2) continue; // Error
                double b = stack.top(); stack.pop();
                double a = stack.top(); stack.pop();
                if (tok.str == "+") stack.push(a + b);
                else if (tok.str == "-") stack.push(a - b);
                else if (tok.str == "*") stack.push(a * b);
                else if (tok.str == "/") stack.push(b != 0 ? a / b : 0);
                else if (tok.str == "^") stack.push(std::pow(a, b));
            } else if (tok.type == Func) {
                 if (stack.empty()) continue;
                 double a = stack.top(); stack.pop();
                 if (tok.str == "sin") stack.push(std::sin(a));
                 else if (tok.str == "cos") stack.push(std::cos(a));
                 else if (tok.str == "tan") stack.push(std::tan(a));
                 else if (tok.str == "abs") stack.push(std::abs(a));
                 else if (tok.str == "sqrt") stack.push(a>=0 ? std::sqrt(a) : 0);
                 else if (tok.str == "log") stack.push(a>0 ? std::log(a) : -100);
                 else if (tok.str == "exp") stack.push(std::exp(a));
            }
        }
        if (!stack.empty()) fx = stack.top();
        else fx = 0.0f;
        
        // Approx derivative for AA
        float h = 0.001f;
        float fa = fx;
        std::stack<double> stack2;
        float x2 = x + h; 
        for (const auto& tok : m_rpn) {
            if (tok.type == Number) stack2.push(tok.val);
            else if (tok.type == Variable) stack2.push(x2);
            else if (tok.type == Op) {
                 if (stack2.size() < 2) continue;
                 double b = stack2.top(); stack2.pop();
                 double a = stack2.top(); stack2.pop();
                 if (tok.str == "+") stack2.push(a + b);
                 else if (tok.str == "-") stack2.push(a - b);
                 else if (tok.str == "*") stack2.push(a * b);
                 else if (tok.str == "/") stack2.push(b != 0 ? a / b : 0);
                 else if (tok.str == "^") stack2.push(std::pow(a, b));
            } else if (tok.type == Func) {
                 if (stack2.empty()) continue;
                 double a = stack2.top(); stack2.pop();
                 if (tok.str == "sin") stack2.push(std::sin(a));
                 else if (tok.str == "cos") stack2.push(std::cos(a));
                 else if (tok.str == "tan") stack2.push(std::tan(a));
                 else if (tok.str == "abs") stack2.push(std::abs(a));
                 else if (tok.str == "sqrt") stack2.push(a>=0 ? std::sqrt(a) : 0);
                 else if (tok.str == "log") stack2.push(a>0 ? std::log(a) : -100);
                 else if (tok.str == "exp") stack2.push(std::exp(a));
            }
        }
        float fxh = (!stack2.empty()) ? stack2.top() : 0.0f;
        dfx = (fxh - fa) / h;
    }
        break;
    }

    // Output Y Value (Raw)
    if (socket == m_outputSockets[1]) { 
        if (m_functionType == 12) {
             return x*x + y*y; // Return r^2 for Circle
        } else {
             return defined ? fx : 0.0f;
        }
    } else { // Plot (Visual)
        float intensity = 0.0f;
        
        // Simplified Thickness Logic: use resolved thickness directly
        float visualThickness = thickness; 

        if (m_functionType == 12) {
            // Circle special case
            float dist = std::sqrt(x*x + y*y); // Centered at origin (0,0) in graph space
            float d = std::abs(dist - A); 
            float aaWidth = 0.01f; // Fixed AA width
            
            auto smoothstep = [](float edge0, float edge1, float x) {
                x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
                return x * x * (3.0f - 2.0f * x);
            };

            intensity = 1.0f - smoothstep(visualThickness - aaWidth, visualThickness + aaWidth, d);
            
            // Fill Below (Inside Circle)
            if (m_fillBelow && dist < A) {
                 intensity = std::max(intensity, 0.5f); 
            }
        }
        else if (defined) {
            float dist = std::abs(y - fx);
            // Gradient correction for steep slopes
            float grad = std::sqrt(1.0f + dfx * dfx);
            float dEstim = dist / grad; 
            
            float aaWidth = 0.005f; // Fixed AA width
            
            auto smoothstep = [](float edge0, float edge1, float x) {
                x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
                return x * x * (3.0f - 2.0f * x);
            };

            intensity = 1.0f - smoothstep(visualThickness - aaWidth, visualThickness + aaWidth, dEstim);
            
            if (m_fillBelow && y < fx) {
                intensity = std::max(intensity, 1.0f); 
            }
        }
        
        // Draw Axes Logic - Simplified & Robust
        if (m_showAxes) {
            float axisThickness = visualThickness * 0.5f; // Axes are half as thick as the curve
            float aaWidth = 0.005f;
            
            auto smoothstep = [](float edge0, float edge1, float x) {
                x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
                return x * x * (3.0f - 2.0f * x);
            };

            // X Axis (y=0) -> Distance is |y|
            float dX = std::abs(y);
            float iX = 1.0f - smoothstep(axisThickness - aaWidth, axisThickness + aaWidth, dX);
            
            // Y Axis (x=0) -> Distance is |x|
            float dY = std::abs(x);
             // x range logic for visual consistency
            // Use resolved xMin/xMax
            float xRange = xMax - xMin;
            float axisThicknessX = thickness * std::abs(xRange) * 0.5f; 
            if (axisThicknessX < 0.001) axisThicknessX = axisThickness; // Fallback

            float iY = 1.0f - smoothstep(axisThickness - aaWidth, axisThickness + aaWidth, dY);

            // Combine
            float axes = std::max(iX, iY);
            
            // Axes always full brightness if enabled
            intensity = std::max(intensity, axes); 
        }

        return intensity;
    }
}

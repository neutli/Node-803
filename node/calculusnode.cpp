#include "calculusnode.h"
#include "appsettings.h"
#include <QDebug>
#include <cmath>
#include <QColor>

CalculusNode::CalculusNode() 
    : Node("Calculus")
    , m_mode(Mode::Gradient)
{
    // 入力ソケット
    m_valueInput = new NodeSocket("値", SocketType::Float, SocketDirection::Input, this);
    m_valueInput->setDefaultValue(0.0);
    
    m_vectorInput = new NodeSocket("ベクトル", SocketType::Vector, SocketDirection::Input, this);
    
    m_sampleDistInput = new NodeSocket("サンプル距離", SocketType::Float, SocketDirection::Input, this);
    m_sampleDistInput->setDefaultValue(1.0);  // 1ピクセル
    
    m_scaleInput = new NodeSocket("スケール", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(1.0);

    addInputSocket(m_valueInput);
    addInputSocket(m_vectorInput);
    addInputSocket(m_sampleDistInput);
    addInputSocket(m_scaleInput);

    // 出力ソケット
    m_facOutput = new NodeSocket("係数", SocketType::Float, SocketDirection::Output, this);
    m_colorOutput = new NodeSocket("カラー", SocketType::Color, SocketDirection::Output, this);

    addOutputSocket(m_facOutput);
    addOutputSocket(m_colorOutput);
}

CalculusNode::~CalculusNode() {}

QVector<Node::ParameterInfo> CalculusNode::parameters() const {
    return {
        ParameterInfo("Operation Mode", 
                      {"Derivative X", "Derivative Y", "Gradient", "Laplacian", "Integral X", "Integral Y"}, 
                      QVariant::fromValue(static_cast<int>(m_mode)), 
                      [this](const QVariant& v) {
                          const_cast<CalculusNode*>(this)->setMode(static_cast<Mode>(v.toInt()));
                      },
                      "Select the calculus operation"),
        ParameterInfo("サンプル距離", 0.1, 10.0, 1.0, 0.1, 
            "微分計算時のサンプリング間隔（ピクセル）\n"
            "小さいほど精密、大きいほど滑らか"),
        ParameterInfo("スケール", 0.0, 100.0, 1.0, 0.1, 
            "出力値の倍率\n"
            "微分結果は小さいことが多いので拡大して可視化"),
    };
}

QJsonObject CalculusNode::save() const {
    QJsonObject root = Node::save();
    root["mode"] = static_cast<int>(m_mode);
    return root;
}

void CalculusNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("mode")) {
        m_mode = static_cast<Mode>(json["mode"].toInt());
        notifyStructureChanged(); // Update UI
    }
}

void CalculusNode::evaluate() {
    // Stateless
}

void CalculusNode::setDirty(bool dirty) {
    Node::setDirty(dirty);
}

CalculusNode::Mode CalculusNode::mode() const { return m_mode; }
double CalculusNode::sampleDistance() const { return m_sampleDistInput->value().toDouble(); }
double CalculusNode::scale() const { return m_scaleInput->value().toDouble(); }

void CalculusNode::setMode(Mode m) { 
    if (m_mode != m) {
        m_mode = m; 
        setDirty(true);
        notifyStructureChanged();
    }
}

void CalculusNode::setSampleDistance(double d) { 
    m_sampleDistInput->setValue(d); 
    setDirty(true); 
}

void CalculusNode::setScale(double s) { 
    m_scaleInput->setValue(s); 
    setDirty(true); 
}

// 指定位置での入力値をサンプリング
double CalculusNode::sampleValue(const QVector3D& pos) {
    if (!m_valueInput->isConnected()) {
        return 0.0;
    }
    
    QVariant val = m_valueInput->getValue(pos);
    
    if (val.canConvert<double>()) {
        return val.toDouble();
    } else if (val.canConvert<QColor>()) {
        QColor c = val.value<QColor>();
        // グレースケール変換（輝度）
        return 0.299 * c.redF() + 0.587 * c.greenF() + 0.114 * c.blueF();
    }
    
    return 0.0;
}

// X方向の微分（∂f/∂x）
// 中心差分法: (f(x+h) - f(x-h)) / (2h)
double CalculusNode::computeDerivativeX(const QVector3D& pos, double h) {
    QVector3D posPlus = pos + QVector3D(h, 0, 0);
    QVector3D posMinus = pos - QVector3D(h, 0, 0);
    
    double fPlus = sampleValue(posPlus);
    double fMinus = sampleValue(posMinus);
    
    return (fPlus - fMinus) / (2.0 * h);
}

// Y方向の微分（∂f/∂y）
double CalculusNode::computeDerivativeY(const QVector3D& pos, double h) {
    QVector3D posPlus = pos + QVector3D(0, h, 0);
    QVector3D posMinus = pos - QVector3D(0, h, 0);
    
    double fPlus = sampleValue(posPlus);
    double fMinus = sampleValue(posMinus);
    
    return (fPlus - fMinus) / (2.0 * h);
}

// 勾配の大きさ（|∇f| = √((∂f/∂x)² + (∂f/∂y)²)）
double CalculusNode::computeGradient(const QVector3D& pos, double h) {
    double dx = computeDerivativeX(pos, h);
    double dy = computeDerivativeY(pos, h);
    
    return std::sqrt(dx * dx + dy * dy);
}

// ラプラシアン（∇²f = ∂²f/∂x² + ∂²f/∂y²）
// 5点ステンシル: (f(x+h,y) + f(x-h,y) + f(x,y+h) + f(x,y-h) - 4*f(x,y)) / h²
double CalculusNode::computeLaplacian(const QVector3D& pos, double h) {
    double fCenter = sampleValue(pos);
    double fRight = sampleValue(pos + QVector3D(h, 0, 0));
    double fLeft = sampleValue(pos - QVector3D(h, 0, 0));
    double fUp = sampleValue(pos + QVector3D(0, h, 0));
    double fDown = sampleValue(pos - QVector3D(0, h, 0));
    
    return (fRight + fLeft + fUp + fDown - 4.0 * fCenter) / (h * h);
}

QVariant CalculusNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D p;
    if (m_vectorInput->isConnected()) {
        p = m_vectorInput->getValue(pos).value<QVector3D>();
        // Convert normalized UV to pixel coordinates
        int w = AppSettings::instance().renderWidth();
        int h = AppSettings::instance().renderHeight();
        p = QVector3D(p.x() * w, p.y() * h, p.z());
    } else {
        p = pos;
    }
    
    double h = m_sampleDistInput->getValue(pos).toDouble();
    if (h < 0.1) h = 0.1;  // 最小値
    
    double scaleVal = m_scaleInput->getValue(pos).toDouble();
    
    double result = 0.0;
    
    switch (m_mode) {
        case Mode::DerivativeX:
            result = computeDerivativeX(p, h);
            break;
            
        case Mode::DerivativeY:
            result = computeDerivativeY(p, h);
            break;
            
        case Mode::Gradient:
            result = computeGradient(p, h);
            break;
            
        case Mode::Laplacian:
            result = computeLaplacian(p, h);
            // ラプラシアンは正負あるので、0.5にオフセット
            result = result * 0.5 + 0.5;
            break;
            
        case Mode::IntegralX:
        case Mode::IntegralY:
            // 簡易積分（現在位置までの累積）
            // 注意: これは近似であり、厳密な積分ではない
            {
                double sum = 0.0;
                int steps = static_cast<int>(m_mode == Mode::IntegralX ? p.x() : p.y());
                steps = qMin(steps, 100);  // 計算量制限
                
                for (int i = 0; i <= steps; ++i) {
                    QVector3D samplePos = (m_mode == Mode::IntegralX) 
                        ? QVector3D(i, p.y(), p.z())
                        : QVector3D(p.x(), i, p.z());
                    sum += sampleValue(samplePos) * h;
                }
                
                result = sum / (steps + 1);  // 正規化
            }
            break;
    }
    
    result *= scaleVal;
    
    // 出力
    if (socket == m_facOutput) {
        // 微分値は負になることがあるので、適切に処理
        return result;
    } else if (socket == m_colorOutput) {
        // カラー出力（グレースケール、0-1にクランプ）
        double clamped = qBound(0.0, result, 1.0);
        int gray = static_cast<int>(clamped * 255);
        return QColor(gray, gray, gray);
    }
    
    return QVariant();
}

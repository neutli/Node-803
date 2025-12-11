#include "mixnode.h"
#include <algorithm>
#include <cmath>
#include <QColor>

MixNode::MixNode() : Node("Mix") {
    m_dataType = DataType::Float;
    m_colorBlendMode = ColorBlendMode::Mix;
    m_vectorMixMode = VectorMixMode::Uniform;
    m_clampResult = false;
    m_language = Language::English;

    // Inputs
    m_factorInput = new NodeSocket("Factor", SocketType::Float, SocketDirection::Input, this);
    m_factorInput->setDefaultValue(0.5);

    m_inputA = new NodeSocket("A", SocketType::Float, SocketDirection::Input, this);
    m_inputA->setDefaultValue(0.0);

    m_inputB = new NodeSocket("B", SocketType::Float, SocketDirection::Input, this);
    m_inputB->setDefaultValue(0.0);

    addInputSocket(m_factorInput);
    addInputSocket(m_inputA);
    addInputSocket(m_inputB);

    // Output
    m_output = new NodeSocket("Result", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_output);
}

void MixNode::setDataType(DataType type) {
    if (m_dataType == type) return;
    m_dataType = type;

    // Update socket types
    SocketType socketType = SocketType::Float;
    if (type == DataType::Vector) socketType = SocketType::Vector;
    else if (type == DataType::Color) socketType = SocketType::Color;

    // Reuse sockets to preserve connections and avoid dangling pointers
    m_inputA->setType(socketType);
    m_inputB->setType(socketType);
    m_output->setType(socketType);
    
    // Set defaults based on type
    if (type == DataType::Color) {
        m_inputA->setDefaultValue(QColor(128, 128, 128));
        m_inputB->setDefaultValue(QColor(128, 128, 128));
    } else if (type == DataType::Vector) {
        m_inputA->setDefaultValue(QVector3D(0, 0, 0));
        m_inputB->setDefaultValue(QVector3D(0, 0, 0));
    } else {
        m_inputA->setDefaultValue(0.0);
        m_inputB->setDefaultValue(0.0);
    }

    // Update Factor socket for Vector Non-Uniform mode
    if (type == DataType::Vector && m_vectorMixMode == VectorMixMode::NonUniform) {
        // Factor should be Vector
        if (m_factorInput->type() != SocketType::Vector) {
             m_factorInput->setType(SocketType::Vector);
             m_factorInput->setDefaultValue(QVector3D(0.5, 0.5, 0.5));
        }
    } else {
        // Factor should be Float for other modes
        if (m_factorInput->type() != SocketType::Float) {
             m_factorInput->setType(SocketType::Float);
             m_factorInput->setDefaultValue(0.5);
        }
    }

    setDirty(true);
    notifyStructureChanged();
}

void MixNode::setColorBlendMode(ColorBlendMode mode) {
    if (m_colorBlendMode == mode) return;
    m_colorBlendMode = mode;
    setDirty(true);
}

void MixNode::setVectorMixMode(VectorMixMode mode) {
    if (m_vectorMixMode == mode) return;
    m_vectorMixMode = mode;
    
    if (m_dataType == DataType::Vector) {
        // If switching between Uniform (Float factor) and Non-Uniform (Vector factor)
        // We need to update Factor socket type.
        
        SocketType targetFactorType = (mode == VectorMixMode::Uniform) ? SocketType::Float : SocketType::Vector;
        
        if (m_factorInput->type() != targetFactorType) {
            m_factorInput->setType(targetFactorType);
            if (targetFactorType == SocketType::Float) m_factorInput->setDefaultValue(0.5);
            else m_factorInput->setDefaultValue(QVector3D(0.5, 0.5, 0.5));
            
            notifyStructureChanged();
        }
    }
    setDirty(true);
}

void MixNode::setClampResult(bool clamp) {
    m_clampResult = clamp;
    setDirty(true);
}

void MixNode::setLanguage(Language lang) {
    m_language = lang;
    // UI needs to update labels, but logic doesn't change.
    // We notify structure change to force UI rebuild/update.
    notifyStructureChanged(); 
}

double MixNode::factor() const {
    if (m_factorInput->type() == SocketType::Float)
        return m_factorInput->value().toDouble();
    return 0.5; // Fallback
}

void MixNode::setFactor(double v) {
    if (m_factorInput->type() == SocketType::Float) {
        m_factorInput->setValue(v);
        setDirty(true);
    }
}

void MixNode::evaluate() {
    // Evaluation is done in compute for texture nodes usually, 
    // but for simple value nodes we can do it here.
    // However, since this is likely used in a texture context, compute() is key.
    setDirty(false);
}

QVariant MixNode::compute(const QVector3D& pos, NodeSocket* socket) {
    if (socket != m_output) return QVariant();

    // 1. Get Factor
    QVariant factorVal = m_factorInput->getValue(pos);
    
    // 2. Get Inputs
    QVariant valA = m_inputA->getValue(pos);
    QVariant valB = m_inputB->getValue(pos);

    if (m_dataType == DataType::Float) {
        double f = factorVal.toDouble();
        double a = valA.toDouble();
        double b = valB.toDouble();
        double res = a * (1.0 - f) + b * f;
        if (m_clampResult) res = std::clamp(res, 0.0, 1.0);
        return res;
    }
    else if (m_dataType == DataType::Vector) {
        QVector3D a = valA.value<QVector3D>();
        QVector3D b = valB.value<QVector3D>();
        QVector3D res;
        
        if (m_vectorMixMode == VectorMixMode::Uniform) {
            double f = factorVal.toDouble();
            res = a * (1.0 - f) + b * f;
        } else {
            QVector3D f = factorVal.value<QVector3D>();
            res.setX(a.x() * (1.0 - f.x()) + b.x() * f.x());
            res.setY(a.y() * (1.0 - f.y()) + b.y() * f.y());
            res.setZ(a.z() * (1.0 - f.z()) + b.z() * f.z());
        }
        return res;
    }
    else if (m_dataType == DataType::Color) {
        double f = factorVal.toDouble();
        QColor ca = valA.value<QColor>();
        QColor cb = valB.value<QColor>();
        
        QColor res = blendColor(ca, cb, f);
        return res;
    }

    return QVariant();
}

float MixNode::blendFloat(float a, float b, float t) const {
    return a * (1.0f - t) + b * t;
}

QColor MixNode::blendColor(const QColor& c1, const QColor& c2, double t) const {
    // Convert to float 0-1 for precision
    float r1 = c1.redF(); float g1 = c1.greenF(); float b1 = c1.blueF();
    float r2 = c2.redF(); float g2 = c2.greenF(); float b2 = c2.blueF();
    
    float r = r1, g = g1, b = b1;

    // Helper lambda for mixing
    auto mix = [](float base, float blend) { return blend; };
    
    switch (m_colorBlendMode) {
        case ColorBlendMode::Mix:
            r = r2; g = g2; b = b2;
            break;
        case ColorBlendMode::Darken:
            r = std::min(r1, r2); g = std::min(g1, g2); b = std::min(b1, b2);
            break;
        case ColorBlendMode::Multiply:
            r = r1 * r2; g = g1 * g2; b = b1 * b2;
            break;
        case ColorBlendMode::ColorBurn:
            r = (r2 == 0.0f) ? 0.0f : std::max(0.0f, 1.0f - (1.0f - r1) / r2);
            g = (g2 == 0.0f) ? 0.0f : std::max(0.0f, 1.0f - (1.0f - g1) / g2);
            b = (b2 == 0.0f) ? 0.0f : std::max(0.0f, 1.0f - (1.0f - b1) / b2);
            break;
        case ColorBlendMode::Lighten:
            r = std::max(r1, r2); g = std::max(g1, g2); b = std::max(b1, b2);
            break;
        case ColorBlendMode::Screen:
            r = 1.0f - (1.0f - r1) * (1.0f - r2);
            g = 1.0f - (1.0f - g1) * (1.0f - g2);
            b = 1.0f - (1.0f - b1) * (1.0f - b2);
            break;
        case ColorBlendMode::ColorDodge:
            r = (r2 == 1.0f) ? 1.0f : std::min(1.0f, r1 / (1.0f - r2));
            g = (g2 == 1.0f) ? 1.0f : std::min(1.0f, g1 / (1.0f - g2));
            b = (b2 == 1.0f) ? 1.0f : std::min(1.0f, b1 / (1.0f - b2));
            break;
        case ColorBlendMode::Overlay:
            r = (r1 < 0.5f) ? (2.0f * r1 * r2) : (1.0f - 2.0f * (1.0f - r1) * (1.0f - r2));
            g = (g1 < 0.5f) ? (2.0f * g1 * g2) : (1.0f - 2.0f * (1.0f - g1) * (1.0f - g2));
            b = (b1 < 0.5f) ? (2.0f * b1 * b2) : (1.0f - 2.0f * (1.0f - b1) * (1.0f - b2));
            break;
        case ColorBlendMode::Add:
            r = r1 + r2; g = g1 + g2; b = b1 + b2;
            break;
        case ColorBlendMode::SoftLight:
            // Simplified Soft Light
            r = (1.0f - 2.0f * r2) * r1 * r1 + 2.0f * r2 * r1;
            g = (1.0f - 2.0f * g2) * g1 * g1 + 2.0f * g2 * g1;
            b = (1.0f - 2.0f * b2) * b1 * b1 + 2.0f * b2 * b1;
            break;
        case ColorBlendMode::LinearLight:
             r = r1 + 2.0f * r2 - 1.0f;
             g = g1 + 2.0f * g2 - 1.0f;
             b = b1 + 2.0f * b2 - 1.0f;
             break;
        case ColorBlendMode::Difference:
            r = std::abs(r1 - r2); g = std::abs(g1 - g2); b = std::abs(b1 - b2);
            break;
        case ColorBlendMode::Exclusion:
            r = r1 + r2 - 2.0f * r1 * r2;
            g = g1 + g2 - 2.0f * g1 * g2;
            b = b1 + b2 - 2.0f * b1 * b2;
            break;
        case ColorBlendMode::Subtract:
            r = r1 - r2; g = g1 - g2; b = b1 - b2;
            break;
        case ColorBlendMode::Divide:
            r = (r2 == 0.0f) ? 0.0f : r1 / r2;
            g = (g2 == 0.0f) ? 0.0f : g1 / g2;
            b = (b2 == 0.0f) ? 0.0f : b1 / b2;
            break;
        case ColorBlendMode::Hue:
            // TODO: Implement Hue blend
            break;
        case ColorBlendMode::Saturation:
            // TODO: Implement Saturation blend
            break;
        case ColorBlendMode::Color:
            // TODO: Implement Color blend
            break;
        case ColorBlendMode::Value:
            // TODO: Implement Value blend
            break;
    }

    // Interpolate between original (c1) and blended result based on factor t
    float finalR = r1 * (1.0f - t) + r * t;
    float finalG = g1 * (1.0f - t) + g * t;
    float finalB = b1 * (1.0f - t) + b * t;

    if (m_clampResult) {
        finalR = std::clamp(finalR, 0.0f, 1.0f);
        finalG = std::clamp(finalG, 0.0f, 1.0f);
        finalB = std::clamp(finalB, 0.0f, 1.0f);
    }

    return QColor::fromRgbF(finalR, finalG, finalB);
}

QJsonObject MixNode::save() const {
    QJsonObject json = Node::save();
    json["dataType"] = static_cast<int>(m_dataType);
    json["colorBlendMode"] = static_cast<int>(m_colorBlendMode);
    json["vectorMixMode"] = static_cast<int>(m_vectorMixMode);
    json["clampResult"] = m_clampResult;
    json["language"] = static_cast<int>(m_language);
    return json;
}

void MixNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("dataType")) setDataType(static_cast<DataType>(json["dataType"].toInt()));
    if (json.contains("colorBlendMode")) setColorBlendMode(static_cast<ColorBlendMode>(json["colorBlendMode"].toInt()));
    if (json.contains("vectorMixMode")) setVectorMixMode(static_cast<VectorMixMode>(json["vectorMixMode"].toInt()));
    if (json.contains("clampResult")) setClampResult(json["clampResult"].toBool());
    if (json.contains("language")) setLanguage(static_cast<Language>(json["language"].toInt()));
}

QString MixNode::getBlendModeString(ColorBlendMode mode, Language lang) {
    switch (lang) {
        case Language::Japanese:
            switch (mode) {
                case ColorBlendMode::Mix: return "ミックス";
                case ColorBlendMode::Darken: return "暗い方";
                case ColorBlendMode::Multiply: return "乗算";
                case ColorBlendMode::ColorBurn: return "焼き込みカラー";
                case ColorBlendMode::Lighten: return "明るい方";
                case ColorBlendMode::Screen: return "スクリーン";
                case ColorBlendMode::ColorDodge: return "覆い焼きカラー";
                case ColorBlendMode::Overlay: return "オーバーレイ";
                case ColorBlendMode::Add: return "加算";
                case ColorBlendMode::SoftLight: return "ソフトライト";
                case ColorBlendMode::LinearLight: return "リニアライト";
                case ColorBlendMode::Difference: return "差分";
                case ColorBlendMode::Exclusion: return "除外";
                case ColorBlendMode::Subtract: return "減算";
                case ColorBlendMode::Divide: return "除算";
                case ColorBlendMode::Hue: return "色相";
                case ColorBlendMode::Saturation: return "彩度";
                case ColorBlendMode::Color: return "カラー";
                case ColorBlendMode::Value: return "明度";
            }
            break;
        case Language::Chinese:
            switch (mode) {
                case ColorBlendMode::Mix: return "混合";
                case ColorBlendMode::Darken: return "变暗";
                case ColorBlendMode::Multiply: return "正片叠底";
                case ColorBlendMode::ColorBurn: return "颜色加深";
                case ColorBlendMode::Lighten: return "变亮";
                case ColorBlendMode::Screen: return "滤色";
                case ColorBlendMode::ColorDodge: return "颜色减淡";
                case ColorBlendMode::Overlay: return "叠加";
                case ColorBlendMode::Add: return "相加";
                case ColorBlendMode::SoftLight: return "柔光";
                case ColorBlendMode::LinearLight: return "线性光";
                case ColorBlendMode::Difference: return "差值";
                case ColorBlendMode::Exclusion: return "排除";
                case ColorBlendMode::Subtract: return "减去";
                case ColorBlendMode::Divide: return "除";
                case ColorBlendMode::Hue: return "色相";
                case ColorBlendMode::Saturation: return "饱和度";
                case ColorBlendMode::Color: return "颜色";
                case ColorBlendMode::Value: return "明度";
            }
            break;
        default: // English
            switch (mode) {
                case ColorBlendMode::Mix: return "Mix";
                case ColorBlendMode::Darken: return "Darken";
                case ColorBlendMode::Multiply: return "Multiply";
                case ColorBlendMode::ColorBurn: return "Color Burn";
                case ColorBlendMode::Lighten: return "Lighten";
                case ColorBlendMode::Screen: return "Screen";
                case ColorBlendMode::ColorDodge: return "Color Dodge";
                case ColorBlendMode::Overlay: return "Overlay";
                case ColorBlendMode::Add: return "Add";
                case ColorBlendMode::SoftLight: return "Soft Light";
                case ColorBlendMode::LinearLight: return "Linear Light";
                case ColorBlendMode::Difference: return "Difference";
                case ColorBlendMode::Exclusion: return "Exclusion";
                case ColorBlendMode::Subtract: return "Subtract";
                case ColorBlendMode::Divide: return "Divide";
                case ColorBlendMode::Hue: return "Hue";
                case ColorBlendMode::Saturation: return "Saturation";
                case ColorBlendMode::Color: return "Color";
                case ColorBlendMode::Value: return "Value";
            }
            break;
    }
    return "";
}

QString MixNode::getDataTypeString(DataType type, Language lang) {
    switch (lang) {
        case Language::Japanese:
            switch (type) {
                case DataType::Float: return "浮動小数点 (Float)";
                case DataType::Vector: return "ベクトル (Vector)";
                case DataType::Color: return "カラー (Color)";
            }
            break;
        case Language::Chinese:
            switch (type) {
                case DataType::Float: return "浮点数 (Float)";
                case DataType::Vector: return "向量 (Vector)";
                case DataType::Color: return "颜色 (Color)";
            }
            break;
        default:
            switch (type) {
                case DataType::Float: return "Float";
                case DataType::Vector: return "Vector";
                case DataType::Color: return "Color";
            }
            break;
    }
    return "";
}

QString MixNode::getVectorMixModeString(VectorMixMode mode, Language lang) {
    switch (lang) {
        case Language::Japanese:
            return (mode == VectorMixMode::Uniform) ? "均一 (Uniform)" : "非均一 (Non-Uniform)";
        case Language::Chinese:
            return (mode == VectorMixMode::Uniform) ? "均匀 (Uniform)" : "非均匀 (Non-Uniform)";
        default:
            return (mode == VectorMixMode::Uniform) ? "Uniform" : "Non-Uniform";
    }
}

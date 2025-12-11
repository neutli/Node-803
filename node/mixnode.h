#ifndef MIXNODE_H
#define MIXNODE_H

#include "node.h"

class MixNode : public Node {
public:
    enum class DataType {
        Float,
        Vector,
        Color
    };

    enum class ColorBlendMode {
        Mix,
        Darken,
        Multiply,
        ColorBurn,
        Lighten,
        Screen,
        ColorDodge,
        Overlay,
        Add,
        SoftLight,
        LinearLight,
        Difference,
        Exclusion,
        Subtract,
        Divide,
        Hue,
        Saturation,
        Color,
        Value
    };

    enum class VectorMixMode {
        Uniform,
        NonUniform
    };

    enum class Language {
        English,
        Japanese,
        Chinese
    };

    MixNode();

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override {
        QVector<ParameterInfo> params;
        
        // Data Type
        QStringList dataTypes = {
            getDataTypeString(DataType::Float, m_language),
            getDataTypeString(DataType::Vector, m_language),
            getDataTypeString(DataType::Color, m_language)
        };
        params.append(ParameterInfo("Data Type", dataTypes, 
            QVariant::fromValue(static_cast<int>(m_dataType)),
            [this](const QVariant& v) { const_cast<MixNode*>(this)->setDataType(static_cast<DataType>(v.toInt())); }
        ));

        // Mixed Mode
        if (m_dataType == DataType::Vector) {
            QStringList vectorModes = {
                getVectorMixModeString(VectorMixMode::Uniform, m_language),
                getVectorMixModeString(VectorMixMode::NonUniform, m_language)
            };
            params.append(ParameterInfo("Mic Mode", vectorModes, // Typo in UI label was "Mic Mode" or "Mix Mode"? using "Vector Mix Mode" in param name
                QVariant::fromValue(static_cast<int>(m_vectorMixMode)),
                [this](const QVariant& v) { const_cast<MixNode*>(this)->setVectorMixMode(static_cast<VectorMixMode>(v.toInt())); }
            ));
        } else if (m_dataType == DataType::Color) {
            QStringList blendModes;
            // Iterate all blend modes
            for (int i = 0; i <= static_cast<int>(ColorBlendMode::Value); ++i) {
                blendModes << getBlendModeString(static_cast<ColorBlendMode>(i), m_language);
            }
            params.append(ParameterInfo("Blend Mode", blendModes,
                QVariant::fromValue(static_cast<int>(m_colorBlendMode)),
                [this](const QVariant& v) { const_cast<MixNode*>(this)->setColorBlendMode(static_cast<ColorBlendMode>(v.toInt())); }
            ));
        }

        // Clamp
        if (m_dataType == DataType::Float || m_dataType == DataType::Color) {
            params.append(ParameterInfo("Clamp Result", m_clampResult, 
                [this](const QVariant& v) { const_cast<MixNode*>(this)->setClampResult(v.toBool()); }
            ));
        }

        // Language
        QStringList langs = {"English", "日本語", "中文"};
        params.append(ParameterInfo("Language", langs,
            QVariant::fromValue(static_cast<int>(m_language)),
            [this](const QVariant& v) { const_cast<MixNode*>(this)->setLanguage(static_cast<Language>(v.toInt())); }
        ));

        // Input Ranges (ensure widgets are created)
        params.append(ParameterInfo("Factor", 0.0, 1.0, 0.5));
        params.append(ParameterInfo("A", -10000.0, 10000.0, 0.0));
        params.append(ParameterInfo("B", -10000.0, 10000.0, 0.0));

        return params;
    }

    // Properties
    DataType dataType() const { return m_dataType; }
    void setDataType(DataType type);

    ColorBlendMode colorBlendMode() const { return m_colorBlendMode; }
    void setColorBlendMode(ColorBlendMode mode);

    VectorMixMode vectorMixMode() const { return m_vectorMixMode; }
    void setVectorMixMode(VectorMixMode mode);

    bool clampResult() const { return m_clampResult; }
    void setClampResult(bool clamp);

    double factor() const;
    void setFactor(double v);

    // Language support
    Language language() const { return m_language; }
    void setLanguage(Language lang);
    
    static QString getBlendModeString(ColorBlendMode mode, Language lang);
    static QString getDataTypeString(DataType type, Language lang);
    static QString getVectorMixModeString(VectorMixMode mode, Language lang);

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    DataType m_dataType;
    ColorBlendMode m_colorBlendMode;
    VectorMixMode m_vectorMixMode;
    bool m_clampResult;
    Language m_language;

    NodeSocket* m_factorInput;
    NodeSocket* m_inputA;
    NodeSocket* m_inputB;
    NodeSocket* m_output;

    // Helper for color blending
    QColor blendColor(const QColor& c1, const QColor& c2, double t) const;
    float blendFloat(float a, float b, float t) const; // For channel blending
};

#endif // MIXNODE_H

#ifndef NOISETEXTURENODE_H
#define NOISETEXTURENODE_H

#include "node.h"
#include "noise.h"
#include <QColor>
#include <memory>
#include <QJsonObject>
#include <QRecursiveMutex>

// ノイズテクスチャノード - Blenderのノイズテクスチャノードを模倣
class NoiseTextureNode : public Node {
public:
    NoiseTextureNode();
    ~NoiseTextureNode() override;
    
    // ノード評価
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    // Serialization
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

    // Dimensions
    enum class Dimensions {
        D2,
        D3,
        D4
    };

    // Distortion Type
    enum class DistortionType {
        Legacy, // Simple offset
        Blender // Domain warping
    };

    FractalType fractalType() const { return m_fractalType; }
    void setFractalType(FractalType t);

    // Getters
    double scale() const;
    double detail() const;
    double roughness() const;
    double distortion() const;
    double lacunarity() const;
    double offset() const;
    double w() const;
    NoiseType noiseType() const;
    Dimensions dimensions() const { return m_dimensions; }
    DistortionType distortionType() const { return m_distortionType; }
    bool normalize() const { return m_normalize; }
    
    // Setters
    // Setters
    void setScale(double v);
    void setDetail(double v);
    void setRoughness(double v);
    void setDistortion(double v);
    void setLacunarity(double v);
    void setOffset(double v);
    void setW(double v);
    void setNoiseType(NoiseType t);
    void setDimensions(Dimensions d);
    void setDistortionType(DistortionType t);
    void setNormalize(bool b);

    QVector<ParameterInfo> parameters() const override;

    double getNoiseValue(double x, double y, double z) const;
    QColor getColorValue(double x, double y, double z) const;

private:
    std::unique_ptr<PerlinNoise> m_noise;
    mutable QRecursiveMutex m_mutex;

    NoiseType m_noiseType;
    FractalType m_fractalType = FractalType::FBM;
    Dimensions m_dimensions;
    DistortionType m_distortionType;
    bool m_normalize;
    
    // ソケット参照（高速アクセス用）
    NodeSocket* m_vectorInput;
    NodeSocket* m_wInput; // For 4D
    NodeSocket* m_scaleInput;
    NodeSocket* m_detailInput;
    NodeSocket* m_roughnessInput;
    NodeSocket* m_distortionInput;
    NodeSocket* m_lacunarityInput;
    NodeSocket* m_offsetInput;
    NodeSocket* m_noiseTypeInput; // 0=Perlin, 1=Simplex, etc.
    
    NodeSocket* m_facOutput;
    NodeSocket* m_colorOutput;
};

#endif // NOISETEXTURENODE_H

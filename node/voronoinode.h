#ifndef VORONOINODE_H
#define VORONOINODE_H

#include "node.h"

class VoronoiNode : public Node {
public:
    VoronoiNode();
    ~VoronoiNode() override = default;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    QVector<ParameterInfo> parameters() const override;

    // Dimensions
    enum class Dimensions {
        D2,
        D3,
        D4
    };

    // Metric (Distance Metric)
    enum class Metric {
        Euclidean,
        Manhattan,
        Chebyshev,
        Minkowski
    };

    // Feature (Output Type)
    enum class Feature {
        F1,
        F2,
        SmoothF1,
        DistanceToEdge,
        NSphereRadius
    };

    // Getters
    double scale() const;
    double randomness() const;
    double detail() const;
    double roughness() const;
    double lacunarity() const;
    double w() const;
    Dimensions dimensions() const { return m_dimensions; }
    Metric metric() const { return m_metric; }
    Feature feature() const { return m_feature; }
    bool normalize() const { return m_normalize; }

    // Setters
    void setScale(double v);
    void setRandomness(double v);
    void setDetail(double v);
    void setRoughness(double v);
    void setLacunarity(double v);
    void setW(double v);
    void setDimensions(Dimensions d);
    void setMetric(Metric m);
    void setFeature(Feature f);
    void setNormalize(bool b);
    
    // Serialization
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    Dimensions m_dimensions;
    Metric m_metric;
    Feature m_feature;
    bool m_normalize;

    NodeSocket* m_vectorInput;
    NodeSocket* m_wInput; // For 4D
    NodeSocket* m_scaleInput;
    NodeSocket* m_detailInput; // For fractal voronoi (if we implement it) or just for consistency
    NodeSocket* m_roughnessInput;
    NodeSocket* m_lacunarityInput;
    NodeSocket* m_randomnessInput;
    
    NodeSocket* m_distanceOutput;
    NodeSocket* m_colorOutput;
    NodeSocket* m_positionOutput;
    NodeSocket* m_wOutput; // Optional
    NodeSocket* m_radiusOutput; // For N-Sphere Radius
};

#endif // VORONOINODE_H

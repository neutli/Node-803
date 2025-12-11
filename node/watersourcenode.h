#ifndef WATERSOURCENODE_H
#define WATERSOURCENODE_H

#include "node.h"
#include "noise.h"
#include <memory>
#include <QRecursiveMutex>
#include <QVector>
#include <QColor>

/**
 * WaterSourceNode - Creates a water source/lake shape
 * 
 * Features:
 * - Spherical gradient with noise distortion
 * - Built-in Color Ramp for lake size and color control
 * - Position X/Y controls for easy lake placement
 */
class WaterSourceNode : public Node {
public:
    WaterSourceNode();
    ~WaterSourceNode() override;

    QVector<ParameterInfo> parameters() const override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

    // === Built-in Color Ramp ===
    struct Stop {
        double position;
        QColor color;
    };
    
    void addStop(double pos, const QColor& color);
    void removeStop(int index);
    void setStopPosition(int index, double pos);
    void setStopColor(int index, const QColor& color);
    void clearStops();
    QVector<Stop> stops() const { return m_stops; }

    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

private:
    QColor evaluateRamp(double t);

    std::unique_ptr<PerlinNoise> m_noise;
    mutable QRecursiveMutex m_mutex;

    // Inputs
    NodeSocket* m_vectorInput;       // Texture coordinates
    NodeSocket* m_positionXInput;    // Lake position X
    NodeSocket* m_positionYInput;    // Lake position Y
    NodeSocket* m_scaleInput;        // Noise scale
    NodeSocket* m_mixFactorInput;    // Distortion amount
    NodeSocket* m_seedInput;         // Noise seed
    NodeSocket* m_detailInput;       // Noise octaves
    NodeSocket* m_roughnessInput;    // Noise roughness
    NodeSocket* m_lacunarityInput;   // Noise lacunarity

    // Outputs
    NodeSocket* m_facOutput;
    NodeSocket* m_colorOutput;

    // Built-in Color Ramp
    QVector<Stop> m_stops;
};

#endif // WATERSOURCENODE_H

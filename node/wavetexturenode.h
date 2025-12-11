#ifndef WAVETEXTURENODE_H
#define WAVETEXTURENODE_H

#include "node.h"

class WaveTextureNode : public Node {
public:
    WaveTextureNode();
    ~WaveTextureNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    QVector<ParameterInfo> parameters() const override;

    enum class WaveType { Bands, Rings };
    enum class WaveProfile { Sin, Saw, Tri };
    enum class WaveDirection { X, Y, Z, Diagonal };

    WaveType waveType() const { return m_waveType; }
    void setWaveType(WaveType t);

    WaveProfile waveProfile() const { return m_waveProfile; }
    void setWaveProfile(WaveProfile p);

    WaveDirection waveDirection() const { return m_waveDirection; }
    void setWaveDirection(WaveDirection d);

private:
    NodeSocket* m_vectorInput;
    NodeSocket* m_scaleInput;
    NodeSocket* m_distortionInput;
    NodeSocket* m_detailInput;
    NodeSocket* m_detailScaleInput;
    NodeSocket* m_detailRoughnessInput;
    NodeSocket* m_phaseOffsetInput;

    NodeSocket* m_colorOutput;
    NodeSocket* m_facOutput;

    WaveType m_waveType = WaveType::Bands;
    WaveProfile m_waveProfile = WaveProfile::Sin;
    WaveDirection m_waveDirection = WaveDirection::X;
};

#endif // WAVETEXTURENODE_H

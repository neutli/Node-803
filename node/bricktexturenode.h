#ifndef BRICKTEXTURENODE_H
#define BRICKTEXTURENODE_H

#include "node.h"

class BrickTextureNode : public Node {
public:
    BrickTextureNode();
    ~BrickTextureNode() override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    QVector<ParameterInfo> parameters() const override;

    double offset() const { return m_offset; }
    void setOffset(double v);
    
    int offsetFrequency() const { return m_offsetFrequency; }
    void setOffsetFrequency(int v);
    
    double squash() const { return m_squash; }
    void setSquash(double v);
    
    int squashFrequency() const { return m_squashFrequency; }
    void setSquashFrequency(int v);

private:
    NodeSocket* m_vectorInput;
    NodeSocket* m_color1Input;
    NodeSocket* m_color2Input;
    NodeSocket* m_mortarInput;
    NodeSocket* m_scaleInput;
    NodeSocket* m_mortarSizeInput;
    NodeSocket* m_mortarSmoothInput;
    NodeSocket* m_biasInput;
    NodeSocket* m_brickWidthInput;
    NodeSocket* m_rowHeightInput;

    NodeSocket* m_colorOutput;
    NodeSocket* m_facOutput;

    double m_offset = 0.5;
    int m_offsetFrequency = 2;
    double m_squash = 1.0;
    int m_squashFrequency = 2;
};

#endif // BRICKTEXTURENODE_H

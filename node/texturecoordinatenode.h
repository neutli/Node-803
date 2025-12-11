#ifndef TEXTURECOORDINATENODE_H
#define TEXTURECOORDINATENODE_H

#include "node.h"
#include <QVector3D>

// テクスチャ座標ノード - UV座標を生成
class TextureCoordinateNode : public Node {
public:
    TextureCoordinateNode();
    ~TextureCoordinateNode() override;
    
    // ノード評価
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;

    // Coordinate type
    enum class CoordinateType {
        Generated = 0,  // 0..1
        Object = 1,     // -0.5..0.5 (centered)
        UV = 2,
        Camera = 3,
        Window = 4,
        Normal = 5,
        Reflection = 6
    };
    
    CoordinateType coordinateType() const;
    void setCoordinateType(CoordinateType type);

private:
    NodeSocket* m_typeInput;  // Coordinate type selector
    NodeSocket* m_output;
    
    CoordinateType m_coordinateType;
};

#endif // TEXTURECOORDINATENODE_H

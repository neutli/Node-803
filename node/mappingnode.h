#ifndef MAPPINGNODE_H
#define MAPPINGNODE_H

#include "node.h"
#include <QVector3D>
#include <QMatrix4x4>

class MappingNode : public Node {
public:
    MappingNode();
    ~MappingNode() override;
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;

    // パラメータ取得
    QVector3D location() const;
    QVector3D rotation() const;
    QVector3D scale() const;
    
    // パラメータ設定
    void setLocation(const QVector3D& loc);
    void setRotation(const QVector3D& rot);
    void setScale(const QVector3D& scale);
    
    // 変換適用
    QVector3D mapVector(const QVector3D& vec) const;

private:
    NodeSocket* m_vectorInput;
    
    NodeSocket* m_locationInput;
    NodeSocket* m_rotationInput;
    NodeSocket* m_scaleInput;
    
    NodeSocket* m_vectorOutput;
};

#endif // MAPPINGNODE_H

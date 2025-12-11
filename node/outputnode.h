#ifndef OUTPUTNODE_H
#define OUTPUTNODE_H

#include "node.h"
#include <QColor>
#include <QImage>

class OutputNode : public Node {
public:
    OutputNode();
    ~OutputNode() override;
    
    void evaluate() override;
    
    
    // 画像生成（ノードリストからTextureCoordinateNodeを探して解像度を取得）
    QImage render(const QVector<Node*>& nodes) const;
    
    // パラメータ取得
    QColor surfaceColor() const;

private:
    NodeSocket* m_surfaceInput;
    bool m_autoUpdate;
    
public:
    bool autoUpdate() const { return m_autoUpdate; }
    void setAutoUpdate(bool active) { m_autoUpdate = active; }
};

#endif // OUTPUTNODE_H

#ifndef TEXTNODE_H
#define TEXTNODE_H

#include "node.h"
#include <QImage>

class TextNode : public Node {
public:
    TextNode();

    // Explicit overrides not needed for name/counts based on new Node.h structure?
    // checking base class.. Node.h has separate name member.
    // Construct sets name.

    QVector<ParameterInfo> parameters() const override;

    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;

private:
    void renderText();

    // Parameters
    QString m_text;
    float m_size;
    float m_xOffset;
    float m_yOffset;
    
    // Internal cache
    QImage m_cachedImage;
    bool m_cacheDirty;
};

#endif // TEXTNODE_H

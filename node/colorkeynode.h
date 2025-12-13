#ifndef COLORKEYNODE_H
#define COLORKEYNODE_H

#include "node.h"
#include <QRecursiveMutex>
#include <QColor>

// Color Key Node - Removes specific color and converts to alpha (chroma key)
class ColorKeyNode : public Node {
public:
    ColorKeyNode();
    ~ColorKeyNode() override;
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    
    QVector<ParameterInfo> parameters() const override;
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;
    
    // Accessors for custom widget
    QColor keyColor() const { return m_keyColor; }
    void setKeyColor(const QColor& c) { m_keyColor = c; setDirty(true); }
    
private:
    // Input sockets
    NodeSocket* m_colorInput;      // Input color to process
    
    // Output sockets
    NodeSocket* m_colorOutput;
    NodeSocket* m_alphaOutput;
    
    // Parameters
    QColor m_keyColor = QColor(0, 255, 0);  // Default: green screen
    double m_tolerance = 0.3;               // Color matching tolerance
    double m_falloff = 0.1;                 // Edge softness
    bool m_invert = false;                  // Invert selection
    
    mutable QRecursiveMutex m_mutex;
    
    // Helper: compute color distance
    double colorDistance(const QVector4D& c1, const QVector4D& c2) const;
};

#endif // COLORKEYNODE_H

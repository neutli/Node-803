#ifndef IMAGETEXTURENODE_H
#define IMAGETEXTURENODE_H

#include "node.h"
#include <QImage>
#include <QString>
#include <QJsonObject>

class ImageTextureNode : public Node {
public:
    ImageTextureNode();
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    QVector<ParameterInfo> parameters() const override;
    
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;
    
    QString filePath() const;
    void setFilePath(const QString& path);
    
    // Scale (like Blender's UV scale)
    double scaleX() const { return m_scaleX; }
    double scaleY() const { return m_scaleY; }
    void setScaleX(double s);
    void setScaleY(double s);
    
    // Stretch mode - stretch image to fill UV space
    bool stretchToFit() const { return m_stretchToFit; }
    void setStretchToFit(bool stretch);
    
    // Keep aspect ratio - adjusts UV range to match image aspect
    bool keepAspectRatio() const { return m_keepAspectRatio; }
    void setKeepAspectRatio(bool keep);
    
    // Repeat mode
    bool repeat() const { return m_repeat; }
    void setRepeat(bool r);
    
    // Get image dimensions
    int imageWidth() const { return m_image.width(); }
    int imageHeight() const { return m_image.height(); }
    
    // Helper to get color at UV
    QColor getColorAt(double u, double v) const;

private:
    NodeSocket* m_vectorInput;
    NodeSocket* m_colorOutput;
    NodeSocket* m_alphaOutput;
    
    QString m_filePath;
    QImage m_image;
    bool m_imageLoaded;
    
    double m_scaleX;
    double m_scaleY;
    bool m_stretchToFit;
    bool m_keepAspectRatio;
    bool m_repeat;
    
    void loadImage();
    void applyAspectRatio();  // Apply aspect ratio to render settings
};

#endif // IMAGETEXTURENODE_H

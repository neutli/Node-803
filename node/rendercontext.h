#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include <QVector3D>

// Global rendering context accessible by all nodes
class RenderContext {
public:
    static RenderContext& instance();
    
    void setRenderSize(int width, int height);
    int renderWidth() const { return m_renderWidth; }
    int renderHeight() const { return m_renderHeight; }
    
    void setCurrentPixel(const QVector3D& pixel);
    QVector3D currentPixel() const { return m_currentPixel; }
    
private:
    RenderContext() : m_renderWidth(512), m_renderHeight(512) {}
    
    int m_renderWidth;
    int m_renderHeight;
    QVector3D m_currentPixel;
};

#endif // RENDERCONTEXT_H

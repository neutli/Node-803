#include "rendercontext.h"

RenderContext& RenderContext::instance() {
    static thread_local RenderContext ctx;
    return ctx;
}

void RenderContext::setRenderSize(int width, int height) {
    m_renderWidth = width;
    m_renderHeight = height;
}

void RenderContext::setCurrentPixel(const QVector3D& pixel) {
    m_currentPixel = pixel;
}

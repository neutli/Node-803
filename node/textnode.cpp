#include "textnode.h"
#include <QPainter>
#include <QFont>
#include <QColor>

TextNode::TextNode() : Node("Text") {
    // Inputs
    addInputSocket(new NodeSocket("UV", SocketType::Vector, SocketDirection::Input, this)); // 0

    // Outputs
    addOutputSocket(new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this)); // 0
    addOutputSocket(new NodeSocket("Alpha", SocketType::Float, SocketDirection::Output, this)); // 1
    
    // Default params
    m_text = "Text";
    m_size = 50.0f;
    m_xOffset = 0.5f;
    m_yOffset = 0.5f;

    m_cacheDirty = true;
}

QVector<Node::ParameterInfo> TextNode::parameters() const {
    return {
        ParameterInfo(ParameterInfo::String, "Text", "Text", [this](const QVariant& v) {
            auto* node = const_cast<TextNode*>(this);
            node->m_text = v.toString();
            node->m_cacheDirty = true;
            node->setDirty(true);
        }),
        ParameterInfo("Size", 10.0f, 200.0f, 50.0f, 1.0f, "Font Size", [this](const QVariant& v) {
            auto* node = const_cast<TextNode*>(this);
            node->m_size = v.toFloat();
            node->m_cacheDirty = true;
            node->setDirty(true);
        }),
        ParameterInfo("X", -1.0f, 2.0f, 0.5f, 0.01f, "X Position", [this](const QVariant& v) {
            auto* node = const_cast<TextNode*>(this);
            node->m_xOffset = v.toFloat();
            node->setDirty(true);
        }),
        ParameterInfo("Y", -1.0f, 2.0f, 0.5f, 0.01f, "Y Position", [this](const QVariant& v) {
            auto* node = const_cast<TextNode*>(this);
            node->m_yOffset = v.toFloat();
            node->setDirty(true);
        })
    };
}

void TextNode::renderText() {
    if (!m_cacheDirty) return;

    // Create a reasonable resolution image
    int size = 1024;
    m_cachedImage = QImage(size, size, QImage::Format_ARGB32);
    m_cachedImage.fill(Qt::transparent);

    QPainter painter(&m_cachedImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont font("Arial"); // Default simple font
    font.setPixelSize(static_cast<int>(m_size * (size / 512.0f))); // Scale relative to texture size
    painter.setFont(font);

    painter.setPen(Qt::white);
    
    // Draw text centered for now, alignment logic handled by UV offset in compute or here?
    // Let's draw centered and let UV inputs handle detailed placement, 
    // BUT user asked for X/Y params.
    // The X/Y params in UV space (0-1) correspond to where the text center should be.
    // If we draw strictly to the image, we can just center it and shift UV reading.
    // Or we can draw at the offset position. 
    // Drawing at offset position is intuitive.

    // Font metrics for centering
    QFontMetrics params(font);
    int textWidth = params.horizontalAdvance(m_text);
    int textHeight = params.height();

    // Map 0-1 offset to pixel coords
    // Note: Y is usually top-down in QImage, but UV is bottom-up in 3D usually.
    // Let's assume standard image coords for drawing (0,0 top-left).
    // If X/Y is 0.5, 0.5, we want text center at 512, 512.
    
    // However, since we cache the image once, baking the offset into the image means
    // if the user pans X, we have to redraw.
    // Retaining 'centered' image and shifting UV in compute is much faster (no redraw).
    // BUT, 'TextNode' usually implies baking for specific layout.
    // Let's bake it for simplicity of "What You See".
    
    int x = static_cast<int>(m_xOffset * size) - textWidth / 2;
    // For UV y=0 at bottom, image y=size at bottom.
    // yOffset 0.5 -> center. yOffset 0 -> bottom?
    // Let's stick to standard 2D image Top-Left origin for parameters for now, 
    // unless this is a texture node chain which assumes UV (0,0 bottom-left).
    // Most param inputs in this app seem 0-1 range.
    // Let's treat Y=0.5 as center.
    int y = static_cast<int>((1.0f - m_yOffset) * size) + textHeight / 4; // Approx baseline adjustment

    // Actually, let's just center draw and apply offset in compute? 
    // No, reusing cached image is good, but re-rendering text is cheap enough for parameter change triggers.
    // It's cleaner to bake it if we support rotation later.
    
    painter.drawText(x, y, m_text);
    painter.end();

    m_cacheDirty = false;
}

void TextNode::evaluate() {
    if (m_cacheDirty) {
        renderText();
    }
    setDirty(false);
}

QVariant TextNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // Get UV
    QVector3D uv = pos;
    if (m_inputSockets[0]->isConnected()) {
        uv = m_inputSockets[0]->getValue(pos).value<QVector3D>();
    }

    // Since we baked position into the image, we just sample directly at UV.
    // UV is 0-1.
    // Image is 0-1 (mapped to 0-1023).
    // QImage: (0,0) is top-left. UV (0,0) is usually bottom-left.
    
    float u = uv.x();
    float v = 1.0f - uv.y(); // Flip V for image sampling

    // Wrap or Clamp? Usually Clamp or Repeat. 
    // For text, Clamp to Border (Transparent) is best.
    if (u < 0 || u > 1 || v < 0 || v > 1) {
        if (socket->type() == SocketType::Color) return QColor(0,0,0,0);
        return 0.0f;
    }

    int x = static_cast<int>(u * (m_cachedImage.width() - 1));
    int y = static_cast<int>(v * (m_cachedImage.height() - 1));

    QColor c = m_cachedImage.pixelColor(x, y);

    if (socket == m_outputSockets[0]) { // Color
        return c;
    } else { // Alpha
        return c.alphaF();
    }
}

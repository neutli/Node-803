#include "imagetexturenode.h"
#include "appsettings.h"
#include <QColor>
#include <QDebug>
#include <cmath>

ImageTextureNode::ImageTextureNode()
    : Node("Image Texture")
    , m_imageLoaded(false)
    , m_scaleX(1.0)
    , m_scaleY(1.0)
    , m_stretchToFit(false)  // Default OFF - center image with original aspect
    , m_keepAspectRatio(false)
    , m_repeat(false)
{
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    addInputSocket(m_vectorInput);
    
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);
    addOutputSocket(m_colorOutput);
    
    m_alphaOutput = new NodeSocket("Alpha", SocketType::Float, SocketDirection::Output, this);
    addOutputSocket(m_alphaOutput);
}

void ImageTextureNode::evaluate() {
    // Stateless
}

QVector<Node::ParameterInfo> ImageTextureNode::parameters() const {
    QVector<ParameterInfo> params;
    
    // File
    params.append(ParameterInfo("Image File", m_filePath, 
        [this](const QVariant& v) { const_cast<ImageTextureNode*>(this)->setFilePath(v.toString()); }
    ));

    // Scale X
    ParameterInfo scaleXParam("Scale X", 0.01, 100.0, m_scaleX, 0.1, "");
    scaleXParam.setter = [this](const QVariant& v) { const_cast<ImageTextureNode*>(this)->setScaleX(v.toDouble()); };
    params.append(scaleXParam);

    ParameterInfo scaleYParam("Scale Y", 0.01, 100.0, m_scaleY);
    scaleYParam.setter = [this](const QVariant& v) { const_cast<ImageTextureNode*>(this)->setScaleY(v.toDouble()); };
    params.append(scaleYParam);

    // Bools
    params.append(ParameterInfo("Stretch to Fit", m_stretchToFit, 
        [this](const QVariant& v) { const_cast<ImageTextureNode*>(this)->setStretchToFit(v.toBool()); },
        "ONにすると画像をUV空間に合わせて引き伸ばす"
    ));

    params.append(ParameterInfo("Keep Aspect Ratio", m_keepAspectRatio, 
        [this](const QVariant& v) { const_cast<ImageTextureNode*>(this)->setKeepAspectRatio(v.toBool()); },
        "ONにするとビューポート範囲を画像のアスペクト比に合わせる"
    ));

    params.append(ParameterInfo("Repeat", m_repeat, 
        [this](const QVariant& v) { const_cast<ImageTextureNode*>(this)->setRepeat(v.toBool()); },
        "ON: 画像をタイル状に繰り返す\nOFF: 画像を1回だけ表示"
    ));

    return params;
}

QVariant ImageTextureNode::compute(const QVector3D& pos, NodeSocket* socket) {
    QVector3D uv = pos;
    if (m_vectorInput->isConnected()) {
        uv = m_vectorInput->getValue(pos).value<QVector3D>();
    }
    
    QColor c = getColorAt(uv.x(), uv.y());
    
    if (socket == m_colorOutput) {
        return c;
    } else if (socket == m_alphaOutput) {
        return c.alphaF();
    }
    
    return QVariant();
}

QColor ImageTextureNode::getColorAt(double u, double v) const {
    if (!m_imageLoaded || m_image.isNull()) {
        return QColor(0, 0, 0, 255);
    }
    
    // Apply scale (from center)
    u = (u - 0.5) * m_scaleX + 0.5;
    v = (v - 0.5) * m_scaleY + 0.5;
    
    // Get render resolution
    int renderW = AppSettings::instance().renderWidth();
    int renderH = AppSettings::instance().renderHeight();
    int imgW = m_image.width();
    int imgH = m_image.height();
    
    if (m_stretchToFit) {
        // Stretch: UV 0-1 maps directly to image 0-1
        // Image fills entire render, may be distorted
    } else {
        // Center the image without distortion
        // Calculate where the image should be placed in UV space
        
        // Image aspect vs render aspect
        double imgAspect = (double)imgW / imgH;
        double renderAspect = (double)renderW / renderH;
        
        // Calculate UV extent of image when centered
        double imgUvWidth, imgUvHeight;
        
        if (imgAspect > renderAspect) {
            // Image is wider than render
            // Fit by width, image will be shorter than render
            imgUvWidth = 1.0;
            imgUvHeight = renderAspect / imgAspect;
        } else {
            // Image is taller than render
            // Fit by height, image will be narrower than render
            imgUvHeight = 1.0;
            imgUvWidth = imgAspect / renderAspect;
        }
        
        // Center offsets
        double imgUvMinU = (1.0 - imgUvWidth) / 2.0;
        double imgUvMaxU = imgUvMinU + imgUvWidth;
        double imgUvMinV = (1.0 - imgUvHeight) / 2.0;
        double imgUvMaxV = imgUvMinV + imgUvHeight;
        
        // Check if UV is outside image bounds
        if (u < imgUvMinU || u > imgUvMaxU || v < imgUvMinV || v > imgUvMaxV) {
            return QColor(0, 0, 0, 255);  // Black outside
        }
        
        // Remap UV from image bounds to 0-1
        u = (u - imgUvMinU) / imgUvWidth;
        v = (v - imgUvMinV) / imgUvHeight;
    }
    
    if (m_repeat) {
        u = u - std::floor(u);
        v = v - std::floor(v);
    } else {
        if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0) {
            return QColor(0, 0, 0, 255);
        }
    }
    
    int x = static_cast<int>(u * imgW);
    int y = static_cast<int>(v * imgH);
    
    x = qBound(0, x, imgW - 1);
    y = qBound(0, y, imgH - 1);
    
    return m_image.pixelColor(x, y);
}

QString ImageTextureNode::filePath() const {
    return m_filePath;
}

void ImageTextureNode::setFilePath(const QString& path) {
    if (m_filePath != path) {
        m_filePath = path;
        loadImage();
        
        // Re-apply aspect ratio if enabled
        if (m_keepAspectRatio && m_imageLoaded && m_image.width() > 0 && m_image.height() > 0) {
            applyAspectRatio();
        }
        
        setDirty(true);
        notifyStructureChanged();
    }
}

void ImageTextureNode::setScaleX(double s) {
    if (m_scaleX != s) {
        m_scaleX = s;
        setDirty(true);
    }
}

void ImageTextureNode::setScaleY(double s) {
    if (m_scaleY != s) {
        m_scaleY = s;
        setDirty(true);
    }
}

void ImageTextureNode::setStretchToFit(bool stretch) {
    if (m_stretchToFit != stretch) {
        m_stretchToFit = stretch;
        setDirty(true);
    }
}

void ImageTextureNode::setKeepAspectRatio(bool keep) {
    if (m_keepAspectRatio != keep) {
        m_keepAspectRatio = keep;
        
        // When enabled, adjust render resolution to match image aspect ratio
        if (keep && m_imageLoaded && m_image.width() > 0 && m_image.height() > 0) {
            applyAspectRatio();
        }
        
        setDirty(true);
    }
}

void ImageTextureNode::setRepeat(bool r) {
    if (m_repeat != r) {
        m_repeat = r;
        setDirty(true);
    }
}

void ImageTextureNode::applyAspectRatio() {
    if (!m_imageLoaded || m_image.width() <= 0 || m_image.height() <= 0) {
        return;
    }
    
    int renderW = AppSettings::instance().renderWidth();
    int renderH = AppSettings::instance().renderHeight();
    int totalPixels = renderW * renderH;
    
    double imgAspect = (double)m_image.width() / m_image.height();
    
    // Calculate new dimensions that match image aspect
    // while keeping approximately the same total pixel count
    int newW = static_cast<int>(std::sqrt(totalPixels * imgAspect));
    int newH = static_cast<int>(std::sqrt(totalPixels / imgAspect));
    
    AppSettings::instance().setRenderWidth(newW);
    AppSettings::instance().setRenderHeight(newH);
}

void ImageTextureNode::loadImage() {
    if (m_filePath.isEmpty()) {
        m_image = QImage();
        m_imageLoaded = false;
        return;
    }
    
    bool success = m_image.load(m_filePath);
    if (success) {
        m_imageLoaded = true;
    } else {
        qDebug() << "Failed to load image:" << m_filePath;
        m_image = QImage();
        m_imageLoaded = false;
    }
}

QJsonObject ImageTextureNode::save() const {
    QJsonObject json = Node::save();
    json["filePath"] = m_filePath;
    json["scaleX"] = m_scaleX;
    json["scaleY"] = m_scaleY;
    json["stretchToFit"] = m_stretchToFit;
    json["keepAspectRatio"] = m_keepAspectRatio;
    json["repeat"] = m_repeat;
    return json;
}

void ImageTextureNode::restore(const QJsonObject& json) {
    Node::restore(json);
    if (json.contains("filePath")) {
        setFilePath(json["filePath"].toString());
    }
    if (json.contains("scaleX")) {
        m_scaleX = json["scaleX"].toDouble();
    }
    if (json.contains("scaleY")) {
        m_scaleY = json["scaleY"].toDouble();
    }
    if (json.contains("stretchToFit")) {
        m_stretchToFit = json["stretchToFit"].toBool();
    }
    if (json.contains("keepAspectRatio")) {
        m_keepAspectRatio = json["keepAspectRatio"].toBool();
    }
    if (json.contains("repeat")) {
        m_repeat = json["repeat"].toBool();
    }
}

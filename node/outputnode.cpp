#include "outputnode.h"
#include "texturecoordinatenode.h"
#include "rendercontext.h"
#include "appsettings.h"
#include <QVector>
#include <QVector4D>
#include <QSemaphore>
#include <QThreadPool>

OutputNode::OutputNode()
    : Node("Material Output")
{
    // 入力
    m_surfaceInput = new NodeSocket("Surface", SocketType::Color, SocketDirection::Input, this);
    m_surfaceInput->setDefaultValue(QColor(Qt::black));
    addInputSocket(m_surfaceInput);
    m_autoUpdate = true;
}

OutputNode::~OutputNode() {
}

void OutputNode::evaluate() {
    // 出力ノードは評価の終点なので、特に計算はしない
    // 接続されたノードの値をプルするだけ
    setDirty(false);
}

QImage OutputNode::render(const QVector<Node*>& nodes) const {
    // Get resolution from global AppSettings
    int width = AppSettings::instance().renderWidth();
    int height = AppSettings::instance().renderHeight();
    
    // Safety check for resolution
    if (width <= 0 || height <= 0) return QImage();
    if (width > 8192 || height > 8192) {
        // Limit max resolution to prevent bad_alloc
        width = qMin(width, 8192);
        height = qMin(height, 8192);
    }
    
    // Try to allocate image, handle bad_alloc
    QImage image;
    try {
        image = QImage(width, height, QImage::Format_ARGB32);
        if (image.isNull()) return QImage();
        image.fill(Qt::black);
    } catch (...) {
        return QImage();
    }
    
    if (!m_surfaceInput->isConnected()) {
        return image;
    }
    
    const QVector<NodeSocket*>& connections = m_surfaceInput->connections();
    if (connections.isEmpty()) return image;
    
    NodeSocket* sourceSocket = connections[0];
    Node* sourceNode = sourceSocket->parentNode();
    
    // Parallel rendering using QtConcurrent
    // We process by rows to reduce overhead
    QVector<int> rows;
    rows.reserve(height);
    for (int y = 0; y < height; ++y) {
        rows.append(y);
    }

    // Function to process a single row
    auto processRow = [&](int y) {
        QRgb* scanLine = (QRgb*)image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            // Pass pixel coordinates - nodes like TextureCoordinate will generate UV
            QVector3D pixelPos(x, y, 0.0);
            QVariant result = sourceNode->compute(pixelPos, sourceSocket);
            
            QColor color;
            if (result.canConvert<QVector4D>()) {
                // RGBA color with alpha channel (from Color Key, etc.)
                QVector4D vec = result.value<QVector4D>();
                int r = qBound(0, static_cast<int>(vec.x() * 255.0f), 255);
                int g = qBound(0, static_cast<int>(vec.y() * 255.0f), 255);
                int b = qBound(0, static_cast<int>(vec.z() * 255.0f), 255);
                int a = qBound(0, static_cast<int>(vec.w() * 255.0f), 255);
                color = QColor(r, g, b, a);
            } else if (result.canConvert<QColor>()) {
                color = result.value<QColor>();
                if (!color.isValid()) color = Qt::black;
            } else if (result.canConvert<QVector3D>()) {
                QVector3D vec = result.value<QVector3D>();
                // Map -1..1 to 0..1 for visualization
                float r = (vec.x() * 0.5f + 0.5f) * 255.0f;
                float g = (vec.y() * 0.5f + 0.5f) * 255.0f;
                float b = (vec.z() * 0.5f + 0.5f) * 255.0f;
                color = QColor(qBound(0, (int)r, 255), qBound(0, (int)g, 255), qBound(0, (int)b, 255));
            } else if (result.canConvert<double>()) {
                double val = result.toDouble();
                if (std::isnan(val)) val = 0.0;
                int gray = static_cast<int>(val * 255);
                gray = qBound(0, gray, 255);
                color = QColor(gray, gray, gray);
            } else {
                color = Qt::black;
            }
            
            scanLine[x] = color.rgba();
        }
    };

    // Set thread count
    int maxThreads = AppSettings::instance().maxThreads();
    QThreadPool::globalInstance()->setMaxThreadCount(maxThreads);

    // Parallel rendering using QThreadPool directly
    // This avoids the 'astd' compilation error in QtConcurrent headers
    QSemaphore sem(0);

    for (int y : rows) {
        QThreadPool::globalInstance()->start([&, y]() {
            processRow(y);
            sem.release();
        });
    }
    
    // Wait for all rows to finish
    sem.acquire(height);
    
    return image;
}

QColor OutputNode::surfaceColor() const {
    if (m_surfaceInput->isConnected()) {
        return m_surfaceInput->value().value<QColor>();
    }
    return m_surfaceInput->defaultValue().value<QColor>();
}

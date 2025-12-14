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
        // Use Format_RGBA8888 for explicit byte-order handling (R, G, B, A)
        // This avoids endian confusion with ARGB32's word-based format
        image = QImage(width, height, QImage::Format_RGBA8888);
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
    
    // Parallel rendering processing rows
    QVector<int> rows;
    rows.reserve(height);
    for (int y = 0; y < height; ++y) {
        rows.append(y);
    }

    auto processRow = [&](int y) {
        // Get raw byte pointer for RGBA8888
        uchar* scanLine = image.scanLine(y);
        for (int x = 0; x < width; ++x) {
            QVector3D pixelPos(x, y, 0.0);
            QVariant result = sourceNode->compute(pixelPos, sourceSocket);
            
            // Standardize color extraction
            int r = 0, g = 0, b = 0, a = 255;
            
            if (result.canConvert<QVector4D>()) {
                QVector4D vec = result.value<QVector4D>();
                r = qBound(0, static_cast<int>(vec.x() * 255.0f), 255);
                g = qBound(0, static_cast<int>(vec.y() * 255.0f), 255);
                b = qBound(0, static_cast<int>(vec.z() * 255.0f), 255);
                a = qBound(0, static_cast<int>(vec.w() * 255.0f), 255);
            } else if (result.canConvert<QColor>()) {
                QColor c = result.value<QColor>();
                if (c.isValid()) {
                    r = c.red(); g = c.green(); b = c.blue(); a = c.alpha();
                }
            } else if (result.canConvert<QVector3D>()) {
                QVector3D vec = result.value<QVector3D>();
                r = qBound(0, static_cast<int>((vec.x() * 0.5f + 0.5f) * 255.0f), 255);
                g = qBound(0, static_cast<int>((vec.y() * 0.5f + 0.5f) * 255.0f), 255);
                b = qBound(0, static_cast<int>((vec.z() * 0.5f + 0.5f) * 255.0f), 255);
            } else if (result.canConvert<double>()) {
                double val = result.toDouble();
                if (!std::isnan(val)) {
                    int gray = qBound(0, static_cast<int>(val * 255), 255);
                    r = g = b = gray;
                }
            }
            
            // Explicit byte assignment: RGBA8888 = R, G, B, A
            int offset = x * 4;
            scanLine[offset + 0] = static_cast<uchar>(r);
            scanLine[offset + 1] = static_cast<uchar>(g);
            scanLine[offset + 2] = static_cast<uchar>(b);
            scanLine[offset + 3] = static_cast<uchar>(a);
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

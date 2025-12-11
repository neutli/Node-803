#include "rivernode.h"
#include "appsettings.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <QColor>
#include <QVector2D>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

RiverNode::RiverNode() : Node("River Texture"), m_isCached(false), m_noiseType(NoiseType::Perlin), m_edgeConnection(true) {
    m_noise = std::make_unique<PerlinNoise>();

    // Inputs
    m_vectorInput = new NodeSocket("Vector", SocketType::Vector, SocketDirection::Input, this);
    
    m_waterMaskInput = new NodeSocket("Water Mask", SocketType::Color, SocketDirection::Input, this);
    m_waterMaskInput->setDefaultValue(QColor(Qt::black));

    m_scaleInput = new NodeSocket("Scale", SocketType::Float, SocketDirection::Input, this);
    m_scaleInput->setDefaultValue(5.0);  // Noise frequency for distortion
    
    m_distortionInput = new NodeSocket("Distortion", SocketType::Float, SocketDirection::Input, this);
    m_distortionInput->setDefaultValue(20.0);  // Distortion strength
    
    m_widthInput = new NodeSocket("Width", SocketType::Float, SocketDirection::Input, this);
    m_widthInput->setDefaultValue(0.02);  // River width (0-1 range)
    
    m_widthVariationInput = new NodeSocket("Width Variation", SocketType::Float, SocketDirection::Input, this);
    m_widthVariationInput->setDefaultValue(0.5); // 0.0 = constant width, 1.0 = max variation
    
    m_attenuationInput = new NodeSocket("Attenuation", SocketType::Float, SocketDirection::Input, this);
    m_attenuationInput->setDefaultValue(0.0); // 0.0 = no attenuation, 1.0 = full taper
    
    m_countInput = new NodeSocket("Source Count", SocketType::Integer, SocketDirection::Input, this);
    m_countInput->setDefaultValue(3.0);  // Number of rivers (Source points)
    
    m_pointsInput = new NodeSocket("Points", SocketType::Integer, SocketDirection::Input, this);
    m_pointsInput->setDefaultValue(50.0);  // Number of points per river
    
    m_seedInput = new NodeSocket("Seed", SocketType::Float, SocketDirection::Input, this);
    m_seedInput->setDefaultValue(0.0);
    
    m_targetColorInput = new NodeSocket("Target Color", SocketType::Color, SocketDirection::Input, this);
    m_targetColorInput->setDefaultValue(QColor(255, 255, 255)); // White (matches Water Source default)
    
    m_toleranceInput = new NodeSocket("Tolerance", SocketType::Float, SocketDirection::Input, this);
    m_toleranceInput->setDefaultValue(0.1);
    
    m_destinationColorInput = new NodeSocket("Dest Color", SocketType::Color, SocketDirection::Input, this);
    m_destinationColorInput->setDefaultValue(QColor(255, 0, 0)); // Red

    m_mergeDistanceInput = new NodeSocket("Merge Distance", SocketType::Float, SocketDirection::Input, this);
    m_mergeDistanceInput->setDefaultValue(0.15); // Default merge distance (0-1 range)
    
    m_riverColorInput = new NodeSocket("River Color", SocketType::Color, SocketDirection::Input, this);
    m_riverColorInput->setDefaultValue(QColor(255, 255, 255)); // Default White

    addInputSocket(m_vectorInput);
    addInputSocket(m_waterMaskInput);
    addInputSocket(m_scaleInput);
    addInputSocket(m_distortionInput);
    addInputSocket(m_widthInput);
    addInputSocket(m_widthVariationInput);
    addInputSocket(m_attenuationInput);
    addInputSocket(m_countInput);
    addInputSocket(m_pointsInput);
    addInputSocket(m_seedInput);
    addInputSocket(m_targetColorInput);
    addInputSocket(m_toleranceInput);
    addInputSocket(m_mergeDistanceInput);
    addInputSocket(m_riverColorInput);
    addInputSocket(m_destinationColorInput);

    m_destCountInput = new NodeSocket("Dest Count", SocketType::Integer, SocketDirection::Input, this);
    m_destCountInput->setDefaultValue(3.0);

    m_destToleranceInput = new NodeSocket("Dest Tolerance", SocketType::Float, SocketDirection::Input, this);
    m_destToleranceInput->setDefaultValue(0.1);

    m_destMergeDistanceInput = new NodeSocket("Dest Merge Dist", SocketType::Float, SocketDirection::Input, this);
    m_destMergeDistanceInput->setDefaultValue(0.15);

    m_mapSizeInput = new NodeSocket("Map Size", SocketType::Integer, SocketDirection::Input, this);
    m_mapSizeInput->setDefaultValue(512);

    m_minDistanceInput = new NodeSocket("Min Distance", SocketType::Float, SocketDirection::Input, this);
    m_minDistanceInput->setDefaultValue(0.1);

    addInputSocket(m_destCountInput);
    addInputSocket(m_destToleranceInput);
    addInputSocket(m_destMergeDistanceInput);
    addInputSocket(m_mapSizeInput);
    addInputSocket(m_minDistanceInput);

    // Outputs
    m_facOutput = new NodeSocket("Fac", SocketType::Float, SocketDirection::Output, this);
    m_colorOutput = new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this);

    addOutputSocket(m_facOutput);
    addOutputSocket(m_colorOutput);
}

RiverNode::~RiverNode() {}

QVector<Node::ParameterInfo> RiverNode::parameters() const {
    return {
        ParameterInfo("Scale", 0.0, 100.0, 5.0, 0.1, "Noise frequency"),
        ParameterInfo("Distortion", 0.0, 100.0, 20.0, 0.1, "Distortion strength"),
        ParameterInfo("Width", 0.001, 0.5, 0.02, 0.001, "River width"),
        ParameterInfo("Width Variation", 0.0, 1.0, 0.5, 0.01, "Width variation strength"),
        ParameterInfo("Attenuation", 0.0, 1.0, 0.0, 0.01, "Width tapering"),
        ParameterInfo("Source Count", 1.0, 100.0, 3.0, 1.0, "Source (Point 1) Count"),
        ParameterInfo("Points", 2.0, 500.0, 50.0, 1.0, "Points per river"),
        ParameterInfo("Seed", 0.0, 100.0, 0.0, 1.0, "Random seed"),
        ParameterInfo("Target Color", 0.0, 1.0, QColor(255, 255, 255)),
        ParameterInfo("Tolerance", 0.0, 1.0, 0.1, 0.01, "Source Color Tolerance"),
        ParameterInfo("Merge Distance", 0.0, 0.5, 0.15, 0.001, "Source Merge Distance"),
        ParameterInfo("Min Distance", 0.0, 0.5, 0.1, 0.01, "Min Separation between Rivers"),
        ParameterInfo("River Color", 0.0, 1.0, QColor(255, 255, 255)),
        ParameterInfo("Dest Color", 0.0, 1.0, QColor(255, 0, 0)),
        ParameterInfo("Dest Count", 1.0, 100.0, 3.0, 1.0, "Dest (Point 2) Count"),
        ParameterInfo("Dest Tolerance", 0.0, 1.0, 0.1, 0.01, "Dest Color Tolerance"),
        ParameterInfo("Dest Merge Dist", 0.0, 0.5, 0.15, 0.001, "Dest Merge Distance"),
        ParameterInfo("Map Size", 64.0, 4096.0, 512.0, 64.0, "Internal Map Resolution")
    };
}

double RiverNode::scale() const { return m_scaleInput->value().toDouble(); }
double RiverNode::distortionStrength() const { return m_distortionInput->value().toDouble(); }
double RiverNode::riverWidth() const { return m_widthInput->value().toDouble(); }
double RiverNode::widthVariation() const { return m_widthVariationInput->value().toDouble(); }
double RiverNode::attenuation() const { return m_attenuationInput->value().toDouble(); }
int RiverNode::riverCount() const { return static_cast<int>(m_countInput->value().toDouble()); }
int RiverNode::pointCount() const { return static_cast<int>(m_pointsInput->value().toDouble()); }
NoiseType RiverNode::noiseType() const { return m_noiseType; }
double RiverNode::seed() const { return m_seedInput->value().toDouble(); }
QColor RiverNode::targetColor() const { 
    if (!m_targetColorInput) return QColor(255, 255, 255);
    QVariant val = m_targetColorInput->value();
    if (!val.isValid() || !val.canConvert<QColor>()) return QColor(255, 255, 255);
    QColor c = val.value<QColor>();
    return c.isValid() ? c : QColor(255, 255, 255);
}
double RiverNode::tolerance() const { 
    if (!m_toleranceInput) return 0.1;
    double val = m_toleranceInput->value().toDouble();
    return (val > 0.0) ? val : 0.1;
}
double RiverNode::mergeDistance() const { return m_mergeDistanceInput->value().toDouble(); }
double RiverNode::minDistance() const { return m_minDistanceInput->value().toDouble(); }
QColor RiverNode::riverColor() const { 
    if (!m_riverColorInput) return QColor(255, 255, 255);
    QVariant val = m_riverColorInput->value();
    if (!val.isValid() || !val.canConvert<QColor>()) return QColor(255, 255, 255);
    QColor c = val.value<QColor>();
    return c.isValid() ? c : QColor(255, 255, 255);
}
bool RiverNode::edgeConnection() const { return m_edgeConnection; }
QColor RiverNode::destinationColor() const { 
    if (!m_destinationColorInput) return QColor(255, 0, 0);
    QVariant val = m_destinationColorInput->value();
    if (!val.isValid() || !val.canConvert<QColor>()) return QColor(255, 0, 0);
    QColor c = val.value<QColor>();
    return c.isValid() ? c : QColor(255, 0, 0);
}
int RiverNode::destCount() const { return static_cast<int>(m_destCountInput->value().toDouble()); }
double RiverNode::destTolerance() const { return m_destToleranceInput->value().toDouble(); }
double RiverNode::destMergeDistance() const { return m_destMergeDistanceInput->value().toDouble(); }
int RiverNode::mapSize() const { return static_cast<int>(m_mapSizeInput->value().toDouble()); }

void RiverNode::setScale(double v) { m_scaleInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setDistortionStrength(double v) { m_distortionInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setRiverWidth(double v) { m_widthInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setWidthVariation(double v) { m_widthVariationInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setAttenuation(double v) { m_attenuationInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setRiverCount(int v) { m_countInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setPointCount(int v) { m_pointsInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setNoiseType(NoiseType v) { m_noiseType = v; setDirty(true); m_isCached = false; }
void RiverNode::setSeed(double v) { m_seedInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setTargetColor(QColor c) { m_targetColorInput->setValue(c); setDirty(true); m_isCached = false; }
void RiverNode::setTolerance(double v) { m_toleranceInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setMergeDistance(double v) { m_mergeDistanceInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setMinDistance(double v) { m_minDistanceInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setRiverColor(QColor c) { m_riverColorInput->setValue(c); setDirty(true); m_isCached = false; }
void RiverNode::setEdgeConnection(bool v) { m_edgeConnection = v; setDirty(true); m_isCached = false; notifyStructureChanged(); }
void RiverNode::setDestinationColor(QColor c) { m_destinationColorInput->setValue(c); setDirty(true); m_isCached = false; }
void RiverNode::setDestCount(int v) { m_destCountInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setDestTolerance(double v) { m_destToleranceInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setDestMergeDistance(double v) { m_destMergeDistanceInput->setValue(v); setDirty(true); m_isCached = false; }
void RiverNode::setMapSize(int v) { 
    if (v > 4096) v = 4096;
    if (v < 64) v = 64;
    m_mapSizeInput->setValue(v); 
    setDirty(true); 
    m_isCached = false; 
}

void RiverNode::setDirty(bool dirty) {
    if (dirty) {
        m_isCached = false;
    }
    Node::setDirty(dirty);
}

void RiverNode::evaluate() {
    if (m_dirty) {
        m_isCached = false;
    }
}

// Ridged Multifractal Noise (Legacy Logic)
static double ridgedMultifractal(PerlinNoise* noise, double x, double y, double z, int octaves, double roughness, double gain) {
    double result = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double maxAmplitude = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        double n = noise->noise(x * frequency, y * frequency, z * frequency);
        // Ridged: 1 - abs(n) creates valleys
        n = 1.0 - std::fabs(n);
        // Square for sharper ridges
        n = n * n;
        // Apply gain
        n *= amplitude * gain;
        
        result += n;
        maxAmplitude += amplitude;
        
        amplitude *= roughness;
        frequency *= 2.0;
    }
    
    return result / (maxAmplitude * gain); // Normalize roughly to 0-1
}

void RiverNode::generateRiverMap() {
    int mapSize = this->mapSize();
    if (mapSize < 64) mapSize = 64;
    if (mapSize > 4096) mapSize = 4096; // Safety Cap
    
    m_cachedMap = QImage(mapSize, mapSize, QImage::Format_RGB32);
    
    // 0. Initialize Map with Water Mask (Composite)
    // OPTIMIZATION: Sample at lower resolution then scale up
    if (m_waterMaskInput->isConnected()) {
        int sampleSize = qMin(256, mapSize); // Sample at max 256x256
        QImage sampledMap(sampleSize, sampleSize, QImage::Format_RGB32);
        
        int renderW = AppSettings::instance().renderWidth();
        int renderH = AppSettings::instance().renderHeight();
        
        for (int y = 0; y < sampleSize; ++y) {
            QRgb* scanLine = (QRgb*)sampledMap.scanLine(y);
            for (int x = 0; x < sampleSize; ++x) {
                double u = static_cast<double>(x) / sampleSize;
                double v = static_cast<double>(y) / sampleSize;
                QVector3D pos(u * renderW, v * renderH, 0.0);
                QVariant val = m_waterMaskInput->getValue(pos);
                QColor c;
                if (val.canConvert<QColor>()) {
                    c = val.value<QColor>();
                } else {
                    double gray = val.toDouble();
                    int g = static_cast<int>(gray * 255);
                    c = QColor(g, g, g);
                }
                scanLine[x] = c.rgba();
            }
        }
        // Scale up to mapSize
        m_cachedMap = sampledMap.scaled(mapSize, mapSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        m_cachedMap.fill(Qt::black);
    }

    QVector<QPointF> sourcePoints;
    QVector<QPointF> destPoints;
    int sampleRes = 128; 
    
    QRandomGenerator rng(static_cast<quint32>(seed() * 1000));

    // --- 1. Generate Point 1 (Source) ---
    if (m_waterMaskInput->isConnected()) {
        QColor target = targetColor();
        double tol = tolerance();

        // Optimization: Use a spatial grid for merging
        double mergeDist = mergeDistance();
        if (mergeDist < 0.001) mergeDist = 0.001; // Avoid division by zero
        
        int gridDim = static_cast<int>(1.0 / mergeDist) + 1;
        QVector<QVector<QList<QPointF>>> grid(gridDim, QVector<QList<QPointF>>(gridDim));
        
        int candidatesCount = 0;

        for (int y = 0; y < sampleRes; ++y) {
            for (int x = 0; x < sampleRes; ++x) {
                double u = static_cast<double>(x) / sampleRes;
                double v = static_cast<double>(y) / sampleRes;
                
                // Convert UV to Pixel Coordinates for upstream sampling
                int renderW = AppSettings::instance().renderWidth();
                int renderH = AppSettings::instance().renderHeight();
                QVector3D pos(u * renderW, v * renderH, 0.0);
                
                QVariant val = m_waterMaskInput->getValue(pos);
                QColor c;
                if (val.canConvert<QColor>()) {
                    c = val.value<QColor>();
                } else {
                    double gray = val.toDouble();
                    c = QColor::fromRgbF(gray, gray, gray);
                }
                
                double distSource = std::sqrt(std::pow(c.redF() - target.redF(), 2) + 
                                              std::pow(c.greenF() - target.greenF(), 2) + 
                                              std::pow(c.blueF() - target.blueF(), 2));
                
                if (distSource <= tol) {
                    candidatesCount++;
                    QPointF p(u, v);
                    
                    // Check neighbors in grid
                    int gx = static_cast<int>(u / mergeDist);
                    int gy = static_cast<int>(v / mergeDist);
                    
                    bool merged = false;
                    
                    // Check 3x3 neighborhood
                    for (int ny = std::max(0, gy - 1); ny <= std::min(gridDim - 1, gy + 1); ++ny) {
                        for (int nx = std::max(0, gx - 1); nx <= std::min(gridDim - 1, gx + 1); ++nx) {
                            for (const QPointF& existing : grid[ny][nx]) {
                                double dist = std::sqrt(std::pow(p.x() - existing.x(), 2) + std::pow(p.y() - existing.y(), 2));
                                if (dist < mergeDist) {
                                    merged = true;
                                    goto end_check_source; // Break out of all loops
                                }
                            }
                        }
                    }
                    end_check_source:
                    
                    if (!merged) {
                        if (gx >= 0 && gx < gridDim && gy >= 0 && gy < gridDim) {
                            grid[gy][gx].append(p);
                            sourcePoints.append(p);
                        }
                    }
                }
            }
        }
        
    } else {
        // Fallback: Random Points (Source Count)
        int count = riverCount();
        double minDist = minDistance();
        int attempts = 0;
        int maxAttempts = count * 50;
        
        while (sourcePoints.size() < count && attempts < maxAttempts) {
            attempts++;
            QPointF p(rng.generateDouble(), rng.generateDouble());
            
            // Check distance against existing points
            bool tooClose = false;
            for (const QPointF& existing : sourcePoints) {
                if (QVector2D(p - existing).length() < minDist) {
                    tooClose = true;
                    break;
                }
            }
            
            if (!tooClose) {
                sourcePoints.append(p);
            }
        }
    }

    // Fallback if connected but no candidates found (e.g. color mismatch)
    if (sourcePoints.isEmpty()) {
        // Fallback: No candidates found
        int count = std::max(1, riverCount());
        double minDist = minDistance();
        int attempts = 0;
        int maxAttempts = count * 50;
        
        while (sourcePoints.size() < count && attempts < maxAttempts) {
            attempts++;
            QPointF p(rng.generateDouble(), rng.generateDouble());
            
            bool tooClose = false;
            for (const QPointF& existing : sourcePoints) {
                if (QVector2D(p - existing).length() < minDist) {
                    tooClose = true;
                    break;
                }
            }
            
            if (!tooClose) {
                sourcePoints.append(p);
            }
        }
    }

    // Limit Source Points
    int maxSources = std::max(0, riverCount()); // Ensure non-negative
    if (sourcePoints.size() > maxSources) {
        // Shuffle and pick first N
        // Note: Shuffling might violate min distance if we picked from a larger set, 
        // but here we generated sequentially with checks, so subset is also valid.
        for (int i = 0; i < sourcePoints.size(); ++i) {
            int j = rng.bounded(sourcePoints.size());
            std::swap(sourcePoints[i], sourcePoints[j]);
        }
        sourcePoints.resize(maxSources);
    }
    


    // --- 2. Generate Point 2 (Destination) ---
    int maxDest = destCount();
    
    if (m_edgeConnection) {
        // Edge Mode
        for (int i = 0; i < maxDest; ++i) {
            int edge = rng.bounded(4);
            double r = rng.generateDouble();
            QPointF p;
            switch (edge) {
                case 0: p = QPointF(r, 0.0); break; // Top
                case 1: p = QPointF(1.0, r); break; // Right
                case 2: p = QPointF(r, 1.0); break; // Bottom
                case 3: p = QPointF(0.0, r); break; // Left
            }
            destPoints.append(p);
        }
    } else {
        // Color Mode
        if (m_waterMaskInput->isConnected()) {
            QColor destTarget = destinationColor();
            double destTol = destTolerance();
            
            // Optimization: Spatial Grid for Dest Points
            double destMergeDist = destMergeDistance();
            if (destMergeDist < 0.001) destMergeDist = 0.001;
            
            int gridDim = static_cast<int>(1.0 / destMergeDist) + 1;
            QVector<QVector<QList<QPointF>>> grid(gridDim, QVector<QList<QPointF>>(gridDim));
            
            int candidatesCount = 0;
            
            for (int y = 0; y < sampleRes; ++y) {
                for (int x = 0; x < sampleRes; ++x) {
                    double u = static_cast<double>(x) / sampleRes;
                    double v = static_cast<double>(y) / sampleRes;
                    
                    // Convert UV to Pixel Coordinates
                    int renderW = AppSettings::instance().renderWidth();
                    int renderH = AppSettings::instance().renderHeight();
                    QVector3D pos(u * renderW, v * renderH, 0.0);
                    
                    QVariant val = m_waterMaskInput->getValue(pos);
                    QColor c;
                    if (val.canConvert<QColor>()) {
                        c = val.value<QColor>();
                    } else {
                        double gray = val.toDouble();
                        c = QColor::fromRgbF(gray, gray, gray);
                    }
                    
                    double distDest = std::sqrt(std::pow(c.redF() - destTarget.redF(), 2) + 
                                                std::pow(c.greenF() - destTarget.greenF(), 2) + 
                                                std::pow(c.blueF() - destTarget.blueF(), 2));
                    
                    if (distDest <= destTol) {
                        candidatesCount++;
                        QPointF p(u, v);
                        
                        // Check neighbors
                        int gx = static_cast<int>(u / destMergeDist);
                        int gy = static_cast<int>(v / destMergeDist);
                        
                        bool merged = false;
                        for (int ny = std::max(0, gy - 1); ny <= std::min(gridDim - 1, gy + 1); ++ny) {
                            for (int nx = std::max(0, gx - 1); nx <= std::min(gridDim - 1, gx + 1); ++nx) {
                                for (const QPointF& existing : grid[ny][nx]) {
                                    double dist = std::sqrt(std::pow(p.x() - existing.x(), 2) + std::pow(p.y() - existing.y(), 2));
                                    if (dist < destMergeDist) {
                                        merged = true;
                                        goto end_check_dest;
                                    }
                                }
                            }
                        }
                        end_check_dest:
                        
                        if (!merged) {
                            if (gx >= 0 && gx < gridDim && gy >= 0 && gy < gridDim) {
                                grid[gy][gx].append(p);
                                destPoints.append(p);
                            }
                        }
                    }
                }
            }


        }
        
        // Fallback if no dest points found in color mode
        if (destPoints.isEmpty()) {
            if (m_waterMaskInput->isConnected()) {
                 // Connected but no match -> Fallback to edges
                 // Fallback to edges
                for (int i = 0; i < maxDest; ++i) {
                    int edge = rng.bounded(4);
                    double r = rng.generateDouble();
                    QPointF p;
                    switch (edge) {
                        case 0: p = QPointF(r, 0.0); break;
                        case 1: p = QPointF(1.0, r); break;
                        case 2: p = QPointF(r, 1.0); break;
                        case 3: p = QPointF(0.0, r); break;
                    }
                    destPoints.append(p);
                }
            } else {
                // Not connected -> Random points
                // Random dest points
                for(int i=0; i<maxDest; ++i) {
                    destPoints.append(QPointF(rng.generateDouble(), rng.generateDouble()));
                }
            }
        }
    }

    // Limit Dest Points
    if (destPoints.size() > maxDest) {
        for (int i = 0; i < destPoints.size(); ++i) {
            int j = rng.bounded(destPoints.size());
            std::swap(destPoints[i], destPoints[j]);
        }
        destPoints.resize(maxDest);
    }

    // --- 3. Connect and Draw ---
    QPainter painter(&m_cachedMap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(riverColor());

    double baseWidth = riverWidth() * mapSize;
    double variationStrength = widthVariation();
    double atten = attenuation();
    int points = pointCount();
    double distortion = distortionStrength();
    double scaleVal = scale();
    double seedVal = seed();
    NoiseType type = noiseType();

    if (destPoints.isEmpty()) return;

    for (const QPointF& start : sourcePoints) {
        // Find closest destination
        QPointF end;
        double minDist = std::numeric_limits<double>::max();
        
        for (const QPointF& d : destPoints) {
            double dist = QVector2D(d - start).length();
            if (dist < minDist) {
                minDist = dist;
                end = d;
            }
        }
        
        // Draw River
        QPointF currentPos = start * mapSize;
        
        for (int j = 1; j <= points; ++j) {
            double t = static_cast<double>(j) / points;
            
            // Linear interpolation
            double lx = start.x() + (end.x() - start.x()) * t;
            double ly = start.y() + (end.y() - start.y()) * t;
            
            // Apply Noise Distortion
            double nx = 0, ny = 0;
            double noiseInputX = lx * scaleVal + seedVal;
            double noiseInputY = ly * scaleVal + seedVal;
            double noiseInputZ = 0.0;

            if (type == NoiseType::Ridged) {
                nx = ridgedMultifractal(m_noise.get(), noiseInputX, noiseInputY, noiseInputZ, 4, 0.5, 1.0);
                ny = ridgedMultifractal(m_noise.get(), noiseInputX + 100.0, noiseInputY + 100.0, noiseInputZ, 4, 0.5, 1.0);
                nx = (nx - 0.5) * 2.0;
                ny = (ny - 0.5) * 2.0;
            } else {
                nx = m_noise->noise(noiseInputX, noiseInputY, noiseInputZ);
                ny = m_noise->noise(noiseInputX + 100.0, noiseInputY + 100.0, noiseInputZ);
            }
            
            double distortionEnvelope = std::sin(t * 3.14159); // Pin both ends
            
            double dx = nx * distortion * 0.01 * distortionEnvelope; 
            double dy = ny * distortion * 0.01 * distortionEnvelope;
            
            QPointF nextPos((lx + dx) * mapSize, (ly + dy) * mapSize);
            
            // Calculate variable width with Attenuation
            double widthNoise = m_noise->noise(lx * 10.0, ly * 10.0, seedVal + 50.0);
            double taper = 1.0 - (t * atten);
            double currentWidth = baseWidth * taper * (1.0 + widthNoise * variationStrength);
            
            // Draw segment
            double dist = QVector2D(nextPos - currentPos).length();
            int steps = std::max(1, static_cast<int>(dist / (currentWidth * 0.25))); 
            
            for (int k = 0; k < steps; ++k) {
                double subT = static_cast<double>(k) / steps;
                QPointF p = currentPos + (nextPos - currentPos) * subT;
                painter.drawEllipse(p, currentWidth * 0.5, currentWidth * 0.5);
            }
            
            currentPos = nextPos;
        }
    }
    
    m_isCached = true;
    m_dirty = false;
}

QVariant RiverNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // Check if inputs have changed or if we need to re-generate
    if (m_dirty || !m_isCached) {
        QMutexLocker locker(&m_mutex);
        if (m_dirty || !m_isCached) {
            generateRiverMap();
        }
    }

    int mapSize = this->mapSize();
    if (mapSize < 64) mapSize = 64;

    QVector3D p;
    if (m_vectorInput->isConnected()) {
        p = m_vectorInput->getValue(pos).value<QVector3D>();
    } else {
        // Normalize based on Render Resolution, NOT Map Size
        int w = AppSettings::instance().renderWidth();
        int h = AppSettings::instance().renderHeight();
        double u = pos.x() / static_cast<double>(w);
        double v = pos.y() / static_cast<double>(h);
        p = QVector3D(u, v, 0.0);
    }
    
    int x = static_cast<int>(p.x() * m_cachedMap.width());
    int y = static_cast<int>(p.y() * m_cachedMap.height());
    
    // Clip coordinates (No Repeat)
    if (x < 0 || x >= m_cachedMap.width() || y < 0 || y >= m_cachedMap.height()) {
        if (socket == m_facOutput) return 0.0;
        if (socket == m_colorOutput) return QColor(0, 0, 0, 0); // Transparent
        return QVariant();
    }
    
    QRgb pixel = m_cachedMap.pixel(x, y);
    double val = qRed(pixel) / 255.0;
    
    if (socket == m_facOutput) {
        return val;
    } else if (socket == m_colorOutput) {
        return QColor(pixel);
    }
    
    return QVariant();
}

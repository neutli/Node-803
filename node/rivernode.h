#ifndef RIVERNODE_H
#define RIVERNODE_H

#include "node.h"
#include "noise.h"
#include <memory>
#include <QImage>
#include <QRecursiveMutex>

class RiverNode : public Node {
public:
    RiverNode();
    virtual ~RiverNode();

    void generateRiverMap();
    
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    void setDirty(bool dirty) override;
    QVector<ParameterInfo> parameters() const override;

    // Getters and Setters
    double scale() const;
    void setScale(double v);
    
    double distortionStrength() const;
    void setDistortionStrength(double v);
    
    double riverWidth() const;
    void setRiverWidth(double v);
    
    double widthVariation() const;
    void setWidthVariation(double v);
    
    int riverCount() const;
    void setRiverCount(int v);
    
    int pointCount() const;
    void setPointCount(int v);
    
    double seed() const;
    void setSeed(double v);
    
    double attenuation() const;
    void setAttenuation(double v);
    
    QColor targetColor() const;
    void setTargetColor(QColor c);
    
    double tolerance() const;
    void setTolerance(double v);

    double mergeDistance() const;
    void setMergeDistance(double v);

    double minDistance() const;
    void setMinDistance(double v);

    QColor riverColor() const;
    void setRiverColor(QColor c);

    // Point 2 (Destination) Settings
    int destCount() const;
    void setDestCount(int v);

    double destTolerance() const;
    void setDestTolerance(double v);

    double destMergeDistance() const;
    void setDestMergeDistance(double v);
    
    NoiseType noiseType() const;
    void setNoiseType(NoiseType v);

    int mapSize() const;
    void setMapSize(int v);

    std::unique_ptr<PerlinNoise> m_noise;
    
    // Caching
    QImage m_cachedMap;
    bool m_isCached;
    QRecursiveMutex m_mutex;

    NodeSocket* m_vectorInput;
    NodeSocket* m_waterMaskInput; // Defines water source regions
    
    NodeSocket* m_scaleInput;
    NodeSocket* m_distortionInput; 
    NodeSocket* m_widthInput;
    NodeSocket* m_widthVariationInput;
    NodeSocket* m_attenuationInput; // Width tapering
    NodeSocket* m_countInput;      
    NodeSocket* m_pointsInput;     
    NodeSocket* m_seedInput;
    
    NodeSocket* m_targetColorInput; // Color to spawn rivers from
    NodeSocket* m_toleranceInput;   // Color matching tolerance
    NodeSocket* m_mergeDistanceInput; // Minimum distance between sources
    NodeSocket* m_minDistanceInput; // Minimum separation between rivers
    NodeSocket* m_riverColorInput; // Color of the river itself
    
    NodeSocket* m_destinationColorInput; // Color to flow towards (if not edge connection)
    NodeSocket* m_destCountInput;
    NodeSocket* m_destToleranceInput;
    NodeSocket* m_destMergeDistanceInput;
    NodeSocket* m_mapSizeInput;
    
    // Properties
    bool edgeConnection() const;
    void setEdgeConnection(bool v);
    
    QColor destinationColor() const;
    void setDestinationColor(QColor c);

    NodeSocket* m_facOutput;
    NodeSocket* m_colorOutput;
    
    // Internal state for parameters
    NoiseType m_noiseType;
    bool m_edgeConnection;
};

#endif // RIVERNODE_H

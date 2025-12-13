#include "noderegistry.h"
#include "mathnode.h"
#include "vectormathnode.h"
#include "noisetexturenode.h"
#include "rivernode.h"
#include "watersourcenode.h"
#include "invertnode.h"
#include "voronoinode.h"
#include "mappingnode.h"
#include "texturecoordinatenode.h"
#include "outputnode.h"
#include "colorrampnode.h"
#include "mixnode.h"
#include "bumpnode.h"
#include "maprangenode.h"
#include "principledbsdfnode.h"
#include "mixshadernode.h"
#include "imagetexturenode.h"

#include "separatexyznode.h"
#include "combinexyznode.h"
#include "clampnode.h"
#include "wavetexturenode.h"
#include "bricktexturenode.h"
#include "radialtilingnode.h"
#include "calculusnode.h"
#include "gabortexturenode.h"
#include "everlingtexturenode.h"
#include "polygonnode.h"
#include "pointcreatenode.h"
#include "scatteronpointsnode.h"
#include "colorkeynode.h"

NodeRegistry& NodeRegistry::instance() {
    static NodeRegistry instance;
    return instance;
}

void NodeRegistry::registerNode(const QString& category, const QString& name, NodeFactory factory) {
    NodeRegistration reg;
    reg.name = name;
    reg.category = category;
    reg.factory = factory;
    
    m_nodes[name] = reg;
    
    if (!m_categories.contains(category)) {
        m_categories[category] = QStringList();
    }
    if (!m_categories[category].contains(name)) {
        m_categories[category].append(name);
    }
}

Node* NodeRegistry::createNode(const QString& name) {
    if (m_nodes.contains(name)) {
        return m_nodes[name].factory();
    }
    return nullptr;
}

QStringList NodeRegistry::getCategories() const {
    return m_categories.keys();
}

QStringList NodeRegistry::getNodesByCategory(const QString& category) const {
    if (m_categories.contains(category)) {
        return m_categories[category];
    }
    return QStringList();
}

void NodeRegistry::registerNodes() {
    registerNode("Math", "Math", []() { return new MathNode(); });
    registerNode("Vector", "Vector Math", []() { return new VectorMathNode(); });
    registerNode("Texture", "Noise Texture", []() { return new NoiseTextureNode(); });
    registerNode("Texture", "River Texture", []() { return new RiverNode(); });
    registerNode("Texture", "Water Source", []() { return new WaterSourceNode(); });
    registerNode("Color", "Invert", []() { return new InvertNode(); });
    registerNode("Texture", "Voronoi Texture", []() { return new VoronoiNode(); });
    registerNode("Vector", "Mapping", []() { return new MappingNode(); });
    registerNode("Input", "Texture Coordinate", []() { return new TextureCoordinateNode(); });
    registerNode("Output", "Material Output", []() { return new OutputNode(); });
    
    // New Blender-style nodes
    registerNode("Converter", "Color Ramp", []() { return new ColorRampNode(); });
    registerNode("Color", "Mix", []() { return new MixNode(); });
    registerNode("Vector", "Bump", []() { return new BumpNode(); });
    registerNode("Converter", "Map Range", []() { return new MapRangeNode(); });
    registerNode("Shader", "Principled BSDF", []() { return new PrincipledBSDFNode(); });
    registerNode("Shader", "Mix Shader", []() { return new MixShaderNode(); });
    registerNode("Texture", "Image Texture", []() { return new ImageTextureNode(); });

    // New nodes from user request
    registerNode("Converter", "Separate XYZ", []() { return new SeparateXYZNode(); });
    registerNode("Converter", "Combine XYZ", []() { return new CombineXYZNode(); });
    registerNode("Converter", "Clamp", []() { return new ClampNode(); });
    registerNode("Texture", "Wave Texture", []() { return new WaveTextureNode(); });
    registerNode("Texture", "Brick Texture", []() { return new BrickTextureNode(); });
    registerNode("Texture", "Radial Tiling", []() { return new RadialTilingNode(); });
    registerNode("Converter", "Calculus", []() { return new CalculusNode(); });
    registerNode("Texture", "Gabor Texture", []() { return new GaborTextureNode(); });
    registerNode("Texture", "Everling Texture", []() { return new EverlingTextureNode(); });
    
    // Geometry nodes
    registerNode("Geometry", "Polygon", []() { return new PolygonNode(); });
    registerNode("Geometry", "Point Create", []() { return new PointCreateNode(); });
    registerNode("Geometry", "Scatter on Points", []() { return new ScatterOnPointsNode(); });
    registerNode("Color", "Color Key", []() { return new ColorKeyNode(); });
}

# Node Editor Project - Developer Guide

This document provides a comprehensive overview of the Node Editor project, including architecture, API, node descriptions, and implementation details for developers.

## 1. Project Structure & Architecture

The project is a node-based editor built with Qt (C++). It allows users to create procedural textures and materials by connecting various nodes.

### Core Components

*   **`Node` (Base Class)** (`node.h` / `node.cpp`)
    *   The abstract base class for all nodes.
    *   Manages input/output sockets, parameters, and computation logic.
    *   **Key Methods:**
        *   `compute(pos, socket)`: Calculates the output value for a given position and socket.
        *   `evaluate()`: (Optional) Pre-computation step.
        *   `parameters()`: Defines parameters for UI generation.
        *   `save()` / `restore()`: JSON serialization.

*   **`NodeSocket`** (`node.h`)
    *   Represents connection points on a node.
    *   Types: `Float`, `Vector`, `Color`, `Integer`, `Shader`.
    *   Directions: `Input`, `Output`.

*   **`NodeRegistry`** (`noderegistry.cpp`)
    *   Singleton that manages available node types.
    *   Registers nodes with a category and a factory function.

*   **`NodeEditorWidget`** (`nodeeditorwidget.h` / `.cpp`)
    *   The main visual editor widget (inherits `QGraphicsView`).
    *   Manages the `QGraphicsScene`, nodes, connections, and interactions (panning, zooming).

*   **`NodeGraphicsItem`** (`nodegraphicsitem.cpp`)
    *   Visual representation of a `Node` in the scene.
    *   Handles rendering, socket positions, and embedded widgets (spinboxes, combo boxes, custom widgets).
    *   **`updateLayout()`**: Crucial method that rebuilds the UI of the node when parameters change.

### Data Flow

1.  **Rendering**: When `OutputViewerWidget` needs to render, it iterates through pixels.
2.  **Sampling**: For each pixel (coordinate `pos`), it calls `compute(pos)` on the final output node.
3.  **Recursion**: Nodes recursively call `getValue(pos)` on their input sockets, which triggers `compute(pos)` on connected upstream nodes.
4.  **Parameters**: If a socket is not connected, it uses a default value or a value from a UI widget.

## 2. API & Implementation

### Creating a New Node

To add a new node, follow these steps:

1.  **Create Helper Class**: Inherit from `Node`.
    ```cpp
    class MyNode : public Node {
    public:
        MyNode(); // Define inputs/outputs here
        QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    };
    ```

2.  **Define Inputs/Outputs (Constructor)**:
    ```cpp
    MyNode::MyNode() : Node("My Node Name") {
        addInputSocket(new NodeSocket("Input 1", SocketType::Float, SocketDirection::Input, this));
        addOutputSocket(new NodeSocket("Output", SocketType::Float, SocketDirection::Output, this));
    }
    ```

3.  **Implement Logic**:
    ```cpp
    QVariant MyNode::compute(const QVector3D& pos, NodeSocket* socket) {
        // Get input values (recursive)
        double input = m_inputSocket->getValue(pos).toDouble();
        
        // Process
        double result = input * 2.0;

        // Return result
        return result;
    }
    ```

4.  **Register Node**:
    Add to `NodeRegistry::registerNodes()` in `noderegistry.cpp`:
    ```cpp
    registerNode("Category", "My Node Name", []() { return new MyNode(); });
    ```

### Custom UI Widgets

For nodes requiring complex UI (like Color Ramp or Curves):
1.  Create a custom `QWidget` (e.g., `WaterSourceRampWidget`).
2.  In `NodeGraphicsItem::updateLayout()`, check for the specific node type using `dynamic_cast`.
3.  Instantiate the widget and wrap it in a `QGraphicsProxyWidget` to embed it in the scene.

## 3. Available Nodes

### Texture Nodes
*   **Noise Texture**: Generates 3D Perlin/OpenSimplex noise.
*   **Voronoi Texture**: Generates cell-based patterns.
*   **River Texture**: Simulates river flow paths.
*   **Water Source**: Generates a lake/water source shape with distortion and color ramp.
*   **Image Texture**: Loads and samples an external image file.
*   **Wave Texture**: Generates sine/saw/triangular wave bands.
*   **Brick Texture**: Generates brick patterns with customizable offset and mortar.
*   **Radial Tiling**: Angular repetition pattern.

### Math / Converter Nodes
*   **Math**: Basic arithmetic (Add, Sub, Mul, Div) and functions (Sin, Cos, Power, Greater Than, etc.).
*   **Vector Math**: Operations on 3D vectors (Dot, Cross, Reflect, Length, etc.).
*   **Clamp**: Restricts values to a range.
*   **Map Range**: Remaps a value from one range to another (Linear, Stepped, Smoothstep).
*   **Color Ramp**: Maps a value (0-1) to a color gradient.
*   **Separate/Combine XYZ**: Splits or merges vector components.
*   **Invert**: Inverts a color or float.
*   **Mix**: Mixes two colors/floats/vectors based on a factor.

### Inputs / Outputs
*   **Texture Coordinate**: Provides geometric coordinates (Generated, Normal, UV, Object).
*   **Mapping**: Transforms coordinates (Location, Rotation, Scale).
*   **Material Output**: The final node that outputs the result to the viewer.

### Shaders (Experimental)
*   **Principled BSDF**: PBR shader node.
*   **Mix Shader**: Mixes two shader closures.

## 4. Key Files & Responsibilities

| File | Responsibility |
|------|----------------|
| `node.h/cpp` | Base class for logic and connectivity. |
| `nodegraphicsitem.cpp` | **Visuals**. Handles node drawing, sockets, and **UI widgets**. |
| `nodeeditorwidget.cpp` | **Editor Interaction**. Handles mouse events, connections, and scene management. |
| `noderegistry.cpp` | **Registration**. Central place to register new nodes. |
| `mainwindow.cpp` | **App Shell**. Menus, toolbars, and global settings. |
| `watersourcenode.cpp` | Example of a **complex node** with custom logic and parameters. |
| `noise.cpp` | Implementation of Perlin and OpenSimplex noise algorithms. |

## 5. Basic Operations for Users

*   **Add Node**: Right-click background -> Select from menu.
*   **Connect**: Drag from an Output socket to an Input socket.
*   **Disconnect**: Drag from a connected Input socket to empty space, or Alt-click connection (if implemented).
*   **Delete**: Select node(s) and press `Delete` key.
*   **Pan**: Middle-mouse drag (or Alt+Left drag).
*   **Zoom**: Mouse wheel.
*   **Parameters**: Adjust values directly on the node widgets (Spinboxes, Color buttons).

## 6. Implementation Notes for Developers

*   **Thread Safety**: `compute()` is called from multiple threads during rendering. Ensure `m_noise` or shared resources are thread-safe or stateless.
*   **Coordinates**: Texture space is usually 0..1 or -1..1 depending on the node. Be consistent with coordinate spaces.
*   **UI Updates**: If you change node structure (add/remove inputs dynamically), verify `updateLayout()` handles it correctly.
*   **Memory**: Nodes manage their own sockets. The Registry manages creation. The Editor manages deletion context.

## 7. Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Shift+A` / `Tab` | Open node search menu |
| `Shift+Q` | Open category menu |
| `Delete` / `Backspace` | Delete selected nodes/connections |
| `R` | Reset selected nodes to default state |
| `S` | Toggle node scale (100% ↔ 75%) |
| `C` | Connect two selected nodes (auto-detect sockets) |
| `Ctrl+Shift+Click` | Quick connect node to Material Output |
| `Space + Drag` | Pan view |
| `Middle Mouse` | Pan view |
| `Scroll` | Zoom |

## 8. Quick Socket Connection (Shift+Click / Ctrl+Click)

A fast way to connect sockets without dragging:

### Usage
1. **Shift+ソケットクリック** - Select the first socket (a **yellow ring** appears)
2. **Ctrl+ソケットクリック** - Connect to the second socket

### Features
- **Direction auto-detection**: Output→Input or Input→Output (order doesn't matter)
- **Auto-disconnect**: Existing connections on the target input are automatically removed
- **Visual feedback**: Selected socket displays a yellow highlight ring
- **25px hit radius**: Easier to click small sockets

### Technical Details
- Implementation: `NodeEditorWidget::mousePressEvent()` (lines 283-355)
- Socket detection: `NodeEditorWidget::socketAt()` with expanded search radius
- Visual highlight: `NodeGraphicsSocket::setHighlighted()` and `m_highlighted` flag

## 9. Gabor Texture Node

A dedicated node for Gabor noise generation with phase and intensity outputs.

### Parameters
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Scale** | 0.01-100 | Overall texture scale |
| **Frequency** | 0.1-20 | Wave carrier frequency |
| **Anisotropy** | 0-1 | 0=isotropic (circular), 1=highly directional |
| **Orientation** | 3D Vector | Wave direction (X, Y, Z) |
| **Distortion** | 0-10 | Domain warping amount |

### Outputs
| Output | Description |
|--------|-------------|
| **Value** | Main Gabor noise value (0-1) |
| **Phase** | Phase component (0-1, normalized from -π to π) |
| **Intensity** | Signal intensity (magnitude of complex result) |

### Technical Details
- Implementation: `gabortexturenode.cpp`
- Uses complex Gabor kernel: `cos(arg) + i*sin(arg)`
- Phase/Intensity calculated from real/imaginary components

## 10. Noise Texture Gabor Mode Enhancements

The Noise Texture node's Gabor mode now provides additional outputs:

### New Outputs (Gabor mode only)
- **Phase**: Phase component of Gabor noise
- **Intensity**: Signal intensity

### Parameter Mapping (Gabor mode)
| UI Parameter | Gabor Parameter |
|--------------|-----------------|
| Lacunarity | Frequency |
| Detail / 10 | Anisotropy (0-1) |
| Roughness × 2π | Orientation angle |

## 11. Node Reset Functionality

Press `R` to reset selected nodes to their default state.

### Behavior
- All parameters return to default values
- UI widgets are rebuilt via `updateLayout()`
- Mode selections (e.g., Noise Type) reset to default

### Technical Details
- Creates a temporary default node instance
- Copies default JSON state to selected node
- Calls `updateLayout()` to refresh parameter widgets

## 12. Input Socket Widget Layout Rules

### Design Principle
Input sockets and their value widgets (sliders, spinboxes) are ALWAYS displayed as a unified pair.

### Layout Structure
```
● Socket Name ───────────────────
     [══════slider══════] [value]
```

### Rules
1. **Socket Label First**: The socket name is displayed on the same line as the socket dot
2. **Widget Below**: The input widget (SliderSpinBox) is placed DIRECTLY below the socket name
3. **Aligned Left**: Widget is left-aligned with the node body (not the socket dot)
4. **Full Width**: Widget spans the full width of the node minus margins
5. **Connected = Hidden**: When a socket is connected, its widget is hidden (value comes from connection)

### Implementation
- Located in `NodeGraphicsItem::updateLayout()` (lines 762-920)
- Each input socket iteration:
  1. Draw socket dot and label
  2. Increment yPos for label height (~20px)
  3. If parameter exists for socket and not connected: draw widget
  4. Increment yPos for widget height (~30px)
  5. Add socket spacing

### Parameter Matching
- Widget is shown only if `parameters()` contains a `ParameterInfo` with matching `name`
## 13. Recent Feature Additions (v2.0)

### New Nodes
*   **Polygon Node**:
    *   Generates regular or irregular polygon shapes.
    *   **Parameters**:
        *   `Sides` (Float): Interpolates shape between integer side counts.
        *   `Seed` (Int): If 0, produces regular polygons. If > 0, randomizes vertex radii for irregular shapes.
*   **Point Create Node**:
    *   Generates a point distribution map.
    *   **Modes**: Grid, Random, Poisson Disc.
*   **Scatter on Points Node**:
    *   Instances a texture input onto points generated by the Point Create node.
*   **Color Key Node**:
    *   Makes specific colors transparent based on a key color and tolerance.
    *   Supports proper Alpha channel output.

### Application Features
*   **Bulk Node Addition**:
    *   **Menu**: `File > Add Multiple Nodes...` (Ctrl+Shift+A).
    *   **Dialog**: Allows multi-selection of nodes to add simultaneously.
*   **Bulk Image Import**:
    *   **Drag & Drop**: Drag multiple image files from OS onto the editor.
    *   **Menu**: `File > Add Multiple Images`.
*   **Material Management**:
    *   **Default Graphs**:
        *   **Startup**: Complex River Graph.
        *   **New Material**: Simple Graph (Image + Principled BSDF).
    *   **Unique Naming**: Automatically ensures new material names (e.g., `Material.5`) do not collide with existing files.

### Developer Components
*   **PopupAwareComboBox** (`uicomponents.h`):
    *   Custom `QComboBox` subclass that works correctly within `QGraphicsScene` / `QGraphicsView`.
    *   Uses a `QEvent` filter to handle mouse clicks and popup display, solving issues where standard combo boxes fail to open in graphics items.
    *   **Usage**: Use this instead of `QComboBox` for node widgets overlaying the canvas.
*   **WheelEventFilter** (`uicomponents.h`):
    *   Prevents scroll events on spinboxes/combos from propagating to the view (zooming) when the widget is focused.

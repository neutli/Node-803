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

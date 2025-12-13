# Node Editor Project - Comprehensive Developer & User Guide

## 🌟 Introduction & Philosophy

**The ultimate goal of this project is to procedurally generate traditional villages, beautiful nature, and cities.**

Inspired by **Blender's Node Editor** and standard industry procedural tools like Houdini and Substance Designer, this application provides a robust nodegraph interface for creating complex procedural textures, materials, and eventually geometry. It serves as the foundational "Material Editor" for a larger procedural generation system.

### Core Design Principles
1.  **Non-Destructive Workflow**: All operations are node-based, allowing for infinite iteration without data loss.
2.  **Stateless Computation**: The rendering pipeline (`compute()`) is designed to be functional and stateless per-pixel, allowing for massive parallelism. All node state is immutable during a render pass.
3.  **Blender-Familiarity**: Terminology, hotkeys, and node behaviors mimic Blender where possible to lower the learning curve for 3D artists.
4.  **Extensibility**: The system is designed to allow adding new nodes with minimal boilerplate (Subclass `Node` -> Register in `NodeRegistry`).

---

## 🛠️ Build & Installation Guide

This project is built using **Qt 6** and **CMake**. It manages dependencies using CMake and does not require Qt Creator, although it can be used.

### Prerequisites

| Component | Requirement | Notes |
|-----------|-------------|-------|
| **Qt Framework** | **6.2+** | Tested on Qt 6.8.0. Requires `Qt6::Widgets`, `Qt6::Gui`, `Qt6::Concurrent`. |
| **CMake** | **3.16+** | Build system generator. |
| **Compiler** | **C++17** | MSVC 2019+, GCC 9+, or Clang 10+. |

### 🚀 Building with Visual Studio Code (Recommended)
1.  **Prepare Environment**: Ensure Qt 6 bin path is in your implementation or defined in CMake kits.
2.  **Install Extensions**:
    *   C/C++ (Microsoft)
    *   CMake Tools (Microsoft)
3.  **Open Project**: Open the `node` directory (containing `CMakeLists.txt`) in VS Code.
4.  **Configure**:
    *   Open Command Palette (`Ctrl+Shift+P`).
    *   Run `CMake: Scan for Kits`.
    *   Select your compiler kit (e.g., `Visual Studio Community 2022 Release - amd64`).
    *   *Tip*: If Qt is not found, add `"CMAKE_PREFIX_PATH": "C:/Qt/6.x.x/msvc2019_64"` to your `.vscode/cmake-kits.json` or `settings.json`.
5.  **Build**: Press **F7** (or run `CMake: Build`).
6.  **Run**: Press **Shift+F5** (or run `CMake: Debug`).

### 💻 Building with Visual Studio (2019/2022)
1.  Launch Visual Studio and select **"Open a local folder"**.
2.  Navigate to the `node` folder.
3.  Visual Studio should auto-detect `CMakeLists.txt`.
4.  If configuration fails/hangs, go to `Project > CMake Settings` and explicitly set the `Qt6_DIR` or `CMAKE_PREFIX_PATH`.
5.  Select `NodeEditor.exe` as the startup item in the toolbar and press **F5**.

### 🔧 Command Line Build
```bash
# 1. Create a build directory to keep source clean
mkdir build
cd build

# 2. Configure (Replace path with your actual Qt installation)
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2019_64"

# 3. Build (Release mode recommended for performance)
cmake --build . --config Release
```

---

## 🎮 Interface & Interaction Guide

### The Canvas (Node Editor)
The main workspace where you construct your node graphs.
*   **Panning**: 
    *   Hold **Middle Mouse Button** and drag.
    *   Hold **Spacebar + Left Mouse Button** and drag.
*   **Zooming**: Use the **Mouse Wheel**. Focus is centered on the cursor.
*   **Selection**: 
    *   **Left Click** to select a single node.
    *   **Drag** (Left Mouse) to box select multiple nodes.
    *   **Shift + Click** to toggle selection of a node.

### Managing Nodes
*   **Adding Nodes**:
    *   **Shift + A**: Opens the full Category Menu at the cursor position.
    *   **Tab**: Opens the Search Menu (Type to filter, Enter to confirm).
    *   **Right Click**: Opens context menu with Add options.
    *   **Ctrl + Shift + A**: Opens **Bulk Add Dialog** (Checkbox list to add multiple nodes at once).
*   **Deleting**: Select nodes and press **Delete** or **Backspace**.
*   **Reset**: Press **R** to reset selected nodes to their default parameter values.
*   **Minimize**: Press **S** (or toggle the arrow in the header) to collapse the node to a smaller footprint.

### Connections (The Wiring)
*   **Standard Connect**: Drag from an Output Socket (Right side) to an Input Socket (Left side).
*   **Quick Connect**:
    1.  **Shift + Click** a socket (Source). It highlights yellow.
    2.  **Ctrl + Click** another socket (Destination).
    3.  *Note*: Order doesn't matter; the system auto-detects Input/Output direction.
*   **Disconnect**:
    *   **Alt + Click** on a connected wire to cut it.
    *   Drag from the Input socket into empty space to detach.
*   **Auto-Connect to Output**:
    *   **Ctrl + Shift + Click** on a node (or its output socket) connects it directly to the `Material Output` node's Surface input. This is the **fastest way to preview** a specific step in your graph.

### File Support
*   **Drag & Drop**:
    *   Drag **Image Files** (`.png`, `.jpg`, `.bmp`, etc.) from Explorer onto the canvas. This automatically creates `Image Texture` nodes with the paths pre-filled.
*   **Saving/Loading**:
    *   **Ctrl + S**: Save the current material graph to a JSON file.
    *   **Ctrl + O**: Load a material graph from a JSON file.
*   **Material Management**:
    *   Use the "Material" menu in the menu bar to Create New or Reset.
    *   Materials are auto-assigned unique names upon creation to prevent accidental overwrites.

---

## 🏛️ System Architecture

### 1. Data Flow Pipeline
The application uses a **Pull-Based** rendering architecture (Lazy Evaluation).

1.  **Output Request**: The `OutputViewerWidget` (the right-side preview) requests a render update.
2.  **Pixel Iteration**: `RenderContext` iterates through the viewport pixels 512x512. This is done using `QtConcurrent::map` for multi-threaded performance.
3.  **Recursion Start**: For each pixel `(x, y)`, the system asks the `Material Output` node to `compute()` its input.
4.  **Graph Traversal**: `Node::compute(pos)` is called.
    *   The `pos` argument contains the current coordinate (usually UV space).
    *   The node requests values from its input sockets.
    *   If a socket is connected, it recursively calls `compute(pos)` on the connected upstream node.
    *   If unconnected, it uses the static parameter value stored in the socket.
5.  **Termination**: Leaf nodes (like `Texture Coordinate`, `Value`, or disconnected inputs) return raw values, terminating the recursion branch.

### 2. Coordinate Spaces
*   **UV Space**: (0.0, 0.0) to (1.0, 1.0). The default coordinate system for textures. This is what `RenderContext` primarily supplies.
*   **Object Space**: Coordinates based on the physical dimensions of the object (-1 to +1 range typically).
*   **Pixel Space**: Screen coordinates (used for UI events), converted to Scene coordinates for the Graph, and UV coordinates for the Renderer.

### 3. Class Hierarchy
*   **Logic Layer**:
    *   **`Node`** (Base, Abstract): Handles sockets, parameters, dirty propagation, and serialization.
    *   **`NodeSocket`**: Handles connection state and value transmission. Supports types: `Float`, `Vector`, `Color`, `Shader`.
    *   **`NodeRegistry`**: Singleton factory that maps string names (e.g., "Math") to constructor lambdas.
*   **UI Layer**:
    *   **`NodeGraphicsItem`**: The `QGraphicsItem` representing a node. Handles painting rounded rectangles, headers, and embedding widgets.
    *   **`NodeGraphicsSocket`**: The visual dot for a socket. Handles mouse hover and drag initiation.
    *   **`ConnectionGraphicsItem`**: The bezier curve wire connecting two sockets.
    *   **`NodeEditorWidget`**: The `QGraphicsView` controller. Manages the `QGraphicsScene`, events, and tools.

---

## 📚 Complete Node Reference

### 1. Geometry Nodes

#### **Polygon Node**
A 2D signed distance field (SDF) generator for polygonal shapes. It differs from standard vector graphics by operating in a pixel-shader-like manor, calculating distances per-pixel.

*   **Mathematical Concept**:
    Uses the analytical SDF of a regular polygon: `d(p) = cos(floor(0.5 + a/r)*r - a) * length(p) - radius`.
    For irregular polygons (Seed > 0), it generates random vertex radii and interpolates the SDF logic.
*   **Sockets**:
    *   (Out) **Value**: Binary mask (1.0 inside, 0.0 outside). Anti-aliasing is not intrinsic but can be achieved by looking at the *Distance* socket.
    *   (Out) **Distance**: The signed distance field.
        *   `< 0.0`: Inside the shape.
        *   `> 0.0`: Outside.
        *   `0.0`: Exact edge.
        *   Useful for creating gradients, glows (e.g., `1.0 / Distance`), or outlines (e.g., `abs(Distance) < Width`).
*   **Parameters**:
    *   **Sides**: The number of vertices.
        *   **Fractional Values**: Uniquely supports non-integer sides (e.g., 4.5). The system interpolates the SDF angle logic, allowing smooth animation between a Square (4) and Pentagon (5).
    *   **Radius**: The circumradius of the polygon (distance from center to potential vertex).
    *   **Rotation**: Z-axis rotation in degrees.
    *   **Seed**:
        *   `0`: Enforces symmetry (Regular Polygon).
        *   `> 0`: Uses the seed to jitter the radius of each vertex. Creates organic "rock-like" or "island-like" shapes.
    *   **Fill**:
        *   `True`: Returns `1.0` for all pixels inside.
        *   `False`: Returns `1.0` only for the edge strip defined by *Edge Width*.
    *   **Edge Width**: Defines the smoothstep feathering for the outline mode.

#### **Point Create Node**
The entry point for "Scattering" workflows. Generates a list of positions in 2D space.

*   **Internal Data Structure**:
    Currently, points are not passed as a list of vectors (which would require a Geometry shader paradigm). Instead, they are encoded into a special protocol where the "Points" socket outputs a procedural texture map that encodes "Distance to nearest point" and "ID of nearest point". This is a "Scatter Map".
*   **Sockets**:
    *   (Out) **Points**: Connect this *only* to a **Scatter on Points** node.
*   **Parameters**:
    *   **Count**: The approximate target number of points. (Exact count depends on mode).
    *   **Mode**:
        *   **Grid**: Places points on a strictly uniform lattice.
        *   **Random**: Uses a white noise generator. Points may overlap or bunch up (clustering). Good for "Grain" or "Sand".
        *   **Poisson Disc**: Uses a "Blue Noise" algorithm (Bridson's algorithm or similar). Ensures no two points are closer than a minimum radius $r$. This creates pleasing, natural-looking distributions (like trees in a forest) without unnatural overlaps.
    *   **Jitter**:
        *   Only active in **Grid** mode.
        *   Adds a random offset vector to each grid point.
        *   `0.0` = Perfect Grid. `1.0` = Equivalent to Random mode.

#### **Scatter on Points Node**
The "Instance" nodes. Takes the *Point Create* distribution and duplicates a *Texture* at each point.

*   **Rendering Logic**:
    For every pixel being rendered:
    1.  It queries the **Points** input to find the "Nearest Point Center" and "Local Coordinate relative to that point".
    2.  It queries the **Texture** input using that *Local Coordinate*.
    3.  It masks the result based on the texture's bounds (usually circular or square around the point).
*   **Sockets**:
    *   (In) **Points**: The distribution map from *Point Create*.
    *   (In) **Texture**: The visual pattern to draw. **Note**: The texture is sampled in a local `-0.5 to +0.5` coordinate space relative to each point.
    *   (Out) **Color**: The accumulated RGB values.
*   **Parameters**:
    *   **Scale**: Global size of every instance.
    *   **Scale Variation**: `Random(ID) * Scale`.
    *   **Rotation Variation**: Rotates the local coordinate system of the input texture per instance.
    *   **Seed**: Shifts the randomization lookup table.

#### **Color Key Node** (Keyer)
A chroma-keying utility.

*   **Function**:
    Takes an input color, compares it to a "Key Color", and creates transparency if they match.
*   **Algorithm**:
    Calculates distance $d = |Color_{in} - Color_{key}|$.
    Alpha is calculated via smoothstep: `alpha = smoothstep(Tolerance, Tolerance + Falloff, d)`.
*   **Parameters**:
    *   **Key Color**: The color to remove (e.g., Green screen, Black background).
    *   **Tolerance**: How close a color must be to be considered transparent.
    *   **Falloff**: The softness width of the opacity transition.


---

### 2. Texture Nodes

#### **Noise Texture**
The `Noise Texture` node is the primary generator for organic patterns. It implements multiple varying algorithms to produce pseudo-random gradients.

*   **Inputs**:
    *   **Vector**: The 3D coordinate to sample. Defaults to `Generated` coordinates if unconnected.
    *   **W**: The 4th dimension coordinate. Used for evolving the noise over time (animation) without moving the texture in space.
    *   **Scale**: Global scaling factor. Corresponds to the frequency of the noise features.
    *   **Detail**: The number of Fractal Brownian Motion (FBM) octaves.
        *   `0`: Single base layer of noise.
        *   `1+`: Adds successively higher frequency (smaller) layers with lower amplitude.
    *   **Roughness**: The "Lacunarity" or "Persistence" of the fractal layers. Controls how much each octave contributes.
        *   Low values (< 0.5): Smoother, blotchy noise.
        *   High values (> 0.5): Gritty, harsh noise.
    *   **Distortion**: The amount of "Domain Warping". This offsets the coordinate vector by a separate noise lookup before sampling the final noise, creating swirling, fluid-like effects.
*   **Parameters**:
    *   **Dimensions**:
        *   `2D`: Ignores Z component.
        *   `3D`: Standard volumetric noise.
        *   `4D`: volumetric + Time.
    *   **Noise Type**:
        *   **Perlin**: The classic gradient noise. Tends to align to grid axes (`x, y, z`), visible as "blocky" artifacts in some rotations.
        *   **OpenSimplex2S**: A modern implementation of Simplex noise (Super Simplex). Isotropic (no grid bias) and faster than Perlin. Recommended for most organic textures.
        *   **OpenSimplex2F**: "Fast" variant. Marginally faster but may have slightly different visual characteristics.
        *   **Gabor**: Sparse Convolution noise. Composed of random kernels. Allows for explicit control over **Anisotropy** (directionality) and **Frequency**. Excellent for fabrics, brushed metal, and wood grain.
    *   **Fractal Type**:
        *   `FBM`: Standard addition of layers.
        *   `Multifractal`: Layers are multiplied.
        *   `Hybrid Multifractal`: Complex blend of add/mul.
        *   `Ridged Multifractal`: Absolute value of noise, inverted. Creates sharp "canyon" or "vein" lines.
    *   **Normalize**: If checked, attempts to remap the output range (mathematically unbounded or -1..1) to exactly [0, 1].

#### **Everling Texture**
A unique "Crystal Growth" based noise algorithm. It generates patterns by simulating a random walk or crystallization process on a 3D grid. Good for biological tissues, erosion patterns, or alien surfaces.

*   **Inputs**:
    *   **Mean**: Controls the bias of the growth steps. Positive values tend to infinite growth (mountains), negative values tend to decay (valleys).
    *   **Std Dev**: Controls the roughness/variance of the steps. Higher values make the terrain more jagged.
    *   **Spread**: (Gaussian Mode only) Controls the tightness of the growth clusters.
*   **Parameters**:
    *   **Access Method**: Defines how the algorithm selects the next cell to grow from.
        *   `Stack`: Depth-First. Creates long, vein-like fractals.
        *   `Random`: Uniform random. Creates localized erosion/pockmarks.
        *   `Gaussian`: Gaussian distributed. Creates cloud-like clusters.
        *   `Mixed`: A blend of Stack and Random (50/50).
    *   **Cluster Spread**: Controls the Gaussian distribution width for the "Gaussian" access method.
    *   **Tile Resolution**: Controls the size of the internal simulation grid (default 256). Higher values reduce repetition frequency.
    *   **Tiling Mode (Periodicity)**: **(New)**
        *   `Repeat`: Standard modulo wrapping. Hard edges unless Smooth Width is used.
        *   `Mirror`: Ping-pong wrapping. Seamless edges by symmetry. Best for continuous infinite surfaces.
    *   **Distortion**: **(New)** Amount of domain warping applied to the coordinates before lookup. This uses a random noise field to distort the grid, effectively hiding the repetitive grid structure and creating a more organic, non-periodic look.
    *   **Detail (Octaves)**: **(New)** Number of Fractal layers. Adds fine-grained detail by summing the noise at smaller scales. Helps break up large monochromatic clusters.
    *   **Roughness**: Controls the influence of higher octaves (standard Fractal Gain).
    *   **Smooth Width**: **(Legacy)** Width of edge fading in `Repeat` mode. (0.0 - 0.5).

#### **Voronoi Texture**
Generates a cellular pattern (Worley noise) by scattering points in space and calculating distances to them.

*   **Outputs**:
    *   **Distance**: The Euclidean (or other metric) distance to the nearest feature point.
    *   **Color**: A unique random color assigned to the specific cell (Voronoi region) the pixel is inside.
    *   **Position**: The center coordinate of the nearest feature point.
    *   **W**, **Radius**: Auxiliary geometric data.
*   **Parameters**:
    *   **Key Feature Modes**:
        *   **F1**: Distance to the closest point. Standard cellular look.
        *   **F2**: Distance to the *second* closest point.
        *   **Smooth F1**: Using a "Smooth Minimum" function to blend between cell borders. This creates organic, metaball-like shapes instead of sharp crystal edges.
        *   **Distance to Edge**: Analytically calculates `(F2 - F1)`. This results in a value that is 0 exactly at the border and increases towards the center of cells. Perfect for circuitry, stone paving, or scales.
    *   **Metric (Distance Function)**:
        *   **Euclidean**: Standard `sqrt(x^2 + y^2)`. Circular/Spherical cells.
        *   **Manhattan**: `|x| + |y|`. Diamond/Square shaped cells.
        *   **Chebyshev**: `max(|x|, |y|)`. Square cells (Chessboard-like).
        *   **Minkowski**: Generalized metric with exponent.

#### **River Texture**
A specialized, simulation-based node designed for terrain generation.

*   **Design Philosophy**:
    Unlike noise, rivers must be continuous and usually directed. This node essentially "simulates" a river network on a 2D grid (internally `512x512` or similar) and then samples that grid.
*   **Performance Warning**:
    Because it builds a full map, changing parameters triggers a re-simulation which can take 100-200ms. It is flagged as an "Expensive" node.
*   **Inputs**:
    *   **Water Mask**: A critical input. The river generation algorithm looks at this mask.
        *   If Mask > 0.5: The area is considered "Ocean/Lake". Rivers will try to terminate here.
        *   If Mask < 0.5: The area is "Land". Rivers will flow through here.
*   **Parameters**:
    *   **Source Count**: How many river springs to spawn.
    *   **Dest Count**: How many attraction points (mouths) to simulate.
    *   **Flow**: A graphical parameter that displaces the noise used for river "meander" (wiggle). Animating this makes the river shape evolve.
    *   **Width / Variation**: Controls the rasterization thickness of the river lines.

#### **Water Source**
A helper node for the `River Texture`.

*   **Function**:
    Generates a radial Signed Distance Field (SDF) perturbed by noise.
*   **Usage**:
    Connect this to the `Water Mask` input of a River node to define a central "Ocean" or "Lake" into which rivers should flow.
*   **Parameters**:
    *   **Size**: Radius of the water body.
    *   **Smoothness**: Smoothstep falloff width for the shore.
    *   **Distortion**: Magnitude of noise applied to the coastline.

#### **Image Texture**
Loads and samples raster images.

*   **Color Space**: Currently assumes sRGB and converts to Linear internally (Qt default behavior).
*   **Inputs**:
    *   **Vector**: UV coordinates. If unconnected, uses standard UVs.
*   **Parameters**:
    *   **Interpolation**:
        *   **Linear**: Bilinear. Fast, smooth.
        *   **Closest**: Nearest neighbor. useful for pixel art or data maps where intermediate values are invalid.
        *   **Cubic**: Bicubic. Smoother, sharper, but slower.
    *   **Extension**:
        *   **Repeat**: Tiling.
        *   **Clip**: Returns (0,0,0,0) outside UV 0..1 range.
        *   **Extend**: Streaks the edge pixels.

#### **Brick Texture**
Procedural brick/tile generator.

*   **Algorithms**:
    Calculates row and column indices based on input vector. Shifts every other row (or based on `Frequency 2`) to create bond patterns.
*   **Parameters**:
    *   **Offset**: The normalized amount (0..1) to shift alternate rows. 0.5 = Standard Running Bond.
    *   **Mortar Size**: Thickness of the gap between bricks. 0.0 = No Mortar.
    *   **Squash**: Stretches the coordinate system to make bricks wider or taller.
    *   **Bias**: Varies the color of individual bricks (`Brick Color` vs `Mortar Color` is usually handled by mixing two external color nodes using the `Mortar` output mask).

#### **Wave Texture**
Generates bands or rings.

*   **Types**:
    *   **Bands**: Linear sine wave `sin(x)`. Patterns lines.
    *   **Rings**: Radial sine wave `sin(sqrt(x^2 + y^2))`. Concentric circles.
*   **Profiles**:
    *   **Sine**: Smooth.
    *   **Saw**: Linear ramp 0->1.
    *   **Triangle**: Linear Time 0->1->0.

#### **Radial Tiling**
A utility texture that generates a "Fan" or "Star" pattern.
*   **Logic**: Calculates `atan2(y, x)` to get the angle, then scales it by `Frequency`.
*   Uses `fmod` to repeat the gradient `N` times around the center.


---

### 3. Converter & Math Nodes

#### **Math Node**
The swiss-army knife for scalar values. It performs mathematical operations on one or two input floats.

*   **Inputs**:
    *   **Value 1**: Primary operand.
    *   **Value 2**: Secondary operand (or exponent, or modulo divisor).
    *   **Value 3**: Used for specific three-input operations (e.g., Multiply-Add).
*   **Operations**:
    *   **Arithmetic**: `Add` ($a+b$), `Subtract` ($a-b$), `Multiply` ($a \times b$), `Divide` ($a/b$), `Multiply Add` ($a \times b + c$).
    *   **Power / Root**:
        *   `Power`: $a^b$.
        *   `Logarithm`: $\log_b(a)$.
        *   `Sqrt`: $\sqrt{a}$.
        *   `Inverse Sqrt`: $1 / \sqrt{a}$ (Optimized for normalizing).
        *   `Exponent`: $e^a$.
    *   **Comparison**:
        *   `Minimum`: $\min(a, b)$.
        *   `Maximum`: $\max(a, b)$.
        *   `Less Than`: Return 1 if $a < b$ else 0.
        *   `Greater Than`: Return 1 if $a > b$ else 0.
        *   `Sign`: Return 1 if positive, -1 if negative, 0 if zero.
        *   `Compare`: Return 1 if $a$ roughly equals $b$ (within epsilon).
    *   **Smooth Comparison**:
        *   `Smooth Minimum`: Softly blends two values using polynomial smin. good for melting shapes together.
        *   `Smooth Maximum`: Soft blend max.
    *   **Rounding**:
        *   `Round`: Nearest integer.
        *   `Floor`: Round down.
        *   `Ceil`: Round up.
        *   `Truncate`: Remove decimal part.
        *   `Fraction`: Return only decimal part ($0.5$).
    *   **Periodic**:
        *   `Modulo`: $a \% b$ (Standard fmod, signs match dividend).
        *   `Floored Modulo`: Periodic modulo (signs cycle correctly).
        *   `Snap`: Rounds $a$ to the nearest multiple of $b$.
        *   `Wrap`: Wraps $a$ into the range $[Min, Max]$.
        *   `PingPong`: Bounces $a$ back and forth between $0$ and $b$.
    *   **Trigonometry**:
        *   `Sine`, `Cosine`, `Tangent`.
        *   `Arcsine`, `Arccosine`, `Arctangent`.
        *   `Arctangent2`: `atan2(a, b)`.
        *   `Sinh`, `Cosh`, `Tanh`.
        *   `To Radians`, `To Degrees`.
*   **Clamp**: Restricts output to [0, 1].

#### **Vector Math Node**
Performs 3D vector operations.

*   **Operations**:
    *   **Add**, **Subtract**, **Multiply**, **Divide**: Component-wise operations.
    *   **Cross Product**: Returns vector perpendicular to A and B.
    *   **Dot Product**: Returns geometry cosine ($A \cdot B$). Output is on the **Value** socket.
    *   **Distance**: Euclidean distance between A and B.
    *   **Length**: Magnitude of vector A.
    *   **Normalize**: Returns unit vector $\hat{A}$.
    *   **Reflect**: Reflected direction of vector A off normal B. Formula: $I - 2(N \cdot I)N$.
    *   **Refract**: Refracted direction based on IOR (Index of Refraction).
    *   **Faceforward**: Flips vector A if it points somewhat opposite to Reference B.
    *   **Project**: Projects A onto B.
    *   **Snap**: Snaps vector components to nearest grid step.
    *   **Modulo**: Component-wise modulo.

#### **Color Ramp**
Maps a scalar input (0-1) to a color gradient.

*   **UI Controls**:
    *   **Gradient Bar**: Visual representation. Click top manipulate stops.
    *   **Interpolation Modes**:
        *   `Linear`: Standard Lerp.
        *   **Constant**: Step function. No interpolation between stops.
        *   **Cardinal**: Cubic spline. Very smooth transitions (No kinks at stops).
*   **Usage**: Primary tool for converting a black-and-white Noise Mask into a colored Map (e.g., Height -> Terrain Colors).

#### **Mix Node**
The universal blender.

*   **Input Types**:
    *   **Float**: Linear interpolation between two numbers.
    *   **Vector**: Component-wise lerp. Can be **Uniform** (one Factor) or **Non-Uniform** (Vector Factor).
    *   **Color**: Blends two RGBA colors using Photoshop-style blend modes.
*   **Blend Modes** (Color):
    *   **Mix**: Standard Alpha blending.
    *   **Darken**: Keeps the darker of the two.
    *   **Multiply**: Multiplies RGB values. Always darkens.
    *   **Color Burn**: Darkens base color to reflect blend color by increasing contrast.
    *   **Lighten**: Keeps the lighter of the two.
    *   **Screen**: Inverts, multiplies, inverts. Always lightens.
    *   **Color Dodge**: Brightens base color to reflect blend color.
    *   **Overlay**: Multiplies or Screens based on base color. High contrast.
    *   **Soft Light**: Organic version of Overlay.
    *   **Linear Light**: Burn or Dodge based on blend color.
    *   **Difference**: Absolute difference. Good for checking alignment.
    *   **Exclusion**: Lower contrast difference.
    *   **Subtract**: $A - B$.
    *   **Divide**: $A / B$.
    *   **Hue**: Takes Hue from B, Sat/Val from A.
    *   **Saturation**: Takes Sat from B, Hue/Val from A.
    *   **Color**: Takes Hue/Sat from B, Val from A.
    *   **Value**: Takes Val from B, Hue/Sat from A.

#### **Map Range**
Remaps value $v$ from $[InMin, InMax]$ to $[OutMin, OutMax]$.

*   **Formula**: $result = OutMin + \frac{(v - InMin)(OutMax - OutMin)}{InMax - InMin}$
*   **Clamping**: Checkbox to force result within OutMin/OutMax.
*   **Steps**:
    *   `Linear`: Standard.
    *   `Stepped`: Steps output (quantized).
    *   `Smoothstep`: Hermite interpolation. $t = t * t * (3 - 2 * t)$.
    *   `Smootherstep`: Ken Perlin's 5th order polynomial. $t * t * t * (t * (t * 6 - 15) + 10)$.

#### **Separate / Combine XYZ**
*   **Separate**: Deconstructs a Vector into float X, Y, Z.
*   **Combine**: Reconstructs a Vector from float X, Y, Z.


---

### 4. Input & Output Nodes

#### **Texture Coordinate**
The starting point for most graphs.
*   **Generated**: (0..1) bounding box coordinates.
*   **Normal**: Surface normal vector (currently defaults to Z-up (0,0,1) for 2D planes).
*   **UV**: Explicit UV map coordinates.
*   **Object**: Local object coordinates (centered at 0,0, range -1 to 1).
*   **Window**: Screen space coordinates (0..width, 0..height).

#### **Mapping**
Applies geometric transformations to coordinates.
*   **Inputs**: Vector input.
*   **Parameters**:
    *   **Location**: Translation/Offset.
    *   **Rotation**: Euler rotation (Degrees).
    *   **Scale**: Scaling factors.
*   **Type**: 
    *   `Point`: Translates locations.
    *   `Texture`: Inverse transform (Rotating +30 degrees rotates texture -30).
    *   `Vector`: Ignores translation (for direction vectors).

#### **Material Output**
The final node. **CRITICAL**: To see any result, your graph must eventually connect to this node.
*   **Inputs**:
    *   **Surface**: The Color/Shader to render.
    *   **Displacement**: Height map (Future support for tessellation).

---

## 🔧 Developer Guide (Deep Dive)

### 📂 Directory & File Breakdown

The project structure is flat but logically organized. Here is a guided tour:

*   **Core Application**:
    *   `main.cpp`: Entry point. Sets up the `QApplication` and shows the `MainWindow`.
    *   `mainwindow.h/cpp`: The main frame. Manages the Menubar, Toolbar, and Dock Widgets. It handles high-level actions like "Save Project" or "Add Node".
    *   `appsettings.h`: A Singleton configuration store. Manages global settings like `defaultRenderSize` or `lastDirectory`.
    
*   **The Node Engine (`node` class)**:
    *   `node.h/cpp`: The abstract base class.
        *   **`compute(pos)`**: The virtual function every node must implement.
        *   **`sockets`**: Stores the list of inputs and outputs.
        *   **`setDirty(bool)`**: When called, recursively notifies all outputs that they need to re-evaluate.
    *   **`noderegistry.h/cpp`**: The Factory.
        *   It maps a string (e.g., "Noise Texture") to a lambda function that calls `new NoiseTextureNode()`.
        *   **Crucial**: If you add a node class, you **MUST** register it here, or it won't appear in the menu.
    *   **`node_defs.h`**: Common structs (e.g., `NodeId`, `SocketType`) to avoid circular dependencies.

*   **Graphics View Framework (The UI)**:
    *   `nodeeditorwidget.h/cpp`: The Controller.
        *   Inherits `QGraphicsView`.
        *   Handles `mousePress`, `mouseMove` (dragging nodes, panning).
        *   Manages the `QGraphicsScene` which holds all items.
    *   `nodGraphicsItem.h/cpp`: The View.
        *   Inherits `QGraphicsItem`.
        *   Responsible for `paint()` (drawing the rounded body, title, gradient backgrounds).
        *   Manages layout of embedded widgets (Sliders, Comboboxes) using `QGraphicsProxyWidget`.
    *   `nodesocketgraphicsitem.h/cpp`: The visual "Dot".
        *   Color-codes itself based on `SocketType` (e.g., Gray for Float, Blue for Vector, Yellow for Color).

*   **Custom UI Components (`uicomponents.h/cpp`)**:
    *   **`PopupAwareComboBox`**: A critical fix for Qt. Standard `QComboBox` inside a `QGraphicsScene` often renders its popup in the wrong location or closes immediately. This custom class forces the popup to respect global screen coordinates.
    *   **`WheelEventFilter`**: Intercepts scroll events on embedded widgets to prevent the whole Canvas from zooming when you just wanted to scroll a dropdown.

### ➕ Tutorial: Adding a New Node (Step-by-Step)

Let's say we want to add a **"Lens Blur"** node (hypothetically).

**Step 1: Create the Header (`lensblurnode.h`)**
```cpp
#pragma once
#include "node.h"

class LensBlurNode : public Node {
public:
    LensBlurNode();
    // The core calculation logic
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    // Define UI parameters that are not sockets (if any)
    QVector<ParameterInfo> parameters() const override; 
};
```

**Step 2: Implementation (`lensblurnode.cpp`)**
```cpp
#include "lensblurnode.h"

LensBlurNode::LensBlurNode() : Node("Lens Blur") {
    // 1. Define Inputs
    addInputSocket(new NodeSocket("Image", SocketType::Color, SocketDirection::Input, this));
    addInputSocket(new NodeSocket("Blur Radius", SocketType::Float, SocketDirection::Input, this));
    
    // 2. Define Outputs
    addOutputSocket(new NodeSocket("Color", SocketType::Color, SocketDirection::Output, this));
    
    // 3. Set Defaults
    inputSockets()[1]->setDefaultValue(5.0); // Default blur radius
}

QVariant LensBlurNode::compute(const QVector3D& pos, NodeSocket* socket) {
    // Note: A true blur needs access to the whole image, not just one pixel.
    // However, our current architecture is per-pixel.
    // A real implementation would pre-calculate the blur in evaluate() or use stochastic sampling.
    
    // Simple stochastic example (slow but working):
    QVector3D sumColor(0,0,0);
    int samples = 10;
    float radius = m_inputs[1]->getValue(pos).toDouble();
    
    for(int i=0; i<samples; ++i) {
        // Offset pos slightly
        QVector3D offset(randF()*radius, randF()*radius, 0);
        sumColor += m_inputs[0]->getValue(pos + offset).value<QVector3D>();
    }
    
    return sumColor / samples;
}
```

**Step 3: Registration (`noderegistry.cpp`)**
```cpp
// Inside NodeRegistry::NodeRegistry()
registerNode("Filter", "Lens Blur", []() { return new LensBlurNode(); });
```

**Step 4: Build System**
*   Add `lensblurnode.cpp` and `.h` to the `add_executable` list in `CMakeLists.txt`.
*   Run `cmake --build .`

### ⚡ Performance & Optimization
*   **Thread Safety**: The `compute` function is called from **multiple worker threads** simultaneously during rendering.
    *   **Do Not**: Modify member variables inside `compute` (e.g., `m_lastResult = ...` is FORBIDDEN).
    *   **Do**: Use local stack variables.
    *   **Do**: Use `const` methods where possible.
    *   **Mutexes**: If you must modify state (e.g., lazy initialization of a lookup table), use `QMutexLocker`.
*   **Caching Strategy**:
    *   For expensive computations (like `RiverNode` or `PointCreateNode`), do not recompute per-pixel.
    *   Generate the data once when parameters change (`setDirty(true)` triggers this), store it in a member variable (e.g. `QVector<QPointF>`), and read-only access it in `compute()`.
*   **Dirty Propagation**:
    *   Calling `setDirty(true)` on a node automatically propagates to all downstream nodes (outputs).
    *   This invalidates the `OutputViewerWidget`'s cache and triggers a re-render.

### 🛑 Memory Management
*   **Ownership Model**:
    *   `NodeEditorWidget` owns `Node` instances (Logic).
    *   `QGraphicsScene` owns `NodeGraphicsItem` instances (Visuals).
    *   `Node` owns `NodeSocket` instances.
*   **Lifecycle**:
    *   When a node is deleted from the UI, `NodeEditorWidget::deleteSelection` handles the cleanup.
    *   It removes the `GraphicsItem` from the Scene (scene deletes it).
    *   It manually deletes the `Node` pointer.
*   **Undo/Redo**:
    *   We use `QUndoStack`. Operations like `ConnectCommand`, `MoveNodeCommand` capture the state *before* and *after*.
    *   **Warning**: Storing pointers in Undo Commands is dangerous if the object is deleted. We typically store internal IDs or ensure commands are cleared on major graph changes.

### 🔍 Debugging
*   **Visual Debugging**: The easiest way to debug is to connect your specific node output to `Material Output`.
*   **Console Logging**: Use `qDebug() << "Value:" << val;`. This output appears in the Visual Studio Output window or terminal.
*   **Breakpoints**: Since the render loop runs on worker threads, ensure your breakpoints are set correctly in VS Code/Visual Studio. The UI thread remains responsive (mostly), but stepping might freeze the render preview.


---

## ❓ FAQ & Troubleshooting

**Q: My node output is black?**
*   Check if **Scale** is too large or too small (noise might be zoomed out).
*   Check connection to `Material Output`.
*   Check if inputs are valid (e.g., dividing by zero).
*   Check alpha channel if using Color nodes.

**Q: The UI is unresponsive during render?**
*   This is known behavior in the current architecture if the render threads saturate the CPU. We use `QtConcurrent` which attempts to leave the GUI thread free, but heavy lock contention or high thread counts can cause stutter.

**Q: How do I create a new Socket Type?**
*   Edit `SocketType` enum in `node.h`.
*   Update `NodeGraphicsSocket::paint()` to define its color code.
*   Update `NodeConnection::isValid()` definition to define compatibility rules (e.g., Auto-cast Float to Vector).

**Q: Can I use GPU acceleration?**
*   Currently, the engine is **CPU-based** for flexibility and ease of development. Future plans include porting the `Node::compute` logic to GLSL shaders for real-time OpenGL preview, but this requires a significant architecture refactor (AST generation).

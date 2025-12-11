#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QMap>
#include <QString>

class AppSettings : public QObject {
    Q_OBJECT
public:
    static AppSettings& instance() {
        static AppSettings instance;
        return instance;
    }

    enum class Language {
        English,
        Japanese,
        Chinese
    };
    Q_ENUM(Language)

    enum class Theme {
        Dark,
        Light,
        Colorful
    };
    Q_ENUM(Theme)

    int maxThreads() const { return m_maxThreads; }
    void setMaxThreads(int count) {
        if (m_maxThreads != count) {
            m_maxThreads = count;
            emit maxThreadsChanged(count);
        }
    }

    bool showFPS() const { return m_showFPS; }
    void setShowFPS(bool show) {
        if (m_showFPS != show) {
            m_showFPS = show;
            emit showFPSChanged(show);
        }
    }

    Language language() const { return m_language; }
    void setLanguage(Language lang) {
        if (m_language != lang) {
            m_language = lang;
            emit languageChanged(lang);
        }
    }

    Theme theme() const { return m_theme; }
    void setTheme(Theme theme) {
        if (m_theme != theme) {
            m_theme = theme;
            emit themeChanged(theme);
        }
    }
    
    int renderWidth() const { return m_renderWidth; }
    void setRenderWidth(int width) {
        if (m_renderWidth != width) {
            m_renderWidth = width;
            emit renderResolutionChanged(width, m_renderHeight);
        }
    }
    
    int renderHeight() const { return m_renderHeight; }
    void setRenderHeight(int height) {
        if (m_renderHeight != height) {
            m_renderHeight = height;
            emit renderResolutionChanged(m_renderWidth, height);
        }
    }
    
    // Viewport range in UV space
    double viewportMinU() const { return m_viewportMinU; }
    double viewportMinV() const { return m_viewportMinV; }
    double viewportMaxU() const { return m_viewportMaxU; }
    double viewportMaxV() const { return m_viewportMaxV; }
    
    void setViewportMinU(double value) {
        if (m_viewportMinU != value) {
            m_viewportMinU = value;
            emit viewportRangeChanged();
        }
    }
    void setViewportMinV(double value) {
        if (m_viewportMinV != value) {
            m_viewportMinV = value;
            emit viewportRangeChanged();
        }
    }
    void setViewportMaxU(double value) {
        if (m_viewportMaxU != value) {
            m_viewportMaxU = value;
            emit viewportRangeChanged();
        }
    }
    void setViewportMaxV(double value) {
        if (m_viewportMaxV != value) {
            m_viewportMaxV = value;
            emit viewportRangeChanged();
        }
    }

    QString translate(const QString& key) const {
        if (m_language == Language::English) return key;
        
        static const QMap<QString, QMap<Language, QString>> dictionary = {
            // Node parameters
            {"Scale", {{Language::Japanese, "スケール"}, {Language::Chinese, "缩放"}}},
            {"Scale X", {{Language::Japanese, "スケール X"}, {Language::Chinese, "缩放 X"}}},
            {"Scale Y", {{Language::Japanese, "スケール Y"}, {Language::Chinese, "缩放 Y"}}},
            {"Detail", {{Language::Japanese, "詳細"}, {Language::Chinese, "细节"}}},
            {"Roughness", {{Language::Japanese, "粗さ"}, {Language::Chinese, "粗糙度"}}},
            {"Distortion", {{Language::Japanese, "歪み"}, {Language::Chinese, "失真"}}},
            {"Lacunarity", {{Language::Japanese, "空隙性"}, {Language::Chinese, "隙度"}}},
            {"Offset", {{Language::Japanese, "オフセット"}, {Language::Chinese, "偏移"}}},
            {"W", {{Language::Japanese, "W (時間)"}, {Language::Chinese, "W (时间)"}}},
            {"Dimensions", {{Language::Japanese, "次元"}, {Language::Chinese, "维度"}}},
            {"Type", {{Language::Japanese, "タイプ"}, {Language::Chinese, "类型"}}},
            {"Normalize", {{Language::Japanese, "正規化"}, {Language::Chinese, "归一化"}}},
            {"Fac", {{Language::Japanese, "係数"}, {Language::Chinese, "系数"}}},
            {"Color", {{Language::Japanese, "カラー"}, {Language::Chinese, "颜色"}}},
            {"Vector", {{Language::Japanese, "ベクトル"}, {Language::Chinese, "向量"}}},
            {"Operation", {{Language::Japanese, "演算"}, {Language::Chinese, "运算"}}},
            
            // Image Texture Node
            {"Open Image", {{Language::Japanese, "画像を開く"}, {Language::Chinese, "打开图像"}}},
            {"No image", {{Language::Japanese, "画像なし"}, {Language::Chinese, "无图像"}}},
            {"Stretch", {{Language::Japanese, "引き伸ばし"}, {Language::Chinese, "拉伸"}}},
            {"Keep Aspect Ratio", {{Language::Japanese, "アスペクト比固定"}, {Language::Chinese, "保持纵横比"}}},
            {"Repeat", {{Language::Japanese, "リピート"}, {Language::Chinese, "重复"}}},
            
            // Noise/Texture types
            {"Basis", {{Language::Japanese, "基本"}, {Language::Chinese, "基础"}}},
            {"Fractal", {{Language::Japanese, "フラクタル"}, {Language::Chinese, "分形"}}},
            {"Feature", {{Language::Japanese, "特徴"}, {Language::Chinese, "特征"}}},
            {"Metric", {{Language::Japanese, "距離"}, {Language::Chinese, "度量"}}},
            {"Coordinate", {{Language::Japanese, "座標"}, {Language::Chinese, "坐标"}}},
            {"Noise Type", {{Language::Japanese, "ノイズタイプ"}, {Language::Chinese, "噪波类型"}}},
            
            // Math/Vector operations
            {"Data Type", {{Language::Japanese, "データ型"}, {Language::Chinese, "数据类型"}}},
            {"Blend Mode", {{Language::Japanese, "ブレンドモード"}, {Language::Chinese, "混合模式"}}},
            {"Mix Mode", {{Language::Japanese, "ミックスモード"}, {Language::Chinese, "混合模式"}}},
            {"Operation Mode", {{Language::Japanese, "演算モード"}, {Language::Chinese, "运算模式"}}},
            
            // Calculus modes
            {"Derivative X", {{Language::Japanese, "X微分 (∂f/∂x)"}, {Language::Chinese, "X偏导数"}}},
            {"Derivative Y", {{Language::Japanese, "Y微分 (∂f/∂y)"}, {Language::Chinese, "Y偏导数"}}},
            {"Gradient", {{Language::Japanese, "勾配 (|∇f|)"}, {Language::Chinese, "梯度"}}},
            {"Laplacian", {{Language::Japanese, "ラプラシアン"}, {Language::Chinese, "拉普拉斯"}}},
            {"Integral X", {{Language::Japanese, "X積分 (∫dx)"}, {Language::Chinese, "X积分"}}},
            {"Integral Y", {{Language::Japanese, "Y積分 (∫dy)"}, {Language::Chinese, "Y积分"}}},
            
            // Wave Texture
            {"Wave Type", {{Language::Japanese, "波形タイプ"}, {Language::Chinese, "波形类型"}}},
            {"Direction", {{Language::Japanese, "方向"}, {Language::Chinese, "方向"}}},
            {"Profile", {{Language::Japanese, "プロファイル"}, {Language::Chinese, "轮廓"}}},
            
            // Node names
            {"Noise Texture", {{Language::Japanese, "ノイズテクスチャ"}, {Language::Chinese, "噪波纹理"}}},
            {"River Texture", {{Language::Japanese, "川テクスチャ"}, {Language::Chinese, "河流纹理"}}},
            {"Water Source", {{Language::Japanese, "水源"}, {Language::Chinese, "水源"}}},
            {"Voronoi Texture", {{Language::Japanese, "ボロノイテクスチャ"}, {Language::Chinese, "沃罗诺伊纹理"}}},
            {"Image Texture", {{Language::Japanese, "画像テクスチャ"}, {Language::Chinese, "图像纹理"}}},
            {"Texture Coordinate", {{Language::Japanese, "テクスチャ座標"}, {Language::Chinese, "纹理坐标"}}},
            {"Mapping", {{Language::Japanese, "マッピング"}, {Language::Chinese, "映射"}}},
            {"Color Ramp", {{Language::Japanese, "カラーランプ"}, {Language::Chinese, "颜色渐变"}}},
            {"Math", {{Language::Japanese, "数学"}, {Language::Chinese, "数学"}}},
            {"Vector Math", {{Language::Japanese, "ベクトル数学"}, {Language::Chinese, "向量数学"}}},
            {"Mix", {{Language::Japanese, "ミックス"}, {Language::Chinese, "混合"}}},
            {"Material Output", {{Language::Japanese, "マテリアル出力"}, {Language::Chinese, "材质输出"}}},
            {"Wave Texture", {{Language::Japanese, "波テクスチャ"}, {Language::Chinese, "波纹纹理"}}},
            {"Bump", {{Language::Japanese, "バンプ"}, {Language::Chinese, "凹凸"}}},
            {"Map Range", {{Language::Japanese, "範囲マッピング"}, {Language::Chinese, "映射范围"}}},
            {"Calculus", {{Language::Japanese, "微積分"}, {Language::Chinese, "微积分"}}},
            {"Separate XYZ", {{Language::Japanese, "XYZ分離"}, {Language::Chinese, "分离XYZ"}}},
            {"Combine XYZ", {{Language::Japanese, "XYZ合成"}, {Language::Chinese, "合并XYZ"}}},
            {"Clamp", {{Language::Japanese, "範囲制限"}, {Language::Chinese, "钳制"}}},
            {"Brick Texture", {{Language::Japanese, "レンガテクスチャ"}, {Language::Chinese, "砖块纹理"}}},
            {"Radial Tiling", {{Language::Japanese, "放射タイリング"}, {Language::Chinese, "径向平铺"}}},
            {"Invert", {{Language::Japanese, "反転"}, {Language::Chinese, "反转"}}},
            {"Principled BSDF", {{Language::Japanese, "プリンシプルBSDF"}, {Language::Chinese, "原理化BSDF"}}},
            {"Mix Shader", {{Language::Japanese, "シェーダーミックス"}, {Language::Chinese, "混合着色器"}}},
            
            // Categories
            {"Texture", {{Language::Japanese, "テクスチャ"}, {Language::Chinese, "纹理"}}},
            {"Converter", {{Language::Japanese, "コンバータ"}, {Language::Chinese, "转换器"}}},
            {"Input", {{Language::Japanese, "入力"}, {Language::Chinese, "输入"}}},
            {"Output", {{Language::Japanese, "出力"}, {Language::Chinese, "输出"}}},
            {"Shader", {{Language::Japanese, "シェーダー"}, {Language::Chinese, "着色器"}}},
            
            // Settings menu
            {"Settings", {{Language::Japanese, "設定"}, {Language::Chinese, "设置"}}},
            {"CPU Usage (Threads):", {{Language::Japanese, "CPU使用率 (スレッド):"}, {Language::Chinese, "CPU使用率 (线程):"}}},
            {"Show FPS", {{Language::Japanese, "FPSを表示"}, {Language::Chinese, "显示FPS"}}},
            {"Language:", {{Language::Japanese, "言語:"}, {Language::Chinese, "语言:"}}},
            {"Language", {{Language::Japanese, "言語"}, {Language::Chinese, "语言"}}},
            {"Theme", {{Language::Japanese, "テーマ"}, {Language::Chinese, "主题"}}},
            {"Dark", {{Language::Japanese, "ダーク"}, {Language::Chinese, "暗色"}}},
            {"Light", {{Language::Japanese, "ライト"}, {Language::Chinese, "亮色"}}},
            {"Colorful", {{Language::Japanese, "カラフル"}, {Language::Chinese, "多彩"}}},
            
            // Menu items
            {"File", {{Language::Japanese, "ファイル"}, {Language::Chinese, "文件"}}},
            {"Edit", {{Language::Japanese, "編集"}, {Language::Chinese, "编辑"}}},
            {"Run", {{Language::Japanese, "実行"}, {Language::Chinese, "运行"}}},
            {"Exit", {{Language::Japanese, "終了"}, {Language::Chinese, "退出"}}},
            {"Export", {{Language::Japanese, "エクスポート"}, {Language::Chinese, "导出"}}},
            {"Save", {{Language::Japanese, "保存"}, {Language::Chinese, "保存"}}},
            {"Load", {{Language::Japanese, "読み込み"}, {Language::Chinese, "加载"}}},
            {"Save Nodes", {{Language::Japanese, "ノードを保存"}, {Language::Chinese, "保存节点"}}},
            {"Load Nodes", {{Language::Japanese, "ノードを読み込み"}, {Language::Chinese, "加载节点"}}},
            {"Editor", {{Language::Japanese, "エディタ"}, {Language::Chinese, "编辑器"}}},
            
            // Render Settings
            {"Render Settings", {{Language::Japanese, "レンダー設定"}, {Language::Chinese, "渲染设置"}}},
            {"Resolution:", {{Language::Japanese, "解像度:"}, {Language::Chinese, "分辨率:"}}},
            {"Auto Update", {{Language::Japanese, "自動更新"}, {Language::Chinese, "自动更新"}}},
            {"Viewport Range (UV Space)", {{Language::Japanese, "ビューポート範囲 (UV空間)"}, {Language::Chinese, "视口范围 (UV空间)"}}},
            {"Reset (0-1)", {{Language::Japanese, "リセット (0-1)"}, {Language::Chinese, "重置 (0-1)"}}},
            {"Link U/V", {{Language::Japanese, "U/Vをリンク"}, {Language::Chinese, "链接 U/V"}}},
            {"Min U:", {{Language::Japanese, "最小 U:"}, {Language::Chinese, "最小 U:"}}},
            {"Min V:", {{Language::Japanese, "最小 V:"}, {Language::Chinese, "最小 V:"}}},
            {"Max U:", {{Language::Japanese, "最大 U:"}, {Language::Chinese, "最大 U:"}}},
            {"Max V:", {{Language::Japanese, "最大 V:"}, {Language::Chinese, "最大 V:"}}},
            
            // Output viewer
            {"Double-click to reset", {{Language::Japanese, "ダブルクリックでリセット"}, {Language::Chinese, "双击重置"}}},
            {"No output", {{Language::Japanese, "出力なし"}, {Language::Chinese, "无输出"}}},
            {"Connect nodes and run", {{Language::Japanese, "ノードを接続して実行"}, {Language::Chinese, "连接节点并运行"}}},
            {"Drag edges to adjust UV range", {{Language::Japanese, "端をドラッグしてUV範囲を調整"}, {Language::Chinese, "拖动边缘调整UV范围"}}},
            {"Add Node", {{Language::Japanese, "ノードを追加"}, {Language::Chinese, "添加节点"}}},
            {"Connect to Node", {{Language::Japanese, "ノードに接続"}, {Language::Chinese, "连接到节点"}}},
            {"Search...", {{Language::Japanese, "検索..."}, {Language::Chinese, "搜索..."}}}
        };
        
        if (dictionary.contains(key)) {
            const auto& trans = dictionary[key];
            if (trans.contains(m_language)) {
                return trans[m_language];
            }
        }
        return key;
    }

signals:
    void maxThreadsChanged(int count);
    void showFPSChanged(bool show);
    void languageChanged(Language lang);
    void themeChanged(Theme theme);
    void renderResolutionChanged(int width, int height);
    void viewportRangeChanged();

private:
    AppSettings() : m_maxThreads(4), m_showFPS(false), m_language(Language::English), m_theme(Theme::Dark),
                    m_renderWidth(512), m_renderHeight(512),
                    m_viewportMinU(0.0), m_viewportMinV(0.0), m_viewportMaxU(1.0), m_viewportMaxV(1.0) {}
    Q_DISABLE_COPY(AppSettings)

    int m_maxThreads;
    bool m_showFPS;
    Language m_language;
    Theme m_theme;
    int m_renderWidth;
    int m_renderHeight;
    double m_viewportMinU;
    double m_viewportMinV;
    double m_viewportMaxU;
    double m_viewportMaxV;
};

#endif // APPSETTINGS_H

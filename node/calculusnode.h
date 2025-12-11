#ifndef CALCULUSNODE_H
#define CALCULUSNODE_H

#include "node.h"
#include <QRecursiveMutex>

// 微分積分ノード - 初心者にもわかりやすい微積分操作
// Calculus Node - Beginner-friendly differential and integral operations
class CalculusNode : public Node {
public:
    // 操作モード
    enum class Mode {
        DerivativeX,      // X方向の微分（横方向の変化率）
        DerivativeY,      // Y方向の微分（縦方向の変化率）
        Gradient,         // 勾配の大きさ（全体的な変化率）
        Laplacian,        // ラプラシアン（エッジ検出）
        // 積分は複雑なため、簡易版を提供
        IntegralX,        // X方向の累積和
        IntegralY         // Y方向の累積和
    };

    CalculusNode();
    ~CalculusNode() override;

    QVector<ParameterInfo> parameters() const override;
    void evaluate() override;
    QVariant compute(const QVector3D& pos, NodeSocket* socket) override;
    void setDirty(bool dirty) override;
    
    QJsonObject save() const override;
    void restore(const QJsonObject& json) override;

    // Getters
    Mode mode() const;
    double sampleDistance() const;
    double scale() const;
    bool normalize() const;

    // Setters
    void setMode(Mode m);
    void setSampleDistance(double d);
    void setScale(double s);
    void setNormalize(bool n);

private:
    // 近傍サンプリングによる微分計算
    double sampleValue(const QVector3D& pos);
    double computeDerivativeX(const QVector3D& pos, double h);
    double computeDerivativeY(const QVector3D& pos, double h);
    double computeGradient(const QVector3D& pos, double h);
    double computeLaplacian(const QVector3D& pos, double h);
    
    NodeSocket* m_valueInput;        // 入力値
    NodeSocket* m_vectorInput;       // 座標入力（オプション）
    NodeSocket* m_sampleDistInput;   // サンプリング距離
    NodeSocket* m_scaleInput;        // 出力スケール
    NodeSocket* m_normalizeInput;    // 正規化フラグ

    NodeSocket* m_facOutput;         // 数値出力
    NodeSocket* m_colorOutput;       // カラー出力

    Mode m_mode;
    mutable QRecursiveMutex m_mutex;
};

#endif // CALCULUSNODE_H

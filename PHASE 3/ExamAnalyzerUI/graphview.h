#pragma once

#include <QHash>
#include <QPair>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>







class GraphView : public QWidget
{
    Q_OBJECT

public:
    struct Node {
        int topic;
        QString name;
        double priority;   
    };

    explicit GraphView(QWidget *parent = nullptr);

    
    void setGraph(const QVector<Node> &nodes, const QVector<QPair<int, int>> &edges,
                  const QVector<int> &order);
    void setSelection(int topic, const QSet<int> &before, const QSet<int> &after);

signals:
    void topicClicked(int topic);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void layoutNodes();
    int nodeAt(const QPointF &pos) const;

    QVector<Node> nodes;
    QVector<QPair<int, int>> edges;
    QVector<int> order;
    QHash<int, QRectF> rects;          
    QHash<int, int> indexOf;           

    int selected = -1;
    QSet<int> before, after;
    int hovered = -1;

    static constexpr double kNodeW = 156;
    static constexpr double kNodeH = 40;
    static constexpr double kGapX = 48;
    static constexpr double kGapY = 14;
    static constexpr double kMargin = 16;
};

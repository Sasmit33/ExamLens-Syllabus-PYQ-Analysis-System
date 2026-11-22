#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QVariantAnimation;







class PriorityChart : public QWidget
{
    Q_OBJECT

public:
    struct Bar {
        int topic;
        QString label;
        double value;       
        QString details;    
    };

    explicit PriorityChart(QWidget *parent = nullptr);

    void setBars(const QVector<Bar> &bars);   
    void setHighlighted(int topic);           

signals:
    void topicClicked(int topic);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sizeHint() const override;

private:
    QRectF rowRect(int i) const;
    int rowAt(const QPointF &pos) const;

    QVector<Bar> bars;
    QVector<double> startValues;   
    QVector<double> shownValues;   
    QVariantAnimation *animation;
    int hoverRow = -1;
    int highlightedTopic = -1;

    static constexpr int kRowHeight = 28;
    static constexpr int kLabelWidth = 200;
    static constexpr int kValueWidth = 54;
};

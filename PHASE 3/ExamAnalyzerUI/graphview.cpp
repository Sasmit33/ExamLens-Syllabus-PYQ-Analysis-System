#include "graphview.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QToolTip>

#include <algorithm>

namespace {

QColor blend(const QColor &a, const QColor &b, double t)
{
    t = qBound(0.0, t, 1.0);
    return QColor(int(a.red() + (b.red() - a.red()) * t),
                  int(a.green() + (b.green() - a.green()) * t),
                  int(a.blue() + (b.blue() - a.blue()) * t));
}

const QColor kLow(238, 244, 251);      
const QColor kHigh(20, 55, 105);       
const QColor kSelected(192, 57, 43);   
const QColor kBefore(217, 130, 43);    
const QColor kAfter(46, 139, 122);     

} 

GraphView::GraphView(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
}

void GraphView::setGraph(const QVector<Node> &n, const QVector<QPair<int, int>> &e,
                         const QVector<int> &o)
{
    nodes = n;
    edges = e;
    order = o;
    indexOf.clear();
    for (int i = 0; i < nodes.size(); i++) indexOf.insert(nodes[i].topic, i);
    layoutNodes();
    update();
}

void GraphView::setSelection(int topic, const QSet<int> &b, const QSet<int> &a)
{
    selected = topic;
    before = b;
    after = a;
    update();
}

void GraphView::layoutNodes()
{
    rects.clear();
    if (nodes.isEmpty()) return;

    
    
    QHash<int, QVector<int>> preds;
    for (const auto &edge : edges) preds[edge.second].append(edge.first);

    QHash<int, int> layer;
    int maxLayer = 0;
    for (int t : order) {
        int l = 0;
        for (int u : preds.value(t)) l = qMax(l, layer.value(u, 0) + 1);
        layer.insert(t, l);
        maxLayer = qMax(maxLayer, l);
    }
    for (const Node &node : nodes)                 
        if (!layer.contains(node.topic)) layer.insert(node.topic, maxLayer + 1);
    for (int l : layer) maxLayer = qMax(maxLayer, l);

    
    QVector<QVector<int>> columns(maxLayer + 1);
    for (const Node &node : nodes) columns[layer.value(node.topic)].append(node.topic);
    int maxRows = 0;
    for (auto &col : columns) {
        std::sort(col.begin(), col.end(), [this](int a, int b) {
            return nodes[indexOf.value(a)].priority > nodes[indexOf.value(b)].priority;
        });
        maxRows = qMax(maxRows, int(col.size()));
    }

    const double contentW = 2 * kMargin + columns.size() * kNodeW + (columns.size() - 1) * kGapX;
    const double contentH = 2 * kMargin + maxRows * (kNodeH + kGapY) - kGapY;
    setMinimumSize(int(contentW), int(contentH));

    const double offsetX = qMax(0.0, (width() - contentW) / 2);
    for (int c = 0; c < columns.size(); c++) {
        const double colH = columns[c].size() * (kNodeH + kGapY) - kGapY;
        const double top = kMargin + (contentH - 2 * kMargin - colH) / 2;
        for (int r = 0; r < columns[c].size(); r++) {
            const double x = offsetX + kMargin + c * (kNodeW + kGapX);
            const double y = top + r * (kNodeH + kGapY);
            rects.insert(columns[c][r], QRectF(x, y, kNodeW, kNodeH));
        }
    }
}

int GraphView::nodeAt(const QPointF &pos) const
{
    for (auto it = rects.constBegin(); it != rects.constEnd(); ++it)
        if (it.value().contains(pos)) return it.key();
    return -1;
}

void GraphView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (nodes.isEmpty()) {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(rect(), Qt::AlignCenter, "No prerequisite data loaded");
        return;
    }

    const bool focus = (selected >= 0);
    QSet<int> upChain = before;  upChain.insert(selected);   
    QSet<int> downChain = after; downChain.insert(selected); 

    
    for (const auto &edge : edges) {
        if (!rects.contains(edge.first) || !rects.contains(edge.second)) continue;
        const QRectF a = rects.value(edge.first), b = rects.value(edge.second);
        const QPointF start(a.right(), a.center().y()), end(b.left(), b.center().y());
        const double dx = (end.x() - start.x()) / 2;

        QPainterPath path(start);
        path.cubicTo(start + QPointF(dx, 0), end - QPointF(dx, 0), end);

        QColor colour = palette().color(QPalette::Mid);
        double widthPx = 1.4;
        if (focus) {
            if (upChain.contains(edge.first) && upChain.contains(edge.second)) { colour = kBefore; widthPx = 2.4; }
            else if (downChain.contains(edge.first) && downChain.contains(edge.second)) { colour = kAfter; widthPx = 2.4; }
            else colour.setAlpha(60);
        }
        p.setPen(QPen(colour, widthPx));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        QPolygonF head;                                   
        head << end << end + QPointF(-8, -4) << end + QPointF(-8, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(colour);
        p.drawPolygon(head);
    }

    
    for (const Node &node : nodes) {
        const QRectF r = rects.value(node.topic);
        const bool isSel = (node.topic == selected);
        const bool isBefore = before.contains(node.topic);
        const bool isAfter = after.contains(node.topic);
        const bool related = isSel || isBefore || isAfter;

        p.setOpacity(focus && !related ? 0.28 : 1.0);    

        QPen border(palette().color(QPalette::Mid), 1);
        if (isSel) border = QPen(kSelected, 3);
        else if (focus && isBefore) border = QPen(kBefore, 2.5);
        else if (focus && isAfter) border = QPen(kAfter, 2.5);
        else if (node.topic == hovered) border = QPen(palette().color(QPalette::Highlight), 2);

        p.setPen(border);
        p.setBrush(blend(kLow, kHigh, node.priority));
        p.drawRoundedRect(r, 8, 8);

        p.setPen(node.priority > 0.5 ? QColor(Qt::white) : QColor(27, 42, 73));
        const QString name = fontMetrics().elidedText(node.name, Qt::ElideRight, int(r.width()) - 12);
        p.drawText(r.adjusted(6, 2, -6, -14), Qt::AlignCenter, name);
        QFont small = font();
        small.setPointSizeF(font().pointSizeF() * 0.8);
        p.setFont(small);
        p.drawText(r.adjusted(6, 18, -6, -2), Qt::AlignCenter, QString::number(node.priority, 'f', 2));
        p.setFont(font());
    }
    p.setOpacity(1.0);
}

void GraphView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    const int topic = nodeAt(event->position());
    if (topic >= 0) emit topicClicked(topic);
}

void GraphView::mouseMoveEvent(QMouseEvent *event)
{
    const int topic = nodeAt(event->position());
    if (topic != hovered) {
        hovered = topic;
        update();
    }
    if (topic >= 0) {
        setCursor(Qt::PointingHandCursor);
        const Node &n = nodes[indexOf.value(topic)];
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1\nPriority %2\nClick to see its prerequisites")
                               .arg(n.name).arg(n.priority, 0, 'f', 3), this);
    } else {
        unsetCursor();
        QToolTip::hideText();
    }
}

void GraphView::resizeEvent(QResizeEvent *)
{
    layoutNodes();   
}

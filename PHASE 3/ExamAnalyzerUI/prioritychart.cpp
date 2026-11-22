#include "prioritychart.h"

#include <QEasingCurve>
#include <QHash>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QVariantAnimation>

PriorityChart::PriorityChart(QWidget *parent)
    : QWidget(parent), animation(new QVariantAnimation(this))
{
    setMouseTracking(true);                       
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(450);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    
    connect(animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        const double t = v.toDouble();
        for (int i = 0; i < bars.size(); i++)
            shownValues[i] = startValues[i] + (bars[i].value - startValues[i]) * t;
        update();                                 
    });
}

void PriorityChart::setBars(const QVector<Bar> &newBars)
{
    
    
    QHash<int, double> previous;
    for (int i = 0; i < bars.size(); i++) previous.insert(bars[i].topic, shownValues[i]);

    bars = newBars;
    startValues.resize(bars.size());
    shownValues.resize(bars.size());
    for (int i = 0; i < bars.size(); i++) {
        startValues[i] = previous.value(bars[i].topic, 0.0);
        shownValues[i] = startValues[i];
    }
    setMinimumHeight(sizeHint().height());
    updateGeometry();
    animation->stop();
    animation->start();
}

void PriorityChart::setHighlighted(int topic)
{
    highlightedTopic = topic;
    update();
}

QSize PriorityChart::sizeHint() const
{
    return QSize(520, 12 + int(bars.size()) * kRowHeight);
}

QRectF PriorityChart::rowRect(int i) const
{
    return QRectF(0, 6 + i * kRowHeight, width(), kRowHeight);
}

int PriorityChart::rowAt(const QPointF &pos) const
{
    for (int i = 0; i < bars.size(); i++)
        if (rowRect(i).contains(pos)) return i;
    return -1;
}

void PriorityChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QColor text = palette().color(QPalette::WindowText);
    const QColor track = palette().color(QPalette::Mid).lighter(150);
    const QColor bar(62, 92, 153);
    const QColor accent(192, 57, 43);

    const double barX = kLabelWidth + 8;
    const double barMax = qMax(40.0, width() - barX - kValueWidth - 8);

    for (int i = 0; i < bars.size(); i++) {
        const QRectF row = rowRect(i);
        const bool hovered = (i == hoverRow);
        const bool marked = (bars[i].topic == highlightedTopic);

        if (hovered) {                            
            QColor bg = palette().color(QPalette::Highlight);
            bg.setAlpha(35);
            p.fillRect(row, bg);
        }

        
        QFont f = font();
        f.setBold(marked);
        p.setFont(f);
        p.setPen(text);
        const QString name = fontMetrics().elidedText(bars[i].label, Qt::ElideRight, kLabelWidth - 10);
        p.drawText(QRectF(8, row.top(), kLabelWidth - 8, row.height()), Qt::AlignVCenter | Qt::AlignLeft, name);

        
        const QRectF trackRect(barX, row.center().y() - 6, barMax, 12);
        p.setPen(Qt::NoPen);
        p.setBrush(track);
        p.drawRoundedRect(trackRect, 6, 6);

        QColor fill = marked ? accent : (hovered ? bar.lighter(125) : bar);
        p.setBrush(fill);
        const double w = qBound(0.0, shownValues[i], 1.0) * barMax;
        if (w > 0.5) p.drawRoundedRect(QRectF(barX, trackRect.top(), w, 12), 6, 6);

        p.setPen(text);
        p.drawText(QRectF(barX + barMax + 6, row.top(), kValueWidth, row.height()),
                   Qt::AlignVCenter | Qt::AlignLeft, QString::number(shownValues[i], 'f', 3));
    }

    if (bars.isEmpty()) {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(rect(), Qt::AlignCenter, "Run the analysis to see topic priorities");
    }
}

void PriorityChart::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAt(event->position());
    if (row != hoverRow) {
        hoverRow = row;
        update();
    }
    if (row >= 0) {
        setCursor(Qt::PointingHandCursor);
        QToolTip::showText(event->globalPosition().toPoint(), bars[row].details, this);
    } else {
        unsetCursor();
        QToolTip::hideText();
    }
}

void PriorityChart::mousePressEvent(QMouseEvent *event)
{
    const int row = rowAt(event->position());
    if (row >= 0 && event->button() == Qt::LeftButton) emit topicClicked(bars[row].topic);
}

void PriorityChart::leaveEvent(QEvent *)
{
    hoverRow = -1;
    unsetCursor();
    update();
}

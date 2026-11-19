#include "CanvasView.h"
#include "Compositor.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>

using namespace spx;

CanvasView::CanvasView(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::WheelFocus);
    setCursor(Qt::CrossCursor);
    m_ants.setInterval(120);
    connect(&m_ants, &QTimer::timeout, this, [this] {
        if (!m_sel.isActive()) return;
        m_antOffset = (m_antOffset + 1) % 8;
        update();
    });
    m_ants.start();
}

void CanvasView::setDocument(Document *doc)
{
    if (m_doc) disconnect(m_doc, nullptr, this, nullptr);
    m_doc = doc;
    m_ctx.doc = doc;
    m_ctx.layer = m_ctx.frame = 0;
    m_sel = {};
    m_drawButton = Qt::NoButton;
    m_pan = {};
    m_cachedFrame = -1;
    if (doc) connect(doc, &Document::changed, this, [this] { m_cachedFrame = -1; update(); });
    update();
    emit contextChanged();
}

void CanvasView::setOnionSkin(bool on, int before, int after)
{
    m_onion = on;
    m_onionN[0] = qBound(1, before, 3);
    m_onionN[1] = qBound(1, after, 3);
    m_cachedFrame = -1;
    update();
}

void CanvasView::setZoomAt(int factor, QPointF anchor)
{
    factor = qBound(1, factor, 64);
    if (factor == m_zoom || !m_doc) return;
    const QSize s = m_doc->sprite().size;
    const QPointF canvasPt = (anchor - origin()) / m_zoom; // keep this canvas point under the anchor
    m_zoom = factor;
    const QPointF centered((width() - s.width() * m_zoom) / 2.0, (height() - s.height() * m_zoom) / 2.0);
    m_pan = anchor - canvasPt * m_zoom - centered;
    update();
    emit zoomChanged(m_zoom);
}

QPoint CanvasView::origin() const
{
    const QSize s = m_doc ? m_doc->sprite().size : QSize();
    return (QPointF((width() - s.width() * m_zoom) / 2.0, (height() - s.height() * m_zoom) / 2.0) + m_pan).toPoint();
}

QPoint CanvasView::toCanvas(QPointF screen) const
{
    const QPointF d = (screen - origin()) / m_zoom;
    return {int(std::floor(d.x())), int(std::floor(d.y()))};
}

QRect CanvasView::toScreen(QRect r) const
{
    return {origin() + r.topLeft() * m_zoom, r.size() * m_zoom};
}

void CanvasView::recomposite()
{
    const Sprite &s = m_doc->sprite();
    const int f = m_ctx.frame;
    m_composite = compositeFrame(s, f);
    m_onionBefore.clear();
    m_onionAfter.clear();
    for (int i = 1; m_onion && i <= m_onionN[0] && f - i >= 0; ++i) m_onionBefore << compositeFrame(s, f - i);
    for (int i = 1; m_onion && i <= m_onionN[1] && f + i < s.frames.size(); ++i) m_onionAfter << compositeFrame(s, f + i);
    m_cachedFrame = f;
}

void CanvasView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0x1e1f22));
    if (!m_doc) return;
    if (m_cachedFrame != m_ctx.frame) recomposite();
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    const Sprite &s = m_doc->sprite();
    const QRect canvas = toScreen(QRect(QPoint(), s.size));
    const QRect visible = canvas & rect();
    const QPoint o = origin();

    static const QPixmap checker = [] {
        QPixmap pm(16, 16);
        pm.fill(QColor(0x2b2b2b));
        QPainter cp(&pm);
        cp.fillRect(8, 0, 8, 8, QColor(0x3a3a3a));
        cp.fillRect(0, 8, 8, 8, QColor(0x3a3a3a));
        return pm;
    }();
    p.setBrushOrigin(o);
    p.fillRect(visible, QBrush(checker));

    p.save();
    p.setClipRect(canvas);
    static const qreal onionAlpha[] = {0.3, 0.2, 0.1};
    for (int i = 0; i < m_onionBefore.size(); ++i) { p.setOpacity(onionAlpha[i]); p.drawImage(canvas, m_onionBefore[i]); }
    for (int i = 0; i < m_onionAfter.size(); ++i) { p.setOpacity(onionAlpha[i]); p.drawImage(canvas, m_onionAfter[i]); }
    p.setOpacity(1.0);
    p.drawImage(canvas, m_composite);
    if (m_sel.isFloating()) p.drawImage(toScreen(QRect(m_sel.floatingPos, m_sel.floating.size())), m_sel.floating);
    if (m_tool)
        for (const QPoint &pt : m_tool->preview()) p.fillRect(QRect(o + pt * m_zoom, QSize(m_zoom, m_zoom)), m_ctx.primary);

    // Only the visible pixel range is drawn: a 2048 canvas at 64x would otherwise mean 4096 lines per frame.
    const QPoint v0 = toCanvas(visible.topLeft()), v1 = toCanvas(visible.bottomRight());
    if (m_grid && m_zoom >= 4) {
        p.setPen(QColor(255, 255, 255, 45));
        for (int x = v0.x() + 1; x <= v1.x(); ++x) p.drawLine(o.x() + x * m_zoom, visible.top(), o.x() + x * m_zoom, visible.bottom());
        for (int y = v0.y() + 1; y <= v1.y(); ++y) p.drawLine(visible.left(), o.y() + y * m_zoom, visible.right(), o.y() + y * m_zoom);
    }
    if (s.sliceGrid) {
        const SliceGrid &g = *s.sliceGrid;
        const int sx = g.cellWidth + g.spacingX, sy = g.cellHeight + g.spacingY;
        p.setPen(QColor(0x3d, 0x8b, 0xfd, 170));
        p.setBrush(Qt::NoBrush);
        for (int y = g.offsetY + qMax(0, (v0.y() - g.offsetY) / sy) * sy; y <= v1.y() && y + g.cellHeight <= s.size.height(); y += sy)
            for (int x = g.offsetX + qMax(0, (v0.x() - g.offsetX) / sx) * sx; x <= v1.x() && x + g.cellWidth <= s.size.width(); x += sx)
                p.drawRect(toScreen(QRect(x, y, g.cellWidth, g.cellHeight)).adjusted(0, 0, -1, -1));
    }
    p.restore();

    if (m_sel.isActive()) {
        const QRect r = toScreen(m_sel.rect).adjusted(0, 0, -1, -1);
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(r);
        QPen ants(Qt::black, 1, Qt::DashLine);
        ants.setDashOffset(m_antOffset);
        p.setPen(ants);
        p.drawRect(r);
    }
}

void CanvasView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && m_space)) {
        m_panning = true;
        m_lastPan = e->position();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (!m_doc || !m_tool || m_drawButton != Qt::NoButton || !isDrawButton(e->button())) return;
    m_drawButton = e->button();
    m_ctx.constrain = e->modifiers() & Qt::ShiftModifier;
    m_tool->press(m_ctx, toCanvas(e->position()), e->button());
    update();
    emit contextChanged(); // the eyedropper changes colors
}

void CanvasView::mouseMoveEvent(QMouseEvent *e)
{
    if (m_panning) {
        m_pan += e->position() - m_lastPan;
        m_lastPan = e->position();
        update();
    } else if (m_drawButton != Qt::NoButton) {
        m_ctx.constrain = e->modifiers() & Qt::ShiftModifier;
        m_tool->move(m_ctx, toCanvas(e->position()));
        update();
    }
}

void CanvasView::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_panning && (e->button() == Qt::MiddleButton || e->button() == Qt::LeftButton)) {
        m_panning = false;
        setCursor(m_space ? Qt::OpenHandCursor : Qt::CrossCursor);
    } else if (e->button() == m_drawButton) {
        m_drawButton = Qt::NoButton;
        m_tool->release(m_ctx, toCanvas(e->position()));
        update();
        emit contextChanged();
    }
}

void CanvasView::wheelEvent(QWheelEvent *e)
{
    const int dy = e->angleDelta().y();
    if (dy) setZoomAt(dy > 0 ? m_zoom * 2 : m_zoom / 2, e->position());
}

void CanvasView::keyPressEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_Space) return QWidget::keyPressEvent(e);
    if (!e->isAutoRepeat()) { m_space = true; if (!m_panning) setCursor(Qt::OpenHandCursor); }
}

void CanvasView::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_Space) return QWidget::keyReleaseEvent(e);
    if (!e->isAutoRepeat()) { m_space = false; if (!m_panning) setCursor(Qt::CrossCursor); }
}

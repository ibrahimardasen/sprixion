#pragma once
#include "Selection.h"
#include "tools/Tool.h"
#include <QImage>
#include <QTimer>
#include <QVector>
#include <QWidget>

class CanvasView : public QWidget
{
    Q_OBJECT
public:
    explicit CanvasView(QWidget *parent = nullptr);
    void setDocument(spx::Document *doc); // resets selection, layer and frame
    void setTool(spx::Tool *tool) { m_tool = tool; update(); }
    spx::ToolContext &context() { return m_ctx; } // change layer/frame here, then call update()
    spx::Selection &selection() { return m_sel; }
    int zoom() const { return m_zoom; }
    void setZoom(int factor) { setZoomAt(factor, rect().center()); }
    void setGridVisible(bool on) { m_grid = on; update(); }
    void setOnionSkin(bool on, int before, int after);

signals:
    void contextChanged();
    void zoomChanged(int);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;

private:
    void setZoomAt(int factor, QPointF anchor);
    QPoint origin() const; // screen position of canvas pixel (0,0)
    QPoint toCanvas(QPointF screen) const;
    QRect toScreen(QRect canvas) const;
    void recomposite();

    spx::Document *m_doc = nullptr;
    spx::Tool *m_tool = nullptr;
    spx::ToolContext m_ctx{nullptr};
    spx::Selection m_sel;
    QImage m_composite;
    QVector<QImage> m_onionBefore, m_onionAfter; // [0] = nearest frame
    int m_cachedFrame = -1;                      // -1 = cache stale
    int m_zoom = 8;
    QPointF m_pan;
    bool m_grid = false, m_onion = false;
    int m_onionN[2] = {1, 1};
    bool m_space = false, m_panning = false;
    QPointF m_lastPan;
    Qt::MouseButton m_drawButton = Qt::NoButton;
    QTimer m_ants;
    int m_antOffset = 0;
};

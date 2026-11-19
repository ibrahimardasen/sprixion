#include "ColorPicker.h"
#include "CanvasView.h"
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QSlider>
#include <QVBoxLayout>
#include <functional>

using namespace spx;

// A custom-painted widget: the owner supplies painting and mouse handling as callbacks.
class ColorPicker::Surface : public QWidget
{
public:
    std::function<void(QPainter &, const QRect &)> paint;
    std::function<void(QPoint)> mouse;

protected:
    void paintEvent(QPaintEvent *) override { QPainter p(this); paint(p, rect()); }
    void mousePressEvent(QMouseEvent *e) override { mouse(e->position().toPoint()); }
    void mouseMoveEvent(QMouseEvent *e) override { if (e->buttons() & Qt::LeftButton) mouse(e->position().toPoint()); }
};

QColor ColorPicker::parseHex(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^#?([0-9a-fA-F]{6})([0-9a-fA-F]{2})?$"));
    const auto m = re.match(text.trimmed());
    if (!m.hasMatch()) return {};
    QColor c(QLatin1Char('#') + m.captured(1));
    if (!m.captured(2).isEmpty()) c.setAlpha(m.captured(2).toInt(nullptr, 16));
    return c;
}

QString ColorPicker::toHex(const QColor &c)
{
    QString s = QStringLiteral("#%1").arg(c.rgb() & 0xffffff, 6, 16, QLatin1Char('0')).toUpper();
    if (c.alpha() != 255) s += QStringLiteral("%1").arg(c.alpha(), 2, 16, QLatin1Char('0')).toUpper();
    return s;
}

ColorPicker::ColorPicker(CanvasView *canvas, QWidget *parent)
    : QWidget(parent), m_canvas(canvas), m_sv(new Surface), m_swatches(new Surface),
      m_hue(new QSlider(Qt::Horizontal)), m_alpha(new QSlider(Qt::Horizontal)), m_hex(new QLineEdit)
{
    m_sv->setMinimumSize(120, 120);
    m_sv->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_sv->setCursor(Qt::CrossCursor);
    m_sv->paint = [this](QPainter &p, const QRect &r) {
        QImage img(r.size(), QImage::Format_RGB32); // shortcut: repainted per-pixel each time; fine at dock size
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x)
                img.setPixelColor(x, y, QColor::fromHsv(m_h, x * 255 / qMax(1, img.width() - 1), 255 - y * 255 / qMax(1, img.height() - 1)));
        p.drawImage(0, 0, img);
        const QPoint c(m_s * (r.width() - 1) / 255, (255 - m_v) * (r.height() - 1) / 255);
        p.setPen(Qt::white); p.drawEllipse(c, 5, 5);
        p.setPen(Qt::black); p.drawEllipse(c, 6, 6);
    };
    m_sv->mouse = [this](QPoint pt) {
        m_s = qBound(0, pt.x() * 255 / qMax(1, m_sv->width() - 1), 255);
        m_v = 255 - qBound(0, pt.y() * 255 / qMax(1, m_sv->height() - 1), 255);
        fromHsv();
    };

    m_swatches->setFixedSize(56, 40);
    m_swatches->setToolTip(tr("Primary and secondary colors (click to edit, X swaps)"));
    m_swatches->paint = [this](QPainter &p, const QRect &) {
        const ToolContext &c = m_canvas->context();
        const QRect pr(0, 0, 30, 30), sr(24, 10, 30, 30);
        auto draw = [&](const QRect &r, const QColor &col, bool active) {
            p.fillRect(r, Qt::white); // opaque backing so alpha is visible
            p.fillRect(r, col);
            p.setPen(active ? QColor(0x3d8bfd) : QColor(0x43454a));
            p.drawRect(r.adjusted(0, 0, -1, -1));
        };
        if (m_editSecondary) { draw(pr, c.primary, false); draw(sr, c.secondary, true); }
        else { draw(sr, c.secondary, false); draw(pr, c.primary, true); }
    };
    m_swatches->mouse = [this](QPoint pt) {
        const bool inP = QRect(0, 0, 30, 30).contains(pt), inS = QRect(24, 10, 30, 30).contains(pt);
        if (inP && inS) return; // overlap belongs to the square drawn on top, which is already the active one
        if (inP || inS) { m_editSecondary = inS; refresh(); }
    };

    m_hue->setRange(0, 359);
    m_alpha->setRange(0, 255);
    m_hue->setToolTip(tr("Hue"));
    m_alpha->setToolTip(tr("Alpha"));
    m_hex->setMaxLength(9);
    m_hex->setToolTip(tr("#RRGGBB or #RRGGBBAA"));

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_swatches);
    bottom->addWidget(m_hex);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(m_sv, 1);
    layout->addWidget(m_hue);
    layout->addWidget(m_alpha);
    layout->addLayout(bottom);

    connect(m_hue, &QSlider::valueChanged, this, [this](int v) { if (!m_syncing) { m_h = v; fromHsv(); } });
    connect(m_alpha, &QSlider::valueChanged, this, [this](int a) {
        if (m_syncing) return;
        QColor c = target();
        c.setAlpha(a);
        setColor(c);
    });
    connect(m_hex, &QLineEdit::editingFinished, this, [this] {
        const QColor c = parseHex(m_hex->text());
        if (c.isValid()) setColor(c);
        else refresh(); // invalid: revert
    });
    connect(canvas, &CanvasView::contextChanged, this, &ColorPicker::refresh);
    refresh();
}

QColor &ColorPicker::target()
{
    ToolContext &c = m_canvas->context();
    return m_editSecondary ? c.secondary : c.primary;
}

void ColorPicker::fromHsv()
{
    setColor(QColor::fromHsv(m_h, m_s, m_v, target().alpha()));
}

void ColorPicker::setColor(const QColor &c)
{
    target() = c;
    refresh();
}

void ColorPicker::swap()
{
    ToolContext &c = m_canvas->context();
    std::swap(c.primary, c.secondary);
    refresh();
}

void ColorPicker::refresh()
{
    const QColor c = target();
    m_syncing = true;
    if (c.hsvHue() >= 0) m_h = c.hsvHue(); // grays have no hue: keep the last one so the slider doesn't jump
    m_s = c.hsvSaturation();
    m_v = c.value();
    m_hue->setValue(m_h);
    m_alpha->setValue(c.alpha());
    if (!m_hex->hasFocus() || parseHex(m_hex->text()) != c) m_hex->setText(toHex(c));
    m_syncing = false;
    m_sv->update();
    m_swatches->update();
}

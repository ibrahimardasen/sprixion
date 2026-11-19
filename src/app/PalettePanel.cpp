#include "PalettePanel.h"
#include "CanvasView.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

using namespace spx;

class PalettePanel::Grid : public QWidget
{
public:
    static constexpr int Cell = 16;
    std::function<void(int, Qt::MouseButton)> onClick;
    QVector<QColor> colors;
    int selected = -1;

    void setColors(const QVector<QColor> &c)
    {
        colors = c;
        if (selected >= c.size()) selected = -1;
        fit();
        update();
    }

protected:
    int cols() const { return qMax(1, width() / Cell); }
    // shortcut: no scrollbar-width feedback, a scrollbar may cost one column; fine for palettes up to ~256 colors.
    void fit() { setMinimumHeight(((colors.size() + cols() - 1) / cols()) * Cell); }
    void resizeEvent(QResizeEvent *) override { fit(); }
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        for (int i = 0; i < colors.size(); ++i) {
            const QRect r(i % cols() * Cell, i / cols() * Cell, Cell, Cell);
            p.fillRect(r, colors[i]);
            if (i == selected) { p.setPen(Qt::white); p.drawRect(r.adjusted(0, 0, -1, -1)); p.setPen(Qt::black); p.drawRect(r.adjusted(1, 1, -2, -2)); }
        }
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        const int x = e->position().toPoint().x() / Cell, y = e->position().toPoint().y() / Cell;
        const int i = y * cols() + x;
        if (x < cols() && i >= 0 && i < colors.size() && onClick) onClick(i, e->button());
    }
};

PalettePanel::PalettePanel(CanvasView *canvas, QWidget *parent) : QWidget(parent), m_canvas(canvas), m_grid(new Grid)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(m_grid);
    scroll->setMinimumHeight(80);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(scroll);
    auto *row = new QHBoxLayout;
    layout->addLayout(row);
    auto *add = new QPushButton(QStringLiteral("+"));
    add->setToolTip(tr("Add the primary color to the palette"));
    auto *del = new QPushButton(QStringLiteral("−"));
    del->setToolTip(tr("Remove the selected swatch"));
    row->addWidget(add);
    row->addWidget(del);
    row->addStretch();

    m_grid->onClick = [this](int i, Qt::MouseButton b) {
        ToolContext &c = m_canvas->context();
        if (b == Qt::LeftButton) c.primary = m_grid->colors[i];
        else if (b == Qt::RightButton) c.secondary = m_grid->colors[i];
        else return;
        m_grid->selected = i;
        m_grid->update();
        emit colorsChanged();
    };
    connect(add, &QPushButton::clicked, this, [this] {
        editPalette(tr("Add Palette Color"), [this](QVector<QColor> &p) { p.append(m_canvas->context().primary); });
    });
    connect(del, &QPushButton::clicked, this, [this] {
        const int i = m_grid->selected;
        if (i >= 0) editPalette(tr("Remove Palette Color"), [i](QVector<QColor> &p) { p.remove(i); });
    });
}

void PalettePanel::editPalette(const QString &text, const std::function<void(QVector<QColor> &)> &edit)
{
    if (!m_doc) return;
    Sprite s = m_doc->sprite();
    edit(s.palette);
    m_doc->apply(text, std::move(s));
}

void PalettePanel::setDocument(Document *doc)
{
    QObject::disconnect(m_conn);
    m_doc = doc;
    const auto show = [this] { m_grid->selected = -1; m_grid->setColors(m_doc->sprite().palette); };
    m_conn = connect(doc, &Document::changed, this, [show](const QRect &r) { if (r.isNull()) show(); });
    show();
}

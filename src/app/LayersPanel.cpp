#include "LayersPanel.h"
#include "CanvasView.h"
#include "SpriteOps.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

using namespace spx;

LayersPanel::LayersPanel(CanvasView *canvas, std::function<void()> settle, std::function<void(int)> select, QWidget *parent)
    : QWidget(parent), m_canvas(canvas), m_settle(std::move(settle)), m_select(std::move(select)), m_list(new QListWidget), m_opacity(new QSlider(Qt::Horizontal))
{
    m_opacity->setRange(0, 255);
    m_opacity->setToolTip(tr("Layer opacity"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(m_list);
    layout->addWidget(m_opacity);
    auto *row = new QHBoxLayout;
    layout->addLayout(row);
    auto button = [&](const QString &text, const QString &tip, auto fn) {
        auto *b = new QPushButton(text);
        b->setToolTip(tip);
        connect(b, &QPushButton::clicked, this, fn);
        row->addWidget(b);
    };
    const auto cur = [this] { return m_canvas->context().layer; };
    button(QStringLiteral("+"), tr("Add layer"), [this, cur] {
        if (!m_doc) return;
        m_settle();
        const int at = cur() + 1;
        m_canvas->context().layer = at; // before apply: the rebuild it triggers then selects the new row
        m_doc->apply(tr("Add Layer"), addLayer(m_doc->sprite(), at));
    });
    button(QStringLiteral("−"), tr("Delete layer"), [this, cur] {
        if (!m_doc || m_doc->sprite().layers.size() <= 1) return;
        m_settle();
        m_doc->apply(tr("Delete Layer"), removeLayer(m_doc->sprite(), cur())); // the canvas clamps the current layer
    });
    auto move = [this, cur](int delta) {
        const int to = cur() + delta;
        if (!m_doc || to < 0 || to >= m_doc->sprite().layers.size()) return;
        m_settle();
        m_canvas->context().layer = to;
        m_doc->apply(tr("Move Layer"), moveLayer(m_doc->sprite(), to - delta, to));
    };
    button(QStringLiteral("↑"), tr("Move layer up"), [move] { move(1); });
    button(QStringLiteral("↓"), tr("Move layer down"), [move] { move(-1); });

    connect(m_list, &QListWidget::currentRowChanged, this, &LayersPanel::rowChanged);
    connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem *it) {
        if (m_syncing || !m_doc) return;
        const int layer = m_list->count() - 1 - m_list->row(it);
        const Layer &l = m_doc->sprite().layers[layer];
        const QString name = it->text();
        const bool visible = it->checkState() == Qt::Checked;
        if (name == l.name && visible == l.visible) return;
        m_doc->apply(visible != l.visible ? tr("Toggle Layer Visibility") : tr("Rename Layer"),
                     setLayerProps(m_doc->sprite(), layer, name, visible, l.opacity));
    });
    // A drag previews without undo entries, then restores and applies once; keys and wheel apply per change.
    connect(m_opacity, &QSlider::sliderPressed, this, [this] { if (!m_doc) return; m_dragging = true; m_beforeDrag = m_doc->sprite(); });
    connect(m_opacity, &QSlider::sliderMoved, this, [this](int v) { setProps(m_canvas->context().layer, m_beforeDrag, v, false); });
    connect(m_opacity, &QSlider::sliderReleased, this, [this] {
        m_dragging = false;
        setProps(m_canvas->context().layer, m_beforeDrag, m_opacity->value(), true);
    });
    connect(m_opacity, &QSlider::valueChanged, this, [this](int v) {
        if (!m_syncing && !m_opacity->isSliderDown() && m_doc) setProps(m_canvas->context().layer, m_doc->sprite(), v, true);
    });
}

// `base` is the sprite the opacity is applied to; for a drag it is the pre-drag sprite.
void LayersPanel::setProps(int layer, const Sprite &base, int opacity, bool apply)
{
    if (!m_doc) return;
    const Layer &l = base.layers[layer];
    const int old = l.opacity;
    Sprite s = setLayerProps(base, layer, l.name, l.visible, opacity);
    if (!apply) { m_doc->previewSprite(std::move(s)); return; }
    // A drag's base is the saved pre-drag copy: restore it so undo's "before" is right (keyboard base is the live sprite).
    if (&base != &m_doc->sprite()) m_doc->previewSprite(base); // undo's "before" is the pre-drag sprite
    if (opacity != old) m_doc->apply(tr("Layer Opacity"), std::move(s));
}

void LayersPanel::setDocument(Document *doc)
{
    QObject::disconnect(m_conn);
    m_doc = doc;
    m_dragging = false;
    if (doc) m_conn = connect(doc, &Document::changed, this, [this](const QRect &r) { if (r.isNull()) rebuild(); });
    rebuild();
}

void LayersPanel::rowChanged(int row)
{
    if (!m_syncing && row >= 0 && m_doc) m_select(m_list->count() - 1 - row); // its currentChanged rebuilds this panel
}

void LayersPanel::rebuild()
{
    if (!m_doc) { m_list->clear(); return; }
    const QVector<Layer> &layers = m_doc->sprite().layers;
    const int n = layers.size();
    const int cur = m_canvas->context().layer;
    m_syncing = true;
    // Items are updated in place: a rename edit that triggered this must not have its item destroyed.
    while (m_list->count() > n) delete m_list->takeItem(0);
    while (m_list->count() < n) {
        auto *it = new QListWidgetItem;
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        m_list->addItem(it);
    }
    for (int r = 0; r < n; ++r) {
        const Layer &l = layers[n - 1 - r];
        m_list->item(r)->setText(l.name);
        m_list->item(r)->setCheckState(l.visible ? Qt::Checked : Qt::Unchecked);
    }
    m_list->setCurrentRow(n - 1 - cur);
    if (!m_dragging) m_opacity->setValue(layers[cur].opacity);
    m_syncing = false;
}

#include "Timeline.h"
#include "CanvasView.h"
#include "SpriteOps.h"
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QSpinBox>
#include <QStyle>
#include <QToolBar>
#include <QToolTip>
#include <algorithm>

using namespace spx;

namespace {
constexpr int kNameW = 96, kCellW = 20, kTagH = 16, kHeadH = 18, kRowH = 20;
const QColor kTagColors[] = {QColor(0xe0, 0x6c, 0x75), QColor(0xe5, 0xc0, 0x7b), QColor(0x98, 0xc3, 0x79),
                             QColor(0x56, 0xb6, 0xc2), QColor(0x61, 0xaf, 0xef), QColor(0xc6, 0x78, 0xdd)};
const QColor kAccent(0x3d8bfd), kText(0xdfe1e5), kDim(0x9da1a8), kLine(0x35373b);
} // namespace

Timeline::Timeline(CanvasView *canvas, std::function<void()> settle, std::function<void(int, int)> select, QWidget *parent)
    : QWidget(parent), m_canvas(canvas), m_settle(std::move(settle)), m_select(std::move(select)), m_bar(new QToolBar)
{
    setFocusPolicy(Qt::ClickFocus);
    m_bar->setIconSize({16, 16});
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &Timeline::tick);
    connect(canvas, &CanvasView::contextChanged, this, &Timeline::pause); // emitted on every canvas press: drawing pauses

    // Widget-scoped so Enter still commits line edits and spin boxes elsewhere in the window.
    auto add = [&](QStyle::StandardPixmap icon, const QString &text, const QList<QKeySequence> &keys) {
        QAction *a = m_bar->addAction(style()->standardIcon(icon), text);
        a->setShortcuts(keys);
        a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        if (!keys.isEmpty()) {
            a->setToolTip(tr("%1 (%2)").arg(text, keys.first().toString(QKeySequence::NativeText)));
            canvas->addAction(a);
            addAction(a);
        }
        return a;
    };
    QAction *prev = add(QStyle::SP_MediaSeekBackward, tr("Previous Frame"), {QKeySequence(Qt::ALT | Qt::Key_Comma)});
    m_play = add(QStyle::SP_MediaPlay, tr("Play/Pause"), {QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)});
    QAction *next = add(QStyle::SP_MediaSeekForward, tr("Next Frame"), {QKeySequence(Qt::ALT | Qt::Key_Period)});
    m_loop = add(QStyle::SP_BrowserReload, tr("Loop"), {});
    m_tagOnly = m_bar->addAction(tr("Tag Only"));
    m_tagOnly->setToolTip(tr("Play only the tag containing the current frame"));
    m_play->setCheckable(true);
    m_loop->setCheckable(true);
    m_loop->setChecked(true);
    m_tagOnly->setCheckable(true);
    connect(prev, &QAction::triggered, this, [this] { step(-1); });
    connect(next, &QAction::triggered, this, [this] { step(1); });
    connect(m_play, &QAction::toggled, this, &Timeline::togglePlay);
}

void Timeline::setDocument(Document *doc)
{
    pause();
    QObject::disconnect(m_conn);
    m_doc = doc;
    m_anchor = m_selEnd = 0;
    auto relayout = [this] {
        const Sprite &s = m_doc->sprite();
        setFixedSize(kNameW + s.frames.size() * kCellW, kTagH + kHeadH + s.layers.size() * kRowH);
        update();
    };
    if (doc) m_conn = connect(doc, &Document::changed, this, [this, relayout](const QRect &r) { if (r.isNull()) relayout(); else update(); });
    if (doc) relayout();
}

int Timeline::frame() const { return m_canvas->context().frame; }

std::pair<int, int> Timeline::selection() const
{
    const int last = m_doc->sprite().frames.size() - 1;
    return {std::min(std::min(m_anchor, m_selEnd), last), std::min(std::max(m_anchor, m_selEnd), last)};
}

Timeline::Hit Timeline::hitTest(QPoint p) const
{
    const Sprite &s = m_doc->sprite();
    const int f = p.x() < kNameW ? -1 : (p.x() - kNameW) / kCellW;
    Hit h{p.y() < kTagH ? TagBar : p.y() < kTagH + kHeadH ? Header : Cells, f < s.frames.size() ? f : -1, -1, -1};
    if (h.zone == Cells) {
        const int row = (p.y() - kTagH - kHeadH) / kRowH;
        if (row < s.layers.size()) h.layer = s.layers.size() - 1 - row;
    }
    // shortcut: overlapping tags share one bar and the last one wins the hit, add lanes if overlap becomes common.
    for (int i = 0; h.zone == TagBar && h.frame >= 0 && i < s.tags.size(); ++i)
        if (s.tags[i].from <= h.frame && h.frame <= s.tags[i].to) h.tag = i;
    return h;
}

void Timeline::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(e->rect(), QColor(0x232427));
    if (!m_doc) return;
    const Sprite &s = m_doc->sprite();
    const ToolContext &c = m_canvas->context();
    const int nF = s.frames.size(), nL = s.layers.size();
    const auto [lo, hi] = selection();
    const int f0 = std::max(0, (e->rect().left() - kNameW) / kCellW), f1 = std::min(nF - 1, (e->rect().right() - kNameW) / kCellW);
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.8);
    p.setFont(small);

    for (int i = 0; i < s.tags.size(); ++i) {
        const Tag &t = s.tags[i];
        const QRect r(kNameW + t.from * kCellW, 1, (t.to - t.from + 1) * kCellW, kTagH - 2);
        p.fillRect(r, kTagColors[i % std::size(kTagColors)]);
        p.setPen(QColor(0x1e1f22));
        p.drawText(r.adjusted(4, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft, p.fontMetrics().elidedText(t.name, Qt::ElideRight, r.width() - 6));
    }
    for (int f = f0; f <= f1; ++f) {
        const QRect r(kNameW + f * kCellW, kTagH, kCellW, kHeadH);
        if (f >= lo && f <= hi) p.fillRect(r, QColor(0x23395b));
        p.setPen(f == c.frame ? kText : kDim);
        p.drawText(r, Qt::AlignCenter, QString::number(f + 1));
    }
    for (int row = 0; row < nL; ++row) {
        const int layer = nL - 1 - row, y = kTagH + kHeadH + row * kRowH;
        const Layer &l = s.layers[layer];
        const QRect name(0, y, kNameW, kRowH);
        if (layer == c.layer) p.fillRect(name, QColor(0x2b2d30));
        p.setPen(l.visible ? kText : kDim);
        p.drawText(name.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, p.fontMetrics().elidedText(l.name, Qt::ElideRight, kNameW - 10));
        for (int f = f0; f <= f1; ++f) {
            const QRect r(kNameW + f * kCellW, y, kCellW, kRowH);
            if (layer == c.layer && f == c.frame) p.fillRect(r, kAccent);
            else if (layer == c.layer || f == c.frame) p.fillRect(r, QColor(0x2b2d30));
            p.setPen(kLine);
            p.drawRect(r.adjusted(0, 0, -1, -1));
            if (!l.cels[f].isNull()) {
                p.setPen(Qt::NoPen);
                p.setBrush(layer == c.layer && f == c.frame ? QColor(Qt::white) : kText);
                p.drawEllipse(r.center() + QPoint(1, 1), 3, 3);
                p.setBrush(Qt::NoBrush);
            }
        }
    }
}

void Timeline::mousePressEvent(QMouseEvent *e)
{
    if (!m_doc || e->button() != Qt::LeftButton) return;
    const Hit h = hitTest(e->position().toPoint());
    if (h.zone == TagBar) return;
    const int f = h.frame >= 0 ? h.frame : frame();
    m_dragHeader = h.zone == Header && h.frame >= 0;
    if (m_dragHeader && (e->modifiers() & Qt::ShiftModifier)) m_selEnd = f;
    else m_anchor = m_selEnd = f;
    m_select(h.layer >= 0 ? h.layer : m_canvas->context().layer, f);
    update();
}

void Timeline::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_doc || !m_dragHeader || !(e->buttons() & Qt::LeftButton)) return;
    m_selEnd = std::clamp(int(e->position().x() - kNameW) / kCellW, 0, int(m_doc->sprite().frames.size()) - 1);
    update();
}

void Timeline::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!m_doc || e->button() != Qt::LeftButton) return;
    const Hit h = hitTest(e->position().toPoint());
    if (h.zone == Header && h.frame >= 0) editDurations(h.frame);
    else if (h.tag >= 0) editTag(h.tag);
}

void Timeline::contextMenuEvent(QContextMenuEvent *e)
{
    if (!m_doc) return;
    const Hit h = hitTest(e->pos());
    QMenu menu(this);
    if (h.tag >= 0) {
        menu.addAction(tr("Edit Tag…"), this, [this, i = h.tag] { editTag(i); });
        menu.addAction(tr("Delete Tag"), this, [this, i = h.tag] { m_doc->apply(tr("Delete Tag"), removeTag(m_doc->sprite(), i)); });
        menu.exec(e->globalPos());
        return;
    }
    if (h.frame >= 0) {
        const auto [lo, hi] = selection();
        if (h.frame < lo || h.frame > hi) m_anchor = m_selEnd = h.frame;
        m_select(h.layer >= 0 ? h.layer : m_canvas->context().layer, h.frame);
    }
    const int f = frame(), n = m_doc->sprite().frames.size();
    menu.addAction(tr("Add Frame"), this, [this, f] { frameOp(tr("Add Frame"), f + 1, [f](Sprite s) { return addFrame(std::move(s), f + 1); }); });
    menu.addAction(tr("Duplicate Frame"), this, [this, f] { frameOp(tr("Duplicate Frame"), f + 1, [f](Sprite s) { return duplicateFrame(std::move(s), f); }); });
    menu.addAction(tr("Delete Frame"), this, [this, f] { frameOp(tr("Delete Frame"), f, [f](Sprite s) { return removeFrame(std::move(s), f); }); })->setEnabled(n > 1);
    menu.addAction(tr("Move Left"), this, [this, f] { frameOp(tr("Move Frame"), f - 1, [f](Sprite s) { return moveFrame(std::move(s), f, f - 1); }); })->setEnabled(f > 0);
    menu.addAction(tr("Move Right"), this, [this, f] { frameOp(tr("Move Frame"), f + 1, [f](Sprite s) { return moveFrame(std::move(s), f, f + 1); }); })->setEnabled(f < n - 1);
    menu.addSeparator();
    menu.addAction(tr("New Tag…"), this, &Timeline::newTag);
    menu.exec(e->globalPos());
}

bool Timeline::event(QEvent *e)
{
    if (e->type() != QEvent::ToolTip || !m_doc) return QWidget::event(e);
    auto *he = static_cast<QHelpEvent *>(e);
    const Hit h = hitTest(he->pos());
    const Sprite &s = m_doc->sprite();
    if (h.tag >= 0) QToolTip::showText(he->globalPos(), tr("%1: frames %2–%3").arg(s.tags[h.tag].name).arg(s.tags[h.tag].from + 1).arg(s.tags[h.tag].to + 1), this);
    else if (h.zone == Header && h.frame >= 0) QToolTip::showText(he->globalPos(), tr("Frame %1: %2 ms").arg(h.frame + 1).arg(s.frames[h.frame].durationMs), this);
    else QToolTip::hideText();
    return true;
}

void Timeline::frameOp(const QString &text, int newCurrent, const std::function<Sprite(Sprite)> &op)
{
    pause();
    m_settle(); // floating pixels must land in their own frame before frames shift
    m_canvas->context().frame = newCurrent; // before apply: the canvas clamps it on the change apply emits
    m_anchor = m_selEnd = newCurrent;
    m_doc->apply(text, op(m_doc->sprite()));
}

void Timeline::editDurations(int f)
{
    auto [lo, hi] = selection();
    if (f < lo || f > hi) lo = hi = f;
    bool ok = false;
    const int ms = QInputDialog::getInt(this, tr("Frame Duration"), tr("Duration (ms):"), m_doc->sprite().frames[f].durationMs, 1, 65535, 1, &ok);
    if (!ok) return;
    Sprite s = m_doc->sprite();
    for (int i = lo; i <= hi; ++i) s = setFrameDuration(std::move(s), i, ms);
    m_doc->apply(tr("Frame Duration"), std::move(s));
}

void Timeline::newTag()
{
    const auto [lo, hi] = selection();
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Tag"), tr("Name:"), QLineEdit::Normal, tr("Tag"), &ok).trimmed();
    if (ok && !name.isEmpty()) m_doc->apply(tr("New Tag"), addTag(m_doc->sprite(), Tag{name, lo, hi}));
}

void Timeline::editTag(int index)
{
    const Tag t = m_doc->sprite().tags[index];
    const int n = m_doc->sprite().frames.size();
    QDialog d(this);
    d.setWindowTitle(tr("Edit Tag"));
    auto *form = new QFormLayout(&d);
    auto *name = new QLineEdit(t.name);
    auto spin = [n](int v) { auto *s = new QSpinBox; s->setRange(1, n); s->setValue(v + 1); return s; };
    QSpinBox *from = spin(t.from), *to = spin(t.to);
    form->addRow(tr("Name:"), name);
    form->addRow(tr("From frame:"), from);
    form->addRow(tr("To frame:"), to);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    form->addRow(buttons);
    if (d.exec() != QDialog::Accepted || name->text().trimmed().isEmpty()) return;
    const auto [a, b] = std::minmax({from->value() - 1, to->value() - 1}); // initializer_list form returns values, not refs
    m_doc->apply(tr("Edit Tag"), setTag(m_doc->sprite(), index, Tag{name->text().trimmed(), a, b}));
}

std::pair<int, int> Timeline::playRange() const
{
    const Sprite &s = m_doc->sprite();
    for (const Tag &t : s.tags)
        if (m_tagOnly->isChecked() && t.from <= frame() && frame() <= t.to) return {t.from, t.to};
    return {0, int(s.frames.size()) - 1};
}

void Timeline::pause() { m_play->setChecked(false); }

void Timeline::togglePlay(bool on)
{
    m_play->setIcon(style()->standardIcon(on ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
    if (!on || !m_doc) { m_timer.stop(); if (!m_doc) pause(); return; }
    m_settle(); // once per play: ticks change the frame without settling
    if (!m_play->isChecked()) return; // settling ended an open stroke, which pauses
    const auto [a, b] = playRange();
    if (!m_loop->isChecked() && frame() == b) { m_canvas->context().frame = a; m_canvas->update(); update(); }
    m_timer.start(m_doc->sprite().frames[frame()].durationMs);
}

void Timeline::tick()
{
    const auto [a, b] = playRange();
    int f = frame() + 1;
    if (f > b) {
        if (!m_loop->isChecked()) return pause();
        f = a;
    }
    m_canvas->context().frame = f; // bypasses MainWindow::setCurrent: settled once when play started
    m_canvas->update();
    update();
    m_timer.start(m_doc->sprite().frames[f].durationMs);
}

void Timeline::step(int delta)
{
    if (!m_doc) return;
    pause();
    const int n = m_doc->sprite().frames.size();
    m_select(m_canvas->context().layer, (frame() + delta + n) % n);
    m_anchor = m_selEnd = frame();
    update();
}

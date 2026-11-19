#include "ResizeCanvasDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

ResizeCanvasDialog::ResizeCanvasDialog(QSize current, QWidget *parent)
    : QDialog(parent), m_w(new QSpinBox), m_h(new QSpinBox), m_anchor(new QComboBox)
{
    setWindowTitle(tr("Resize Canvas"));
    m_w->setRange(1, 2048);
    m_h->setRange(1, 2048);
    m_w->setValue(current.width());
    m_h->setValue(current.height());
    m_w->setSuffix(tr(" px"));
    m_h->setSuffix(tr(" px"));
    const std::pair<QString, Qt::Alignment> anchors[] = {
        {tr("Top Left"), Qt::AlignTop | Qt::AlignLeft},       {tr("Top"), Qt::AlignTop | Qt::AlignHCenter},
        {tr("Top Right"), Qt::AlignTop | Qt::AlignRight},     {tr("Left"), Qt::AlignVCenter | Qt::AlignLeft},
        {tr("Center"), Qt::AlignCenter},                      {tr("Right"), Qt::AlignVCenter | Qt::AlignRight},
        {tr("Bottom Left"), Qt::AlignBottom | Qt::AlignLeft}, {tr("Bottom"), Qt::AlignBottom | Qt::AlignHCenter},
        {tr("Bottom Right"), Qt::AlignBottom | Qt::AlignRight}};
    for (const auto &[name, a] : anchors) m_anchor->addItem(name, int(a));
    m_anchor->setCurrentIndex(4);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Width:"), m_w);
    form->addRow(tr("Height:"), m_h);
    form->addRow(tr("Anchor:"), m_anchor);
    form->addRow(buttons);
}

QSize ResizeCanvasDialog::canvasSize() const { return {m_w->value(), m_h->value()}; }
Qt::Alignment ResizeCanvasDialog::anchor() const { return Qt::Alignment(m_anchor->currentData().toInt()); }

#include "NewSpriteDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

NewSpriteDialog::NewSpriteDialog(QWidget *parent) : QDialog(parent), m_w(new QSpinBox), m_h(new QSpinBox)
{
    setWindowTitle(tr("New Sprite"));
    for (QSpinBox *s : {m_w, m_h}) { s->setRange(1, 2048); s->setValue(32); s->setSuffix(tr(" px")); }
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *form = new QFormLayout(this);
    form->addRow(tr("Width:"), m_w);
    form->addRow(tr("Height:"), m_h);
    form->addRow(buttons);
}

QSize NewSpriteDialog::spriteSize() const { return {m_w->value(), m_h->value()}; }

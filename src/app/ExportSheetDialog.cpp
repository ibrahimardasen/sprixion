#include "ExportSheetDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

ExportSheetDialog::ExportSheetDialog(const QString &suggestedPath, QWidget *parent)
    : QDialog(parent), m_layout(new QComboBox), m_columns(new QSpinBox), m_path(new QLineEdit(suggestedPath))
{
    setWindowTitle(tr("Export Sprite Sheet"));
    m_layout->addItem(tr("Horizontal strip"), int(spx::SheetLayout::Horizontal));
    m_layout->addItem(tr("Grid"), int(spx::SheetLayout::Grid));
    m_columns->setRange(1, 1000);
    m_columns->setValue(4);
    m_columns->setEnabled(false);
    connect(m_layout, &QComboBox::currentIndexChanged, this, [this] { m_columns->setEnabled(sheetLayout() == spx::SheetLayout::Grid); });

    auto *browse = new QPushButton(tr("Browse…"));
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString p = QFileDialog::getSaveFileName(this, tr("Export Sprite Sheet"), m_path->text(), tr("PNG image (*.png)"));
        if (!p.isEmpty()) m_path->setText(p);
    });
    auto *file = new QHBoxLayout;
    file->addWidget(m_path, 1);
    file->addWidget(browse);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Export"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_path, &QLineEdit::textChanged, this,
            [this, buttons] { buttons->button(QDialogButtonBox::Ok)->setEnabled(!m_path->text().trimmed().isEmpty()); });

    auto *form = new QFormLayout(this);
    form->addRow(tr("Layout:"), m_layout);
    form->addRow(tr("Columns:"), m_columns);
    form->addRow(tr("File:"), file);
    form->addRow(buttons);
    resize(480, sizeHint().height());
}

spx::SheetLayout ExportSheetDialog::sheetLayout() const { return spx::SheetLayout(m_layout->currentData().toInt()); }
int ExportSheetDialog::columns() const { return m_columns->value(); }

QString ExportSheetDialog::pngPath() const
{
    const QString p = m_path->text().trimmed();
    return p.endsWith(QLatin1String(".png"), Qt::CaseInsensitive) ? p : p + QLatin1String(".png");
}

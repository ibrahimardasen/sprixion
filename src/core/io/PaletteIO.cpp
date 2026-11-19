#include "PaletteIO.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace spx {

namespace {
constexpr qint64 kMaxBytes = 1 << 20; // trust boundary

QString tr(const char* s) { return QCoreApplication::translate("PaletteIO", s); }

std::optional<QVector<QColor>> fail(QString* e, const QString& msg) { if (e) *e = msg; return std::nullopt; }
std::optional<QVector<QColor>> badLine(QString* e, int n) { return fail(e, tr("Invalid palette entry on line %1").arg(n)); }
}

std::optional<QVector<QColor>> loadPalette(const QString& path, QString* error)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext != "gpl" && ext != "hex") return fail(error, tr("Unsupported palette format (use .gpl or .hex)"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return fail(error, tr("Cannot open palette file"));
    if (f.size() > kMaxBytes) return fail(error, tr("Palette file is too large"));
    const QStringList lines = QString::fromUtf8(f.readAll()).split('\n');

    QVector<QColor> out;
    bool header = ext == "hex";
    static const QRegularExpression hexRe("^#?([0-9a-fA-F]{6})$");
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;
        if (!header) {
            if (line != "GIMP Palette") return fail(error, tr("Not a GIMP palette (line %1)").arg(i + 1));
            header = true;
            continue;
        }
        if (ext == "hex") {
            const auto m = hexRe.match(line);
            if (!m.hasMatch()) return badLine(error, i + 1);
            out << QColor("#" + m.captured(1));
        } else {
            if (line.startsWith('#') || line.startsWith("Name:") || line.startsWith("Columns:")) continue;
            const QStringList parts = line.split(QRegularExpression("\\s+"));
            int v[3] = {};
            bool ok = parts.size() >= 3;
            for (int k = 0; ok && k < 3; ++k) { v[k] = parts[k].toInt(&ok); ok = ok && v[k] >= 0 && v[k] <= 255; }
            if (!ok) return badLine(error, i + 1);
            out << QColor(v[0], v[1], v[2]);
        }
    }
    if (!header) return fail(error, tr("Empty palette file"));
    return out;
}

} // namespace spx

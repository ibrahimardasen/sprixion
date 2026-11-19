#include "SprxIO.h"
#include "Compositor.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <miniz.h>

namespace spx {
namespace {

QString tr(const char *s) { return QCoreApplication::translate("SprxIO", s); }

QByteArray toPng(const QImage &i)
{
    QByteArray b; QBuffer buf(&b); buf.open(QIODevice::WriteOnly); i.save(&buf, "PNG"); return b;
}

// Colors as #rrggbbaa (not QColor::HexArgb, which puts alpha first).
QString colorToStr(const QColor &c) { return QString::asprintf("#%02x%02x%02x%02x", c.red(), c.green(), c.blue(), c.alpha()); }

std::optional<QColor> colorFromStr(const QString &s)
{
    bool ok = s.size() == 9 && s[0] == '#';
    const uint v = ok ? s.mid(1).toUInt(&ok, 16) : 0;
    if (!ok) return std::nullopt;
    return QColor(v >> 24 & 255, v >> 16 & 255, v >> 8 & 255, v & 255);
}

bool readEntry(mz_zip_archive &z, const QString &name, QByteArray &out)
{
    size_t n = 0;
    void *p = mz_zip_reader_extract_file_to_heap(&z, name.toUtf8().constData(), &n, 0);
    if (!p) return false;
    out = QByteArray(static_cast<const char *>(p), qsizetype(n));
    mz_free(p);
    return true;
}

} // namespace

bool saveSprx(const Sprite &s, const QString &path, QString *error)
{
    auto fail = [&](const QString &m) { if (error) *error = m; return false; };
    QJsonArray layers, frames, tags, palette, cels;
    for (const Layer &l : s.layers) layers.append(QJsonObject{{"name", l.name}, {"visible", l.visible}, {"opacity", l.opacity}});
    for (int f = 0; f < s.frames.size(); ++f)
        frames.append(QJsonObject{{"duration", s.frames[f].durationMs}, {"flat", QString("flat/%1.png").arg(f)}});
    for (const Tag &t : s.tags) tags.append(QJsonObject{{"name", t.name}, {"from", t.from}, {"to", t.to}});
    for (const QColor &c : s.palette) palette.append(colorToStr(c));
    QJsonValue grid;
    if (s.sliceGrid) {
        const SliceGrid &g = *s.sliceGrid;
        grid = QJsonObject{{"cellWidth", g.cellWidth}, {"cellHeight", g.cellHeight}, {"offsetX", g.offsetX}, {"offsetY", g.offsetY},
                           {"spacingX", g.spacingX}, {"spacingY", g.spacingY}};
    }

    mz_zip_archive z{};
    if (!mz_zip_writer_init_heap(&z, 0, 0)) return fail(tr("Could not create the file."));
    bool ok = true;
    auto add = [&](const QString &name, const QByteArray &data) {
        ok = ok && mz_zip_writer_add_mem(&z, name.toUtf8().constData(), data.constData(), size_t(data.size()), MZ_DEFAULT_COMPRESSION);
    };
    for (int l = 0; l < s.layers.size(); ++l)
        for (int f = 0; f < s.frames.size(); ++f) {
            const QImage &c = s.layers[l].cels[f];
            if (c.isNull()) continue;
            const QString file = QString("cels/%1_%2.png").arg(l).arg(f);
            cels.append(QJsonObject{{"layer", l}, {"frame", f}, {"file", file}});
            add(file, toPng(c));
        }
    for (int f = 0; f < s.frames.size(); ++f) add(QString("flat/%1.png").arg(f), toPng(compositeFrame(s, f)));
    add("manifest.json", QJsonDocument(QJsonObject{{"formatVersion", 1}, {"width", s.size.width()}, {"height", s.size.height()},
        {"layers", layers}, {"frames", frames}, {"tags", tags}, {"palette", palette}, {"sliceGrid", grid}, {"cels", cels}}).toJson());

    void *buf = nullptr; size_t n = 0;
    ok = ok && mz_zip_writer_finalize_heap_archive(&z, &buf, &n);
    if (!ok) { mz_zip_writer_end(&z); return fail(tr("Could not create the file.")); }
    QSaveFile f(path);
    bool saved = f.open(QIODevice::WriteOnly) && f.write(static_cast<const char *>(buf), qint64(n)) == qint64(n) && f.commit();
    QString why = f.errorString();
    mz_zip_writer_end(&z);
    mz_free(buf);
    return saved ? true : fail(why);
}

std::optional<Sprite> loadSprx(const QString &path, QString *error)
{
    auto fail = [&](const QString &m) -> std::optional<Sprite> { if (error) *error = m; return std::nullopt; };
    const auto bad = [&] { return fail(tr("This is not a valid Sprixion file.")); };

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return fail(file.errorString());
    const QByteArray data = file.readAll();
    mz_zip_archive z{};
    if (!mz_zip_reader_init_mem(&z, data.constData(), size_t(data.size()), 0)) return bad();
    struct End { mz_zip_archive &z; ~End() { mz_zip_reader_end(&z); } } end{z};

    QByteArray mf;
    QJsonParseError pe;
    if (!readEntry(z, "manifest.json", mf)) return bad();
    const QJsonObject m = QJsonDocument::fromJson(mf, &pe).object();
    if (pe.error != QJsonParseError::NoError || m.isEmpty()) return bad();
    if (m["formatVersion"].toInt() != 1) {
        return fail(m["formatVersion"].toInt() > 1 ? tr("This file was made with a newer version of Sprixion.") : tr("This is not a valid Sprixion file."));
    }

    Sprite s;
    s.size = QSize(m["width"].toInt(), m["height"].toInt());
    if (s.size.width() < 1 || s.size.width() > 2048 || s.size.height() < 1 || s.size.height() > 2048) return bad();
    const QJsonArray layers = m["layers"].toArray(), frames = m["frames"].toArray();
    if (layers.isEmpty() || frames.isEmpty()) return bad();
    for (const QJsonValue &v : frames) {
        const int d = v["duration"].toInt();
        if (d < 1) return bad();
        s.frames.append(Frame{d});
    }
    for (const QJsonValue &v : layers)
        s.layers.append(Layer{v["name"].toString(), v["visible"].toBool(true), v["opacity"].toInt(255), QVector<QImage>(s.frames.size())});
    for (const QJsonValue &v : m["tags"].toArray()) {
        const Tag t{v["name"].toString(), v["from"].toInt(), v["to"].toInt()};
        if (t.from < 0 || t.from > t.to || t.to >= s.frames.size()) return bad();
        s.tags.append(t);
    }
    for (const QJsonValue &v : m["palette"].toArray()) {
        const auto c = colorFromStr(v.toString());
        if (!c) return bad();
        s.palette.append(*c);
    }
    if (const QJsonObject g = m["sliceGrid"].toObject(); !g.isEmpty())
        s.sliceGrid = SliceGrid{g["cellWidth"].toInt(), g["cellHeight"].toInt(), g["offsetX"].toInt(), g["offsetY"].toInt(),
                                g["spacingX"].toInt(), g["spacingY"].toInt()};
    for (const QJsonValue &v : m["cels"].toArray()) {
        const int l = v["layer"].toInt(-1), f = v["frame"].toInt(-1);
        QByteArray png;
        if (l < 0 || l >= s.layers.size() || f < 0 || f >= s.frames.size() || !readEntry(z, v["file"].toString(), png)) return bad();
        QImage img = QImage::fromData(png).convertToFormat(QImage::Format_ARGB32);
        if (img.size() != s.size) return bad();
        s.layers[l].cels[f] = img;
    }
    return s;
}

} // namespace spx

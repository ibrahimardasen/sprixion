using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;
using UnityEngine;

namespace Sprixion.Importer
{
    // Mirrors manifest.json as written by src/core/io/SprxIO.cpp. Layers, cels and palette are ignored.
    [Serializable] class Manifest { public int formatVersion; public int width; public int height; public FrameDto[] frames; public TagDto[] tags; public SliceGrid sliceGrid; }
    [Serializable] class FrameDto { public int duration; public string flat; }
    [Serializable] class TagDto { public string name; public int from; public int to; }
    [Serializable] public class SliceGrid { public int cellWidth, cellHeight, offsetX, offsetY, spacingX, spacingY; }

    public class SprxFile
    {
        public int Width, Height;
        public List<int> DurationsMs = new();
        public List<Texture2D> Flats = new();
        public List<(string Name, int From, int To)> Tags = new();
        public SliceGrid Grid; // null when the file has no slice grid

        public static SprxFile Read(byte[] data)
        {
            var f = new SprxFile();
            try { f.Load(data); return f; }
            catch { f.DestroyFlats(); throw; }
        }

        public void DestroyFlats()
        {
            foreach (var t in Flats) UnityEngine.Object.DestroyImmediate(t);
            Flats.Clear();
        }

        static InvalidDataException Bad(string why) => new InvalidDataException("Not a valid Sprixion file: " + why);

        void Load(byte[] data)
        {
            using var zip = new ZipArchive(new MemoryStream(data), ZipArchiveMode.Read);
            var m = JsonUtility.FromJson<Manifest>(Encoding.UTF8.GetString(Entry(zip, "manifest.json", 16 << 20)));
            if (m == null) throw Bad("manifest.json is empty.");
            if (m.formatVersion > 1) throw new InvalidDataException("This file was made with a newer version of Sprixion. Update the Sprixion Importer package.");
            if (m.formatVersion != 1) throw Bad($"unsupported formatVersion {m.formatVersion}.");
            if (m.width < 1 || m.width > 2048 || m.height < 1 || m.height > 2048) throw Bad($"canvas size {m.width}x{m.height} is outside 1-2048.");
            if (m.frames == null || m.frames.Length == 0 || m.frames.Length > 4096) throw Bad("it needs 1-4096 frames.");
            Width = m.width;
            Height = m.height;

            foreach (var t in m.tags ?? new TagDto[0])
            {
                if (t.from < 0 || t.from > t.to || t.to >= m.frames.Length) throw Bad($"tag '{t.name}' range {t.from}-{t.to} is outside the {m.frames.Length} frames.");
                Tags.Add((t.name, t.from, t.to));
            }

            // JsonUtility turns a null or missing object into one with all fields zero.
            var g = m.sliceGrid;
            if (g != null && (g.cellWidth | g.cellHeight | g.offsetX | g.offsetY | g.spacingX | g.spacingY) != 0)
            {
                if (g.cellWidth < 1 || g.cellHeight < 1 || g.offsetX < 0 || g.offsetY < 0 || g.spacingX < 0 || g.spacingY < 0)
                    throw Bad("the slice grid has a cell size below 1 or a negative offset or spacing.");
                Grid = g;
            }

            for (int i = 0; i < m.frames.Length; i++)
            {
                var fr = m.frames[i];
                if (fr.duration <= 0) throw Bad($"frame {i} has duration {fr.duration} ms.");
                DurationsMs.Add(fr.duration);
                var tex = new Texture2D(2, 2);
                Flats.Add(tex); // added first so a failure below still frees it
                if (!tex.LoadImage(Entry(zip, fr.flat, 2048L * 2048 * 4 + (1 << 20)))) throw Bad($"{fr.flat} is not a PNG.");
                if (tex.width != Width || tex.height != Height) throw Bad($"{fr.flat} is {tex.width}x{tex.height}, not {Width}x{Height}.");
            }
        }

        // Reads at most max bytes, whatever the zip header claims (zip-bomb guard).
        static byte[] Entry(ZipArchive zip, string name, long max)
        {
            var e = string.IsNullOrEmpty(name) ? null : zip.GetEntry(name);
            if (e == null) throw Bad($"{name} is missing.");
            using var s = e.Open();
            var ms = new MemoryStream();
            var buf = new byte[81920];
            int n;
            while ((n = s.Read(buf, 0, buf.Length)) > 0)
            {
                if (ms.Length + n > max) throw Bad($"{name} is too large.");
                ms.Write(buf, 0, n);
            }
            return ms.ToArray();
        }
    }
}

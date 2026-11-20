using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.AssetImporters;
using UnityEngine;

namespace Sprixion.Importer
{
    [ScriptedImporter(1, "sprx")]
    public class SprxImporter : ScriptedImporter
    {
        [Min(0.01f)] public float pixelsPerUnit = 16;
        public Vector2 pivot = new(0.5f, 0f);
        public bool loopClips = true;

        const int MaxAtlas = 16384;

        public override void OnImportAsset(AssetImportContext ctx)
        {
            SprxFile file = null;
            try
            {
                file = SprxFile.Read(File.ReadAllBytes(ctx.assetPath));
                Import(ctx, file);
            }
            catch (Exception e)
            {
                ctx.LogImportError($"{ctx.assetPath}: {e.Message}");
            }
            finally
            {
                file?.DestroyFlats();
            }
        }

        void Import(AssetImportContext ctx, SprxFile f)
        {
            string name = Path.GetFileNameWithoutExtension(ctx.assetPath);
            int w = f.Width, h = f.Height, n = f.Flats.Count;
            int cols = (int)Math.Ceiling(Math.Sqrt(n)), rows = (n + cols - 1) / cols;
            if (cols * w > MaxAtlas || rows * h > MaxAtlas)
                throw new InvalidDataException($"The packed texture would be {cols * w}x{rows * h} pixels, over Unity's {MaxAtlas} limit. Use fewer or smaller frames.");

            var atlas = new Texture2D(cols * w, rows * h, TextureFormat.RGBA32, false)
            {
                name = name, filterMode = FilterMode.Point, wrapMode = TextureWrapMode.Clamp
            };
            var rects = new List<(Rect Rect, string Id, string Name)>();
            for (int i = 0; i < cols * rows; i++)
            {
                // Row-major from the top; Unity's texture origin is bottom-left.
                int x = i % cols * w, y = atlas.height - (i / cols + 1) * h;
                if (i >= n) { atlas.SetPixels32(x, y, w, h, new Color32[w * h]); continue; }
                var px = f.Flats[i].GetPixels32(); // bottom row first
                atlas.SetPixels32(x, y, w, h, px);
                if (f.Grid == null) { rects.Add((new Rect(x, y, w, h), $"frame_{i}", $"{name}_{i}")); continue; }

                var g = f.Grid;
                int index = 0;
                for (int cy = g.offsetY; cy + g.cellHeight <= h; cy += g.cellHeight + g.spacingY)
                    for (int cx = g.offsetX; cx + g.cellWidth <= w; cx += g.cellWidth + g.spacingX, index++)
                        if (!IsEmpty(px, w, h, cx, cy, g.cellWidth, g.cellHeight))
                            rects.Add((new Rect(x + cx, y + h - cy - g.cellHeight, g.cellWidth, g.cellHeight), $"frame_{i}_{index}", $"{name}_{i}_{index}"));
            }
            atlas.Apply(false);
            ctx.AddObjectToAsset("texture", atlas);
            ctx.SetMainObject(atlas);

            var sprites = new List<Sprite>();
            foreach (var (rect, id, spriteName) in rects)
            {
                var s = Sprite.Create(atlas, rect, pivot, pixelsPerUnit, 0, SpriteMeshType.FullRect);
                s.name = spriteName;
                ctx.AddObjectToAsset(id, s); // ids don't depend on the file name, so renaming keeps references
                sprites.Add(s);
            }
            if (f.Grid != null) return;

            // shortcut: tags play forward only (the format stores no direction yet), add reverse/pingpong when it does.
            var binding = EditorCurveBinding.PPtrCurve("", typeof(SpriteRenderer), "m_Sprite");
            var ids = new HashSet<string>();
            for (int t = 0; t < f.Tags.Count; t++)
            {
                var (tagName, from, to) = f.Tags[t];
                var keys = new ObjectReferenceKeyframe[to - from + 2];
                int ms = 0;
                for (int k = from; k <= to; k++)
                {
                    keys[k - from] = new ObjectReferenceKeyframe { time = ms / 1000f, value = sprites[k] };
                    ms += f.DurationsMs[k];
                }
                // Repeat the last sprite at the end time so the last frame keeps its duration.
                keys[keys.Length - 1] = new ObjectReferenceKeyframe { time = ms / 1000f, value = sprites[to] };

                var clip = new AnimationClip { name = tagName, frameRate = 1000 };
                AnimationUtility.SetObjectReferenceCurve(clip, binding, keys);
                var settings = AnimationUtility.GetAnimationClipSettings(clip);
                settings.loopTime = loopClips;
                AnimationUtility.SetAnimationClipSettings(clip, settings);
                string id = "clip_" + tagName;
                if (!ids.Add(id)) id += "_" + t; // overlapping tags may share a name
                ctx.AddObjectToAsset(id, clip);
            }
        }

        static bool IsEmpty(Color32[] px, int w, int h, int cx, int cy, int cw, int ch)
        {
            for (int r = cy; r < cy + ch; r++)
            {
                int row = (h - 1 - r) * w; // px is bottom-up, r counts from the top
                for (int c = cx; c < cx + cw; c++)
                    if (px[row + c].a != 0) return false;
            }
            return true;
        }
    }
}

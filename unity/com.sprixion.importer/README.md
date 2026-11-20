# Sprixion Importer

Unity editor package that imports Sprixion `.sprx` files. Supports Unity 2022.3 LTS and Unity 6.

## Install

In Unity, open **Window > Package Manager > + > Add package from git URL** and enter
`<repo-url>.git?path=unity/com.sprixion.importer`.
Or use **Add package from disk** and pick `unity/com.sprixion.importer/package.json` from a local clone.

## What you get

Save or drop a `.sprx` file into `Assets/`. It imports as:

- one texture with all frames packed in a grid (Point filter, no compression, no mipmaps), named after the file
- one sprite per frame, named `<file>_<frame>`
- one animation clip per tag, named after the tag, using the frame durations and targeting `SpriteRenderer.sprite`

If the file has a slice grid, every frame is cut into one sprite per non-empty cell, named `<file>_<frame>_<cell>`
(cells counted row by row from the top left), and no clips are made.

Re-saving the file in Sprixion reimports it. Removed frames and tags disappear.

## Inspector options

- **Pixels Per Unit** (default 16)
- **Pivot**, normalized (default bottom center, `0.5, 0`)
- **Loop Clips** (default on)

Clips always play forward, from the tag's first frame to its last.

## License

GPLv3, see `LICENSE`. The package is editor-only (`includePlatforms: Editor`), so none of its code ships in your game builds.

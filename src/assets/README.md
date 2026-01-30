# Assets Folder

This folder contains LVGL image assets converted to C arrays.

## How to Add Images

1. **Resize your images** (see `../../IMAGE_CONVERSION_GUIDE.md`)
   - Icons: 192×192 px
   - Background: 800×480 px
   - Logo: ~200px wide

2. **Convert using LVGL Image Converter**: https://lvgl.io/tools/imageconverter
   - Output format: **C array**
   - Color format: **True color with alpha** (for icons with transparency)
   - Name each image: `img_home_icon`, `img_windows_icon`, etc.

3. **Place generated `.c` files here**:
   ```
   src/assets/img_home_icon.c
   src/assets/img_windows_icon.c
   src/assets/img_locks_icon.c
   src/assets/img_runningboards_icon.c
   src/assets/img_background.c
   src/assets/img_bronco_logo.c
   ```

4. **Build and upload**: `pio run --target upload`

## Current Status

⚠️ **No image files yet** - UI uses text placeholders

Once you add the `.c` files:
- Uncomment the `#include "assets/images.h"` lines in `main.cpp`
- Uncomment the image creation code (search for "// lv_obj_t* bg = lv_img_create")
- The images will automatically display

## Image Specifications

| Image | Size | Format | Purpose |
|-------|------|--------|---------|
| `img_home_icon` | 192×192 | ARGB | Home nav button |
| `img_windows_icon` | 192×192 | ARGB | Windows nav button |
| `img_locks_icon` | 192×192 | ARGB | Locks nav button |
| `img_runningboards_icon` | 192×192 | ARGB | Running boards nav button |
| `img_background` | 800×480 | RGB | Full-screen background |
| `img_bronco_logo` | 200×auto | RGB | Header logo |

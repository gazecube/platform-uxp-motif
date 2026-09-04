# UXP Motif backend

This branch resurrects Mozilla's historical Motif/Xt platform model on the UXP codebase without inserting GTK or ViewKit between XUL and X11.

## Reference points

- UXP base: `Jazzzny/platform-uxp` commit `4c3877e354a6adc55c0adf515b4617110e10ae31`
- Historical reference: Mozilla 0.9 `widget/src/motif` and `gfx/src/motif`
- Working branch: `motif-backend`

The old Mozilla source is treated as the behavioral/reference implementation.  Modern UXP interfaces are implemented directly where the old interfaces no longer exist.

## Architecture

```
XUL application chrome
        |
        v
       UXP
        |
        v
 widget/motif
        |
        v
   Motif / Xt
        |
        v
       X11
```

Rendering is deliberately not a resurrection of `gfx/src/motif`.  UXP already has modern Cairo, Fontconfig/FreeType, and Xlib code that is independent of GTK.  The Motif backend reuses those pieces while replacing the native widget/event integration.

## First build configuration

Use `.mozconfig-motif.example` as the platform portion of the application's mozconfig.  The important settings are:

```
ac_add_options --enable-default-toolkit=cairo-motif
export USE_FC_FREETYPE=1
ac_add_options --disable-skia
ac_add_options --disable-printing
ac_add_options --disable-accessibility
```

`USE_FC_FREETYPE=1` is required because this UXP generation's old configure automatically enables the Fontconfig/FreeType checks only for GTK toolkits.  The Motif backend uses those same GTK-independent font classes.

The first pass intentionally disables Skia because `gfx/skia/moz.build` still selects its Unix Cairo/FreeType font host by testing for `gtk2`/`gtk3`.  Cairo/Xlib is the clean GTK-free bring-up path and also follows the historical backend more closely.

### System dependencies

At minimum the build host needs development headers/libraries for:

- Motif (`libXm`)
- Xt (`libXt`)
- X11
- Cairo with Xlib support
- Fontconfig
- FreeType
- the ordinary non-widget UXP dependencies required by the selected application

OpenMotif or a compatible Motif implementation should work on Linux.  The backend itself does not use GTK or GLib for its event loop.

## Implemented in this pass

### Build/platform selection

`cairo-motif` is a first-class `MOZ_WIDGET_TOOLKIT` choice and defines `MOZ_WIDGET_MOTIF`.  It is an X11 toolkit and links `Xm`, `Xt`, and `X11` directly.

### App shell

`nsAppShell` creates an `XtAppContext`, opens the X display through Xt, dispatches native events with `XtAppNextEvent`/`XtDispatchEvent`, and uses a nonblocking pipe registered with `XtAppAddInput` to wake UXP's `nsBaseAppShell`.

This is intentionally close to the event-loop architecture of Mozilla's historical Motif backend.

### Native windows

`nsWindow` provides:

- Xt top-level, dialog, and override-redirect popup shells
- `XmDrawingArea` child widgets
- show/hide/move/resize/focus
- WM_DELETE_WINDOW handling
- expose/configure/focus event translation
- basic X11 mouse and keyboard translation
- native X cursors
- pointer grabs for popup/capture behavior
- X11 native window/display handles
- Cairo-Xlib drawing targets for XUL painting
- UXP widget listener paint/resize/move notifications

### Graphics

`gfxPlatformMotif.cpp` reuses UXP's Fontconfig/FreeType and Xlib implementation rather than GTK/GDK.  It supplies:

- Cairo/Xlib offscreen surfaces
- XRender probing
- Fontconfig platform font list
- DPI calculation from the X screen
- fallback fonts
- X11 ICC profile lookup
- Xlib flushing

### Look and feel

`nsLookAndFeel` provides Motif/SGI-oriented system colors, classic 3D face/highlight/shadow colors, selection colors, common UI metrics, and Helvetica as the initial system font.

These are bootstrap defaults.  Resource-database-derived Motif colors/fonts can replace the constants after first bring-up.

### Other platform services

Implemented:

- X screen manager / screen objects
- bidi keyboard service fallback
- UXP transferable support
- selection/global clipboard slots
- XUL file picker fallback
- X11 GfxInfo registration

## Deliberate first-test limitations

These are not hidden behind GTK; they are simply the next native integrations after the application successfully creates and paints a Motif window.

- Clipboard data currently stays inside the UXP process.  X11 `PRIMARY`/`CLIPBOARD` selection ownership and conversion are not wired yet.
- Native Motif file selection is not wired yet; the XUL picker is registered as the fallback.  Historical Mozilla's `XmCreateFileSelectionDialog` implementation is the reference for the native follow-up.
- Drag and drop service is not registered yet.
- IME/XIM integration is not implemented yet.
- Custom image cursors are not implemented yet; standard X cursor-font shapes are.
- Mouse-wheel buttons are ignored until the dedicated wheel-event mapping is added.
- Skia is disabled for the bootstrap build.
- Printing and accessibility are disabled for the bootstrap build.
- `nsNativeTheme` is not yet backed by Motif drawing.  XUL chrome and the Linux/non-Mac theme assets remain responsible for chrome appearance during bring-up.
- Multi-screen `ScreenForNativeWidget` currently falls back to the primary X screen.

## Expected first milestone

The first useful test is deliberately small:

1. configure with `cairo-motif`;
2. link UXP without GTK widget libraries;
3. initialize the Xt application context;
4. create a top-level Motif shell;
5. paint XUL through a Cairo-Xlib surface;
6. receive keyboard and mouse events back through the UXP widget listener.

Compiler errors from this first build are expected to expose any exact API drift that cannot be checked without building the complete UXP application tree.  Keep the first error batch intact: the line numbers and signatures will let the remaining adaptation be mechanical rather than speculative.

## Historical fidelity rule

When an old Mozilla Motif implementation exists, preserve its original class/function names and semantics where UXP still has an equivalent concept.  New names should only be introduced for UXP concepts that did not exist in the historical source, and reconstructed behavior should be identified as such in comments.

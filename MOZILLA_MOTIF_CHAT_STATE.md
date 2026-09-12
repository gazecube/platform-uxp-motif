# Mozilla / Motif Project Chat State

Last updated: 2026-09-12

## Project

Repository:

```text
~/Projects/seamonkey-work
```

Remote/project context:

```text
gazecube/platform-uxp-motif
branch: motif-backend
```

Goal:

Port the UXP runtime to a real Motif widget backend, then run the actual old Mozilla/XPFE chrome on top of it. This is not a fake "classic" skin. The objective is a genuinely native Motif-backed Mozilla suitable for IID / IRIX-style environments.

## Current known commits

Relevant commits reached before the filesystem detour:

```text
92b32f...
0fd74a...
930b5b...
b50e6b...
5db06b...
b46cc3...
498822ec578de867d1c36632413c410fec2c2e50
5e7d29f9f0b637aec153962ed4512d4b9e411b21
```

Important recent fixes:

```text
498822ec578de867d1c36632413c410fec2c2e50
```

Fixed X display startup ordering in the Motif `nsAppShell`.

```text
5e7d29f9f0b637aec153962ed4512d4b9e411b21
```

"Treat invisible Motif windows as top-level shells"

This fixed the hidden startup XUL window path. `nsAppShellService::JustCreateTopWindow` creates an invisible window type; the Motif backend had excluded it from `mTopLevel`, causing it to be treated as a child with no parent.

## Current runtime state

The build successfully linked and launched far enough to create two empty windows.

It then crashed with SIGSEGV in the painting / layer-manager path.

Crash stack:

```text
nsBaseWidget::ShouldUseOffMainThreadCompositing()
nsBaseWidget::GetLayerManager(...)
nsWindow::Paint(XExposeEvent const&) widget/motif/nsWindow.cpp:661
nsWindow::HandleXEvent(_XEvent*) widget/motif/nsWindow.cpp:602
nsWindow::XtEventHandler(...)
nsAppShell::ProcessNextNativeEvent(bool)
...
```

Relevant base-widget code:

```cpp
bool nsBaseWidget::ShouldUseOffMainThreadCompositing()
{
  return gfxPlatform::UsesOffMainThreadCompositing();
}

LayerManager* nsBaseWidget::GetLayerManager(...)
{
  if (!mLayerManager) {
    if (!mShutdownObserver) return nullptr;

    if (ShouldUseOffMainThreadCompositing()) {
      NS_ASSERTION(aShadowManager == nullptr, ...);
      CreateCompositor();
    }

    if (!mLayerManager) {
      mLayerManager = CreateBasicLayerManager();
    }
  }

  return mLayerManager;
}
```

## Next high-confidence fix

The Motif backend should not attempt off-main-thread compositing yet.

The likely fix is to override:

```cpp
ShouldUseOffMainThreadCompositing()
```

in the Motif `nsWindow` class and force it to return `false`, causing the backend to stay on BasicLayers / main-thread painting.

Expected shape, subject to inspection of the actual class declaration:

```cpp
virtual bool ShouldUseOffMainThreadCompositing() override;
```

and:

```cpp
bool
nsWindow::ShouldUseOffMainThreadCompositing()
{
    return false;
}
```

Before patching, inspect the exact class/header layout and all existing declarations:

```bash
cd ~/Projects/seamonkey-work

grep -Rni "ShouldUseOffMainThreadCompositing" \
  widget/motif \
  widget/xpwidgets \
  widget 2>/dev/null | head -50

grep -n "class nsWindow" widget/motif/nsWindow.h

grep -n -A20 -B20 \
  "GetLayerManager\|ShouldUseOffMainThreadCompositing" \
  widget/xpwidgets/nsBaseWidget.cpp

grep -n -A30 -B20 \
  "Paint(" \
  widget/motif/nsWindow.cpp
```

Do not blindly patch until the exact inheritance and method visibility are confirmed.

## Build-performance context

The previous full link of `libxul` took roughly 49 minutes while the system was on Btrfs with Snapper integration.

That filesystem setup has now been removed.

The root filesystem migration is complete:

```text
/dev/sda2 xfs rw,noatime,inode64,logbufs=8,logbsize=32k,noquota
```

The system now boots successfully from XFS.

This means the next Mozilla build/link is also a useful test of whether the old Btrfs/Snapper setup was responsible for the extremely slow link stage.

## Filesystem migration detour - completed

Old root:

```text
Btrfs UUID:
ee5508b7-2fa2-46c0-bc97-c02af9301e52
```

New root:

```text
XFS UUID:
d8dd665c-a858-446e-8a32-32d914b34fa8
```

Boot partition:

```text
/dev/sda1
FAT32 UUID:
45FD-E2F8
```

The old Btrfs subvolumes were:

```text
@
@home
@root
@srv
@cache
@tmp
@log
```

These are now ordinary directories on the single XFS root.

The root was archived with tar using xattrs, ACLs, numeric ownership and sparse-file support, then restored onto XFS.

Final root mount verification:

```bash
findmnt -no SOURCE,FSTYPE,OPTIONS /
```

returned:

```text
/dev/sda2 xfs rw,noatime,inode64,logbufs=8,logbsize=32k,noquota
```

## Limine state

The generated kernel command line source is:

```text
/etc/default/limine
```

The old Btrfs line was:

```text
KERNEL_CMDLINE[default]+="quiet nowatchdog splash rw rootflags=subvol=/@ root=UUID=ee5508b7-2fa2-46c0-bc97-c02af9301e52"
```

The desired current line is intentionally verbose and does not use splash/quiet:

```text
KERNEL_CMDLINE[default]+="rw root=UUID=d8dd665c-a858-446e-8a32-32d914b34fa8"
```

`limine-mkinitcpio` is the correct wrapper for this system, because it both rebuilds the initramfs and updates `/boot/limine.conf`.

Do not use plain `mkinitcpio` from outside the chroot/live environment and interpret its `archiso` errors as installed-system failures.

## Snapper / Btrfs cleanup

The following snapshot-oriented packages were identified:

```text
btrfs-assistant 2.3.1-1.1
btrfs-progs 7.1-1
cachyos-snapper-support 1.0.2-1
limine-snapper-sync 1.31.0-1
snap-pac 3.0.1-3
snapper 0.13.1-3.1
```

The snapshot stack was removed from the installed system.

`btrfs-progs` can remain installed harmlessly for rescue/inspection use.

The old `/boot/limine.conf` snapshot tree generated by `limine-snapper-sync` was removed.

## Console font note

The console font is intentionally SGI Screen.

Files currently associated with it:

```text
/usr/share/kbd/consolefonts/Scr12.pcf
/usr/share/kbd/consolefonts/Scr12.bdf
```

The Linux console requires PSF/PSFU rather than PCF/BDF directly.

`bdf2psf` was used to convert the BDF. Missing-glyph warnings for box-drawing, arrows, shading, pi, etc. are expected if those glyphs are not present in SGI Screen.

The desired `/etc/vconsole.conf` setting is:

```text
FONT=Scr12
```

and the resulting PSF should live under:

```text
/usr/share/kbd/consolefonts/Scr12.psf
```

## Immediate next session plan

1. Stay focused on Mozilla/Motif. The XFS migration is done.
2. Inspect `nsWindow.h`, `nsWindow.cpp`, and `nsBaseWidget.cpp`.
3. Confirm whether `ShouldUseOffMainThreadCompositing()` is virtual and can be overridden cleanly in the Motif backend.
4. If confirmed, force Motif to return `false`.
5. Rebuild.
6. Measure whether XFS materially improves the `libxul` link time.
7. Launch and observe whether the two empty windows progress past the previous paint/compositor crash.
8. If the crash moves, capture the new backtrace before making another large change.

## Project philosophy

Keep the implementation behaviorally authentic and source-driven.

Prefer real widget/backend integration over visual imitation.

Avoid broad rewrites when a narrow backend-specific compatibility fix is sufficient.

Because full links are expensive, prefer one high-confidence patch at a time, with runtime verification after each.

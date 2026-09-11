# Mozilla 0.9 Navigator on the Motif UXP backend

This directory stages the first runnable Mozilla 0.9 Navigator window on the resurrected Motif widget backend.

The base window comes from `xpfe/browser/resources/content/navigator.xul` in `mozilla-source-0.9.tar.gz`.  For the first-boot milestone only, the old external overlay and script processing instructions are disabled so that Navigator can paint before the rest of the 0.9 chrome dependency graph (bookmarks, sidebar, security, communicator/global overlays) is restored.  The original toolbar, personal toolbar, browser area, RDF templates, status bar, event attributes, IDs, and period localization text remain in place.

The source snapshot contains a duplicated stray `onkeypress` attribute line immediately after the URL bar textbox; that malformed duplicate is removed here so current UXP can parse the document.

On Unix, Mozilla 0.9 relied on the host widget set for platform appearance.  Accordingly this first-boot window does not import the Windows or Mac classic platform CSS.  Motif is intentionally allowed to supply the host widget appearance.

The staged files land directly in `dist/bin`, including `application.ini`, so the ordinary `./mach run` command launches this app when building with `--enable-default-toolkit=motif`.

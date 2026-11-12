# Handbücher (keller54.de/handbuch/)

Quelle der Handbuch-Seiten auf keller54.de. Pro Plugin eine Datei:

- `k-54.html` → /handbuch/k-54/
- `k-54-s.html` → /handbuch/k-54-s/
- `analyzer.html` → /handbuch/analyzer/

Jede Datei beginnt mit einer `meta`-Zeile (Titel, Version, Kurztext) und besteht aus
`<section id="…">`-Abschnitten. Die `id` ist der Sprunganker für den Hilfe-Link im Plugin
(z. B. `/handbuch/k-54/#looper`) – Anker nicht umbenennen.

**Bei jeder neuen Version:** Text und `version` in der meta-Zeile anpassen, `STAND` in
`build.py` setzen, `python3 Handbuch/build.py Handbuch/dist/pages.json` ausführen und die
Seiten auf keller54.de aktualisieren (Inhalt aus `dist/pages.json`).

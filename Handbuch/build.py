#!/usr/bin/env python3
"""Baut aus den Handbuch-Quellen (*.html) die WordPress-Seiteninhalte für keller54.de.

Aufruf: python3 Handbuch/build.py <ausgabe.json>
Jede Quelle beginnt mit <!-- meta: title=… | slug=… | version=… | kicker=… | lead=… -->
und enthält <section id="…"><h2>…</h2>…</section>-Abschnitte. Die Anker (id) sind die
Sprungziele für den Hilfe-Link in den Plugins, z. B. /handbuch/k-54/#looper.
"""
import json, re, sys, html, pathlib

HERE = pathlib.Path(__file__).parent
STAND = "12.11.2026"
ORDER = ["k-54", "k-54-s", "analyzer"]

CSS = """<style>
.k54h-wrap{max-width:980px;margin:0 auto}
.k54h-bar{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin:0 0 22px}
.k54h-badge{display:inline-block;border:1px solid rgba(212,175,106,.45);color:#d4af6a;border-radius:999px;padding:4px 12px;font-size:12px;letter-spacing:.14em;text-transform:uppercase}
.k54h-btn{display:inline-block;cursor:pointer;border:1px solid #d4af6a;background:transparent;color:#d4af6a;border-radius:10px;padding:8px 14px;font:inherit;font-size:14px;text-decoration:none}
.k54h-btn:hover{background:rgba(212,175,106,.12)}
.k54h-btn.is-gold{background:#d4af6a;color:#141414;font-weight:700}
.k54h-toc{border:1px solid rgba(212,175,106,.22);border-radius:16px;padding:18px 22px;background:rgba(255,255,255,.02);margin:0 0 28px}
.k54h-toc h2{font-size:13px;letter-spacing:.18em;text-transform:uppercase;color:#d4af6a;margin:0 0 10px}
.k54h-toc ol{margin:0;padding-left:20px;columns:2 220px;column-gap:28px}
.k54h-toc li{margin:0 0 6px;break-inside:avoid}
.k54h-toc a,.k54h-doc a{color:#d4af6a}
.k54h-doc section{border:1px solid rgba(212,175,106,.16);border-radius:16px;padding:22px 24px;margin:0 0 22px;background:rgba(255,255,255,.015);scroll-margin-top:90px}
.k54h-doc h2{margin:0 0 12px;letter-spacing:.06em}
.k54h-doc h3{margin:18px 0 8px;font-size:15px;letter-spacing:.12em;text-transform:uppercase;color:#d4af6a}
.k54h-doc p{line-height:1.65;margin:0 0 12px}
.k54h-list{margin:0 0 12px;padding-left:20px}
.k54h-list li{margin:0 0 7px;line-height:1.6}
.k54h-steps{list-style:none;margin:0 0 12px;padding:0;counter-reset:s}
.k54h-steps li{counter-increment:s;position:relative;padding:0 0 12px 38px;line-height:1.6}
.k54h-steps li:before{content:counter(s);position:absolute;left:0;top:0;width:24px;height:24px;border-radius:50%;border:1px solid #d4af6a;color:#d4af6a;font-size:12px;font-weight:700;display:flex;align-items:center;justify-content:center}
.k54h-table{width:100%;border-collapse:collapse;margin:4px 0 14px;font-size:14.5px}
.k54h-table th,.k54h-table td{text-align:left;vertical-align:top;padding:8px 10px;border-bottom:1px solid rgba(212,175,106,.14)}
.k54h-table th{color:#d4af6a;font-weight:600;white-space:nowrap}
.k54h-note{font-size:14px;opacity:.82;border-top:1px solid rgba(212,175,106,.15);padding-top:10px}
.k54h-warn{border-left:3px solid #e8913a;padding:8px 12px;background:rgba(232,145,58,.08);border-radius:6px}
.k54h-tag{display:inline-block;font-size:10px;letter-spacing:.14em;border:1px solid rgba(212,175,106,.5);color:#d4af6a;border-radius:6px;padding:1px 6px;vertical-align:middle}
.k54h-two{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:18px}
.k54h-doc code{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.9em;background:rgba(212,175,106,.08);border-radius:5px;padding:1px 5px;word-break:break-all}
.k54h-cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:20px}
.k54h-card{border:1px solid rgba(212,175,106,.25);border-radius:16px;padding:22px 24px;background:rgba(255,255,255,.02);display:flex;flex-direction:column;gap:10px}
.k54h-card h3{margin:0;letter-spacing:.06em}
.k54h-card p{margin:0;line-height:1.55;opacity:.9}
.k54h-card .k54h-btn{align-self:flex-start;margin-top:auto}
@media (max-width:640px){.k54h-doc section{padding:18px 16px}.k54h-table{font-size:13.5px}.k54h-table th{white-space:normal}}
@media print{
 header,footer,nav,#wpadminbar,.site-header,.site-footer,.k54-actions,.k54h-noprint,#k54-cookie-hinweis,[class*="cookie"]{display:none!important}
 html,body,.k54-land,.k54h-doc section,.k54h-toc{background:#fff!important;color:#000!important}
 .k54h-doc section{border:1px solid #bbb;break-inside:auto;page-break-inside:auto}
 .k54h-doc h2{break-after:avoid}
 .k54h-doc h3,.k54h-toc h2,.k54h-table th,.k54h-badge,.k54h-tag,.k54h-doc a,.k54h-toc a{color:#7a5a12!important}
 .k54-hero *{color:#000!important}
 a[href^="http"]:after{content:""}
}
</style>"""


def parse(src: str):
    m = re.search(r"<!--\s*meta:(.*?)-->", src, re.S)
    meta = {}
    for part in m.group(1).split("|"):
        k, v = part.split("=", 1)
        meta[k.strip()] = v.strip()
    body = src[m.end():].strip()
    toc = re.findall(r'<section id="([^"]+)"><h2>(.*?)</h2>', body)
    return meta, body, toc


def hero(h1, kicker, lead):
    return f"""<!-- wp:group {{"className":"k54-hero"}} -->
<div class="wp-block-group k54-hero">
<!-- wp:heading {{"level":1,"className":"k54-h1"}} -->
<h1 class="wp-block-heading k54-h1">{h1}</h1>
<!-- /wp:heading -->
<!-- wp:paragraph {{"className":"k54-kicker"}} -->
<p class="k54-kicker">{kicker}</p>
<!-- /wp:paragraph -->
<!-- wp:paragraph {{"className":"k54-lead"}} -->
<p class="k54-lead">{lead}</p>
<!-- /wp:paragraph -->
</div>
<!-- /wp:group -->"""


def wrap(inner_html):
    return f"""<!-- wp:group {{"align":"full","className":"k54-land"}} -->
<div class="wp-block-group alignfull k54-land">

<!-- wp:html -->
{CSS}
<!-- /wp:html -->

{inner_html}

</div>
<!-- /wp:group -->"""


def section(html_block):
    return f"""<!-- wp:group {{"className":"k54-section"}} -->
<div class="wp-block-group k54-section">
<!-- wp:html -->
{html_block}
<!-- /wp:html -->
</div>
<!-- /wp:group -->"""


def manual_page(meta, body, toc):
    toc_html = "".join(f'<li><a href="#{i}">{re.sub(r"<[^>]+>", "", t)}</a></li>' for i, t in toc)
    bar = (f'<div class="k54h-bar k54h-noprint"><span class="k54h-badge">Version {meta["version"]} · Stand {STAND}</span>'
           f'<button class="k54h-btn is-gold" type="button" onclick="window.print()">Als PDF speichern</button>'
           f'<a class="k54h-btn" href="/mein-bereich/#/downloads">Zum Download</a>'
           f'<a class="k54h-btn" href="/installieren/">Installieren – Hilfe</a>'
           f'<a class="k54h-btn" href="/handbuch/">Alle Handbücher</a></div>')
    doc = (f'<div class="k54h-wrap">{bar}<nav class="k54h-toc"><h2>Inhalt</h2><ol>{toc_html}</ol></nav>'
           f'<div class="k54h-doc">{body}</div>'
           f'<p class="k54h-note">Fragen oder ein Fehler im Handbuch? Schreib mir an <a href="mailto:info@keller54.de">info@keller54.de</a> – Frank E.</p></div>')
    name = meta["title"].replace("Handbuch ", "")
    return wrap(hero(meta["title"], meta["kicker"], meta["lead"]) + "\n\n" + section(doc))


def overview(entries):
    cards = "".join(
        f'<div class="k54h-card"><span class="k54h-badge">Version {m["version"]}</span>'
        f'<h3>{m["title"].replace("Handbuch ", "")}</h3><p>{m["kicker"]}</p>'
        f'<a class="k54h-btn is-gold" href="/handbuch/{m["slug"]}/">Handbuch öffnen</a></div>'
        for m in entries)
    doc = (f'<div class="k54h-wrap"><div class="k54h-cards">{cards}</div>'
           f'<p class="k54h-note" style="margin-top:22px">Jedes Handbuch gibt es auch als PDF: auf der Handbuch-Seite oben auf <strong>„Als PDF speichern“</strong> klicken und im Druckfenster „Als PDF sichern“ bzw. „Microsoft Print to PDF“ wählen. Die Handbücher werden mit jeder neuen Version aktualisiert.</p></div>')
    return wrap(hero("Handbücher", "Anleitungen zu allen Keller54-Plugins",
                     "Hier findest du zu jedem Plugin eine Anleitung in einfacher Sprache – vom ersten Start bis zu den Profi-Funktionen.")
                + "\n\n" + section(doc))


def main(out):
    pages = {}
    metas = []
    for slug in ORDER:
        meta, body, toc = parse((HERE / f"{slug}.html").read_text(encoding="utf-8"))
        assert meta["slug"] == slug
        pages[slug] = {"title": meta["title"], "content": manual_page(meta, body, toc)}
        metas.append(meta)
    pages["_index"] = {"title": "Handbücher", "content": overview(metas)}
    pathlib.Path(out).write_text(json.dumps(pages, ensure_ascii=False), encoding="utf-8")
    for k, v in pages.items():
        print(k, len(v["content"]))


if __name__ == "__main__":
    main(sys.argv[1])

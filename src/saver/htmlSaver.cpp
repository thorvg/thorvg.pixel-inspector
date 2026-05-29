/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>

#include "htmlSaver.h"
#include "common.h"

namespace
{

// Evaluator::metrics() defines the first metric as the primary report metric.
constexpr size_t PrimaryMetric = 0;

std::string _html(const std::string& text)
{
    std::string ret;
    for (auto c : text) {
        if (c == '&') ret += "&amp;";
        else if (c == '<') ret += "&lt;";
        else if (c == '>') ret += "&gt;";
        else if (c == '"') ret += "&quot;";
        else ret += c;
    }
    return ret;
}

std::string _link(const std::filesystem::path& from, const std::filesystem::path& to)
{
    return _html(to.lexically_relative(from.parent_path()).string());
}

float _metricValue(const TestResult::Comparison& comparison, size_t index)
{
    return index < comparison.metricValues.size() ? comparison.metricValues[index] : 0.0f;
}

static constexpr auto HtmlHead = R"HTML(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ThorVG Pixel Inspector</title>
<style>
)HTML";

static constexpr auto Style = R"CSS(
*, ::before, ::after { box-sizing: border-box; }
body { margin: 0; background: #f6f8fa; color: #24292f; font: 13px/1.45 -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
button, input, select, textarea { font: inherit; }
main { max-width: 1440px; margin: 0 auto; padding: 24px; }
h1 { margin: 0 0 8px; font-size: 24px; }
h2 { margin: 24px 0 10px; font-size: 17px; }
.muted, .meta dt, th, .toolbar label, .pixel-inspector b, .coord { color: #57606a; }
.panel, .summary div { border: 1px solid #d0d7de; background: #fff; border-radius: 6px; }
.panel { margin: 14px 0; padding: 14px; }
.meta { display: grid; grid-template-columns: max-content 1fr; gap: 5px 12px; max-width: 760px; }
.meta dd, .asset { margin: 0; word-break: break-all; }
.summary { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 10px; }
.summary div { padding: 12px; }
.summary b { display: block; margin-top: 4px; font-size: 24px; }
.toolbar { position: sticky; top: 0; z-index: 6; display: flex; flex-wrap: wrap; gap: 10px; align-items: center; margin: 0 0 14px; padding: 10px 14px; }
.toolbar label { display: grid; gap: 4px; font-size: 11px; font-weight: 700; text-transform: uppercase; }
.toolbar input[type=range] { width: 220px; height: 32px; padding: 0; accent-color: #0969da; }
#diff-threshold-value { display: inline-block; min-width: 52px; font-variant-numeric: tabular-nums; }
#visible-count { display: inline-block; min-width: 4ch; text-align: right; font-variant-numeric: tabular-nums; }
#theme-toggle { min-width: 70px; }
.toolbar input[type=search] { height: 32px; min-width: 200px; padding: 0 10px; border: 1px solid #d0d7de; border-radius: 6px; background: #fff; }
.toolbar input[type=search].invalid { border-color: #cf222e; }
.tabs { display: inline-flex; gap: 4px; margin-right: auto; }
.tab, #only-diff-toggle, #theme-toggle { height: 32px; padding: 0 14px; border: 1px solid #d0d7de; border-radius: 6px; background: #f6f8fa; font-weight: 700; cursor: pointer; }
.tab { text-transform: uppercase; }
.tab[aria-selected="true"], #only-diff-toggle[aria-pressed="true"] { border-color: #0969da; background: #0969da; color: #fff; }
body.dark { background: #0d1117; color: #c9d1d9; }
body.dark .panel, body.dark .summary div, body.dark table, body.dark .tab, body.dark #only-diff-toggle, body.dark #theme-toggle, body.dark summary { background: #161b22; border-color: #30363d; color: #c9d1d9; }
body.dark th { background: #21262d; }
body.dark th, body.dark td { border-color: #30363d; }
body.dark .images { background: #161b22; }
body.dark .muted, body.dark th, body.dark .coord, body.dark .pixel-inspector b { color: #8b949e; }
body.dark .tab[aria-selected="true"], body.dark #only-diff-toggle[aria-pressed="true"] { background: #1f6feb; border-color: #1f6feb; color: #fff; }
body.dark .toolbar input[type=search] { background: #0d1117; border-color: #30363d; color: #c9d1d9; }
body.dark .pixel-inspector { background: #161b22; border-color: #30363d; }
body.dark .pixel-inspector canvas { background: #161b22; border-color: #30363d; }
summary { display: inline-flex; margin-bottom: 8px; padding: 6px 10px; border: 1px solid #d0d7de; border-radius: 4px; background: #f0f3f6; font-weight: 700; cursor: pointer; }
table { width: 100%; table-layout: fixed; border-collapse: collapse; background: #fff; }
th, td { padding: 7px; border: 1px solid #d0d7de; vertical-align: top; }
th { background: #f0f3f6; font-size: 11px; text-align: left; text-transform: uppercase; }
.comparison th:nth-child(1) { width: 6%; }
.comparison th:nth-child(2) { width: 20%; }
.comparison th:nth-child(3), .comparison th:nth-child(4), .comparison th:nth-child(5) { width: 18%; }
.pass { color: #1a7f37; }
.diff, .failed { color: #cf222e; }
.images { min-width: 190px; text-align: center; background: #fbfbfc; }
.images img { display: block; max-width: 100%; max-height: 210px; margin: 0 auto; border: 1px solid #d8dee4; }
.pixel-inspector { position: fixed; display: none; z-index: 10; padding: 10px; border: 1px solid #8c959f; background: #fff; box-shadow: 0 12px 32px rgba(31,35,40,.18); pointer-events: none; }
.pixel-inspector.on { display: block; }
.coord { margin: 0 0 8px; font-size: 12px; }
.views { display: grid; grid-template-columns: repeat(3, 180px); gap: 8px; }
.pixel-inspector b { display: block; margin: 0 0 4px; font-size: 11px; text-transform: uppercase; }
.pixel-inspector canvas { width: 180px; height: 180px; border: 1px solid #d0d7de; background: #fbfbfc; image-rendering: pixelated; }
.rgba { margin-top: 4px; font-size: 11px; }
.failed-list { margin: 8px 0 0; padding-left: 20px; }
tr[hidden] { display: none; }
@media (max-width: 900px) {
    main { padding: 14px; }
    .summary { grid-template-columns: 1fr 1fr; }
    .comparison { display: block; overflow-x: auto; }
    .views { grid-template-columns: 1fr; }
}
@media print {
    @page { size: A4 landscape; margin: 8mm; }
    * { box-shadow: none !important; }
    body { background: #fff; color: #111; font-size: 9px; }
    main { max-width: none; padding: 0; }
    h1 { margin: 0 0 4mm; font-size: 18px; }
    h2 { margin: 0 0 3mm; font-size: 13px; }
    .toolbar, .pixel-inspector { display: none !important; }
    .panel { margin: 0 0 5mm; padding: 0; border: 0; background: #fff; break-inside: auto; }
    .meta { max-width: none; margin: 0 0 4mm; padding: 3mm; border: 1px solid #bbb; grid-template-columns: 22mm 1fr; }
    .summary { grid-template-columns: repeat(4, 1fr); gap: 3mm; margin: 0 0 5mm; }
    .summary div { padding: 3mm; border-color: #bbb; }
    .summary b { font-size: 16px; }
    summary, thead { display: none; }
    .comparison { overflow: visible; }
    table, tbody { display: block; width: 100% !important; }
    tr { display: grid !important; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 2mm 3mm; margin: 0 0 5mm; padding: 3mm; border: 1px solid #bbb; break-inside: avoid; page-break-inside: avoid; background: #fff; }
    tr[hidden] { display: none !important; }
    td { min-width: 0; padding: 0; border: 0; }
    td::before { display: block; margin: 0 0 1mm; color: #555; font-size: 7px; font-weight: 700; text-transform: uppercase; content: attr(data-label); }
    .status { grid-column: 1 / 2; grid-row: 1; font-size: 8px; text-transform: uppercase; }
    .asset { grid-column: 2 / -1; grid-row: 1; font-size: 8px; overflow-wrap: anywhere; word-break: break-word; }
    .images { width: auto !important; min-width: 0; background: #fff; text-align: left; break-inside: avoid; }
    .images img { width: 100%; height: 42mm; max-width: 100%; max-height: 42mm; object-fit: contain; border-color: #bbb; }
    td.num { display: flex !important; gap: 2mm; align-items: baseline; padding: 1.5mm 2mm; border: 1px solid #ddd; border-radius: 2mm; background: #fafafa; text-align: left; font-size: 8px; overflow-wrap: anywhere; }
    .num::before { flex: 1 1 auto; margin: 0; }
}
)CSS";

static constexpr auto Script = R"JS(
(() => {
  const diffThreshold = document.getElementById('diff-threshold');
  const diffThresholdValue = document.getElementById('diff-threshold-value');
  const onlyDiffToggle = document.getElementById('only-diff-toggle');
  const themeToggle = document.getElementById('theme-toggle');
  const assetSearch = document.getElementById('asset-search');
  const visibleCount = document.getElementById('visible-count');
  const rows = [...document.querySelectorAll('tr[data-diff-ratio]')];
  const summaries = [...document.querySelectorAll('summary[data-total]')];
  const tabs = [...document.querySelectorAll('.tab')];
  const panels = [...document.querySelectorAll('section[data-backend]')];

  const applyThreshold = () => {
    const diffValue = Number.parseFloat(diffThreshold.value);
    const diffLimit = Number.isFinite(diffValue) ? diffValue : 0;
    const onlyDiff = onlyDiffToggle.getAttribute('aria-pressed') === 'true';
    diffThresholdValue.textContent = diffLimit.toFixed(4);
    let search = null;
    if (assetSearch.value) {
      try { search = new RegExp(assetSearch.value, 'i'); assetSearch.classList.remove('invalid'); }
      catch (_) { assetSearch.classList.add('invalid'); }
    } else {
      assetSearch.classList.remove('invalid');
    }
    let visible = 0;
    summaries.forEach((summary) => summary.dataset.shownCount = '0');
    rows.forEach((row) => {
      const panel = row.closest('section[data-backend]');
      const show = panel && !panel.hidden &&
                   Number.parseFloat(row.dataset.diffRatio) >= diffLimit &&
                   (!onlyDiff || row.dataset.different === '1') &&
                   (!search || search.test(row.dataset.asset));
      row.hidden = !show;
      if (show) {
        ++visible;
        const summary = panel.querySelector('summary[data-total]');
        summary.dataset.shownCount = String(Number.parseInt(summary.dataset.shownCount, 10) + 1);
      }
    });
    summaries.forEach((summary) => {
      summary.textContent = `Comparisons (${summary.dataset.shownCount} shown / ${summary.dataset.total} total)`;
    });
    visibleCount.textContent = visible;
  };

  const selectTab = (name) => {
    tabs.forEach((tab) => tab.setAttribute('aria-selected', String(tab.dataset.tab === name)));
    panels.forEach((panel) => { panel.hidden = name !== 'all' && panel.dataset.backend !== name; });
    applyThreshold();
  };

  diffThreshold.addEventListener('input', applyThreshold);
  assetSearch.addEventListener('input', applyThreshold);
  onlyDiffToggle.addEventListener('click', () => {
    const pressed = onlyDiffToggle.getAttribute('aria-pressed') === 'true';
    onlyDiffToggle.setAttribute('aria-pressed', String(!pressed));
    applyThreshold();
  });
  themeToggle.addEventListener('click', () => {
    const dark = document.body.classList.toggle('dark');
    themeToggle.textContent = dark ? 'Light' : 'Dark';
  });
  tabs.forEach((tab) => tab.addEventListener('click', () => selectTab(tab.dataset.tab)));
  // Allow preselecting a tab via URL hash (e.g. reporter.html#sw) for per-backend printing.
  const hashTab = decodeURIComponent(location.hash.slice(1));
  const initial = tabs.some((tab) => tab.dataset.tab === hashTab) ? hashTab : (tabs[0] && tabs[0].dataset.tab);
  if (tabs.length) selectTab(initial);
  else applyThreshold();

  const inspector = document.getElementById('pixel-inspector');
  const coord = inspector.querySelector('.coord');
  const canvases = {
    reference: inspector.querySelector('canvas[data-role="reference"]'),
    test: inspector.querySelector('canvas[data-role="test"]'),
    diff: inspector.querySelector('canvas[data-role="diff"]')
  };
  const labels = {
    reference: inspector.querySelector('.rgba[data-role="reference"]'),
    test: inspector.querySelector('.rgba[data-role="test"]'),
    diff: inspector.querySelector('.rgba[data-role="diff"]')
  };
  const pixel = document.createElement('canvas');
  pixel.width = 1;
  pixel.height = 1;
  const pixelCtx = pixel.getContext('2d', {willReadFrequently: true});
  const sample = 15;
  const scale = 12;
  const half = Math.floor(sample / 2);

  const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

  const imagePoint = (event, img) => {
    const rect = img.getBoundingClientRect();
    return {
      x: clamp(Math.floor((event.clientX - rect.left) * img.naturalWidth / rect.width), 0, img.naturalWidth - 1),
      y: clamp(Math.floor((event.clientY - rect.top) * img.naturalHeight / rect.height), 0, img.naturalHeight - 1)
    };
  };

  const rgba = (img, x, y) => {
    try {
      pixelCtx.clearRect(0, 0, 1, 1);
      pixelCtx.drawImage(img, x, y, 1, 1, 0, 0, 1, 1);
      const p = pixelCtx.getImageData(0, 0, 1, 1).data;
      return `RGBA ${p[0]}, ${p[1]}, ${p[2]}, ${p[3]}`;
    } catch (_) {
      return 'RGBA -';
    }
  };

  const draw = (canvas, img, x, y) => {
    const ctx = canvas.getContext('2d');
    const sx = clamp(x - half, 0, img.naturalWidth - 1);
    const sy = clamp(y - half, 0, img.naturalHeight - 1);
    const ex = clamp(x + half + 1, 1, img.naturalWidth);
    const ey = clamp(y + half + 1, 1, img.naturalHeight);
    const sw = ex - sx;
    const sh = ey - sy;
    const dx = (half - (x - sx)) * scale;
    const dy = (half - (y - sy)) * scale;
    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if (sw > 0 && sh > 0) ctx.drawImage(img, sx, sy, sw, sh, dx, dy, sw * scale, sh * scale);
    ctx.strokeStyle = '#0969da';
    ctx.lineWidth = 2;
    ctx.strokeRect(half * scale + 1, half * scale + 1, scale - 2, scale - 2);
  };

  const moveInspector = (clientX, clientY) => {
    const margin = 14;
    let left = clientX + margin;
    let top = clientY + margin;
    const rect = inspector.getBoundingClientRect();
    if (left + rect.width > window.innerWidth) left = clientX - rect.width - margin;
    if (top + rect.height > window.innerHeight) top = clientY - rect.height - margin;
    inspector.style.left = `${Math.max(margin, left)}px`;
    inspector.style.top = `${Math.max(margin, top)}px`;
  };

  const renderInspector = (tr, sourceImg, point, clientX, clientY) => {
    if (!tr || tr.hidden || !sourceImg.complete || !sourceImg.naturalWidth) return;

    coord.textContent = `x ${point.x}, y ${point.y}`;
    for (const role of Object.keys(canvases)) {
      const target = tr.querySelector(`img[data-role="${role}"]`);
      if (!target || !target.complete || !target.naturalWidth) continue;
      const x = clamp(Math.round(point.x * target.naturalWidth / sourceImg.naturalWidth), 0, target.naturalWidth - 1);
      const y = clamp(Math.round(point.y * target.naturalHeight / sourceImg.naturalHeight), 0, target.naturalHeight - 1);
      draw(canvases[role], target, x, y);
      labels[role].textContent = rgba(target, x, y);
    }

    moveInspector(clientX, clientY);
    inspector.classList.add('on');
  };

  const showInspector = (event, img) => {
    const tr = img.closest('tr');
    if (!tr || tr.hidden || !img.complete || !img.naturalWidth) return;

    renderInspector(tr, img, imagePoint(event, img), event.clientX, event.clientY);
  };

  document.addEventListener('mousemove', (event) => {
    const img = event.target.closest && event.target.closest('img[data-role]');
    if (img) showInspector(event, img);
    else inspector.classList.remove('on');
  });

  document.addEventListener('scroll', () => {
    inspector.classList.remove('on');
  }, true);
})();
)JS";

TestResult::Summary _total(const TestResult& result)
{
    TestResult::Summary total;
    for (const auto& backend : result.backends) {
        total.compared += backend.summary.compared;
        total.different += backend.summary.different;
        total.failed += backend.summary.failed;
    }
    return total;
}

void _writeSummary(std::ofstream& report, const TestResult& result)
{
    const auto total = _total(result);
    size_t stored = 0;
    for (const auto& backend : result.backends) stored += backend.comparisons.size();

    report << "<section class=\"summary\">";
    report << "<div><span>compared</span><b>" << total.compared << "</b></div>";
    report << "<div><span>stored</span><b>" << stored << "</b></div>";
    report << "<div><span>different</span><b>" << total.different << "</b></div>";
    report << "<div><span>failed</span><b class=\"failed\">" << total.failed << "</b></div>";
    report << "</section>";
}

void _writeToolbar(std::ofstream& report, const TestResult& result)
{
    report << "<section class=\"panel toolbar\">";
    report << "<div class=\"tabs\" role=\"tablist\">";
    report << "<button class=\"tab\" type=\"button\" data-tab=\"all\" aria-selected=\"true\">all</button>";
    for (const auto& backend : result.backends) {
        const auto name = _html(backend.name);
        report << "<button class=\"tab\" type=\"button\" data-tab=\"" << name << "\" aria-selected=\"false\">" << name << "</button>";
    }
    report << "</div>";
    report << "<input id=\"asset-search\" type=\"search\" placeholder=\"filter asset (regex)\">";
    report << "<label>Diff Ratio<input id=\"diff-threshold\" type=\"range\" min=\"0\" max=\"1\" step=\"0.0005\" value=\"" << result.config.threshold.diffRatio << "\"></label>";
    report << "<output id=\"diff-threshold-value\" for=\"diff-threshold\">" << result.config.threshold.diffRatio << "</output>";
    report << "<button id=\"only-diff-toggle\" type=\"button\" aria-pressed=\"true\">Only diff</button>";
    report << "<button id=\"theme-toggle\" type=\"button\">Dark</button>";
    report << "<span class=\"muted\">visible <strong id=\"visible-count\">0</strong></span>";
    report << "</section>";
}

void _writeImage(std::ofstream& report, const std::filesystem::path& reportPath, const std::string& path, const char* label)
{
    report << "<td class=\"images\" data-label=\"" << label << "\"><img loading=\"lazy\" data-role=\"" << label << "\" alt=\"" << label << "\" src=\"" << _link(reportPath, path) << "\"></td>";
}

void _writeMetricHeader(std::ofstream& report, const TestResult& result)
{
    for (const auto& metric : result.metrics) report << "<th>" << _html(metric.label) << "</th>";
}

void _writeMetricCells(std::ofstream& report, const TestResult& result, const TestResult::Comparison& comparison)
{
    for (size_t i = 0; i < result.metrics.size(); ++i) {
        const auto label = _html(result.metrics[i].label);
        report << "<td class=\"num\" data-label=\"" << label << "\">" << _metricValue(comparison, i) << "</td>";
    }
}

void _writeFailed(std::ofstream& report, const TestResult::BackendResult& backend)
{
    if (backend.failed.empty()) return;

    report << "<h3>Failed</h3><ul class=\"failed-list\">";
    for (const auto& failure : backend.failed) report << "<li><code>" << _html(failure) << "</code></li>";
    report << "</ul>";
}

void _writeBackend(std::ofstream& report, const TestResult& result, const TestResult::BackendResult& backend, const std::filesystem::path& reportPath)
{
    report << "<section class=\"panel backend\" data-backend=\"" << _html(backend.name) << "\">";
    report << "<h2>" << _html(backend.name) << "</h2>";
    report << "<p class=\"muted\">compared " << backend.summary.compared
           << ", stored " << backend.comparisons.size()
           << ", different " << backend.summary.different
           << ", failed " << backend.summary.failed << "</p>";

    auto comparisons = backend.comparisons;
    std::sort(comparisons.begin(), comparisons.end(), [](const auto& a, const auto& b) {
        return _metricValue(a, PrimaryMetric) > _metricValue(b, PrimaryMetric);
    });

    report << "<details open><summary data-total=\"" << comparisons.size() << "\">Comparisons (" << comparisons.size() << " shown / " << comparisons.size() << " total)</summary>";
    report << "<div class=\"comparison\"><table><thead><tr>";
    report << "<th>status</th><th>asset</th><th>reference</th><th>test</th><th>diff</th>";
    _writeMetricHeader(report, result);
    report << "</tr></thead><tbody>";
    for (const auto& comparison : comparisons) {
        report << "<tr data-diff-ratio=\"" << _metricValue(comparison, PrimaryMetric)
               << "\" data-different=\"" << (comparison.different ? 1 : 0)
               << "\" data-asset=\"" << _html(comparison.asset) << "\">";
        report << "<td class=\"status " << (comparison.different ? "diff" : "pass") << "\" data-label=\"status\">" << (comparison.different ? "diff" : "pass") << "</td>";
        report << "<td class=\"asset\" data-label=\"asset\">" << _html(comparison.asset) << "</td>";
        _writeImage(report, reportPath, comparison.reference, "reference");
        _writeImage(report, reportPath, comparison.test, "test");
        _writeImage(report, reportPath, comparison.diff, "diff");
        _writeMetricCells(report, result, comparison);
        report << "</tr>";
    }
    report << "</tbody></table></div></details>";

    _writeFailed(report, backend);
    report << "</section>";
}

void _writeInspector(std::ofstream& report)
{
    report << R"HTML(
<div id="pixel-inspector" class="pixel-inspector">
  <div class="coord">x -, y -</div>
  <div class="views">
    <div><b>reference</b><canvas data-role="reference" width="180" height="180"></canvas><div class="rgba" data-role="reference">RGBA -</div></div>
    <div><b>test</b><canvas data-role="test" width="180" height="180"></canvas><div class="rgba" data-role="test">RGBA -</div></div>
    <div><b>diff</b><canvas data-role="diff" width="180" height="180"></canvas><div class="rgba" data-role="diff">RGBA -</div></div>
  </div>
</div>
)HTML";
}

}  // namespace

bool HtmlSaver::save(const TestResult& result, const std::string& artifactsDir)
{
    std::error_code error;
    std::filesystem::create_directories(artifactsDir, error);
    if (error) return false;

    const auto reportPath = std::filesystem::path(artifactsDir) / "reporter.html";
    std::ofstream report(reportPath);
    if (!report.is_open()) return false;

    report << std::setprecision(6);
    report << HtmlHead << Style;
    report << "</style></head><body><main>";
    report << "<h1>ThorVG Pixel Inspector</h1>";
    _writeToolbar(report, result);
    _writeSummary(report, result);
    for (const auto& backend : result.backends) _writeBackend(report, result, backend, reportPath);

    report << "</main>";
    _writeInspector(report);
    report << "<script>\n" << Script << "\n</script>";
    report << "</body></html>";
    return true;
}

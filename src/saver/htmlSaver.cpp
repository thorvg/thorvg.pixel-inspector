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
#include "jsonSaver.h"
#include "common.h"

namespace
{

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

const char* _title(EvaluatorType evaluatorType)
{
    switch (evaluatorType) {
        case EvaluatorType::Pixel: {
            return "ThorVG Pixel Test Pixel Report";
        }
        case EvaluatorType::Flip: {
            return "ThorVG Pixel Test FLIP Report";
        }
    }
    return "ThorVG Pixel Test Report";
}

float _threshold(const TestResult& result)
{
    switch (result.config.evaluatorType) {
        case EvaluatorType::Pixel: {
            return result.config.pixel.surfaceDiffRatioThreshold;
        }
        case EvaluatorType::Flip: {
            return result.config.flip.surfaceMeanThreshold;
        }
    }
    return 0.0f;
}

const char* _thresholdStep(EvaluatorType evaluatorType)
{
    switch (evaluatorType) {
        case EvaluatorType::Pixel: {
            return "0.0005";
        }
        case EvaluatorType::Flip: {
            return "0.0005";
        }
    }
    return "0.0005";
}

float _metricValue(const TestResult::Comparison& comparison, size_t index)
{
    return index < comparison.metricValues.size() ? comparison.metricValues[index] : 0.0f;
}

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

void _writeStyle(std::ofstream& report)
{
    report << R"(
body{margin:0;background:#f6f8fa;color:#24292f;font:13px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;}
main{max-width:1440px;margin:0 auto;padding:24px;}
h1{margin:0 0 8px;font-size:24px;}h2{margin:24px 0 10px;font-size:17px;}
.muted{color:#57606a}.panel{margin:14px 0;padding:14px;border:1px solid #d0d7de;background:#fff;border-radius:6px;}
.meta{display:grid;grid-template-columns:max-content 1fr;gap:5px 12px;max-width:760px}.meta dt{color:#57606a}.meta dd{margin:0;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;word-break:break-all;}
.summary{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px}.summary div{padding:12px;border:1px solid #d0d7de;background:#fff;border-radius:6px}.summary span{display:block;color:#57606a;font-size:11px;text-transform:uppercase}.summary b{display:block;margin-top:4px;font-size:24px}
.filter{display:flex;flex-wrap:wrap;gap:10px;align-items:end}.filter label{display:grid;gap:4px;color:#57606a;font-size:11px;font-weight:700;text-transform:uppercase}.filter input[type=range]{width:260px;height:32px;padding:0;accent-color:#0969da}.filter output{min-width:56px;height:32px;display:inline-flex;align-items:center;justify-content:flex-end;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;color:#24292f}.filter strong{color:#24292f}
details{margin-top:10px}summary{display:inline-flex;align-items:center;gap:6px;margin-bottom:8px;padding:6px 10px;border:1px solid #d0d7de;border-radius:4px;background:#f0f3f6;color:#24292f;font-weight:700;cursor:pointer}
table{width:100%;border-collapse:collapse;background:#fff}th,td{padding:7px;border:1px solid #d0d7de;vertical-align:top}th{background:#f0f3f6;color:#57606a;font-size:11px;text-align:left;text-transform:uppercase}td.num{text-align:right;font-family:ui-monospace,SFMono-Regular,Menlo,monospace}.status{font-weight:700}.status.pass{color:#1a7f37}.status.diff{color:#cf222e}.asset{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;word-break:break-all}
.images{min-width:190px;text-align:center;background:#fbfbfc}.images img{display:block;max-width:260px;max-height:210px;margin:0 auto;border:1px solid #d8dee4;background:#fff}
.pixel-inspector{position:fixed;display:none;z-index:10;padding:10px;border:1px solid #8c959f;background:#fff;box-shadow:0 12px 32px rgba(31,35,40,.18);pointer-events:none}.pixel-inspector.on{display:block}.pixel-inspector .coord{margin:0 0 8px;font:12px ui-monospace,SFMono-Regular,Menlo,monospace;color:#57606a}.pixel-inspector .views{display:grid;grid-template-columns:repeat(3,180px);gap:8px}.pixel-inspector b{display:block;margin:0 0 4px;color:#57606a;font-size:11px;text-transform:uppercase}.pixel-inspector canvas{width:180px;height:180px;border:1px solid #d0d7de;background:#fff;image-rendering:pixelated}.pixel-inspector .rgba{margin-top:4px;font:11px ui-monospace,SFMono-Regular,Menlo,monospace}
.failed{color:#cf222e}.failed-list{margin:8px 0 0;padding-left:20px}.failed-list code{font-family:ui-monospace,SFMono-Regular,Menlo,monospace}
tr[hidden]{display:none}@media(max-width:900px){main{padding:14px}.summary{grid-template-columns:1fr 1fr}.images img{max-width:180px}.comparison{display:block;overflow-x:auto}.pixel-inspector .views{grid-template-columns:1fr}}
@media print{
@page{size:A4 landscape;margin:8mm}
*{box-shadow:none!important}body{background:#fff;color:#111;font-size:9px}main{max-width:none;padding:0}
h1{font-size:18px;margin:0 0 4mm}h2{font-size:13px;margin:0 0 3mm}.muted{color:#444}
.filter,.pixel-inspector{display:none!important}.panel{margin:0 0 5mm;padding:0;border:0;background:#fff;break-inside:auto}.meta{display:grid;grid-template-columns:22mm 1fr;max-width:none;margin:0 0 4mm;padding:3mm;border:1px solid #bbb}.summary{grid-template-columns:repeat(4,1fr);gap:3mm;margin:0 0 5mm}.summary div{padding:3mm;border-color:#bbb}.summary b{font-size:16px}details{display:block}summary{display:none}
.comparison{overflow:visible}table,thead,tbody,tr,td{display:block;width:100%!important}thead{display:none}tr{margin:0 0 5mm;padding:3mm;border:1px solid #bbb;break-inside:avoid;page-break-inside:avoid;background:#fff}tr::after{content:"";display:block;clear:both}td{border:0;padding:0}
td:nth-child(1),td:nth-child(2){display:inline-block;width:auto!important;margin:0 4mm 2mm 0}.asset{font-size:9px}
.images{float:left;width:31.7%!important;min-width:0;margin:0 2.4% 2mm 0;background:#fff;text-align:left}.images:nth-child(5){margin-right:0}.images::before{display:block;margin:0 0 1mm;color:#555;font-size:7px;font-weight:700;text-transform:uppercase;letter-spacing:.04em}.images:nth-child(3)::before{content:"reference"}.images:nth-child(4)::before{content:"test"}.images:nth-child(5)::before{content:"diff"}.images img{width:100%;max-width:100%;max-height:45mm;object-fit:contain;border:1px solid #bbb}
td.num{display:inline-block;width:auto!important;margin:1mm 3mm 0 0;padding-top:1mm;border-top:1px solid #ddd;text-align:left;font-size:8px}.failed-list{font-size:9px}
}
)";
}

template<typename T>
void _writeMeta(std::ofstream& report, const char* label, const T& value)
{
    report << "<dt>" << label << "</dt><dd>" << value << "</dd>";
}

void _writeConfig(std::ofstream& report, const TestResult& result)
{
    report << "<dl class=\"meta panel\">";
    _writeMeta(report, "resource", _html(result.config.resourceTargetDir));
    _writeMeta(report, "reference", _html(result.config.referenceDir));
    _writeMeta(report, "test", _html(result.config.testDir));
    _writeMeta(report, "evaluator", _html(evaluatorName(result.config.evaluatorType)));
    _writeMeta(report, "ci metric", result.metrics.empty() ? "" : _html(result.metrics[result.primaryMetric].label));
    _writeMeta(report, "ci threshold", _threshold(result));
    switch (result.config.evaluatorType) {
        case EvaluatorType::Pixel: {
            _writeMeta(report, "pixel channel threshold", result.config.pixel.channelThreshold);
            _writeMeta(report, "pixel surface diff ratio threshold", result.config.pixel.surfaceDiffRatioThreshold);
            _writeMeta(report, "pixel background ratio", result.config.pixel.backgroundRatio);
            break;
        }
        case EvaluatorType::Flip: {
            _writeMeta(report, "flip surface error floor threshold", result.config.flip.surfaceErrorFloorThreshold);
            break;
        }
    }
    _writeMeta(report, "data", "data.json");
    report << "</dl>";
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

void _writeFilter(std::ofstream& report, const TestResult& result)
{
    const auto metric = result.metrics.empty() ? "" : _html(result.metrics[result.primaryMetric].label);
    report << "<section class=\"panel filter\">";
    report << "<label>Primary threshold<input id=\"primary-threshold\" type=\"range\" min=\"0\" max=\"1\" step=\""
           << _thresholdStep(result.config.evaluatorType) << "\" value=\"" << _threshold(result) << "\"></label>";
    report << "<output id=\"primary-threshold-value\" for=\"primary-threshold\">" << _threshold(result) << "</output>";
    report << "<span class=\"muted\">metric <strong>" << metric << "</strong>, visible <strong id=\"visible-count\">0</strong></span>";
    report << "</section>";
}

void _writeImage(std::ofstream& report, const std::filesystem::path& reportPath, const std::string& path, const char* label)
{
    report << "<td class=\"images\"><img loading=\"lazy\" data-role=\"" << label << "\" alt=\"" << label << "\" src=\"" << _link(reportPath, path) << "\"></td>";
}

void _writeMetricHeader(std::ofstream& report, const TestResult& result)
{
    for (const auto& metric : result.metrics) report << "<th>" << _html(metric.label) << "</th>";
}

void _writeMetricCells(std::ofstream& report, const TestResult& result, const TestResult::Comparison& comparison)
{
    for (size_t i = 0; i < result.metrics.size(); ++i) {
        report << "<td class=\"num\">" << _metricValue(comparison, i) << "</td>";
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
    report << "<section class=\"panel\">";
    report << "<h2>" << _html(backend.name) << "</h2>";
    report << "<p class=\"muted\">compared " << backend.summary.compared
           << ", stored " << backend.comparisons.size()
           << ", different " << backend.summary.different
           << ", failed " << backend.summary.failed << "</p>";

    auto comparisons = backend.comparisons;
    std::sort(comparisons.begin(), comparisons.end(), [&result](const auto& a, const auto& b) {
        return _metricValue(a, result.primaryMetric) > _metricValue(b, result.primaryMetric);
    });

    report << "<details open><summary>Comparisons (" << comparisons.size() << ")</summary>";
    report << "<div class=\"comparison\"><table><thead><tr>";
    report << "<th>status</th><th>asset</th><th>reference</th><th>test</th><th>diff</th>";
    _writeMetricHeader(report, result);
    report << "</tr></thead><tbody>";
    for (const auto& comparison : comparisons) {
        report << "<tr data-primary=\"" << _metricValue(comparison, result.primaryMetric) << "\">";
        report << "<td class=\"status " << (comparison.different ? "diff" : "pass") << "\">" << (comparison.different ? "diff" : "pass") << "</td>";
        report << "<td class=\"asset\">" << _html(comparison.asset) << "</td>";
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

void _writeScript(std::ofstream& report)
{
    report << R"HTML(
<script>
(() => {
  const threshold = document.getElementById('primary-threshold');
  const thresholdValue = document.getElementById('primary-threshold-value');
  const visibleCount = document.getElementById('visible-count');
  const rows = [...document.querySelectorAll('tr[data-primary]')];

  const applyThreshold = () => {
    const value = Number.parseFloat(threshold.value);
    const limit = Number.isFinite(value) ? value : 0;
    thresholdValue.textContent = limit.toFixed(4).replace(/0+$/, '').replace(/\.$/, '');
    let visible = 0;
    rows.forEach((row) => {
      const show = Number.parseFloat(row.dataset.primary) >= limit;
      row.hidden = !show;
      if (show) ++visible;
    });
    visibleCount.textContent = visible;
  };
  threshold.addEventListener('input', applyThreshold);
  applyThreshold();

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
  let locked = null;

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

  const renderInspector = (tr, sourceImg, point, clientX, clientY, isLocked) => {
    if (!tr || tr.hidden || !sourceImg.complete || !sourceImg.naturalWidth) return;

    coord.textContent = `${isLocked ? 'locked ' : ''}x ${point.x}, y ${point.y}`;
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

    renderInspector(tr, img, imagePoint(event, img), event.clientX, event.clientY, false);
  };

  const unlock = () => {
    locked = null;
    inspector.classList.remove('on');
  };

  document.addEventListener('mousemove', (event) => {
    if (locked) return;
    const img = event.target.closest && event.target.closest('img[data-role]');
    if (img) showInspector(event, img);
    else inspector.classList.remove('on');
  });

  document.addEventListener('click', (event) => {
    const img = event.target.closest && event.target.closest('img[data-role]');
    if (!img) {
      if (locked) unlock();
      return;
    }

    const tr = img.closest('tr');
    const point = imagePoint(event, img);
    locked = {tr, img, point, clientX: event.clientX, clientY: event.clientY};
    renderInspector(tr, img, point, event.clientX, event.clientY, true);
  });

  document.addEventListener('keydown', (event) => {
    if (!locked) return;
    const delta = {
      ArrowLeft: [-1, 0],
      ArrowRight: [1, 0],
      ArrowUp: [0, -1],
      ArrowDown: [0, 1]
    }[event.key];
    if (event.key == 'Escape') {
      unlock();
      return;
    }
    if (!delta) return;

    event.preventDefault();
    locked.point.x = clamp(locked.point.x + delta[0], 0, locked.img.naturalWidth - 1);
    locked.point.y = clamp(locked.point.y + delta[1], 0, locked.img.naturalHeight - 1);
    renderInspector(locked.tr, locked.img, locked.point, locked.clientX, locked.clientY, true);
  });

  document.addEventListener('scroll', () => {
    if (!locked) inspector.classList.remove('on');
  }, true);
})();
</script>
)HTML";
}

}  // namespace

bool HtmlSaver::save(const TestResult& result, const std::string& reportDir)
{
    std::error_code error;
    std::filesystem::create_directories(reportDir, error);
    if (error) return false;

    const auto reportPath = std::filesystem::path(reportDir) / "reporter.html";
    std::ofstream report(reportPath);
    if (!report.is_open()) return false;

    report << std::setprecision(6);
    report << "<!doctype html><html><head><meta charset=\"utf-8\">";
    report << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    report << "<title>" << _title(result.config.evaluatorType) << "</title><style>";
    _writeStyle(report);
    report << "</style></head><body><main>";
    report << "<h1>" << _title(result.config.evaluatorType) << "</h1>";
    report << "<p class=\"muted\">Static image comparison report.</p>";

    _writeConfig(report, result);
    _writeSummary(report, result);
    _writeFilter(report, result);
    for (const auto& backend : result.backends) _writeBackend(report, result, backend, reportPath);

    report << "</main>";
    _writeInspector(report);
    _writeScript(report);
    report << "</body></html>";
    return saveReportJson(result, reportDir);
}

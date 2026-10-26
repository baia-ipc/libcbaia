/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "html-page.h"
#include "numutils.h"
#include "strutils.h"
#include "test.h"

/* plots */
#define HTML_PLOT_WIDTH 760
#define HTML_LABEL_WIDTH 190
#define HTML_BAR_HEIGHT 16
#define HTML_BAR_GAP 4
#define HTML_MAX_KNEE_POINTS 400

const char *const html_page_css =
  ":root{--bg:#ffffff;--card:#f6f5f2;--ink:#0b0b0b;--ink2:#52514e;"
  "--grid:#e4e3df;--sample:#2a78d6;--pseudo:#b9b8b2;--accent:#eb6834;"
  "--warn:#fff4e5}\n"
  "@media (prefers-color-scheme: dark){:root{--bg:#1b1b1a;--card:#262624;"
  "--ink:#f2f1ee;--ink2:#b5b3ad;--grid:#3a3936;--sample:#5b9be6;"
  "--pseudo:#6d6b66;--accent:#f08a5d;--warn:#3a2e1e}}\n"
  "body{background:var(--bg);color:var(--ink);font-family:system-ui,"
  "-apple-system,'Segoe UI',Roboto,sans-serif;margin:0;line-height:1.45}\n"
  "main{max-width:1000px;margin:0 auto;padding:16px}\n"
  "h1{font-size:1.6em;margin:0.4em 0 0.1em}"
  "h1.logo{display:inline-block;margin-top:0.2em}"
  "h1.logo img{display:block;width:auto}"
  /* (centered: the spacing after the last letter compensated) */
  "h1.logo span{display:block;margin-top:6px;font-size:0.6em;"
  "font-weight:700;letter-spacing:0.8em;padding-left:0.8em;"
  "text-align:center;text-transform:uppercase;color:var(--ink2)}\n"
  ".head{display:flex;flex-wrap:wrap;align-items:center;gap:0 32px}"
  ".head .brand{text-align:center}.head .updated{font-size:0.8em}"
  ".head .side{flex:1 1 420px;min-width:0}"
  ".head .side>h2:first-child{border-top:none;margin-top:0.4em;"
  "padding-top:0}\n"
  "h2{font-size:1.3em;margin:1.6em 0 0.4em;padding-top:0.6em;"
  "border-top:2px solid var(--grid)}h3{font-size:1.05em;margin:1.2em 0 "
  "0.3em}\n"
  ".sub{color:var(--ink2);margin:0}"
  ".tiles{display:flex;flex-wrap:wrap;gap:10px;margin:0.8em 0}\n"
  ".tile{background:var(--card);border-radius:8px;padding:8px 14px;"
  "min-width:140px}.tile .v{font-size:1.5em;font-weight:600}"
  ".tile .l{color:var(--ink2);font-size:0.85em}\n"
  "table{border-collapse:collapse;font-size:0.9em;margin:0.4em 0}"
  "th,td{padding:3px 10px;text-align:right;border-bottom:1px solid "
  "var(--grid)}th{color:var(--ink2);font-weight:600}"
  "td:first-child,th:first-child{text-align:left}\n"
  "td.seq{font-family:ui-monospace,monospace;text-align:left}\n"
  ".note{color:var(--ink2);font-size:0.85em}"
  ".running{background:var(--warn);border-radius:8px;padding:8px 14px}\n"
  "code{font-family:ui-monospace,monospace;font-size:0.9em;"
  "word-break:break-all}\n"
  "svg{max-width:100%;height:auto;display:block}"
  "svg text{fill:var(--ink);font-size:11px;font-family:inherit}\n"
  "svg .muted{fill:var(--ink2)}svg .grid{stroke:var(--grid)}"
  "svg .halo{paint-order:stroke;stroke:var(--bg);stroke-width:3px;"
  "stroke-linejoin:round}"
  "svg .axis{stroke:var(--ink2)}\n"
  "svg .s{fill:var(--sample)}svg .p{fill:var(--pseudo)}"
  "svg .a{stroke:var(--accent);fill:none}svg .line{stroke:var(--sample);"
  "fill:none;stroke-width:2}\n";

/* the head of the page and the start of the body */
static void write_head(FILE *f, const char *title, long refresh) {
  fputs("<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8"
        "\">\n<meta name=\"viewport\" content=\"width=device-width, "
        "initial-scale=1\">\n", f);
  if (refresh > 0)
    fprintf(f, "<meta http-equiv=\"refresh\" content=\"%ld\">\n", refresh);
  fputs("<title>", f);
  fputs_html_escaped(title, f);
  fprintf(f, "</title>\n<style>\n%s</style>\n</head>\n<body>\n<main>\n",
          html_page_css);
}

/* the time of writing */
static void write_updated(FILE *f) {
  time_t now = time(NULL);
  char date[64];
  strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&now));
  fprintf(f, "<p class=\"sub updated\">Updated: %s</p>\n", date);
}

void html_page_start(FILE *f, const char *title, long refresh) {
  assert(f != NULL);
  assert(title != NULL);
  write_head(f, title, refresh);
  fputs("<h1>", f);
  fputs_html_escaped(title, f);
  fputs("</h1>\n", f);
  write_updated(f);
}

void html_page_start_logo(FILE *f, const char *title,
    const html_logo_t *logo, const char *heading, long refresh) {
  assert(f != NULL);
  assert(title != NULL);
  assert(logo != NULL && logo->src != NULL);
  assert(heading != NULL);
  write_head(f, title, refresh);
  fputs("<div class=\"head\">\n<div class=\"brand\">\n<h1 class=\"logo\">"
        "<picture>", f);
  if (logo->src_dark != NULL) {
    fputs("<source media=\"(prefers-color-scheme: dark)\" srcset=\"", f);
    fputs_html_escaped(logo->src_dark, f);
    fputs("\">", f);
  }
  fputs("<img src=\"", f);
  fputs_html_escaped(logo->src, f);
  fputs("\" alt=\"", f);
  fputs_html_escaped(logo->alt != NULL ? logo->alt : "", f);
  fprintf(f, "\" height=\"%d\"></picture><span>", logo->height);
  fputs_html_escaped(heading, f);
  fputs("</span></h1>\n", f);
  write_updated(f);
  fputs("</div>\n<div class=\"side\">\n", f);
}

void html_page_head_end(FILE *f) {
  assert(f != NULL);
  fputs("</div>\n</div>\n", f);
}

void html_page_end(FILE *f, const char *footer) {
  assert(f != NULL);
  if (footer != NULL) {
    fputs("<p class=\"note\">", f);
    fputs_html_escaped(footer, f);
    fputs("</p>\n", f);
  }
  fputs("</main>\n</body>\n</html>\n", f);
}

void html_tile(FILE *f, const char *value, const char *label) {
  fprintf(f, "<div class=\"tile\"><div class=\"v\">%s</div>"
          "<div class=\"l\">%s</div></div>\n", value, label);
}

/* number with thousands separators (static buffer, 4 in rotation) */
const char *html_num(size_t n) {
  static char bufs[4][32];
  static int which = 0;
  char tmp[32], *out = bufs[which];
  which = (which + 1) % 4;
  int len = snprintf(tmp, sizeof(tmp), "%zu", n), j = 0;
  for (int i = 0; i < len; i++) {
    if (i > 0 && (len - i) % 3 == 0)
      out[j++] = ',';
    out[j++] = tmp[i];
  }
  out[j] = '\0';
  return out;
}

double html_percent(size_t a, size_t b) {
  return b == 0 ? 0.0 : 100.0 * (double)a / (double)b;
}

/* --- plots --- */

/*
   horizontal bar chart: one bar for each value, with its label and the
   value and percentage of <total>; <classes>: "s" or "p" for each bar;
   <mean>: if > 0, a dashed line at this value
*/
void html_hbars(FILE *f, size_t n, const char *const *labels,
    const size_t *values, const char *const *classes, size_t total,
    double mean) {
  size_t max = 1;
  for (size_t i = 0; i < n; i++)
    if (values[i] > max)
      max = values[i];
  int area = HTML_PLOT_WIDTH - HTML_LABEL_WIDTH - 150;
  int height = (int)n * (HTML_BAR_HEIGHT + HTML_BAR_GAP) + 10;
  fprintf(f, "<svg viewBox=\"0 0 %d %d\" width=\"%d\" role=\"img\">\n",
          HTML_PLOT_WIDTH, height, HTML_PLOT_WIDTH);
  if (mean > 0) {
    double x = HTML_LABEL_WIDTH + (double)area * mean / (double)max;
    fprintf(f, "<line class=\"a\" x1=\"%.1f\" y1=\"0\" x2=\"%.1f\" "
            "y2=\"%d\" stroke-dasharray=\"4 3\"><title>mean</title>"
            "</line>\n", x, x, height);
  }
  for (size_t i = 0; i < n; i++) {
    int y = (int)i * (HTML_BAR_HEIGHT + HTML_BAR_GAP) + 4;
    double w = (double)area * (double)values[i] / (double)max;
    fprintf(f, "<text x=\"%d\" y=\"%d\" text-anchor=\"end\">",
            HTML_LABEL_WIDTH - 8, y + HTML_BAR_HEIGHT - 4);
    fputs_html_escaped(labels[i], f);
    fprintf(f, "</text><rect class=\"%s\" x=\"%d\" y=\"%d\" width=\"%.1f\" "
            "height=\"%d\" rx=\"2\"><title>", classes[i], HTML_LABEL_WIDTH, y,
            w > 0 && w < 1 ? 1.0 : w, HTML_BAR_HEIGHT);
    fputs_html_escaped(labels[i], f);
    fprintf(f, ": %s</title></rect><text class=\"muted halo\" x=\"%.1f\" "
            "y=\"%d\">%s (%.1f%%)</text>\n", html_num(values[i]),
            HTML_LABEL_WIDTH + w + 5, y + HTML_BAR_HEIGHT - 4,
            html_num(values[i]),
            html_percent(values[i], total));
  }
  fputs("</svg>\n", f);
}

/* histogram of the insert lengths */
void html_histogram(FILE *f, const size_t *hist, size_t n_bins_all,
    size_t bin_width, const char *what, const char *unit,
    const char *empty) {
  size_t total = 0, max = 0, last = 0;
  for (size_t b = 0; b < n_bins_all; b++) {
    total += hist[b];
    if (hist[b] > max)
      max = hist[b];
    if (hist[b] > 0)
      last = b;
  }
  if (total == 0) {
    fputs("<p class=\"note\">", f);
    fputs_html_escaped(empty, f);
    fputs("</p>\n", f);
    return;
  }
  /* show the bins up to the 99.5th percentile */
  size_t cum = 0, shown = 0;
  for (size_t b = 0; b <= last; b++) {
    cum += hist[b];
    shown = b;
    if ((double)cum >= 0.995 * (double)total)
      break;
  }
  size_t n_bins = shown + 1;
  int left = 60, bottom = 30, top = 10, width = HTML_PLOT_WIDTH - left - 20,
      height = 200;
  double ymax = nice_number((double)max), bw = (double)width / (double)n_bins;
  fprintf(f, "<svg viewBox=\"0 0 %d %d\" width=\"%d\" role=\"img\">\n",
          HTML_PLOT_WIDTH, height + top + bottom, HTML_PLOT_WIDTH);
  for (int g = 0; g <= 4; g++) {
    double y = top + height - height * g / 4.0;
    fprintf(f, "<line class=\"grid\" x1=\"%d\" y1=\"%.1f\" x2=\"%d\" "
            "y2=\"%.1f\"/><text class=\"muted\" x=\"%d\" y=\"%.1f\" "
            "text-anchor=\"end\">%s</text>\n", left, y, left + width, y,
            left - 6, y + 4, html_num((size_t)(ymax * g / 4)));
  }
  for (size_t b = 0; b < n_bins; b++) {
    double h = height * (double)hist[b] / ymax;
    fprintf(f, "<rect class=\"s\" x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" "
            "height=\"%.1f\"><title>%zu-%zu %s: %s</title></rect>\n",
            left + b * bw, top + height - h, bw > 2 ? bw - 1 : bw, h,
            b * bin_width, (b + 1) * bin_width - 1, unit,
            html_num(hist[b]));
  }
  size_t max_len = n_bins * bin_width,
         step = (size_t)nice_number((double)max_len / 8);
  for (size_t x = 0; x <= max_len; x += step) {
    double px = left + (double)x / bin_width * bw;
    fprintf(f, "<line class=\"axis\" x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" "
            "y2=\"%d\"/><text class=\"muted\" x=\"%.1f\" y=\"%d\" "
            "text-anchor=\"middle\">%zu</text>\n", px, top + height, px,
            top + height + 4, px, top + height + 16, x);
  }
  fprintf(f, "<text class=\"muted\" x=\"%d\" y=\"%d\" text-anchor=\"middle\""
          ">", left + width / 2, top + height + 28);
  fputs_html_escaped(what, f);
  fprintf(f, " (%s)%s</text>\n</svg>\n", unit,
          shown < last ? ", longest 0.5% not shown" : "");
}

/* reads of the barcodes by rank (log-log), with the cell threshold */
void html_knee(FILE *f, const uint64_t *counts, size_t n,
    size_t n_cells) {
  if (n == 0 || counts[0] == 0)
    return;
  int left = 60, bottom = 34, top = 10, width = HTML_PLOT_WIDTH - left - 20,
      height = 240;
  double lx = log10((double)n) > 0 ? log10((double)n) : 1,
         ly = log10((double)counts[0]) > 0 ? log10((double)counts[0]) : 1;
  fprintf(f, "<svg viewBox=\"0 0 %d %d\" width=\"%d\" role=\"img\">\n",
          HTML_PLOT_WIDTH, height + top + bottom, HTML_PLOT_WIDTH);
  for (int e = 0; e <= (int)ceil(ly); e++) {
    double y = top + height - height * e / ly;
    if (y < top)
      break;
    fprintf(f, "<line class=\"grid\" x1=\"%d\" y1=\"%.1f\" x2=\"%d\" "
            "y2=\"%.1f\"/><text class=\"muted\" x=\"%d\" y=\"%.1f\" "
            "text-anchor=\"end\">%s</text>\n", left, y, left + width, y,
            left - 6, y + 4, html_num((size_t)pow(10, e)));
  }
  for (int e = 0; e <= (int)ceil(lx); e++) {
    double x = left + width * e / lx;
    if (x > left + width + 1)
      break;
    fprintf(f, "<text class=\"muted\" x=\"%.1f\" y=\"%d\" "
            "text-anchor=\"middle\">%s</text>\n", x, top + height + 16,
            html_num((size_t)pow(10, e)));
  }
  fputs("<polyline class=\"line\" points=\"", f);
  double prev = -1;
  for (size_t i = 0; i < n; i++) {
    double pos = log10((double)(i + 1)) / lx;
    if (i + 1 < n && pos - prev < 1.0 / HTML_MAX_KNEE_POINTS)
      continue;
    prev = pos;
    double y = counts[i] > 0 ? log10((double)counts[i]) / ly : 0;
    fprintf(f, "%.1f,%.1f ", left + width * pos, top + height - height * y);
  }
  fputs("\"/>\n", f);
  if (n_cells > 0) {
    double x = left + width * log10((double)n_cells) / lx;
    fprintf(f, "<line class=\"a\" x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" "
            "y2=\"%d\" stroke-dasharray=\"4 3\"/><text x=\"%.1f\" y=\"%d\">"
            "%s cells</text>\n", x, top, x, top + height, x + 4, top + 12,
            html_num(n_cells));
  }
  fprintf(f, "<text class=\"muted\" x=\"%d\" y=\"%d\" text-anchor=\"middle\""
          ">barcodes, by decreasing number of reads (log scale)</text>\n"
          "<text class=\"muted\" transform=\"translate(14,%d) rotate(-90)\" "
          "text-anchor=\"middle\">reads (log scale)</text>\n</svg>\n",
          left + width / 2, top + height + 30, top + height / 2);
}


/* --- unit tests --- */

void html_page_test(void) {
  test_assert(strcmp(html_num(0), "0") == 0);
  test_assert(strcmp(html_num(999), "999") == 0);
  test_assert(strcmp(html_num(1234567), "1,234,567") == 0);
  test_assert(html_percent(1, 4) == 25.0 && html_percent(1, 0) == 0.0);
  char *out = NULL;
  size_t size = 0;
  FILE *f = open_memstream(&out, &size);
  test_assert(f != NULL);
  html_page_start(f, "Test <report>", 30);
  html_tile(f, "1,234", "reads");
  const char *labels[] = {"S1", "unassigned"};
  const size_t values[] = {30, 10};
  const char *classes[] = {"s", "p"};
  html_hbars(f, 2, labels, values, classes, 40, 20);
  const size_t hist[] = {0, 2, 5, 1};
  html_histogram(f, hist, 4, 10, "insert length", "bp", "No inserts.");
  const size_t none[] = {0, 0};
  html_histogram(f, none, 2, 10, "insert length", "bp", "No inserts.");
  const uint64_t counts[] = {500, 300, 100, 5, 1};
  html_knee(f, counts, 5, 3);
  html_page_end(f, "Generated by test");
  fclose(f);
  const char *exp[] = {"<!doctype html>", "<title>Test &lt;report&gt;</title>",
    "http-equiv=\"refresh\" content=\"30\"", "<h1>Test &lt;report&gt;</h1>",
    "Updated: ", "<div class=\"v\">1,234</div>",
    ">30 (75.0%)</text>", "stroke-dasharray=\"4 3\"><title>mean</title>",
    "<title>20-29 bp: 5</title>", ">insert length (bp)</text>",
    "<p class=\"note\">No inserts.</p>", "3 cells</text>",
    "<p class=\"note\">Generated by test</p>", "</html>\n"};
  for (size_t i = 0; i < sizeof(exp) / sizeof(exp[0]); i++)
    test_assert(strstr(out, exp[i]) != NULL);
  free(out);
  /* with a logo */
  f = open_memstream(&out, &size);
  test_assert(f != NULL);
  html_logo_t logo = {"logo.png", "logo-dark.png", "Test", 80};
  html_page_start_logo(f, "Test report", &logo, "report", 0);
  fputs("<p>side</p>\n", f);
  html_page_head_end(f);
  html_page_end(f, NULL);
  fclose(f);
  test_assert(strstr(out, "<title>Test report</title>") != NULL);
  test_assert(strstr(out, "http-equiv") == NULL);
  test_assert(strstr(out, "<h1 class=\"logo\"><picture><source media=\""
              "(prefers-color-scheme: dark)\" srcset=\"logo-dark.png\">"
              "<img src=\"logo.png\" alt=\"Test\" height=\"80\"></picture>"
              "<span>report</span></h1>") != NULL);
  test_assert(strstr(out, "<div class=\"side\">\n<p>side</p>\n</div>\n"
              "</div>\n") != NULL);
  free(out);
}

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

/* plots: width (the height depends on the plot) */
#define HTML_PLOT_WIDTH 760
/* plots with axes: margins of the plot area (the left one for the labels
   of the y axis) */
#define HTML_PLOT_LEFT 60
#define HTML_PLOT_RIGHT 20
#define HTML_PLOT_TOP 10
/* plots with axes: distance from the x axis of the tick labels */
#define HTML_TICK_LABEL_DY 16
/* bar chart: width of the labels, space right of the bars for the values,
   height of the bars and space between them */
#define HTML_LABEL_WIDTH 190
#define HTML_VALUE_WIDTH 150
#define HTML_BAR_HEIGHT 16
#define HTML_BAR_GAP 4
/* histogram: height of the plot area, space below it (x axis labels and
   title), distance of the title from the x axis */
#define HTML_HIST_HEIGHT 200
#define HTML_HIST_BOTTOM 30
#define HTML_HIST_TITLE_DY 28
/* rank plot: the same; maximal number of points of the line */
#define HTML_KNEE_HEIGHT 240
#define HTML_KNEE_BOTTOM 34
#define HTML_KNEE_TITLE_DY 30
#define HTML_MAX_KNEE_POINTS 400

/* generated from html-page.css by the Makefile */
const char *const html_page_css =
#include "html-page-css.inc"
  ;

/*
   reload of a page rewritten during a run: the script contains the time
   the page was written (<updated>); at each reload, if it changed, the interval
   is halved (down to HTML_PAGE_MIN_RELOAD seconds), otherwise doubled (up
   to <max>); without JavaScript (or session storage), every <max> seconds
*/
static void write_reload(FILE *f, long max, const char *updated) {
  fprintf(f, "<noscript><meta http-equiv=\"refresh\" content=\"%ld\">"
          "</noscript>\n<script>\n(function () {\n"
          "  var max = %ld, min = %d, updated = \"%s\", t = min,\n"
          "      key = \"html_page_reload:\" + location.pathname;\n"
          "  try {\n"
          "    var last = JSON.parse(sessionStorage.getItem(key));\n"
          "    if (last)\n"
          "      t = last.updated !== updated ? Math.max(min, last.t / 2)\n"
          "                                   : Math.min(max, last.t * 2);\n"
          "    sessionStorage.setItem(key, JSON.stringify("
          "{updated: updated, t: t}));\n"
          "  } catch (e) {\n"
          "    t = max;\n"
          "  }\n"
          "  setTimeout(function () { location.reload(); }, t * 1000);\n"
          "})();\n</script>\n", max, max, HTML_PAGE_MIN_RELOAD, updated);
}

/* the head of the page and the start of the body */
static void write_head(FILE *f, const char *title, long refresh,
    const char *updated, const char *body_class) {
  fputs("<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8"
        "\">\n<meta name=\"viewport\" content=\"width=device-width, "
        "initial-scale=1\">\n", f);
  if (refresh > 0)
    write_reload(f, refresh, updated);
  fputs("<title>", f);
  fputs_html_escaped(title, f);
  fprintf(f, "</title>\n<style>\n%s</style>\n</head>\n<body%s%s%s>\n"
          "<main>\n", html_page_css, body_class != NULL ? " class=\"" : "",
          body_class != NULL ? body_class : "", body_class != NULL ? "\"" : "");
}

/* the time of writing, in <date> (at least 32 bytes) */
static void now_date(char *date, size_t size) {
  time_t now = time(NULL);
  strftime(date, size, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

/* the time of writing, as a subtitle */
static void write_updated(FILE *f, const char *date) {
  fprintf(f, "<p class=\"sub updated\">Updated: %s</p>\n", date);
}

void html_page_start(FILE *f, const char *title, long refresh) {
  assert(f != NULL);
  assert(title != NULL);
  char date[64];
  now_date(date, sizeof(date));
  write_head(f, title, refresh, date, NULL);
  fputs("<h1>", f);
  fputs_html_escaped(title, f);
  fputs("</h1>\n", f);
  write_updated(f, date);
}

/* the logo, with its dark variant, if any */
static void write_logo(FILE *f, const html_logo_t *logo) {
  assert(logo->src != NULL);
  fputs("<picture>", f);
  if (logo->src_dark != NULL) {
    fputs("<source media=\"(prefers-color-scheme: dark)\" srcset=\"", f);
    fputs_html_escaped(logo->src_dark, f);
    fputs("\">", f);
  }
  fputs("<img src=\"", f);
  fputs_html_escaped(logo->src, f);
  fputs("\" alt=\"", f);
  fputs_html_escaped(logo->alt != NULL ? logo->alt : "", f);
  fprintf(f, "\" height=\"%d\"></picture>", logo->height);
}

void html_page_start_logo(FILE *f, const char *title,
    const html_logo_t *logo, const char *heading, long refresh) {
  assert(f != NULL);
  assert(title != NULL);
  assert(logo != NULL && logo->src != NULL);
  assert(heading != NULL);
  char date[64];
  now_date(date, sizeof(date));
  write_head(f, title, refresh, date, NULL);
  fputs("<div class=\"head\">\n<div class=\"brand\">\n<h1 class=\"logo\">",
        f);
  write_logo(f, logo);
  fputs("<span>", f);
  fputs_html_escaped(heading, f);
  fputs("</span></h1>\n", f);
  write_updated(f, date);
  fputs("</div>\n<div class=\"side\">\n", f);
}

void html_dashboard_start(FILE *f, const char *title,
    const html_logo_t *logo, const char *heading, long refresh) {
  assert(f != NULL);
  assert(title != NULL);
  assert(heading != NULL);
  char date[64];
  now_date(date, sizeof(date));
  write_head(f, title, refresh, date, "dash");
  fputs("<header class=\"dbar\">\n<div class=\"brand\">", f);
  if (logo != NULL)
    write_logo(f, logo);
  fputs("<span>", f);
  fputs_html_escaped(heading, f);
  fputs("</span>\n", f);
  write_updated(f, date);
  fputs("</div>\n<div class=\"dinfo\">\n", f);
}

void html_dashboard_bar_end(FILE *f) {
  assert(f != NULL);
  fputs("</div>\n</header>\n", f);
}

void html_panel_start(FILE *f, const char *title, const char *cls) {
  assert(f != NULL);
  assert(title != NULL);
  fprintf(f, "<section class=\"panel%s%s\"><h3>", cls != NULL ? " " : "",
          cls != NULL ? cls : "");
  fputs_html_escaped(title, f);
  fputs("</h3>\n", f);
}

void html_panel_end(FILE *f) {
  assert(f != NULL);
  fputs("</section>\n", f);
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
  assert(f != NULL);
  assert(value != NULL);
  assert(label != NULL);
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
  assert(f != NULL);
  assert(n == 0 || (labels != NULL && values != NULL && classes != NULL));
  size_t max = 1;
  for (size_t i = 0; i < n; i++)
    if (values[i] > max)
      max = values[i];
  int area = HTML_PLOT_WIDTH - HTML_LABEL_WIDTH - HTML_VALUE_WIDTH;
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

void html_histogram(FILE *f, const size_t *hist, size_t n_bins_all,
    size_t bin_width, const char *what, const char *unit,
    const char *empty) {
  assert(f != NULL);
  assert(hist != NULL || n_bins_all == 0);
  assert(bin_width > 0);
  assert(what != NULL);
  assert(unit != NULL);
  assert(empty != NULL);
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
  int left = HTML_PLOT_LEFT, bottom = HTML_HIST_BOTTOM, top = HTML_PLOT_TOP,
      width = HTML_PLOT_WIDTH - left - HTML_PLOT_RIGHT,
      height = HTML_HIST_HEIGHT;
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
            "height=\"%.1f\"><title>", left + b * bw, top + height - h,
            bw > 2 ? bw - 1 : bw, h);
    /* (the last bin: all larger values) */
    if (b == n_bins_all - 1)
      fprintf(f, "%zu %s or more", b * bin_width, unit);
    else
      fprintf(f, "%zu-%zu %s", b * bin_width, (b + 1) * bin_width - 1, unit);
    fprintf(f, ": %s</title></rect>\n", html_num(hist[b]));
  }
  size_t max_len = n_bins * bin_width,
         step = (size_t)nice_number((double)max_len / 8);
  for (size_t x = 0; x <= max_len; x += step) {
    double px = left + (double)x / bin_width * bw;
    fprintf(f, "<line class=\"axis\" x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" "
            "y2=\"%d\"/><text class=\"muted\" x=\"%.1f\" y=\"%d\" "
            "text-anchor=\"middle\">%zu</text>\n", px, top + height, px,
            top + height + 4, px, top + height + HTML_TICK_LABEL_DY, x);
  }
  fprintf(f, "<text class=\"muted\" x=\"%d\" y=\"%d\" text-anchor=\"middle\""
          ">", left + width / 2, top + height + HTML_HIST_TITLE_DY);
  fputs_html_escaped(what, f);
  fprintf(f, " (%s)%s</text>\n</svg>\n", unit,
          shown < last ? ", longest 0.5% not shown" : "");
}

void html_knee(FILE *f, const uint64_t *counts, size_t n,
    size_t n_selected, const char *x_label, const char *y_label,
    const char *selected_label) {
  assert(f != NULL);
  assert(counts != NULL || n == 0);
  assert(x_label != NULL);
  assert(y_label != NULL);
  assert(selected_label != NULL);
  if (n == 0 || counts[0] == 0)
    return;
  int left = HTML_PLOT_LEFT, bottom = HTML_KNEE_BOTTOM, top = HTML_PLOT_TOP,
      width = HTML_PLOT_WIDTH - left - HTML_PLOT_RIGHT,
      height = HTML_KNEE_HEIGHT;
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
            "text-anchor=\"middle\">%s</text>\n", x,
            top + height + HTML_TICK_LABEL_DY,
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
  if (n_selected > 0) {
    double x = left + width * log10((double)n_selected) / lx;
    fprintf(f, "<line class=\"a\" x1=\"%.1f\" y1=\"%d\" x2=\"%.1f\" "
            "y2=\"%d\" stroke-dasharray=\"4 3\"/><text x=\"%.1f\" y=\"%d\">"
            "%s ", x, top, x, top + height, x + 4, top + 12,
            html_num(n_selected));
    fputs_html_escaped(selected_label, f);
    fputs("</text>\n", f);
  }
  fprintf(f, "<text class=\"muted\" x=\"%d\" y=\"%d\" text-anchor=\"middle\""
          ">", left + width / 2, top + height + HTML_KNEE_TITLE_DY);
  fputs_html_escaped(x_label, f);
  fprintf(f, " (log scale)</text>\n"
          "<text class=\"muted\" transform=\"translate(14,%d) rotate(-90)\" "
          "text-anchor=\"middle\">", top + height / 2);
  fputs_html_escaped(y_label, f);
  fputs(" (log scale)</text>\n</svg>\n", f);
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
  const char *labels[] = {"a", "other"};
  const size_t values[] = {30, 10};
  const char *classes[] = {"s", "p"};
  html_hbars(f, 2, labels, values, classes, 40, 20);
  const size_t hist[] = {0, 2, 5, 1};
  html_histogram(f, hist, 4, 10, "length", "bp", "No values.");
  const size_t none[] = {0, 0};
  html_histogram(f, none, 2, 10, "length", "bp", "No values.");
  const uint64_t counts[] = {500, 300, 100, 5, 1};
  html_knee(f, counts, 5, 3, "items, by decreasing count", "count",
            "selected");
  html_page_end(f, "Generated by test");
  fclose(f);
  const char *exp[] = {"<!doctype html>", "<title>Test &lt;report&gt;</title>",
    "<noscript><meta http-equiv=\"refresh\" content=\"30\"></noscript>",
    "var max = 30, min = 1, updated = \"", "sessionStorage", "<h1>Test &lt;report&gt;</h1>",
    "Updated: ", "<div class=\"v\">1,234</div>",
    ">30 (75.0%)</text>", "stroke-dasharray=\"4 3\"><title>mean</title>",
    "<title>20-29 bp: 5</title>", "<title>30 bp or more: 1</title>",
    ">length (bp)</text>",
    "<p class=\"note\">No values.</p>", "3 selected</text>",
    ">items, by decreasing count (log scale)</text>",
    ">count (log scale)</text>",
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
  /* dashboard */
  f = open_memstream(&out, &size);
  test_assert(f != NULL);
  html_dashboard_start(f, "Test dashboard", &logo, "dashboard", 30);
  fputs("<p>state</p>\n", f);
  html_dashboard_bar_end(f);
  fputs("<div class=\"dgrid\">\n", f);
  html_panel_start(f, "Values", "tall");
  html_panel_end(f);
  html_panel_start(f, "Steps", NULL);
  html_panel_end(f);
  fputs("</div>\n", f);
  html_page_end(f, NULL);
  fclose(f);
  test_assert(strstr(out, "<body class=\"dash\">") != NULL);
  test_assert(strstr(out, "<span>dashboard</span>") != NULL);
  test_assert(strstr(out, "<div class=\"dinfo\">\n<p>state</p>\n</div>\n"
              "</header>") != NULL);
  test_assert(strstr(out, "<section class=\"panel tall\"><h3>Values</h3>"
              "\n</section>") != NULL);
  test_assert(strstr(out, "<section class=\"panel\"><h3>Steps</h3>") != NULL);
  free(out);
}

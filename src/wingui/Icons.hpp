#pragma once

#include <algorithm>
#include <d2d1.h>

// Small vector glyphs drawn with plain Direct2D primitives so the app needs
// no bitmap/image assets to tell a physical drive apart from a mounted disk
// image in the sidebar.
namespace icons {

// A simple external hard-drive glyph: a rounded case, a divider between the
// "label" area and the "vents" band, a couple of vent lines, and an LED dot.
inline void draw_hard_drive_icon(ID2D1RenderTarget *rt, D2D1_RECT_F box,
  ID2D1SolidColorBrush *stroke, ID2D1SolidColorBrush *fill) {
  const float w = box.right - box.left;
  const float h = box.bottom - box.top;

  rt->DrawRectangle(box, stroke, w * 0.09f);

  const float dividerY = box.top + h * 0.62f;
  rt->DrawLine(D2D1::Point2F(box.left + w * 0.12f, dividerY), D2D1::Point2F(box.right - w * 0.12f, dividerY), stroke, w * 0.07f);

  const float ventY = box.top + h * 0.80f;
  rt->DrawLine(D2D1::Point2F(box.left + w * 0.20f, ventY), D2D1::Point2F(box.left + w * 0.5f, ventY), stroke, w * 0.07f);

  D2D1_ELLIPSE led = D2D1::Ellipse(D2D1::Point2F(box.right - w * 0.24f, ventY), w * 0.06f, w * 0.06f);
  rt->FillEllipse(led, fill);
}

// A simple optical-disc glyph, used for volumes/partitions loaded from a
// mounted .dmg file rather than a physically attached drive.
inline void draw_dmg_icon(ID2D1RenderTarget *rt, D2D1_RECT_F box, ID2D1SolidColorBrush *stroke, ID2D1SolidColorBrush *fill) {
  const float w = box.right - box.left;
  const float h = box.bottom - box.top;
  const D2D1_POINT_2F center = D2D1::Point2F((box.left + box.right) * 0.5f, (box.top + box.bottom) * 0.5f);
  const float outerR = (w < h ? w : h) * 0.5f;

  D2D1_ELLIPSE outer = D2D1::Ellipse(center, outerR, outerR);
  rt->DrawEllipse(outer, stroke, w * 0.07f);

  D2D1_ELLIPSE inner = D2D1::Ellipse(center, outerR * 0.32f, outerR * 0.32f);
  rt->FillEllipse(inner, fill);
  rt->DrawEllipse(inner, stroke, w * 0.05f);

  // A small downward "into disk" notch to hint at a disk *image* rather than
  // a spinning physical disc.
  D2D1_ELLIPSE hub = D2D1::Ellipse(center, outerR * 0.12f, outerR * 0.12f);
  rt->FillEllipse(hub, stroke);
}

// An "X" drawn as two crossing lines, centered in `box`
inline void draw_close_glyph(ID2D1RenderTarget *rt, D2D1_RECT_F box, ID2D1SolidColorBrush *brush) {
  const float w = box.right - box.left;
  const float h = box.bottom - box.top;
  const float inset_x = w * 0.28f;
  const float inset_y = h * 0.28f;
  const float stroke = (w < h ? w : h) * 0.12f;

  rt->DrawLine(D2D1::Point2F(box.left + inset_x, box.top + inset_y), D2D1::Point2F(box.right - inset_x, box.bottom - inset_y), brush, stroke);
  rt->DrawLine(D2D1::Point2F(box.right - inset_x, box.top + inset_y), D2D1::Point2F(box.left + inset_x, box.bottom - inset_y), brush, stroke);
}

// A small filled triangle -- pointing right when collapsed, down when
// expanded -- used as the sidebar's disk expand/collapse indicator
inline void draw_chevron(ID2D1RenderTarget *rt, ID2D1Factory *factory, D2D1_RECT_F box, bool expanded, ID2D1SolidColorBrush *brush) {
  const float cx = (box.left + box.right) * 0.5f;
  const float cy = (box.top + box.bottom) * 0.5f;
  const float size = (std::min)(box.right - box.left, box.bottom - box.top) * 0.5f;

  ID2D1PathGeometry *geometry = nullptr;
  if (FAILED(factory->CreatePathGeometry(&geometry))) {
    return;
  }
  ID2D1GeometrySink *sink = nullptr;
  if (FAILED(geometry->Open(&sink))) {
    geometry->Release();
    return;
  }

  if (expanded) {
    sink->BeginFigure(D2D1::Point2F(cx - size, cy - size * 0.5f), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(D2D1::Point2F(cx + size, cy - size * 0.5f));
    sink->AddLine(D2D1::Point2F(cx, cy + size * 0.5f));
  } else {
    sink->BeginFigure(D2D1::Point2F(cx - size * 0.5f, cy - size), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(D2D1::Point2F(cx - size * 0.5f, cy + size));
    sink->AddLine(D2D1::Point2F(cx + size * 0.5f, cy));
  }
  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  sink->Close();
  sink->Release();

  rt->FillGeometry(geometry, brush);
  geometry->Release();
}

} // namespace icons

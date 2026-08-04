#include "AppWindow.hpp"
#include "Format.hpp"
#include "Icons.hpp"
#include "Theme.hpp"
#include "Utf8.hpp"
#include "callbacks.hpp"

#include <algorithm>
#include <commdlg.h>

using utf8::to_wstring;

// Initialises the app window
AppWindow::AppWindow(HWND hwnd) : hwnd_(hwnd) {
  dpi_scale_ = float(GetDpiForWindow(hwnd_)) / 96.0f;

  D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d_factory_.GetAddressOf());
  DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
    (IUnknown **)dwrite_factory_.GetAddressOf()
  );

  create_text_formats();
  disks_ = callbacks::list_disks();
}

// Sets up text rendering
void AppWindow::create_text_formats() {
  auto make = [&](ComPtr<IDWriteTextFormat> &fmt, float size, DWRITE_FONT_WEIGHT weight) {
    dwrite_factory_->CreateTextFormat(
      L"Segoe UI", nullptr, weight,
      DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
      size, L"en-us", &fmt
    );
    fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
  };

  make(font_row_name_, 14.0f, DWRITE_FONT_WEIGHT_MEDIUM);
  make(font_row_subtitle_, 11.5f, DWRITE_FONT_WEIGHT_REGULAR);

  make(font_partition_row_, 13.0f, DWRITE_FONT_WEIGHT_REGULAR);
  font_partition_row_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  make(font_disk_name_, 26.0f, DWRITE_FONT_WEIGHT_BOLD);
  make(font_partition_name_, 14.0f, DWRITE_FONT_WEIGHT_REGULAR);

  make(font_capacity_, 20.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD);
  font_capacity_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
  font_capacity_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  make(font_section_label_, 11.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD);

  make(font_volume_row_, 13.0f, DWRITE_FONT_WEIGHT_REGULAR);
  font_volume_row_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  make(font_volume_name_, 20.0f, DWRITE_FONT_WEIGHT_BOLD);
  make(font_volume_detail_, 13.0f, DWRITE_FONT_WEIGHT_REGULAR);

  make(font_button_, 14.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD);
  font_button_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  font_button_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  make(font_placeholder_, 14.0f, DWRITE_FONT_WEIGHT_REGULAR);
  font_placeholder_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  font_placeholder_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  make(font_glyph_, 22.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD);
  font_glyph_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  font_glyph_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  make(font_empty_state_title_, 32.0f, DWRITE_FONT_WEIGHT_BOLD);
  font_empty_state_title_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  font_empty_state_title_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

// Sets up brushes/etc
void AppWindow::create_device_resources() {
  if (render_target_) {
    return;
  }

  RECT rc;
  GetClientRect(hwnd_, &rc);
  D2D1_SIZE_U size = D2D1::SizeU(UINT32(rc.right - rc.left), UINT32(rc.bottom - rc.top));

  // Fixed at 96 DPI deliberately (rather than the default, which would pick
  // up the current monitor's actual DPI and have D2D itself silently
  // rescale every DIP): we do our own scaling via SetTransform in on_paint
  // using dpi_scale_, so mouse-coordinate conversion in the on_mouse_*
  // handlers (which divides by that same dpi_scale_) has one single,
  // explicit scale factor to reason about instead of an implicit one baked
  // into the render target.
  HRESULT hr = d2d_factory_->CreateHwndRenderTarget(
    D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.0f, 96.0f),
    D2D1::HwndRenderTargetProperties(hwnd_, size),
    &render_target_
  );
  if (FAILED(hr)) {
    return;
  }

  render_target_->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &brush_);
  render_target_->CreateSolidColorBrush(theme::icon_stroke, &icon_stroke_brush_);
  render_target_->CreateSolidColorBrush(theme::icon_fill, &icon_fill_brush_);
}

void AppWindow::discard_device_resources() {
  brush_.Reset();
  icon_stroke_brush_.Reset();
  icon_fill_brush_.Reset();
  render_target_.Reset();
}

void AppWindow::draw_text(const D2D1_RECT_F &rect, IDWriteTextFormat *format, const D2D1_COLOR_F &color, const std::wstring &text) {
  brush_->SetColor(color);
  render_target_->DrawText(
    text.c_str(), UINT32(text.size()), format,
    rect, brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP
  );
}

void AppWindow::on_paint() {
  create_device_resources();
  if (!render_target_) {
    return;
  }

  render_target_->BeginDraw();
  // All layout/drawing below is done in 96-DPI logical units; scale up to
  // physical pixels here so it renders at the right physical size on a
  // monitor above 100% scaling (the on_mouse_* handlers divide incoming
  // physical mouse coordinates by the same factor, so hit-testing stays in
  // that same logical-unit space as the layout it's testing against).
  render_target_->SetTransform(D2D1::Matrix3x2F::Scale(dpi_scale_, dpi_scale_));
  render_target_->Clear(theme::window_bg);

  HitRegions hits;
  draw_sidebar(hits);
  draw_content(hits);

  HRESULT hr = render_target_->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    discard_device_resources();
  } else {
    hits_ = std::move(hits);
  }
}

void AppWindow::on_resize(UINT width, UINT height) {
  if (!render_target_) {
    return;
  }
  render_target_->Resize(D2D1::SizeU(width, height));
}

void AppWindow::draw_sidebar(HitRegions &hits) {
  // GetSize() is in physical pixels (the render target is fixed at 96 DPI,
  // see create_device_resources) -- divide by dpi_scale_ to get the logical
  // (96-DPI-equivalent) canvas size everything below is laid out in; the
  // render-time transform in on_paint scales it back up to fill the actual
  // physical window.
  D2D1_SIZE_F physical_size = render_target_->GetSize();
  D2D1_SIZE_F size = D2D1::SizeF(physical_size.width / dpi_scale_, physical_size.height / dpi_scale_);
  D2D1_RECT_F sidebar_rect = D2D1::RectF(0, 0, theme::sidebar_width, size.height);
  sidebar_viewport_ = sidebar_rect;

  brush_->SetColor(theme::sidebar_bg);
  render_target_->FillRectangle(sidebar_rect, brush_.Get());

  brush_->SetColor(theme::sidebar_border);
  render_target_->DrawLine(
    D2D1::Point2F(theme::sidebar_width, 0), D2D1::Point2F(theme::sidebar_width, size.height),
    brush_.Get(), 1.0f
  );

  // Total content height: every disk row, plus (if expanded) its partition
  // rows, plus the trailing "add a disk image" row.
  float content_height = theme::sidebar_row_height; // add row
  for (auto &disk : disks_) {
    content_height += theme::sidebar_row_height;
    if (disk.expanded) {
      content_height += float(disk.partitions.size()) * theme::partition_row_height;
    }
  }
  const float max_scroll = std::max(0.0f, content_height - size.height);
  sidebar_scroll_ = std::clamp(sidebar_scroll_, 0.0f, max_scroll);

  hits.disk_rows.assign(disks_.size(), D2D1_RECT_F{});
  hits.disk_close_buttons.assign(disks_.size(), D2D1_RECT_F{});
  hits.partition_rows.clear();
  hits.partition_row_targets.clear();

  float cursor_y = -sidebar_scroll_;

  for (size_t d = 0; d < disks_.size(); ++d) {
    auto &disk = disks_[d];

    D2D1_RECT_F row = D2D1::RectF(0, cursor_y, theme::sidebar_width, cursor_y + theme::sidebar_row_height);
    hits.disk_rows[d] = row;

    const bool disk_hovered = hover_kind_ == HoverKind::DiskRow && hover_index_ == int(d);
    if (disk_hovered) {
      brush_->SetColor(theme::row_hover_bg);
      render_target_->FillRectangle(row, brush_.Get());
    }

    D2D1_RECT_F chevron_rect = D2D1::RectF(
      theme::sidebar_row_padding_x - 6.0f, row.top + (theme::sidebar_row_height - 14.0f) * 0.5f,
      theme::sidebar_row_padding_x - 6.0f + theme::sidebar_chevron_width, row.top + (theme::sidebar_row_height + 14.0f) * 0.5f
    );

    const float icon_top = row.top + (theme::sidebar_row_height - theme::sidebar_row_icon_size) * 0.5f;
    D2D1_RECT_F icon_box = D2D1::RectF(
      chevron_rect.right + 6.0f, icon_top,
      chevron_rect.right + 6.0f + theme::sidebar_row_icon_size,
      icon_top + theme::sidebar_row_icon_size
    );

    if (!disk.expanded) {
      // A permanent (non-hover) tint behind the chevron so a collapsed disk
      // is recognizable without needing to hover over it first.
      D2D1_RECT_F gutter = D2D1::RectF(0, row.top, icon_box.left, row.bottom);
      brush_->SetColor(theme::hierarchy_indicator_bg);
      render_target_->FillRectangle(gutter, brush_.Get());
    }

    brush_->SetColor(theme::text_secondary);
    icons::draw_chevron(render_target_.Get(), d2d_factory_.Get(), chevron_rect, disk.expanded, brush_.Get());

    if (disk.kind == model::DiskKind::Physical) {
      icons::draw_hard_drive_icon(render_target_.Get(), icon_box, icon_stroke_brush_.Get(), icon_fill_brush_.Get());
    } else {
      icons::draw_dmg_icon(render_target_.Get(), icon_box, icon_stroke_brush_.Get(), icon_fill_brush_.Get());
    }

    const float text_left = icon_box.right + 12.0f;
    const float text_right = row.right - theme::sidebar_row_padding_x - (disk.removable ? (theme::sidebar_close_btn_size + 10.0f) : 0.0f);

    D2D1_RECT_F name_rect = D2D1::RectF(text_left, row.top + 12.0f, text_right, row.top + 32.0f);
    draw_text(name_rect, font_row_name_.Get(), theme::text_primary, to_wstring(disk.name));

    const size_t partition_count = disk.partitions.size();
    std::string subtitle = std::to_string(partition_count) + (partition_count == 1 ? " partition" : " partitions");
    D2D1_RECT_F sub_rect = D2D1::RectF(text_left, row.top + 32.0f, text_right, row.top + 50.0f);
    draw_text(sub_rect, font_row_subtitle_.Get(), theme::text_secondary, to_wstring(subtitle));

    if (disk.removable) {
      constexpr float close_margin = 8.0f;
      D2D1_RECT_F close_rect = D2D1::RectF(
        row.right - theme::sidebar_row_padding_x - theme::sidebar_close_btn_size,
        row.top + close_margin,
        row.right - theme::sidebar_row_padding_x,
        row.top + close_margin + theme::sidebar_close_btn_size
      );
      hits.disk_close_buttons[d] = close_rect;

      const bool close_hovered = hover_kind_ == HoverKind::DiskClose && hover_index_ == int(d);
      brush_->SetColor(close_hovered ? theme::danger : theme::text_primary);
      icons::draw_close_glyph(render_target_.Get(), close_rect, brush_.Get());
    }

    cursor_y += theme::sidebar_row_height;

    if (disk.expanded) {
      for (size_t p = 0; p < disk.partitions.size(); ++p) {
        auto &part = disk.partitions[p];

        D2D1_RECT_F prow = D2D1::RectF(0, cursor_y, theme::sidebar_width, cursor_y + theme::partition_row_height);
        const int flat_index = int(hits.partition_rows.size());
        hits.partition_rows.push_back(prow);
        hits.partition_row_targets.push_back({int(d), int(p)});

        // A permanent (not hover/selection-dependent) tint along the left
        // edge of every partition row, so it reads as nested under its disk
        // rather than a plain top-level entry even when nothing is hovered
        // or selected.
        D2D1_RECT_F pgutter = D2D1::RectF(0, prow.top, theme::partition_row_indent - 8.0f, prow.bottom);
        brush_->SetColor(theme::hierarchy_indicator_bg);
        render_target_->FillRectangle(pgutter, brush_.Get());

        // Inset so the highlight starts under the parent disk's icon rather
        // than spanning the full sidebar width -- otherwise a selected
        // partition reads as a top-level item instead of a nested one.
        D2D1_RECT_F phighlight = D2D1::RectF(theme::partition_row_indent - 8.0f, prow.top, prow.right, prow.bottom);

        const bool selected = int(d) == selected_disk_ && int(p) == selected_partition_;
        const bool phovered = hover_kind_ == HoverKind::PartitionRow && hover_index_ == flat_index;
        if (selected) {
          brush_->SetColor(theme::row_selected_bg);
          render_target_->FillRectangle(phighlight, brush_.Get());
        } else if (phovered) {
          brush_->SetColor(theme::row_hover_bg);
          render_target_->FillRectangle(phighlight, brush_.Get());
        }

        D2D1_RECT_F ptext_rect = D2D1::RectF(
          theme::partition_row_indent, prow.top,
          prow.right - theme::sidebar_row_padding_x, prow.bottom
        );
        draw_text(
          ptext_rect, font_partition_row_.Get(), theme::text_primary,
          to_wstring(part.name.empty() ? "Untitled Partition" : part.name)
        );

        cursor_y += theme::partition_row_height;
      }
    }
  }

  {
    // The "add a disk image" row: flows right after the last drive/partition
    // tree, and scrolls with the rest of the list.
    D2D1_RECT_F row = D2D1::RectF(0, cursor_y, theme::sidebar_width, cursor_y + theme::sidebar_row_height);
    hits.add_button = row;

    const bool add_hovered = hover_kind_ == HoverKind::AddButton;
    brush_->SetColor(add_hovered ? theme::add_row_bg_hover : theme::add_row_bg);
    render_target_->FillRectangle(row, brush_.Get());

    draw_text(row, font_glyph_.Get(), theme::text_secondary, L"+");
  }

  draw_scrollbar(sidebar_rect, content_height, sidebar_scroll_);
}

void AppWindow::draw_content(HitRegions &hits) {
  // See draw_sidebar -- GetSize() is physical pixels, convert to the same
  // logical-unit space the rest of the layout is in.
  D2D1_SIZE_F physical_size = render_target_->GetSize();
  D2D1_SIZE_F size = D2D1::SizeF(physical_size.width / dpi_scale_, physical_size.height / dpi_scale_);
  D2D1_RECT_F content_rect = D2D1::RectF(theme::sidebar_width, 0, size.width, size.height);

  if (disks_.empty()) {
    const float center_y = (content_rect.top + content_rect.bottom) * 0.5f;
    D2D1_RECT_F title_rect = D2D1::RectF(
      content_rect.left + 40.0f, center_y - 42.0f, content_rect.right - 40.0f, center_y
    );
    draw_text(title_rect, font_empty_state_title_.Get(), theme::text_primary, L"libapfs-gui");

    D2D1_RECT_F subtitle_rect = D2D1::RectF(
      content_rect.left + 40.0f, center_y, content_rect.right - 40.0f, center_y + 34.0f
    );
    draw_text(subtitle_rect, font_placeholder_.Get(), theme::text_placeholder, L"No APFS drives could be detected on your computer right now.");
    return;
  }

  if (selected_disk_ < 0 || selected_disk_ >= int(disks_.size())) {
    selected_disk_ = 0;
  }
  auto &disk = disks_[selected_disk_];

  if (disk.partitions.empty()) {
    draw_text(content_rect, font_placeholder_.Get(), theme::text_placeholder, L"This disk has no partitions");
    return;
  }

  if (selected_partition_ < 0 || selected_partition_ >= int(disk.partitions.size())) {
    selected_partition_ = 0;
  }

  constexpr float info_height = 92.0f;
  D2D1_RECT_F info_area = D2D1::RectF(
    content_rect.left,
    content_rect.top,
    content_rect.right,
    content_rect.top + info_height
  );
  draw_partition_info(info_area);

  brush_->SetColor(theme::divider);
  render_target_->DrawLine(
    D2D1::Point2F(content_rect.left, info_area.bottom),
    D2D1::Point2F(content_rect.right, info_area.bottom),
    brush_.Get(), 1.0f
  );

  D2D1_RECT_F volume_area = D2D1::RectF(
    content_rect.left, info_area.bottom + 1.0f,
    content_rect.right, content_rect.bottom
  );
  draw_volume_section(volume_area, disk.partitions[selected_partition_], hits);
}

void AppWindow::draw_partition_info(const D2D1_RECT_F &area) {
  auto &disk = disks_[selected_disk_];
  auto &part = disk.partitions[selected_partition_];

  D2D1_RECT_F name_row = D2D1::RectF(
    area.left + theme::content_padding,
    area.top + theme::content_padding - 6.0f,
    area.right - theme::content_padding - 140.0f,
    area.top + theme::content_padding + 30.0f
  );
  draw_text(name_row, font_disk_name_.Get(), theme::text_primary, to_wstring(disk.name));

  D2D1_RECT_F capacity_rect = D2D1::RectF(
    area.right - theme::content_padding - 140.0f,
    area.top + theme::content_padding - 6.0f,
    area.right - theme::content_padding,
    area.top + theme::content_padding + 30.0f
  );
  draw_text(capacity_rect, font_capacity_.Get(), theme::text_secondary, to_wstring(format::bytes(part.capacity_bytes)));

  D2D1_RECT_F subtitle_rect = D2D1::RectF(
    area.left + theme::content_padding,
    area.top + theme::content_padding + 32.0f,
    area.right - theme::content_padding,
    area.top + theme::content_padding + 54.0f
  );
  draw_text(subtitle_rect, font_partition_name_.Get(), theme::text_secondary, to_wstring(part.name.empty() ? "Untitled Partition" : part.name));
}

void AppWindow::draw_volume_section(const D2D1_RECT_F &area, model::Partition &part, HitRegions &hits) {
  if (selected_volume_ >= int(part.volumes.size())) {
    selected_volume_ = -1;
  }

  const float list_left = area.left + theme::content_padding;
  const float list_right = list_left + theme::volume_list_width;

  D2D1_RECT_F label_rect = D2D1::RectF(list_left, area.top + 16.0f, list_right, area.top + 34.0f);
  draw_text(label_rect, font_section_label_.Get(), theme::text_secondary, L"VOLUMES");

  const float list_top = area.top + 40.0f;
  D2D1_RECT_F list_viewport = D2D1::RectF(list_left, list_top, list_right, area.bottom);
  volume_list_viewport_ = list_viewport;

  const float content_height = float(part.volumes.size()) * theme::volume_row_height;
  const float viewport_height = list_viewport.bottom - list_viewport.top;
  const float max_scroll = std::max(0.0f, content_height - viewport_height);
  volume_scroll_ = std::clamp(volume_scroll_, 0.0f, max_scroll);

  hits.volume_rows.assign(part.volumes.size(), D2D1_RECT_F{});

  render_target_->PushAxisAlignedClip(list_viewport, D2D1_ANTIALIAS_MODE_ALIASED);

  for (size_t i = 0; i < part.volumes.size(); ++i) {
    const float row_top = list_top + float(i) * theme::volume_row_height - volume_scroll_;
    D2D1_RECT_F row = D2D1::RectF(list_left, row_top, list_right, row_top + theme::volume_row_height);
    hits.volume_rows[i] = row;

    const bool selected = int(i) == selected_volume_;
    const bool hovered = hover_kind_ == HoverKind::VolumeRow && hover_index_ == int(i);
    if (selected) {
      brush_->SetColor(theme::row_selected_bg);
      render_target_->FillRectangle(row, brush_.Get());
    } else if (hovered) {
      brush_->SetColor(theme::row_hover_bg);
      render_target_->FillRectangle(row, brush_.Get());
    }

    D2D1_ELLIPSE dot = D2D1::Ellipse(D2D1::Point2F(row.left + 10.0f, (row.top + row.bottom) * 0.5f), 4.0f, 4.0f);
    brush_->SetColor(part.volumes[i].mounted ? theme::progress_fill : theme::text_placeholder);
    render_target_->FillEllipse(dot, brush_.Get());

    D2D1_RECT_F text_rect = D2D1::RectF(row.left + 22.0f, row.top, row.right - 8.0f, row.bottom);
    draw_text(text_rect, font_volume_row_.Get(), theme::text_primary, to_wstring(part.volumes[i].name));
  }

  render_target_->PopAxisAlignedClip();

  draw_scrollbar(list_viewport, content_height, volume_scroll_);

  const float divider_x = list_right + theme::content_padding * 0.5f;
  brush_->SetColor(theme::divider);
  render_target_->DrawLine(
    D2D1::Point2F(divider_x, area.top + 8.0f),
    D2D1::Point2F(divider_x, area.bottom - 8.0f),
    brush_.Get(), 1.0f
  );

  D2D1_RECT_F detail_area = D2D1::RectF(
    list_right + theme::content_padding,
    area.top, area.right - theme::content_padding,
    area.bottom - theme::content_padding
  );

  if (selected_volume_ < 0) {
    draw_text(detail_area, font_placeholder_.Get(), theme::text_placeholder, L"Select a volume to view details");
    hits.mount_button_valid = false;
    return;
  }

  auto &vol = part.volumes[selected_volume_];

  D2D1_RECT_F vol_name_rect = D2D1::RectF(
    detail_area.left,
    detail_area.top + 12.0f,
    detail_area.right,
    detail_area.top + 40.0f
  );
  draw_text(vol_name_rect, font_volume_name_.Get(), theme::text_primary, to_wstring(vol.name));

  D2D1_RECT_F vol_detail_rect = D2D1::RectF(
    detail_area.left,
    detail_area.top + 46.0f,
    detail_area.right,
    detail_area.top + 66.0f
  );
  std::string detail_text = format::bytes(vol.used_bytes) + " used of " + format::bytes(vol.capacity_bytes);
  draw_text(vol_detail_rect, font_volume_detail_.Get(), theme::text_secondary, to_wstring(detail_text));

  const float bar_width = std::min(detail_area.right - detail_area.left, 360.0f);
  D2D1_RECT_F bar_track = D2D1::RectF(
    detail_area.left,
    detail_area.top + 76.0f,
    detail_area.left + bar_width,
    detail_area.top + 84.0f
  );
  brush_->SetColor(theme::progress_track);
  render_target_->FillRectangle(bar_track, brush_.Get());

  double fraction = vol.capacity_bytes > 0 ? double(vol.used_bytes) / double(vol.capacity_bytes) : 0.0;
  fraction = std::min(1.0, std::max(0.0, fraction));
  D2D1_RECT_F bar_fill = D2D1::RectF(bar_track.left, bar_track.top, bar_track.left + float(bar_width * fraction), bar_track.bottom);
  if (bar_fill.right > bar_fill.left) {
    brush_->SetColor(theme::progress_fill);
    render_target_->FillRectangle(bar_fill, brush_.Get());
  }

  if (vol.mounted && !vol.mount_point.empty()) {
    D2D1_RECT_F mount_info_rect = D2D1::RectF(detail_area.left, detail_area.top + 96.0f, detail_area.right, detail_area.top + 116.0f);
    draw_text(mount_info_rect, font_volume_detail_.Get(), theme::text_secondary, L"(Mounted at " + vol.mount_point + L")");
  }

  D2D1_RECT_F mount_button = D2D1::RectF(
    detail_area.right - theme::mount_button_width,
    detail_area.bottom - theme::mount_button_height,
    detail_area.right, detail_area.bottom
  );
  hits.mount_button = mount_button;
  hits.mount_button_valid = true;

  const bool mount_hovered = hover_kind_ == HoverKind::MountButton;
  const D2D1_COLOR_F btn_color = vol.mounted ? theme::danger : theme::accent;
  const D2D1_COLOR_F btn_color_hover = vol.mounted ? theme::danger_hover : theme::accent_hover;
  brush_->SetColor(mount_hovered ? btn_color_hover : btn_color);
  render_target_->FillRectangle(mount_button, brush_.Get());
  draw_text(mount_button, font_button_.Get(), theme::text_on_accent, vol.mounted ? L"Unmount" : L"Mount");
}

void AppWindow::draw_scrollbar(const D2D1_RECT_F &viewport, float content_height, float scroll_offset) {
  const float viewport_height = viewport.bottom - viewport.top;
  if (content_height <= viewport_height) {
    return;
  }

  D2D1_RECT_F track = D2D1::RectF(viewport.right - theme::scrollbar_width - theme::scrollbar_margin, viewport.top, viewport.right - theme::scrollbar_margin, viewport.bottom);
  brush_->SetColor(theme::divider);
  render_target_->FillRectangle(track, brush_.Get());

  const float max_scroll = content_height - viewport_height;
  const float thumb_height = std::max(24.0f, viewport_height * (viewport_height / content_height));
  const float thumb_top = viewport.top + (viewport_height - thumb_height) * (scroll_offset / max_scroll);
  D2D1_RECT_F thumb = D2D1::RectF(track.left, thumb_top, track.right, thumb_top + thumb_height);
  brush_->SetColor(theme::text_placeholder);
  render_target_->FillRectangle(thumb, brush_.Get());
}

bool AppWindow::hit_test(const D2D1_RECT_F &r, float x, float y) {
  return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

void AppWindow::select_partition(int disk_index, int partition_index) {
  if (disk_index < 0 || disk_index >= int(disks_.size())) {
    return;
  }
  auto &disk = disks_[disk_index];
  if (partition_index < 0 || partition_index >= int(disk.partitions.size())) {
    return;
  }
  selected_disk_ = disk_index;
  selected_partition_ = partition_index;
  selected_volume_ = -1;
}

void AppWindow::select_volume(int index) { selected_volume_ = index; }

void AppWindow::toggle_disk_expanded(int index) {
  if (index < 0 || index >= int(disks_.size())) {
    return;
  }
  disks_[index].expanded = !disks_[index].expanded;
}

void AppWindow::remove_disk(int index) {
  if (index < 0 || index >= int(disks_.size())) {
    return;
  }
  auto &disk = disks_[index];
  if (!disk.removable) {
    return;
  }

  int mounted_count = 0;
  for (auto &partition : disk.partitions) {
    for (auto &volume : partition.volumes) {
      if (volume.mounted) {
        ++mounted_count;
      }
    }
  }

  if (mounted_count > 0) {
    const std::wstring message = L"\"" + to_wstring(disk.name) + L"\" has " + std::to_wstring(mounted_count) + L" mounted volume" +
                                 (mounted_count == 1 ? L"" : L"s") + L". Unmount " +
                                 (mounted_count == 1 ? L"it" : L"them") + L" and remove this disk?";
    if (MessageBoxW(hwnd_, message.c_str(), L"Unmount before removing?", MB_YESNO | MB_ICONWARNING) != IDYES) {
      return;
    }

    for (auto &partition : disk.partitions) {
      for (auto &volume : partition.volumes) {
        if (volume.mounted) {
          callbacks::unmount_volume(volume);
        }
      }
    }
  }

  disks_.erase(disks_.begin() + index);
  selected_volume_ = -1;

  if (disks_.empty()) {
    selected_disk_ = 0;
    selected_partition_ = 0;
    return;
  }

  if (selected_disk_ == index) {
    selected_disk_ = 0;
    selected_partition_ = 0;
  } else if (selected_disk_ > index) {
    --selected_disk_;
  }
}

void AppWindow::toggle_mount() {
  if (selected_disk_ < 0 || selected_disk_ >= int(disks_.size())) {
    return;
  }
  auto &disk = disks_[selected_disk_];
  if (selected_partition_ < 0 || selected_partition_ >= int(disk.partitions.size())) {
    return;
  }
  auto &part = disk.partitions[selected_partition_];
  if (selected_volume_ < 0 || selected_volume_ >= int(part.volumes.size())) {
    return;
  }

  auto &vol = part.volumes[selected_volume_];
  if (vol.mounted) {
    callbacks::unmount_volume(vol);
  } else {
    callbacks::mount_volume(disk, part.disk_partition_index, selected_volume_, vol);
  }
}

void AppWindow::add_disk_from_dmg() {
  wchar_t file[MAX_PATH] = L"";

  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd_;
  ofn.lpstrFilter = L"Disk Images (*.dmg)\0*.dmg\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = L"Choose a disk image";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

  if (!GetOpenFileNameW(&ofn)) {
    return; // user cancelled
  }

  disks_.push_back(callbacks::load_dmg_disk(file));
  const int new_index = int(disks_.size()) - 1;
  disks_[new_index].expanded = true;

  if (!disks_[new_index].partitions.empty()) {
    select_partition(new_index, 0);
  } else {
    selected_disk_ = new_index;
  }
}

void AppWindow::refresh_physical_disks() {
  auto fresh_physical = callbacks::list_disks();

  // list_disks() also rediscovers .dmg disks still served by a mount helper
  // left over from a previous GUI session -- useful for the very first scan
  // (the constructor's plain assignment, where disks_ starts empty), but
  // redundant here: disks_ already carries every .dmg disk it knows about
  // forward below, unconditionally. Keeping list_disks()'s copies too would
  // duplicate any .dmg whose mount helper is now running (e.g. because a
  // volume was just mounted, which is what triggers this rescan in the
  // first place) -- drop everything but the actual physical drives.
  fresh_physical.erase(
    std::remove_if(
      fresh_physical.begin(), fresh_physical.end(),
      [](const model::Disk &d) { return d.kind != model::DiskKind::Physical; }
    ),
    fresh_physical.end()
  );

  // Captured so the selection can be restored below -- not just which disk,
  // but which partition/volume on it, since this whole rescan is triggered
  // by WM_DEVICECHANGE, which mounting/unmounting a volume itself fires (a
  // drive letter appearing/disappearing is a device change). Without this,
  // mounting a volume would immediately un-select it.
  std::string selected_source_path;
  int selected_disk_partition_index = -1;
  int selected_volume_index = -1;
  const bool had_selection = selected_disk_ >= 0 && selected_disk_ < int(disks_.size());
  if (had_selection) {
    auto &sel_disk = disks_[selected_disk_];
    selected_source_path = sel_disk.source_path;
    if (selected_partition_ >= 0 && selected_partition_ < int(sel_disk.partitions.size())) {
      selected_disk_partition_index = sel_disk.partitions[selected_partition_].disk_partition_index;
      selected_volume_index = selected_volume_;
    }
  }

  // Carry over expand state for physical disks that are still connected,
  // rather than always snapping back to expanded.
  for (auto &fresh : fresh_physical) {
    for (auto &existing : disks_) {
      if (existing.kind == model::DiskKind::Physical && existing.source_path == fresh.source_path) {
        fresh.expanded = existing.expanded;
        break;
      }
    }
  }

  // A physical disk that dropped out of the fresh scan was unplugged --
  // proactively unmount anything that was mounted from it (terminating its
  // helper process) rather than leaving an orphaned helper pointed at a
  // now-vanished device.
  for (auto &existing : disks_) {
    if (existing.kind != model::DiskKind::Physical) {
      continue;
    }
    const bool still_present = std::any_of(fresh_physical.begin(), fresh_physical.end(), [&](const model::Disk &d) {
      return d.source_path == existing.source_path;
    });
    if (still_present) {
      continue;
    }
    for (auto &partition : existing.partitions) {
      for (auto &volume : partition.volumes) {
        if (volume.mounted) {
          callbacks::unmount_volume(volume);
        }
      }
    }
  }

  std::vector<model::Disk> merged = std::move(fresh_physical);
  for (auto &existing : disks_) {
    if (existing.kind == model::DiskKind::Dmg) {
      merged.push_back(std::move(existing));
    }
  }
  disks_ = std::move(merged);

  selected_disk_ = 0;
  selected_partition_ = 0;
  selected_volume_ = -1;
  if (had_selection && !selected_source_path.empty()) {
    for (size_t i = 0; i < disks_.size(); ++i) {
      if (disks_[i].source_path != selected_source_path) {
        continue;
      }
      selected_disk_ = int(i);

      auto &partitions = disks_[i].partitions;
      for (size_t p = 0; p < partitions.size(); ++p) {
        if (partitions[p].disk_partition_index != selected_disk_partition_index) {
          continue;
        }
        selected_partition_ = int(p);
        if (selected_volume_index >= 0 && selected_volume_index < int(partitions[p].volumes.size())) {
          selected_volume_ = selected_volume_index;
        }
        break;
      }
      break;
    }
  }

  InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::on_lbutton_down(float x, float y) {
  // Incoming coordinates are physical pixels; hits_ was recorded in the same
  // logical-unit space as layout (see draw_sidebar/draw_content), so convert
  // here rather than at every call site below.
  x /= dpi_scale_;
  y /= dpi_scale_;

  if (hit_test(hits_.add_button, x, y)) {
    add_disk_from_dmg();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return;
  }

  const size_t close_count = std::min(hits_.disk_close_buttons.size(), disks_.size());
  for (size_t i = 0; i < close_count; ++i) {
    if (disks_[i].removable && hit_test(hits_.disk_close_buttons[i], x, y)) {
      remove_disk(int(i));
      InvalidateRect(hwnd_, nullptr, FALSE);
      return;
    }
  }

  const size_t disk_row_count = std::min(hits_.disk_rows.size(), disks_.size());
  for (size_t i = 0; i < disk_row_count; ++i) {
    if (hit_test(hits_.disk_rows[i], x, y)) {
      toggle_disk_expanded(int(i));
      InvalidateRect(hwnd_, nullptr, FALSE);
      return;
    }
  }

  for (size_t i = 0; i < hits_.partition_rows.size(); ++i) {
    if (hit_test(hits_.partition_rows[i], x, y)) {
      auto [disk_index, partition_index] = hits_.partition_row_targets[i];
      select_partition(disk_index, partition_index);
      InvalidateRect(hwnd_, nullptr, FALSE);
      return;
    }
  }

  for (size_t i = 0; i < hits_.volume_rows.size(); ++i) {
    if (hit_test(hits_.volume_rows[i], x, y)) {
      select_volume(int(i));
      InvalidateRect(hwnd_, nullptr, FALSE);
      return;
    }
  }

  if (hits_.mount_button_valid && hit_test(hits_.mount_button, x, y)) {
    toggle_mount();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return;
  }
}

void AppWindow::set_hover(HoverKind kind, int index) {
  if (hover_kind_ == kind && hover_index_ == index) {
    return;
  }
  hover_kind_ = kind;
  hover_index_ = index;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::on_mouse_move(float x, float y) {
  x /= dpi_scale_;
  y /= dpi_scale_;

  if (!tracking_mouse_) {
    TRACKMOUSEEVENT tme{};
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hwnd_;
    TrackMouseEvent(&tme);
    tracking_mouse_ = true;
  }

  if (hit_test(hits_.add_button, x, y)) {
    set_hover(HoverKind::AddButton, 0);
    return;
  }

  const size_t close_count = std::min(hits_.disk_close_buttons.size(), disks_.size());
  for (size_t i = 0; i < close_count; ++i) {
    if (disks_[i].removable && hit_test(hits_.disk_close_buttons[i], x, y)) {
      set_hover(HoverKind::DiskClose, int(i));
      return;
    }
  }

  const size_t disk_row_count = std::min(hits_.disk_rows.size(), disks_.size());
  for (size_t i = 0; i < disk_row_count; ++i) {
    if (hit_test(hits_.disk_rows[i], x, y)) {
      set_hover(HoverKind::DiskRow, int(i));
      return;
    }
  }

  for (size_t i = 0; i < hits_.partition_rows.size(); ++i) {
    if (hit_test(hits_.partition_rows[i], x, y)) {
      set_hover(HoverKind::PartitionRow, int(i));
      return;
    }
  }

  for (size_t i = 0; i < hits_.volume_rows.size(); ++i) {
    if (hit_test(hits_.volume_rows[i], x, y)) {
      set_hover(HoverKind::VolumeRow, int(i));
      return;
    }
  }

  if (hits_.mount_button_valid && hit_test(hits_.mount_button, x, y)) {
    set_hover(HoverKind::MountButton, 0);
    return;
  }

  set_hover(HoverKind::None, -1);
}

void AppWindow::on_mouse_leave() {
  tracking_mouse_ = false;
  set_hover(HoverKind::None, -1);
}

void AppWindow::on_mouse_wheel(float x, float y, float notches) {
  x /= dpi_scale_;
  y /= dpi_scale_;

  constexpr float pixels_per_notch = 54.0f;
  const float scroll_delta = -notches * pixels_per_notch;

  if (hit_test(sidebar_viewport_, x, y)) {
    sidebar_scroll_ += scroll_delta;
    InvalidateRect(hwnd_, nullptr, FALSE);
  } else if (hit_test(volume_list_viewport_, x, y)) {
    volume_scroll_ += scroll_delta;
    InvalidateRect(hwnd_, nullptr, FALSE);
  }
}

void AppWindow::on_get_min_max_info(MINMAXINFO *info) {
  info->ptMinTrackSize.x = 900;
  info->ptMinTrackSize.y = 600;
}

void AppWindow::on_dpi_changed(UINT new_dpi) {
  dpi_scale_ = float(new_dpi) / 96.0f;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

#pragma once

#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Model.hpp"
#include <d2d1.h>
#include <dwrite.h>
#include <windows.h>
#include <wrl/client.h>

#include <utility>
#include <vector>

using Microsoft::WRL::ComPtr;

// Owns the Direct2D/DirectWrite resources, the display data model, and all
// selection/hover state for the main window. Rendering is immediate-mode:
// on every WM_PAINT the whole UI is laid out and drawn from scratch, and the
// rectangles used for layout are recorded into `HitRegions` so subsequent
// mouse input can be hit-tested against them.
//
// The sidebar shows a disk -> partition tree (a disk can be expanded or
// collapsed to reveal/hide its partitions); selecting a partition drives the
// content pane (partition info + its volumes).
class AppWindow {
public:
  explicit AppWindow(HWND hwnd);

  void on_paint();
  void on_resize(UINT width, UINT height);
  void on_lbutton_down(float x, float y);
  void on_mouse_move(float x, float y);
  void on_mouse_leave();
  void on_mouse_wheel(float x, float y, float notches);
  void on_get_min_max_info(MINMAXINFO *info);

  // Called from WM_DPICHANGED with the window's new DPI. Layout/hit-testing
  // is all done in 96-DPI logical units, scaled up to physical pixels only
  // at render time (see on_paint) -- this keeps that scale factor current so
  // rendering and mouse-coordinate handling agree on monitors above 100%.
  void on_dpi_changed(UINT new_dpi);

  // Re-scans physically-attached drives (see callbacks::list_disks) and
  // merges the result into disks_, leaving any .dmg-loaded disks alone.
  // Called (debounced) from WM_DEVICECHANGE so the sidebar picks up drives
  // being plugged in or removed without needing a restart.
  void refresh_physical_disks();

private:
  struct HitRegions {
    std::vector<D2D1_RECT_F> disk_rows;
    std::vector<D2D1_RECT_F> disk_close_buttons; // parallel to disk_rows
    std::vector<D2D1_RECT_F> partition_rows;     // flattened, only currently-expanded disks
    std::vector<std::pair<int, int>> partition_row_targets; // (disk_index, partition_index), parallel to partition_rows
    D2D1_RECT_F add_button{};
    std::vector<D2D1_RECT_F> volume_rows;
    D2D1_RECT_F mount_button{};
    bool mount_button_valid = false;
  };

  enum class HoverKind {
    None, DiskRow, DiskClose, PartitionRow, AddButton, VolumeRow, MountButton
  };

  void create_device_resources();
  void discard_device_resources();
  void create_text_formats();

  void draw_sidebar(HitRegions &hits);
  void draw_content(HitRegions &hits);
  void draw_partition_info(const D2D1_RECT_F &area);
  void draw_volume_section(const D2D1_RECT_F &area, model::Partition &part, HitRegions &hits);
  void draw_scrollbar(const D2D1_RECT_F &viewport, float content_height, float scroll_offset);

  void draw_text(const D2D1_RECT_F &rect, IDWriteTextFormat *format, const D2D1_COLOR_F &color, const std::wstring &text);

  void select_partition(int disk_index, int partition_index);
  void select_volume(int index);
  void toggle_disk_expanded(int index);
  void remove_disk(int index);
  void add_disk_from_dmg();
  void toggle_mount();

  void set_hover(HoverKind kind, int index);

  static bool hit_test(const D2D1_RECT_F &r, float x, float y);

  HWND hwnd_;

  ComPtr<ID2D1Factory> d2d_factory_;
  ComPtr<ID2D1HwndRenderTarget> render_target_;
  ComPtr<IDWriteFactory> dwrite_factory_;
  ComPtr<ID2D1SolidColorBrush> brush_;
  ComPtr<ID2D1SolidColorBrush> icon_stroke_brush_;
  ComPtr<ID2D1SolidColorBrush> icon_fill_brush_;

  ComPtr<IDWriteTextFormat> font_row_name_;
  ComPtr<IDWriteTextFormat> font_row_subtitle_;
  ComPtr<IDWriteTextFormat> font_partition_row_;
  ComPtr<IDWriteTextFormat> font_disk_name_;
  ComPtr<IDWriteTextFormat> font_partition_name_;
  ComPtr<IDWriteTextFormat> font_capacity_;
  ComPtr<IDWriteTextFormat> font_section_label_;
  ComPtr<IDWriteTextFormat> font_volume_row_;
  ComPtr<IDWriteTextFormat> font_volume_name_;
  ComPtr<IDWriteTextFormat> font_volume_detail_;
  ComPtr<IDWriteTextFormat> font_button_;
  ComPtr<IDWriteTextFormat> font_placeholder_;
  ComPtr<IDWriteTextFormat> font_glyph_;
  ComPtr<IDWriteTextFormat> font_empty_state_title_;

  std::vector<model::Disk> disks_;
  int selected_disk_ = 0;
  int selected_partition_ = 0;
  int selected_volume_ = -1;

  HitRegions hits_;

  HoverKind hover_kind_ = HoverKind::None;
  int hover_index_ = -1;
  bool tracking_mouse_ = false;

  float sidebar_scroll_ = 0.0f;
  float volume_scroll_ = 0.0f;
  D2D1_RECT_F sidebar_viewport_{};
  D2D1_RECT_F volume_list_viewport_{};

  // Physical-pixels-per-96-DPI-logical-unit scale factor for the monitor
  // this window is currently on (1.0 at 100% scaling). All layout/hit-test
  // math is done in logical units; see on_paint (render-time scale
  // transform) and the on_mouse_* handlers (which divide incoming physical
  // coordinates by this before hit-testing).
  float dpi_scale_ = 1.0f;
};

#pragma once

#include <d2d1.h>

// Central place for colors and layout constants so the look of the app can be
// tweaked without hunting through the rendering code.
namespace theme {

// Colors

inline constexpr D2D1_COLOR_F window_bg = {1.0f, 1.0f, 1.0f, 1.0f};

inline constexpr D2D1_COLOR_F sidebar_bg = {0.961f, 0.961f, 0.973f, 1.0f};
inline constexpr D2D1_COLOR_F sidebar_border = {0.87f, 0.87f, 0.88f, 1.0f};
inline constexpr D2D1_COLOR_F row_hover_bg = {0.914f, 0.914f, 0.929f, 1.0f};
inline constexpr D2D1_COLOR_F row_selected_bg = {0.0f, 0.478f, 1.0f, 0.14f};

inline constexpr D2D1_COLOR_F text_primary = {0.11f, 0.11f, 0.118f, 1.0f};
inline constexpr D2D1_COLOR_F text_secondary = {0.525f, 0.525f, 0.545f, 1.0f};
inline constexpr D2D1_COLOR_F text_placeholder = {0.68f, 0.68f, 0.70f, 1.0f};
inline constexpr D2D1_COLOR_F text_on_accent = {1.0f, 1.0f, 1.0f, 1.0f};

inline constexpr D2D1_COLOR_F accent = {0.0f, 0.478f, 1.0f, 1.0f};
inline constexpr D2D1_COLOR_F accent_hover = {0.0f, 0.40f, 0.90f, 1.0f};

inline constexpr D2D1_COLOR_F danger = {0.90f, 0.28f, 0.24f, 1.0f};
inline constexpr D2D1_COLOR_F danger_hover = {0.78f, 0.20f, 0.17f, 1.0f};

inline constexpr D2D1_COLOR_F divider = {0.90f, 0.90f, 0.91f, 1.0f};
inline constexpr D2D1_COLOR_F icon_stroke = {0.42f, 0.42f, 0.45f, 1.0f};
inline constexpr D2D1_COLOR_F icon_fill = {0.80f, 0.80f, 0.83f, 1.0f};

inline constexpr D2D1_COLOR_F progress_track = {0.90f, 0.90f, 0.91f, 1.0f};
inline constexpr D2D1_COLOR_F progress_fill = {0.0f, 0.478f, 1.0f, 1.0f};

// Deliberately distinct from row_hover_bg -- this is a persistent, non-hover
// background, so it must read differently from the transient hover state.
inline constexpr D2D1_COLOR_F add_row_bg = {0.945f, 0.945f, 0.955f, 1.0f};
inline constexpr D2D1_COLOR_F add_row_bg_hover = row_hover_bg;

// A faint, permanent (not hover-dependent) tint used to signal tree
// structure in the sidebar: behind a collapsed disk's chevron, and along the
// left edge of every partition row so it visually reads as nested under its
// disk rather than a sibling top-level entry. Deliberately darker than
// row_hover_bg so it doesn't get mistaken for hover feedback.
inline constexpr D2D1_COLOR_F hierarchy_indicator_bg = {0.93f, 0.93f, 0.94f, 1.0f};

// Layout

inline constexpr float sidebar_width = 245.0f;

inline constexpr float sidebar_row_height = 64.0f;
inline constexpr float sidebar_row_icon_size = 26.0f;
inline constexpr float sidebar_row_padding_x = 16.0f;
inline constexpr float sidebar_close_btn_size = 24.0f;
inline constexpr float sidebar_chevron_width = 16.0f;

inline constexpr float partition_row_height = 36.0f;
inline constexpr float partition_row_indent = 34.0f;

inline constexpr float content_padding = 28.0f;

inline constexpr float volume_list_width = 200.0f;
inline constexpr float volume_row_height = 44.0f;

inline constexpr float mount_button_width = 120.0f;
inline constexpr float mount_button_height = 36.0f;

inline constexpr float scrollbar_width = 4.0f;
inline constexpr float scrollbar_margin = 2.0f;

} // namespace theme

#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Notification cards ────────────────────────────────────────────────
//
// A notification card shows one Home Assistant message on a single line,
// vertically centred, under an ordinary card icon that follows the severity.
// The severity attribute only changes the colours and that icon, never the
// wording; a message too wide for the card scrolls as a marquee. Tapping the
// card opens the full text, and an optional Home Assistant action
// acknowledges it.

// NotificationLevel and its alias matching live in button_grid_config_parser.h
// so the browser and firmware can be compared without LVGL.

struct NotificationPalette {
  uint32_t background;
  uint32_t text;
};

inline NotificationPalette notification_level_palette(NotificationLevel level) {
  switch (level) {
    case NotificationLevel::WARNING:
      return {NOTIFICATION_WARNING_BG, NOTIFICATION_WARNING_TEXT};
    case NotificationLevel::ALERT:
      return {NOTIFICATION_ALERT_BG, NOTIFICATION_ALERT_TEXT};
    case NotificationLevel::INFORMATION:
    default:
      return {NOTIFICATION_INFORMATION_BG, NOTIFICATION_INFORMATION_TEXT};
  }
}

// The modal names the level in a coloured pill. The modal panel already uses
// the information background, so that level swaps in the neutral card grey and
// the other two keep their severity colours.
inline NotificationPalette notification_level_pill_palette(NotificationLevel level) {
  if (level == NotificationLevel::INFORMATION) {
    return {SECONDARY_GREY, DARK_TEXT_PRIMARY};
  }
  return notification_level_palette(level);
}

inline std::string notification_level_title(NotificationLevel level) {
  switch (level) {
    case NotificationLevel::WARNING:
      return espcontrol_i18n(std::string("Warning"));
    case NotificationLevel::ALERT:
      return espcontrol_i18n(std::string("Alert"));
    case NotificationLevel::INFORMATION:
    default:
      return espcontrol_i18n(std::string("Information"));
  }
}

inline const char *notification_level_icon(NotificationLevel level) {
  switch (level) {
    case NotificationLevel::WARNING: return find_icon("Alert");
    case NotificationLevel::ALERT: return find_icon("Alert Octagon");
    case NotificationLevel::INFORMATION:
    default: return find_icon("Information");
  }
}

struct NotificationCardCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  std::string entity_id;
  std::string ack_action;
  std::string message;
  NotificationLevel level = NotificationLevel::INFORMATION;
  int width_compensation_percent = 100;
};

struct NotificationModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *message_lbl = nullptr;
  lv_obj_t *ack_btn = nullptr;
  std::string entity_id;
  std::string ack_action;
};

inline NotificationModalUi &notification_modal_ui() {
  static NotificationModalUi ui;
  return ui;
}

inline void notification_card_hide_modal() {
  NotificationModalUi &ui = notification_modal_ui();
  control_modal_delete_overlay(ControlModalKind::NOTIFICATION, ui.overlay);
  ui = NotificationModalUi();
}

inline void notification_card_acknowledge() {
  NotificationModalUi &ui = notification_modal_ui();
  const std::string entity_id = ui.entity_id;
  const std::string action = ui.ack_action;
  notification_card_hide_modal();
  if (entity_id.empty() || action.empty()) return;
  if (!ha_send_entity_action(entity_id, action.c_str())) {
    ESP_LOGW("notification", "Acknowledge action %s failed for %s",
             action.c_str(), entity_id.c_str());
  }
}

// Layout is a pure function of the slot and display, so the generic card
// layout pass can reapply it without the runtime context.
inline void notification_card_apply_card_layout(
    const BtnSlot &s, const DisplayProfile &display) {
  if (s.sensor_container) lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  // The icon keeps the standard card position, size, and width compensation;
  // only its glyph follows the severity.
  if (s.icon_lbl) lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.text_lbl) {
    lv_obj_clear_flag(s.text_lbl, LV_OBJ_FLAG_HIDDEN);
    // Messages are sentences, so they get the larger title role rather than the
    // ordinary card label size.
    const lv_font_t *font = display_media_title_font(display);
    if (font) lv_obj_set_style_text_font(s.text_lbl, font, LV_PART_MAIN);
    // A circular scroll only animates once the text is wider than the label, so
    // a short message simply sits still on its single centred line.
    lv_obj_set_style_max_height(s.text_lbl, LV_COORD_MAX, LV_PART_MAIN);
    lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s.text_lbl, lv_pct(100));
    lv_obj_set_height(s.text_lbl, LV_SIZE_CONTENT);
    lv_obj_align(s.text_lbl, LV_ALIGN_LEFT_MID, 0, 0);
  }
}

inline void notification_card_apply_message(NotificationCardCtx *ctx) {
  if (ctx == nullptr || ctx->text_lbl == nullptr) return;
  lv_label_set_text(ctx->text_lbl, ctx->message.c_str());
}

inline void notification_card_apply_level(NotificationCardCtx *ctx) {
  if (ctx == nullptr) return;
  const NotificationPalette palette = notification_level_palette(ctx->level);
  if (ctx->btn != nullptr) {
    lv_obj_set_style_bg_color(
      ctx->btn, lv_color_hex(palette.background),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  if (ctx->text_lbl != nullptr) {
    lv_obj_set_style_text_color(ctx->text_lbl, lv_color_hex(palette.text), LV_PART_MAIN);
  }
  if (ctx->icon_lbl != nullptr) {
    lv_obj_set_style_text_color(ctx->icon_lbl, lv_color_hex(palette.text), LV_PART_MAIN);
    lv_label_set_text(ctx->icon_lbl, notification_level_icon(ctx->level));
  }
}

inline void notification_card_refresh(NotificationCardCtx *ctx) {
  notification_card_apply_level(ctx);
  notification_card_apply_message(ctx);
}

inline void setup_notification_card(
    BtnSlot &s, const ParsedCfg &p, const DisplayProfile &display) {
  notification_card_apply_card_layout(s, display);
  if (s.icon_lbl) {
    lv_label_set_text(s.icon_lbl, notification_level_icon(NotificationLevel::INFORMATION));
    lv_obj_set_style_text_color(
      s.icon_lbl, lv_color_hex(NOTIFICATION_INFORMATION_TEXT), LV_PART_MAIN);
  }
  if (s.btn) {
    lv_obj_set_style_bg_color(
      s.btn, lv_color_hex(NOTIFICATION_INFORMATION_BG),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  if (s.text_lbl) {
    lv_obj_set_style_text_color(
      s.text_lbl, lv_color_hex(NOTIFICATION_INFORMATION_TEXT), LV_PART_MAIN);
    lv_label_set_text(s.text_lbl, p.label.empty() ? "--" : p.label.c_str());
  }
}

inline void notification_card_open_modal(NotificationCardCtx *ctx) {
  if (ctx == nullptr) return;
  const lv_font_t *label_font = ctx->label_font;
  const lv_font_t *icon_font = ctx->icon_font ? ctx->icon_font : label_font;

  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::NOTIFICATION, ctx->btn, ctx->width_compensation_percent,
    icon_font, notification_card_hide_modal);
  if (shell.overlay == nullptr) return;
  NotificationModalUi &ui = notification_modal_ui();
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;
  ui.entity_id = ctx->entity_id;
  ui.ack_action = ctx->ack_action;

  ControlModalLayout &layout = shell.layout;
  const lv_coord_t content_w = shell.content_w;
  lv_coord_t gap = control_modal_scaled_px(16, layout.short_side);
  if (gap < 10) gap = 10;

  const NotificationPalette pill = notification_level_pill_palette(ctx->level);
  lv_coord_t pill_pad_x = control_modal_scaled_px(14, layout.short_side);
  if (pill_pad_x < 10) pill_pad_x = 10;
  lv_coord_t pill_pad_y = control_modal_scaled_px(6, layout.short_side);
  if (pill_pad_y < 4) pill_pad_y = 4;
  ui.title_lbl = lv_label_create(ui.panel);
  lv_label_set_text(ui.title_lbl, notification_level_title(ctx->level).c_str());
  lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(pill.text), LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui.title_lbl, lv_color_hex(pill.background), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.title_lbl, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_left(ui.title_lbl, pill_pad_x, LV_PART_MAIN);
  lv_obj_set_style_pad_right(ui.title_lbl, pill_pad_x, LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui.title_lbl, pill_pad_y, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(ui.title_lbl, pill_pad_y, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(ui.title_lbl, label_font, LV_PART_MAIN);
  lv_obj_update_layout(ui.title_lbl);
  lv_obj_set_style_radius(
    ui.title_lbl, lv_obj_get_height(ui.title_lbl) / 2, LV_PART_MAIN);

  ui.message_lbl = lv_label_create(ui.panel);
  lv_label_set_text(
    ui.message_lbl,
    ctx->message.empty() ? espcontrol_i18n("No message") : ctx->message.c_str());
  lv_label_set_long_mode(ui.message_lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(ui.message_lbl, content_w);
  lv_obj_set_style_text_align(ui.message_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui.message_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(ui.message_lbl, label_font, LV_PART_MAIN);

  lv_coord_t ack_height = 0;
  if (!ui.ack_action.empty() && !ui.entity_id.empty()) {
    lv_coord_t button_h = control_modal_scaled_px(52, layout.short_side);
    if (button_h < 36) button_h = 36;
    lv_coord_t button_min_w = button_h + control_modal_scaled_px(16, layout.short_side);
    if (button_min_w < 56) button_min_w = 56;
    if (button_min_w > content_w) button_min_w = content_w;
    const uint32_t ack_color = current_button_primary_color();
    ui.ack_btn = control_modal_create_text_button(
      ui.panel, espcontrol_i18n(std::string("Acknowledge")), content_w,
      button_min_w, button_h, button_h / 2, ack_color, label_font);
    if (ui.ack_btn != nullptr) {
      lv_obj_t *ack_label = lv_obj_get_child(ui.ack_btn, 0);
      if (ack_label) {
        lv_obj_set_style_text_color(
          ack_label, lv_color_hex(readable_text_color_for_bg(ack_color)), LV_PART_MAIN);
      }
      lv_obj_add_event_cb(ui.ack_btn, [](lv_event_t *) {
        notification_card_acknowledge();
      }, LV_EVENT_CLICKED, nullptr);
      lv_obj_update_layout(ui.ack_btn);
      ack_height = lv_obj_get_height(ui.ack_btn) + gap;
    }
  }

  // Keep the whole stack inside the panel, leaving room for the close button.
  const lv_coord_t top_limit = layout.inset + layout.back_size + gap;
  lv_coord_t message_max_h = layout.panel_h - top_limit - layout.inset - ack_height;
  if (message_max_h < layout.short_side / 6) message_max_h = layout.short_side / 6;
  lv_obj_set_style_max_height(ui.message_lbl, message_max_h, LV_PART_MAIN);

  lv_obj_update_layout(ui.title_lbl);
  lv_obj_update_layout(ui.message_lbl);
  const lv_coord_t title_h = lv_obj_get_height(ui.title_lbl);
  const lv_coord_t message_h = lv_obj_get_height(ui.message_lbl);
  const lv_coord_t group_h = title_h + gap + message_h + ack_height;
  lv_coord_t group_top = (layout.panel_h - group_h) / 2;
  if (group_top < top_limit) group_top = top_limit;

  lv_obj_align(ui.title_lbl, LV_ALIGN_CENTER, 0,
               group_top + title_h / 2 - layout.panel_h / 2);
  lv_obj_align(ui.message_lbl, LV_ALIGN_CENTER, 0,
               group_top + title_h + gap + message_h / 2 - layout.panel_h / 2);
  if (ui.ack_btn != nullptr) {
    const lv_coord_t ack_h = lv_obj_get_height(ui.ack_btn);
    lv_obj_align(ui.ack_btn, LV_ALIGN_CENTER, 0,
                 group_top + group_h - ack_h / 2 - layout.panel_h / 2);
  }

  lv_obj_move_foreground(ui.overlay);
}

// The message can arrive as either the entity state or one of its attributes;
// both land here so the wrap-or-scroll decision is made in one place.
inline std::function<void(esphome::StringRef)> notification_card_message_callback(
    NotificationCardCtx *ctx) {
  return [ctx](esphome::StringRef state) {
    ctx->message = ha_state_unavailable_ref(state)
      ? std::string("--")
      : string_ref_limited(state, NOTIFICATION_MAX_MESSAGE_LENGTH);
    notification_card_apply_message(ctx);
  };
}

inline void notification_card_bind_runtime(
    BtnSlot &s, const ParsedCfg &p, int width_compensation_percent,
    bool is_subpage) {
  if (s.btn == nullptr) return;
  NotificationCardCtx *ctx = is_subpage
    ? grid_delete_with_owner(s.btn, new NotificationCardCtx())
    : grid_track_runtime_allocation(s.btn, new NotificationCardCtx());
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  // The card message uses the enlarged title font; the modal wraps a paragraph,
  // so it keeps the ordinary card label size from the button itself.
  ctx->label_font = s.btn
    ? lv_obj_get_style_text_font(s.btn, LV_PART_MAIN) : nullptr;
  ctx->icon_font = s.icon_lbl
    ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  ctx->entity_id = p.entity;
  ctx->ack_action = notification_card_ack_action(p.options);
  if (!notification_card_ack_action_valid(ctx->ack_action)) ctx->ack_action.clear();
  ctx->width_compensation_percent = width_compensation_percent;
  ctx->message = p.label.empty() ? std::string("--") : p.label;
  lv_obj_set_user_data(s.btn, ctx);
  lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  if (is_subpage) {
    lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
      lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
      notification_card_open_modal(
        target ? static_cast<NotificationCardCtx *>(lv_obj_get_user_data(target))
               : nullptr);
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (p.entity.empty()) {
    notification_card_refresh(ctx);
    return;
  }

  const std::string message_attribute = notification_card_message_attribute(p.options);
  if (message_attribute.empty()) {
    ha_subscribe_state(p.entity, notification_card_message_callback(ctx));
  } else {
    ha_subscribe_attribute(
      p.entity, message_attribute, notification_card_message_callback(ctx),
      HA_SUBSCRIPTION_SCOPE_DEFAULT, true);
  }

  ha_subscribe_attribute(
    p.entity, notification_card_level_attribute(p.options),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      ctx->level = notification_level_from_text(
        string_ref_limited(state, NOTIFICATION_MAX_LEVEL_LENGTH));
      notification_card_apply_level(ctx);
    }),
    HA_SUBSCRIPTION_SCOPE_DEFAULT,
    true);
  notification_card_refresh(ctx);
}

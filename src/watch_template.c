#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xF2, 0xA1, 0xFC, 0x3D, 0x66, 0x53, 0x4F, 0xC5, 0x86, 0xB2, 0x6E, 0x2A, 0xDC, 0x08, 0xBC, 0xD4 }
PBL_APP_INFO(MY_UUID,
             "Altimeter", "Sasha Ognev",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define SCREEN_HEIGHT 168
#define SCREEN_WIDTH 144

Window window;
BitmapLayer background_layer;
Layer hour_layer;
Layer minute_layer;
Layer seconds_layer;
BmpContainer background_image;

double ah, am, as;

const GPathInfo HOUR_HAND_PATH_POINTS = {
  5,
  (GPoint []) {
    {4, 0},
    {8, -30},
    {0,  -45},
    {-8, -30},
    {-4, 0}
  }
};

const GPathInfo MINUTE_HAND_PATH_POINTS = {
  5,
  (GPoint []) {
    {4, 14},
    {4, -55},
    {0,  -65},
    {-4, -55},
    {-4, 14}
  }
};


const GPathInfo SECOND_HAND_PATH_POINTS = {
  4,
  (GPoint []) {
    {2, 12},
    {2, -70},
    {-2,  -70},
    {-2, 12}
  }
};

GPath hour_hand_path;
GPath minute_hand_path;
GPath second_hand_path;

void draw_hand(GContext* ctx, GPath* path, int angle)
{
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_rotate_to(path, angle);
  gpath_draw_filled(ctx, path);
  gpath_draw_outline(ctx, path);

}

void hour_layer_update_callback(Layer *me, GContext* ctx)
{
  (void)me;
  draw_hand(ctx, &hour_hand_path, ah);
  graphics_fill_circle(ctx, grect_center_point(&me->frame), 7);
  graphics_draw_circle(ctx, grect_center_point(&me->frame), 3);

}

void minute_layer_update_callback(Layer *me, GContext* ctx)
{
  draw_hand(ctx, &minute_hand_path, am);
}

void seconds_layer_update_callback(Layer *me, GContext* ctx)
{
  (void)me;
  GPoint ptc = grect_center_point(&me->frame);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  int r = 70;
  int r2 = -12;
  int angle = as - TRIG_MAX_RATIO / 4;
  if (angle < 0)
    angle += TRIG_MAX_RATIO;
  int x2 = ptc.x + cos_lookup(angle) * r/TRIG_MAX_RATIO;
  int y2 = ptc.y + sin_lookup(angle) * r/TRIG_MAX_RATIO;
  int x1 = ptc.x + cos_lookup(angle) * r2/TRIG_MAX_RATIO;
  int y1 = ptc.y + sin_lookup(angle) * r2/TRIG_MAX_RATIO;

  gpath_rotate_to(&second_hand_path, as);
  gpath_draw_filled(ctx, &second_hand_path);

  graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
  graphics_draw_pixel(ctx, ptc);
}

void update_angles(PblTm* time)
{
  int ah_new = ((time->tm_hour * 60 + time->tm_min) * TRIG_MAX_ANGLE / (24 * 60));
  if (ah_new != ah)
  {
      ah = ah_new;
      layer_mark_dirty(&hour_layer);
  }
  int am_new = ((time->tm_min * 60 + time->tm_sec) * TRIG_MAX_ANGLE / (60 * 60));
  if (am_new != am)
  {
      am = am_new;
      layer_mark_dirty(&minute_layer);
  }
  as = (time->tm_sec * TRIG_MAX_ANGLE / 60);
  layer_mark_dirty(&seconds_layer);
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Altimeter");
  window_stack_push(&window, true /* Animated */);

  // background image
  resource_init_current_app(&APP_RESOURCES);
  bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image);
  bitmap_layer_init(&background_layer, window.layer.frame);
  bitmap_layer_set_bitmap(&background_layer, &background_image.bmp);
  layer_add_child(&window.layer, &background_layer.layer);

  // Layers for hands
  layer_init(&hour_layer, window.layer.frame);
  hour_layer.update_proc = &hour_layer_update_callback;
  layer_add_child(&window.layer, &hour_layer);

  layer_init(&minute_layer, window.layer.frame);
  minute_layer.update_proc = &minute_layer_update_callback;
  layer_add_child(&window.layer, &minute_layer);

  layer_init(&seconds_layer, window.layer.frame);
  seconds_layer.update_proc = &seconds_layer_update_callback;
  layer_add_child(&window.layer, &seconds_layer);

  gpath_init(&minute_hand_path, &MINUTE_HAND_PATH_POINTS);
  gpath_move_to(&minute_hand_path, grect_center_point(&minute_layer.frame));
  gpath_init(&hour_hand_path, &HOUR_HAND_PATH_POINTS);
  gpath_move_to(&hour_hand_path, grect_center_point(&hour_layer.frame));
  gpath_init(&second_hand_path, &SECOND_HAND_PATH_POINTS);
  gpath_move_to(&second_hand_path, grect_center_point(&hour_layer.frame));



  PblTm time;
  get_time(&time);
  update_angles(&time);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  update_angles(t->tick_time);
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  bmp_deinit_container(&background_image);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}

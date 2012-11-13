
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include "osd_window.h"

#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#define check_cairo_status() { \
  int cs = cairo_status(cr); \
  if (cs) printf("%s:%d: cairo_status=%s\n", __FILE__, __LINE__, cairo_status_to_string(cs)); \
}
#else
#define debug(...)
#define check_cairo_status()
#endif

#define MSG_LIFE 10000
int time_interval = 300;

char *font_name = "文泉驿正黑";
int font_size = 28;
float font_color[] = {0.40, 1.0, 0.50};
float border_color[] = {0.0, 0.0, 0.0};
float shadow_color[] = {0.6, 0.6, 0.6};

cairo_surface_t *sf = NULL;
cairo_t *cr = NULL;

GtkWidget *window = NULL;
GtkWidget *setup_window = NULL;
GdkPixmap *pixmap_mask = NULL;
GdkPixmap *pixmap = NULL;
int screen_width = 0;
int screen_height = 0;
int window_width = 0;
int window_height = 0;

pthread_t thread;
pthread_mutex_t mutex;
char new_msg = FALSE;
char msg_from[256];
char msg_content[1024];

void copy_surface_to_pixmap(cairo_surface_t *sf, GdkPixmap *pixmap)
{
  cairo_t *cr = gdk_cairo_create (pixmap);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (cr, sf, 0, 0);
  cairo_paint (cr);
  check_cairo_status();
  cairo_destroy (cr);
}

void paint_text(cairo_t *cr, const char *text, int x, int y, int border, int shadow,
  const float font_color[3], const float border_color[3], const float shadow_color[3])
{
  cairo_save(cr);

  if (shadow > 0)
    paint_text(cr, text, x + shadow, y + shadow, border, 0, shadow_color, shadow_color, shadow_color);
  if (border > 0) {
    cairo_set_source_rgb (cr, border_color[0], border_color[1], border_color[2]);
    int dx, dy;
    for (dx = -border; dx <= border; dx ++) {
      for (dy = -border; dy <= border; dy ++) {
        cairo_move_to (cr, x + dx, y + dy);
        cairo_show_text(cr, text);
      }
    }
  }

  cairo_set_source_rgb (cr, font_color[0], font_color[1], font_color[2]);
  cairo_move_to (cr, x, y);
  cairo_show_text(cr, text);

  cairo_restore (cr);
  check_cairo_status();
}

void paint_message(cairo_t *cr, const char *from, const char *content)
{
  static char from_buffer[256];
  if (strlen(from) > 0)
    snprintf(from_buffer, sizeof(from_buffer), "[%s]", from);
  else
    strncpy(from_buffer, "", sizeof(from_buffer));
  cairo_save(cr);
  
  //清空背景
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  //设置字体
  cairo_select_font_face (cr, font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, font_size);

  //计算文字位置
  cairo_text_extents_t extents_from, extents_content;
  cairo_text_extents(cr, from_buffer, &extents_from);
  cairo_text_extents(cr, content, &extents_content);
  int x = (window_width - extents_from.width - extents_content.width) / 2;
  if (x < 0)
    x = 0;
  int y = window_height - 10;

  //实现昵称和消息内容
  paint_text(cr, from_buffer, x, y, 1, 2, font_color, border_color, shadow_color);
  paint_text(cr, content, x + extents_from.width + font_size / 2, y, 1, 2, font_color, border_color, shadow_color);

  cairo_restore (cr);
  check_cairo_status();
}

void show_message(char *from, char *content)
{
  debug("show_message(%s, %s)\n", from, content);
  if (strlen(from) == 0 && strlen(content) == 0 ) {
    gdk_window_resize(window->window, 1, 1);
    return;
  }
  //初始化cairo环境
  paint_message (cr, from, content);
  cairo_surface_flush(sf);

  //复制生成位图
  copy_surface_to_pixmap (sf, pixmap);
  copy_surface_to_pixmap (sf, pixmap_mask);

  //刷新显示
  gtk_window_set_title(GTK_WINDOW(window), content);
  gdk_window_set_back_pixmap (window->window, pixmap, FALSE);
  gtk_widget_shape_combine_mask (window, pixmap_mask, 0, 0);
  gtk_widget_queue_draw(window);
  gdk_window_resize(window->window, window_width, window_height);
}

void* main_thread(void *p)
{
  gtk_main();
  return NULL;
}

static gboolean time_handler(GtkWidget *widget)
{
  static int msg_life = 0;
  pthread_mutex_lock(&mutex);
  if (new_msg) {
    new_msg = FALSE;
    msg_life = MSG_LIFE / time_interval;
    pthread_mutex_unlock(&mutex);
    show_message(msg_from, msg_content);
  }
  else {
    pthread_mutex_unlock(&mutex);
    if (--msg_life == 0)
      win_show("", "");
  }
  return TRUE;
}

void win_init()
{
  //创建窗口
  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_decorated (GTK_WINDOW(window), FALSE);       // 设置无边框
  gtk_window_set_keep_above (GTK_WINDOW(window), TRUE);
  gtk_widget_set_app_paintable (window, TRUE);
  gtk_widget_realize (window);

  GdkScreen *screen = gtk_widget_get_screen(window);
  screen_width = gdk_screen_get_width(screen);
  screen_height = gdk_screen_get_height(screen);
  window_width = screen_width;
  window_height = font_size * 1.5;
  debug("screen_size=(%d, %d), window_size=(%d,%d)\n", screen_width, screen_height, window_width, window_height);

  //创建位图与蒙板
  pixmap = gdk_pixmap_new (window->window, window_width, window_height, -1);
  pixmap_mask = gdk_pixmap_new (window->window, window_width, window_height, 1);

  //初始化绘图环境
  sf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, window_width, window_height);
  cr = cairo_create (sf);

  pthread_mutex_init (&mutex,NULL);
  g_timeout_add(time_interval, (GSourceFunc) time_handler, (gpointer) window);

  //显示窗口
  win_show("", "OSDChat");
  gtk_widget_show_all (window);
  gdk_window_move_resize(window->window, 0, screen_height - window_height, window_width, window_height);

  //启动线程
  pthread_create(&thread, NULL, main_thread, NULL);

}

void win_show(char *from, char *content)
{
  debug("win_show(%s, %s)\n", from, content);
  if (strlen(from) + 1 >= sizeof(msg_from)) return;
  if (strlen(content) + 1 >= sizeof(msg_content)) return;
  pthread_mutex_lock(&mutex);
  new_msg = TRUE;
  strncpy(msg_from, from, sizeof(msg_from));
  strncpy(msg_content, content, sizeof(msg_content));
  pthread_mutex_unlock(&mutex);
}

void win_set_font_color(int color)
{
  font_color[0] = ((color >> 16) & 0xff) / 255.0;
  font_color[1] = ((color >> 8) & 0xff) / 255.0;
  font_color[2] = (color & 0xff) / 255.0;
}

#ifdef TEST_WINDOW
int main (int argc, char *argv[])
{
  time_interval = 50;
  gtk_init (&argc, &argv);
  win_init ();
  win_set_font_color(0xffffff);
  win_show ("Somebody", "Hello，你好。");
  sleep(2);
  win_set_font_color(0xff0000);
  win_show ("Somebody2", "Hello，你好2。");
  sleep(3);
  win_set_font_color(0x00ff00);
  win_show ("Somebody3", "Hello，你好3。");
  win_show ("Somebody4", "Hello，你好4。");
  win_show ("Somebody5", "Hello，你好5。");
  win_show ("Somebody6", "Hello，你好6。");
  sleep(12);
  win_set_font_color(0x0000ff);
  win_show ("Jack", "No no no ....................");
  sleep(1);
  win_show ("Tom", "1234567890abcdefghijklmnopqrstuvwxyz");
  sleep(1);
  return 0;
}
#endif

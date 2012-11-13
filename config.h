
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
  char name[32];
  int font_color;
} ChannelConfig;

void config_init();
void config_add(const char *name, int font_color);
ChannelConfig* config_get(const char *name);

#endif

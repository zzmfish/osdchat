#include "config.h"
#include <string.h>
#define MAX_CONFIG_COUNT 32

ChannelConfig config_list[MAX_CONFIG_COUNT];
int config_count = 0;

void config_init()
{
  memset(config_list, 0, sizeof(config_list));
}

void config_add(const char *name, int font_color)
{
  if (config_count >= MAX_CONFIG_COUNT) return;
  strncpy(config_list[config_count].name, name, sizeof(config_list[config_count].name));
  config_list[config_count].font_color = font_color;
  config_count ++;
}

ChannelConfig* config_get(const char *name)
{
  int i = 0;
  for (; i < config_count; i ++) {
    if (strcasecmp(name, config_list[i].name) == 0)
      return &config_list[i];
  }
  return NULL;
}

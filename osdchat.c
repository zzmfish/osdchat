#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "xchat-plugin.h"
#include "osd_window.h"
#include "config.h"

#define PNAME "OSD-Chat"
#define PDESC "Display XChat Messages On Screen"
#define PVERSION "0.1"

static char enabled = 1;
static xchat_plugin *ph = NULL;

static int osd_cb(char *word[], char *word_eol[], void *userdata)
{
  char *cmd2 = word[2];
  if (strcasecmp(cmd2, "on") == 0) {
    xchat_print(ph, "OSD is on.");
    enabled = 1;
  }
  else if (strcasecmp(cmd2, "off") == 0) {
    xchat_print(ph, "OSD is off.");
    enabled = 0;
  }
  else if (strcasecmp(cmd2, "setup") == 0) {
    xchat_print(ph, "===setup===\n");
    xchat_list *list = xchat_list_get(ph, "channels");
    while(xchat_list_next(ph, list)) {
      const char *channel = xchat_list_str(ph, list, "channel");
      if (!channel)
        channel = "NULL";
      xchat_printf(ph, "channel=%s\n", channel);
    }
  }
  else if (strcasecmp(cmd2, "testwin") == 0) {
    win_show("test", "???");
  }
  return XCHAT_EAT_ALL;
}
static int msg_cb(char *word[], void *userdata)
{
  if (enabled) {
    const char *channel = xchat_get_info(ph, "channel");
    ChannelConfig* config = config_get(channel);
    win_set_font_color(config ? config->font_color : 0x70ff80);
    win_show(word[1], word[2]);
  }
  return XCHAT_EAT_NONE;
}

void xchat_plugin_get_info(char **name, char **desc, char **version, void **reserved)
{
   *name = PNAME;
   *desc = PDESC;
   *version = PVERSION;
}

int xchat_plugin_init(xchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
   ph = plugin_handle;

   *plugin_name = PNAME;
   *plugin_desc = PDESC;
   *plugin_version = PVERSION;

   xchat_hook_command(ph, "OSD", XCHAT_PRI_NORM, osd_cb, "Usage: osd on/off", 0);
   xchat_hook_print(ph, "Channel Message", XCHAT_PRI_NORM, msg_cb, 0);
   win_init();
   config_init();
   config_add("#ubuntu", 0x49cff4);
   config_add("#ubuntu-cn", 0x70ff80);
   config_add("#arch-cn", 0xffff5c);
   return 1;
}


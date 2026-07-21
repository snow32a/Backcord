#include "globals.h"
#include "components/snoctrl.h"
int DiscordSendMessage(const char *channelID, const char* content);
int DiscordListGuildChannels(char* guildID, DiscordChannel** out);
#define GUILD_TEXT 0
#define DM_CHANNEL 1
#define GUILD_VOICE 2
#define GROUPCHAT_CHANNEL 3
#define GUILD_CATEGORY 4
#define GUILD_ANNOUNCEMENT 5
int DiscordGetChannelHistory(const char* channelID, unsigned int amount, DiscordMessage** msgs);
char *DiscordFetchTmpPfp(char *userID, char *hash);
char *DiscordFetchTmpGuildIcon(char *guildID, char *hash);
int DiscordListPrivateChannels(DiscordChannel** out);
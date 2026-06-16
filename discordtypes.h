#pragma once
typedef struct{
    char* Username;
    char* DisplayName;
    char* id;
	char *avatar;
} DiscordUser;
typedef struct {
    char *channelID;
    DiscordUser author;
    char* content;
    char* id;
    int type;
} DiscordMessage;
typedef struct {
	int MentionCount;
} ReadState;
typedef struct {
    char            *id;
    char            *name;
    char            *parentID;      /* category ID this channel belongs to, NULL if none */
    char            *topic;         /* channel topic/description, NULL if unset */
    char            *lastMessageID; /* snowflake of last message, NULL if none */
    int type;
    int              position;      /* sort order within the guild/category */
	int flags;
	int receipentCount;
	DiscordUser* receipents;
	ReadState ReadState;
	char* GuildID;
} DiscordChannel;
typedef struct {
    char *id;
    char *name;
    char *IconHash;
    char *ObtainedIconPath;
    DiscordChannel* channels;
	int ChannelCount;
	int MentionCount;
} DiscordGuild;
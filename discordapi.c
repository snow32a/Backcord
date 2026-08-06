#include "discordtypes.h"
#include "http.h"
#include "config.h"
#include <openssl/core.h>
#include <windows.h>
#include <winnt.h>
#include <winternl.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "ws.h"
#include "globals.h"
#include "hmap/hashmap.h"
hmap GuildsTable = NULL;
hmap ChannelsTable = NULL;
DiscordChannel *PrivateChannels = NULL;
HTTPConnection* maincon;
HTTPConnection* cdncon;
int PrivateChannelCount = 0;
static uint64_t GuildHash(const void *item, uint64_t seed0, uint64_t seed1) {
	const DiscordGuild *e = item;
	return hashmap_sip(e->id, strlen(e->id), seed0, seed1);
}
static int GuildCompare(const void *a, const void *b, void *udata) {
	return strcmp(((DiscordGuild *)a)->id, ((DiscordGuild *)b)->id);
}
static void GuildFree(void *item) {}

static uint64_t ChannelHash(const void *item, uint64_t seed0, uint64_t seed1) {
	const DiscordChannel *e = item;
	return hashmap_sip(e->id, strlen(e->id), seed0, seed1);
}
static int ChannelCompare(const void *a, const void *b, void *udata) {
	return strcmp(((DiscordChannel *)a)->id, ((DiscordChannel *)b)->id);
}
static void ChannelFree(void *item) {}
extern void onDiscordGuildLoad(DiscordGuild *guild, char *id);
extern void onDiscordUpdatedGuildReadState(DiscordGuild gld);
extern void onDiscordReady(DiscordUser user, DiscordGuild *guilds, int guildscount);
int DiscordHTTPConnect(){
	maincon=HTTPConnect("discord.com");
	cdncon=HTTPConnect("cdn.discordapp.com");
	return maincon->connected && cdncon->connected;
}
void DiscordHTTPClose(){
	CloseHTTPConnection(maincon);
	CloseHTTPConnection(cdncon);
}
int DiscordSendMessage(const char *channelID, const char *content) {
	cJSON *payload = cJSON_CreateObject();
	cJSON *name = cJSON_CreateString(content);
	cJSON_AddItemToObject(payload, "content", name);
	char *endpoint = malloc(28 + strlen(channelID));
	endpoint[0] = '\0';
	strcat(endpoint, "/api/v9/channels/");
	strcat(endpoint, channelID);
	strcat(endpoint, "/messages");
	char *forNothingGng;
	char *payld = cJSON_Print(payload);
	long resplen;
	char headers[17 + strlen(token) + 1];
	wsprintf(headers, "Authorization: %s", token);
	SendHTTPRequest(maincon,"POST", endpoint, headers, user_agent,
					 "application/json", payld, strlen(payld), &forNothingGng,
					 &resplen);
	free(payld);
	cJSON_free(payload);
	free(forNothingGng);
}
int DiscordListGuildChannels(char *guildID, DiscordChannel **out) {
	DiscordGuild *gld;
	DiscordGuild search;
	search.id = guildID;
	gld = hashmap_get(GuildsTable, &search);
	DiscordChannel *arr1 = (RtlAllocateHeap(
		GetProcessHeap(), 0, sizeof(DiscordChannel) * gld->ChannelCount));
	memset(arr1, 0, sizeof(DiscordChannel) * gld->ChannelCount);
	*out = arr1;
	for (int i = 0; i < gld->ChannelCount; i++) {
		(*out)[i] = gld->channels[i];
	}
	return gld->ChannelCount;
}
char *DiscordFetchTmpPfp(char *userID, char *hash) {
	char *endpoint = malloc(80);
	wsprintfA(endpoint, "/avatars/%s/%s.png?size=512", userID, hash);
	char *HTTPResp;
	long resplen;
	char headers[17 + strlen(token) + 1];
	wsprintf(headers, "Authorization: %s", token);
	SendHTTPRequest(cdncon, "GET", endpoint, headers, user_agent,
					 "application/json", NULL, 0, &HTTPResp, &resplen);
	free(endpoint);

	char tempPath[MAX_PATH];

	DWORD len = GetTempPathA(MAX_PATH, tempPath);
	char *path = malloc(MAX_PATH + strlen(hash) + 1);
	wsprintfA(path, "%s%s.png", tempPath, hash);
	HANDLE file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
							  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD written;
	if (file == INVALID_HANDLE_VALUE) {
		MessageBoxA(NULL,
					"oof, failed opening the file to save the tmp pfp file!",
					"Backcord - DiscordFetchTmpPfp", 0);
		free(path);
		return NULL;
	}
	WriteFile(file, HTTPResp, resplen, &written, NULL);
	CloseHandle(file);
	free(HTTPResp);
	return path;
}
char *DiscordFetchTmpGuildIcon(char *guildID, char *hash) {
	char *endpoint = malloc(80);
	wsprintfA(endpoint, "/icons/%s/%s.png?size=512", guildID, hash);
	char *HTTPResp;
	long resplen;
	char headers[17 + strlen(token) + 1];
	wsprintf(headers, "Authorization: %s", token);
	SendHTTPRequest(cdncon, "GET", endpoint, headers, user_agent,
					 "application/json", NULL, 0, &HTTPResp, &resplen);
	free(endpoint);

	char tempPath[MAX_PATH];

	DWORD len = GetTempPathA(MAX_PATH, tempPath);
	char *path = malloc(MAX_PATH + strlen(hash) + 1);
	wsprintfA(path, "%s%s.png", tempPath, hash);
	HANDLE file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
							  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD written;
	if (file == INVALID_HANDLE_VALUE) {
		MessageBoxA(NULL,
					"oof, failed opening the file to save the tmp icon file!",
					"Backcord - Guild Icon Retrieval", 0);
		free(path);
		return NULL;
	}
	WriteFile(file, HTTPResp, resplen, &written, NULL);
	CloseHandle(file);
	free(HTTPResp);
	return path;
}
int DiscordGetChannelHistory(const char *channelID, unsigned int amount,
							 DiscordMessage **msgs) {
	cJSON *payload = cJSON_CreateObject();
	char *endpoint = malloc(38 + strlen(channelID));
	memset((endpoint), 0, (27 + strlen(channelID)));
	strcat(endpoint, "/api/v9/channels/");
	strcat(endpoint, channelID);
	strcat(endpoint, "/messages?limit=");
	char *amountstr = malloc(3 * sizeof(char));
	wsprintf(amountstr, "%i", amount);
	strcat(endpoint, amountstr);
	free(amountstr);
	char *HTTPResp;
	long resplen;

	char headers[17 + strlen(token) + 1];
	wsprintf(headers, "Authorization: %s", token);

	SendHTTPRequest(maincon, "GET", endpoint, headers, user_agent,
					 "application/json", NULL, 0, &HTTPResp, &resplen);
	cJSON *resp = cJSON_Parse(HTTPResp);
	if (!resp)
		return 0;
	if (!cJSON_IsArray(resp))
		return 0;
	*msgs = malloc(sizeof(DiscordMessage) * amount);
	for (int i = 0; i < cJSON_GetArraySize(resp); i++) {
		cJSON *item = cJSON_GetArrayItem(resp, i);

		(*msgs)[i].content = cJSON_GetObjectItem(item, "content")->valuestring;
		(*msgs)[i].channelID =
			cJSON_GetObjectItem(item, "channel_id")->valuestring;
		(*msgs)[i].id = cJSON_GetObjectItem(item, "id")->valuestring;
		(*msgs)[i].type = cJSON_GetObjectItem(item, "type")->valueint;
		cJSON *author = cJSON_GetObjectItem(item, "author");
		cJSON *dispname = cJSON_GetObjectItem(author, "global_name");
		(*msgs)[i].author.DisplayName = (dispname && cJSON_IsString(dispname))
											? strdup(dispname->valuestring)
											: NULL;

		cJSON *av = cJSON_GetObjectItem(author, "avatar");
		(*msgs)[i].author.avatar =
			(av && cJSON_IsString(av)) ? strdup(av->valuestring) : NULL;

		cJSON *ct = cJSON_GetObjectItem(item, "content");
		(*msgs)[i].content =
			(ct && cJSON_IsString(ct)) ? strdup(ct->valuestring) : strdup("");
		(*msgs)[i].author.Username =
			cJSON_GetObjectItem(author, "username")->valuestring;
		(*msgs)[i].author.DisplayName =
			cJSON_GetObjectItem(author, "global_name")->valuestring;
		(*msgs)[i].author.id = cJSON_GetObjectItem(author, "id")->valuestring;
		(*msgs)[i].author.avatar =
			cJSON_GetObjectItem(author, "avatar")->valuestring;
	}
	return cJSON_GetArraySize(resp);
	free(HTTPResp);
}
int ConnectGateway(const char *channelID, const char *content) {}
DiscordGuild *GetGuild(char *id) {
	DiscordGuild *g;
	DiscordGuild search;
	search.id = id;
	g = (DiscordGuild*)hashmap_get(GuildsTable, &search);
	return g;
}
int DiscordListPrivateChannels(DiscordChannel **out) {
	*out = PrivateChannels;
	return PrivateChannelCount;
}
void HandleREADY(cJSON *json) {
	cJSON *data = cJSON_GetObjectItem(json, "d");
	if (!cJSON_IsObject(data))
		return;

	cJSON *guilds = cJSON_GetObjectItem(data, "guilds");
	if (!cJSON_IsArray(guilds))
		return;

	int size = cJSON_GetArraySize(guilds);
	GuildsTable = hashmap_new(sizeof(DiscordGuild), 256, 0, 0, GuildHash,
							  GuildCompare, GuildFree, NULL);
	ChannelsTable = hashmap_new(sizeof(DiscordChannel), 256, 0, 0, ChannelHash,
							  ChannelCompare, ChannelFree, NULL);

	cJSON *privchannels = cJSON_GetObjectItem(data, "private_channels");
	PrivateChannelCount = cJSON_GetArraySize(privchannels);
	PrivateChannels = malloc(sizeof(DiscordChannel) * PrivateChannelCount);
	for (int j = 0; j < PrivateChannelCount; j++) {
		cJSON *ch = cJSON_GetArrayItem(privchannels, j);
		if (!cJSON_IsObject(ch))
			continue;

		cJSON *chName = cJSON_GetObjectItem(ch, "name");
		cJSON *chType = cJSON_GetObjectItem(ch, "type");
		cJSON *chId = cJSON_GetObjectItem(ch, "id");
		cJSON *chPid = cJSON_GetObjectItem(ch, "parent_id");
		cJSON *chRecp = cJSON_GetObjectItem(ch, "recipients");

		if (!cJSON_IsString(chId) || !cJSON_IsNumber(chType))
			continue;
			
		PrivateChannels[j].GuildID = NULL;
		PrivateChannels[j].name = chName ? strdup(chName->valuestring) : NULL;
		PrivateChannels[j].id = strdup(chId->valuestring);
		PrivateChannels[j].type = chType->valueint;
		PrivateChannels[j].parentID = (chPid && cJSON_IsString(chPid))
										  ? strdup(chPid->valuestring)
										  : NULL;
		PrivateChannels[j].receipents =
			malloc(cJSON_GetArraySize(chRecp) * sizeof(DiscordUser));
		for (int i = 0; i < cJSON_GetArraySize(chRecp); i++) {
			cJSON *recep = cJSON_GetArrayItem(chRecp, i);
			cJSON *recpAv = cJSON_GetObjectItem(recep, "avatar");
			PrivateChannels[j].receipents[i].avatar =
				(recpAv && cJSON_IsString(recpAv)) ? strdup(recpAv->valuestring)
												   : NULL;

			cJSON *recpUn = cJSON_GetObjectItem(recep, "username");
			PrivateChannels[j].receipents[i].Username =
				(recpUn && cJSON_IsString(recpUn)) ? strdup(recpUn->valuestring)
												   : NULL;

			cJSON *recpDn = cJSON_GetObjectItem(recep, "global_name");
			PrivateChannels[j].receipents[i].DisplayName =
				(recpDn && cJSON_IsString(recpDn)) ? strdup(recpDn->valuestring)
												   : NULL;

			cJSON *recpId = cJSON_GetObjectItem(recep, "id");
			PrivateChannels[j].receipents[i].id =
				(recpId && cJSON_IsString(recpId)) ? strdup(recpId->valuestring)
												   : NULL;
		}
	}

	for (int i = size - 1; i >= 0; i--) {
		cJSON *guild = cJSON_GetArrayItem(guilds, i);
		if (!cJSON_IsObject(guild))
			continue;

		cJSON *channels = cJSON_GetObjectItem(guild, "channels");
		int channelCount = cJSON_GetArraySize(channels);
		DiscordChannel *gchannels =
			malloc(sizeof(DiscordChannel) * channelCount);
		for (int j = 0; j < channelCount; j++) {
			cJSON *ch = cJSON_GetArrayItem(channels, j);
			if (!cJSON_IsObject(ch))
				continue;

			cJSON *chName = cJSON_GetObjectItem(ch, "name");
			cJSON *chType = cJSON_GetObjectItem(ch, "type");
			cJSON *chId = cJSON_GetObjectItem(ch, "id");
			cJSON *chPid = cJSON_GetObjectItem(ch, "parent_id");

			if (!cJSON_IsString(chName) || !cJSON_IsString(chId) ||
				!cJSON_IsNumber(chType))
				continue;

			gchannels[j].name = strdup(chName->valuestring);
			gchannels[j].id = strdup(chId->valuestring);
			gchannels[j].type = chType->valueint;
			gchannels[j].parentID = (chPid && cJSON_IsString(chPid))
										? strdup(chPid->valuestring)
										: NULL;
			gchannels[j].GuildID =
			strdup(cJSON_GetObjectItem(guild, "id")->valuestring);
			hashmap_set(ChannelsTable,&gchannels[j]);
		}
		char *name = cJSON_GetObjectItem(guild, "name")->valuestring;
		char *id = cJSON_GetObjectItem(guild, "id")->valuestring;
		DiscordGuild *g = malloc(sizeof(DiscordGuild));
		g->id = strdup(id);
		g->name = strdup(name);
		cJSON *icon = cJSON_GetObjectItem(guild, "icon");
		g->IconHash =
			(icon && cJSON_IsString(icon)) ? strdup(icon->valuestring) : NULL;
		g->ObtainedIconPath = NULL;
		g->channels = gchannels;
		g->ChannelCount = channelCount;
		g->MentionCount = 0;
		hashmap_set(GuildsTable, g);

		onDiscordGuildLoad(g, id);
	}
	cJSON *readstates = cJSON_GetObjectItem(data, "read_state");
	for (int i = 0; i < cJSON_GetArraySize(readstates); i++) {
		cJSON *readstate = cJSON_GetArrayItem(readstates, i);
			DiscordChannel chnprop;
			chnprop.id = cJSON_GetObjectItem(readstate, "id")->valuestring;
			DiscordChannel *che =
				(DiscordChannel *)hashmap_get(ChannelsTable, &chnprop);
			if (!che) {
				// bradar WHAT IS THIS
				printf("NOT FOUND: Channel %s\n", chnprop.id);
				continue;
			}
			che->ReadState.MentionCount =
				cJSON_GetObjectItem(readstate, "mention_count")
					? cJSON_GetObjectItem(readstate, "mention_count")->valueint
					: 0;
			if (cJSON_GetObjectItem(readstate, "mention_count")
					? cJSON_GetObjectItem(readstate, "mention_count")->valueint
					: 0) {
				if (che->GuildID) {
					DiscordGuild *gld = GetGuild(che->GuildID);
					if (gld) {
					MessageBoxA(NULL, gld->name, "AAAAAAAAAA", 0);
						gld->MentionCount += che->ReadState.MentionCount;
						onDiscordUpdatedGuildReadState(*gld);
					}
				}
			}
	}
	cJSON *jsonusr = cJSON_GetObjectItem(data, "user");
	DiscordUser curuser = {0};
	curuser.Username = cJSON_GetObjectItem(jsonusr, "username")->valuestring;
	cJSON *jsondispname = cJSON_GetObjectItem(jsonusr, "global_name");
	curuser.DisplayName = jsondispname ? jsondispname->valuestring : NULL;
	cJSON *jsonavatar = cJSON_GetObjectItem(jsonusr, "avatar");
	curuser.avatar = jsonavatar ? jsonavatar->valuestring : NULL;
	curuser.id = cJSON_GetObjectItem(jsonusr, "id")->valuestring;
	onDiscordReady(curuser,NULL,0);
}
extern void onDiscordReceiveMessage(DiscordMessage msg);
void HandleMessageCreate(cJSON *json) {
	cJSON *d = cJSON_GetObjectItem(json, "d");
	cJSON *author = cJSON_GetObjectItem(d, "author");
	DiscordMessage msg;
	msg.author.DisplayName =
		strdup(cJSON_GetObjectItem(author, "global_name")->valuestring);
	msg.author.id = strdup(cJSON_GetObjectItem(author, "id")->valuestring);
	msg.channelID = strdup(cJSON_GetObjectItem(d, "channel_id")->valuestring);
	msg.content = strdup(cJSON_GetObjectItem(d, "content")->valuestring);
	if (cJSON_GetObjectItem(author, "avatar")) {
		msg.author.avatar = cJSON_GetObjectItem(author, "avatar")->valuestring;
	}
	onDiscordReceiveMessage(msg);
}
void HandleDiscordDispatch(cJSON *json) {
	if (!cJSON_IsObject(json))
		return;
	cJSON *t = cJSON_GetObjectItem(json, "t");
	if (!t || !cJSON_IsString(t))
		return;
	if (strcmp(t->valuestring, "READY") == 0) {
		HandleREADY(json);
	}
	if (strcmp(t->valuestring, "MESSAGE_CREATE") == 0) {
		HandleMessageCreate(json);
	}
}

void CALLBACK HeartbeatProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
							DWORD dwTime) {
	SendWebSocket(GatewaySSL, "{\"op\": 1,\"d\": null}", 19, 0x81);
}
void WebSocketOnDataArrival(SSL *ssl, char *buffer, size_t length) {
	cJSON *json = cJSON_Parse(buffer);
	if (json == NULL) {
		if (strstr(buffer, "Authentication failed") != 0) {
			MessageBoxA(NULL,
						"The provided token may be outdated, there was an "
						"issue in authentication!",
						"Backcord - Gateway Error", MB_ICONERROR);
		}
		const char *error = cJSON_GetErrorPtr();
		if (error) {
			printf("Parse error before: %s\n", error);
			return;
		}
	}

	int opcode = (cJSON_GetObjectItem(json, "op"))->valueint;
	cJSON *data = (cJSON_GetObjectItem(json, "d"));

	if (opcode == 10) {
		// received hello.. discord api wants us to send a heartbeat rn and
		// identify

		SendWebSocket(ssl, "{\"op\": 1,\"d\": null}", 19, FIN_LAST | WS_TEXT);
		char identifybuf[650 + strlen(token)];
		wsprintf(
			identifybuf,
			"{\"op\":2,\"d\":{\"token\":\"%s\",\"properties\":{\"os\":"
			"\"Linux\",\"browser\":\"Chrome\",\"device\":\"\",\"has_client_"
			"mods\":false,\"browser_user_agent\":\"Mozilla/5.0 (Linux; Android "
			"6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) "
			"Chrome/145.0.0.0 Mobile Safari/537.36\",\"release_channel\": "
			"\"stable\"},\"presence\":{},\"compress\":false,"
			"\"client_state\":{\"guild_versions\":{}}}}",
			token);
		SendWebSocket(ssl, identifybuf, strlen(identifybuf), 0x81);;
		SetTimer(NULL, 0,
				 cJSON_GetObjectItem(data, "heartbeat_interval")->valueint * 4 /
					 5,
				 HeartbeatProc);

	} else if (opcode == 11) {
		// heartbeat ack

	} else if (opcode == 1) {
		// discords heart fluttered :wilted_rose:
		SendWebSocket(GatewaySSL, "{\"op\": 1,\"d\": null}", 19, 0x81);
	} else if (opcode == 0) {
		HandleDiscordDispatch(json);
	} else if (opcode == 7) {
		MessageBoxA(NULL, "reconnect", "discordapi.c", 0);
	}
	cJSON_Delete(json);
}

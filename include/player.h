#ifndef PLAYER_H
#define PLAYER_H

void coglink_playSong(struct lavaInfo lavaInfo, char *track, u64snowflake guildId);

void coglink_stopPlayer(struct lavaInfo lavaInfo, u64snowflake guildID);

void coglink_pausePlayer(struct lavaInfo lavaInfo, u64snowflake guildID, char *pause);

void coglink_seekTrack(struct lavaInfo lavaInfo, u64snowflake guildID, char *position);

void coglink_setPlayerVolume(struct lavaInfo lavaInfo, u64snowflake guildID, char *volume);

void coglink_destroyPlayer(struct lavaInfo lavaInfo, u64snowflake guildID);

#endif
#include <Core/Core.h>
#include "Overwatch_MemoryCrude.h"
/*
	Memory crud is kind of Philosophy 
	where the database is stored in memory at programe starting.
	Then when someone want read data supposed stored in data base,
	He will just read the memory copy without accessing data base.
	However if someone want write, then he will write the dataBase then
	the data will be write in memory.
*/
using namespace Upp;

void   Player::SetPlayerId(int _playerID){if(IsValide())player_ID = _playerID;}
int    Player::GetPlayerId(){if(IsValide())return player_ID;return -1;}
void   Player::SetBattleTag(String _BattTetag){if(IsValide())battle_tag =_BattTetag;}
String Player::GetBattleTag(){if(IsValide())return battle_tag;return "";}
void   Player::SetDiscordId(String _DiscordId){if(IsValide())discord_id=_DiscordId;}
String Player::GetDiscordId(){if(IsValide())return discord_id;return "";}
void   Player::SetCommunName(String _CommunName){if(IsValide())commun_name=_CommunName;}
String Player::GetCommunName(){if(IsValide())return commun_name;return "";}

String Player::GetPersonneDiscordId(){if(IsValide())return String() << "<@!" << player_ID << ">";return "";}
bool Player::IsValide(){return (player_ID!=-1)? true:false;}

Player::Player(int _playerId,String _battleTag,String _discordId,String _communName){
	if(_playerId > 0 && _battleTag.GetCount() > 0 && _discordId.GetCount() > 0){
		player_ID=_playerId;
		battle_tag=_battleTag;
		discord_id=_discordId;
		commun_name=_communName;
	}
}

///////////////////////////////////////////////////////////////////

void   Equipe::SetEquipeId(int _equipeId){equipe_id = _equipeId;}
int    Equipe::GetEquipeId(){return equipe_id;}
void   Equipe::SetEquipeName(String equipeName){equipe_name =equipeName;}
String Equipe::GetEquipeName(){return equipe_name;}
bool   Equipe::IsValide(){return (equipe_id!=-1)? true:false;}	

Equipe::Equipe(int _equipeId,String _equipeName){
	if(_equipeId> 0 && _equipeName.GetCount()>0){
		equipe_id=_equipeId;
		equipe_name=_equipeName;
	}
}
/////////////////////////////////////////////////////////////////
int EquipeCreator::GetPlayerID(){return playerId;}
bool     EquipeCreator::IsAdmin(){return (playerId!=-1)? true:false;}

EquipeCreator::EquipeCreator(int _playerId, bool _admin){
	playerId=_playerId;
	isAdmin = _admin;
}
/////////////////////////////////////////////////////////////////

int   PlayerData::GetDataId(){return data_id;}
void  PlayerData::SetDataId(int _dataId){data_id = _dataId;}
Date& PlayerData::GetRetrieveDate(){return retrieve_date;}
void  PlayerData::SetRetrieveDate(Date _date){retrieve_date=_date;}
int   PlayerData::GetGamesPlayed(){return games_played;}
void  PlayerData::SetGamesPlayed(int gameP){games_played = gameP;}
int   PlayerData::GetLevel(){return level;}
void  PlayerData::SetLevel(int _level){level = _level;}
int   PlayerData::GetRating(){return rating;}
void  PlayerData::SetRating(int rate){rating = rate;}
int   PlayerData::GetMedalsCount(){return medals_count;}
void  PlayerData::SetMedalsCount(int medalsC){medals_count = medalsC;}
int   PlayerData::GetMedalsB(){return medals_bronze;}
void  PlayerData::SetMedalsB(int medalB){medals_bronze = medalB;}
int   PlayerData::GetMedalsS(){return medals_silver;}
void  PlayerData::SetMedalsS(int medalS){medals_silver = medalS;}
int   PlayerData::GetMedalsG(){return medals_gold;}
void  PlayerData::SetMedalsG(int medalG){medals_gold = medalG;}
bool  PlayerData::IsValide(){return (data_id!=-1)? true:false;}

PlayerData::PlayerData(int _id,Date _date,int _gamesPlayed,int _level,int _rating,int _medalsC,int _medalsB,int _medalsS,int _medalsG){
	if(_id>0){
		data_id=_id;
		retrieve_date= _date;
		games_played=_gamesPlayed;
		level=_level;
		rating=_rating;
		medals_count=_medalsC;
		medals_bronze=_medalsB;
		medals_silver=_medalsS;
		medals_gold=-_medalsG;
	}
}

PlayerData::PlayerData(int _id,int _gamesPlayed,int _level,int _rating,int _medalsC,int _medalsB,int _medalsS,int _medalsG){
	if(_id>0){
		data_id=_id;
		retrieve_date=GetSysDate();
		games_played=_gamesPlayed;
		level=_level;
		rating=_rating;
		medals_count=_medalsC;
		medals_bronze=_medalsB;
		medals_silver=_medalsS;
		medals_gold=-_medalsG;
	}
}	
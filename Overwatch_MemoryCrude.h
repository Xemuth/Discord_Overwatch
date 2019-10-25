#ifndef _Discord_Overwatch_Overwatch_MemoryCrude_h_
#define _Discord_Overwatch_Overwatch_MemoryCrude_h_
#include <Core/Core.h>

using namespace Upp;
/*
	Memory crud is kind of Philosophy 
	where the database is stored in memory at programe starting.
	Then when someone want read data supposed stored in data base,
	He will just read the memory copy without accessing data base.
	However if someone want write, then he will write the dataBase then
	the data will be write in memory.
*/
class Player;
class Equipe;
class EquipeCreator;
class PlayerData;

class Player: Moveable<Player>{
	private:
		int player_ID=-1;
		String battle_tag="";
		String discord_id="";
		String commun_name="";
		
	public:
		Vector<PlayerData> datas; // All The player DATA
		
		void SetPlayerId(int _playerID);
		int GetPlayerId();
		void SetBattleTag(String _BattTetag);
		String GetBattleTag();
		void SetDiscordId(String _DiscordId);
		String GetDiscordId();
		void SetCommunName(String _CommunName);
		String GetCommunName();
		String GetPersonneDiscordId();
		bool IsValide();
		
		Player(int _playerId,String _battleTag,String _discordId,String _communName="");
};

class Equipe: Moveable<Equipe>{
	private:
		int equipe_id=-1;
		String equipe_name="";

	public:
		Vector<int> playersId;  //players represent all ppl in equipe
		Vector<EquipeCreator> creators; //Creators represent all ppl having right on the equipes
		
		void SetEquipeId(int _equipeId);
		int GetEquipeId();
		void SetEquipeName(String equipeName);
		String GetEquipeName();
		bool IsValide();	
		
		Equipe(int _equipeId,String _equipeName);
};

class EquipeCreator: Moveable<EquipeCreator> {
	private:
		int playerId=-1;
		bool isAdmin=false;
	public:
		int GetPlayerID();
		bool IsAdmin();
		
		EquipeCreator(int _playerId, bool _admin=false);
};

class PlayerData: Moveable<PlayerData> {
	private:
		int data_id=-1;
		Date retrieve_date= Date();
		int games_played=-1;
		int level=-1;
		
		int rating=-1;
		int rating_damage=-1;
		int rating_heal=-1;
		int rating_tank=-1;

		int medals_count=-1;
		int medals_bronze=-1;
		int medals_silver=-1;
		int medals_gold=-1;
	public:
		int GetDataId();
		void SetDataId(int _dataId);
		Date& GetRetrieveDate();
		void SetRetrieveDate(Date _date);
		int GetGamesPlayed();
		void SetGamesPlayed(int gameP);
		int GetLevel();
		void SetLevel(int _level);
		int GetRating();
		void SetRating(int rate);
		
		int GetRatingDamage();
		void SetRatingDamage(int rate);
		int GetRatingHeal();
		void SetRatingHeal(int rate);
		int GetRatingTank();
		void SetRatingTank(int rate);
		
		
		int GetMedalsCount();
		void SetMedalsCount(int medalsC);
		int GetMedalsB();
		void SetMedalsB(int medalB);
		int GetMedalsS();
		void SetMedalsS(int medalS);
		int GetMedalsG();
		void SetMedalsG(int medalG);
		bool IsValide();
		
		PlayerData(int _id,Date _date,int _gamesPlayed=-1,int _level=-1,int _rating=-1,int _rating_damage =-1,int _rating_tank = -1,int _rating_heal =-1,int _medalsC=-1,int _medalsB=-1,int _medalsS=-1,int _medalsG=-1);
		PlayerData(int _id,int _gamesPlayed=-1,int _level=-1,int _rating=-1,int _rating_damage =-1,int _rating_heal =-1,int _rating_tank =-1,int _medalsC=-1,int _medalsB=-1,int _medalsS=-1,int _medalsG=-1);	
};


#endif

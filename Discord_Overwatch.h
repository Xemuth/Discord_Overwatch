#ifndef _Discord_Overwatch_Discord_Overwatch_h_
#define _Discord_Overwatch_Discord_Overwatch_h_
#include <SmartUppBot/SmartBotUpp.h>
#include <GraphBuilder/GraphBuilder.h>
#include "Overwatch_MemoryCrude.h"

#define NOAPPSQL
#include <plugin/sqlite3/Sqlite3.h>

using namespace Upp;

#define SCHEMADIALECT <plugin/sqlite3/Sqlite3Schema.h>

#define MODEL <Discord_Overwatch/Overwatch_DataBase.sch>
#include "Sql/sch_header.h"

/*
In UPP VERSION 13444, the sqlite3 schema definition is not complete :
	
#ifndef REFERENCES_
#define REFERENCES_(n, x)           INLINE_ATTRIBUTE("REFERENCES " #n "(" #x ")")
#endif

#ifndef REFERENCES_CASCADE_
#define REFERENCES_CASCADE_(n, x)   INLINE_ATTRIBUTE("REFERENCES " #n "(" #x ") ON DELETE CASCADE")
#endif

Need to be defined in Sqlite3Schema.h stored in Plugin/sqlite3.

It will allow Foreign key to be create properly
Also, do not forget to activate it at your first connection by using :
	
	Sql sql;
	sql.Execute("PRAGMA foreign_keys = ON;"); //Enable Foreign keys

at the end of Database loading function
*/

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

class Discord_Overwatch: public DiscordModule{
	private:
		void PrepareEvent();
		//Commands
		void CheckApi(ValueMap& payload);//!ow register(battletag:BASTION#21406; pseudo:Clément) // The main idea is " You must be registered to be addable to an equipe or create Equipe
		void Register(ValueMap& payload);//!ow remove 
		void DeleteProfil(ValueMap& payload); //Remove user from the bdd 
		void CreateEquipe(ValueMap& payload); //!ow createEquipe(team:Sombre est mon histoire)//To add an equipe you must be registered. when an equipe is created, only 
		void RemoveEquipe(ValueMap& payload);//!ow removeEquipe(team:Sombre est mon histoire) //you must have the right to remove equipe
		void GiveRight(ValueMap& payload);//!ow GiveRight(discordid:@NattyRoots; team:Sombre est mon histoire) //Allow equipe owner/ equipe righter to add person to the equipe
		void RemoveRight(ValueMap& payload);//!ow RemoveRight(discordid:@NattyRoots;team:Sombre est mon histoire) //Remove equipe righter to one personne By discord ID
		void AddPersonToEquipe(ValueMap& payload);//!ow AddPerson(discordid:@NattyRoots,team:Sombre est mon histoire)//To add a person to an equipe you must have the right to add it
		void RemovePersonFromEquipe(ValueMap& payload); //!ow RemovePerson(discordid:@NattyRoots,team:Sombre est mon histoire)//Remove Person from  equipe (only righter can do it)
		void RemoveMeFromEquipe(ValueMap& payload);//!ow RemoveMeFromEquipe Sombre est mon histoire //Remove u from one of your equipe
		void ForceUpdate(ValueMap& payload);//!ow upd //Force update, based on the personne who make the call
		void ForceEquipeUpdate(ValueMap& payload);//!ow Eupd Sombre est mon histoire //Idk if only ppl who have right on equipe must do it or letting it free.
		#ifdef flagGRAPHBUILDER_DB //Flag must be define to activate all DB func
		void DrawStatsEquipe(ValueMap& payload);//!ow DrawStatsEquipe rating Sombre est mon histoire //Permet de dessiner le graph 
		void saveActualGraph(ValueMap& payload);// saveActualGraph
		void DrawStatsPlayer(ValueMap& payload);
		#else
		void DrawStatsEquipe(ValueMap& payload);//!ow DrawStatsEquipe rating Sombre est mon histoire //Permet de dessiner le graph 
		void DrawStatsPlayer(ValueMap& payload);
		#endif
		void GetCRUD(ValueMap& payload);
		void ReloadCRUD(ValueMap& payload);
		void Help(ValueMap& payload);
		virtual String Credit(ValueMap& json,bool sendCredit = true);
		void startThread(ValueMap& payload);//Used to launch and stop thread used for auto update
		void stopThread(ValueMap& payload);	
		bool GetEtatThread(ValueMap& payload);
		void GraphProperties(ValueMap& payload); //Allow you to define some property of the graph (if call without arg, just send Help)
		//*********

		//READING Memory Func
		
		bool UpdatePlayer(int playerId); //Function to call to update a player
		
		bool bddLoaded = false;
		GraphDotCloud myGraph;
		Sqlite3Session sqlite3; //DataBase
		Thread autoUpdate;
		bool threadStarted =false;
		bool HowManyTimeBeforeUpdate= false;
		Array<Equipe> equipes;
		Array<Player> players;

		enum Critere{GlobalRating, HealRating,TankRating, DpsRating, AutoRating};
		
		bool DoUserHaveRightOnTeam(String TeamName,String userId); // team ID, is here to handle team id
		bool IsRegestered(String Id);
		bool DoEquipeExist(String teamName);
		
		Player* GetPlayersFromId(int Id);
		Player* GetPlayersFromDiscordId(String Id);
		Equipe* GetEquipeById(int ID);
		Equipe* GetEquipeByName(String teamName);
		
		void DeleteEquipeById(int ID);
		bool DeletePlayerById(int ID);
		bool DoUserIsCreatorOfTeam(int TeamId,int PlayerId);
		bool DoUserIsAdminOfTeam(int TeamId,int PlayerId);
		bool DoUserIsStillOnTeam(int TeamId,int PlayerId);
		
		String PrintMemoryCrude();
		//Miscealenous Func
		void LoadMemoryCRUD();
		void prepareOrLoadBDD(); //Used to load BDD
	public:
		Discord_Overwatch(Upp::String _name, Upp::String _prefix);
		Discord_Overwatch(Upp::String _name,Vector<String> _prefix);
		void EventsMessageCreated(ValueMap payload);
};

#endif

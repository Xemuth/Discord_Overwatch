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

class Discord_Overwatch: public DiscordModule{
	private:
		bool bddLoaded = false;
		GraphDotCloud myGraph;
		Sqlite3Session sqlite3; //DataBase

		Array<Equipe> equipes;
		Array<Player> players;

		//Test
		void getStats(ValueMap payload);
		void executeSQL(ValueMap payload);
		void testApi(ValueMap payload);
		
		//!ow register(battletag:BASTION#21406; pseudo:Clément)
		void Register();// The main idea is " You must be registered to be addable to an equipe or create Equipe
		
		//!ow remove 
		void DeleteProfil(); //Remove user from the bdd 
		
		//!ow createEquipe(team:Sombre est mon histoire)
		void CreateEquipe(String teamName); //To add an equipe you must be registered. when an equipe is created, only 
		
		//!ow removeEquipe(team:Sombre est mon histoire)
		void RemoveEquipe(String teamName); //you must have the right to remove equipe
		
		//!ow GiveRight(discordid:@NattyRoots; team:Sombre est mon histoire)
		void GiveRight(String discordID, String teamName); //Allow equipe owner/ equipe righter to add person to the equipe
		
		//!ow RemoveRight(discordid:@NattyRoots;team:Sombre est mon histoire)
		void RemoveRight(String discordID, String teamName); //Remove equipe righter to one personne By discord ID
		
		//!ow AddPerson(discordid:@NattyRoots,team:Sombre est mon histoire)
		void AddPersonToEquipe(String discordID, String teamName); //To add a person to an equipe you must have the right to add it
		
		//!ow RemovePerson(discordid:@NattyRoots,team:Sombre est mon histoire)
		void RemovePersonFromEquipe(String discordID, String teamName); //Remove Person from  equipe (only righter can do it)
		
		//!ow RemoveMeFromEquipe Sombre est mon histoire
		void RemoveMeFromEquipe(String discordID, String teamName); //Remove u from one of your equipe
		
		//!ow upd
		void ForceUpdate(String discordID); //Force update, based on the personne who make the call
		
		//!ow Eupd Sombre est mon histoire
		void ForceEquipeUpdate(String teamName); //Idk if only ppl who have right on equipe must do it or letting it free.
		
		//!ow DrawStatsEquipe rating Sombre est mon histoire
		#ifdef flagGRAPHBUILDER_DB //Flag must be define to activate all DB func
		void DrawStatsEquipe(String teamName); //Permet de dessiner le graph 
		
		// saveActualGraph
		void saveActualGraph(ValueMap payload);
		void DrawStatsPlayer();
		#endif
		#ifndef flagGRAPHBUILDER_DB
		void DrawStatsEquipe(String teamName); //Permet de dessiner le graph 
		void DrawStatsPlayer();
		#endif
		//Force la mise a jour de la personne (elle doit spécifier son elo)
		void updateRating(ValueMap payload);
		
		
		void GraphProperties(ValueMap payload); //Allow you to define some property of the graph (if call without arg, just send Help)
		
		bool UpdatePlayer(int playerId); //Function to call to update a player
		void RetrieveData(); //USed to refresh all team 
		
		//Used to launch and stop thread used for auto update
		Thread autoUpdate;
		bool threadStarted =false;
		bool HowManyTimeBeforeUpdate= false;
		bool GetEtatThread();
		
		void startThread();
		void stopThread();	
	
		//READING Memory Func
		
		void GetCRUD(ValueMap payload);
		void ReloadCRUD(ValueMap payload);
		
		void Help(ValueMap payload);
		
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
		
		virtual String Credit(ValueMap json,bool sendCredit = true);
		Discord_Overwatch(Upp::String _name, Upp::String _prefix);
		void EventsMessageCreated(ValueMap payload);
};

#endif

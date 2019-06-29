#ifndef _Discord_Overwatch_Discord_Overwatch_h_
#define _Discord_Overwatch_Discord_Overwatch_h_
#include <plugin/sqlite3/Sqlite3.h>
#include <SmartUppBot/SmartBotUpp.h>
#include "Overwatch_MemoryCrude.h"
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
		Sqlite3Session sqlite3; //DataBase

		Vector<Equipe> equipes;
		Vector<Player> players;

		//Test
		void getStats(ValueMap payload);
		void executeSQL(ValueMap payload);
		void testApi(ValueMap payload);
		
		//!ow register BASTION#21406 Clément
		void Register(ValueMap payload);// The main idea is " You must be registered to be addable to an equipe or create Equipe
		//!ow remove 
		void DeleteProfil(ValueMap payload); //Remove user from the bdd 
		//!ow createEquipe Sombre est mon histoire
		void CreateEquipe(ValueMap payload); //To add an equipe you must be registered. when an equipe is created, only 
		//!ow removeEquipe Sombre est mon histoire 
		void RemoveEquipe(ValueMap payload); //you must have the right to remove equipe
		//!ow GiveRight @NattyRoots Sombre est mon histoire
		void GiveRight(ValueMap payload); //Allow equipe owner/ equipe righter to add person to the equipe
		//!ow RemoveRight @NattyRoots Sombre est mon histoire
		void RemoveRight(ValueMap payload); //Remove equipe righter to one personne By discord ID
		
		//!ow AddPerson @NattyRoots Sombre est mon histoire
		void AddPersonToEquipe(ValueMap payload); //To add a person to an equipe you must have the right to add it
		//!ow RemovePerson @NattyRoots Sombre est mon histoire
		void RemovePersonToEquipe(ValueMap payload); //Remove Person from and equipe (only righter can do it)
		
		//!ow upd
		void ForceUpdate(ValueMap payload); //Force update, based on the personne who make the call
		//!ow Eupd Sombre est mon histoire
		void ForceEquipeUpdate(ValueMap payload); // Idk if only ppl who have right on equipe must do it or letting it free.
		
		void RetrieveData(); //USed to refresh all team 
		//READING Memory Func
		
		void GetCRUD(ValueMap payload);
		
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
	
		
		String PrintMemoryCrude();
		//Miscealenous Func
		void LoadMemoryCRUD();
		void prepareOrLoadBDD(); //Used to load BDD
	public:
		Discord_Overwatch(Upp::String _name, Upp::String _prefix);
		void EventsMessageCreated(ValueMap payload);
};

#endif

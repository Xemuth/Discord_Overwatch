#ifndef _Discord_Overwatch_Discord_Overwatch_h_
#define _Discord_Overwatch_Discord_Overwatch_h_
#include <plugin/sqlite3/Sqlite3.h>
#include <SmartUppBot/SmartBotUpp.h>
using namespace Upp;

#define SCHEMADIALECT <plugin/sqlite3/Sqlite3Schema.h>
#define MODEL <Discord_Overwatch/Overwatch_DataBase.sch>
#include "Sql/sch_header.h"
#define FREE_DATA(t) SCHEMA(t "\n", NULL)

class Discord_Overwatch: public DiscordModule{
	private:
		bool bddLoaded = false;
		Sqlite3Session sqlite3; //DataBase


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
		//!ow AddRight @NattyRoots Sombre est mon histoire
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
		
		int isTeam(String TeamName,String userId,String& teamId); // team ID, is here to handle team id
		bool isRegestered(String Id);
		void RetrieveData(); //USed to refresh all team 
		void prepareOrLoadBDD(); //Used to load BDD
	public:
		Discord_Overwatch(Upp::String _name, Upp::String _prefix);
		void EventsMessageCreated(ValueMap payload);
};

#endif

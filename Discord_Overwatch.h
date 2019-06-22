#ifndef _Discord_Overwatch_Discord_Overwatch_h_
#define _Discord_Overwatch_Discord_Overwatch_h_
#include <plugin/sqlite3/Sqlite3.h>
#include <SmartUppBot/SmartBotUpp.h>
using namespace Upp;

#define SCHEMADIALECT <plugin/sqlite3/Sqlite3Schema.h>
#define MODEL <Discord_Overwatch/Overwatch_DataBase.sch>
#include "Sql/sch_header.h"


class Discord_Overwatch: public DiscordModule{
	private:
		bool bddLoaded = false;
		Sqlite3Session sqlite3;
	
		
		void getStats(ValueMap payload);
		void executeSQL(ValueMap payload);
		
		void prepareOrLoadBDD(); //Used to load BDD
	public:
		
		Discord_Overwatch(Upp::String _name, Upp::String _prefix);
		void Events(ValueMap payload);
	
	
};

#endif

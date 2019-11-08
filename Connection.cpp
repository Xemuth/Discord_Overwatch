#include "Discord_Overwatch.h"
#include "Connection.h"
#include <Sql/sch_source.h>
#include <Sql/sch_schema.h>

Sql Connection::getConnection(){
	sqlite3.LogErrors(true);
	if(!sqlite3.Open("Overwatch.db")) {
		LOG("Bdd impossible à ouvrir !");
		exit(0);
	}
	SQL = sqlite3;
	SqlSchema sch(SQLITE3);
	All_Tables(sch);
	
	SqlPerformScript(sch.Upgrade());
	SqlPerformScript(sch.Attributes());
	SqlPerformScript(sch.ConfigDrop());
	SqlPerformScript(sch.Config());
	
	sch.SaveNormal();
	
	return sqlite3;
}
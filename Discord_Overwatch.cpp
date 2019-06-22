#include "Discord_Overwatch.h"
#include <Sql/sch_schema.h>
#include <Sql/sch_source.h>

using namespace Upp;


void Discord_Overwatch::getStats(ValueMap payload){
	Upp::String message = payload["d"]["content"];
	if(message.Find("test")!=-1){
		String channel  = payload["d"]["channel_id"];
	    String content  = payload["d"]["content"];
	    String userName = payload["d"]["author"]["username"];
	    String id = payload["d"]["author"]["id"];
	    String discriminator = payload["d"]["author"]["discriminator"];
		
		Cout() << "Event Overwatch" <<"\n";
		ptrBot->CreateMessage(channel, "ow");
	}
	//Ce commentaire permet de tester un git push !
}

void Discord_Overwatch::executeSQL(ValueMap payload){
	String channel  = payload["d"]["channel_id"];
	Upp::String message = payload["d"]["content"];
	message.Replace(String("!" +prefix +" "),"");
	if(message.GetCount() >7 &&  message.Find("ExecSQL")!=-1){
		if(bddLoaded){
			Sql sql;
			message.Replace(String("ExecSQL "),"");
			message+=";";
			if(message.Find("SELECT")!=-1){
				sql.Execute(message);
				int cpt=0;
				Upp::String result = "";
				while(sql.Fetch()){
					cpt = 0;
					while(sql[cpt].IsNull() == false){
						result << sql[cpt].ToString();
						result << " ";
						cpt++;
					}
					result<<"\n";
				}
				if(result.GetCount() == 0) result <<"No values";
				ptrBot->CreateMessage(channel,result);	
			}else{
				sql.Execute(message);
				ptrBot->CreateMessage(channel,sql.ToString());
			}
		}else{
			ptrBot->CreateMessage(channel,"Bdd non chargée");
		}
	}
}
	
Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix){
	name = _name;
	prefix = _prefix;
	
	prepareOrLoadBDD();
	EventsMap.Add([&](ValueMap e){this->getStats(e);});
	EventsMap.Add([&](ValueMap e){this->executeSQL(e);});
}

void Discord_Overwatch::Events(ValueMap payload){
	for(auto &e : EventsMap){
		e(payload);
	}
}


/*

	// Get the list of tables
	Vector<String> table_list = sqlite3.EnumTables("");
	LOG(Format("Tables: (%d)",table_list.GetCount()));
	for (int i = 0; i < table_list.GetCount(); ++i)
	LOG(Format("  #%d: %s",i+1,table_list[i]));
	
	sql*Insert(OW_PLAYERS)(BATTLE_TAG,"BASTION#21406")(DISCORD_ID,"131915014419382272")(COMMUN_NAME,"Clément");
	sql*Insert(OW_EQUIPES)(EQUIPE_NAME,"Sombre est mon histoire");
	sql*Insert(OW_EQUIPES_PLAYERS)(EP_EQUIPE_ID,1)(EP_PLAYER_ID,1);
	sql*Insert(OW_PLAYER_DATA)(DATA_PLAYER_ID,1)(GAMES_PLAYED,1000)(LEVEL,42)(RATING,3000)(MEDALS_COUNT,10)(MEDALS_BRONZE,3)(MEDALS_SILVER,3)(MEDALS_GOLD,4);
		
	S_OW_PLAYERS row;
	sql*Select(row).From(OW_PLAYERS);
	LOG(sql.ToString());
	while (sql.Fetch()) {
		LOG(Format("%d %s %s %d",sql[0],sql[1],sql[2],sql[3]));
		LOG(Format("%s %s %s %s",sql[PLAYER_ID],sql[BATTLE_TAG],sql[DISCORD_ID],sql[COMMUN_NAME]));
	}
*/

void Discord_Overwatch::prepareOrLoadBDD(){
	bddLoaded =false;
	sqlite3.LogErrors(true);
	if(sqlite3.Open("Overwatch_DataBase.db")) {
		SQL = sqlite3;
		#ifdef _DEBUG
			SqlSchema sch(SQLITE3);
			All_Tables(sch);
			if(sch.ScriptChanged(SqlSchema::UPGRADE))
				SqlPerformScript(sch.Upgrade());
			if(sch.ScriptChanged(SqlSchema::ATTRIBUTES)) {	
				SqlPerformScript(sch.Attributes());
			}
			if(sch.ScriptChanged(SqlSchema::CONFIG)) {
				SqlPerformScript(sch.ConfigDrop());
				SqlPerformScript(sch.Config());
			}
			sch.SaveNormal();
		#endif
		bddLoaded =true;
	}
}
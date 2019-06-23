#include "Discord_Overwatch.h"
#include <Sql/sch_schema.h>
#include <Sql/sch_source.h>
#include <iostream>
using namespace Upp;

void Discord_Overwatch::getStats(ValueMap payload){
	Upp::String message = payload["d"]["content"];
	if(message.Find("test")!=-1){
		Cout() << "Event Overwatch" <<"\n";
		ptrBot->CreateMessage(channelLastMessage, "Event ow");
	}
	//Ce commentaire permet de tester un git push !
}

void Discord_Overwatch::executeSQL(ValueMap payload){
	if(Message.GetCount() >7){
		if(bddLoaded){
			Sql sql;
			Message.Replace(String("ExecSQL "),"");
			Message+=";";
			if(Message.Find("SELECT")!=-1){
				sql.Execute(Message);
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
				ptrBot->CreateMessage(channelLastMessage,result);	
			}else{
				sql.Execute(Message);
				ptrBot->CreateMessage(channelLastMessage,sql.ToString());
			}
		}else{
			ptrBot->CreateMessage(channelLastMessage,"Bdd non chargée");
		}
	}
}

void Discord_Overwatch::testApi(ValueMap payload){
	if( MessageArgs.GetCount() == 2 ){
		HttpRequest reqApi;
		MessageArgs[1].Replace("#","-");
		reqApi.Url("https://ow-api.com/v1/stats/pc/euw/" + MessageArgs[1] + "/profile");
		reqApi.GET();
		auto json = ParseJSON(reqApi.Execute());
		ptrBot->CreateMessage(channelLastMessage,json["rating"].ToString());
	}else{
		ptrBot->CreateMessage(channelLastMessage,"Error url");
	}
}

//!ow register BASTION#21406 Clément
void Discord_Overwatch::Register(ValueMap payload){// The main idea is " You must be registered to be addable to an equipe or create Equipe
	if(MessageArgs.GetCount()>2){
		try{
			Sql sql;
			if(MessageArgs.GetCount() == 3){
				sql*Insert(OW_PLAYERS)(BATTLE_TAG,MessageArgs[1])(DISCORD_ID,AuthorId)(COMMUN_NAME,MessageArgs[2]);
				ptrBot->CreateMessage(channelLastMessage,"Enregistrement réussie");
			}else if (MessageArgs.GetCount() == 2){
				sql*Insert(OW_PLAYERS)(BATTLE_TAG,MessageArgs[1])(DISCORD_ID,AuthorId)(COMMUN_NAME,"");
				ptrBot->CreateMessage(channelLastMessage,"Enregistrement réussie");	
			}else
				ptrBot->CreateMessage(channelLastMessage,"Error Registering");
		}catch(...){
			ptrBot->CreateMessage(channelLastMessage,"Error Registering");
		}
	}
}

//!ow remove 
void Discord_Overwatch::DeleteProfil(ValueMap payload){ //Remove user from the bdd 
	if(MessageArgs.GetCount()==1){
		try{
			Sql sql;
			sql*Delete(OW_PLAYERS).Where(DISCORD_ID == AuthorId);
			ptrBot->CreateMessage(channelLastMessage,"Supression reussie");
		}catch(...){
			ptrBot->CreateMessage(channelLastMessage,"Error deletion");
		}
	}
}

//!ow createEquipe Sombre est mon histoire
void Discord_Overwatch::CreateEquipe(ValueMap payload){ //To add an equipe you must be registered. when an equipe is created, only 
	if(MessageArgs.GetCount()>2 && isRegestered(AuthorId)){
		try{
			String teamName="";
			for(int e =1; e < MessageArgs.GetCount(); e++){
				if(e!=1 && e !=  MessageArgs.GetCount()-1)teamName << " ";
				teamName << MessageArgs[e];
			}
			Sql sql;
			if(sql*Insert(OW_EQUIPES)(EQUIPE_NAME,teamName)){
				int idInserted = 0;
				idInserted =std::stoi( sql.GetInsertedId().ToString().ToStd());
				if(idInserted !=0){
					if(sql*Insert(OW_EQUIPES_CREATOR)(EQUIPES_ID,idInserted)(CREATOR_DISCORD_ID,AuthorId)){
						ptrBot->CreateMessage(channelLastMessage,"Ajout d'équipe reussie");
						return;
					}
				}
			}
			ptrBot->CreateMessage(channelLastMessage,"Erreur création team");
		}catch(...){
			ptrBot->CreateMessage(channelLastMessage,"Erreur création team");
		}
	}else{
		ptrBot->CreateMessage(channelLastMessage,"You must be registered Before creating team ! ");
	}
}

//!ow removeEquipe Sombre est mon histoire 
void Discord_Overwatch::RemoveEquipe(ValueMap payload){ //you must have the right to remove equipe
}

//!ow AddRight @NattyRoots Sombre est mon histoire
void Discord_Overwatch::GiveRight(ValueMap payload){ //Allow equipe owner/ equipe righter to add person to the equipe
}

//!ow RemoveRight @NattyRoots Sombre est mon histoire
void Discord_Overwatch::RemoveRight(ValueMap payload){ //Remove equipe righter to one personne By discord ID
}

//!ow AddPerson @NattyRoots Sombre est mon histoire
void Discord_Overwatch::AddPersonToEquipe(ValueMap payload){ //To add a person to an equipe you must have the right to add it
}

//!ow RemovePerson @NattyRoots Sombre est mon histoire
void Discord_Overwatch::RemovePersonToEquipe(ValueMap payload){ //Remove Person from and equipe (only righter can do it)
}

//!ow upd
void Discord_Overwatch::ForceUpdate(ValueMap payload){ //Force update, based on the personne who make the call
}

//!ow Eupd Sombre est mon histoire
void Discord_Overwatch::ForceEquipeUpdate(ValueMap payload){ // Idk if only ppl who have right on equipe must do it or letting it free.
	
}

bool Discord_Overwatch::isRegestered(String Id){
	Sql sql;
	S_OW_PLAYERS row;
	sql*Select(row).From(OW_PLAYERS).Where(DISCORD_ID == Id);
	if(sql.Fetch())return true;
	return false;
}

Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix){
	name = _name;
	prefix = _prefix;
	
	prepareOrLoadBDD();
	EventsMap.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("test"))this->getStats(e);});
	EventsMap.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("execsql"))this->executeSQL(e);});
	EventsMap.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("api"))this->testApi(e);});
	EventsMap.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("register"))this->Register(e);});
	EventsMap.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("remove"))this->DeleteProfil(e);});
	EventsMap.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("createequipe"))this->CreateEquipe(e);});
}

void Discord_Overwatch::Events(ValueMap payload){ //We admit BDD must be loaded to be active
	channelLastMessage =  payload["d"]["channel_id"]; //HEre we hook chan  
	AuthorId = payload["d"]["author"]["id"];
	Message = payload["d"]["content"];
	Message.Replace(String("!" +prefix +" "),"");
	MessageArgs = Split(Message," ");
	MessageArgs[0] = ToLower(MessageArgs[0]);
	for(auto &e : EventsMap){
		if(bddLoaded){
			e(payload);
		}
		else{ 
			ptrBot->CreateMessage(channelLastMessage,"DataBase not loaded !"); 
			break;
		}
	}
}

void Discord_Overwatch::prepareOrLoadBDD(){
	bddLoaded =false;
	sqlite3.LogErrors(true);
	bool mustCreate = false;
	if(!FileExists("Overwatch_DataBase.db"))mustCreate =true;
	if(sqlite3.Open("Overwatch_DataBase.db")) {
		SQL = sqlite3;
		#ifdef _DEBUG
			if(mustCreate){
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
			}
		#endif
		bddLoaded =true;
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


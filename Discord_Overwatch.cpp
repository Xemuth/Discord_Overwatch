#include "Discord_Overwatch.h"
#include <Sql/sch_schema.h>
#include <Sql/sch_source.h>
#include <iostream>
using namespace Upp;

void Discord_Overwatch::getStats(ValueMap payload){
	Upp::String message = payload["d"]["content"];
	if(message.Find("test")!=-1){
		Cout() << "Event Overwatch" <<"\n";
		ptrBot->CreateMessage(ChannelLastMessage, "Event ow");
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
				ptrBot->CreateMessage(ChannelLastMessage,result);	
			}else{
				sql.Execute(Message);
				ptrBot->CreateMessage(ChannelLastMessage,sql.ToString());
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Bdd non chargée");
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
		ptrBot->CreateMessage(ChannelLastMessage,json["rating"].ToString());
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Error url");
	}
}

//!ow register BASTION#21406 Clément
void Discord_Overwatch::Register(ValueMap payload){// The main idea is " You must be registered to be addable to an equipe or create Equipe
	if(MessageArgs.GetCount()>=2){
		try{
			Sql sql;
			if(MessageArgs.GetCount() == 3){
				if(sql*Insert(OW_PLAYERS)(BATTLE_TAG,MessageArgs[1])(DISCORD_ID,AuthorId)(COMMUN_NAME,MessageArgs[2])){
					ptrBot->CreateMessage(ChannelLastMessage,"Enregistrement réussie");
					return;
				}
			}else if (MessageArgs.GetCount() == 2){
				if(sql*Insert(OW_PLAYERS)(BATTLE_TAG,MessageArgs[1])(DISCORD_ID,AuthorId)(COMMUN_NAME,"")){
					ptrBot->CreateMessage(ChannelLastMessage,"Enregistrement réussie");	
					return;
				}
			}
			ptrBot->CreateMessage(ChannelLastMessage,"Error Registering");
		}catch(...){
			ptrBot->CreateMessage(ChannelLastMessage,"Error Registering");
		}
	}
}

//!ow remove 
void Discord_Overwatch::DeleteProfil(ValueMap payload){ //Remove user from the bdd 
	if(MessageArgs.GetCount()==1){
		try{
			Sql sql;
			sql*Delete(OW_PLAYERS).Where(DISCORD_ID == AuthorId);
			ptrBot->CreateMessage(ChannelLastMessage,"Supression reussie");
		}catch(...){
			ptrBot->CreateMessage(ChannelLastMessage,"Error deletion");
		}
	}
}

//!ow createEquipe Sombre est mon histoire
void Discord_Overwatch::CreateEquipe(ValueMap payload){ //To add an equipe you must be registered. when an equipe is created, only 
	if(MessageArgs.GetCount()>2 && isRegestered(AuthorId)){
		try{
			String teamName="";
			for(int e =1; e < MessageArgs.GetCount(); e++){
				if(e!=1 && e !=  MessageArgs.GetCount())teamName << " ";
				teamName << MessageArgs[e];
			}
			Sql sql;
			if(sql*Insert(OW_EQUIPES)(EQUIPE_NAME,teamName)){
				int idInserted = 0;
				idInserted =std::stoi( sql.GetInsertedId().ToString().ToStd());
				if(idInserted !=0){
					if(sql*Insert(OW_EQUIPES_CREATOR)(EC_EQUIPES_ID,idInserted)(EC_CREATOR_DISCORD_ID,AuthorId)){
						ptrBot->CreateMessage(ChannelLastMessage,"Ajout d'équipe reussie");
						return;
					}
				}
			}
			ptrBot->CreateMessage(ChannelLastMessage,"Erreur création team");
		}catch(...){
			ptrBot->CreateMessage(ChannelLastMessage,"Erreur création team");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"You must be registered Before creating team ! ");
	}
}

//!ow removeEquipe Sombre est mon histoire 
void Discord_Overwatch::RemoveEquipe(ValueMap payload){ //you must have the right to remove equipe
if(MessageArgs.GetCount()>=2){
		String teamName="";
		for(int e =1; e < MessageArgs.GetCount(); e++){
			if(e!=1 && e !=  MessageArgs.GetCount())teamName << " ";
			teamName << MessageArgs[e];
		}
		String teamId="";
		int retour = isTeam(teamName, AuthorId, teamId);
		if(retour == 2 && teamId.GetCount() != 0 ){
			Sql sql;
			sql*Delete(OW_EQUIPES).Where(EQUIPE_ID == teamId);
			ptrBot->CreateMessage(ChannelLastMessage,"Done ! ");
		}else if(retour = -1){
			ptrBot->CreateMessage(ChannelLastMessage,"You're not the team leader !");
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
		}
	}
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

int Discord_Overwatch::isTeam(String TeamName,String userId,String &teamId){
	Sql sql;
	sql*Select(EQUIPE_ID).From(OW_EQUIPES).InnerJoin(OW_EQUIPES_CREATOR).On(EQUIPE_ID.Of(OW_EQUIPES) == EC_EQUIPES_ID.Of(OW_EQUIPES_CREATOR) ).Where(EQUIPE_NAME == TeamName && EC_CREATOR_DISCORD_ID == userId);
	LOG(sql.ToString());
	if(	sql.Fetch()){
		teamId = sql[EQUIPE_ID].ToString();
		return 2; // If its ok
	}
	sql*Select(EQUIPE_ID).From(OW_EQUIPES).Where(EQUIPE_NAME == TeamName);
	if(sql.Fetch()){
		return -1; //if user dt have right of team
	}
	return 0; //If team do not exist
}

Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix){
	name = _name;
	prefix = _prefix;
	
	prepareOrLoadBDD();
	
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("test"))this->getStats(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("execsql"))this->executeSQL(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("api"))this->testApi(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("register"))this->Register(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("remove"))this->DeleteProfil(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("createequipe"))this->CreateEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("removeequipe"))this->RemoveEquipe(e);});
}

void Discord_Overwatch::EventsMessageCreated(ValueMap payload){ //We admit BDD must be loaded to be active
	for(auto &e : EventsMapMessageCreated){
		if(bddLoaded){
			e(payload);
		}
		else{ 
			ptrBot->CreateMessage(ChannelLastMessage,"DataBase not loaded !"); 
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
				if(sch.ScriptChanged(SqlSchema::UPGRADE)){
					SqlPerformScript(sch.Upgrade());
				}	
				if(sch.ScriptChanged(SqlSchema::ATTRIBUTES)){	
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


#include "Discord_Overwatch.h"
#include <Sql/sch_schema.h>
#include <Sql/sch_source.h>
#include <iostream>

using namespace Upp;

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


Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix){
	name = _name;
	prefix = _prefix;
	
	prepareOrLoadBDD();
	LoadMemoryCRUD();
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("execsql"))this->executeSQL(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("api"))this->testApi(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("register"))this->Register(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("remove"))this->DeleteProfil(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("createequipe"))this->CreateEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("removeequipe"))this->RemoveEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("giveright"))this->GiveRight(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("removeright"))this->RemoveRight(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("crud"))this->GetCRUD(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs[0].IsEqual("help"))this->Help(e);});
	
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

void Discord_Overwatch::LoadMemoryCRUD(){
	if(bddLoaded){
		Sql sql;
		sql*Select(SqlAll()).From(OW_PLAYERS);
		while(sql.Fetch()){
			players.Add(Player(sql[0].Get<int64>(),sql[1].ToString(),sql[2].ToString(),sql[3].ToString()));	
		}
		for(Player &p : players){
			sql*Select(SqlAll()).From(OW_PLAYER_DATA).Where(DATA_PLAYER_ID == p.GetPlayerId());
			while(sql.Fetch()){
				p.datas.Add(PlayerData(sql[0].Get<int64>() ,sql[1].Get<Date>(),sql[2].Get<int64>(),sql[3].Get<int64>(),sql[4].Get<int64>(),sql[5].Get<int64>(),sql[6].Get<int64>(),sql[7].Get<int64>(),sql[8].Get<int64>()));
			}
		}
		sql*Select(SqlAll()).From(OW_EQUIPES);
		while(sql.Fetch()){
			equipes.Add(Equipe(sql[0].Get<int64>(),sql[1].ToString()));	
		}
		for(Equipe &eq : equipes){
			sql*Select(SqlAll()).From(OW_EQUIPES_CREATOR).Where(EC_EQUIPES_ID == eq.GetEquipeId());
			while(sql.Fetch()){
				bool response =  (sql[2].ToString().IsEqual("1"))? true:false;
				eq.creators.Add(EquipeCreator(GetPlayersFromId(sql[1].Get<int64>())->GetPlayerId(),response));
			}
			
			sql*Select(SqlAll()).From(OW_EQUIPES_PLAYERS).Where(EP_EQUIPE_ID == eq.GetEquipeId());
			while(sql.Fetch()){
				eq.playersId.Add(sql[1].Get<int64>());
			}		
		}
		Cout()<<PrintMemoryCrude()<<"\n";
	}
}

String Discord_Overwatch::PrintMemoryCrude(){
	String information ="";
	information << "-----MEMORY CRUDE-----\n";
	information << "Number of players : " << players.GetCount() <<"\n";
	information << "Number of Equipes : " << equipes.GetCount() <<"\n";
	information <<"-----Equipes------------\n";
	for(Equipe &eq : equipes){
		information << "    Equipe : " << eq.GetEquipeName() <<"\n";
		information << "           ID = " << eq.GetEquipeId() <<"\n";
		information << "           ------Players-----" <<"\n";
		for(int &pl : eq.playersId){
			information << "             Players number " << pl<<"\n";
		}
		information << "           ------Creator-----" <<"\n";
		for(EquipeCreator &ec : eq.creators){
			information << "             Players number " << ec.GetPlayerID() <<"\n";
		}
		information <<"    End Equipe-------\n";
	}
	information <<"-------------End Equipes--------------\n";	
	for(Player &pl : players){
		information << "---Players number " << pl.GetPlayerId() <<"\n";
		information << "      Commun Name : " << pl.GetCommunName() <<"\n";
		information << "      Battle Tag : " << pl.GetBattleTag() <<"\n";
		information << "      Discord ID : " << pl.GetDiscordId() <<"\n";
		information << "      ------Player data------\n";
		for(PlayerData &pd : pl.datas){
			information << "      Data "<< pd.GetDataId() << " Date : "<<  Format(pd.GetRetrieveDate()) << " Number of games played : " << pd.GetGamesPlayed() << " Level : " << pd.GetLevel() << " Rank : " << pd.GetRating() <<"\n";;
		}
	}
	return information;
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
			Sql sql;
			sql.Execute("PRAGMA foreign_keys = ON;"); //Enable Foreign keys
		#endif
		bddLoaded =true;
	}
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

void Discord_Overwatch::GetCRUD(ValueMap payload){
	ptrBot->CreateMessage(ChannelLastMessage, PrintMemoryCrude() );	
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
					players.Add(Player(sql.GetInsertedId().Get<int64>(),MessageArgs[1],AuthorId,MessageArgs[2] ));
					ptrBot->CreateMessage(ChannelLastMessage,"Enregistrement réussie");
					return;
				}
			}else if (MessageArgs.GetCount() == 2){
				if(sql*Insert(OW_PLAYERS)(BATTLE_TAG,MessageArgs[1])(DISCORD_ID,AuthorId)(COMMUN_NAME,"")){
					players.Add(Player(sql.GetInsertedId().Get<int64>(),MessageArgs[1],""));
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
			int id = GetPlayersFromDiscordId(AuthorId)->GetPlayerId();
			if(DeletePlayerById(id)){
				Sql sql;
				if(sql*Delete(OW_PLAYERS).Where(DISCORD_ID == id)){
					ptrBot->CreateMessage(ChannelLastMessage,"Supression reussie");
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Erreur SQL");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Impossible de vous supprimer !  Vous êtes leader d'une équipe");	
			}
		}catch(...){
			ptrBot->CreateMessage(ChannelLastMessage,"Error deletion");
		}
	}
}

//!ow createEquipe Sombre est mon histoire
void Discord_Overwatch::CreateEquipe(ValueMap payload){ //To add an equipe you must be registered. when an equipe is created, only 
	if(MessageArgs.GetCount()>=2 && IsRegestered(AuthorId)){
		try{
			String teamName="";
			for(int e =1; e < MessageArgs.GetCount(); e++){
				if(e!=1 && e !=  MessageArgs.GetCount())teamName << " ";
				teamName << MessageArgs[e];
			}
			Sql sql;
			if(sql*Insert(OW_EQUIPES)(EQUIPE_NAME,teamName)){
				int idInserted = 0;
				idInserted =sql.GetInsertedId().Get<int64>();
				int playerID = GetPlayersFromDiscordId(AuthorId)->GetPlayerId();
				if(idInserted !=0){
					if(sql*Insert(OW_EQUIPES_CREATOR)(EC_EQUIPES_ID,idInserted)(EC_PLAYER_ID,GetPlayersFromDiscordId(AuthorId)->GetPlayerId())(EC_ADMIN,true) && sql*Insert(OW_EQUIPES_PLAYERS)(EP_EQUIPE_ID,idInserted)(EP_PLAYER_ID,playerID)){
						auto& e= 	equipes.Add(Equipe(idInserted,teamName));
					    
						e.creators.Add(EquipeCreator(playerID,true));
						e.playersId.Add(playerID);
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
if(MessageArgs.GetCount()>=2 && IsRegestered(AuthorId)){
		String teamName="";
		for(int e =1; e < MessageArgs.GetCount(); e++){
			if(e!=1 && e !=  MessageArgs.GetCount())teamName << " ";
			teamName << MessageArgs[e];
		}
		if(DoEquipeExist(teamName)){
			if(DoUserHaveRightOnTeam(teamName, AuthorId)){
				Sql sql;
				int equipeId =GetEquipeByName(teamName)->GetEquipeId();
				if(sql*Delete(OW_EQUIPES).Where(EQUIPE_ID ==equipeId)){
					DeleteEquipeById(equipeId);
					ptrBot->CreateMessage(ChannelLastMessage,"L'Equipe à été supprimée !");
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Erreur SQL !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"You're not the team leader !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
		}
	}
}

//!ow GiveRight @NattyRoots Sombre est mon histoire
void Discord_Overwatch::GiveRight(ValueMap payload){ //Allow equipe owner/ equipe righter to add person to the equipe
	if(MessageArgs.GetCount()>=3){
		String idCite = Replace(MessageArgs[1],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
		if( IsRegestered(AuthorId)){
			if(IsRegestered(idCite)){
				String teamName="";
				for(int e =2; e < MessageArgs.GetCount(); e++){
					if(e!=2 && e != MessageArgs.GetCount())teamName << " ";
					teamName << MessageArgs[e];
				}
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
							int id= GetPlayersFromDiscordId(idCite)->GetPlayerId();
							Sql sql;
							if(sql*Insert(OW_EQUIPES_CREATOR)(EC_EQUIPES_ID,GetEquipeByName(teamName)->GetEquipeId())(EC_PLAYER_ID,id )(EC_ADMIN,false ) && sql*Insert(OW_EQUIPES_PLAYERS)(EP_EQUIPE_ID,GetEquipeByName(teamName)->GetEquipeId())(EP_PLAYER_ID,id )){
								auto* equipe = GetEquipeByName(teamName);
								equipe->creators.Add(EquipeCreator(id,false));
								equipe->playersId.Add(id);
								ptrBot->CreateMessage(ChannelLastMessage,"Ajout de  "+ MessageArgs[1] + " dans " + teamName +" !");
								ptrBot->CreateMessage(ChannelLastMessage,"Droits donnés à "+ MessageArgs[1] + "!");
							}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,  MessageArgs[1] + " n'est pas enregistré !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}
}

//!ow RemoveRight @NattyRoots Sombre est mon histoire
void Discord_Overwatch::RemoveRight(ValueMap payload){ //Remove equipe righter to one personne By discord ID
	if(MessageArgs.GetCount()>=3){
		String idCite = Replace(MessageArgs[1],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
		if( IsRegestered(AuthorId)){
			if(IsRegestered(idCite)){
				String teamName="";
				for(int e =2; e < MessageArgs.GetCount(); e++){
					if(e!=2 && e != MessageArgs.GetCount())teamName << " ";
					teamName << MessageArgs[e];
				}
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
							int id= GetPlayersFromDiscordId(idCite)->GetPlayerId();
							int teamId= GetEquipeByName(teamName)->GetEquipeId();
							if(DoUserIsCreatorOfTeam(teamId,id)){
								if(!DoUserIsAdminOfTeam(teamId,id)){
									Sql sql;
									if(sql*Delete(OW_EQUIPES_CREATOR).Where(EC_PLAYER_ID==id && EC_EQUIPES_ID==teamId)){
										bool trouver = false;
										int cpt = 0;
										for(EquipeCreator &ec : GetEquipeById(teamId)->creators){
											if(ec.GetPlayerID()== id){
												trouver =true;
												break;
											}
											cpt++;
										}
										if(trouver){
											GetEquipeById(teamId)->creators.Remove(cpt,1);
											ptrBot->CreateMessage(ChannelLastMessage,"Destitution réussie !");
										}else{
											ptrBot->CreateMessage(ChannelLastMessage,"Erreur CRUD !");
										}
									}else{
										ptrBot->CreateMessage(ChannelLastMessage,"Erreur SQL !");
									}
								}else{
									ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " est Admin de l'équipe !");
								}
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " ne possède pas de droit sur l'équipe !");
							}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,  MessageArgs[1] + " n'est pas enregistré !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}
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

void Discord_Overwatch::Help(ValueMap payload){
	String help ="";
	help << "```Prefixe : !ow\n";
	help << "ExecSQL(String SqlToExecute)" <<" -> Execute du SQL passé en paramètre.\n\n";
	help << "Api(String BattleTag)"<<" -> Récupère le Rank, depuis l'api OverWatch, du BattleTag passé en paramètre.\n\n";
	help << "Register(String BattleTag,String Pseudo)"<<" -> Enregistre votre identité auprès du bot(Necessaire pour toutes les commandes en liaison avec les équipes.\n\n";
	help << "Remove()"<<" -> Efface votre identité auprès du bot #RGPD_Friendly.\n\n";
	help << "CreateEquipe(String EquipeName)"<<" -> Vous permet de créer une équipe(Une equipes contient des players et permet la génération de graph).\n\n";
	help << "RemoveEquipe(String EquipeName)"<<" -> Vous permet de supprimer une équipe(n'importe qui ayant les droits sur l'équipe à le droit de supprimer l'équipe).\n\n";
	help << "GiveRight(String DiscordName,String EquipeName)"<<" -> Vous permet de donner des droits pour administrer votre équipe.\n\n";
	help << "RemoveRight(String DiscordName,String EquipeName)"<<" -> Vous permet de retirer les droits à un des administrateurs de l'équipe(sauf Admin).\n\n";
	help << "Crud()"<<" -> Affiche le memory crud du programme.\n\n";
	help <<"```\n\n";
	
	ptrBot->CreateMessage(ChannelLastMessage,help);
}

bool Discord_Overwatch::IsRegestered(String Id){
	for(Player &p : players){
		if(p.GetDiscordId().IsEqual(Id)){
			return true;
		}
	}
	return false;
}

bool Discord_Overwatch::DoUserHaveRightOnTeam(String TeamName,String userId){
	for(Equipe& eq : equipes){
		if(eq.GetEquipeName().IsEqual(TeamName)){
			for(EquipeCreator& ec : eq.creators){
				if(ec.GetPlayerID() == GetPlayersFromDiscordId(userId)->GetPlayerId()){
					return true;
				}
			}
			
		}
	}
	return false;
}
bool Discord_Overwatch::DoEquipeExist(String teamName){
	for(Equipe& eq : equipes){
		if(eq.GetEquipeName().IsEqual(teamName)){
			return true;
		}
	}
	return false;
}


Player* Discord_Overwatch::GetPlayersFromId(int Id){
	for(Player &p : players){
		if(p.GetPlayerId() == Id){
			return &p;
		}
	}
	return nullptr;
}

Player* Discord_Overwatch::GetPlayersFromDiscordId(String Id){
	for(Player &p : players){
		if(p.GetDiscordId().IsEqual(Id)){
			return &p;
		}
	}
	return nullptr;
}

Equipe* Discord_Overwatch::GetEquipeById(int ID){
	for(Equipe& eq : equipes){
		if(eq.GetEquipeId() == ID){
			return &eq;	
		}
	}
	return nullptr;
}

Equipe* Discord_Overwatch::GetEquipeByName(String teamName){
	for(Equipe& eq : equipes){
		if(eq.GetEquipeName().IsEqual(teamName)){
			return &eq;	
		}
	}
	return nullptr;
}

void Discord_Overwatch::DeleteEquipeById(int ID){
	int cpt =0;
	for(Equipe& eq : equipes){
		if(eq.GetEquipeId() == ID){
			break; 	
		}
		cpt++;
	}
	equipes.Remove(cpt,1);
}

bool Discord_Overwatch::DeletePlayerById(int ID){
	for(Equipe& eq : equipes){
		for(EquipeCreator &ec : eq.creators){
			if(ec.GetPlayerID() == ID && ec.IsAdmin()){
				return false;
			}
		}
		int cpt=0;
		bool trouver =false;
		for(int& id : eq.playersId){
			if(id == ID){
				trouver=true;
				break;
			}
			cpt++;
		}
		if(trouver) eq.playersId.Remove(cpt,1);
	}
	int cpt=0;
	bool trouver =false;
	for(Player& pl : players){
		if(pl.GetPlayerId() == ID){
			trouver =true;
			break;	
		}
		cpt++;
	}
	if(trouver) players.Remove(cpt,1);
	return true;
}

bool Discord_Overwatch::DoUserIsCreatorOfTeam(int TeamId,int PlayerId){
	for(Equipe& eq : equipes){
		Cout() << eq.GetEquipeId() <<"\n";
		if(eq.GetEquipeId() == TeamId){
			for(EquipeCreator& ec : eq.creators){
				if(ec.GetPlayerID() == PlayerId )return true;
			}
			break;
		}
	}
	return false;
}

bool Discord_Overwatch::DoUserIsAdminOfTeam(int TeamId,int PlayerId){
	for(Equipe& eq : equipes){
		if(eq.GetEquipeId() == TeamId){
			for(EquipeCreator& ec : eq.creators){
				if(ec.GetPlayerID() == PlayerId &&  ec.IsAdmin())return true;
			}
			break;
		}
	}
	return false;
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


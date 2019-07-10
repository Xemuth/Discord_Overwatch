#include "Discord_Overwatch.h"
#include <Sql/sch_schema.h>
#include <Sql/sch_source.h>
#include <iostream>
#include <GraphBuilder/GraphBuilder.h>
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


Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix):myGraph(1920,1080){
	name = _name;
	prefix = _prefix;
	
	prepareOrLoadBDD();
	LoadMemoryCRUD();
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("execsql"))this->executeSQL(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("api"))this->testApi(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("register"))this->Register(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("remove"))this->DeleteProfil(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("createequipe"))this->CreateEquipe(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("removeequipe"))this->RemoveEquipe(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("giveright"))this->GiveRight(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("removeright"))this->RemoveRight(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("crud"))this->GetCRUD(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("help"))this->Help(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("reloadcrud"))this->ReloadCRUD(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("addtoequipe"))this->AddPersonToEquipe(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("removefromequipe"))this->RemovePersonToEquipe(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("removemefromequipe"))this->RemoveMeFromEquipe(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("upd"))this->ForceUpdate(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("eupd"))this->ForceEquipeUpdate(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("drawstatsequipe"))this->DrawStatsEquipe(e);ActionDone=true;});
	EventsMapMessageCreated.Add([&](ValueMap e){if(MessageArgs.GetCount()>0 && MessageArgs[0].IsEqual("graphproperties"))this->GraphProperties(e);ActionDone=true;});
	
}

void Discord_Overwatch::EventsMessageCreated(ValueMap payload){ //We admit BDD must be loaded to be active
	for(auto &e : EventsMapMessageCreated){
		if(bddLoaded){
			e(payload);
			if(ActionDone){
				break;
			}
		}
		else{ 
			ptrBot->CreateMessage(ChannelLastMessage,"DataBase not loaded !"); 
			break;
		}
	}
}

void Discord_Overwatch::LoadMemoryCRUD(){
	if(bddLoaded){
		players.Clear();
		equipes.Clear();
		
		Sql sql;
		sql*Select(SqlAll()).From(OW_PLAYERS);
		while(sql.Fetch()){
			players.Add(Player(sql[0].Get<int64>(),sql[1].ToString(),sql[2].ToString(),sql[3].ToString()));	
		}
		for(Player &p : players){
			sql*Select(SqlAll()).From(OW_PLAYER_DATA).Where(DATA_PLAYER_ID == p.GetPlayerId());
			while(sql.Fetch()){
				Date date;
				if(sql[2].GetTypeName().IsEqual("Date")){
					date = sql[2].Get<Date>();
				}else{
					date.Set(sql[1].Get<int64>());
				}
				p.datas.Add(PlayerData(sql[0].Get<int64>() ,date,sql[3].Get<int64>(),sql[4].Get<int64>(),sql[5].Get<int64>(),sql[6].Get<int64>(),sql[7].Get<int64>(),sql[8].Get<int64>(),sql[9].Get<int64>()));
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

void Discord_Overwatch::ReloadCRUD(ValueMap payload){
	LoadMemoryCRUD();
	ptrBot->CreateMessage(ChannelLastMessage,"Crud rechargé !");	
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
							if(!DoUserHaveRightOnTeam(teamName, idCite)){
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
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " à déjà les droits !");
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

//!ow AddToEquipe @NattyRoots Sombre est mon histoire
void Discord_Overwatch::AddPersonToEquipe(ValueMap payload){ //To add a person to an equipe you must have the right to add it
	if(MessageArgs.GetCount()>=3){
		if( IsRegestered(AuthorId)){
			String idCite = Replace(MessageArgs[1],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
			if(IsRegestered(idCite)){
				String teamName="";
				for(int e =2; e < MessageArgs.GetCount(); e++){
					if(e!=2 && e != MessageArgs.GetCount())teamName << " ";
					teamName << MessageArgs[e];
				}
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
						auto* player = GetPlayersFromDiscordId(idCite);
						auto* equipe = GetEquipeByName(teamName);
						if(!DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
							Sql sql;
							if(sql*Insert(OW_EQUIPES_PLAYERS)(EP_PLAYER_ID,player->GetPlayerId())(EP_EQUIPE_ID,equipe->GetEquipeId())){
								equipe->playersId.Add(player->GetPlayerId());
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] +" a été ajouté !");
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,"SQL Error !");
							}
						}else{
							ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " est déjà dans l'équipe ! -_-'");
						}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right to add ppl to this team.");
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

//!ow RemoveFromEquipe @NattyRoots Sombre est mon histoire
void Discord_Overwatch::RemovePersonToEquipe(ValueMap payload){ //Remove Person from and equipe (only righter can do it)
	if(MessageArgs.GetCount()>=3){
		if( IsRegestered(AuthorId)){
			String idCite = Replace(MessageArgs[1],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
			if(IsRegestered(idCite)){
				String teamName="";
				for(int e =2; e < MessageArgs.GetCount(); e++){
					if(e!=2 && e != MessageArgs.GetCount())teamName << " ";
					teamName << MessageArgs[e];
				}
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
						auto* player = GetPlayersFromDiscordId(idCite);
						auto* equipe = GetEquipeByName(teamName);
						if(DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
							Sql sql;
							if(sql*Delete(OW_EQUIPES_PLAYERS).Where(EP_PLAYER_ID==player->GetPlayerId()&& EP_EQUIPE_ID==equipe->GetEquipeId())){
								int cpt=0;
								bool trouver=false;
								for(int& pid: equipe->playersId){
									if(pid == player->GetPlayerId()){
										trouver=true;
										break;	
									}
									cpt++;
								}
								if(trouver) equipe->playersId.Remove(cpt,1);
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] +" a été retiré de "+ teamName +" !");
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,"SQL Error !");
							}
						}else{
							ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " n'est pas dans l'équipe !");
						}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right to add ppl to this team.");
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

//!ow RemoveMeFromEquipe Sombre est mon histoire
void Discord_Overwatch::RemoveMeFromEquipe(ValueMap payload){ //Remove u from one of your equipe
	if(MessageArgs.GetCount()>=2){
		if( IsRegestered(AuthorId)){
			String teamName="";
			for(int e =2; e < MessageArgs.GetCount(); e++){
				if(e!=2 && e != MessageArgs.GetCount())teamName << " ";
				teamName << MessageArgs[e];
			}
			
			if(DoEquipeExist(teamName)){
				auto* player = GetPlayersFromDiscordId(AuthorId);
				auto* equipe = GetEquipeByName(teamName);
				if(DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
					Sql sql;
					if(sql*Delete(OW_EQUIPES_PLAYERS).Where(EP_PLAYER_ID==player->GetPlayerId()&& EP_EQUIPE_ID==equipe->GetEquipeId())){
						int cpt=0;
						bool trouver=false;
						for(int& pid: equipe->playersId){
							if(pid == player->GetPlayerId()){
								trouver=true;
								break;	
							}
							cpt++;
						}
						if(trouver)equipe->playersId.Remove(cpt,1);
						ptrBot->CreateMessage(ChannelLastMessage,"Vous avez été retiré de "+ teamName +" !");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas dans cette équipe !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}
}

//!ow upd
void Discord_Overwatch::ForceUpdate(ValueMap payload){ //Force update, based on the personne who make the call
	if(IsRegestered(AuthorId)){
		Player* player = GetPlayersFromDiscordId(AuthorId);
		if(player != nullptr){
			if(UpdatePlayer(player->GetPlayerId()))
				ptrBot->CreateMessage(ChannelLastMessage,player->GetBattleTag() + " a été mis à jour !");
			else 
				ptrBot->CreateMessage(ChannelLastMessage,"Mise à jours impossible ! :(");
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Le joueur n'existe pas -_-'");
		}
	}
}

//!ow Eupd Sombre est mon histoire
void Discord_Overwatch::ForceEquipeUpdate(ValueMap payload){ // Idk if only ppl who have right on equipe must do it or letting it free.
	if(MessageArgs.GetCount()>=2){
		if( IsRegestered(AuthorId)){
			String teamName="";
			for(int e =1; e < MessageArgs.GetCount(); e++){
				if(e!=1 && e != MessageArgs.GetCount())teamName << " ";
				teamName << MessageArgs[e];
			}
			if(DoEquipeExist(teamName)){
				auto* player = GetPlayersFromDiscordId(AuthorId);
				auto* equipe = GetEquipeByName(teamName);
				if(player !=nullptr && equipe !=nullptr){
					if(DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
						bool result=true;
						for(int& pid : equipe->playersId){
							if(!UpdatePlayer(pid))
								result=false;
						}
						ptrBot->CreateMessage(ChannelLastMessage,"Mise à jour de l'équipe terminée !!");
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas dans cette équipe !");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Joueur introuvable !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Je ne peux pas mettre à jours une équipe si vous ne me spécifier pas d'équipe !");
	}
}

bool Discord_Overwatch::UpdatePlayer(int playerId){
	Player* player = GetPlayersFromId(playerId);
	if(player != nullptr){
		String bTag = player->GetBattleTag();
		
		HttpRequest reqApi;
		bTag.Replace("#","-");
		reqApi.Url("https://ow-api.com/v1/stats/pc/euw/" + bTag + "/profile");
		reqApi.GET();
		auto json = ParseJSON(reqApi.Execute());
		if(!json["error"].ToString().IsEqual("Player not found")){
			Date date = GetSysDate();
			Cout() << json["competitiveStats"]["games"]["played"].GetTypeName() <<"\n";
			int gameP = json["competitiveStats"]["games"]["played"].Get<double>();
			int level = (json["prestige"].Get<double>() *100) + json["level"].Get<double>();
			int rating = json["rating"].Get<double>();
			int medalC = json["competitiveStats"]["awards"]["medals"].Get<double>();
			int medalB = json["competitiveStats"]["awards"]["medalsBronze"].Get<double>();
			int medalS = json["competitiveStats"]["awards"]["medalsSilver"].Get<double>();
			int medalG = json["competitiveStats"]["awards"]["medalsGold"].Get<double>();
			Sql sql;
			if(sql*Insert(OW_PLAYER_DATA)(DATA_PLAYER_ID,player->GetPlayerId())(RETRIEVE_DATE,date)(GAMES_PLAYED,gameP)(LEVEL,level)(RATING,rating)(MEDALS_COUNT,medalC)(MEDALS_BRONZE,medalB)(MEDALS_SILVER,medalS)(MEDALS_GOLD,medalG)){
				player->datas.Add(PlayerData(sql.GetInsertedId().Get<int64>(),date,gameP,level,rating,medalC,medalB,medalS,medalG));
				return true;
			}
			return false;
		}
		ptrBot->CreateMessage(ChannelLastMessage,player->GetBattleTag() + " est introuvable sur l'Api Overwatch !");
		return false;
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,+ "Ce joueur n'existe pas !");
	}
}

//!ow DrawStatsEquipe rating Sombre est mon histoire
void Discord_Overwatch::DrawStatsEquipe(ValueMap payload){ //Permet de déssiner le graph 
	if(MessageArgs.GetCount()>=3){
		String teamName="";
		for(int e =2; e < MessageArgs.GetCount(); e++){
			if(e!=2 && e != MessageArgs.GetCount())teamName << " ";
			teamName << MessageArgs[e];
		}
		myGraph.ClearData();
		MessageArgs[1] = ToLower(MessageArgs[1]);
		if(MessageArgs[1].IsEqual("rating")){
			myGraph.SetGraphName("Evolution du rank pour " + teamName);
			myGraph.DefineXName( "Date");
			myGraph.DefineYName("Rating");

			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,AllColors[cpt]));
					myGraph[cpt].ShowDot(true);
					for(PlayerData& pd : atmP->datas){
						myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetRating()),&myGraph[cpt]));
					}
					cpt++;
				}
				PNGEncoder png;
				png.SaveFile("temp.png", myGraph.DrawGraph());
				ptrBot->SendFile(ChannelLastMessage,"Evolution du rank pour " + teamName,"","temp.png");
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"La team n'existe pas ! -_-'");
			}
		}else if(MessageArgs[1].IsEqual("levels")){
			myGraph.SetGraphName("Evolution des levels pour " + teamName);
			myGraph.DefineXName("Date");
			myGraph.DefineYName("Level");

			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,AllColors[cpt]));
					myGraph[cpt].ShowDot(true);
					for(PlayerData& pd : atmP->datas){
						myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetLevel()),&myGraph[cpt]));
					}
					cpt++;
				}
				PNGEncoder png;
				png.SaveFile("temp.png", myGraph.DrawGraph());
				ptrBot->SendFile(ChannelLastMessage,"Evolution des levels pour " + teamName,"","temp.png");
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"La team n'existe pas ! -_-'");
			}
		}else if(MessageArgs[1].IsEqual("medals")){
			myGraph.SetGraphName("Evolution des medailles pour " + teamName);
			myGraph.DefineXName( "Date");
			myGraph.DefineYName("medals");

			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,AllColors[cpt]));
					myGraph[cpt].ShowDot(true);
					for(PlayerData& pd : atmP->datas){
						myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetMedalsCount()),&myGraph[cpt]));
					}
					cpt++;
				}
				PNGEncoder png;
				png.SaveFile("temp.png", myGraph.DrawGraph());
				ptrBot->SendFile(ChannelLastMessage,"Evolution des medailles pour " + teamName,"","temp.png");
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"La team n'existe pas ! -_-'");
			}
		}else if(MessageArgs[1].IsEqual("games")){
			myGraph.SetGraphName("Evolution du nombre de games pour " + teamName);
			myGraph.DefineXName( "Date");
			myGraph.DefineYName("Nombre de games");
			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,AllColors[cpt]));
					myGraph[cpt].ShowDot(true);
					for(PlayerData& pd : atmP->datas){
						myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetGamesPlayed()),&myGraph[cpt]));
					}
					cpt++;
				}
				PNGEncoder png;
				png.SaveFile("temp.png", myGraph.DrawGraph());
				ptrBot->SendFile(ChannelLastMessage,"Evolution du nombre de games pour " + teamName,"","temp.png");
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"La team n'existe pas ! -_-'");
			}
		}
		
	}
}

void Discord_Overwatch::GraphProperties(ValueMap payload){
	if(	MessageArgs.GetCount() >1){
		bool isSuccess = false;
		String message1 = ToLower(MessageArgs[1]);
		Value vF =ResolveType(MessageArgs[2]);
		ptrBot->CreateMessage(ChannelLastMessage,"inheritance de type : " +vF.GetTypeName());
		if(MessageArgs.GetCount() ==3 && message1.IsEqual("showgraphname")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowGraphName(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("showlegendsofcourbes")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowLegendsOfCourbes(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("showvalueofdot")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowValueOfDot(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("activatemaxdatepadding")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ActivateMaxDatePadding(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("setmaxdatepadding")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("int")){
				int vi =v.Get<int>();
				if(vi<0) vi =0;
				myGraph.SetMaxDatePadding(vi);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("setactivatedspecifiedlowestaxisy")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.SetActivatedSpecifiedLowestAxisY(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("setspecifiedloweststartingnumberaxisy")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("int")){
				int vi =v.Get<int>();
				if(vi<0) vi =0;
				myGraph.SetSpecifiedLowestStartingNumberAxisY(vi);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("setactivatedspecifiedhighestaxisy")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.SetActivatedSpecifiedHighestAxisY(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==3 && message1.IsEqual("setspecifiedhigheststartingnumberaxisy")){
			Value v =ResolveType(MessageArgs[2]);
			if(v.GetTypeName().IsEqual("int")){
				int vi =v.Get<int>();
				if(vi<0) vi =0;
				myGraph.SetSpecifiedHighestStartingNumberAxisY(vi);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==5 &&message1.IsEqual("setalphacolor")){
			Value r =ResolveType(MessageArgs[2]);
			Value g =ResolveType(MessageArgs[3]);
			Value b =ResolveType(MessageArgs[4]);
			if(r.GetTypeName().IsEqual("int") && g.GetTypeName().IsEqual("int") && b.GetTypeName().IsEqual("int")){
				int ri =r.Get<int>();
				int gi =g.Get<int>();
				int bi =b.Get<int>();
				if(ri>255)r=255;
				if(ri< 0) r=0;
				if(gi>255)r=255;
				if(gi< 0) r=0;
				if(bi>255)r=255;
				if(bi< 0) r=0;
				myGraph.SetAlphaColor(Color(ri,gi,bi));
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==5 && message1.IsEqual("setmaincolor")){
			Value r =ResolveType(MessageArgs[2]);
			Value g =ResolveType(MessageArgs[3]);
			Value b =ResolveType(MessageArgs[4]);
			if(r.GetTypeName().IsEqual("int") && g.GetTypeName().IsEqual("int") && b.GetTypeName().IsEqual("int")){
				int ri =r.Get<int>();
				int gi =g.Get<int>();
				int bi =b.Get<int>();
				if(ri>255)r=255;
				if(ri< 0) r=0;
				if(gi>255)r=255;
				if(gi< 0) r=0;
				if(bi>255)r=255;
				if(bi< 0) r=0;
				myGraph.SetMainColor(Color(ri,gi,bi));
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}
		else{
			ptrBot->CreateMessage(ChannelLastMessage,"Proprieté invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
		}
		if(isSuccess) ptrBot->CreateMessage(ChannelLastMessage,"Succès !");
	}else{
		String help ="";
		help << "```Aide pour les propriéter du graph :\n";
		help << "Property: ShowGraphName || Value:True,False" <<" -> Définie si le nom du graph est affiché sur l'image.\n\n";
		help << "Property: ShowLegendsOfCourbes || Value:True,False"<<" -> Définie si la légende du graph est affiché sur l'image.\n\n";
		help << "Property: ShowValueOfDot || Value:True,False"<<" -> Définie si la valeur des points est affichées\n\n";
		help << "Property: ActivateMaxDatePadding || Value:True,False"<<" -> Définie si le graph affiche un nombre maximal de date ou pas\n\n";
		help << "Property: SetMaxDatePadding || Value:int>0"<<" -> Définie le nombre maximun de date à afficher par le graph.\n\n";
		help << "Property: SetActivatedSpecifiedLowestAxisY || Value:True,False"<<" -> Définie si le graph est délimité par un nombre minimal.\n\n";
		help << "Property: SetSpecifiedLowestStartingNumberAxisY || Value:int>0"<<" -> Définie le nombre minimun utilisée pour déléminité le graph.\n\n";
		help << "Property: SetActivatedSpecifiedHighestAxisY || Value:True,False"<<" -> Définie si le graph est délimité par un nombre maximal.\n\n";
		help << "Property: SetSpecifiedHighestStartingNumberAxisY || Value:int>0"<<" -> Définie le nombre maximun utilisée pour déléminité le graph.\n\n";
		help << "Property: SetAlphaColor || Value:0<=int>=255,0<=int>=255,0<=int>=255"<<" ->Définie la couleur Alpha.\n\n";
		help << "Property: SetMainColor || Value:0<=int>=255,0<=int>=255,0<=int>=255"<<" -> Définie la couleur principale du graph.\n\n";
		help <<"```\n\n";
		ptrBot->CreateMessage(ChannelLastMessage,help);
	}
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
	help << "AddToEquipe(String DiscordName,String EquipeName)"<<" -> Ajoute l'utilisateur à l'équipe (vous devez avoir les droits sur l'équipe !).\n\n";
	help << "RemoveFromEquipe(String DiscordName,String EquipeName)"<<" -> Retire l'utilisateur de l'équipe(vous devez avoir les droits sur l'équipe !).\n\n";
	help << "RemoveMeFromEquipe(String EquipeName)"<<" -> Vous retire de l'équipe.\n\n";
	help << "Upd()"<<" -> Mets à jours votre profile niveau stats.\n\n";
	help << "Eupd(String EquipeName)"<<" -> Mets à jours toute l'équipe par rapport à l'Api(Vous devez être dans l'équipe.).\n\n";
	help << "DrawStatsEquipe(String ValueToStatify,String EquipeName)"<<" -> Dessine un graph par rapport à la value à afficher (rating, levels, medals, games).\n\n";
	help << "GraphProperties([String Property],[String Value])"<<" ->Definie les proprieté pour le graph, Si aucun arguments, renvoie l'aide sur les proprietées.\n\n";
	help << "Crud()"<<" -> Affiche le memory crud du programme.\n\n";
	help << "ReloadCRUD()"<<" -> Recharge le memory crud du programme.\n\n";
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

bool Discord_Overwatch::DoUserIsStillOnTeam(int TeamId,int PlayerId){
	for(Equipe& eq : equipes){
		if(eq.GetEquipeId() == TeamId){
			for(int& pid : eq.playersId){
				if(pid == PlayerId) return true;
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


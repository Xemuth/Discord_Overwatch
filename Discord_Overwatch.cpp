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
	
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("execsql"))executeSQL(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("api"))testApi(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("register"))Register(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("remove"))DeleteProfil(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("createequipe"))CreateEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("removeequipe"))RemoveEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("giveright"))GiveRight(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("removeright"))RemoveRight(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("crud"))GetCRUD(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("help"))Help(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("reloadcrud"))ReloadCRUD(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("addtoequipe"))AddPersonToEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("removefromequipe"))RemovePersonToEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("removemefromequipe"))RemoveMeFromEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("upd"))ForceUpdate(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("eupd"))ForceEquipeUpdate(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("upr"))updateRating(e);});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("graphproperties"))GraphProperties(e);});
	#ifdef flagGRAPHBUILDER_DB //Flag must be define to activate all DB func
		EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("drawstatsequipe"))DrawStatsEquipe(e);});
		EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("savegraphparam"))saveActualGraph(e);});
		EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("drawstats"))DrawStatsPlayer();});
	#endif
	#ifndef flagGRAPHBUILDER_DB
		EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("drawstatsequipe"))DrawStatsEquipe(e);}); 
	#endif
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("startautoupdate"))startThread();});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("stopautoupdate"))stopThread();});
	EventsMapMessageCreated.Add([&](ValueMap e){if(NameOfFunction.IsEqual("autoupdate"))GetEtatThread();});
	
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
		players.Clear();
		equipes.Clear();
		
		Sql sql(sqlite3);
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
		Cout()<< "Memory crude loaded"<<"\n";//PrintMemoryCrude()<<"\n";
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
		//SQL = sqlite3;
		if(mustCreate){
			SqlSchema sch_OW(SQLITE3);
			All_Tables(sch_OW);
			if(sch_OW.ScriptChanged(SqlSchema::UPGRADE)){
				SqlPerformScript(sch_OW.Upgrade());
			}	
			if(sch_OW.ScriptChanged(SqlSchema::ATTRIBUTES)){	
				SqlPerformScript(sch_OW.Attributes());
			}
			if(sch_OW.ScriptChanged(SqlSchema::CONFIG)) {
				SqlPerformScript(sch_OW.ConfigDrop());
				SqlPerformScript(sch_OW.Config());
			}
			sch_OW.SaveNormal();
			
		}
		Sql sql(sqlite3);
		sql.Execute("PRAGMA foreign_keys = ON;"); //Enable Foreign keys
		bddLoaded =true;
	}
	#undef MODEL
}

void Discord_Overwatch::ReloadCRUD(ValueMap payload){
	LoadMemoryCRUD();
	ptrBot->CreateMessage(ChannelLastMessage,"Crud rechargé !");	
}

void Discord_Overwatch::executeSQL(ValueMap payload){
	if(MessageArgs.GetCount()>=1){
		if(bddLoaded){
			Sql sql(sqlite3);
			String querry ="";
			for(int e = 0; e < MessageArgs.GetCount();e++){
				if(e==0)
					querry << MessageArgs[e];
				else
					querry << " " << MessageArgs[e];
			}
			querry << ";";
			String bufferedQuery = ToLower(querry);
			
			if(bufferedQuery.Find("select")!=-1){
				sql.Execute(querry);
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
				sql.Execute(querry);
				if(sql.WasError()){
					ptrBot->CreateMessage(ChannelLastMessage,sql.GetErrorCodeString());
					sql.ClearError();
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Cela semble avoir marché ! (oui c'est précis lol)");
				}
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Bdd non chargée");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur nombre d'argument incorrect, tu veux que j'execute du sql sans me donner de sql #Génie...```!ow ExecSql(SELECT * FROM MA_PUTAIN_DE_TABLE)```");	
	}
}

void Discord_Overwatch::GetCRUD(ValueMap payload){
	ptrBot->CreateMessage(ChannelLastMessage, PrintMemoryCrude());	
}

void Discord_Overwatch::testApi(ValueMap payload){
	if( MessageArgs.GetCount() == 1 ){
		HttpRequest reqApi;
		MessageArgs[0].Replace("#","-");
		reqApi.Url("https://ow-api.com/v1/stats/pc/euw/" + MessageArgs[0] + "/profile");
		reqApi.GET();
		auto json = ParseJSON(reqApi.Execute());
		ptrBot->CreateMessage(ChannelLastMessage,"Rating de " +  MessageArgs[0] +": "+json["rating"].ToString());
	}else{
		if(IsRegestered(AuthorId)){
			Player* p = GetPlayersFromDiscordId(AuthorId);
			if(p){
				HttpRequest reqApi;
				String btag = p->GetBattleTag();
				btag.Replace("#","-");
				reqApi.Url("https://ow-api.com/v1/stats/pc/euw/" + btag + "/profile");
				reqApi.GET();
				auto json = ParseJSON(reqApi.Execute());
				ptrBot->CreateMessage(ChannelLastMessage,"Rating de " + p->GetPersonneDiscordId() +": "+  json["rating"].ToString());
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Erreur lors de la récupération... Ptr player indéfini");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Je ne vous connait pas ! Enrgistrer vous ou donné moi un BattleTag valide sous la forme suivante :\"Xemuth#2392\"" );
		}
	}
}

//!ow register(BASTION#21406,Clément)
void Discord_Overwatch::Register(ValueMap payload){// The main idea is " You must be registered to be addable to an equipe or create Equipe
	if(MessageArgs.GetCount()==2){
		try{
			Sql sql(sqlite3);
			if(sql*Insert(OW_PLAYERS)(BATTLE_TAG,MessageArgs[0])(DISCORD_ID,AuthorId)(COMMUN_NAME,MessageArgs[1])){
				players.Add(Player(sql.GetInsertedId().Get<int64>(),MessageArgs[0],AuthorId,MessageArgs[1] ));
				ptrBot->CreateMessage(ChannelLastMessage,"Enregistrement réussie");
				return;
			}
			ptrBot->CreateMessage(ChannelLastMessage,"Error Registering");
		}catch(...){
			ptrBot->CreateMessage(ChannelLastMessage,"Error Registering");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Donne moi juste ton BTag et ton Pseudo```!ow Register(BASTION#21406,Mon Super Pseudo De Merde)```");	
	}
}

//!ow remove 
void Discord_Overwatch::DeleteProfil(ValueMap payload){ //Remove user from the bdd 
	if(MessageArgs.GetCount()==0){
		try{
			int id = GetPlayersFromDiscordId(AuthorId)->GetPlayerId();
			if(DeletePlayerById(id)){
				Sql sql(sqlite3);
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
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Recommence sans mettre d'arguments```!ow Remove()```");	
	}
}

//!ow createEquipe(Sombre est mon histoire)
void Discord_Overwatch::CreateEquipe(ValueMap payload){ //To add an equipe you must be registered. when an equipe is created, only 
	if(MessageArgs.GetCount()==1){
		if(IsRegestered(AuthorId)){
			try{
				String teamName=MessageArgs[0];
				
				Sql sql(sqlite3);
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
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! j'attends juste le nom de ton équipe```!ow CreateEquipe(Ton equipe bronze avec 1K2 d elo moyen)```");	
	}
}

//!ow removeEquipe Sombre est mon histoire 
void Discord_Overwatch::RemoveEquipe(ValueMap payload){ //you must have the right to remove equipe
	if(MessageArgs.GetCount()==1 ){
		if(IsRegestered(AuthorId)){
			String teamName=MessageArgs[0];
			if(DoEquipeExist(teamName)){
				if(DoUserHaveRightOnTeam(teamName, AuthorId)){
					Sql sql(sqlite3);
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
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! j'attends juste le nom de ton équipe```!ow RemoveEquipe(Ton equipe bronze avec 1K2 d elo moyen)```");	
	}
}

//!ow GiveRight(@NattyRoots,Sombre est mon histoire)
void Discord_Overwatch::GiveRight(ValueMap payload){ //Allow equipe owner/ equipe righter to add person to the equipe
	if(MessageArgs.GetCount()==2){
		String idCite = Replace(MessageArgs[0],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
		if( IsRegestered(AuthorId)){
			if(IsRegestered(idCite)){
				String teamName= MessageArgs[1];
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
							if(!DoUserHaveRightOnTeam(teamName, idCite)){
								int id= GetPlayersFromDiscordId(idCite)->GetPlayerId();
								Sql sql(sqlite3);
								if(sql*Insert(OW_EQUIPES_CREATOR)(EC_EQUIPES_ID,GetEquipeByName(teamName)->GetEquipeId())(EC_PLAYER_ID,id )(EC_ADMIN,false ) && sql*Insert(OW_EQUIPES_PLAYERS)(EP_EQUIPE_ID,GetEquipeByName(teamName)->GetEquipeId())(EP_PLAYER_ID,id )){
									auto* equipe = GetEquipeByName(teamName);
									equipe->creators.Add(EquipeCreator(id,false));
									equipe->playersId.Add(id);
									ptrBot->CreateMessage(ChannelLastMessage,"Ajout de  "+ MessageArgs[0] + " dans " + teamName +" !");
									ptrBot->CreateMessage(ChannelLastMessage,"Droits donnés à "+ MessageArgs[0] + "!");
								}
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] + " à déjà les droits !");
							}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,  MessageArgs[0] + " n'est pas enregistré !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! j'attends juste ton mate et ton equipe mon petit canard```!ow GiveRight(@NattyRoots,Ton equipe bronze avec 1K2 d elo moyen)```");	
	}
}

//!ow RemoveRight @NattyRoots Sombre est mon histoire
void Discord_Overwatch::RemoveRight(ValueMap payload){ //Remove equipe righter to one personne By discord ID
	if(MessageArgs.GetCount()==2){
		String idCite = Replace(MessageArgs[0],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
		if( IsRegestered(AuthorId)){
			if(IsRegestered(idCite)){
				String teamName=MessageArgs[1];
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
							int id= GetPlayersFromDiscordId(idCite)->GetPlayerId();
							int teamId= GetEquipeByName(teamName)->GetEquipeId();
							if(DoUserIsCreatorOfTeam(teamId,id)){
								if(!DoUserIsAdminOfTeam(teamId,id)){
									Sql sql(sqlite3);
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
									ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] + " est Admin de l'équipe !");
								}
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] + " ne possède pas de droit sur l'équipe !");
							}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,  MessageArgs[0] + " n'est pas enregistré !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! j'attends juste ton mate et ton equipe...```!ow RemoveRight(@NattyRoots,Ton equipe bronze avec 1K2 d elo moyen)```");	
	}
}

//!ow AddToEquipe @NattyRoots Sombre est mon histoire
void Discord_Overwatch::AddPersonToEquipe(ValueMap payload){ //To add a person to an equipe you must have the right to add it
	if(MessageArgs.GetCount()==2){
		if( IsRegestered(AuthorId)){
			String idCite = Replace(MessageArgs[0],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
			if(IsRegestered(idCite)){
				String teamName=MessageArgs[1];
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
						auto* player = GetPlayersFromDiscordId(idCite);
						auto* equipe = GetEquipeByName(teamName);
						if(!DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
							Sql sql(sqlite3);
							if(sql*Insert(OW_EQUIPES_PLAYERS)(EP_PLAYER_ID,player->GetPlayerId())(EP_EQUIPE_ID,equipe->GetEquipeId())){
								equipe->playersId.Add(player->GetPlayerId());
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] +" a été ajouté !");
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,"SQL Error !");
							}
						}else{
							ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] + " est déjà dans l'équipe ! -_-'");
						}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right to add ppl to this team.");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,  MessageArgs[0] + " n'est pas enregistré !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Logiquement, tu me donne ton mate et ton equipe #debile```!ow AddToEquipe(@NattyRoots,Ton equipe bronze avec 1K2 d elo moyen)```");	
	}
}

//!ow RemoveFromEquipe @NattyRoots Sombre est mon histoire
void Discord_Overwatch::RemovePersonToEquipe(ValueMap payload){ //Remove Person from and equipe (only righter can do it)
	if(MessageArgs.GetCount()==2){
		if( IsRegestered(AuthorId)){
			String idCite = Replace(MessageArgs[0],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
			if(IsRegestered(idCite)){
				String teamName=MessageArgs[1];
				
				if(DoEquipeExist(teamName)){
					if(DoUserHaveRightOnTeam(teamName, AuthorId)){
						auto* player = GetPlayersFromDiscordId(idCite);
						auto* equipe = GetEquipeByName(teamName);
						if(DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
							Sql sql(sqlite3);
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
								ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] +" a été retiré de "+ teamName +" !");
							}else{
								ptrBot->CreateMessage(ChannelLastMessage,"SQL Error !");
							}
						}else{
							ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] + " n'est pas dans l'équipe !");
						}
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"You do not have right to add ppl to this team.");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Team do not exist !");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,  MessageArgs[0] + " n'est pas enregistré !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Donne moi le mec à virer et ton equipe #AbusDeDroit```!ow RemoveFromEquipe(@NattyRoots,Ton equipe bronze avec 1K2 d elo moyen)```");	
	}
}

//!ow RemoveMeFromEquipe(Sombre est mon histoire)
void Discord_Overwatch::RemoveMeFromEquipe(ValueMap payload){ //Remove u from one of your equipe
	if(MessageArgs.GetCount()==1){
		if( IsRegestered(AuthorId)){
			String teamName=MessageArgs[0];

			if(DoEquipeExist(teamName)){
				auto* player = GetPlayersFromDiscordId(AuthorId);
				auto* equipe = GetEquipeByName(teamName);
				if(DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
					Sql sql(sqlite3);
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
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! donne juste ton équipe ...```!ow RemoveMeFromEquipe(le stup crew)```");	
	}
}

//!ow upd
void Discord_Overwatch::ForceUpdate(ValueMap payload){ //Force update, based on the personne who make the call
	if(MessageArgs.GetCount()==0 && IsRegestered(AuthorId)){
		Player* player = GetPlayersFromDiscordId(AuthorId);
		if(player != nullptr){
			if(UpdatePlayer(player->GetPlayerId()))
				ptrBot->CreateMessage(ChannelLastMessage,player->GetBattleTag() + " a été mis à jour !");
			else 
				ptrBot->CreateMessage(ChannelLastMessage,"Mise à jours impossible ! :(");
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Le joueur n'existe pas -_-'");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Me donne rien...```!ow upd()```");	
	}
}

//!ow Eupd Sombre est mon histoire
void Discord_Overwatch::ForceEquipeUpdate(ValueMap payload){ // Idk if only ppl who have right on equipe must do it or letting it free.
	if(MessageArgs.GetCount()==1){
		if( IsRegestered(AuthorId)){
			String teamName=MessageArgs[0];
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
			Sql sql(sqlite3);
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
		return false;
	}
}

#ifdef flagGRAPHBUILDER_DB //Flag must be define to activate all DB func
//!ow DrawStatsEquipe (rating,Sombre est mon histoire,graphproperties)
void Discord_Overwatch::DrawStatsEquipe(ValueMap payload){ //Permet de déssiner le graph 
	if(MessageArgs.GetCount()>=2){
		if(MessageArgs.GetCount() ==3){
			if(myGraph.LoadGraphParamFromBdd(MessageArgs[2]))
				ptrBot->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
			else
				ptrBot->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
		}
		myGraph.ClearData();
		String teamName=MessageArgs[1];
		MessageArgs[0] = ToLower(MessageArgs[0]);
		if(MessageArgs[0].IsEqual("rating")){
			myGraph.SetGraphName("Evolution du rank pour " + teamName);
			myGraph.DefineXName( "Date");
			myGraph.DefineYName("Rating");

			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,myGraph.GenerateColor()));
					myGraph[cpt].ShowDot(true);
					Date lastDate;
					int cpt2 =0;
					for(PlayerData& pd : atmP->datas){
						//Ici le code est 'dégueu' en gros pour récup tj la last date de la
						//journée, je boucle sur mes objets data contenant une date. Si l'objet
						//data sur lequel je suis possède la même date que le précédent alors
						//je delete le précédant et add celui-ci a la place. Je coderais plus
						//proprement a l'occass //TODO
						if(lastDate == pd.GetRetrieveDate()){
							cpt2--;
							myGraph[cpt].removeDot(cpt2);
							myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetRating()),&myGraph[cpt]));
						}else{
							myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetRating()),&myGraph[cpt]));
						}
						lastDate = pd.GetRetrieveDate();
						cpt2++;
					}
					cpt++;
				}
				PNGEncoder png;
				png.SaveFile("temp.png", myGraph.DrawGraph());
				ptrBot->SendFile(ChannelLastMessage,"Evolution du rank pour " + teamName,"","temp.png");
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"La team n'existe pas ! -_-'");
			}
		}else if(MessageArgs[0].IsEqual("levels")){
			myGraph.SetGraphName("Evolution des levels pour " + teamName);
			myGraph.DefineXName("Date");
			myGraph.DefineYName("Level");

			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,myGraph.GenerateColor()));
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
		}else if(MessageArgs[0].IsEqual("medals")){
			myGraph.SetGraphName("Evolution des medailles pour " + teamName);
			myGraph.DefineXName( "Date");
			myGraph.DefineYName("medals");

			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,myGraph.GenerateColor()));
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
		}else if(MessageArgs[0].IsEqual("games")){
			myGraph.SetGraphName("Evolution du nombre de games pour " + teamName);
			myGraph.DefineXName( "Date");
			myGraph.DefineYName("Nombre de games");
			Equipe* equipe = GetEquipeByName(teamName);
			if(equipe != nullptr){
				int cpt = 0;
				for(int& pid : equipe->playersId){
					Player* atmP = GetPlayersFromId(pid);
					String name = ((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag());
					myGraph.AddCourbe(Courbe(name,ValueTypeEnum::DATE, ValueTypeEnum::INT,myGraph.GenerateColor()));
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
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Toi tu veux analyser des graphs sans être capable d'appeler des fonctions correctement...```!ow DrawStatsEquipe(rating,ton equipe[,Eventuellement le nom des paramètres a charger])```");	
	}
}
void Discord_Overwatch::saveActualGraph(ValueMap payload){
	if(MessageArgs.GetCount() ==1){
		if(myGraph.SaveGraphParamInBDD(MessageArgs[0])!=-1){
			ptrBot->CreateMessage(ChannelLastMessage,"Paramètres sauvegardés.");	
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Erreur lors de l'enregistrement des paramètres !");		
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Précise le nom de la conf STP```!ow SaveGraphParam(ma conf trop belle)```");		
	}
}
//!ow DrawStatsPlayer(20,[@BASTION#21406]) I actually want 20 last elo change
void Discord_Overwatch::DrawStatsPlayer(){
	GraphDotCloud test(1920,1080);
	int number = -1;
	bool date =false;
	Date d1;
	Date d2;
	Player* p = nullptr;
	if(MessageArgs.GetCount() >=1 &&  isStringisANumber(MessageArgs[0])){
		number = std::stoi(MessageArgs[0].ToStd());
		if(MessageArgs.GetCount() >= 2){// Here I look if param is here
			if(MessageArgs[1].Find("<@!") !=-1){
				String id = Replace(MessageArgs[1],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
				if(IsRegestered(id)){
					p= GetPlayersFromDiscordId(id);
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " n'est pas enregistré. Tapez ```!ow register(votreBtag,votre pseudo)```" );
					return;	
				}
			}else{
				if(test.LoadGraphParamFromBdd(MessageArgs[1])){
					ptrBot->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
				}
			}
		}else{
			p = GetPlayersFromDiscordId(AuthorId);	
		}
		if(MessageArgs.GetCount() >= 3){
			if(MessageArgs[2].Find("<@!") !=-1){
				String id = Replace(MessageArgs[2],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
				if(IsRegestered(id)){
					p= GetPlayersFromDiscordId(id);
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[2] + " n'est pas enregistré. Tapez ```!ow register(votreBtag,votre pseudo)```" );
					return;	
				}
			}else{
				if(test.LoadGraphParamFromBdd(MessageArgs[2])){
					ptrBot->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
				}
			}
		}else if(p == nullptr){
			p = GetPlayersFromDiscordId(AuthorId);	
		}
	}else if(MessageArgs.GetCount() >=2 && MessageArgs[0].Find("/") !=-1 && MessageArgs[1].Find("/") !=-1){
		date =true;
		MessageArgs[0] = Replace(MessageArgs[0],Vector<String>{"\\","."," "},Vector<String>{"/","/","/"});
		MessageArgs[1] = Replace(MessageArgs[1],Vector<String>{"\\","."," "},Vector<String>{"/","/","/"});
		auto d = Split(MessageArgs[0],"/");
		if(!isStringisANumber(d[0]) || !isStringisANumber(d[1]) || !isStringisANumber(d[2])){
			ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[0] + " n'est pas une Date valide." );
			return;	
		}
		d1 =Date(std::stoi(d[2].ToStd()),std::stoi(d[1].ToStd()),std::stoi(d[0].ToStd()));
		d = Split(MessageArgs[1],"/");
		if(!isStringisANumber(d[0]) || !isStringisANumber(d[1]) || !isStringisANumber(d[2])){
			ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[1] + " n'est pas une Date valide." );
			return;
		}
		d2 =Date(std::stoi(d[2].ToStd()),std::stoi(d[1].ToStd()),std::stoi(d[0].ToStd()));
		if(MessageArgs.GetCount() >= 3){// Here I look if param is here
			if(MessageArgs[2].Find("<@!") !=-1){
				String id = Replace(MessageArgs[2],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
				if(IsRegestered(id)){
					p= GetPlayersFromDiscordId(id);
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[2] + " n'est pas enregistré. Tapez ```!ow register(votreBtag,votre pseudo)```" );
					return;	
				}
			}else{
				if(test.LoadGraphParamFromBdd(MessageArgs[2])){
					ptrBot->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
				}
			}
		}else{
			p = GetPlayersFromDiscordId(AuthorId);	
		}
		if(MessageArgs.GetCount() >= 4){
			if(MessageArgs[3].Find("<@!") !=-1){
				String id = Replace(MessageArgs[3],Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
				if(IsRegestered(id)){
					p= GetPlayersFromDiscordId(id);
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,MessageArgs[3] + " n'est pas enregistré. Tapez ```!ow register(votreBtag,votre pseudo)```" );
					return;	
				}
			}else{
				if(test.LoadGraphParamFromBdd(MessageArgs[3])){
					ptrBot->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
				}
			}
		}else if(!p){
			p = GetPlayersFromDiscordId(AuthorId);	
		}
	}else if(MessageArgs.GetCount()==0){
		p = GetPlayersFromDiscordId(AuthorId);	
	}
	if(p){
	
		test.DefineXValueType(ValueTypeEnum::INT);
		test.SetGraphName("Evolution du rating pour " + p->GetCommunName());
		test.DefineXName( "Enregistrement");
		test.DefineYName("Elo");
		int cpt = 0;
		test.AddCourbe(Courbe(p->GetCommunName(),ValueTypeEnum::INT,ValueTypeEnum::INT,test.GenerateColor()));
		auto& r = test[0];
		
		r.ShowDot(true);
		for(PlayerData& pd : p->datas){
			if(number !=-1){
				if((p->datas.GetCount() - cpt) <= number){
					r.AddDot(Dot(Value(cpt),Value(pd.GetRating()),&r));
					cpt++;
				}else{
					cpt++;	
				}
			}else if(date){
				if(d1 <= pd.GetRetrieveDate() && d2 >= pd.GetRetrieveDate()){
					r.AddDot(Dot(Value(cpt),Value(pd.GetRating()),&r));
					cpt++;
				}
			}else{
				r.AddDot(Dot(Value(cpt),Value(pd.GetRating()),&r));
				cpt++;
			}
		}
			
		PNGEncoder png;
		png.SaveFile("temp2.png", test.DrawGraph());
		ptrBot->SendFile(ChannelLastMessage,"Evolution du rating pour " + p->GetCommunName(),"","temp2.png");
	}
	else{
		ptrBot->CreateMessage(ChannelLastMessage,"Utilisateur introuvable en BDD.");	
	}	
}

#endif
#ifndef flagGRAPHBUILDER_DB
//!ow DrawStatsPlayer(rating,[@BASTION#21406],20) I actually want 20 last elo change
void Discord_Overwatch::DrawStatsPlayer(){
	
}

//!ow DrawStatsEquipe(rating,Sombre est mon histoire)
void Discord_Overwatch::DrawStatsEquipe(ValueMap payload){ //Permet de déssiner le graph 
	if(MessageArgs.GetCount()==2){
		String teamName=MessageArgs[1];
		myGraph.ClearData();
		MessageArgs[0] = ToLower(MessageArgs[0]);
		if(MessageArgs[0].IsEqual("rating")){
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
					Date lastDate;
					int cpt2 =0;
					for(PlayerData& pd : atmP->datas){
						//Ici le code est 'dégueu' en gros pour récup tj la last date de la
						//journée, je boucle sur mes objets data contenant une date. Si l'objet
						//data sur lequel je suis possède la même date que le précédent alors
						//je delete le précédant et add celui-ci a la place. Je coderais plus
						//proprement a l'occass //TODO
						if(lastDate == pd.GetRetrieveDate()){
							cpt2--;
							myGraph[cpt].removeDot(cpt2);
							myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetRating()),&myGraph[cpt]));
						}else{
							myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(pd.GetRating()),&myGraph[cpt]));
						}
						lastDate = pd.GetRetrieveDate();
						cpt2++;
					}
					cpt++;
				}
				PNGEncoder png;
				png.SaveFile("temp.png", myGraph.DrawGraph());
				ptrBot->SendFile(ChannelLastMessage,"Evolution du rank pour " + teamName,"","temp.png");
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"La team n'existe pas ! -_-'");
			}
		}else if(MessageArgs[0].IsEqual("levels")){
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
		}else if(MessageArgs[0].IsEqual("medals")){
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
		}else if(MessageArgs[0].IsEqual("games")){
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
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur arguments ! Toi tu veux analyser des graphs sans être capable d'appeler des fonctions correctement...```!ow DrawStatsEquipe(rating,ton equipe)```");	
	}
}
#endif

void Discord_Overwatch::updateRating(ValueMap payload){
	if(MessageArgs.GetCount() ==1 ){
		if(IsRegestered(AuthorId)){
			if(isStringisANumber(MessageArgs[0])){
				Player* player = GetPlayersFromDiscordId(AuthorId);
				if(player != nullptr){
					String bTag = player->GetBattleTag();
					int Elo = std::stoi(MessageArgs[0].ToStd());
					auto& buff =  player->datas[player->datas.GetCount()-1];
					Date date = GetSysDate();
					Sql sql(sqlite3);
					if(sql*Insert(OW_PLAYER_DATA)(DATA_PLAYER_ID,player->GetPlayerId())(RETRIEVE_DATE,date)(GAMES_PLAYED,buff.GetGamesPlayed())(LEVEL,buff.GetLevel())(RATING,Elo)(MEDALS_COUNT,buff.GetMedalsCount())(MEDALS_BRONZE,buff.GetMedalsB())(MEDALS_SILVER,buff.GetMedalsS())(MEDALS_GOLD,buff.GetMedalsG())){
						player->datas.Add(PlayerData(sql.GetInsertedId().Get<int64>(),date,buff.GetGamesPlayed(),buff.GetLevel(),Elo,buff.GetMedalsCount(),buff.GetMedalsB(),buff.GetMedalsS(),buff.GetMedalsG()));
						ptrBot->CreateMessage(ChannelLastMessage,"Enregistrement effectué !");
					}else{
						ptrBot->CreateMessage(ChannelLastMessage,"Erreur Insertion en BDD !");
					}
				}else{
					ptrBot->CreateMessage(ChannelLastMessage,"Erreur inconnnue ! Pointeur indéfini");
				}
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"L'argument n'est pas un chiffre !");
			}
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Pas asser d'arguments !```!ow upr(ton rating)```");
	}
}


void Discord_Overwatch::GraphProperties(ValueMap payload){
	if(	MessageArgs.GetCount() >0){
		bool isSuccess = false;
		String message1 = ToLower(MessageArgs[0]);
		Value vF =ResolveType(MessageArgs[1]);
		if(MessageArgs.GetCount() ==2 && message1.IsEqual("showgraphname")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowGraphName(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("showlegendsofcourbes")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowLegendsOfCourbes(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("showvalueofdot")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowValueOfDot(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("activatemaxdatepadding")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ActivateMaxDatePadding(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("setmaxdatepadding")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("int")){
				int vi =v.Get<int>();
				if(vi<0) vi =0;
				myGraph.SetMaxDatePadding(vi);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("setactivatedspecifiedlowestaxisy")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.SetActivatedSpecifiedLowestAxisY(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("setspecifiedloweststartingnumberaxisy")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("int")){
				int vi =v.Get<int>();
				if(vi<0) vi =0;
				myGraph.SetSpecifiedLowestStartingNumberAxisY(vi);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("setactivatedspecifiedhighestaxisy")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.SetActivatedSpecifiedHighestAxisY(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("setspecifiedhigheststartingnumberaxisy")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("int")){
				int vi =v.Get<int>();
				if(vi<0) vi =0;
				myGraph.SetSpecifiedHighestStartingNumberAxisY(vi);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==4 &&message1.IsEqual("setalphacolor")){
			Value r =ResolveType(MessageArgs[1]);
			Value g =ResolveType(MessageArgs[2]);
			Value b =ResolveType(MessageArgs[3]);
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
		}else if(MessageArgs.GetCount() ==4 && message1.IsEqual("setmaincolor")){
			Value r =ResolveType(MessageArgs[1]);
			Value g =ResolveType(MessageArgs[2]);
			Value b =ResolveType(MessageArgs[3]);
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
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("signit")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.SignIt(vb);
				isSuccess=true;
			}else{
				ptrBot->CreateMessage(ChannelLastMessage,"Argument invalide ! Tapez \"!ow GraphProperties\" pour avoir la liste des proprieter disponible");
			}
		}else if(MessageArgs.GetCount() ==2 && message1.IsEqual("showvalueonaxis")){
			Value v =ResolveType(MessageArgs[1]);
			if(v.GetTypeName().IsEqual("bool")){
				bool vb = v.Get<bool>();
				myGraph.ShowValueOnAxis(vb);
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
		help <<"```";
		help << "!ow graphProperties(ShowGraphName, True||False)" <<" -> Définie si le nom du graph est affiché sur l'image.\n\n";
		help << "!ow graphProperties(ShowLegendsOfCourbes, True||False)"<<" -> Définie si la légende du graph est affiché sur l'image.\n\n";
		help << "!ow graphProperties(ShowValueOfDot, True||False)"<<" -> Définie si la valeur des points est affichées\n\n";
		help << "!ow graphProperties(ShowValueOnAxis, True||False)"<<" -> Définie si les valeurs apparaissent sur les Axis.\n\n";
		help << "!ow graphProperties(ActivateMaxDatePadding, True||False)"<<" -> Définie si le graph affiche un nombre maximal de date ou pas\n\n";
		help << "!ow graphProperties(SetMaxDatePadding, valeur >0)"<<" -> Définie le nombre maximun de date à afficher par le graph.\n\n";
		help << "!ow graphProperties(SetActivatedSpecifiedLowestAxisY, True||False)"<<" -> Définie si le graph est délimité par un nombre minimal.\n\n";
		help << "!ow graphProperties(SetSpecifiedLowestStartingNumberAxisY, valeur >0)"<<" -> Définie le nombre minimun utilisée pour déléminité le graph.\n\n";
		help << "!ow graphProperties(SetActivatedSpecifiedHighestAxisY, True||False)"<<" -> Définie si le graph est délimité par un nombre maximal.\n\n";
		help << "!ow graphProperties(SetSpecifiedHighestStartingNumberAxisY, valeur >0)"<<" -> Définie le nombre maximun utilisée pour déléminité le graph.\n\n";
		help << "!ow graphProperties(SetAlphaColor, 0<= valeur >=255,0<= valeur >=255,0<= valeur >=255)"<<" ->Définie la couleur Alpha.\n\n";
		help << "!ow graphProperties(SetMainColor, 0<= valeur >=255,0<= valeur >=255,0<= valeur >=255)"<<" -> Définie la couleur principale du graph.\n\n";
		help << "!ow graphProperties(SignIt, True||False)"<<" -> Définie si le graph est signé ou non.\n\n";
		help <<"```\n\n";
		ptrBot->CreateMessage(ChannelLastMessage,help);
	}
}

bool Discord_Overwatch::GetEtatThread(){//used to return if thread is start or stopped
	if(MessageArgs.GetCount()==0){
		if(autoUpdate.IsOpen()){
			HowManyTimeBeforeUpdate =true;
			ptrBot->CreateMessage(ChannelLastMessage,"Actuellement, l'autoUpdater est lancé !");	
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"Actuellement, l'autoUpdater est coupé !");	
		}
		return true;
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur d'arguments !```!ow AutoUpdate()```");	
		return false;
	}
}

void Discord_Overwatch::startThread(){
	if(MessageArgs.GetCount()==0){
		if(autoUpdate.IsOpen()){
			ptrBot->CreateMessage(ChannelLastMessage,"L'autoUpdated est déjà lancé !");	
		}else{
			autoUpdate.Run([&](){threadStarted =true;for(;;){

									for(int e = 0; e < 3600 ; e++){
										if(!threadStarted) break;
										if(HowManyTimeBeforeUpdate){ 
											ptrBot->CreateMessage(ChannelLastMessage,"Prochaine mise à jours dans : "+ AsString((3600-e)/60)+"min");	
											HowManyTimeBeforeUpdate=false;
										}
										Sleep(1000);
									}
									if(!threadStarted) break;
									for(Player &p : players){
										if(!threadStarted) break;
										UpdatePlayer(p.GetPlayerId());
									}	
									Cout() << "Update complete"<<"\n";
								}
								Cout() << "fin du thread"<<"\n";
								return;});
			ptrBot->CreateMessage(ChannelLastMessage,"L'autoUpdated est désormait lancé !");
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur d'arguments !```!ow startAutoUpdate()```");	
	}
}
void Discord_Overwatch::stopThread(){
	if(MessageArgs.GetCount()==0){
		if(autoUpdate.IsOpen()){
			threadStarted=false;
			autoUpdate.Wait();
			ptrBot->CreateMessage(ChannelLastMessage,"L'autoUpdated est désormait coupé !");	
		}else{
			ptrBot->CreateMessage(ChannelLastMessage,"L'autoUpdated n'est pas lancé !");	
		}
	}else{
		ptrBot->CreateMessage(ChannelLastMessage,"Erreur d'arguments !```!ow stopAutoUpdate()```");	
	}
}


void Discord_Overwatch::Help(ValueMap payload){
	String help ="";
	help <<"```";
	help << "!ow ExecSQL(SqlToExecute)" <<" -> Execute du SQL passé en paramètre.\n\n";
	help << "!ow Api(BattleTag)"<<" -> Récupère le Rank, depuis l'api OverWatch, du BattleTag passé en paramètre.\n\n";
	help << "!ow Register(BattleTag,Pseudo)"<<" -> Enregistre votre identité auprès du bot(Necessaire pour toutes les commandes en liaison avec les équipes.\n\n";
	help << "!ow Remove()"<<" -> Efface votre identité auprès du bot #RGPD_Friendly.\n\n";
	help << "!ow CreateEquipe(EquipeName)"<<" -> Vous permet de créer une équipe(Une equipes contient des players et permet la génération de graph).\n\n";
	help << "!ow RemoveEquipe(EquipeName)"<<" -> Vous permet de supprimer une équipe(n'importe qui ayant les droits sur l'équipe à le droit de supprimer l'équipe).\n\n";
	help << "!ow GiveRight(DiscordName,EquipeName)"<<" -> Vous permet de donner des droits pour administrer votre équipe.\n\n";
	help << "!ow RemoveRight(DiscordName,EquipeName)"<<" -> Vous permet de retirer les droits à un des administrateurs de l'équipe(sauf Admin).\n\n";
	help << "!ow AddToEquipe(DiscordName,EquipeName)"<<" -> Ajoute l'utilisateur à l'équipe (vous devez avoir les droits sur l'équipe !).\n\n";
	help << "!ow RemoveFromEquipe(DiscordName,EquipeName)"<<" -> Retire l'utilisateur de l'équipe(vous devez avoir les droits sur l'équipe !).\n\n";
	help << "!ow RemoveMeFromEquipe(EquipeName)"<<" -> Vous retire de l'équipe.\n\n";
	help << "!ow Upd()"<<" -> Mets à jours votre profile niveau stats.\n\n";
	help << "!ow Eupd(EquipeName)"<<" -> Mets à jours toute l'équipe par rapport à l'Api(Vous devez être dans l'équipe.).\n\n";
	#ifdef flagGRAPHBUILDER_DB
	help << "!ow DrawStatsEquipe(ValueToStatify,EquipeName,graphParameterName)"<<" -> Dessine un graph par rapport à la value à afficher (rating, levels, medals, games) par rapport au paramètres chargés.\n\n";
	help << "!ow SaveGraphParam(NameOfSavedParam)"<<" -> Sauvegarde les paramètres actuel en BDD, n'oubliez pas le nom du paramétrage !\n\n";
	#endif
	#ifndef flagGRAPHBUILDER_DB
	help << "!ow DrawStatsEquipe(ValueToStatify,EquipeName)"<<" -> Dessine un graph par rapport à la value à afficher (rating, levels, medals, games).\n\n";
	#endif
	help << "!ow GraphProperties(Property,Value)"<<" ->Definie les proprieté pour le graph, Si aucun arguments.\n\n";
	help << "!ow GraphProperties()"<<" ->renvoie l'aide sur les proprietées.\n\n";
	help << "!ow Upr(rating)" <<" -> Update l'élo de la personne via le chiffre passé.\n\n";
	help <<"```\n\n";
	ptrBot->CreateMessage(ChannelLastMessage,help);
}

bool Discord_Overwatch::IsRegestered(String Id){
	for(Player &p : players){
		if(TrimBoth(p.GetDiscordId()).IsEqual(TrimBoth(Id))){
			return true;
		}
	}
	return false;
}

bool Discord_Overwatch::DoUserHaveRightOnTeam(String TeamName,String userId){
	for(Equipe& eq : equipes){
		if(TrimBoth(eq.GetEquipeName()).IsEqual(TrimBoth(TeamName))){
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
		if(TrimBoth(eq.GetEquipeName()).IsEqual(TrimBoth(teamName))){
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
		if(TrimBoth(p.GetDiscordId()).IsEqual(TrimBoth(Id))){
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
		if(TrimBoth(eq.GetEquipeName()).IsEqual(TrimBoth(teamName))){
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


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

void Discord_Overwatch::PrepareEvent(){
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("api"))CheckApi(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("register"))Register(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("remove"))DeleteProfil(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("createequipe"))CreateEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("removeequipe"))RemoveEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("giveright"))GiveRight(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("removeright"))RemoveRight(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("crud"))GetCRUD(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("reload"))ReloadCRUD(e);}); 
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("addtoequipe"))AddPersonToEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("removefromequipe"))RemovePersonFromEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("removemefromequipe"))RemoveMeFromEquipe(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("upd"))ForceUpdate(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("eupd"))ForceEquipeUpdate(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("graphproperties"))GraphProperties(e);}); 
	#ifdef flagGRAPHBUILDER_DB //Flag must be define to activate all DB func
		EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("drawstatsequipe"))DrawStatsEquipe(e);});
		EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("savegraphparam"))saveActualGraph(e);}); 
		EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("drawstats"))DrawStatsPlayer(e);}); 
	#else
		EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("drawstatsequipe"))DrawStatsEquipe(e);}); 
	#endif
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("startautoupdate"))startThread(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("stopautoupdate"))stopThread(e);});
	EventsMapMessageCreated.Add([&](ValueMap& e){if(NameOfFunction.IsEqual("autoupdate"))GetEtatThread(e);});
}

Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix):myGraph(1920,1080){
	name = _name;
	AddPrefix(_prefix);
	
	prepareOrLoadBDD();
	LoadMemoryCRUD();
	PrepareEvent();	
}

Discord_Overwatch::Discord_Overwatch(Upp::String _name,Vector<String> _prefix):myGraph(1920,1080){
	name = _name;
	AddPrefix(_prefix);
	
	prepareOrLoadBDD();
	LoadMemoryCRUD();
	PrepareEvent();	
}


void Discord_Overwatch::EventsMessageCreated(ValueMap payload){ //We admit BDD must be loaded to be active
	for(auto &e : EventsMapMessageCreated){
		if(bddLoaded){
			e(payload);
		}
		else{ 
			BotPtr->CreateMessage(ChannelLastMessage,"DataBase not loaded !"); 
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
				p.datas.Create(sql[0].Get<int64>() ,date,sql[3].Get<int64>(),sql[4].Get<int64>(),sql[5].Get<int64>(),sql[6].Get<int64>(),sql[7].Get<int64>(),sql[8].Get<int64>(),sql[9].Get<int64>(),sql[10].Get<int64>(),sql[11].Get<int64>(),sql[12].Get<int64>());
			
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
				SqlPerformScript(sqlite3,sch_OW.Upgrade());
			}	
			if(sch_OW.ScriptChanged(SqlSchema::ATTRIBUTES)){	
				SqlPerformScript(sqlite3,sch_OW.Attributes());
			}
			if(sch_OW.ScriptChanged(SqlSchema::CONFIG)) {
				SqlPerformScript(sqlite3,sch_OW.ConfigDrop());
				SqlPerformScript(sqlite3,sch_OW.Config());
			}
			sch_OW.SaveNormal();
			
		}
		Sql sql(sqlite3);
		sql.Execute("PRAGMA foreign_keys = ON;"); //Enable Foreign keys
		bddLoaded =true;
	}
	#undef MODEL
}

void Discord_Overwatch::ReloadCRUD(ValueMap& payload){
	LoadMemoryCRUD();
	BotPtr->CreateMessage(ChannelLastMessage,"Crud rechargé !");	
}


//Removed
/*
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
				BotPtr->CreateMessage(ChannelLastMessage,result);	
			}else{
				sql.Execute(querry);
				if(sql.WasError()){
					BotPtr->CreateMessage(ChannelLastMessage,sql.GetErrorCodeString());
					sql.ClearError();
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"Cela semble avoir marché ! (oui c'est précis lol)");
				}
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Bdd non chargée");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Erreur nombre d'argument incorrect, tu veux que j'execute du sql sans me donner de sql #Génie...```!ow ExecSql(SELECT * FROM MA_PUTAIN_DE_TABLE)```");	
	}
}*/

void Discord_Overwatch::GetCRUD(ValueMap& payload){
	BotPtr->CreateMessage(ChannelLastMessage, PrintMemoryCrude());	
}

//!ow Check
//!ow Check(btag: BASTION#21406)
void Discord_Overwatch::CheckApi(ValueMap& payload){
	String Btag ="";
	if(MessageArgs.Find("btag") != -1 &&  MessageArgs.Get("btag").GetTypeName().IsEqual("String")) Btag = MessageArgs.Get("btag").ToString();
	if(Btag.GetCount() ==0){
		Player* p = GetPlayersFromDiscordId(AuthorId);
		if(p){
			Btag = p->GetBattleTag();
		}
	}
	if(Btag.GetCount()>0){
		HttpRequest reqApi;
		Btag.Replace("#","-");
		reqApi.Url("https://ow-api.com/v1/stats/pc/euw/" + Btag + "/profile");
		reqApi.GET();
		auto json = ParseJSON(reqApi.Execute());
		if(IsNull(json["error"])){
			String stats = "```Détail pour " << Btag << " :\n\n";;
			stats<< "Parties gagnées : " << ((IsNull(json["gamesWon"]))?"error JSON":json["gamesWon"].ToString()) << "\n";
			if(  !IsNull(json["level"]) && !IsNull(json["prestige"])){
				int prestige = json["prestige"].Get<double>();
				int level = json["level"].Get<double>();
				int levels = 100 *prestige + level;
				stats<< "Level : " << AsString(levels) << "\n";
			}else{
				stats<< "Level : " << "Error JSON" << "\n";
			}
			stats<< "Parties gagnées : " << ((IsNull(json["gamesWon"]))?"error JSON":json["gamesWon"].ToString()) << "\n";
			stats<< "Rating global : " << ((IsNull(json["rating"]))?"error JSON":json["rating"].ToString()) << "\n";
			if(!IsNull(json["ratings"])){
				for(auto& leJson : json["ratings"]){
					if(!IsNull( leJson["role"]) && !IsNull(leJson["level"]) ){
						stats<< "Rating " << leJson["role"] << " : " <<  leJson["level"].ToString()<< "\n";
					}
				}
			}
			if(!IsNull(json["competitiveStats"]) && !IsNull(json["competitiveStats"]["awards"]) && !IsNull(json["competitiveStats"]["games"])){
				stats << "Compétitive Stats : " << "\n\n";
				stats << "Games Jouées:"<< ((IsNull(json["competitiveStats"]["games"]["played"]))?"Error JSON": json["competitiveStats"]["games"]["played"].ToString()) <<" ; Games Gagnées:" << ((IsNull(json["competitiveStats"]["games"]["won"]))?"Error JSON": json["competitiveStats"]["games"]["won"].ToString()) <<"\n";
				stats << "Nombres de cartes de fin de partie:" << ((IsNull(json["competitiveStats"]["awards"]["cards"]))?"Error JSON": json["competitiveStats"]["awards"]["cards"].ToString()) <<" ;Nombre de médails:" << ((IsNull(json["competitiveStats"]["awards"]["medals"]))?"Error JSON": json["competitiveStats"]["awards"]["medals"].ToString()) << " ; Bronze:" << ((IsNull(json["competitiveStats"]["awards"]["medalsBronze"]))?"Error JSON": json["competitiveStats"]["awards"]["medalsBronze"].ToString()) << " ;Silver:" << ((IsNull(json["competitiveStats"]["awards"]["medalsSilver"]))?"Error JSON": json["competitiveStats"]["awards"]["medalsSilver"].ToString()) << " ;Gold: " << ((IsNull(json["competitiveStats"]["awards"]["medalsGold"]))?"Error JSON": json["competitiveStats"]["awards"]["medalsGold"].ToString()) <<"\n";
			}
			if(!IsNull(json["quickPlayStats"])&& !IsNull(json["quickPlayStats"]["awards"]) && !IsNull(json["quickPlayStats"]["games"])){
				stats << "QuickPlay Stats : " << "\n\n";
				stats << "Games Jouées:"<< ((IsNull(json["quickPlayStats"]["games"]["played"]))?"Error JSON": json["quickPlayStats"]["games"]["played"].ToString()) <<" ; Games Gagnées:" << ((IsNull(json["quickPlayStats"]["games"]["won"]))?"Error JSON": json["quickPlayStats"]["games"]["won"].ToString()) <<"\n";
				stats << "Nombres de cartes de fin de partie:" << ((IsNull(json["quickPlayStats"]["awards"]["cards"]))?"Error JSON": json["quickPlayStats"]["awards"]["cards"].ToString()) <<" ;Nombre de médails:" << ((IsNull(json["quickPlayStats"]["awards"]["medals"]))?"Error JSON": json["quickPlayStats"]["awards"]["medals"].ToString()) << " ; Bronze:" << ((IsNull(json["quickPlayStats"]["awards"]["medalsBronze"]))?"Error JSON": json["quickPlayStats"]["awards"]["medalsBronze"].ToString()) << " ;Silver:" << ((IsNull(json["quickPlayStats"]["awards"]["medalsSilver"]))?"Error JSON": json["quickPlayStats"]["awards"]["medalsSilver"].ToString()) << " ;Gold: " << ((IsNull(json["quickPlayStats"]["awards"]["medalsGold"]))?"Error JSON": json["quickPlayStats"]["awards"]["medalsGold"].ToString()) <<"\n";
			
			}
			stats<< "```" <<"\n";
			BotPtr->CreateMessage(ChannelLastMessage,stats);
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"L'Api Overwatch ne connais pas " + Btag );
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Je ne vous connait pas ! Enrgistrez vous ou donné moi un BattleTag valide sous la forme suivante :!ow Check(Btag:Xemuth#2392)" );
	}
}

//!ow Register(btag:BASTION#21406;name:Xemuth)
void Discord_Overwatch::Register(ValueMap& payload){// The main idea is " You must be registered to be addable to an equipe or create Equipe	
	String Btag ="";
	String Name ="";

	if(MessageArgs.Find("btag") != -1 &&  MessageArgs.Get("btag").GetTypeName().IsEqual("String")) Btag = MessageArgs.Get("btag").ToString();
	if(MessageArgs.Find("name") != -1 &&  MessageArgs.Get("name").GetTypeName().IsEqual("String")) Name = MessageArgs.Get("name").ToString();
	
	if (Btag.GetCount() == 0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un battletag ! ```!ow register(btag:NattyRoots#12345; name:Natty)```");
		return;
	}
	if (Name.GetCount() == 0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom ! ```!ow register(btag:NattyRoots#1234; name:Natty)```");
		return;
	}
	try{
		Sql sql(sqlite3);
		if(sql*Insert(OW_PLAYERS)(BATTLE_TAG,Btag)(DISCORD_ID,AuthorId)(COMMUN_NAME,Name)){
			players.Add(Player(sql.GetInsertedId().Get<int64>(),Btag,AuthorId,Name));
			BotPtr->CreateMessage(ChannelLastMessage,"Enregistrement réussie");
			return;
		}
		BotPtr->CreateMessage(ChannelLastMessage,"Error Registering");
	}catch(...){
		BotPtr->CreateMessage(ChannelLastMessage,"Error Registering");
	}
}

//!ow Remove 
void Discord_Overwatch::DeleteProfil(ValueMap& payload){ //Remove user from the bdd 
	try{
		Player* p = GetPlayersFromDiscordId(AuthorId);
		if(p){
			int id = p->GetPlayerId();
			if(DeletePlayerById(id)){
				Sql sql(sqlite3);
				if(sql*Delete(OW_PLAYERS).Where(DISCORD_ID == id)){
					BotPtr->CreateMessage(ChannelLastMessage,"Supression reussie");
				}else{
					LoadMemoryCRUD();
					BotPtr->CreateMessage(ChannelLastMessage,"Erreur SQL");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"Impossible de vous supprimer !  Vous êtes leader d'une équipe");	
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Impossible de vous supprimer! Vous n'êtes même pas enregistré");	
		}
	}catch(...){
		BotPtr->CreateMessage(ChannelLastMessage,"Error deletion");
	}
}

//!ow createEquipe(team:Sombre est mon histoire)
void Discord_Overwatch::CreateEquipe(ValueMap& payload){ //To add an equipe you must be registered. when an equipe is created, only 
	String team="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(team.GetCount() > 0){
		if(IsRegestered(AuthorId)){
			try{			
				Sql sql(sqlite3);
				if(sql*Insert(OW_EQUIPES)(EQUIPE_NAME,team)){
					int idInserted = 0;
					idInserted =sql.GetInsertedId().Get<int64>();
					Player* p = GetPlayersFromDiscordId(AuthorId);
					int playerID  = 0;
					if(p){
						playerID= p->GetPlayerId();
					}
					if(idInserted !=0){
						if(sql*Insert(OW_EQUIPES_CREATOR)(EC_EQUIPES_ID,idInserted)(EC_PLAYER_ID,GetPlayersFromDiscordId(AuthorId)->GetPlayerId())(EC_ADMIN,true) && sql*Insert(OW_EQUIPES_PLAYERS)(EP_EQUIPE_ID,idInserted)(EP_PLAYER_ID,playerID)){
							auto& e= equipes.Add(Equipe(idInserted,team));
							e.creators.Add(EquipeCreator(playerID,true));
							e.playersId.Add(playerID);
							BotPtr->CreateMessage(ChannelLastMessage,"Ajout d'équipe reussie");
							return;
						}
					}
				}
				BotPtr->CreateMessage(ChannelLastMessage,"Erreur création team");
			}catch(...){
				BotPtr->CreateMessage(ChannelLastMessage,"Erreur création team");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Inscrit toi avant de créer une équipe !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Spécifie un nom de team ! Comme ça : !ow CreateEquipe(team:Sombre est mon histoire)");
	}
}

//!ow removeEquipe(team:Sombre est mon histoire)
void Discord_Overwatch::RemoveEquipe(ValueMap& payload){ //you must have the right to remove equipe
	String team="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(team.GetCount() > 0){
		if(IsRegestered(AuthorId)){
			if(DoEquipeExist(team)){
				if(DoUserHaveRightOnTeam(team, AuthorId)){
					Sql sql(sqlite3);
					int equipeId =GetEquipeByName(team)->GetEquipeId();
					if(sql*Delete(OW_EQUIPES).Where(EQUIPE_ID ==equipeId)){
						DeleteEquipeById(equipeId);
						BotPtr->CreateMessage(ChannelLastMessage,"L'Equipe à été supprimée !");
					}else{
						BotPtr->CreateMessage(ChannelLastMessage,"Erreur SQL !");
					}
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas le chef de team !");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"L'équipe n'existe pas !");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"T'es même pas inscrit va pas croire que tu peux faire ça tocard ! hinhin il a un boulot de merde en plus");	
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Spécifie un nom de team ! Comme ça : !ow RemoveEquipe(team:Sombre est mon histoire)");	
	}
}

//!ow GiveRight(discord:@NattyRoots;team:Sombre est mon histoire)
void Discord_Overwatch::GiveRight(ValueMap& payload){ //Allow equipe owner/ equipe righter to add person to the equipe
	String team="";
	String Discord="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(MessageArgs.Find("discord") != -1 &&  MessageArgs.Get("discord").GetTypeName().IsEqual("String")) Discord = MessageArgs.Get("discord").ToString();
	
	if (Discord.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner le nom Discord de quelqu'un ! ```!ow giveright(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom d'equipe pour que je l'ajoute ! ```!ow giveright(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	String idCite = Replace(Discord,Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
	if( IsRegestered(AuthorId)){
		if(IsRegestered(idCite)){
			if(DoEquipeExist(team)){
				if(DoUserHaveRightOnTeam(team, AuthorId)){
						if(!DoUserHaveRightOnTeam(team, idCite)){
							int id= GetPlayersFromDiscordId(idCite)->GetPlayerId();
							Sql sql(sqlite3);
							if(sql*Insert(OW_EQUIPES_CREATOR)(EC_EQUIPES_ID,GetEquipeByName(team)->GetEquipeId())(EC_PLAYER_ID,id )(EC_ADMIN,false ) && sql*Insert(OW_EQUIPES_PLAYERS)(EP_EQUIPE_ID,GetEquipeByName(team)->GetEquipeId())(EP_PLAYER_ID,id )){
								auto* equipe = GetEquipeByName(team);
								equipe->creators.Add(EquipeCreator(id,false));
								equipe->playersId.Add(id);
								BotPtr->CreateMessage(ChannelLastMessage,"Ajout de  "+ Discord + " dans " + team +" !");
								BotPtr->CreateMessage(ChannelLastMessage,"Droits donnés à "+ Discord + "!");
							}
						}else{
							BotPtr->CreateMessage(ChannelLastMessage, Discord + " à déjà les droits sur l'équipe !");
						}
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"Ta pas les droits, grosse merde hinhin !");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"L'équipe n'existe pas !");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,  Discord + " n'est pas enregistré !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
	}

}

//!ow RemoveRight(discord: @NattyRoots;team: Sombre est mon histoire)
void Discord_Overwatch::RemoveRight(ValueMap& payload){ //Remove equipe righter to one personne By discord ID
	String team="";
	String Discord="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(MessageArgs.Find("discord") != -1 &&  MessageArgs.Get("discord").GetTypeName().IsEqual("String")) Discord = MessageArgs.Get("discord").ToString();
	
	if (Discord.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner le nom Discord de quelqu'un ! ```!ow giveright(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom d'equipe pour que je l'ajoute ! ```!ow giveright(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	String idCite = Replace(Discord,Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
	if( IsRegestered(AuthorId)){
		if(IsRegestered(idCite)){
			if(DoEquipeExist(team)){
				if(DoUserHaveRightOnTeam(team, AuthorId)){
						int id= GetPlayersFromDiscordId(idCite)->GetPlayerId();
						int teamId= GetEquipeByName(team)->GetEquipeId();
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
										BotPtr->CreateMessage(ChannelLastMessage,"Destitution réussie !");
									}else{
										BotPtr->CreateMessage(ChannelLastMessage,"Erreur CRUD !");
									}
								}else{
									BotPtr->CreateMessage(ChannelLastMessage,"Erreur SQL !");
								}
							}else{
								BotPtr->CreateMessage(ChannelLastMessage,Discord + " est Admin de l'équipe !");
							}
						}else{
							BotPtr->CreateMessage(ChannelLastMessage,Discord + " ne possède pas de droit sur l'équipe !");
						}
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"You do not have right");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"Team do not exist !");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,Discord + " n'est pas enregistré !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
	}
}

//!ow AddToEquipe(discord: @NattyRoots;team: Sombre est mon histoire)
void Discord_Overwatch::AddPersonToEquipe(ValueMap& payload){ //To add a person to an equipe you must have the right to add it
	String team="";
	String Discord="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(MessageArgs.Find("discord") != -1 &&  MessageArgs.Get("discord").GetTypeName().IsEqual("String")) Discord = MessageArgs.Get("discord").ToString();
	if (Discord.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner le nom Discord de quelqu'un ! ```!ow AddToEquipe(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom d'equipe pour que je l'ajoute ! ```!ow AddToEquipe(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	if( IsRegestered(AuthorId)){
		String idCite = Replace(Discord,Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
		if(IsRegestered(idCite)){
			if(DoEquipeExist(team)){
				if(DoUserHaveRightOnTeam(team, AuthorId)){
					auto* player = GetPlayersFromDiscordId(idCite);
					auto* equipe = GetEquipeByName(team);
					if(!DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
						Sql sql(sqlite3);
						if(sql*Insert(OW_EQUIPES_PLAYERS)(EP_PLAYER_ID,player->GetPlayerId())(EP_EQUIPE_ID,equipe->GetEquipeId())){
							equipe->playersId.Add(player->GetPlayerId());
							BotPtr->CreateMessage(ChannelLastMessage,Discord +" a été ajouté à l'équipe !");
						}else{
							BotPtr->CreateMessage(ChannelLastMessage,"SQL Error !");
						}
					}else{
						BotPtr->CreateMessage(ChannelLastMessage,Discord + " est déjà dans l'équipe ! -_-'");
					}
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"Hinhin, ta pas les droits pour add quelqu'un à l'équipe !");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"Team do not exist !");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,  Discord + " n'est pas enregistré !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
	}
}

//!ow RemoveFromEquipe(discord: @NattyRoots;team:Sombre est mon histoire)
void Discord_Overwatch::RemovePersonFromEquipe(ValueMap& payload){ //Remove Person from and equipe (only righter can do it)
	String team="";
	String Discord="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(MessageArgs.Find("discord") != -1 &&  MessageArgs.Get("discord").GetTypeName().IsEqual("String")) Discord = MessageArgs.Get("discord").ToString();
	if (Discord.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner le nom Discord de quelqu'un ! ```!ow RemoveFromEquipe(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom d'equipe pour que je le vire ! ```!ow RemoveFromEquipe(discord:@NattyRoots;team:MonEquipe)```");
		return;
	}
	
	if( IsRegestered(AuthorId)){
		String idCite = Replace(Discord,Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
		if(IsRegestered(idCite)){
			if(DoEquipeExist(team)){
				if(DoUserHaveRightOnTeam(team, AuthorId)){
					auto* player = GetPlayersFromDiscordId(idCite);
					auto* equipe = GetEquipeByName(team);
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
							BotPtr->CreateMessage(ChannelLastMessage,Discord +" a été retiré de "+ team +" !");
						}else{
							BotPtr->CreateMessage(ChannelLastMessage,"SQL Error !");
						}
					}else{
						BotPtr->CreateMessage(ChannelLastMessage,Discord + " n'est pas dans l'équipe !");
					}
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"Hinhin, ta pas les droits pour delete quelqu'un à l'équipe !");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"L'équipe n'existe pas !");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,  Discord + " n'est pas enregistré !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
	}
}

//!ow RemoveMeFromEquipe(team:Sombre est mon histoire)
void Discord_Overwatch::RemoveMeFromEquipe(ValueMap& payload){ //Remove u from one of your equipe
	String team="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom d'equipe pour que je t'enlève ! ```!ow RemoveMeFromEquipe(team:MonEquipe)```");
		return;
	}
	
	if( IsRegestered(AuthorId)){
		if(DoEquipeExist(team)){
			auto* player = GetPlayersFromDiscordId(AuthorId);
			auto* equipe = GetEquipeByName(team);
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
					BotPtr->CreateMessage(ChannelLastMessage,"Vous avez été retiré de "+ team +" !");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas dans cette équipe !");
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Team do not exist !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
	}
}

//!ow upd
void Discord_Overwatch::ForceUpdate(ValueMap& payload){ //Force update, based on the personne who make the call
	if(IsRegestered(AuthorId)){
		Player* player = GetPlayersFromDiscordId(AuthorId);
		if(player){
			if(UpdatePlayer(player->GetPlayerId()))
				BotPtr->CreateMessage(ChannelLastMessage,player->GetBattleTag() + " a été mis à jour !");
			else 
				BotPtr->CreateMessage(ChannelLastMessage,"Mise à jours impossible ! :(");
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Oups ! je ne t'ai pas retrouvé dans ma mémoire ! RIP l'update");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");	
	}
}

//!ow Eupd(team:Sombre est mon histoire)
void Discord_Overwatch::ForceEquipeUpdate(ValueMap& payload){ // Idk if only ppl who have right on equipe must do it or letting it free.
	String team="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "Il faut me donner un nom d'equipe à update !```!ow Eupd(team:MonEquipe)```");
		return;
	}
	if(IsRegestered(AuthorId)){
		if(DoEquipeExist(team)){
			auto* player = GetPlayersFromDiscordId(AuthorId);
			auto* equipe = GetEquipeByName(team);
			if(player  && equipe){
				if(DoUserIsStillOnTeam(equipe->GetEquipeId(), player->GetPlayerId())){
					bool result=true;
					for(int& pid : equipe->playersId){
						if(!UpdatePlayer(pid))
							result=false;
					}
					BotPtr->CreateMessage(ChannelLastMessage,"Mise à jour de l'équipe terminée !!");
				}else{
					BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas dans cette équipe !");
				}
			}else{
				BotPtr->CreateMessage(ChannelLastMessage, player->GetCommunName()+ " est introuvable dans ma mémoire ! Ou Alors c'est l'équipe "+ team  +" qui n'existe pas !" );
			}
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"L'équipe n'existe pas !");
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Vous n'êtes pas enregistré !");
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
			
			int ratingDamage = 0;
			int ratingTank = 0;
			int ratingHeal = 0;
			for(auto& leJson : json["ratings"]){
				if(!IsNull( leJson["role"]) && !IsNull(leJson["level"]) ){
					String cas =  leJson["role"];
					if(cas.IsEqual("damage")){
						ratingDamage = leJson["level"].Get<double>();
					}else if(cas.IsEqual("tank")){
						ratingTank = leJson["level"].Get<double>();
					}else if(cas.IsEqual("support")){
						ratingHeal = leJson["level"].Get<double>();
					}
				}
			}
			
			int medalC = json["competitiveStats"]["awards"]["medals"].Get<double>();
			int medalB = json["competitiveStats"]["awards"]["medalsBronze"].Get<double>();
			int medalS = json["competitiveStats"]["awards"]["medalsSilver"].Get<double>();
			int medalG = json["competitiveStats"]["awards"]["medalsGold"].Get<double>();
			Sql sql(sqlite3);
			if(sql*Insert(OW_PLAYER_DATA)(DATA_PLAYER_ID,player->GetPlayerId())(RETRIEVE_DATE,date)(GAMES_PLAYED,gameP)(LEVEL,level)(RATING,rating)(RATING_DAMAGE,ratingDamage)(RATING_TANK,ratingTank)(RATING_HEAL,ratingHeal)(MEDALS_COUNT,medalC)(MEDALS_BRONZE,medalB)(MEDALS_SILVER,medalS)(MEDALS_GOLD,medalG)){
				auto& e = player->datas.Add(PlayerData(sql.GetInsertedId().Get<int64>(),date,gameP,level,rating,ratingDamage,ratingTank,ratingHeal,medalC,medalB,medalS,medalG));
				e.SetRatingDamage(ratingDamage);
				e.SetRatingTank(ratingTank);
				e.SetRatingHeal(ratingHeal);
				return true;
			}
			return false;
		}
		BotPtr->CreateMessage(ChannelLastMessage,player->GetBattleTag() + " est introuvable sur l'Api Overwatch !");
		return false;
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,+ "Ce joueur n'existe pas !");
		return false;
	}
}

#ifdef flagGRAPHBUILDER_DB //Flag must be define to activate all DB func

//!ow DrawStatsEquipe(critere:globalrating ; team:MonEquipe ;  graph:GraphName)
void Discord_Overwatch::DrawStatsEquipe(ValueMap& payload){ //Permet de déssiner le graph 
	Critere crit = AutoRating;
	Equipe* equ = nullptr;
	String team="";
	String critere="";
	String graph ="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(MessageArgs.Find("critere") != -1 &&  MessageArgs.Get("critere").GetTypeName().IsEqual("String")) critere = MessageArgs.Get("critere").ToString();
	if(MessageArgs.Find("graph") != -1 &&  MessageArgs.Get("graph").GetTypeName().IsEqual("String")) graph = MessageArgs.Get("graph").ToString();
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "DrawStatsEquipe permet de dessiner un graph de l'évolution de l'équipe selons certains critères qui sont GlobalRating, HealRating,TankRating, DpsRating, AutoRating. De plus, le graph généré est customisable via un ensemble de paramètre de Graph. ```!ow DrawStatsEquipe(critere:AutoRating ; team:MonEquipe ;  graph:GraphName)``` Tous les champs sauf team sont optionnel. Par défault le critère est AutoRating et les paramètres de graph sont les derniers utilisés.");
		return;
	}
	equ = GetEquipeByName(ToLower(team));
	if(!equ){
		BotPtr->CreateMessage(ChannelLastMessage, "Equipe introuvable !");
		return;
	}
	
	if(critere.GetCount()>0){
		String buffer =ToLower(critere);
		if( Levensthein_Distance(buffer,"globalrating") <=2){
			crit = GlobalRating;
		}
		else if( Levensthein_Distance(buffer,"healrating") <=2){
			crit = HealRating;
		}
		else if( Levensthein_Distance(buffer,"tankrating") <=2){
			crit = TankRating;
		}
		else if( Levensthein_Distance(buffer,"autorating") <=2){
			crit = AutoRating;
		}
		else if( Levensthein_Distance(buffer,"dpsrating") <=2){
			crit = DpsRating;
		}
	}
	if(graph.GetCount() > 0){
		if(myGraph.LoadGraphParamFromBdd(ToLower(graph)))
			BotPtr->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
		else
			BotPtr->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
	}
	
	myGraph.ClearData();
	myGraph.DefineXName( "Date");
	myGraph.DefineYName("Rating");
	
	typedef int (PlayerData::*RetrieveTheData)();
	RetrieveTheData function;
	if(crit == GlobalRating){
		myGraph.SetGraphName("Evolution du Rang global pour " + team);
		function = &PlayerData::GetRating;
	}else if(crit == HealRating){
		myGraph.SetGraphName("Evolution du Rang Heal pour " + team);
		function = &PlayerData::GetRatingHeal;
	}else if(crit == TankRating){
		myGraph.SetGraphName("Evolution du Rang Tank pour " + team);
		function = &PlayerData::GetRatingTank;
		
	}else if(crit == AutoRating){
		myGraph.SetGraphName("Evolution du Meilleur rang de chacun pour " + team);
	}else if(crit == DpsRating){
		myGraph.SetGraphName("Evolution du Rang dps pour " + team);
		function = &PlayerData::GetRatingDamage;
	}
	int cpt = 0;
	for(int& pid : equ->playersId){
		Player* atmP = GetPlayersFromId(pid);
		if(atmP){
			String FocussingRate ="";
			if(crit==AutoRating){
				if(atmP->datas.GetCount() > 0){
					PlayerData& LastData = atmP->datas[atmP->datas.GetCount()-1];
					if(LastData.GetRatingDamage() > LastData.GetRatingTank()  && LastData.GetRatingDamage() > LastData.GetRatingHeal()){
						//Damage
						FocussingRate = "Damage";
						function = &PlayerData::GetRatingDamage;
					}else if(LastData.GetRatingTank() > LastData.GetRatingDamage()  && LastData.GetRatingTank() > LastData.GetRatingHeal()){
						//Tank
						FocussingRate = "Tank"; 
						function = &PlayerData::GetRatingTank;
					}else if(LastData.GetRatingHeal() > LastData.GetRatingDamage()  && LastData.GetRatingHeal() > LastData.GetRatingTank()){
						//heal
						FocussingRate = "Heal";
						function = &PlayerData::GetRatingHeal;
					}
				}
			}
			String name = (((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag()) + FocussingRate );
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
					myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(CALL_MEMBER_FN(pd,function)()),&myGraph[cpt]));
				}else{
					myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(CALL_MEMBER_FN(pd,function)()),&myGraph[cpt]));
				}
				lastDate = pd.GetRetrieveDate();
				cpt2++;
			}
			cpt++;
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Un des joueurs de l'équipe est introuvable en mémoire :/");	
		}
	}
	PNGEncoder png;
	png.SaveFile("temp.png", myGraph.DrawGraph());
	BotPtr->SendFile(ChannelLastMessage,"","","temp.png");
}

//!ow SaveGraph(name:My Graph name)
void Discord_Overwatch::saveActualGraph(ValueMap& payload){
	String name ="";
	if(MessageArgs.Find("name") != -1 &&  MessageArgs.Get("name").GetTypeName().IsEqual("String")) name = MessageArgs.Get("name").ToString();
	if(name.GetCount()> 0){
		if(myGraph.SaveGraphParamInBDD(ToLower(name))!=-1){
			BotPtr->CreateMessage(ChannelLastMessage,"Paramètres sauvegardés.");	
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Erreur lors de l'enregistrement des paramètres !");		
		}
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Précise le nom de la conf STP```!ow SaveGraph(name:Ma conf)```");		
	}
}

// !ow DrawStats( (Date Debut,Date fin) || Nombre de point,Type de Ranking, BattleTag, Paramètre de Graph) --> Avant 

// !ow DrawStats(DateDebut: 27/06/2019; DateFin:30/06/2019 ; NbPointMax:20; critere:globalRating ; discord: @NattyRoots ; graph:My Graph Name) --> Après
// Tous les critères sont optionnel. Par défault le Rating est AutoRating

void Discord_Overwatch::DrawStatsPlayer(ValueMap& payload){
	Date DateDebut;
	bool DateD=false;
	Date DateFin;
	bool DateF=false;
	int NbPointMax=0; //If Def to 0 it's ignored
	String discord=AuthorId;
	String critere="";
	String graph ="";
	if(MessageArgs.Find("discord") != -1 &&  MessageArgs.Get("discord").GetTypeName().IsEqual("String")) discord = MessageArgs.Get("discord").ToString();
	if(MessageArgs.Find("critere") != -1 &&  MessageArgs.Get("critere").GetTypeName().IsEqual("String")) critere = MessageArgs.Get("critere").ToString();
	if(MessageArgs.Find("graph") != -1 &&  MessageArgs.Get("graph").GetTypeName().IsEqual("String")) graph = MessageArgs.Get("graph").ToString();
	if(MessageArgs.Find("nbpointmax") != -1 &&  MessageArgs.Get("nbpointmax").GetTypeName().IsEqual("int")) NbPointMax = MessageArgs.Get("nbpointmax").Get<int>();
	if(MessageArgs.Find("datedebut") != -1 &&  MessageArgs.Get("datedebut").GetTypeName().IsEqual("Date")){
		DateDebut = MessageArgs.Get("datedebut").Get<Date>();
		DateD=true;
	}
	if(MessageArgs.Find("datefin") != -1 &&  MessageArgs.Get("datefin").GetTypeName().IsEqual("Date")){
		DateDebut = MessageArgs.Get("datefin").Get<Date>();
		DateF=true;
	}
	
	GraphDotCloud test(1920,1080);
	Player* playerToDraw=nullptr;
	Critere crit = AutoRating;
	//Gestion du player ciblé
	String idCite = Replace(discord,Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
	playerToDraw=GetPlayersFromDiscordId(idCite);
	if(!playerToDraw){
		if(MessageArgs.Find("discord")!= -1){
			BotPtr->CreateMessage(ChannelLastMessage, discord + " n'est pas enregistré !");
		}else{
			BotPtr->CreateMessage(ChannelLastMessage, "Vous n'êtes pas enregistré !");
		}
		return;
	}
	//Gestion des Dates
	if(DateD && DateF){
		if(	DateDebut > DateFin){
			BotPtr->CreateMessage(ChannelLastMessage, "Warning : la date de début est plus récente que la date de Fin ?! Les dates seront ignorées");
			DateD = false;
			DateF = false;
		}
	}else if((MessageArgs.Find("datedebut") != -1 &&  MessageArgs.Get("datedebut").GetTypeName().IsEqual("String")) || (MessageArgs.Find("datefin") != -1 &&  MessageArgs.Get("datefin").GetTypeName().IsEqual("String"))){
		BotPtr->CreateMessage(ChannelLastMessage, "Warning : Le format des dates rentrées est invalide. Les dates seront ignorées. Pour rappel le format valide est : dd/MM/yyyy");
	}
	
	//Gestion du critère
	if(critere.GetCount()>0){
		String buffer =ToLower(critere);
		if( Levensthein_Distance(buffer,"globalrating") <=2){
			crit = GlobalRating;
		}
		else if( Levensthein_Distance(buffer,"healrating") <=2){
			crit = HealRating;
		}
		else if( Levensthein_Distance(buffer,"tankrating") <=2){
			crit = TankRating;
		}
		else if( Levensthein_Distance(buffer,"autorating") <=2){
			crit = AutoRating;
		}
		else if( Levensthein_Distance(buffer,"dpsrating") <=2){
			crit = DpsRating;
		}
	}
	//Gestion du graph
	if(graph.GetCount() > 0){
		if(test.LoadGraphParamFromBdd(ToLower(graph)))
			BotPtr->CreateMessage(ChannelLastMessage,"Chargement des paramètres reussi.");
		else
			BotPtr->CreateMessage(ChannelLastMessage,"Impossible de charger ces paramètres...");
	}
	
	typedef int (PlayerData::*RetrieveTheData)();
	String FocussingRate ="";
	RetrieveTheData function;
	if(crit == GlobalRating){
		myGraph.SetGraphName("Evolution du Rang global pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRating;
	}else if(crit == HealRating){
		myGraph.SetGraphName("Evolution du Rang Heal pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRatingHeal;
	}else if(crit == TankRating){
		myGraph.SetGraphName("Evolution du Rang Tank pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRatingTank;
	}else if(crit == AutoRating){
		myGraph.SetGraphName("Evolution du meilleur rang pour " + playerToDraw->GetCommunName());
		
		if(playerToDraw->datas.GetCount() > 0){
			PlayerData& LastData = playerToDraw->datas[playerToDraw->datas.GetCount()-1];
			if(LastData.GetRatingDamage() > LastData.GetRatingTank()  && LastData.GetRatingDamage() > LastData.GetRatingHeal()){
				//Damage
				FocussingRate = "Damage";
				function = &PlayerData::GetRatingDamage;
			}else if(LastData.GetRatingTank() > LastData.GetRatingDamage()  && LastData.GetRatingTank() > LastData.GetRatingHeal()){
				//Tank
				FocussingRate = "Tank"; 
				function = &PlayerData::GetRatingTank;
			}else if(LastData.GetRatingHeal() > LastData.GetRatingDamage()  && LastData.GetRatingHeal() > LastData.GetRatingTank()){
				//heal
				FocussingRate = "Heal";
				function = &PlayerData::GetRatingHeal;
			}
		}
		
	}else if(crit == DpsRating){
		myGraph.SetGraphName("Evolution du Rang dps pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRatingDamage;
	}
	test.DefineXValueType(ValueTypeEnum::INT);
	test.DefineXName("Enregistrement");
	test.DefineYName("Elo " + FocussingRate );
	bool MaxPoint = (NbPointMax > 0);
	int cpt = 0;
	test.AddCourbe(Courbe(playerToDraw->GetCommunName(),ValueTypeEnum::INT,ValueTypeEnum::INT,test.GenerateColor()));
	auto& r = test[0];
	r.ShowDot(true);
	int NumberPoint = 0;
	for(PlayerData& pd : playerToDraw->datas){
		if(MaxPoint){
			if((playerToDraw->datas.GetCount() - NumberPoint) <=NbPointMax){
				if(DateF && DateD){
					if(DateDebut <= pd.GetRetrieveDate() && DateFin >= pd.GetRetrieveDate()){
						r.AddDot(Dot(Value(cpt),Value(CALL_MEMBER_FN(pd,function)()),&r));
						cpt++;
					}
				}else{
					r.AddDot(Dot(Value(cpt),Value(CALL_MEMBER_FN(pd,function)()),&r));
					cpt++;
				}
			}
		}else{
			r.AddDot(Dot(Value(cpt),Value(CALL_MEMBER_FN(pd,function)()),&r));
			cpt++;
		}
		NumberPoint++;
	}
	PNGEncoder png;
	png.SaveFile("temp2.png", test.DrawGraph());
	BotPtr->SendFile(ChannelLastMessage,"","","temp2.png");
}

#else

//!ow DrawStatsPlayer(rating,[@BASTION#21406],20) I actually want 20 last elo change
void Discord_Overwatch::DrawStatsPlayer(ValueMap& payload){
	Date DateDebut;
	bool DateD=false;
	Date DateFin;
	bool DateF=false;
	int NbPointMax=0; //If Def to 0 it's ignored
	String discord=AuthorId;
	String critere="";
	String graph ="";
	if(MessageArgs.Find("discord") != -1 &&  MessageArgs.Get("discord").GetTypeName().IsEqual("String")) discord = MessageArgs.Get("discord").ToString();
	if(MessageArgs.Find("critere") != -1 &&  MessageArgs.Get("critere").GetTypeName().IsEqual("String")) critere = MessageArgs.Get("critere").ToString();
	if(MessageArgs.Find("graph") != -1 &&  MessageArgs.Get("graph").GetTypeName().IsEqual("String")) graph = MessageArgs.Get("graph").ToString();
	if(MessageArgs.Find("nbpointmax") != -1 &&  MessageArgs.Get("nbpointmax").GetTypeName().IsEqual("int")) NbPointMax = MessageArgs.Get("nbpointmax").Get<int>();
	if(MessageArgs.Find("datedebut") != -1 &&  MessageArgs.Get("datedebut").GetTypeName().IsEqual("Date")){
		DateDebut = MessageArgs.Get("datedebut").Get<Date>();
		DateD=true;
	}
	if(MessageArgs.Find("datefin") != -1 &&  MessageArgs.Get("datefin").GetTypeName().IsEqual("Date")){
		DateDebut = MessageArgs.Get("datefin").Get<Date>();
		DateF=true;
	}
	
	GraphDotCloud test(1920,1080);
	Player* playerToDraw=nullptr;
	Critere crit = AutoRating;
	//Gestion du player ciblé
	String idCite = Replace(discord,Vector<String>{"<",">","@","!"},Vector<String>{"","","",""});
	playerToDraw=GetPlayersFromDiscordId(idCite);
	if(!playerToDraw){
		if(MessageArgs.Find("discord")!= -1){
			BotPtr->CreateMessage(ChannelLastMessage, discord + " n'est pas enregistré !");
		}else{
			BotPtr->CreateMessage(ChannelLastMessage, "Vous n'êtes pas enregistré !");
		}
		return;
	}
	//Gestion des Dates
	if(DateD && DateF){
		if(	DateDebut > DateFin){
			BotPtr->CreateMessage(ChannelLastMessage, "Warning : la date de début est plus récente que la date de Fin ?! Les dates seront ignorées");
			DateD = false;
			DateF = false;
		}
	}else if((MessageArgs.Find("datedebut") != -1 &&  MessageArgs.Get("datedebut").GetTypeName().IsEqual("String")) || (MessageArgs.Find("datefin") != -1 &&  MessageArgs.Get("datefin").GetTypeName().IsEqual("String"))){
		BotPtr->CreateMessage(ChannelLastMessage, "Warning : Le format des dates rentrées est invalide. Les dates seront ignorées. Pour rappel le format valide est : dd/MM/yyyy");
	}
	
	//Gestion du critère
	if(critere.GetCount()>0){
		String buffer =ToLower(critere);
		if( Levensthein_Distance(buffer,"globalrating") <=2){
			crit = GlobalRating;
		}
		else if( Levensthein_Distance(buffer,"healrating") <=2){
			crit = HealRating;
		}
		else if( Levensthein_Distance(buffer,"tankrating") <=2){
			crit = TankRating;
		}
		else if( Levensthein_Distance(buffer,"autorating") <=2){
			crit = AutoRating;
		}
		else if( Levensthein_Distance(buffer,"dpsrating") <=2){
			crit = DpsRating;
		}
	}
	//Gestion du graph
	if(graph.GetCount() > 0){
		BotPtr->CreateMessage(ChannelLastMessage,"Impossible de charder des paramètres de graph ! La fonction est désactivée sur votre version de SmartUppBot");
	}
	
	typedef int (PlayerData::*RetrieveTheData)();
	String FocussingRate ="";
	RetrieveTheData function;
	if(crit == GlobalRating){
		myGraph.SetGraphName("Evolution du Rang global pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRating;
	}else if(crit == HealRating){
		myGraph.SetGraphName("Evolution du Rang Heal pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRatingHeal;
	}else if(crit == TankRating){
		myGraph.SetGraphName("Evolution du Rang Tank pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRatingTank;
	}else if(crit == AutoRating){
		myGraph.SetGraphName("Evolution du meilleur rang pour " + playerToDraw->GetCommunName());
		
		if(playerToDraw->datas.GetCount() > 0){
			PlayerData& LastData = playerToDraw->datas[playerToDraw->datas.GetCount()-1];
			if(LastData.GetRatingDamage() > LastData.GetRatingTank()  && LastData.GetRatingDamage() > LastData.GetRatingHeal()){
				//Damage
				FocussingRate = "Damage";
				function = &PlayerData::GetRatingDamage;
			}else if(LastData.GetRatingTank() > LastData.GetRatingDamage()  && LastData.GetRatingTank() > LastData.GetRatingHeal()){
				//Tank
				FocussingRate = "Tank"; 
				function = &PlayerData::GetRatingTank;
			}else if(LastData.GetRatingHeal() > LastData.GetRatingDamage()  && LastData.GetRatingHeal() > LastData.GetRatingTank()){
				//heal
				FocussingRate = "Heal";
				function = &PlayerData::GetRatingHeal;
			}
		}
		
	}else if(crit == DpsRating){
		myGraph.SetGraphName("Evolution du Rang dps pour " + playerToDraw->GetCommunName());
		function = &PlayerData::GetRatingDamage;
	}
	test.DefineXValueType(ValueTypeEnum::INT);
	test.DefineXName("Enregistrement");
	test.DefineYName("Elo " + FocussingRate );
	bool MaxPoint = (NbPointMax > 0);
	int cpt = 0;
	test.AddCourbe(Courbe(playerToDraw->GetCommunName(),ValueTypeEnum::INT,ValueTypeEnum::INT,test.GenerateColor()));
	auto& r = test[0];
	r.ShowDot(true);
	int NumberPoint = 0;
	for(PlayerData& pd : playerToDraw->datas){
		if(MaxPoint){
			if((playerToDraw->datas.GetCount() - NumberPoint) <=NbPointMax){
				if(DateF && DateD){
					if(DateDebut <= pd.GetRetrieveDate() && DateFin >= pd.GetRetrieveDate()){
						r.AddDot(Dot(Value(cpt),Value(CALL_MEMBER_FN(pd,function)()),&r));
						cpt++;
					}
				}else{
					r.AddDot(Dot(Value(cpt),Value(CALL_MEMBER_FN(pd,function)()),&r));
					cpt++;
				}
			}
		}else{
			r.AddDot(Dot(Value(cpt),Value(CALL_MEMBER_FN(pd,function)()),&r));
			cpt++;
		}
		NumberPoint++;
	}
	PNGEncoder png;
	png.SaveFile("temp2.png", test.DrawGraph());
	BotPtr->SendFile(ChannelLastMessage,"","","temp2.png");
}

//!ow DrawStatsEquipe(rating,Sombre est mon histoire)
void Discord_Overwatch::DrawStatsEquipe(ValueMap& payload){ //Permet de déssiner le graph 
		Critere crit = AutoRating;
	Equipe* equ = nullptr;
	String team="";
	String critere="";
	String graph ="";
	if(MessageArgs.Find("team") != -1 &&  MessageArgs.Get("team").GetTypeName().IsEqual("String")) team = MessageArgs.Get("team").ToString();
	if(MessageArgs.Find("critere") != -1 &&  MessageArgs.Get("critere").GetTypeName().IsEqual("String")) critere = MessageArgs.Get("critere").ToString();
	if(MessageArgs.Find("graph") != -1 &&  MessageArgs.Get("graph").GetTypeName().IsEqual("String")) graph = MessageArgs.Get("graph").ToString();
	if (team.GetCount()==0){
		BotPtr->CreateMessage(ChannelLastMessage, "DrawStatsEquipe permet de dessiner un graph de l'évolution de l'équipe selons certains critères qui sont GlobalRating, HealRating,TankRating, DpsRating, AutoRating. De plus, le graph généré est customisable via un ensemble de paramètre de Graph. ```!ow DrawStatsEquipe(critere:AutoRating ; team:MonEquipe ;  graph:myProperties)``` Tous les champs sauf team sont optionnel. Par défault le critère est AutoRating et les paramètres de graph sont les derniers utilisés.");
		return;
	}
	equ = GetEquipeByName(ToLower(team));
	if(!equ){
		BotPtr->CreateMessage(ChannelLastMessage, "Equipe introuvable !");
		return;
	}
	
	if(critere.GetCount()>0){
		String buffer =ToLower(critere);
		if( Levensthein_Distance(buffer,"globalrating") <=2){
			crit = GlobalRating;
		}
		else if( Levensthein_Distance(buffer,"healrating") <=2){
			crit = HealRating;
		}
		else if( Levensthein_Distance(buffer,"tankrating") <=2){
			crit = TankRating;
		}
		else if( Levensthein_Distance(buffer,"autorating") <=2){
			crit = AutoRating;
		}
		else if( Levensthein_Distance(buffer,"dpsrating") <=2){
			crit = DpsRating;
		}
	}
	if(graph.GetCount() > 0){
		BotPtr->CreateMessage(ChannelLastMessage,"Impossible de charder des paramètres de graph ! La fonction est désactivée sur votre version de SmartUppBot");
	}
	
	myGraph.ClearData();
	myGraph.DefineXName( "Date");
	myGraph.DefineYName("Rating");
	typedef int (PlayerData::*RetrieveTheData)();
	RetrieveTheData function;
	if(crit == GlobalRating){
		myGraph.SetGraphName("Evolution du Rang global pour " + team);
		function = &PlayerData::GetRating;
	}else if(crit == HealRating){
		myGraph.SetGraphName("Evolution du Rang Heal pour " + team);
		function = &PlayerData::GetRatingHeal;
	}else if(crit == TankRating){
		myGraph.SetGraphName("Evolution du Rang Tank pour " + team);
		function = &PlayerData::GetRatingTank;
		
	}else if(crit == AutoRating){
		myGraph.SetGraphName("Evolution du Meilleur rang de chacun pour " + team);
	}else if(crit == DpsRating){
		myGraph.SetGraphName("Evolution du Rang dps pour " + team);
		function = &PlayerData::GetRatingDamage;
	}
	int cpt = 0;
	for(int& pid : equ->playersId){
		Player* atmP = GetPlayersFromId(pid);
		if(atmP){
			String FocussingRate ="";
			if(crit==AutoRating){
				if(atmP->datas.GetCount() > 0){
					PlayerData& LastData = atmP->datas[atmP->datas.GetCount()-1];
					if(LastData.GetRatingDamage() > LastData.GetRatingTank()  && LastData.GetRatingDamage() > LastData.GetRatingHeal()){
						//Damage
						FocussingRate = "Damage";
						function = &PlayerData::GetRatingDamage;
					}else if(LastData.GetRatingTank() > LastData.GetRatingDamage()  && LastData.GetRatingTank() > LastData.GetRatingHeal()){
						//Tank
						FocussingRate = "Tank"; 
						function = &PlayerData::GetRatingTank;
					}else if(LastData.GetRatingHeal() > LastData.GetRatingDamage()  && LastData.GetRatingHeal() > LastData.GetRatingTank()){
						//heal
						FocussingRate = "Heal";
						function = &PlayerData::GetRatingHeal;
					}
				}
			}
			String name = (((atmP->GetCommunName().GetCount()> 0)? atmP->GetCommunName():atmP->GetBattleTag()) + FocussingRate );
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
					myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(CALL_MEMBER_FN(pd,function)()),&myGraph[cpt]));
				}else{
					myGraph[cpt].AddDot(Dot(Value(pd.GetRetrieveDate()),Value(CALL_MEMBER_FN(pd,function)()),&myGraph[cpt]));
				}
				lastDate = pd.GetRetrieveDate();
				cpt2++;
			}
			cpt++;
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Un des joueurs de l'équipe est introuvable en mémoire :/");	
		}
	}
	PNGEncoder png;
	png.SaveFile("temp.png", myGraph.DrawGraph());
	BotPtr->SendFile(ChannelLastMessage,"","","temp.png");
}
#endif

void Discord_Overwatch::GraphProperties(ValueMap& payload){
	int nbModif =0;
	
	bool GraphName=false;
	bool LegendsOfcourbes=false;
	bool ValueOfDot=false;
	bool ShowValueOnAxis=false;
	bool SignIt=false;
	
	bool ActivateMaxDatePadding=false;
	int MaxDatePadding=-1;
	
	bool ActivateLowestAxisY=false;
	bool ActivateHighestAxisY=false;
	int LowestAxisY=-1;
	int HighestAxisY=-1;
	
	String AlphaColor="";
	String MainColor="";

	if(MessageArgs.Find("graphname") != -1 &&  MessageArgs.Get("graphname").GetTypeName().IsEqual("bool")){
		GraphName = MessageArgs.Get("graphname").Get<bool>();
		myGraph.ShowGraphName(GraphName);
		nbModif++;
	}
	if(MessageArgs.Find("legendsofcourbes") != -1 &&  MessageArgs.Get("legendsofcourbes").GetTypeName().IsEqual("bool")){
		LegendsOfcourbes = MessageArgs.Get("legendsofcourbes").Get<bool>();
		myGraph.ShowLegendsOfCourbes(LegendsOfcourbes);
		nbModif++;
	}
	if(MessageArgs.Find("valueofdot") != -1 &&  MessageArgs.Get("valueofdot").GetTypeName().IsEqual("bool")){
		ValueOfDot = MessageArgs.Get("valueofdot").Get<bool>();
		myGraph.ShowValueOfDot(ValueOfDot);
		nbModif++;
	}
	if(MessageArgs.Find("showvalueonaxis") != -1 &&  MessageArgs.Get("showvalueonaxis").GetTypeName().IsEqual("bool")){
		ShowValueOnAxis = MessageArgs.Get("showvalueonaxis").Get<bool>();
		myGraph.ShowValueOnAxis(ShowValueOnAxis);
		nbModif++;
	}
	if(MessageArgs.Find("signit") != -1 &&  MessageArgs.Get("signit").GetTypeName().IsEqual("bool")){
		SignIt = MessageArgs.Get("signit").Get<bool>();
		myGraph.SignIt(SignIt);
		nbModif++;
	}
	if(MessageArgs.Find("activatemaxdatapadding") != -1 &&  MessageArgs.Get("activatemaxdatapadding").GetTypeName().IsEqual("bool")){
		ActivateMaxDatePadding = MessageArgs.Get("activatemaxdatapadding").Get<bool>();
		myGraph.ActivateMaxDatePadding(ActivateMaxDatePadding);
		nbModif++;
	}
	if(MessageArgs.Find("maxdatepadding") != -1 &&  MessageArgs.Get("maxdatepadding").GetTypeName().IsEqual("int")){
		MaxDatePadding = MessageArgs.Get("maxdatepadding").Get<int>();
		myGraph.SetMaxDatePadding(MaxDatePadding);
		nbModif++;
	}
	if(MessageArgs.Find("activatelowestaxisy") != -1 &&  MessageArgs.Get("activatelowestaxisy").GetTypeName().IsEqual("bool")){
		ActivateLowestAxisY = MessageArgs.Get("activatelowestaxisy").Get<bool>();
		myGraph.SetActivatedSpecifiedLowestAxisY(ActivateLowestAxisY);
		nbModif++;
	}
	if(MessageArgs.Find("activatehighestaxisy") != -1 &&  MessageArgs.Get("activatehighestaxisy").GetTypeName().IsEqual("bool")){
		ActivateHighestAxisY = MessageArgs.Get("activatehighestaxisy").Get<bool>();
		myGraph.SetActivatedSpecifiedHighestAxisY(ActivateHighestAxisY);
		nbModif++;
	}
	if(MessageArgs.Find("lowestaxisy") != -1 &&  MessageArgs.Get("lowestaxisy").GetTypeName().IsEqual("int")){
		LowestAxisY = MessageArgs.Get("lowestaxisy").Get<int>();
		myGraph.SetSpecifiedLowestStartingNumberAxisY(LowestAxisY);
		nbModif++;
	}
    if(MessageArgs.Find("hightestaxisy") != -1 &&  MessageArgs.Get("hightestaxisy").GetTypeName().IsEqual("int")){
    	HighestAxisY = MessageArgs.Get("hightestaxisy").Get<int>();
    	myGraph.SetSpecifiedHighestStartingNumberAxisY(HighestAxisY);
    	nbModif++;
    }
	if(MessageArgs.Find("alphacolor") != -1 &&  MessageArgs.Get("alphacolor").GetTypeName().IsEqual("String")){
		AlphaColor = MessageArgs.Get("alphacolor").ToString();
		Vector<String> rgb = Split(AlphaColor,",");
		Value r =ResolveType(rgb[0]);
		Value g =ResolveType(rgb[1]);
		Value b =ResolveType(rgb[2]);
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
			nbModif++;
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Warning : Les Chiffres spécifier pour AlphaColor sont invalide !");
		}
	}
	if(MessageArgs.Find("maincolor") != -1 &&  MessageArgs.Get("maincolor").GetTypeName().IsEqual("String")){
		MainColor = MessageArgs.Get("maincolor").ToString();
		Vector<String> rgb = Split(AlphaColor,",");
		Value r =ResolveType(rgb[0]);
		Value g =ResolveType(rgb[1]);
		Value b =ResolveType(rgb[2]);
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
			nbModif++;
		}else{
			BotPtr->CreateMessage(ChannelLastMessage,"Warning : Les Chiffres spécifier pour MainColor sont invalide !");
		}
		nbModif++;
	}
	if(nbModif ==0){
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
		BotPtr->CreateMessage(ChannelLastMessage,help);
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,nbModif +" Modification(s) effectuée(s) sur le graph !");
	}
}

bool Discord_Overwatch::GetEtatThread(ValueMap& payload){//used to return if thread is start or stopped
	if(autoUpdate.IsOpen()){
		HowManyTimeBeforeUpdate =true;
		BotPtr->CreateMessage(ChannelLastMessage,"Actuellement, l'autoUpdater est lancé !");	
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"Actuellement, l'autoUpdater est coupé !");	
	}
	return true;
}

void Discord_Overwatch::startThread(ValueMap& payload){
	if(autoUpdate.IsOpen()){
		BotPtr->CreateMessage(ChannelLastMessage,"L'autoUpdated est déjà lancé !");	
	}else{
		autoUpdate.Run([&](){threadStarted =true;for(;;){

								for(int e = 0; e < 3600 ; e++){
									if(!threadStarted) break;
									if(HowManyTimeBeforeUpdate){ 
										BotPtr->CreateMessage(ChannelLastMessage,"Prochaine mise à jours dans : "+ AsString((3600-e)/60)+"min");	
										HowManyTimeBeforeUpdate=false;
									}
									Sleep(1000);
								}
								if(!threadStarted) break;
								for(Player &p : players){
									if(!threadStarted) break;
									UpdatePlayer(p.GetPlayerId());
								}	
								BotPtr->CreateMessage(ChannelLastMessage,"Mise à jour en terminée !");
							}
							Cout() << "fin du thread"<<"\n";
							return;});
		BotPtr->CreateMessage(ChannelLastMessage,"L'autoUpdated est désormait lancé !");
	}
}
void Discord_Overwatch::stopThread(ValueMap& payload){
	if(autoUpdate.IsOpen()){
		threadStarted=false;
		autoUpdate.Wait();
		BotPtr->CreateMessage(ChannelLastMessage,"L'autoUpdated est désormait coupé !");	
	}else{
		BotPtr->CreateMessage(ChannelLastMessage,"L'autoUpdated n'est pas lancé !");	
	}
}


void Discord_Overwatch::Help(ValueMap& payload){
	String help ="";
	int pageNumber = 2;
		help << "```";
		help << "Commandes module discord page OverWatch No "  << 1  << "/" << pageNumber << "\n";
		help << "Les différents arguments sont : Btag , Name , Team , Name , Critere , DateDebut , DateFin , Graph \n\n";
		help << "!ow Check(Ops->Btag: Bastion#21406)"<<" -> Récupère le Rank, depuis l'api OverWatch, du BattleTag passé en paramètre.\n\n";
		help << "!ow register(Btag:BASTION#21406; name:Clément)"<<" -> Enregistre votre identitée auprès du bot(Necessaire pour toutes les commandes en liaison avec les équipes. Ses arguments sont btag\n\n";
		help << "!ow Remove"<<" -> Efface votre identité auprès du bot #RGPD_Friendly.\n\n";
		help << "!ow CreateEquipe(team:Ma team)"<<" -> Vous permet de créer une équipe(Une equipes contient des players et permet la génération de graph).\n\n";
		help << "!ow RemoveEquipe(team:Ma team)"<<" -> Vous permet de supprimer une équipe(n'importe qui ayant les droits sur l'équipe à le droit de supprimer l'équipe).\n\n";
		help << "!ow GiveRight(discord: @Xemuth,team:Ma team)"<<" -> Vous permet de donner des droits pour administrer votre équipe.\n\n";
		help << "!ow RemoveRight(discord: @NattyRoots,team:Ma team)"<<" -> Vous permet de retirer les droits à un des administrateurs de l'équipe(sauf Admin).\n\n";
		help << "!ow AddToEquipe(discord: @NattyRoots,team:Ma team)"<<" -> Ajoute l'utilisateur à l'équipe (vous devez avoir les droits sur l'équipe !).\n\n";
		help << "!ow RemoveFromEquipe(discord: @NattyRoots,team:Ma team)"<<" -> Retire l'utilisateur de l'équipe(vous devez avoir les droits sur l'équipe !).\n\n";
		help <<"```";
	
	
	if(MessageArgs.GetCount() ==0){
		BotPtr->CreateMessage(ChannelLastMessage,help);
	}else {
		Value v = ResolveType(MessageArgs[0]);
		if(v.GetTypeName().IsEqual("int")){
			int numberParsed =v.Get<int>();
			switch(numberParsed){
				case 2:	
					help ="";
					help << "```";
					help << "Commandes module discord OverWatch page No "  << 2  << "/" << pageNumber << "\n";
					help << "Les différents arguments sont : Btag , Name , Team , Name , Critere , DateDebut , DateFin , Graph \n\n";
					help << "!ow Upd"<<" -> Mets à jours votre profile niveau stats.\n\n";
					help << "!ow Eupd(team:Ma team)"<<" -> Mets à jours toute l'équipe par rapport à l'Api(Vous devez être dans l'équipe.).\n\n";
					#ifdef flagGRAPHBUILDER_DB
					help << "!ow DrawStatsEquipe(critere:AutoRating ; team:MonEquipe ;  graph:GraphName)"<<" -> DrawStatsEquipe permet de dessiner un graph de l'évolution de l'équipe selons certains critères qui sont GlobalRating, HealRating,TankRating, DpsRating, AutoRating. Tous les champs sauf team sont optionnel. Par défault le critère est AutoRating et les paramètres de graph sont les derniers utilisés.\n\n";
					help << "!ow DrawStats(DateDebut: 27/06/2019; DateFin:30/06/2019 ; NbPointMax:20; critere:globalRating ; discord: @NattyRoots ; graph:My Graph Name)"<<" -> Dessine un graph perso.\n\n";
					help << "!ow SaveGraphParam(name:Propre)"<<" -> Sauvegarde les paramètres actuel en BDD soous le nom définie par 'Name' !\n\n";
					#else
					help << "!ow DrawStatsEquipe(critere:AutoRating ; team:MonEquipe )"<<" -> DrawStatsEquipe permet de dessiner un graph de l'évolution de l'équipe selons certains critères qui sont GlobalRating, HealRating,TankRating, DpsRating, AutoRating. Tous les champs sauf team sont optionnel. Par défault le critère est AutoRating et les paramètres de graph sont les derniers utilisés.\n\n";
					help << "!ow DrawStats(DateDebut: 27/06/2019; DateFin:30/06/2019 ; NbPointMax:20; critere:globalRating ; discord: @NattyRoots )"<<" -> Dessine un graph perso.\n\n";
					#endif
					help << "!ow GraphProperties()"<<" ->renvoie l'aide sur les proprietées.\n\n";
					help << "!ow credit()" <<" -> Affiche les crédit du module Overwatch.\n\n";
					help <<"```";
				break;
			}
		}
		BotPtr->CreateMessage(ChannelLastMessage,help);
	}
}

String Discord_Overwatch::Credit(ValueMap& json,bool sendCredit){
	String credit =  "----OverWatch Module have been made By Clément Hamon---\n";
	credit << "-----------https://github.com/Xemuth-----------\n";
	credit << "Overwatch module is used to track stats of various player on Overwatch\nIt track elo's evolution, medals, wins and provide graphical view of the progression\nThis module use GraphBuilder, a package used to draw graph dynamically\n";
	credit << "https://github.com/Xemuth/Discord_Overwatch\nhttps://github.com/Xemuth/GraphBuilder";
	if(sendCredit) BotPtr->CreateMessage(ChannelLastMessage,"```"+credit +"```");
	return credit;
}

//FIN COMMANDE DISCORD



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


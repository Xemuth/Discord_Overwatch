#include <Core/Core.h>
#include "Discord_Overwatch.h"
using namespace Upp;

void Discord_Overwatch::getStats(ValueMap payload){
	String channel  = payload["d"]["channel_id"];
    String content  = payload["d"]["content"];
    String userName = payload["d"]["author"]["username"];
    String id = payload["d"]["author"]["id"];
    String discriminator = payload["d"]["author"]["discriminator"];
	
	Cout() << "Event Overwatch" <<"\n";
	ptrBot->CreateMessage(channel, "Event Overwatch !");
}
	
Discord_Overwatch::Discord_Overwatch(Upp::String _name, Upp::String _prefix){
	name = _name;
	prefix = _prefix;
	
	Events.Add([&](ValueMap e){this->getStats(e);});
}

void Discord_Overwatch::Event(ValueMap payload){
	for(auto &e : Events){
		e(payload);
	}
}
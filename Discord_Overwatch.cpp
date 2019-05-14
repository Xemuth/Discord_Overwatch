#include <Core/Core.h>
#include "Discord_Overwatch.h"
using namespace Upp;

void Discord_Overwatch::getStats(ValueMap payload){
	Cout() << "Event déclanché ! #Overwatch" <<"\n";
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
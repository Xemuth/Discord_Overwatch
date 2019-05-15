#ifndef _Discord_Overwatch_Discord_Overwatch_h_
#define _Discord_Overwatch_Discord_Overwatch_h_

#include <SmartUppBot/SmartBotUpp.h>

using namespace Upp;
class Discord_Overwatch: public DiscordModule{
	private:
	
		void getStats(ValueMap payload);
		
	public:
		
		Discord_Overwatch(Upp::String _name, Upp::String _prefix);
		void Events(ValueMap payload);
	
	
};

#endif

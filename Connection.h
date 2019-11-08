#ifndef _Discord_Overwatch_Connection_h_
#define _Discord_Overwatch_Connection_h_

#include "Connection.cpp"
#include <plugin/sqlite3/Sqlite3.h>

using namespace Upp;

class Connection {
	private:
	
		int test;
		
	public:
		
		static Sql getConnection();
	
};

#endif

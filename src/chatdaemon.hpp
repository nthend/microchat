#pragma once

#include "daemon.hpp"
#include "database.hpp"

class ChatDaemon : public Daemon
{
private:
	static int print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
	{
		printf ("%s: %s\n", key, value);
		return MHD_YES;
	}
	
private:
	Database *db;

public:
	ChatDaemon(Database *database) : 
		Daemon(), db(database)
	{
		
	}
	
	~ChatDaemon()
	{
		
	}
	
	int respondGet(MHD_Connection *con, const char *url) override
	{
		printf ("GET responded from %s\n", url);
		// MHD_get_connection_values (con, MHD_HEADER_KIND, &print_out_key, NULL);
		
		std::string path(url);
		
		if(path == "/" || path == "/index.html")
		{
			sendFile(con,"res/index.html","text/html");
		}
		else
		if(path == "/engine.js" || path == "/request.js")
		{
			sendFile(con,("res" + path).data(),"application/javascript");
		}
		else
		if(path == "/style.css")
		{
			sendFile(con,("res" + path).data(),"text/css");
		}
		else
		{
			sendFile(con,"res/notfound.html","text/html");
		}
		
		return YES;
	}
	
	int respondPost(MHD_Connection *con, const char *url, void *data, int size) override
	{
		printf ("POST responded\n");
		// MHD_get_connection_values (con, MHD_HEADER_KIND, &print_out_key, NULL);
		
		cout << "+ stert responding" << endl;
		
		std::string query;
		std::string answer;
		try
		{
			query = std::string(static_cast<const char*>(data), size);
			
			cout << "+ execute query" << endl;
			Database::Table *table = db->executeQuery(query);
			db->commit();
			
			cout << "+ construct answer" << endl;
			answer = "[";
			answer += "[";
			const Database::Row *row = table->getHeader();
			for(int i = 0; i < row->getSize(); ++i)
			{
				if(i)
				{
					answer += ",";
				}
				answer += '\'' + row->getValue(i) + '\'';
			}
			answer += "]";
			for(int j = 0; j < table->getRowsNumber(); ++j)
			{
				answer += ",";
				answer += "[";
				const Database::Row *row = table->getRow(j);
				for(int i = 0; i < row->getSize(); ++i)
				{
					if(i)
					{
						answer += ",";
					}
					answer += '\'' + row->getValue(i) + '\'';
				}
				answer += "]";
			}
			answer += "]";
			delete table;
		}
		catch(SQLException &e)
		{
			cout << "+ catch exception" << endl;
			answer = std::string(e.what());
			if(!answer.size())
			{
				answer = "Done";
			}
		}
		
		cout << "+ send data" << endl;
		sendData(con,const_cast<char*>(answer.data()),answer.size(),"text/plain");
		
		cout << "+ return" << endl;
		return YES;
	}
};

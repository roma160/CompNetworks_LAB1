#include <iostream>

#include "AServer.h"

using namespace std;

class Server: public AServer
{
	int cur_id = 0;

public:
	Server(std::string address = "", std::string port = PORT_NUMBER) :
		AServer(move(address), move(port)){}

	struct Context: APeer::AContext
	{
		int id;
		string prev_grid;

		Context(int id, string grid): AContext(), id(id), prev_grid(grid) {}

		int compare_grid(const string& grid)
		{
			if (grid.size() != prev_grid.size())
				return -1;
			int ret = 0;
			for (int i = 0; i < grid.size(); i++)
				if(grid[i] != prev_grid[i])
				{
					ret++;
					ret += prev_grid[i] != '0';
				}
			return ret;
		}

		virtual ~Context()
		{ cout << "Removed finished client!\n"; }
	};

	int checkWinLoose(const string& grid)
	{
		char vert[5], horis[5], d1 = grid[0], d2 = grid[4];
		for(int i = 0; i < 5; i++)
		{
			if (grid[i] == '1' || grid[i] == '2') vert[i] = grid[i];
			else vert[i] = 0;
			if (grid[5*i] == '1' || grid[5*i] == '2') horis[i] = grid[5 *i];
			else horis[i] = 0;
		}

		for(int y = 0; y < 5; y++)
			for(int x = 0; x < 5; x++)
			{
				if (x == y && d1 != grid[x + 5 * y]) d1 = 0;
				if (4 - x == y && d2 != grid[x + 5 * y]) d2 = 0;
				if (grid[x + 5 * y] != vert[x]) vert[x] = 0;
				if (grid[x + 5 * y] != horis[y]) horis[y] = 0;
			}

		int was = 0;
		if (d1 == '2' || d2 == '2') was = 2;
		if (d1 == '1' || d2 == '1') was = 1;
		for (int i = 0; i < 5 && was == 0; i++)
		{
			if (vert[i] == '2' || horis[i] == '2') was = 2;
			if (vert[i] == '1' || horis[i] == '1') was = 1;
		}
		return was;
	}

	APeer::HandlerResponse messageHandler(APeer::AContext** _context, std::string message) override
	{
		if(*_context == nullptr)
		{
			cout << "New client!\n";

			if (message.size() != 25) {
				return APeer::HandlerResponse(true, "2");
			}

			*_context = new Context(cur_id++, message);
		}
		else if(((Context*)*_context)->compare_grid(message) != 1)
			return APeer::HandlerResponse(true, "2");

		Context* context = (Context*)*_context;

		int status = checkWinLoose(message);
		if(status != 0)
		{
			if (status == 1) return APeer::HandlerResponse(true, "1");
			if (status == 2) return APeer::HandlerResponse(true, "2");
		}

		int ind = -1;
		for (int i = 0; i < 25 && ind == -1; i++)
			if (message[i] == '0') ind = i;

		if(ind == -1) return APeer::HandlerResponse(true, "0");
		message[ind] = '2';

		status = checkWinLoose(message);
		if (status != 0)
		{
			if (status == 1) return APeer::HandlerResponse(true, "1" + message);
			if (status == 2) return APeer::HandlerResponse(true, "2" + message);
		}

		context->prev_grid = message;
		return APeer::HandlerResponse(move(message));
	}
};

int main()
{
    Server server;
	server.loop();

    WinSockWrapper::close();

    return 0;
}
#include <iostream>
#include <string>

#include "AClient.h"

using namespace std;

class Client: public AClient
{
public:
    Client(std::string connectionAddress = "localhost", std::string connectionPort = PORT_NUMBER):
		AClient(move(connectionAddress), move(connectionPort)) {}

    void print_grid(const string& grid)
    {
        cout << "\tGrid:\n\t___________\n";
        for (int y = 0; y < 5; y++) {
            cout << "\t|";
            for (int x = 0; x < 5; x++)
            {
                if (grid[y * 5 + x] == '1') cout << "@|";
                else if (grid[y * 5 + x] == '2') cout << "#|";
                else cout << (char)('a' + y * 5 + x) << "|";
            }
            cout << "\n\t-----------\n";
        }
    }

    bool make_move(string& grid)
    {
        cout << "Now enter a character, you want to place on a grid\n(that must not be @ or #, or ! to exit): ";
        char buff;
        cin >> buff;
        while(buff != '!' && ('A' > buff || ('Y' < buff && buff < 'a') || buff > 'y'))
        {
            cout << "Reenter please: ";
            cin >> buff;
        }

        if(buff == '!') return false;

        int i;
        if (buff < 'Y') i = buff - 'A';
        else i = buff - 'a';
        grid[i] = '1';

        return true;
    }

    HandlerResponse messageHandler(AContext** context, std::string message) override
    {
        if(strcmp(message.c_str(), START_MESSAGE) == 0)
        {
            cout << "Successfully connected to the server!\nMake your first move:\n";
            string start_grid(25, '0');
            print_grid(start_grid);
            bool res = make_move(start_grid);
            if (res) return HandlerResponse(start_grid);
            return HandlerResponse(true, "");
        }
        if(message.size() == 25)
        {
            cout << "Received new move from a server. Now its time for yours:\n";
            print_grid(message);
            bool res = make_move(message);
            if (res) return HandlerResponse(message);
            return HandlerResponse(true, "");
        }
        if(message.size() == 1 || message.size() == 26)
        {
            if (message[0] == '2') cout << "!!! Server won\n";
            else if (message[0] == '1') cout << "!!! You won!\n";
            else if (message[0] == '0') cout << "!!! No one win(!\n";

            if(message.size() == 26)
            {
                cout << "It responded with such a grid:\n";
                print_grid(message.substr(1));
            }

            return HandlerResponse(true, "");
        }

        return HandlerResponse(true, "");
    }

    std::unique_ptr<ASockResult> loopEnd(std::unique_ptr<ASockResult> reason) override
    {
        if (reason->type == ASockResult::SHUTDOWN)
            cout << "Connection was shutdown\n";
        else if (reason->type == ASockResult::ERR)
            cout << "Connection closed because of an error: " << ((ErrSockResult*)reason.get())->code;

        return AClient::loopEnd(move(reason));
    }
};

int main()
{
    Client cl;
    cl.loop();
    WinSockWrapper::close();
    cin.get(); cin.get();
}
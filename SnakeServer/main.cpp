#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <SFML/Network.hpp>

const sf::Vector2i GRIDDIMENSIONS = {40, 40};   //the x and y are definitely swapped in some places but oh well
const int PACKETSPERSECOND = 40;

class Player
{
private:
    sf::Vector2i position;
    sf::Vector2i direction;
    sf::Vector2i pendingDirection;
    bool dead;
public:
    sf::IpAddress ip;
    unsigned short port;
    int length;
    std::vector<sf::Vector3i> body; //x, y, life

    Player()
    {
        dead = false;
        length = 4;
    }

    void set(int i)
    {
        if (i == 0)
        {
            pendingDirection = {1, 0};
            body.push_back(sf::Vector3i(5, 10, length));
        }
        else if (i == 1)
        {
            pendingDirection = {-1, 0};
            body.push_back(sf::Vector3i(35, 30, length));
        }
        else if (i == 2)
        {
            pendingDirection = {0, 1};
            body.push_back(sf::Vector3i(10, 5, length));
        }
        else if (i == 3)
        {
            pendingDirection = {0, -1};
            body.push_back(sf::Vector3i(30, 35, length));
        }
        else if (i == 4)
        {
            pendingDirection = {1, 0};
            body.push_back(sf::Vector3i(5, 30, length));

        }
        else if (i == 5)
        {
            pendingDirection = {-1, 0};
            body.push_back(sf::Vector3i(35, 10, length));

        }
        else if (i == 6)
        {
            pendingDirection = {0, 1};
            body.push_back(sf::Vector3i(30, 5, length));

        }
        else if (i == 7)
        {
            pendingDirection = {0, -1};
            body.push_back(sf::Vector3i(10, 35, length));
        }
    }

    void kill()
    {
        dead = true;
    }

    bool isDead()
    {
        return dead;
    }

    void changeDirection(sf::Vector2i newDirection)
    {
        if (direction + newDirection != sf::Vector2i(0, 0)) //not opposite directions
        {
            pendingDirection = newDirection;
        }
    }

    void shrink()       //shrink from tail
    {
        for (unsigned int i = 0; i < body.size(); ++i)
        {
            body[i].z--;

            if (body[i].z == 0)
            {
                body.erase(body.begin() + i);
                i--;
            }
        }
    }

    void grow()        //grow from head
    {
        direction = pendingDirection;

        if (body.back().x + direction.x < 0 || body.back().x + direction.x > GRIDDIMENSIONS.x || body.back().y + direction.y < 0 || body.back().y + direction.y > GRIDDIMENSIONS.y)
        {   //hit outside wall
            dead = true;
        }
        else
        {
            body.push_back(sf::Vector3i(body.back().x + direction.x, body.back().y + direction.y, length));
        }
    }

    void eat()
    {
        length++;
    }
};

std::vector<Player> players;
int grid[40][40];   //00 = nothing; 01-08 = players; 09-16 = player head; 17 = food
sf::UdpSocket socket;
sf::Vector2i food;
int last;

Player * playerAt(sf::IpAddress ip)
{
    for (unsigned int i = 0; i < players.size(); ++i)
    {
        if (ip == players[i].ip)
        {
            return &players[i];
        }
    }
    return NULL;
}

bool gameOver()
{
    for (unsigned int i = 0; i < players.size(); ++i)
    {
        if (!players[i].isDead())
        {
            last = i;
            return false;
        }
    }

    return true;
}

void newFood()
{
    std::vector<sf::Vector2i> emptySpots;

    for (int a = 0; a < GRIDDIMENSIONS.x; ++a)
    {
        for (int b = 0; b < GRIDDIMENSIONS.y; ++b)
        {
            if (grid[a][b] == 0)
            {
                emptySpots.push_back(sf::Vector2i(a, b));
            }
        }
    }

    food = emptySpots[rand() % emptySpots.size()];
}

void updateGrid()
{
    if (food == sf::Vector2i(-1, -1))
    {
        newFood();
    }

    for (int a = 0; a < GRIDDIMENSIONS.x; ++a)
    {
        for (int b = 0; b < GRIDDIMENSIONS.y; ++b)
        {
            grid[a][b] = 0;
        }
    }

    grid[food.x][food.y] = 17;

    for (unsigned int i = 0; i < players.size(); ++i)
    {
        if (!players[i].isDead())
        {
            for (unsigned int a = 0; a < players[i].body.size() - 1; ++a)
            {
                grid[players[i].body[a].x][players[i].body[a].y] = i + 1;   //not head piece
            }
        }
    }

    for (unsigned int i = 0; i < players.size(); ++i)
    {
        if (!players[i].isDead())
        {
            bool headCollision = false;
            for (unsigned int a = 0; a < players.size(); ++a)
            {
                if (a != i)
                {
                    if (!players[a].isDead())
                    {
                        if (players[i].body.back().x == players[a].body.back().x && players[i].body.back().y == players[a].body.back().y)   //heads in same place
                        {
                            players[a].kill();
                            headCollision = true;
                        }
                    }
                }
            }
            if (headCollision)
            {
                players[i].kill();
                continue;
            }

            if (grid[players[i].body.back().x][players[i].body.back().y] != 0 && grid[players[i].body.back().x][players[i].body.back().y] != 17)    //crashed into body
            {
                players[i].kill();
            }
            else    //OK!
            {
                grid[players[i].body.back().x][players[i].body.back().y] = i + 9;
            }
        }
    }
}

void sendToPlayer(int i, std::string message)
{
    socket.send(message.c_str(), message.size() + 1, players[i].ip, players[i].port);
}

void checkFood(int i)
{
    if (players[i].body.back().x == food.x && players[i].body.back().y == food.y)
    {
        players[i].eat();
        newFood();
    }
}

int main()
{
    socket.setBlocking(false);
    socket.bind(29999);

    srand(time(NULL));

    sf::Clock clock;

    int amount = 0;
    int mode = 0;
    int speed = 0;
    int counter = 0;

    food = {-1, -1};

    printf("How many players are you waiting for? (1-8, enter something outside this range to exit)\n");
    scanf("%d", &amount);

    if (amount > 8 || amount < 1)
    {
        printf("Press enter to exit\n");
        fflush(stdin);
        getchar();
        exit(0);
    }

    printf("How fast should the game be? (1 - fast, 2 - medium, 3 - slow)\n");
    scanf("%d", &speed);

    if (speed < 1 || speed > 3)
    {
        printf("Press enter to exit\n");
        fflush(stdin);
        getchar();
        exit(0);
    }

    printf("Waiting for players...\n");

    //clear anything that's already been sent
    std::size_t tempSize = 0;
    do
    {
        char tempBuffer[1024];
        sf::IpAddress tempSender;
        unsigned short tempPort;
        socket.receive(tempBuffer, sizeof(tempBuffer), tempSize, tempSender, tempPort);
    } while (tempSize > 0);

    while(1)
    {
        ////////////////recieve stuff

        while(1)    //process all recieved messages
        {
            char buffer[1024];
            std::size_t received = 0;
            sf::IpAddress sender;
            unsigned short port;
            socket.receive(buffer, sizeof(buffer), received, sender, port);

            if (received > 0)
            {
                if (mode == 0)
                {
                    if (playerAt(sender) == NULL) //this ip not yet connected
                    {
                        if (players.size() < (unsigned int)amount)
                        {
                            players.emplace_back();
                            players.back().set(players.size() - 1);
                            players.back().ip = sender;
                            players.back().port = port;

                            printf("%s has connected\n", sender.toString().c_str());

                            //tell host what number they are
                            char temp[100];
                            std::string identityMessage("y");
                            sprintf(temp, "%d", players.size() - 1);
                            identityMessage += temp;
                            sendToPlayer(players.size() - 1, identityMessage);

                            if (players.size() == (unsigned int)amount)   //all players connected
                            {
                                mode = 1;
                                printf("\nStarting game...\n");
                                updateGrid();
                                printf("3\n");
                                for(int i = 0; i < amount; ++i)
                                {
                                    sendToPlayer(i, "c3");
                                }
                                sf::sleep(sf::seconds(1));
                                printf("2\n");
                                for(int i = 0; i < amount; ++i)
                                {
                                    sendToPlayer(i, "c2");
                                }
                                sf::sleep(sf::seconds(1));
                                printf("1\n");
                                for(int i = 0; i < amount; ++i)
                                {
                                    sendToPlayer(i, "c1");
                                }
                                sf::sleep(sf::seconds(1));
                                printf("Start\n");
                                for(int i = 0; i < amount; ++i)
                                {
                                    sendToPlayer(i, "c0");
                                }
                            }
                        }
                    }
                }
                if (mode == 1)
                {
                    Player * temp = playerAt(sender);

                    if (temp != NULL)
                    {
                        if (buffer[0] == 'u')
                            temp->changeDirection({0, -1});
                        else if (buffer[0] == 'd')
                            temp->changeDirection({0, 1});
                        else if (buffer[0] == 'l')
                            temp->changeDirection({-1, 0});
                        else if (buffer[0] == 'r')
                            temp->changeDirection({1, 0});
                    }
                }
            }
            else
            {
                break;
            }
        }

        /////////////////update stuff

        if (mode == 1)
        {
            if (gameOver())
            {
                printf("Game over\n");
                mode = 2;

                if (amount > 1)
                {
                    printf("Player %d (%s) was the last one alive.\n\n", last, players[last].ip.toString().c_str());
                }
                printf("Score:\n");
                for (int i = 0; i < amount; ++i)
                {
                    printf("Player %d (%s): %d\n", i, players[i].ip.toString().c_str(), players[i].length);
                }
            }

            if (++counter > speed * 2)
            {
                counter = 0;

                for (unsigned int i = 0; i < players.size(); ++i)
                {
                    if (!players[i].isDead())
                    {
                        players[i].shrink();
                        players[i].grow();
                        checkFood(i);
                    }
                }

                updateGrid();
                updateGrid();   //run twice to get rid of dead players...:P

                //grid update
                std::string gridMessage("g");
                for (int a = 0; a < GRIDDIMENSIONS.x; ++a)
                {
                    for (int b = 0; b < GRIDDIMENSIONS.y; ++b)
                    {
                        if (grid[a][b] < 10)
                        {
                            gridMessage += "0";
                            gridMessage += (char)grid[a][b] + 48;
                        }
                        else
                        {
                            gridMessage += "1";
                            gridMessage += (char)(grid[a][b] - 10) + 48;
                        }
                    }
                }

                for (unsigned int i = 0; i < (unsigned int)amount; ++i)
                {
                    sendToPlayer(i, gridMessage);

                    //length update
                    for (int a = 0; a < amount; ++a)
                    {
                        std::string lengthMessage("s");
                        if (players[a].isDead())
                        {
                            lengthMessage += (char)a + 48;
                            lengthMessage += "0 ";
                        }
                        else
                        {
                            lengthMessage += (char)a + 48;
                            char temp[100];
                            sprintf(temp, "%d ", players[a].length);
                            lengthMessage += temp;
                        }
                        sendToPlayer(i, lengthMessage);
                    }
                }
            }
        }
        if (mode == 2)
        {
            printf("Press enter to exit\n");
            fflush(stdin);
            getchar();
            exit(0);
        }

        sf::sleep(sf::milliseconds(1000 / PACKETSPERSECOND - clock.getElapsedTime().asMilliseconds()));
        clock.restart();
    }

    return 0;
}

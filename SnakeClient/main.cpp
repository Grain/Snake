#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>

const int OFFSET = 200;
const sf::Vector2i GRIDDIMENSIONS = {40, 40};
const sf::Vector2i BOXDIMENSIONS = {15, 15};
sf::Color gridColors[18];
sf::UdpSocket socket;

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("Enter the ip address of the server as the argument.");
        exit(0);
    }

    sf::IpAddress server = argv[1];

    socket.setBlocking(false);
    socket.bind(sf::Socket::AnyPort);
    std::string message = "HI";
    socket.send(message.c_str(), message.size() + 1, server, 29999);

    gridColors[0] = sf::Color::White;

    gridColors[1] = sf::Color::Red;
    gridColors[2] = sf::Color::Blue;
    gridColors[3] = sf::Color(255, 216, 0);
    gridColors[4] = sf::Color(72, 0, 255);
    gridColors[5] = sf::Color(0, 127, 14);
    gridColors[6] = sf::Color(255, 106, 0);
    gridColors[7] = sf::Color(255, 0, 110);
    gridColors[8] = sf::Color(0, 148, 255);

    gridColors[9] = sf::Color::Black;
    gridColors[10] = sf::Color::Black;
    gridColors[11] = sf::Color::Black;
    gridColors[12] = sf::Color::Black;
    gridColors[13] = sf::Color::Black;
    gridColors[14] = sf::Color::Black;
    gridColors[15] = sf::Color::Black;
    gridColors[16] = sf::Color::Black;

    gridColors[17] = sf::Color(0, 0, 0, 128);

    sf::RectangleShape side;
    side.setFillColor(sf::Color::Black);
    side.setSize({200, 600});
    side.setPosition({0, 0});

    sf::RectangleShape players[8];
    sf::Text scores[8];
    sf::Font font;
    font.loadFromFile("arial.ttf");

    for (int i = 0; i < 8; ++i)
    {
        players[i].setFillColor(gridColors[i + 1]);
        players[i].setSize(sf::Vector2f(180, 65));
        players[i].setPosition(10, 5 + i * 75);
        players[i].setOutlineColor(sf::Color::White);

        scores[i].setFont(font);
        scores[i].setCharacterSize(24);
        scores[i].setPosition(players[i].getPosition().x + 10, players[i].getPosition().y + (int)players[i].getSize().y / 2);

        scores[i].setString(" ");
    }

    sf::Text countdown(" ", font, 60);
    countdown.setPosition(sf::Vector2f(500, 300));
    countdown.setColor(sf::Color::Black);

    sf::RectangleShape grid[GRIDDIMENSIONS.x][GRIDDIMENSIONS.y];

    for (int a = 0; a < GRIDDIMENSIONS.x; ++a)
    {
        for (int b = 0; b < GRIDDIMENSIONS.y; ++b)
        {
            grid[a][b].setSize(sf::Vector2f(BOXDIMENSIONS.x - 2, BOXDIMENSIONS.y - 2));
            grid[a][b].setPosition(OFFSET + a * BOXDIMENSIONS.x + 1, b * BOXDIMENSIONS.y + 1);
            grid[a][b].setFillColor(sf::Color::White);
        }
    }

    sf::RenderWindow window(sf::VideoMode(800, 600), "Snake", sf::Style::Close);
    window.setFramerateLimit(60);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    break;
                default:
                    break;
            }
        }

        char buffer[10000];
        std::size_t received = 0;
        sf::IpAddress sender;
        unsigned short port;
        socket.receive(buffer, sizeof(buffer), received, sender, port);

        if (received > 0)
        {
            if (buffer[0] == 'g')   //grid
            {
                for (unsigned int i = 2; i < strlen(buffer); ++i)
                {
//                    if (((int)buffer[i - 1] - 48) * 10 + (int)buffer[i] - 48 > 0)
//                    {
//                        printf("%d\n", ((int)buffer[i - 1] - 48) * 10 + (int)buffer[i] - 48);
//                    }
                    grid[((i - 2) / 2) / GRIDDIMENSIONS.x][((i - 2) / 2) % GRIDDIMENSIONS.y].setFillColor(gridColors[((int)buffer[i - 1] - 48) * 10 + (int)buffer[i] - 48]);
                    ++i;
                }
            }
            else if (buffer[0] == 'y')  //your number/color
            {
                players[buffer[1] - 48].setOutlineThickness(2);
            }
            else if (buffer[0] == 's')  //score update
            {
                for (unsigned int i = 1; i < strlen(buffer); ++i)
                {
                    char tempString[100];
                    int temp = 0;
                    int a;

                    for (a = 1; buffer[i + a + 1] != ' ' && buffer[i + a + 1] != '\0'; ++a);    //a = number of digits in score

                    for (int b = 0; b < a; ++b)
                    {
                        temp += (buffer[i + 1 + b] - 48) * (int)pow(10, a - b - 1);
                    }
                    sprintf(tempString, "%d", temp);

                    if (temp == 0)
                    {
                         scores[buffer[i] - 48].setString("Dead");
                    }
                    else
                    {
                        scores[buffer[i] - 48].setString(std::string(tempString));
                    }
                    scores[buffer[i] - 48].setOrigin(0, scores[buffer[i] - 48].getGlobalBounds().height / 2 + 5);

                    i += a + 1;
                }
            }
            else if (buffer[0] == 'c')  //countdown
            {
                if (buffer[1] == '0')
                {
                    countdown.setString(" ");
                }
                else
                {
                    countdown.setString(buffer[1]);
                }
                countdown.setOrigin(countdown.getGlobalBounds().width / 2, countdown.getGlobalBounds().height / 2);
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        {
            std::string message = "u";
            socket.send(message.c_str(), message.size() + 1, server, 29999);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        {
            std::string message = "d";
            socket.send(message.c_str(), message.size() + 1, server, 29999);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        {
            std::string message = "l";
            socket.send(message.c_str(), message.size() + 1, server, 29999);
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        {
            std::string message = "r";
            socket.send(message.c_str(), message.size() + 1, server, 29999);
        }

        //draw
        window.clear(sf::Color::White);
        for (int a = 0; a < GRIDDIMENSIONS.x; ++a)
        {
            for (int b = 0; b < GRIDDIMENSIONS.y; ++b)
            {
                window.draw(grid[a][b]);
            }
        }
        window.draw(side);
        for (int a = 0; a < 8; ++a)
        {
            window.draw(players[a]);
            window.draw(scores[a]);
        }
        window.draw(countdown);
        window.display();
    }

    return 0;
}

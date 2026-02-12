#pragma once
#include <string>

class Player {
public:
    Player(std::string name, int score) : _name(std::move(name)), _score(score) {}

    const char* GetName() const { return _name.c_str(); }
    int GetScore() const { return _score; }

private:
    std::string _name;
    int _score;
};
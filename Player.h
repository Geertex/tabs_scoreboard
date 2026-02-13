#pragma once
#include <string>

class Player {
private:
    std::string _name;
    int _score;

public:
    Player(std::string name, int score) : _name(std::move(name)), _score(score) {}

    void SubmitStreak(int streak) {
        if (streak > _score) _score = streak;
    }

    const char* GetName() const { return _name.c_str(); }
    int GetScore() const { return _score; }
    void ResetScore() { _score = 0; }
};
#include <string>
#include <ctime>

class Match {
private:
    long long timestamp;    // 0 if match is active
    std::string redName;
    std::string blueName;
    std::string winner;
    std::string loser;

public:
    // Constructor sets the names and initializes state
    Match(std::string red, std::string blue) 
        : redName(red), blueName(blue), timestamp(0), winner(""), loser("") {}

    void RedWins() {
        winner = redName;
        loser = blueName;
        timestamp = (long long)std::time(nullptr); 
    }

    void BlueWins() {
        winner = blueName;
        loser = redName;
        timestamp = (long long)std::time(nullptr); 
    }

    // Getters
    bool IsActive() const { return (winner.empty() && loser.empty()); }
    const char* GetRed() const { return redName.c_str(); }
    const char* GetBlue() const { return blueName.c_str(); }
    const char* GetWinner() const { return winner.c_str(); }
    const char* GetLoser() const { return loser.c_str(); }
    long long GetTimestamp() const { return timestamp; }
};
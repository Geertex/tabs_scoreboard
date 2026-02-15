#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include "Player.h"
#include "Match.h"

namespace TABSScoreboard
{
    void SavePlayersToCSV(const std::vector<Player>& players, const std::string& filename);

    void LoadPlayersFromCSV(std::vector<Player>& players, const std::string& filename);

    void SaveMatchHistoryToCSV(const std::vector<Match>& match_history, const std::string& filename);

    void LoadMatchHistoryFromCSV(std::vector<Match>& match_history, const std::string& filename);

    bool NameExists(const std::vector<Player>& list, const std::string& name);
    
    Player* FindPlayer(std::vector<Player>& list, const std::string& name);

    void ResetPlayerScores(std::vector<Player>& players);

    void CalculateStreakScores(const std::vector<Match>& match_history, std::vector<Player>& players);

    int CalculateCurrentStreak(const std::vector<Match>& match_history, const std::string& name);

    void InitializeTABSWindows();

    void RenderTABSWindows();

}
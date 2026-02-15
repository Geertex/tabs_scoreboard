#include "TABSScoreBoard.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace TABSScoreboard
{
    void SavePlayersToCSV(const std::vector<Player>& players, const std::string& filename) {
        fs::path p(filename);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        std::ofstream file(filename);

        if (!file.is_open()) {
            printf("ERROR: Could not create file %s\n", filename.c_str());
            return;
        }

        for (const auto& player : players) {
            file << player.GetName() << "\n";
        }

        file.close();
        printf("Successfully saved %zu players to %s\n", players.size(), filename.c_str());
    }

    void LoadPlayersFromCSV(std::vector<Player>& players, const std::string& filename) {
        std::ifstream file(filename);
        std::string line;

        if (file.is_open()) {
            players.clear();
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    players.emplace_back(line, 0);
                }
            }
            file.close();
        }
    }

    void SaveMatchHistoryToCSV(const std::vector<Match>& match_history, const std::string& filename) {
        fs::path p(filename);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        std::ofstream file(filename);

        if (!file.is_open()) {
            printf("ERROR: Could not create file %s\n", filename.c_str());
            return;
        }

        file << "Timestamp,RedPlayer,BluePlayer,Winner,Loser\n";

        for (const auto& match : match_history) {
            file << match.GetTimestamp() << ","
            << match.GetRed() << ","
            << match.GetBlue() << ","
            << match.GetWinner() << ","
            << match.GetLoser() << ","
            << "\n";
        }

        file.close();
        printf("Successfully saved %zu matches to %s\n", match_history.size(), filename.c_str());
    }

    void LoadMatchHistoryFromCSV(std::vector<Match>& match_history, const std::string& filename) {
        std::ifstream file(filename);
        std::string line;

        if (file.is_open()) {
            match_history.clear();
            std::getline(file, line);
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string timestamp,redName,blueName,winner,loser;
                if (std::getline(ss, timestamp, ',') &&
                    std::getline(ss, redName, ',') &&
                    std::getline(ss, blueName, ',') &&
                    std::getline(ss, winner, ',') &&
                    std::getline(ss, loser, ',')) {
                    match_history.emplace_back(stoll(timestamp), redName, blueName, winner, loser);
                }
            }
            file.close();
        }

        std::sort(match_history.begin(), match_history.end(), [](const Match& a, const Match& b) {
            return a.GetTimestamp() < b.GetTimestamp();
        });
    }

    bool NameExists(const std::vector<Player>& list, const std::string& name) {
        return std::find_if(list.begin(), list.end(), [&](const Player& p) {
            return std::string(p.GetName()) == name;
        }) != list.end();
    }

    Player* FindPlayer(std::vector<Player>& list, const std::string& name) {
        for (auto& p : list) {
            if (std::string(p.GetName()) == name) return &p;
        }
        return nullptr;
    }

    void ResetPlayerScores(std::vector<Player>& players) {
        for (auto& player : players) {
            player.ResetScore();
        }
    }

    void CalculateStreakScores(const std::vector<Match>& match_history, std::vector<Player>& players) {
        ResetPlayerScores(players);
        std::string current_streak_player_name = "";
        int current_streak_count = 0;
        Player* current_streak_player = nullptr;

        for (const auto& match : match_history) {
            if (current_streak_player_name == match.GetWinner()) {
                current_streak_count++;
            } else {
                current_streak_player_name = match.GetWinner();
                current_streak_count = 1;
            }
            current_streak_player = FindPlayer(players, current_streak_player_name);
            if (current_streak_player) {
                current_streak_player->SubmitStreak(current_streak_count);
            }
        }
    }

    int CalculateCurrentStreak(const std::vector<Match>& match_history, const std::string& name) {
        int streak = 0;
        for (int i = (int)match_history.size() - 1; i >= 0; i--) {
            const auto& match = match_history[i];
            if (match.GetWinner() == name) streak ++;
            else break;
        }
        return streak;
    }


    static std::vector<Player> players;
    static std::vector<Player> player_queue;
    static std::vector<Match> match_history;
    static std::string last_winner_name = "";
    static std::unique_ptr<Match> active_match = nullptr;

    // Use a flag to ensure you only load once
    static bool initialized = false;

    static bool show_queue_window = true;
    static bool show_player_window = true;
    static bool show_match_history_window = true;


    void InitializeTABSWindows() {
        if (initialized) return;

        LoadPlayersFromCSV(players, "data/players.csv");
        LoadMatchHistoryFromCSV(match_history, "data/match_history.csv");
        CalculateStreakScores(match_history, players);

        if (!match_history.empty()) last_winner_name = match_history.back().GetWinner();
        
        initialized = true;
    }

    void RenderTABSWindows() {
        {

            ImGui::Begin("TABS Score window selecter");
            ImGui::Checkbox("Show Player Window", &show_player_window);
            ImGui::Checkbox("Show Queue Window", &show_queue_window);
            ImGui::Checkbox("Show Match History Window", &show_match_history_window);
            
            if (!last_winner_name.empty()) {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "HOLDER OF THE CROWN :"); // Gold color
                ImGui::SameLine();
                ImGui::Text("%s", last_winner_name.c_str());
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "WIN STREAK = ");
                ImGui::SameLine();
                ImGui::Text("%d", CalculateCurrentStreak(match_history, last_winner_name));
                ImGui::PopStyleVar();
            }

            if (active_match) {
                ImGui::SeparatorText("ACTIVE MATCH");

                // Calculate window width to make buttons even
                float width = ImGui::GetContentRegionAvail().x;

                // --- RED PLAYER BLOCK ---
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));       // Dark Red
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Bright Red
                
                if (ImGui::Button(active_match->GetRed(), ImVec2(width * 0.45f, 60.0f))) {
                    active_match->RedWins();
                }
                ImGui::PopStyleColor(2);

                ImGui::SameLine();
                ImGui::SetCursorPosX(width * 0.47f);
                ImGui::Text("VS");
                ImGui::SameLine();

                // --- BLUE PLAYER BLOCK ---
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.6f, 1.0f));       // Dark Blue
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.8f, 1.0f)); // Bright Blue

                if (ImGui::Button(active_match->GetBlue(), ImVec2(width * 0.45f, 60.0f))) {
                    active_match->BlueWins();
                }
                ImGui::PopStyleColor(2);

                if (!active_match->IsActive()) {
                    last_winner_name = active_match->GetWinner();
                    player_queue.push_back(*FindPlayer(players, active_match->GetLoser()));
                    match_history.push_back(*active_match);
                    SaveMatchHistoryToCSV(match_history, "data/match_history.csv");
                    active_match.reset();
                    CalculateStreakScores(match_history, players);
                }

                if (ImGui::Button("Cancel Match")) {
                    // Find Red Player in the master list
                    Player* pRed = FindPlayer(players, active_match->GetRed());

                    // Find Blue Player in the master list
                    Player* pBlue = FindPlayer(players, active_match->GetBlue());

                    // Insert them back at the START of the queue (index 0)
                    // We do Blue then Red so that Red ends up being the very first item
                    if (pBlue && (last_winner_name.empty() || active_match->GetRed() == last_winner_name)) {
                        player_queue.insert(player_queue.begin(), *pBlue);
                    }
                    if (pRed && (last_winner_name.empty() || active_match->GetBlue() == last_winner_name)) {
                        player_queue.insert(player_queue.begin(), *pRed);
                    }

                    // Clean up the match
                    active_match.reset();
                }
            } else if (!last_winner_name.empty()){
                if (!player_queue.empty()) {
                    if (ImGui::Button("Next Match")) {
                        if (match_history.back().GetBlue() == last_winner_name) {
                            active_match = std::make_unique<Match>(player_queue[0].GetName(), last_winner_name);
                        } else {
                            active_match = std::make_unique<Match>(last_winner_name, player_queue[0].GetName());
                        }
                        
                        player_queue.erase(player_queue.begin());
                    }
                } else {
                    ImGui::TextDisabled("(Waiting for next challenger to join the queue...)");
                }
            } else if (player_queue.size() >= 2 && !active_match) {
                if (ImGui::Button("Start Match")) {
                    active_match = std::make_unique<Match>(player_queue[0].GetName(), player_queue[1].GetName());
                    player_queue.erase(player_queue.begin());
                    player_queue.erase(player_queue.begin());
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Button("Start Match (Need 2 Players and no active match)");
                ImGui::EndDisabled();
            }

            ImGui::End();
        }

        if (show_player_window) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("Player Window", &show_player_window);
            if (ImGui::BeginTable("PlayerTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit)) {
                // 1. Setup Headers
                ImGui::TableSetupColumn("Player Name");
                ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                    if (sorts_specs->SpecsDirty) {
                        // This block runs ONLY when the user clicks a header
                        std::sort(players.begin(), players.end(), [&](const Player& a, const Player& b) {
                            for (int n = 0; n < sorts_specs->SpecsCount; n++)
                            {
                                const ImGuiTableColumnSortSpecs* spec = &sorts_specs->Specs[n];
                                
                                bool res = false;
                                if (spec->ColumnIndex == 0) // Sort by Name
                                    res = strcmp(a.GetName(), b.GetName()) < 0;
                                else if (spec->ColumnIndex == 1) // Sort by Score
                                    res = a.GetScore() < b.GetScore();

                                if (spec->SortDirection == ImGuiSortDirection_Descending)
                                    res = !res;
                                
                                return res;
                            }
                            return false;
                        });
                        sorts_specs->SpecsDirty = false; // Mark as "handled"
                    }
                }

                // 2. Fill Rows
                for (const auto& player : players)
                {
                    bool alreadyInQueue = NameExists(player_queue, player.GetName());

                    bool isLastWinner = (player.GetName() == last_winner_name);

                    bool alreadyInMatch = false;
                    if (active_match) {
                        if (std::string(player.GetName()) == active_match->GetRed() || 
                            std::string(player.GetName()) == active_match->GetBlue()) {
                            alreadyInMatch = true;
                        }
                    }

                    // 2. Determine if the "Add" button should be disabled
                    bool disableAdd = alreadyInQueue || alreadyInMatch || isLastWinner;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", player.GetName());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", player.GetScore());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushID(&player);
                    if (!disableAdd) {
                        if (ImGui::Button("Add to Queue")) {
                            player_queue.push_back(player);
                        }
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            if (ImGui::Button("New Player"))
                ImGui::OpenPopup("Create New Player");
            ImGui::SameLine();
            if (ImGui::Button("Save Players")){SavePlayersToCSV(players, "data/players.csv");}
            ImGui::SameLine();
            if (ImGui::Button("Load Players")){LoadPlayersFromCSV(players, "data/players.csv");}
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Create New Player", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char name_input[32] = "";
                bool nameExists =  NameExists(players, name_input);
                bool isInvalid = nameExists || (name_input[0] == '\0');

                if (nameExists) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This name is already taken!");
                }

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                }

                ImGui::InputTextWithHint("##NameInput", "Enter player name...", name_input, IM_COUNTOF(name_input));


                if (isInvalid)
                    ImGui::BeginDisabled();
                
                if (ImGui::Button("Save", ImVec2(120, 0))) {
                    players.emplace_back(name_input, 0);
                    SavePlayersToCSV(players, "data/players.csv");
                    name_input[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }

                if (isInvalid)
                    ImGui::EndDisabled();
                else
                    ImGui::SetItemDefaultFocus();
                
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    name_input[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        if (show_queue_window) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("Queue Window", &show_queue_window);
            for (int n = 0; n < player_queue.size(); n++) {
                ImGui::PushID(n);
                ImGui::Button(player_queue[n].GetName());
                // Our buttons are both drag sources and drag targets here!
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    ImGui::SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));
                    ImGui::Text("Move %s", player_queue[n].GetName());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(int));
                        int source_index = *(const int*)payload->Data;
                        int target_index = n;

                        if (source_index != target_index) {
                            Player moved_player = player_queue[source_index];
                            player_queue.erase(player_queue.begin() + source_index);
                            player_queue.insert(player_queue.begin() + target_index, moved_player);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    player_queue.erase(player_queue.begin() + n);
                    ImGui::PopID();
                    break;
                }

                ImGui::PopID();
            }
            ImGui::End();
        }

        if (show_match_history_window) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("Match History Window", &show_match_history_window);

            if (ImGui::BeginTable("MatchHistoryTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |  ImGuiTableFlags_SizingFixedFit )) {
                // 1. Setup Headers
                ImGui::TableSetupColumn("Time Stamp");
                ImGui::TableSetupColumn("Red Player");
                ImGui::TableSetupColumn("Blue Player");
                ImGui::TableSetupColumn("Winner");
                ImGui::TableSetupColumn("Loser");
                ImGui::TableHeadersRow();

                for (int i = (int)match_history.size() - 1; i >= 0; i--) {
                    const auto& match = match_history[i];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    std::time_t ts = (std::time_t)match.GetTimestamp();
                    struct tm buf; 
                    // On Windows, the order is (output_struct_ptr, input_time_ptr)
                    if (localtime_s(&buf, &ts) == 0) {
                        char time_buffer[20];
                        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M", &buf);
                        ImGui::Text("%s", time_buffer);
                    } else {
                        ImGui::Text("Invalid Date");
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", match.GetRed());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", match.GetBlue());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", match.GetWinner());
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", match.GetLoser());
                }
                ImGui::EndTable();
            }
            if (ImGui::Button("Load Match History")){
                LoadMatchHistoryFromCSV(match_history, "data/match_history.csv");
                last_winner_name = "";
                if (!match_history.empty()) last_winner_name = match_history.back().GetWinner();
                CalculateStreakScores(match_history, players);
            }
            ImGui::End();
        }
    }

}
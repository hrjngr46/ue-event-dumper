#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include "json.hpp"

using json = nlohmann::json;

std::string extract_sound_name(const json* entry) {
    if (!entry || entry->is_null()) return "Unknown";

    if (entry->contains("Properties") && entry->at("Properties").is_object()) {
        const json& props = entry->at("Properties");
        for (const std::string& key : {"Event_FP", "Event_TP"}) {
            if (props.contains(key) && props[key].is_object()) {
                std::string n = props[key].value("ObjectName", "");
                if (!n.empty() && n != "None") return n;
            }
        }
    }
    return entry->value("Name", "Unknown");
}

std::pair<bool, std::string> process_json(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {false, "Cannot open file"};

    json data;
    try {
        f >> data;
    } catch (...) {
        return {false, "Failed to parse JSON"};
    }
    f.close();

    const json* anim = nullptr;
    for (const auto& item : data) {
        std::string type = item.value("Type", "");
        if (type == "AnimSequence" || type == "AnimMontage") {
            anim = &item;
            break;
        }
    }
    if (!anim) return {false, "AnimSequence/AnimMontage not found"};

    const json& props = (*anim)["Properties"];
    const json& notifies = props.value("Notifies", json::array());
    double num_frames = props.value("NumFrames", 0.0);
    double seq_length = props.value("SequenceLength", 1.0);
    double fps = (seq_length != 0) ? num_frames / seq_length : 0.0;

    std::unordered_map<std::string, const json*> sound_map;
    for (const auto& e : data) {
        if (e.value("Type", "") == "AnimNotify_WeaponSound") {
            std::string key = "AnimNotify_WeaponSound'" + e.value("Outer", "") + ":" + e.value("Name", "") + "'";
            sound_map[key] = &e;
        }
    }

    std::vector<std::string> result;
    for (const auto& n : notifies) {
        std::string event_type = n.value("NotifyName", "Unknown");
        double t = n.value("Time", n.value("LinkValue", 0.0));
        int frame = static_cast<int>(t * fps);

        std::string name;
        if (event_type == "WeaponSound") {
            std::string ref = n.value("Notify", json::object()).value("ObjectName", "");
            name = extract_sound_name(sound_map.count(ref) ? sound_map[ref] : nullptr);
        } else {
            name = event_type;
        }

        std::ostringstream line;
        line << std::fixed << std::setprecision(6)
             << t << "\t" << frame << "\t" << event_type << "\t" << name;
        result.push_back(line.str());
    }

    if (result.empty()) return {false, "No notifies found"};

    std::string out = path.substr(0, path.find_last_of('.')) + "_events.txt";
    std::ofstream fo(out);
    for (const auto& line : result) fo << line << "\n"; // без заголовка
    fo.close();

    return {true, "Saved → " + out};
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "\nDrag & Drop JSON onto this EXE\n";
        return 0;
    }

    std::string path = argv[1];
    std::ifstream check(path);
    if (!check.good() || path.size() < 5 || path.substr(path.size() - 5) != ".json") {
        std::cout << "Invalid or non-json file.\n";
        return 1;
    }
    check.close();

    auto [ok, msg] = process_json(path);
    std::cout << msg << "\n";

    return 0;
}
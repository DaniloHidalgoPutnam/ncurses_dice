
#include "../../include/DiceModel.h"
#include <iostream>
#include <fstream>
#include <memory>

DiceModel::DiceModel() {
    std::string line, *key, *value;
    key = value = nullptr;
    std::ifstream file;
    file.open(CONFIG_PATH);
    std::string currSection;
    //defaults, in case not found in the file
    aces = false;
    logPath = DEFAULT_LOG_PATH;
    delayMicroSeconds = DEFAULT_DELAY;
    if (file.is_open()) {
        while (getline(file, line)) {
            if (lineIsKey(line, keyAces)) {
                value = extractValue(line, keyAces.length());
                aces = *value == "0";
                delete value;
            } else if (line.rfind(keyLogPath, 0) == 0) {
                value = extractValue(line, keyLogPath.length());
                logPath = *value;
                delete value;
            } else if (line.rfind(keyDelay, 0) == 0) {
                try { //stol is only >c++11
                    value = extractValue(line, keyDelay.length());
                    delayMicroSeconds = std::stol(*value);
                    delete value;
                } catch (std::invalid_argument &e) {
                    delete value;
                }
            } else if (line.rfind('[', 0) == 0) {
                currSection = line;
            }
            if (currSection == sectionRolls) {
                key = extractKey(line);
                value = extractValue(line, 0);
                if (key != nullptr && value != nullptr) {
                    savedRolls.emplace(*key, *value);
                }
                delete key; delete value;
            }
        }
        file.close();
    }
}

DiceModel::~DiceModel() {
    savedRolls.clear();
}

std::string *DiceModel::extractValue(std::string &line, int startAt) {
    int index = line.find('=', startAt) + 1;
    std::string *result;
    if (index == std::string::npos) {
        return nullptr;
    }
    result = new std::string(line.substr(index));
    return result;
}

std::string *DiceModel::extractKey(std::string &line) {
    int index = line.find('=');
    std::string *result;
    if (index == std::string::npos) {
        return nullptr;
    }
    result = new std::string(line.substr(0, index));
    return result;
}

bool DiceModel::isAcing() const {
    return aces;
}

void DiceModel::toggleAces() {
    aces = !aces;
    updateConfig(keyAces, aces ? "1" : "0", sectionSettings);
}

const std::string &DiceModel::getLogPath() const {
    return logPath;
}

long DiceModel::getDelay() const {
    return delayMicroSeconds;
}

void DiceModel::setDelay(long delay) {
    this->delayMicroSeconds = delay;
    updateConfig(keyDelay, std::to_string(delay), sectionSettings);
}

void
DiceModel::updateConfig(const std::string &key, const std::string &value,
                        const std::string &section) {
    if (section == sectionRolls) savedRolls.emplace(key, value);
    bool inTheRightSection = false, updated = false;
    std::string line;
    std::fstream original;
    std::ofstream newFile;
    original.open(CONFIG_PATH, std::ios::out | std::ios::in);
    newFile.open(DEFAULT_TEMP_PATH, std::ios::out);
    if (original.is_open() && newFile.is_open()) {
        while (getline(original, line)) {
            if (line == section) {
                inTheRightSection = true;
                newFile << line << std::endl;
                continue;
            } if (inTheRightSection && line.rfind('[', 0) == 0) {
                break;
            } if (lineIsKey(line, key) && inTheRightSection) {
                newFile << key << '=' << value << std::endl;
                updated = true;
                inTheRightSection = false;
            } else {
                newFile << line << std::endl;
            }
        }
        if (updated) {
            std::remove(CONFIG_PATH);
            std::rename(DEFAULT_TEMP_PATH, CONFIG_PATH);
        } else {
            std::remove(DEFAULT_TEMP_PATH);
        }
    }
    if (original.is_open()) original.close();
    if (newFile.is_open()) newFile.close();
}

void DiceModel::removeLineFromConfig(const std::string &key,
                                     const std::string &section) {
    if (section == sectionRolls) savedRolls.erase(key);
    bool inTheRightSection = false, removed = false;
    std::string line;
    std::fstream original;
    std::ofstream newFile;
    original.open(CONFIG_PATH, std::ios::out | std::ios::in);
    newFile.open(DEFAULT_TEMP_PATH, std::ios::out);
    if (original.is_open() && newFile.is_open()) {
        while (getline(original, line)) {
            if (line == section) {
                inTheRightSection = true;
                newFile << line << std::endl;
                continue;
            } if (inTheRightSection && line.rfind('[', 0) == 0) {
                break;
            } else if (!lineIsKey(line, key) || !inTheRightSection) {
                newFile << line << std::endl;
            } else {
                removed = true;
                inTheRightSection = false; //have to or it'll break above
            }
        }
        if (removed) {
            std::remove(CONFIG_PATH);
            std::rename(DEFAULT_TEMP_PATH, CONFIG_PATH);
        } else {
            std::remove(DEFAULT_TEMP_PATH);
        }
    }
    if (original.is_open()) original.close();
    if (newFile.is_open()) newFile.close();
}

void DiceModel::addLineToConfig(const std::string &key, const std::string &value,
        const std::string &section) {
    if (section == sectionRolls) savedRolls.emplace(key, value);
    std::string line;
    std::fstream original;
    std::ofstream newFile;
    original.open(CONFIG_PATH, std::ios::out | std::ios::in);
    newFile.open(DEFAULT_TEMP_PATH, std::ios::out);
    if (original.is_open() && newFile.is_open() && !configContainsKey(key)) {
        while (getline(original, line)) {
            newFile << line << std::endl;
            if (line == section) { //the line we want to modify
                newFile << key << '=' << value << std::endl;
            }
        }
        std::remove(CONFIG_PATH);
        std::rename(DEFAULT_TEMP_PATH, CONFIG_PATH);
    }
    if (original.is_open()) original.close();
    if (newFile.is_open()) newFile.close();
}

bool DiceModel::configContainsKey(const std::string &key) {
    std::string line;
    std::ifstream file;
    file.open(CONFIG_PATH);
    if (file.is_open()) {
        while (getline(file, line)) {
            if (lineIsKey(line, key)) {
                file.close();
                return true;
            }
        }
        file.close();
    } else {
        return true; //file could not be opened
    }
    return false; //the key was not found
}

std::string DiceModel::getSavedRoll(const std::string &key) {
    if (savedRolls.find(key) != savedRolls.end())
        return savedRolls[key];
    return "";
}

bool DiceModel::lineIsKey(const std::string &line, const std::string &key) {
    return line.rfind(key, 0) == 0 && line.length() > key.length() &&
           line.at(key.length()) == '=';
}

int DiceModel::getNumSavedRolls() {
    return savedRolls.size();
}

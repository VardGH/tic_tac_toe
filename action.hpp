#pragma once

#include <string>
#include <cassert>
#include <cstring>

enum class action : unsigned short {
    TURN,
    MOVE,
    INVALID,
    COUNT,
    UPDATE,
    WAIT,
    LOSE,
    DRAW,
    HOLD,
    START,
    WIN,
};

action string_to_action(const std::string& act)
{
    if (act == "TRN") {
        return action::TURN;
    } else if (act == "MOV") {
        return action::MOVE;
    } else if (act == "INV") {
        return action::INVALID;
    } else if (act == "CNT") {
        return action::COUNT;
    } else if (act == "UPD") {
        return action::UPDATE;
    } else if (act == "WIT") {
        return action::WAIT;
    } else if (act == "LSE") {
        return action::LOSE;
    } else if (act == "DRW") {
        return action::DRAW;
    } else if (act == "HLD") {
        return action::HOLD;
    } else if (act == "SRT") {
        return action::START;
    } else if (act == "WIN") {
        return action::WIN;
    }
    assert(false);
}

std::string action_to_string(action act)
{
    switch (act){
    case action::TURN:
        return "TRN";
    case action::MOVE:
        return "MVE";
    case action::INVALID:
        return "INV";
    case action::COUNT:
        return "CNT";
    case action::UPDATE:
        return "UPD";
    case action::WAIT:
        return "WIT";
    case action::LOSE:
        return "LSE";
    case action::DRAW:
        return "DRW";
    case action::HOLD:
        return "HLD";
    case action::START:
        return "SRT";
    case action::WIN:
        return "WIN";
    }
    assert(false);
    return "";
}
#include "sheet.h"

#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>

using namespace std;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, string text) {
    if(!pos.IsValid()) {
        throw InvalidPositionException("wrong position"s);
    }
    auto temp_cell = move(std::make_unique<Cell>(*this));
    temp_cell->Set(text);
    if(CheckCircularDependencies(temp_cell.get(), pos)) {
        throw CircularDependencyException("Circular dependency"s);
    }
    data_[pos] = move(temp_cell);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if(!pos.IsValid()) {
        throw InvalidPositionException("wrong position"s);
    }
    auto iter = data_.find(pos);
    if(iter != data_.end()) {
        return data_.at(pos).get();
    }
    return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    if(!pos.IsValid()) {
        throw InvalidPositionException("wrong position"s);
    }
    auto iter = data_.find(pos);
    if(iter != data_.end()) {
        return data_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if(pos.IsValid()) {
       data_.erase(pos);
    }
    else {
        throw InvalidPositionException("wrong position"s);
    }
}

Size Sheet::GetPrintableSize() const {
    if(data_.empty()) {
        return {0, 0};
    }
    auto left_right = GetCorners();
    return {left_right.second.row - left_right.first.row + 1,left_right.second.col - left_right.first.col + 1};
}

struct ValueToStringVisitor {
    string operator() (string str) const {
        return str;
    }
    string operator() (double d) const {
        ostringstream os;
        os << d;
        return os.str();
    }
    string operator() (FormulaError f) const {
        ostringstream os;
        os << f;
        return os.str();
    }
};

void Sheet::PrintValues(ostream& output) const {
    if(data_.empty()) {
        return;
    }
    Position left_corner{}; Position right_corner{};
    tie(left_corner, right_corner) = GetCorners();
    for(int i = left_corner.row; i <= right_corner.row; ++i) {
        for(int j = left_corner.col; j <= right_corner.col; ++j) {
            auto current_cell = GetCell({i, j});
            if(current_cell) {
                output << visit(ValueToStringVisitor(), current_cell->GetValue());
            }
            if(j < right_corner.col) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(ostream& output) const {
    if(data_.empty()) {
        return;
    }
    Position left_corner{}; Position right_corner{};
    tie(left_corner, right_corner) = GetCorners();
    for(int i = left_corner.row; i <= right_corner.row; ++i) {
        for(int j = left_corner.col; j <= right_corner.col; ++j) {
            auto current_cell = GetCell({i, j});
            if(current_cell) {
                output << current_cell->GetText();
            }
            if(j < right_corner.col) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

size_t Sheet::position_hasher::operator() (const Position& p) const {
    return hash<int>()(p.row) ^ hash<int>()(p.col);
}

pair<Position, Position> Sheet::GetCorners() const {
    if(data_.empty()) {
        return {{0,0}, {0, 0}};
    }
    else if(data_.size() == 1u) {
        auto pos = data_.begin()->first;
        return {{0, 0}, pos};
    }
    Position right = Position::NONE;
    for(const auto& [key, _] : data_) {
        if(right.col < key.col){
            right.col = key.col;
        }else{
            right.col = right.col;
        }
        if(right.row < key.row){
            right.row = key.row;
        }else{
            right.row = right.row;
        }
    }
    return {{0,0}, right};
}

bool Sheet::CheckCircularDependencies(const CellInterface* cell, Position head) const {
    if(!cell) {
        return false;
    }
    bool res = false;
    for(const auto& next_cell : cell->GetReferencedCells()) {
        if(next_cell == head) {
            return true;
        }
        res = res || CheckCircularDependencies(GetCell(next_cell), head);
    }
    return res;
}

unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
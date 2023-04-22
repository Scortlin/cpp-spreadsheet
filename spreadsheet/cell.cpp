#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

using namespace std;

class Cell::Impl {
public:
    virtual ~Impl() = default; 
    virtual Cell::Value GetValue(SheetInterface& sheet) const = 0;
    virtual string GetString() const = 0;
    virtual vector<Position> GetReferencedCells() const {
        return {};
    }
};

class Cell::EmptyImpl final : public Cell::Impl {
public:
    EmptyImpl() = default;
    Cell::Value GetValue(SheetInterface& sheet) const override {
        return ""s;
    }
    string GetString() const override {
        return ""s;
    }
};

class Cell::TextImpl final : public Cell::Impl {
public:
    explicit TextImpl(string text) 
        : data_(move(text)) {}
        
    Cell::Value GetValue(SheetInterface& sheet) const override {
        return data_;
    }
    
    string GetString() const override {
        return data_;
    }
private:
    const string data_;
};

class Cell::FormulaImpl final : public Cell::Impl {
private:
    struct FormulaVisitor {
        Cell::Value operator() (double d) {
            return Cell::Value{d};
        }
        Cell::Value operator() (FormulaError f) {
            return Cell::Value{f};
        }
    };
public:
    explicit FormulaImpl(string formula) 
        : data_(ParseFormula(move(formula))){}
    
    Cell::Value GetValue(SheetInterface& sheet) const override {
        return visit(FormulaVisitor(), data_->Evaluate(sheet));
    }
    
    string GetString() const override {
        return "="s.append(data_->GetExpression());
    }
    
    vector<Position> GetReferencedCells() const override {
        return data_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> data_;
};

Cell::Cell(Sheet& sheet) 
    : impl_(make_unique<EmptyImpl>(EmptyImpl{})) , sheet_(sheet) {}

Cell::~Cell() = default;

bool Cell::IsModified() const {
    return cache_;
}

void Cell::SetCache(Value&& val) const {
    cache_.val_ = forward<Value>(val);
    cache_.modification_flag_ = false;
}

void Cell::Set(string text) {
    if(text.empty()) {
        impl_ = make_unique<EmptyImpl>();
    }
    else if(text.front() == '=' && text.size() > 1u) {
        try {
            impl_ = make_unique<FormulaImpl>(FormulaImpl{text.substr(1u)});
            for(const auto& ref_cell : impl_->GetReferencedCells()) {
                auto cell_ptr = sheet_.GetCell(ref_cell);
                if(!cell_ptr) {
                    sheet_.SetCell(ref_cell, ""s);
                }
            }
        }
        catch(...) {
            throw FormulaException{"unable to parse: "s.append(text)};
        }
    }
    else {
        impl_ = make_unique<TextImpl>(TextImpl{text});
    }
    cache_.modification_flag_ = true;
}

void Cell::Clear() {
    impl_.reset(nullptr);
    cache_.modification_flag_ = true;
}

struct ValueVisitor {
    Cell::Value operator() (string str) const {
        return str.front() == '\'' ? str.substr(1u) : str;
    }
    Cell::Value operator() (double d) const {
        return d;
    }
    Cell::Value operator() (FormulaError f) const {
        return f;
    }
};

Cell::Value Cell::GetValue() const {
    const auto& dependency = GetReferencedCells();
    if(!cache_ || any_of(dependency.begin(), dependency.end(), 
        [this] (const auto& pos) {const Cell* cell_ptr = reinterpret_cast<const Cell*>(sheet_.GetCell(pos));return cell_ptr->IsModified();})) {
        auto value = impl_ ? visit(ValueVisitor(), impl_->GetValue(sheet_)) : Cell::Value{};
        
        SetCache(move(value));
    }
    return cache_;
}

std::string Cell::GetText() const {
    if(impl_) {
        return impl_->GetString();
    }
    return {};
}
Cell::CellCache::operator bool() const {
    return !modification_flag_;
}

Cell::CellCache::operator Value() const {
    return val_;
}
std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}
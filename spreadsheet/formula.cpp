#include "formula.h"
#include "common.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <sstream>

using namespace std;

ostream& operator<<(ostream& output, FormulaError f) {
    return output << f.ToString();
}

class Formula : public FormulaInterface {
public:
    explicit Formula(string expression): ast_(ParseFormulaAST(move(expression))){
        auto cells = ast_.GetCells();
        cells.unique();
        cells.sort();
        cells_ = {cells.begin(), cells.end()};
    }
    
    Value Evaluate(const SheetInterface& arg) const override {
        try {
            double res = ast_.Execute(arg);
            return res;
        }
        catch(const FormulaError& f) {
            return f;
        }
    }
    string GetExpression() const override {
        ostringstream os;
        ast_.PrintFormula(os);
        return os.str();
    }
    vector<Position> GetReferencedCells() const override {
        return cells_;
    }

private:
    FormulaAST ast_;
    vector<Position> cells_;
};

unique_ptr<FormulaInterface> ParseFormula(string expr) {
    try {
        return make_unique<Formula>(move(expr));
    }
    catch(...) {
        throw FormulaException{"unable to parse: "s.append(expr)};
    }
}

FormulaError::FormulaError(Category category) 
    : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

string_view FormulaError::ToString() const {
       
    switch(category_) {
        case FormulaError::Category::Ref: {
            return "#REF!"sv;
        }
        case FormulaError::Category::Value: {
            return "#VALUE!"sv;
        }
        case FormulaError::Category::Div0: {
            return "#DIV/0!"sv;
        }
        default:
            throw runtime_error{"Wrong FE type"s};
    }
}
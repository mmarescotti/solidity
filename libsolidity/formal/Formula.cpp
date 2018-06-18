// Author: Matteo Marescotti


#include <libsolidity/formal/Formula.h>


using namespace std;
using namespace dev::solidity;

std::string const &to_string(Sort const _sort) {
    static std::map<Sort, std::string> const strings{
            {Sort::Bool, "Bool"},
            {Sort::Int,  "Int"}
    };
    return strings.at(_sort);
}

const Formula::sexpr_t Formula::sexpr(const string &_nonce) const {
    if (m_arguments.empty()) {
        return {{}, m_name};
    } else {
        std::ostringstream ss;
        std::set<std::string> d;
        ss << "(" << m_name;
        for (auto const &argument:m_arguments) {
            auto &pair = argument->sexpr(_nonce);
            d.insert(pair.first.begin(), pair.first.end());
            ss << " " << pair.second;
        }
        ss << ")";
        return {d, ss.str()};
    }
}

const Formula::sexpr_t FormulaVariable::sexpr(const string &_nonce) const {
    std::string name = m_name + (_nonce.empty() ? "" : "." + _nonce);
    return {{"(declare-const " + name + " " + to_string(sort) + ")"}, name};
}

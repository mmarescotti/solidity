// Author: Matteo Marescotti

#pragma once

#include <libsolidity/ast/AST.h>
#include <map>
#include <set>
#include <vector>
#include <sstream>

namespace dev {
    namespace solidity {

        enum class Sort {
            Bool,
            Int
        };

        class Formula {
        public:
            explicit Formula(bool _v) : Formula((_v ? "true" : "false"), Sort::Bool) {}

            Formula(size_t _number) : Formula(std::to_string(_number), Sort::Int) {}

            Formula(u256 const &_number) : Formula(_number.str(), Sort::Int) {}

            Formula(bigint const &_number) : Formula(_number.str(), Sort::Int) {}

            Formula(Formula const &) = default;

            Formula(Formula &&) = default;

            Formula &operator=(Formula const &) = default;

            Formula &operator=(Formula &&) = default;

            bool hasCorrectArity() const {
                static std::map<std::string, unsigned> const operatorsArity{
                        {"ite", 3},
                        {"not", 1},
                        {"and", 2},
                        {"or",  2},
                        {"=",   2},
                        {"<",   2},
                        {"<=",  2},
                        {">",   2},
                        {">=",  2},
                        {"+",   2},
                        {"-",   2},
                        {"*",   2},
                        {"/",   2}
                };
                return m_arguments.empty() ||
                       (operatorsArity.count(m_name) && operatorsArity.at(m_name) == m_arguments.size());
            }

            using sexpr_t = std::pair<std::set<std::string>, std::string>;

            virtual sexpr_t const sexpr(std::string const &_nonce = "") const;

            inline bool isTrue() const {
                return m_name == "true";
            }

            inline bool isFalse() const {
                return m_name == "false";
            }

            template<class T1, class T2, class T3>
            static Formula const ite(T1 const &_condition,
                                     T2 const &_trueValue,
                                     T3 const &_falseValue) {
                return Formula("ite", _condition, _trueValue, _falseValue, _trueValue.sort);
            }

            template<class T1, class T2>
            static Formula const implies(T1 const &_arg1, T2 const &_arg2) {
                return !_arg1 || _arg2;
            }

            template<class T1>
            friend Formula const operator!(T1 const &_arg1) {
                return Formula("not", _arg1, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator&&(T1 const &_arg1, T2 const &_arg2) {
                return Formula("and", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator||(T1 const &_arg1, T2 const &_arg2) {
                return Formula("or", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator==(T1 const &_arg1, T2 const &_arg2) {
                return Formula("=", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator!=(T1 const &_arg1, T2 const &_arg2) {
                return !(_arg1 == _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator<(T1 const &_arg1, T2 const &_arg2) {
                return Formula("<", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator<=(T1 const &_arg1, T2 const &_arg2) {
                return Formula("<=", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator>(T1 const &_arg1, T2 const &_arg2) {
                return Formula(">", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator>=(T1 const &_arg1, T2 const &_arg2) {
                return Formula(">=", _arg1, _arg2, Sort::Bool);
            }

            template<class T1, class T2>
            friend Formula const operator+(T1 const &_arg1, T2 const &_arg2) {
                return Formula("+", _arg1, _arg2, Sort::Int);
            }

            template<class T1, class T2>
            friend Formula const operator-(T1 const &_arg1, T2 const &_arg2) {
                return Formula("-", _arg1, _arg2, Sort::Int);
            }

            template<class T1, class T2>
            friend Formula const operator*(T1 const &_arg1, T2 const &_arg2) {
                return Formula("*", _arg1, _arg2, Sort::Int);
            }

            template<class T1, class T2>
            friend Formula const operator/(T1 const &_arg1, T2 const &_arg2) {
                return Formula("/", _arg1, _arg2, Sort::Int);
            }

//            Formula const operator()(Formula const &_arg1) const {
//                solAssert(
//                        m_arguments.empty(),
//                        "Attempted function application to non-function."
//                );
//                switch (sort) {
//                    case Sort::IntIntFun:
//                        return Formula(name, _arg1, Sort::Int);
//                    case Sort::IntBoolFun:
//                        return Formula(name, _arg1, Sort::Bool);
//                    default:
//                        solAssert(
//                                false,
//                                "Attempted function application to invalid type."
//                        );
//                        break;
//                }
//            }

            Sort const sort;
        protected:
            Formula(std::string const &_name, Sort const _sort) :
                    sort(_sort), m_name(_name) {}

            std::string const m_name;

        private:
            template<class T1>
            Formula(std::string const &_name, T1 const &_arg1, Sort const _sort) :
                    Formula(_name, _sort) {
                m_arguments.push_back(std::make_shared<Formula>(new T1(_arg1)));
            }

            template<class T1, class T2>
            Formula(std::string const &_name, T1 const &_arg1, T2 const &_arg2, Sort const _sort) :
                    Formula(_name, _sort) {
                m_arguments.push_back(std::make_shared<Formula>(new T1(_arg1)));
                m_arguments.push_back(std::make_shared<Formula>(new T2(_arg2)));
            }

            template<class T1, class T2, class T3>
            Formula(std::string const &_name, T1 const &_arg1, T2 const &_arg2, T3 const &_arg3, Sort const _sort) :
                    Formula(_name, _sort) {
                m_arguments.push_back(std::make_shared<Formula>(new T1(_arg1)));
                m_arguments.push_back(std::make_shared<Formula>(new T2(_arg2)));
                m_arguments.push_back(std::make_shared<Formula>(new T3(_arg3)));
            }

            std::vector<std::shared_ptr<Formula>> m_arguments;
        };


        class FormulaVariable : public Formula {
        public:
            FormulaVariable(std::string const &_name, Sort const _sort) : Formula(_name, _sort) {}

            sexpr_t const sexpr(std::string const &_nonce = "") const override;
        };

    }
}
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
            Formula(size_t _number) : Formula(std::to_string(_number), Sort::Int) {}

            Formula(u256 const &_number) : Formula(_number.str(), Sort::Int) {}

            Formula(bigint const &_number) : Formula(_number.str(), Sort::Int) {}

            Formula() : Formula(True()) {}

            Formula(Formula const &) = default;

            Formula(Formula &&) = default;

            Formula &operator=(Formula const &) = default;

            Formula &operator=(Formula &&) = default;

            virtual ~Formula() {}

            using sexpr_t = std::pair<std::set<std::string>, std::string>;

            virtual sexpr_t const sexpr(std::string const &_nonce = "") const;

            static const Formula True() {
                static Formula _true("true", Sort::Bool);
                return _true;
            }

            static const Formula False() {
                static Formula _false("false", Sort::Bool);
                return _false;
            };

            inline bool isTrue() const {
                return m_name == "true";
            }

            inline bool isFalse() const {
                return m_name == "false";
            }

            template<class T>
            static Formula const And(std::vector<T> const &_args) {
                return Formula("and", Sort::Bool, _args);
            }

            template<class T>
            static Formula const Or(std::vector<T> const &_args) {
                return Formula("or", Sort::Bool, _args);
            }

            template<class... Args>
            static Formula const And(Args const &... _args) {
                return Formula("and", Sort::Bool, _args...);
            }

            template<class... Args>
            static Formula const Or(Args const &... _args) {
                return Formula("or", Sort::Bool, _args...);
            }

            template<class T1, class T2, class T3>
            static Formula const ITE(T1 const &_condition,
                                     T2 const &_trueValue,
                                     T3 const &_falseValue) {
                return Formula("ite", _trueValue.sort, _condition, _trueValue, _falseValue);
            }

            template<class T1, class T2>
            static Formula const Implies(T1 const &_arg1, T2 const &_arg2) {
                return !_arg1 || _arg2;
            }

            template<class T1>
            friend Formula const operator!(T1 const &_arg1) {
                return Formula("not", Sort::Bool, _arg1);
            }

            template<class T1, class T2>
            friend Formula const operator&&(T1 const &_arg1, T2 const &_arg2) {
                return Formula("and", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator||(T1 const &_arg1, T2 const &_arg2) {
                return Formula("or", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator==(T1 const &_arg1, T2 const &_arg2) {
                return Formula("=", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator!=(T1 const &_arg1, T2 const &_arg2) {
                return !(_arg1 == _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator<(T1 const &_arg1, T2 const &_arg2) {
                return Formula("<", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator<=(T1 const &_arg1, T2 const &_arg2) {
                return Formula("<=", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator>(T1 const &_arg1, T2 const &_arg2) {
                return Formula(">", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator>=(T1 const &_arg1, T2 const &_arg2) {
                return Formula(">=", Sort::Bool, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator+(T1 const &_arg1, T2 const &_arg2) {
                return Formula("+", Sort::Int, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator-(T1 const &_arg1, T2 const &_arg2) {
                return Formula("-", Sort::Int, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator*(T1 const &_arg1, T2 const &_arg2) {
                return Formula("*", Sort::Int, _arg1, _arg2);
            }

            template<class T1, class T2>
            friend Formula const operator/(T1 const &_arg1, T2 const &_arg2) {
                return Formula("/", Sort::Int, _arg1, _arg2);
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

            template<class T>
            Formula(std::string const &_name, Sort const _sort, std::vector<T> const &_args) :
                    Formula(_name, _sort) {
                for (auto &arg:_args) {
                    init_arguments(arg);
                }
            }

            template<class... Args>
            Formula(std::string const &_name, Sort const _sort, Args const &..._args) :
                    Formula(_name, _sort) {
                init_arguments(_args...);
            }

            template<class T, class... Args>
            void init_arguments(T const &_arg, Args const &..._args) {
                init_arguments(_arg);
                init_arguments(_args...);
            };

            template<class T>
            void init_arguments(T const &_arg) {
                if (std::is_base_of<Formula, T>::value)
                    m_arguments.push_back(std::shared_ptr<Formula>((Formula *) new T(_arg)));
                else
                    m_arguments.push_back(std::shared_ptr<Formula>(new Formula(_arg)));
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
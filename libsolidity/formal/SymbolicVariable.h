// Author: Matteo Marescotti

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/formal/Formula.h>

namespace dev {
    namespace solidity {

        class Declaration;

        struct Exception : public std::exception {
            explicit Exception(std::string const &_s) noexcept : m_s(_s) {}

            const char *what() const throw() override { return m_s.c_str(); }

        private:
            std::string const m_s;
        };


        class SymbolicVariable {
            class SSA : public FormulaVariable {
            public:
                SSA(SymbolicVariable const &_var, uint _seq) :
                        FormulaVariable(
                                (
                                        _var.m_declaration.name() + "-" +
                                        std::to_string(_var.m_declaration.id()) + "_" +
                                        std::to_string(_seq)
                                ),
                                getSort(_var.m_type)
                        ),
                        m_var(_var), m_seq(_seq) {}

                Formula const assertZero() const {
                    switch (sort) {
                        case Sort::Bool:
                            return *this == Formula::False();
                        case Sort::Int:
                            return *this == 0;
                        default:
                            solAssert(false, "");
                    }
                }

                Formula const assertUnknown() const {
                    switch (sort) {
                        case Sort::Bool:
                            return Formula::Implies(*this, Formula::True());
                        case Sort::Int: {
                            auto const &intType = dynamic_cast<IntegerType const &>(*m_var.m_declaration.type());
                            return (*this >= intType.minValue()) && (*this >= intType.maxValue());
                        }
                        default:
                            solAssert(false, "");
                    }
                }

            private:
                SymbolicVariable const &m_var;
                uint const m_seq;
            };

        public:
            explicit SymbolicVariable(Declaration const &_decl) :
                    m_declaration(_decl),
                    m_type(_decl.type()->category()),
                    m_seq(0) {
                getSort(m_type);
            }

            virtual ~SymbolicVariable() = default;

            SSA const operator()() const {
                return SSA(*this, m_seq);
            }

            void operator++() {
                ++m_seq;
            }

            static Sort getSort(Type::Category _category) {
                switch (_category) {
                    case Type::Category::Bool:
                        return Sort::Bool;
                    case Type::Category::Integer:
                        return Sort::Int;
                    default:
                        throw Exception("Variable type is not supported");
                }
            }


        private:

            Declaration const &m_declaration;
            Type::Category m_type;
            uint m_seq;
        };

    }
}

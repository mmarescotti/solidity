// Author: Matteo Marescotti

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/interface/ErrorReporter.h>
#include <libsolidity/formal/SymbolicVariable.h>

namespace dev {
    namespace solidity {
        class FSSA {
            class Statement {
            public:
                Statement(Formula const &_condition, Formula const &_expression, SourceLocation const &_location) :
                        m_condition(_condition), m_expression(_expression), m_location(_location) {}

                Formula const getFormula() const {
                    if (m_condition.isTrue()) {
                        return m_expression;
                    } else {
                        return Formula::Implies(m_condition, m_expression);
                    }
                }

                virtual Formula::sexpr_t const sexpr(std::string const &_nonce = "") const {
                    m_location.isEmpty(); //TODO: remove when use location
                    return getFormula().sexpr(_nonce);
                };

            protected:
                Formula const m_condition;
                Formula const m_expression;

            private:
                SourceLocation const &m_location;
            };

//            class CallStatement : public Statement {
//            public:
//                CallStatement(smt::Expression const &_condition,
//                              smt::Expression const &_return,
//                              FunctionDefinition const &_function);
//            };

        public:
            FSSA(ContractDefinition const &, FunctionDefinition const &, ErrorReporter &);

            void assignment(Declaration const &_variable, Expression const &_value, SourceLocation const &_location);

            void createExpr(Expression const &_e);

            void createExpr(UnaryOperation const &_op);

            void createExpr(BinaryOperation const &_op);

            void defineExpr(Expression const &_e, Expression const &_value);

            template<typename T>
            void defineExpr(Expression const &_e, T t) {
                defineExpr(_e, Formula(t));
            }

            void defineExpr(Expression const &_e, Declaration const &_variable);

            void pushPathCondition(Expression const &_e, bool _sign = true);

            void popPathCondition();

            Formula const getFormula();

            std::vector<Statement> const &getStatements() {
                return m_statements;
            }

        private:
            SymbolicVariable *createVariable(VariableDeclaration const &_varDecl,
                                             std::map<Declaration const *, SymbolicVariable> &_vec);

            SymbolicVariable *getVariable(Declaration const &_decl);

            Formula const &expr(Expression const &_e);

            void defineExpr(Expression const &_e, Formula const &_value);

            void arithmeticOperation(BinaryOperation const &_op);

            void compareOperation(BinaryOperation const &_op);

            void booleanOperation(BinaryOperation const &_op);

            void assignment(Declaration const &_variable,
                            Formula const &_value,
                            SourceLocation const &_location);

            Formula const currentPathCondition();

            void addStatement(Statement const &&_statement);

            //std::shared_ptr<smt::SolverInterface> m_interface;
            ContractDefinition const &m_contract;
            FunctionDefinition const &m_function;
            ErrorReporter &m_errorReporter;
            std::map<Declaration const *, SymbolicVariable> m_stateVariables;
            std::map<Declaration const *, SymbolicVariable> m_parameters;
            std::map<Declaration const *, SymbolicVariable> m_locals;
            std::map<Declaration const *, SymbolicVariable> m_returns;
            std::map<Expression const *, Formula> m_expressions;
            std::vector<Formula> m_pathConditions;
            std::vector<Statement> m_statements;
        };
    }
}


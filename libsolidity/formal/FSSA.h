#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/formal/SSAVariable.h>
#include <libsolidity/formal/SolverInterface.h>
#include <libsolidity/interface/ErrorReporter.h>


namespace dev {
    namespace solidity {
        class FSSA {
            class Statement {
            public:
                Statement(smt::Expression const &_condition,
                          smt::Expression const &_expression,
                          SourceLocation const &_location);

                smt::Expression const expr() const;

            protected:
                smt::Expression const m_expression;

            private:
                smt::Expression const m_condition;
                SourceLocation const &m_location;
            };

            class CallStatement : public Statement {
            public:
                CallStatement(smt::Expression const &_condition,
                              smt::Expression const &_return,
                              FunctionDefinition const &_function);
            };

        public:
            FSSA(ContractDefinition const &, FunctionDefinition const &, ErrorReporter &);

            void assignment(Declaration const &_variable, Expression const &_value, SourceLocation const &_location);

            void createExpr(Expression const &_e);

            void createExpr(UnaryOperation const &_op);

            void createExpr(BinaryOperation const &_op);

            void defineExpr(Expression const &_e, Expression const &_value);

            template<typename T>
            void defineExpr(Expression const &_e, T t) {
                defineExpr(_e, smt::Expression(t));
            }

            void defineExpr(Expression const &_e, Declaration const &_variable);

            void pushPathCondition(Expression const &_e, bool _sign = true);

            void popPathCondition();

            std::string to_smtlib();

        private:
            SSAVariable *createVariable(VariableDeclaration const &_varDecl,
                                        std::map<Declaration const *, SSAVariable> &_vec);

            SSAVariable *getVariable(Declaration const &_decl);

            smt::Expression const &expr(Expression const &_e);

            void defineExpr(Expression const &_e, smt::Expression const &_value);

            void arithmeticOperation(BinaryOperation const &_op);

            void compareOperation(BinaryOperation const &_op);

            void booleanOperation(BinaryOperation const &_op);

            void assignment(Declaration const &_variable,
                            smt::Expression const &_value,
                            SourceLocation const &_location);

            smt::Expression const currentPathCondition();

            void addStatement(Statement const &&_statement);

            std::shared_ptr<smt::SolverInterface> m_interface;
            ContractDefinition const &m_contract;
            FunctionDefinition const &m_function;
            ErrorReporter &m_errorReporter;
            std::map<Declaration const *, SSAVariable> m_stateVariables;
            std::map<Declaration const *, SSAVariable> m_parameters;
            std::map<Declaration const *, SSAVariable> m_locals;
            std::map<Declaration const *, SSAVariable> m_returns;
            std::map<Expression const *, smt::Expression> m_expressions;
            std::vector<smt::Expression> m_pathConditions;
            std::vector<Statement> m_statements;
        };
    }
}


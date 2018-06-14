//
// Created by Matteo on 07/06/2018.
//

#ifdef HAVE_Z3

#include <libsolidity/formal/Z3Interface.h>
#include <exception>

#elif HAVE_CVC4
#include <libsolidity/formal/CVC4Interface.h>
#else
#include <libsolidity/formal/SMTLib2Interface.h>
#endif

#include "FSSA.h"

using namespace std;
using namespace dev;
using namespace dev::solidity;


FSSA::Statement::Statement(smt::Expression const &_condition,
                           smt::Expression const &_expression,
                           SourceLocation const &_location) :
        m_condition(_condition), m_expression(_expression), m_location(_location) {}

smt::Expression const FSSA::Statement::expr() const {
    return smt::Expression::implies(m_condition, m_expression);
}


FSSA::CallStatement::CallStatement(smt::Expression const &_condition,
                                   smt::Expression const &_return,
                                   FunctionDefinition const &_function) :
        Statement(_condition, _return, _function.location()) {}


FSSA::FSSA(ContractDefinition const &_contract, FunctionDefinition const &_function, ErrorReporter &_errorReporter) :
#ifdef HAVE_Z3
        m_interface(make_shared<smt::Z3Interface>()),
#elif HAVE_CVC4
m_interface(make_shared<smt::CVC4Interface>()),
#else
m_interface(make_shared<smt::SMTLib2Interface>(_readFileCallback)),
#endif
        m_contract(_contract), m_function(_function), m_errorReporter(_errorReporter) {
    SSAVariable *var;

    for (auto const &variable : _contract.stateVariables())
        if (variable->type()->isValueType() && (var = createVariable(*variable, m_stateVariables)))
            var->setUnknownValue();

    for (auto const &param: _function.parameters())
        if ((var = createVariable(*param, m_parameters)))
            var->setUnknownValue();

    for (auto const &variable: _function.localVariables())
        if ((var = createVariable(*variable, m_locals)))
            var->setZeroValue();

    if (!_function.returnParameters().empty()) {
        for (auto const &retParam: _function.returnParameters())
            if ((var = createVariable(*retParam, m_returns)))
                var->setZeroValue();
    }
    m_contract.type();
    m_function.type();
}

SSAVariable *FSSA::createVariable(VariableDeclaration const &_varDecl,
                                  std::map<Declaration const *, SSAVariable> &_vec) {
    if (SSAVariable::isSupportedType(_varDecl.type()->category())) {
        auto i_b = _vec.emplace(&_varDecl, SSAVariable(_varDecl, *m_interface));
        if (i_b.second)
            return &i_b.first->second;
        else {
            m_errorReporter.fatalParserError(
                    _varDecl.location(),
                    "Can't register variable."
            );
        }
    } else {
        m_errorReporter.warning(
                _varDecl.location(),
                "Gas estimator does not yet support the type of this variable."
        );
    }
    return nullptr;
}

SSAVariable *FSSA::getVariable(Declaration const &_decl) {
    for (auto _map: {&m_stateVariables, &m_parameters, &m_locals, &m_returns}) {
        if (_map->count(&_decl)) {
            return &_map->at(&_decl);
        }
    }
    return nullptr;
}

std::string uniqueSymbol(Expression const &_expr) {
    return "expr_" + std::to_string(_expr.id());
}

void FSSA::createExpr(Expression const &_e) {
    if (m_expressions.count(&_e)) {
        m_errorReporter.warning(_e.location(), "Internal error: Expression created twice in SMT solver.");
        return;
    }
    solAssert(_e.annotation().type, "");
    switch (_e.annotation().type->category()) {
        case Type::Category::RationalNumber: {
            if (auto rational = dynamic_cast<RationalNumberType const *>(_e.annotation().type.get()))
                solAssert(!rational->isFractional(), "");
            m_expressions.emplace(&_e, m_interface->newInteger(uniqueSymbol(_e)));
            break;
        }
        case Type::Category::Integer:
            m_expressions.emplace(&_e, m_interface->newInteger(uniqueSymbol(_e)));
            break;
        case Type::Category::Bool:
            m_expressions.emplace(&_e, m_interface->newBool(uniqueSymbol(_e)));
            break;
        default:
            solUnimplementedAssert(false, "Type not implemented.");
    }
}

void FSSA::createExpr(UnaryOperation const &_op) {
    switch (_op.getOperator()) {
        case Token::Not: // !
        {
            solAssert(SSAVariable::isBool(_op.annotation().type->category()), "");
            defineExpr(_op, !expr(_op.subExpression()));
            break;
        }
        case Token::Inc: // ++ (pre- or postfix)
        case Token::Dec: // -- (pre- or postfix)
        {

            solAssert(SSAVariable::isInteger(_op.annotation().type->category()), "");
            solAssert(_op.subExpression().annotation().lValueRequested, "");
            if (auto identifier = dynamic_cast<Identifier const *>(&_op.subExpression())) {
                Declaration const *decl = identifier->annotation().referencedDeclaration;
                if (auto v = getVariable(*decl)) {
                    auto innerValue = (*v)();
                    auto newValue = _op.getOperator() == Token::Inc ? innerValue + 1 : innerValue - 1;
                    assignment(*decl, newValue, _op.location());
                    defineExpr(_op, _op.isPrefixOperation() ? newValue : innerValue);
                } else {
                    m_errorReporter.warning(
                            _op.location(),
                            "Gas estimator does not yet implement such assignments."
                    );
                    return;
                }
            } else
                m_errorReporter.warning(
                        _op.location(),
                        "Gas estimator does not yet implement such increments / decrements."
                );
            break;
        }
        case Token::Add: // +
            defineExpr(_op, expr(_op.subExpression()));
            break;
        case Token::Sub: // -
        {
            defineExpr(_op, 0 - expr(_op.subExpression()));
            break;
        }
        default:
            m_errorReporter.warning(
                    _op.location(),
                    "Assertion checker does not yet implement this operator."
            );
    }
}

void FSSA::createExpr(BinaryOperation const &_op) {
    if (Token::isArithmeticOp(_op.getOperator()))
        arithmeticOperation(_op);
    else if (Token::isCompareOp(_op.getOperator()))
        compareOperation(_op);
    else if (Token::isBooleanOp(_op.getOperator()))
        booleanOperation(_op);
    else
        m_errorReporter.warning(
                _op.location(),
                "Gas estimator does not yet implement this operator."
        );
}

smt::Expression const &FSSA::expr(Expression const &_e) {
    if (!m_expressions.count(&_e)) {
        m_errorReporter.warning(_e.location(), "Gas estimator internal error: Expression undefined for SMT solver.");
        createExpr(_e);
    }
    return m_expressions.at(&_e);
}

void FSSA::defineExpr(Expression const &_e, Expression const &_value) {
    defineExpr(_e, expr(_value));
}

void FSSA::defineExpr(Expression const &_e, Declaration const &_variable) {
    if (auto v = getVariable(_variable)) {
        defineExpr(_e, (*v)());
    } else {
        m_errorReporter.warning(_e.location(), "Unknown variable: " + _variable.name());
    }
}

void FSSA::defineExpr(Expression const &_e, smt::Expression const &_value) {
    createExpr(_e);
    addStatement(Statement(currentPathCondition(), expr(_e) == _value, _e.location()));
}

void FSSA::assignment(Declaration const &_variable,
                      Expression const &_value,
                      SourceLocation const &_location) {
    assignment(_variable, expr(_value), _location);
}

void FSSA::assignment(Declaration const &_variable, smt::Expression const &_value, SourceLocation const &_location) {
    if (auto v = getVariable(_variable)) {
        ++(*v);
        addStatement(Statement(currentPathCondition(), (*v)() == _value, _location));
    } else {
        m_errorReporter.warning(_variable.location(), "Unknown variable: " + _variable.name());
    }
}

smt::Expression division(smt::Expression _left, smt::Expression _right, IntegerType const &_type) {
    // Signed division in SMTLIB2 rounds differently for negative division.
    if (_type.isSigned())
        return (smt::Expression::ite(
                _left >= 0,
                smt::Expression::ite(_right >= 0, _left / _right, 0 - (_left / (0 - _right))),
                smt::Expression::ite(_right >= 0, 0 - ((0 - _left) / _right), (0 - _left) / (0 - _right))
        ));
    else
        return _left / _right;
}

void FSSA::arithmeticOperation(BinaryOperation const &_op) {
    switch (_op.getOperator()) {
        case Token::Add:
        case Token::Sub:
        case Token::Mul:
        case Token::Div: {
            solAssert(_op.annotation().commonType, "");
            solAssert(_op.annotation().commonType->category() == Type::Category::Integer, "");
            auto const intType = dynamic_cast<IntegerType const &>(*_op.annotation().commonType);
            smt::Expression left(expr(_op.leftExpression()));
            smt::Expression right(expr(_op.rightExpression()));
            Token::Value op = _op.getOperator();
            smt::Expression value(
                    op == Token::Add ? left + right :
                    op == Token::Sub ? left - right :
                    op == Token::Div ? division(left, right, intType) :
                    /*op == Token::Mul*/ left * right
            );

//            if (_op.getOperator() == Token::Div) {
//                m_interface->addAssertion(right != 0);
//            }

            defineExpr(_op, value);
            break;
        }
        default:
            m_errorReporter.warning(
                    _op.location(),
                    "Assertion checker does not yet implement this operator."
            );
    }
}


void FSSA::compareOperation(BinaryOperation const &_op) {
    solAssert(_op.annotation().commonType, "");
    if (SSAVariable::isSupportedType(_op.annotation().commonType->category())) {
        smt::Expression left(expr(_op.leftExpression()));
        smt::Expression right(expr(_op.rightExpression()));
        Token::Value op = _op.getOperator();
        shared_ptr<smt::Expression> value;
        if (SSAVariable::isInteger(_op.annotation().commonType->category())) {
            value = make_shared<smt::Expression>(
                    op == Token::Equal ? (left == right) :
                    op == Token::NotEqual ? (left != right) :
                    op == Token::LessThan ? (left < right) :
                    op == Token::LessThanOrEqual ? (left <= right) :
                    op == Token::GreaterThan ? (left > right) :
                    /*op == Token::GreaterThanOrEqual*/ (left >= right)
            );
        } else // Bool
        {
            solUnimplementedAssert(SSAVariable::isBool(_op.annotation().commonType->category()),
                                   "Operation not yet supported");
            value = make_shared<smt::Expression>(
                    op == Token::Equal ? (left == right) :
                    /*op == Token::NotEqual*/ (left != right)
            );
        }
        // TODO: check that other values for op are not possible.
        defineExpr(_op, *value);
    } else
        m_errorReporter.warning(
                _op.location(),
                "Assertion checker does not yet implement the type " + _op.annotation().commonType->toString() +
                " for comparisons"
        );
}

void FSSA::booleanOperation(BinaryOperation const &_op) {
    solAssert(_op.getOperator() == Token::And || _op.getOperator() == Token::Or, "");
    solAssert(_op.annotation().commonType, "");
    if (_op.annotation().commonType->category() == Type::Category::Bool) {
        // @TODO check that both of them are not constant
        if (_op.getOperator() == Token::And)
            defineExpr(_op, expr(_op.leftExpression()) && expr(_op.rightExpression()));
        else
            defineExpr(_op, expr(_op.leftExpression()) || expr(_op.rightExpression()));
    } else
        m_errorReporter.warning(
                _op.location(),
                "Assertion checker does not yet implement the type " + _op.annotation().commonType->toString() +
                " for boolean operations"
        );
}

const smt::Expression smtTrue = smt::Expression(true);

smt::Expression const FSSA::currentPathCondition() {
    if (m_pathConditions.empty())
        return smtTrue;
    return m_pathConditions.back();
}

void FSSA::pushPathCondition(Expression const &_e, bool _sign) {
    m_pathConditions.push_back(currentPathCondition() && (_sign ? expr(_e) : !expr(_e)));
}

void FSSA::popPathCondition() {
    solAssert(!m_pathConditions.empty(), "Cannot pop path condition, empty.");
    m_pathConditions.pop_back();
}

void FSSA::addStatement(const FSSA::Statement &&_statement) {
    m_interface->addAssertion(_statement.expr());
    m_statements.emplace_back(_statement);
}

std::string FSSA::to_smtlib() {
    return m_interface->to_string();
}


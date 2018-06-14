/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libsolidity/formal/SMTGas.h>

#ifdef HAVE_Z3

#include <libsolidity/formal/Z3Interface.h>

#elif HAVE_CVC4
#include <libsolidity/formal/CVC4Interface.h>
#else
#include <libsolidity/formal/SMTLib2Interface.h>
#endif

#include <libsolidity/formal/SSAVariable.h>
#include <libsolidity/formal/FSSA.h>
#include <libsolidity/formal/SymbolicIntVariable.h>
#include <libsolidity/formal/VariableUsage.h>

#include <libsolidity/interface/ErrorReporter.h>

#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace std;
using namespace dev;
using namespace dev::solidity;

SMTGas::SMTGas(ErrorReporter &_errorReporter, ReadCallback::Callback const &_readFileCallback,
               ScannerFromSourceNameFun _s) :
#ifdef HAVE_Z3
        m_interface(make_shared<smt::Z3Interface>()),
#elif HAVE_CVC4
m_interface(make_shared<smt::CVC4Interface>()),
#else
m_interface(make_shared<smt::SMTLib2Interface>(_readFileCallback)),
#endif
        m_errorReporter(_errorReporter),
        m_formatter(cout, _s) {
    (void) _readFileCallback;
}

void SMTGas::analyze(SourceUnit const &_source) {
    m_variableUsage = make_shared<VariableUsage>(_source);
    if (_source.annotation().experimentalFeatures.count(ExperimentalFeature::SMTGas))
        _source.accept(*this);
}

bool SMTGas::visit(ContractDefinition const &_contract) {
    m_currentContract = &_contract;
    return true;
}

void SMTGas::endVisit(ContractDefinition const &/*_contract*/) {
    m_currentContract = nullptr;
//    m_stateVariables.clear();
//    cout << "Dependencies for contract " << _contract.name() << ".";
//    for (auto &decl_set:m_dependencies) {
//        cout << endl << "  " << decl_set.first->name() << ": ";
//        for (auto &decl:decl_set.second) {
//            cout << decl->name() << " ";
//        }
//        cout << endl;
//    }
//    cout << "Expressions:" << endl;
//    for (auto &expr_smtexpr:m_expressions) {
//        auto e = expr_smtexpr.first;
//        auto smtexpr = expr_smtexpr.second;
//        cout << smtexpr.name << " in " << m_expression2function[e]->name() << endl;
//        m_formatter.printSourceLocation(&e->location());
//    }
    // TODO check for circular dep.
}

bool SMTGas::visit(FunctionDefinition const &_function) {
    if (!_function.modifiers().empty() || _function.isConstructor())
        m_errorReporter.warning(
                _function.location(),
                "Gas estimator does not yet support constructors and functions with modifiers."
        );
    m_currentFunction = &_function;
    m_fssa.emplace(&_function, FSSA(*m_currentContract, _function, m_errorReporter));

    return true;
}

void SMTGas::endVisit(FunctionDefinition const &) {
    cout << m_fssa.at(m_currentFunction).to_smtlib() << endl;
    m_currentFunction = nullptr;
}

void SMTGas::endVisit(VariableDeclaration const &_varDecl) {
    if (_varDecl.isLocalVariable() && _varDecl.type()->isValueType() && _varDecl.value())
        m_fssa.at(m_currentFunction).assignment(_varDecl, *_varDecl.value(), _varDecl.location());
}

//TODO
bool SMTGas::visit(IfStatement const &_node) {
    _node.condition().accept(*this);
    return false;

    auto countersEndTrue = visitBranch(_node.trueStatement(), expr(_node.condition()));
    vector<Declaration const *> touchedVariables = m_variableUsage->touchedVariables(_node.trueStatement());
    decltype(countersEndTrue) countersEndFalse;
    if (_node.falseStatement()) {
        countersEndFalse = visitBranch(*_node.falseStatement(), !expr(_node.condition()));
        touchedVariables += m_variableUsage->touchedVariables(*_node.falseStatement());
    } else {
        countersEndFalse = m_variables;
    }

    mergeVariables(touchedVariables, expr(_node.condition()), countersEndTrue, countersEndFalse);

    return false;
}

// TODO
bool SMTGas::visit(WhileStatement const &/*_node*/) {
    return false;
//    auto touchedVariables = m_variableUsage->touchedVariables(_node);
//    resetVariables(touchedVariables);
//    if (_node.isDoWhile()) {
//        visitBranch(_node.body());
//        // TODO the assertions generated in the body should still be active in the condition
//        _node.condition().accept(*this);
//    } else {
//        _node.condition().accept(*this);
//        visitBranch(_node.body(), expr(_node.condition()));
//    }
//    m_loopExecutionHappened = true;
//    resetVariables(touchedVariables);
//
//    return false;
}

// TODO
bool SMTGas::visit(ForStatement const &/*_node*/) {
    return false;
//    if (_node.initializationExpression())
//        _node.initializationExpression()->accept(*this);
//
//    // Do not reset the init expression part.
//    auto touchedVariables =
//            m_variableUsage->touchedVariables(_node.body());
//    if (_node.condition())
//        touchedVariables += m_variableUsage->touchedVariables(*_node.condition());
//    if (_node.loopExpression())
//        touchedVariables += m_variableUsage->touchedVariables(*_node.loopExpression());
//    // Remove duplicates
//    std::sort(touchedVariables.begin(), touchedVariables.end());
//    touchedVariables.erase(std::unique(touchedVariables.begin(), touchedVariables.end()), touchedVariables.end());
//
//    resetVariables(touchedVariables);
//
//    if (_node.condition()) {
//        _node.condition()->accept(*this);
//    }
//
//    VariableSequenceCounters sequenceCountersStart = m_variables;
//    m_interface->push();
//    if (_node.condition())
//        m_interface->addAssertion(expr(*_node.condition()));
//    _node.body().accept(*this);
//    if (_node.loopExpression())
//        _node.loopExpression()->accept(*this);
//
//    m_interface->pop();
//
//    m_loopExecutionHappened = true;
//    std::swap(sequenceCountersStart, m_variables);
//
//    resetVariables(touchedVariables);
//
//    return false;
}

void SMTGas::endVisit(VariableDeclarationStatement const &_varDecl) {
    if (_varDecl.declarations().size() != 1)
        m_errorReporter.warning(
                _varDecl.location(),
                "Assertion checker does not yet support such variable declarations."
        );
    else if (_varDecl.initialValue()) {
        m_fssa.at(m_currentFunction).assignment(*_varDecl.declarations()[0], *_varDecl.initialValue(),
                                                _varDecl.location());
    } else
        m_errorReporter.warning(
                _varDecl.location(),
                "Assertion checker does not yet implement such variable declarations."
        );
}

void SMTGas::endVisit(ExpressionStatement const &) {
}

void SMTGas::endVisit(Assignment const &_assignment) {
    if (_assignment.assignmentOperator() != Token::Value::Assign)
        m_errorReporter.warning(
                _assignment.location(),
                "Gas estimator does not yet implement compound assignment."
        );
    else if (!SSAVariable::isSupportedType(_assignment.annotation().type->category()))
        m_errorReporter.warning(
                _assignment.location(),
                "Gas estimator does not yet implement type " + _assignment.annotation().type->toString()
        );
    else if (auto identifier = dynamic_cast<Identifier const *>(&_assignment.leftHandSide())) {
        Declaration const *decl = identifier->annotation().referencedDeclaration;
        m_fssa.at(m_currentFunction).assignment(*decl, _assignment.rightHandSide(), _assignment.location());
        m_fssa.at(m_currentFunction).defineExpr(_assignment, _assignment.rightHandSide());
    } else
        m_errorReporter.warning(
                _assignment.location(),
                "Gas estimator does not yet implement such assignments."
        );
}

void SMTGas::endVisit(TupleExpression const &_tuple) {
    if (_tuple.isInlineArray() || _tuple.components().size() != 1)
        m_errorReporter.warning(
                _tuple.location(),
                "Gas estimator does not yet implement tuples and inline arrays."
        );
    else
        m_fssa.at(m_currentFunction).defineExpr(_tuple, *_tuple.components()[0]);
}

void SMTGas::endVisit(UnaryOperation const &_op) {
    m_fssa.at(m_currentFunction).createExpr(_op);
}

void SMTGas::endVisit(BinaryOperation const &_op) {
    m_fssa.at(m_currentFunction).createExpr(_op);
}

void SMTGas::endVisit(FunctionCall const &_funCall) {

    solAssert(_funCall.annotation().kind != FunctionCallKind::Unset, "");
    if (_funCall.annotation().kind != FunctionCallKind::FunctionCall) {
        m_errorReporter.warning(
                _funCall.location(),
                "Assertion checker does not yet implement this expression."
        );
        return;
    }

    FunctionType const &funType = dynamic_cast<FunctionType const &>(*_funCall.expression().annotation().type);

    std::vector<ASTPointer<Expression const>> const args = _funCall.arguments();
    if (funType.kind() == FunctionType::Kind::Assert) {
        solAssert(args.size() == 1, "");
        solAssert(args[0]->annotation().type->category() == Type::Category::Bool, "");
        checkCondition(!(expr(*args[0])), _funCall.location(), "Assertion violation");
        addPathImpliedExpression(expr(*args[0]));
    } else if (funType.kind() == FunctionType::Kind::Require) {
        solAssert(args.size() == 1, "");
        solAssert(args[0]->annotation().type->category() == Type::Category::Bool, "");
        addPathImpliedExpression(expr(*args[0]));
    } else {
        //const Declaration *decl = static_cast<Identifier const *>(&_funCall.expression())->annotation().referencedDeclaration;
        //m_dependencies[m_currentFunction].insert(decl);
        defineExpr(_funCall, 0);
    }
}

void SMTGas::endVisit(Identifier const &_identifier) {
    Declaration const *decl = _identifier.annotation().referencedDeclaration;
    if (_identifier.annotation().lValueRequested) {
        // Will be translated as part of the node that requested the lvalue.
    } else if (SSAVariable::isSupportedType(_identifier.annotation().type->category())) {
        m_fssa.at(m_currentFunction).defineExpr(_identifier, *decl);
    } else if (auto fun = dynamic_cast<FunctionType const *>(_identifier.annotation().type.get())) {
        if (fun->kind() == FunctionType::Kind::Assert || fun->kind() == FunctionType::Kind::Require)
            return;
    }
}

void SMTGas::endVisit(Literal const &_literal) {
    Type const &type = *_literal.annotation().type;
    if (type.category() == Type::Category::Integer || type.category() == Type::Category::RationalNumber) {
        if (auto rational = dynamic_cast<RationalNumberType const *>(&type))
            solAssert(!rational->isFractional(), "");
        m_fssa.at(m_currentFunction).defineExpr(_literal, type.literalValue(&_literal));
    } else if (type.category() == Type::Category::Bool) {
        m_fssa.at(m_currentFunction).defineExpr(_literal, _literal.token() == Token::TrueLiteral);
    } else
        m_errorReporter.warning(
                _literal.location(),
                "Gas estimator does not yet support the type of this literal (" +
                _literal.annotation().type->toString() +
                ")."
        );
}

void SMTGas::assignment(Declaration const &_variable, Expression const &_value, SourceLocation const &_location) {
    assignment(_variable, expr(_value), _location);
}

void SMTGas::assignment(Declaration const &_variable, smt::Expression const &_value, SourceLocation const &) {
    TypePointer type = _variable.type();
    m_interface->addAssertion(newValue(_variable) == _value);
}

SMTGas::VariableSequenceCounters SMTGas::visitBranch(Statement const &_statement, smt::Expression _condition) {
    return visitBranch(_statement, &_condition);
}

SMTGas::VariableSequenceCounters SMTGas::visitBranch(Statement const &_statement, smt::Expression const *_condition) {
    VariableSequenceCounters beforeVars = m_variables;

    if (_condition)
        pushPathCondition(*_condition);
    _statement.accept(*this);
    if (_condition)
        popPathCondition();

    std::swap(m_variables, beforeVars);

    return beforeVars;
}

void SMTGas::checkCondition(
        smt::Expression _condition,
        SourceLocation const &_location,
        string const &_description,
        string const &_additionalValueName,
        smt::Expression *_additionalValue
) {
    m_interface->push();
    addPathConjoinedExpression(_condition);

    vector<smt::Expression> expressionsToEvaluate;
    vector<string> expressionNames;
    if (m_currentFunction) {
        if (_additionalValue) {
            expressionsToEvaluate.emplace_back(*_additionalValue);
            expressionNames.push_back(_additionalValueName);
        }
        for (auto const &param: m_currentFunction->parameters())
            if (knownVariable(*param)) {
                expressionsToEvaluate.emplace_back(currentValue(*param));
                expressionNames.push_back(param->name());
            }
        for (auto const &var: m_currentFunction->localVariables())
            if (knownVariable(*var)) {
                expressionsToEvaluate.emplace_back(currentValue(*var));
                expressionNames.push_back(var->name());
            }
        for (auto const &var: m_stateVariables)
            if (knownVariable(*var.first)) {
                expressionsToEvaluate.emplace_back(currentValue(*var.first));
                expressionNames.push_back(var.first->name());
            }
    }
    smt::CheckResult result;
    vector<string> values;
    tie(result, values) = checkSatisfiableAndGenerateModel(expressionsToEvaluate);

    string loopComment;
    if (m_loopExecutionHappened)
        loopComment =
                "\nNote that some information is erased after the execution of loops.\n"
                        "You can re-introduce information using require().";
    switch (result) {
        case smt::CheckResult::SATISFIABLE: {
            std::ostringstream message;
            message << _description << " happens here";
            if (m_currentFunction) {
                message << " for:\n";
                solAssert(values.size() == expressionNames.size(), "");
                for (size_t i = 0; i < values.size(); ++i)
                    if (expressionsToEvaluate.at(i).name != values.at(i))
                        message << "  " << expressionNames.at(i) << " = " << values.at(i) << "\n";
            } else
                message << ".";
            m_errorReporter.warning(_location, message.str() + loopComment);
            break;
        }
        case smt::CheckResult::UNSATISFIABLE:
            break;
        case smt::CheckResult::UNKNOWN:
            m_errorReporter.warning(_location, _description + " might happen here." + loopComment);
            break;
        case smt::CheckResult::ERROR:
            m_errorReporter.warning(_location, "Error trying to invoke SMT solver.");
            break;
        default:
            solAssert(false, "");
    }
    m_interface->pop();
}

pair<smt::CheckResult, vector<string>>
SMTGas::checkSatisfiableAndGenerateModel(vector<smt::Expression> const &_expressionsToEvaluate) {
    smt::CheckResult result;
    vector<string> values;
    try {
        tie(result, values) = m_interface->check(_expressionsToEvaluate);
    }
    catch (smt::SolverError const &_e) {
        string description("Error querying SMT solver");
        if (_e.comment())
            description += ": " + *_e.comment();
        m_errorReporter.warning(description);
        result = smt::CheckResult::ERROR;
    }

    for (string &value: values) {
        try {
            // Parse and re-format nicely
            value = formatNumber(bigint(value));
        }
        catch (...) {
        }
    }

    return make_pair(result, values);
}

smt::CheckResult SMTGas::checkSatisfiable() {
    return checkSatisfiableAndGenerateModel({}).first;
}

void SMTGas::resetVariables(vector<Declaration const *> _variables) {
    for (auto const *decl: _variables) {
        newValue(*decl);
        setUnknownValue(*decl);
    }
}

void SMTGas::mergeVariables(vector<Declaration const *> const &_variables, smt::Expression const &_condition,
                            VariableSequenceCounters const &_countersEndTrue,
                            VariableSequenceCounters const &_countersEndFalse) {
    set<Declaration const *> uniqueVars(_variables.begin(), _variables.end());
    for (auto const *decl: uniqueVars) {
        int trueCounter = _countersEndTrue.at(decl).index();
        int falseCounter = _countersEndFalse.at(decl).index();
        solAssert(trueCounter != falseCounter, "");
        m_interface->addAssertion(newValue(*decl) == smt::Expression::ite(
                _condition,
                valueAtSequence(*decl, trueCounter),
                valueAtSequence(*decl, falseCounter))
        );
    }
}

bool SMTGas::createVariable(VariableDeclaration const &_varDecl) {
    if (SSAVariable::isSupportedType(_varDecl.type()->category())) {
        cout << _varDecl.name() << ":" << _varDecl.type()->toString() << " ";
        solAssert(m_variables.count(&_varDecl) == 0, "");
        solAssert(m_stateVariables.count(&_varDecl) == 0, "");
        if (_varDecl.isLocalVariable())
            m_variables.emplace(&_varDecl, SSAVariable(_varDecl, *m_interface));
        else {
            solAssert(_varDecl.isStateVariable(), "");
            m_stateVariables.emplace(&_varDecl, SSAVariable(_varDecl, *m_interface));
        }
        return true;
    } else {
        m_errorReporter.warning(
                _varDecl.location(),
                "Assertion checker does not yet support the type of this variable."
        );
        return false;
    }
}

string SMTGas::uniqueSymbol(Expression const &_expr) {
    return "expr_" + to_string(_expr.id());
}

bool SMTGas::knownVariable(Declaration const &_decl) {
    return m_variables.count(&_decl);
}

smt::Expression SMTGas::currentValue(Declaration const &_decl) {
    solAssert(knownVariable(_decl), "");
    return m_variables.at(&_decl)();
}

smt::Expression SMTGas::valueAtSequence(Declaration const &_decl, int _sequence) {
    solAssert(knownVariable(_decl), "");
    return m_variables.at(&_decl)(_sequence);
}

smt::Expression SMTGas::newValue(Declaration const &_decl) {
    solAssert(knownVariable(_decl), "");
    ++m_variables.at(&_decl);
    return m_variables.at(&_decl)();
}

void SMTGas::setZeroValue(Declaration const &_decl) {
    solAssert(knownVariable(_decl), "");
    m_variables.at(&_decl).setZeroValue();
}

void SMTGas::setUnknownValue(Declaration const &_decl) {
    solAssert(knownVariable(_decl), "");
    m_variables.at(&_decl).setUnknownValue();
}

smt::Expression SMTGas::expr(Expression const &_e) {
    if (!m_expressions.count(&_e)) {
        m_errorReporter.warning(_e.location(), "Internal error: Expression undefined for SMT solver.");
        createExpr(_e);
    }
    return m_expressions.at(&_e);
}

void SMTGas::createExpr(Expression const &_e) {
    if (m_expressions.count(&_e))
        m_errorReporter.warning(_e.location(), "Internal error: Expression created twice in SMT solver.");
    else {
        solAssert(_e.annotation().type, "");
        switch (_e.annotation().type->category()) {
            case Type::Category::RationalNumber: {
                if (RationalNumberType const *rational = dynamic_cast<RationalNumberType const *>(_e.annotation().type.get()))
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
        m_expression2function[&_e] = m_currentFunction;
    }
}

void SMTGas::defineExpr(Expression const &_e, smt::Expression _value) {
    createExpr(_e);
    m_interface->addAssertion(expr(_e) == _value);
}

void SMTGas::popPathCondition() {
    solAssert(m_pathConditions.size() > 0, "Cannot pop path condition, empty.");
    m_pathConditions.pop_back();
}

void SMTGas::pushPathCondition(smt::Expression const &_e) {
    m_pathConditions.push_back(currentPathConditions() && _e);
}

smt::Expression SMTGas::currentPathConditions() {
    if (m_pathConditions.size() == 0)
        return smt::Expression(true);
    return m_pathConditions.back();
}

void SMTGas::addPathConjoinedExpression(smt::Expression const &_e) {
    m_interface->addAssertion(currentPathConditions() && _e);
}

void SMTGas::addPathImpliedExpression(smt::Expression const &_e) {
    m_interface->addAssertion(smt::Expression::implies(currentPathConditions(), _e));
}

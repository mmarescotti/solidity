

#include <libsolidity/formal/SMTGas.h>

//#ifdef HAVE_Z3
//
//#include <libsolidity/formal/Z3Interface.h>
//
//#elif HAVE_CVC4
//#include <libsolidity/formal/CVC4Interface.h>
//#else
//#include <libsolidity/formal/SMTLib2Interface.h>
//#endif

#include <libsolidity/formal/FSSA.h>

#include <libsolidity/interface/ErrorReporter.h>

#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace std;
using namespace dev;
using namespace dev::solidity;

SMTGas::SMTGas(ErrorReporter &_errorReporter, ReadCallback::Callback const &_readFileCallback,
               ScannerFromSourceNameFun _s) :
//#ifdef HAVE_Z3
//        m_interface(make_shared<smt::Z3Interface>()),
//#elif HAVE_CVC4
//m_interface(make_shared<smt::CVC4Interface>()),
//#else
//m_interface(make_shared<smt::SMTLib2Interface>(_readFileCallback)),
//#endif
        m_errorReporter(_errorReporter),
        m_formatter(cout, _s) {
    (void) _readFileCallback;
}

void SMTGas::analyze(SourceUnit const &_source) {
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
    //cout << m_fssa.at(m_currentFunction).getFormula().sexpr().second << endl;
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

//    auto countersEndTrue = visitBranch(_node.trueStatement(), expr(_node.condition()));
//    vector<VariableDeclaration const*> touchedVariables = m_variableUsage->touchedVariables(_node.trueStatement());
//    decltype(countersEndTrue) countersEndFalse;
//    if (_node.falseStatement()) {
//        countersEndFalse = visitBranch(*_node.falseStatement(), !expr(_node.condition()));
//        touchedVariables += m_variableUsage->touchedVariables(*_node.falseStatement());
//    } else {
//        countersEndFalse = m_variables;
//    }
//
//    //mergeVariables(touchedVariables, expr(_node.condition()), countersEndTrue, countersEndFalse);
//
//    return false;
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
        m_errorReporter.fatalParserError(
                _assignment.location(),
                "Gas estimator does not yet implement compound assignment."
        );
    if (auto identifier = dynamic_cast<Identifier const *>(&_assignment.leftHandSide())) {
        Declaration const *decl = identifier->annotation().referencedDeclaration;
        try {
            m_fssa.at(m_currentFunction).assignment(*decl, _assignment.rightHandSide(), _assignment.location());
            m_fssa.at(m_currentFunction).defineExpr(_assignment, _assignment.rightHandSide());
        } catch (Exception &ex) {
            m_errorReporter.fatalParserError(
                    _assignment.location(),
                    ex.what()
            );
        }
    } else
        m_errorReporter.fatalParserError(
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

void SMTGas::endVisit(FunctionCall const &) {

//    solAssert(_funCall.annotation().kind != FunctionCallKind::Unset, "");
//    if (_funCall.annotation().kind != FunctionCallKind::FunctionCall) {
//        m_errorReporter.warning(
//                _funCall.location(),
//                "Assertion checker does not yet implement this expression."
//        );
//        return;
//    }
//
//    FunctionType const &funType = dynamic_cast<FunctionType const &>(*_funCall.expression().annotation().type);
//
//    std::vector<ASTPointer<Expression const>> const args = _funCall.arguments();
//    if (funType.kind() == FunctionType::Kind::Assert) {
//        solAssert(args.size() == 1, "");
//        solAssert(args[0]->annotation().type->category() == Type::Category::Bool, "");
//        checkCondition(!(expr(*args[0])), _funCall.location(), "Assertion violation");
//        addPathImpliedExpression(expr(*args[0]));
//    } else if (funType.kind() == FunctionType::Kind::Require) {
//        solAssert(args.size() == 1, "");
//        solAssert(args[0]->annotation().type->category() == Type::Category::Bool, "");
//        addPathImpliedExpression(expr(*args[0]));
//    } else {
//        //const Declaration *decl = static_cast<Identifier const *>(&_funCall.expression())->annotation().referencedDeclaration;
//        //m_dependencies[m_currentFunction].insert(decl);
//        defineExpr(_funCall, 0);
//    }
}

void SMTGas::endVisit(Identifier const &_identifier) {
    Declaration const *decl = _identifier.annotation().referencedDeclaration;
    if (_identifier.annotation().lValueRequested) {
        // Will be translated as part of the node that requested the lvalue.
    } else {
        try {
            m_fssa.at(m_currentFunction).defineExpr(_identifier, *decl);
        } catch (Exception &ex) {
            if (auto fun = dynamic_cast<FunctionType const *>(_identifier.annotation().type.get())) {
                if (fun->kind() == FunctionType::Kind::Assert || fun->kind() == FunctionType::Kind::Require)
                    return;
            }
        }
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



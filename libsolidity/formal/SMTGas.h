#pragma once


#include <libsolidity/formal/FSSA.h>

#include <libsolidity/ast/ASTVisitor.h>

#include <libsolidity/interface/ReadFile.h>

#include <libsolidity/interface/SourceReferenceFormatter.h>

#include <map>
#include <string>
#include <vector>

namespace dev
{
namespace solidity
{

class ErrorReporter;

class SMTGas: private ASTConstVisitor
{
public:
	using ScannerFromSourceNameFun = std::function<Scanner const&(std::string const&)>;
	SMTGas(ErrorReporter& _errorReporter, ReadCallback::Callback const& _readCallback, ScannerFromSourceNameFun _s);

	void analyze(SourceUnit const& _sources);

private:
	// TODO: Check that we do not have concurrent reads and writes to a variable,
	// because the order of expression evaluation is undefined
	// TODO: or just force a certain order, but people might have a different idea about that.

	bool visit(ContractDefinition const& _node) override;
	void endVisit(ContractDefinition const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	void endVisit(FunctionDefinition const& _node) override;
	void endVisit(VariableDeclaration const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	void endVisit(VariableDeclarationStatement const& _node) override;
	void endVisit(ExpressionStatement const& _node) override;
	void endVisit(Assignment const& _node) override;
	void endVisit(TupleExpression const& _node) override;
	void endVisit(UnaryOperation const& _node) override;
	void endVisit(BinaryOperation const& _node) override;
	void endVisit(FunctionCall const& _node) override;
	void endVisit(Identifier const& _node) override;
	void endVisit(Literal const& _node) override;

	ErrorReporter& m_errorReporter;

	FunctionDefinition const* m_currentFunction = nullptr;
	ContractDefinition const* m_currentContract = nullptr;
	SourceReferenceFormatter m_formatter;
	std::map<FunctionDefinition const*, std::set<FunctionDefinition const*>> m_dependencies;
	std::map<FunctionDefinition const*, FSSA> m_fssa;

};

}
}

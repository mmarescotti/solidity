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

#pragma once

#include <libsolidity/interface/Exceptions.h>
#include <libsolidity/interface/ReadFile.h>

#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>

#include <boost/noncopyable.hpp>

#include <map>
#include <string>
#include <vector>
#include <cstdio>

namespace dev
{
namespace solidity
{
namespace smt
{

enum class CheckResult
{
	SATISFIABLE, UNSATISFIABLE, UNKNOWN, ERROR
};

enum class Sort
{
	Int,
	Bool,
	IntIntFun, // Function of one Int returning a single Int
	IntBoolFun // Function of one Int returning a single Bool
};

/// C++ representation of an SMTLIB2 expression.
class Expression
{
	friend class SolverInterface;
public:
	explicit Expression(bool _v): name(_v ? "true" : "false"), sort(Sort::Bool) {}
	Expression(size_t _number): name(std::to_string(_number)), sort(Sort::Int) {}
	Expression(u256 const& _number): name(_number.str()), sort(Sort::Int) {}
	Expression(bigint const& _number): name(_number.str()), sort(Sort::Int) {}

	Expression(Expression const&) = default;
	Expression(Expression&&) = default;
	Expression& operator=(Expression const&) = default;
	Expression& operator=(Expression&&) = default;

	bool hasCorrectArity() const
	{
		static std::map<std::string, unsigned> const operatorsArity{
			{"ite", 3},
			{"not", 1},
			{"and", 2},
			{"or", 2},
			{"=", 2},
			{"<", 2},
			{"<=", 2},
			{">", 2},
			{">=", 2},
			{"+", 2},
			{"-", 2},
			{"*", 2},
			{"/", 2}
		};
		return operatorsArity.count(name) && operatorsArity.at(name) == arguments.size();
	}

	static Expression const ite(Expression const &_condition,
								Expression const &_trueValue,
								Expression const &_falseValue)
	{
		return Expression("ite", std::vector<Expression>{
			_condition, _trueValue, _falseValue
		}, _trueValue.sort);
	}

	static Expression const implies(Expression const &_a, Expression const &_b)
	{
		return !_a || _b;
	}

	friend Expression const operator!(Expression const &_a)
	{
		return Expression("not", _a, Sort::Bool);
	}
	friend Expression const operator&&(Expression const &_a, Expression const &_b)
	{
		return Expression("and", _a, _b, Sort::Bool);
	}
	friend Expression const operator||(Expression const &_a, Expression const &_b)
	{
		return Expression("or", _a, _b, Sort::Bool);
	}
	friend Expression const operator==(Expression const &_a, Expression const &_b)
	{
		return Expression("=", _a, _b, Sort::Bool);
	}
	friend Expression const operator!=(Expression const &_a, Expression const &_b)
	{
		return !(_a == _b);
	}
	friend Expression const operator<(Expression const &_a, Expression const &_b)
	{
		return Expression("<", _a, _b, Sort::Bool);
	}
	friend Expression const operator<=(Expression const &_a, Expression const &_b)
	{
		return Expression("<=", _a, _b, Sort::Bool);
	}
	friend Expression const operator>(Expression const &_a, Expression const &_b)
	{
		return Expression(">", _a, _b, Sort::Bool);
	}
	friend Expression const operator>=(Expression const &_a, Expression const &_b)
	{
		return Expression(">=", _a, _b, Sort::Bool);
	}
	friend Expression const operator+(Expression const &_a, Expression const &_b)
	{
		return Expression("+", _a, _b, Sort::Int);
	}
	friend Expression const operator-(Expression const &_a, Expression const &_b)
	{
		return Expression("-", _a, _b, Sort::Int);
	}
	friend Expression const operator*(Expression const &_a, Expression const &_b)
	{
		return Expression("*", _a, _b, Sort::Int);
	}
	friend Expression const operator/(Expression const &_a, Expression const &_b)
	{
		return Expression("/", _a, _b, Sort::Int);
	}
	Expression const operator()(Expression const &_a) const
	{
		solAssert(
			arguments.empty(),
			"Attempted function application to non-function."
		);
		switch (sort)
		{
		case Sort::IntIntFun:
			return Expression(name, _a, Sort::Int);
		case Sort::IntBoolFun:
			return Expression(name, _a, Sort::Bool);
		default:
			solAssert(
				false,
				"Attempted function application to invalid type."
			);
			break;
		}
	}

	std::string const name;
	std::vector<Expression> const arguments;
	Sort const sort;

private:
	/// Manual constructor, should only be used by SolverInterface and this class itself.
	Expression(std::string const &_name, std::vector<Expression> const &_arguments, Sort _sort):
		name(_name), arguments(_arguments), sort(_sort) {}

	explicit Expression(std::string const &_name, Sort _sort):
		Expression(_name, std::vector<Expression>{}, _sort) {}
	Expression(std::string const &_name, Expression const &_arg, Sort _sort):
		Expression(_name, std::vector<Expression>{_arg}, _sort) {}
	Expression(std::string const &_name, Expression const &_arg1, Expression const &_arg2, Sort _sort):
		Expression(_name, {_arg1, _arg2}, _sort) {}
};

DEV_SIMPLE_EXCEPTION(SolverError);

class SolverInterface
{
public:
	SolverInterface()= default;
//	SolverInterface(SolverInterface const &_s){
//        m_declarations = _s.m_declarations;
//        m_assertions = _s.m_assertions;
//    }
	virtual ~SolverInterface() = default;
	virtual void reset() = 0;

	virtual void push() = 0;
	virtual void pop() = 0;

	virtual std::string to_string() = 0;

    virtual Expression const newFunction(std::string _name, Sort _domain, Sort _codomain)
	{
		solAssert(_domain == Sort::Int, "Function sort not supported.");
		// Subclasses should do something here
		switch (_codomain)
		{
		case Sort::Int:
			m_declarations.emplace_back(Expression(std::move(_name), {}, Sort::IntIntFun));
			break;
		case Sort::Bool:
			m_declarations.emplace_back(Expression(std::move(_name), {}, Sort::IntBoolFun));
			break;
		default:
			solAssert(false, "Function sort not supported.");
			break;
		}
        return m_declarations.back();
	}
    virtual Expression const newInteger(std::string _name)
	{
		// Subclasses should do something here
		m_declarations.emplace_back(Expression(std::move(_name), {}, Sort::Int));
		return m_declarations.back();
	}
    virtual Expression const newBool(std::string _name)
	{
		// Subclasses should do something here
		m_declarations.emplace_back(Expression(std::move(_name), {}, Sort::Bool));
		return m_declarations.back();
	}

    virtual void addAssertion(Expression const& _expr){
		m_assertions.emplace_back(_expr);
	}

	/// Checks for satisfiability, evaluates the expressions if a model
	/// is available. Throws SMTSolverError on error.
	virtual std::pair<CheckResult, std::vector<std::string>>
	check(std::vector<Expression> const& _expressionsToEvaluate) = 0;

protected:
	std::vector<Expression> m_declarations;
	std::vector<Expression> m_assertions;
};


}
}
}

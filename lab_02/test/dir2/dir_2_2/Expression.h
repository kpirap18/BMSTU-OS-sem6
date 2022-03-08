#pragma once
#include <string>
#include "Row.h"

enum class Type {
	INT, LONG, FLOAT, BOOL, STR, NONE,
	CINT, CLONG, CFLOAT, CBOOL, CSTR,
	UNARY_SIGN, ADD, SUB, MUL, DIV,
	AND, OR, NOT, IS,
	EQ, NE, LT, GT, LE, GE,
};


class BaseExpression {
public:
	Type type;

	BaseExpression(Type type);
	virtual BaseExpression* convolution(Row* row);
	virtual BaseExpression* to_int();
	virtual BaseExpression* to_bool();
};

class PrimaryValue : public BaseExpression {
public:
	PrimaryValue(Type type);
};

class Int : public PrimaryValue {
public:
	int value;

	Int(int value);
	virtual BaseExpression* to_bool();
};

class Long : public PrimaryValue {
public:
	long value;

	Long(long value);
	virtual BaseExpression* to_bool();
};

class Float : public PrimaryValue {
public:
	float value;

	Float(float value);
	virtual BaseExpression* to_bool();
};

class Bool : public PrimaryValue {
public:
	bool value;

	Bool(bool value);
	virtual BaseExpression* to_int();
};

class String : public PrimaryValue {
public:
	std::string value;

	String(std::string value);
	virtual BaseExpression* to_int();
	virtual BaseExpression* to_bool();
};

class Column : public PrimaryValue {
public:
	std::string name;
	int index;

	Column(Type type, std::string name, int index);
	virtual BaseExpression* convolution(Row* row);
};

class None : public PrimaryValue {
public:
	None();
};

class NumericExpression : public BaseExpression {
public:
	NumericExpression(Type type);
};

class UnarySign : public NumericExpression {
public:
	UnarySign(BaseExpression* value, char sign);
	virtual BaseExpression* convolution(Row* row);

protected:
	bool is_minus;
	BaseExpression* value;
};

class DoubleNumericExpression : public NumericExpression {
public:
	DoubleNumericExpression(Type type, BaseExpression* left, BaseExpression* right);

protected:
	BaseExpression* left;
	BaseExpression* right;
};

class Add : public DoubleNumericExpression {
public:
	Add(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Sub : public DoubleNumericExpression {
public:
	Sub(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Mul : public DoubleNumericExpression {
public:
	Mul(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Div : public DoubleNumericExpression {
public:
	Div(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class BooleanExpression : public BaseExpression {
public:
	BooleanExpression(Type type);
};

class Not : BooleanExpression {
public:
	Not(BaseExpression* value);
	virtual BaseExpression* convolution(Row* row);

protected:
	BaseExpression* value;
};

class DoubleBooleanExpression : public BooleanExpression {
public:
	DoubleBooleanExpression(Type type, BaseExpression* left, BaseExpression* right);

protected:
	BaseExpression* left;
	BaseExpression* right;
};

class And : public DoubleBooleanExpression {
public:
	And(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Or : public DoubleBooleanExpression {
public:
	Or(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Is : public BooleanExpression {
public:
	Is(BaseExpression* left, int right);  // -1: None, 0: False, 1: True
	virtual BaseExpression* convolution(Row* row);

protected:
	BaseExpression* left;
	int right;
};

class Eq : public DoubleBooleanExpression {
public:
	Eq(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Ne : public DoubleBooleanExpression {
public:
	Ne(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Lt : public DoubleBooleanExpression {
public:
	Lt(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Gt : public DoubleBooleanExpression {
public:
	Gt(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Le : public DoubleBooleanExpression {
public:
	Le(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

class Ge : public DoubleBooleanExpression {
public:
	Ge(BaseExpression* left, BaseExpression* right);
	virtual BaseExpression* convolution(Row* row);
};

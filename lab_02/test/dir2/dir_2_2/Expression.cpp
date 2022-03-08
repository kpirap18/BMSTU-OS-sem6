#include "Expression.h"
#include <iostream>

BaseExpression::BaseExpression(Type type) : type(type) {}

BaseExpression* BaseExpression::convolution(Row* row)
{
	return this;
}

BaseExpression* BaseExpression::to_int()
{
	return this;
}

BaseExpression* BaseExpression::to_bool()
{
	return this;
}

Int::Int(int value) : PrimaryValue(Type::INT), value(value) {}

BaseExpression* Int::to_bool()
{
	return new Bool((bool)this->value);
}

Long::Long(long value) : PrimaryValue(Type::LONG), value(value) {}

BaseExpression* Long::to_bool()
{
	return new Bool((bool)this->value);
}

Float::Float(float value) : PrimaryValue(Type::FLOAT), value(value) {}

BaseExpression* Float::to_bool()
{
	return new Bool((bool)this->value);
}

Bool::Bool(bool value) : PrimaryValue(Type::BOOL), value(value) {}

BaseExpression* Bool::to_int()
{
	return new Int((int)this->value);
}

String::String(std::string value) : PrimaryValue(Type::STR), value(value) {}

BaseExpression* String::to_int()
{
	throw 1;
}

BaseExpression* String::to_bool()
{
	throw 1;
}

Column::Column(Type type, std::string name, int index) : PrimaryValue(type), name(name), index(index) {}

BaseExpression* Column::convolution(Row* row)
{
	void* ptr = row->get(this->index);
	if (!ptr) {
		return new None();
	}
	switch (this->type)
	{
	case Type::CINT:
		return new Int(*static_cast<int*>(ptr));
	case Type::CLONG:
		return new Long(*static_cast<long*>(ptr));
	case Type::CFLOAT:
		return new Float(*static_cast<float*>(ptr));
	case Type::CBOOL:
		return new Bool(*static_cast<bool*>(ptr));
	case Type::CSTR:
		return new String(*static_cast<std::string*>(ptr));
	default:
		break;
	};
	throw 2;
}

UnarySign::UnarySign(BaseExpression* value, char sign) : NumericExpression(Type::UNARY_SIGN), value(value) {
	this->is_minus = sign == '-';
}

BaseExpression* UnarySign::convolution(Row* row)
{
	BaseExpression* new_value = this->value->convolution(row)->to_int();
	int sign = (this->is_minus) ? -1 : 1;
	switch (new_value->type)
	{
	case Type::INT:
		return new Int(sign * dynamic_cast<Int*>(new_value)->value);
	case Type::LONG:
		return new Long(sign * dynamic_cast<Long*>(new_value)->value);
	case Type::FLOAT:
		return new Float(sign * dynamic_cast<Float*>(new_value)->value);
	case Type::NONE:
		return new None();
	default:
		break;
	}
	throw 3;
}

DoubleNumericExpression::DoubleNumericExpression(Type type, BaseExpression* left, BaseExpression* right) : NumericExpression(type), left(left), right(right) {}

Add::Add(BaseExpression* left, BaseExpression* right) : DoubleNumericExpression(Type::ADD, left, right) {}

BaseExpression* Add::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value + dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value + dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value + dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value + dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value + dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value + dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value + dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value + dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value + dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 4;
}

Sub::Sub(BaseExpression* left, BaseExpression* right) : DoubleNumericExpression(Type::SUB, left, right) {}

BaseExpression* Sub::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value - dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value - dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value - dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value - dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value - dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value - dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value - dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value - dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value - dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 4;
}

Mul::Mul(BaseExpression* left, BaseExpression* right) : DoubleNumericExpression(Type::MUL, left, right) {}

BaseExpression* Mul::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value * dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value * dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value * dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value * dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value * dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value * dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value * dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value * dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value * dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 4;
}

Div::Div(BaseExpression* left, BaseExpression* right) : DoubleNumericExpression(Type::DIV, left, right) {}

BaseExpression* Div::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_right->type == Type::INT) {
		int right_value = dynamic_cast<Int*>(new_left)->value;
		if (right_value == 0) {
			return new None();
		}
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(dynamic_cast<Int*>(new_right)->value / right_value);
		case Type::LONG:
			return new Long(dynamic_cast<Long*>(new_right)->value / right_value);
		case Type::FLOAT:
			return new Float(dynamic_cast<Float*>(new_right)->value / right_value);
		default:
			break;
		}

	}
	else if (new_right->type == Type::LONG) {
		long right_value = dynamic_cast<Long*>(new_left)->value;
		if (right_value == 0) {
			return new None();
		}
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(dynamic_cast<Long*>(new_right)->value / right_value);
		case Type::LONG:
			return new Long(dynamic_cast<Long*>(new_right)->value / right_value);
		case Type::FLOAT:
			return new Float(dynamic_cast<Float*>(new_right)->value / right_value);
		default:
			break;
		}
	}
	else if (new_right->type == Type::FLOAT) {
		float right_value = dynamic_cast<Float*>(new_left)->value;
		if (right_value == 0) {
			return new None();
		}
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(dynamic_cast<Float*>(new_right)->value / right_value);
		case Type::LONG:
			return new Float(dynamic_cast<Float*>(new_right)->value / right_value);
		case Type::FLOAT:
			return new Float(dynamic_cast<Float*>(new_right)->value / right_value);
		default:
			break;
		}
	}
	throw 4;
}


Not::Not(BaseExpression* value) : BooleanExpression(Type::NOT), value(value) {}

BaseExpression* Not::convolution(Row* row)
{
	BaseExpression* new_value = this->value->convolution(row)->to_bool();
	if (value->type == Type::NONE) {
		return new None();
	}
	if (value->type == Type::BOOL) {
		return new Bool(!dynamic_cast<Bool*>(new_value)->value);
	}
	throw 5;
}

DoubleBooleanExpression::DoubleBooleanExpression(Type type, BaseExpression* left, BaseExpression* right) : BooleanExpression(type), left(left), right(right) {}

And::And(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::AND, left, right) {}

BaseExpression* And::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_bool();
	BaseExpression* new_right = this->right->convolution(row)->to_bool();
	if (new_left->type == Type::BOOL && new_right->type == Type::BOOL) {
		return new Bool(dynamic_cast<Bool*>(new_left)->value && dynamic_cast<Bool*>(new_left)->value);
	}
	if (new_left->type == Type::NONE && new_right->type == Type::NONE) {
		return new None();
	}
	if (new_left->type == Type::BOOL) {
		if (dynamic_cast<Bool*>(new_left)->value)
			return new None();
		return new Bool(false);
	}
	if (new_right->type == Type::BOOL) {
		if (dynamic_cast<Bool*>(new_right)->value)
			return new None();
		return new Bool(false);
	}
	throw 6;
}

Or::Or(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::OR, left, right) {}

BaseExpression* Or::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_bool();
	BaseExpression* new_right = this->right->convolution(row)->to_bool();
	if (new_left->type == Type::BOOL && new_right->type == Type::BOOL) {
		return new Bool(dynamic_cast<Bool*>(new_left)->value || dynamic_cast<Bool*>(new_left)->value);
	}
	if (new_left->type == Type::NONE && new_right->type == Type::NONE) {
		return new None();
	}
	if (new_left->type == Type::BOOL) {
		if (dynamic_cast<Bool*>(new_left)->value)
			return new Bool(true);
		return new None();
	}
	if (new_right->type == Type::BOOL) {
		if (dynamic_cast<Bool*>(new_right)->value)
			return new Bool(true);
		return new None();
	}
	throw 6;
}

Is::Is(BaseExpression* left, int right) : BooleanExpression(Type::IS), left(left), right(right) {}

BaseExpression* Is::convolution(Row* row)
{
	BaseExpression* new_left = this->left->convolution(row)->to_bool();
	if (new_left->type == Type::BOOL) {
		return new Bool(dynamic_cast<Bool*>(new_left)->value == this->right && this->right != -1);
	}
	if (new_left->type == Type::NONE) {
		return new Bool(this->right == -1);
	}
	throw 7;
}

Eq::Eq(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::EQ, left, right) {}

BaseExpression* Eq::convolution(Row* row)
BaseExpression* Eq::convolution(Row* row)
{
	/* Приводим к числу, так как пока не сравниваем строки) */
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value == dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value == dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value == dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value == dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value == dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value == dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value == dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value == dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value == dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 8;
}

Ne::Ne(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::NE, left, right) {}

BaseExpression* Ne::convolution(Row* row)
{
	/* Приводим к числу, так как пока не сравниваем строки) */
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value != dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value != dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value != dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value != dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value != dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value != dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value != dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value != dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value != dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 8;
}

Lt::Lt(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::LT, left, right) {}

BaseExpression* Lt::convolution(Row* row)
{
	/* Приводим к числу, так как пока не сравниваем строки) */
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value < dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value < dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value < dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value < dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value < dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value < dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value < dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value < dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value < dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 8;
}

Gt::Gt(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::GT, left, right) {}

BaseExpression* Gt::convolution(Row* row)
{
	/* Приводим к числу, так как пока не сравниваем строки) */
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value > dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value > dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value > dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value > dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value > dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value > dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value > dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value > dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value > dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 8;
}

Le::Le(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::LE, left, right) {}

BaseExpression* Le::convolution(Row* row)
{
	/* Приводим к числу, так как пока не сравниваем строки) */
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value <= dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value <= dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value <= dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value <= dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value <= dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value <= dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value <= dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value <= dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value <= dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 8;
}

Ge::Ge(BaseExpression* left, BaseExpression* right) : DoubleBooleanExpression(Type::GE, left, right) {}

BaseExpression* Ge::convolution(Row* row)
{
	/* Приводим к числу, так как пока не сравниваем строки) */
	BaseExpression* new_left = this->left->convolution(row)->to_int();
	BaseExpression* new_right = this->right->convolution(row)->to_int();
	if (new_left->type == Type::NONE || new_right->type == Type::NONE) {
		return new None();
	}

	if (new_left->type == Type::INT) {
		int left_value = dynamic_cast<Int*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Int(left_value >= dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value >= dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value >= dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}

	}
	else if (new_left->type == Type::LONG) {
		long left_value = dynamic_cast<Long*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Long(left_value >= dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Long(left_value >= dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value >= dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	else if (new_left->type == Type::FLOAT) {
		float left_value = dynamic_cast<Float*>(new_left)->value;
		switch (new_right->type)
		{
		case Type::INT:
			return new Float(left_value >= dynamic_cast<Int*>(new_right)->value);
		case Type::LONG:
			return new Float(left_value >= dynamic_cast<Long*>(new_right)->value);
		case Type::FLOAT:
			return new Float(left_value >= dynamic_cast<Float*>(new_right)->value);
		default:
			break;
		}
	}
	throw 8;
}

None::None() : PrimaryValue(Type::NONE) {}

PrimaryValue::PrimaryValue(Type type) : BaseExpression(type) {}

NumericExpression::NumericExpression(Type type) : BaseExpression(type) {}

BooleanExpression::BooleanExpression(Type type) : BaseExpression(type) {}

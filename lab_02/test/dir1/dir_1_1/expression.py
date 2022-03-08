import logging
from datetime import date, datetime
from typing import Union, List

import pypika as pk

from . import mixins as mx
from . import symbols as ss
from . import utils
from .exceptions import UnreachableException
from functools import reduce


class BaseExpression(mx.AsMixin):
    @property
    def convolution(self):
        """
        Свертка - упрощение выражения
        """
        return self

    @property
    def to_int(self):
        """
        Приведение к числу
        """
        return self

    @property
    def to_bool(self):
        """
        Приведение к True/False
        """
        return self

    def pika(self):
        """
        Для генерации SELECT
        """
        raise NotImplementedError()

    def __init__(self):
        super().__init__()
        self.is_base = False

    def set_base(self, value=True):
        self.is_base = value


class PrimaryValue(BaseExpression):
    INT = 0
    FLOAT = 1
    STR = 2
    DATE = 3
    DATETIME = 4
    BOOL = 5
    NULL = 6
    COLUMN = 7

    KIND = None

    @classmethod
    def auto(cls, value):
        if isinstance(value, int):
            return Int(value)
        elif isinstance(value, float):
            return Float(value)
        elif isinstance(value, str):
            return Str(value)
        elif isinstance(value, datetime):
            return Datetime(value)
        elif isinstance(value, date):
            return Date(value)
        elif isinstance(value, bool):
            return Bool(value)
        elif value is None:
            return Null()
        elif isinstance(value, utils.NamingChain):
            return Column(value)
        raise ValueError('Invalid data type: {}({})'.format(type(value), value))

    def __init__(self, value):
        super().__init__()
        self.value = value

    def pika(self):
        return self.value

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.KIND == other.KIND and self.value == other.value

    def __repr__(self):
        return '{}({})'.format(self.__class__.__name__, self.value)


class PrimaryNumeric(PrimaryValue):
    @property
    def to_bool(self):
        return Bool(bool(self.value))


class Int(PrimaryNumeric):
    KIND = PrimaryValue.INT


class Float(PrimaryNumeric):
    KIND = PrimaryValue.FLOAT


class Str(PrimaryValue):
    # Todo: Does not work
    KIND = PrimaryValue.STR


class Date(PrimaryValue):
    # Todo: Does not work
    KIND = PrimaryValue.DATE


class Datetime(PrimaryValue):
    # Todo: Does not work
    KIND = PrimaryValue.DATETIME


class Bool(PrimaryValue):
    KIND = PrimaryValue.BOOL

    @property
    def to_int(self):
        return Int(int(self.value))


class Null(PrimaryValue):
    KIND = PrimaryValue.NULL

    def __init__(self, value=None):
        super().__init__(value)


class Column(PrimaryValue):
    """
    Используется как заглушка на момент парсинга
    Потом заменяется на объект Column из structures
    """
    KIND = PrimaryValue.COLUMN

    def pika(self):
        return pk.Field(self.value.get_data()[-1])


class SimpleExpression(BaseExpression):
    logger = logging.getLogger('simple_expression')

    def __init__(self, expr):
        super().__init__()
        self.expr = expr

    def pika(self):
        return self.expr.pika()


class NumericExpression(BaseExpression):
    logger = logging.getLogger('numeric_expression')

    def pika(self):
        raise NotImplementedError()


class UnarySign(NumericExpression):
    def __init__(self, value: BaseExpression, sign):
        assert sign in (ss.minus_sign, ss.plus_sign)
        super().__init__()
        self.value = value
        self.is_minus = sign == ss.minus_sign
        self.sign = -1 if self.is_minus else 1

    @property
    def convolution(self):
        self.value = self.value.convolution.to_int
        if isinstance(self.value, Int):
            return Int(self.value.value * self.sign)
        elif isinstance(self.value, Float):
            return Float(-self.value.value * self.sign)
        elif isinstance(self.value, UnarySign):
            check = self.is_minus ^ self.value.is_minus
            sign = ss.minus_sign if check else ss.plus_sign
            return UnarySign(self.value.value, sign)
        return self

    def pika(self):
        value = self.value.pika()
        return value if self.sign == 1 else -value

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.sign == other.sign and self.value == other.value

    def __repr__(self):
        return '{}CastToInt({!r})'.format('-' if self.is_minus else '', self.value)


class DoubleNumericExpression(NumericExpression):
    ADD = '+'
    SUB = '-'
    MUL = '*'
    DIV = '/'
    op = None

    def __init__(self, left: BaseExpression, right: BaseExpression):
        super().__init__()
        self.left = left
        self.right = right

    def action(self, a, b):
        raise NotImplementedError

    def special_rules(self):
        return self

    @property
    def convolution(self):
        self.left = self.left.convolution.to_int
        self.right = self.right.convolution.to_int

        # Если два числа то производим операцию над ними
        if isinstance(self.left, PrimaryNumeric) and isinstance(self.right, PrimaryNumeric):
            value = self.action(self.left.value, self.right.value)
            if isinstance(self.left, Float) or isinstance(self.right, Float):
                return Float(value)
            return Int(value)

        # Если одно Null то все Null
        if isinstance(self.left, Null) or isinstance(self.right, Null):
            return Null()

        return self.special_rules()

    def pika(self):
        raise NotImplementedError()

    def __eq__(self, other):
        return (isinstance(other, self.__class__)
                and self.op == other.op
                and self.left == other.left
                and self.right == other.right)

    def __repr__(self):
        return '({!r} {} {!r})'.format(self.left, self.op, self.right)


class Add(DoubleNumericExpression):
    op = DoubleNumericExpression.ADD

    def action(self, a, b):
        return a + b

    def special_rules(self):
        if isinstance(self.left, PrimaryNumeric) and self.left.value == 0:
            return self.right
        if isinstance(self.right, PrimaryNumeric) and self.right.value == 0:
            return self.left
        return self

    def pika(self):
        return self.left.pika() + self.right.pika()


class Sub(DoubleNumericExpression):
    op = DoubleNumericExpression.SUB

    def action(self, a, b):
        return a - b

    def special_rules(self):
        if isinstance(self.left, PrimaryNumeric) and self.left.value == 0:
            return UnarySign(self.right, ss.minus_sign)
        if isinstance(self.right, PrimaryNumeric) and self.right.value == 0:
            return self.left
        return self

    def pika(self):
        return self.left.pika() - self.right.pika()


class Mul(DoubleNumericExpression):
    op = DoubleNumericExpression.MUL

    def action(self, a, b):
        return a * b

    def special_rules(self):
        if (
            isinstance(self.left, PrimaryNumeric) and self.left.value == 0 or
            isinstance(self.right, PrimaryNumeric) and self.right.value == 0
        ):
            return Int(0)
        return self

    def pika(self):
        return self.left.pika() * self.right.pika()


class Div(DoubleNumericExpression):
    op = DoubleNumericExpression.DIV

    def action(self, a, b):
        return a / b

    def special_rules(self):
        if isinstance(self.right, PrimaryNumeric) and self.right.value == 0:
            return Null()
        if isinstance(self.left, PrimaryNumeric) and self.left.value == 0:
            return Int(0)
        if isinstance(self.right, PrimaryNumeric) and self.right.value == 1:
            return self.left
        return self

    @property
    def convolution(self):
        self.left = self.left.convolution.to_int
        self.right = self.right.convolution.to_int

        # Если два числа то производим операцию над ними
        if isinstance(self.left, PrimaryNumeric) and isinstance(self.right, PrimaryNumeric):
            # Деление на 0 = Null
            if self.right.value == 0:
                return Null()
            value = self.action(self.left.value, self.right.value)
            if isinstance(self.left, Float) or isinstance(self.right, Float):
                return Float(value)
            return Int(value)

        # Если одно Null то все Null
        if isinstance(self.left, Null) or isinstance(self.right, Null):
            return Null()

        return self.special_rules()

    def pika(self):
        return self.left.pika() / self.right.pika()


class BooleanExpression(BaseExpression):
    logger = logging.getLogger('boolean_expression')

    def calculate(self, vector):
        raise NotImplementedError()

    @staticmethod
    def _get_value_for_calculate(value, vector):
        if value.is_base:
            value = vector.pop()
        if isinstance(value, BooleanExpression):
            value = value.calculate(vector)
        elif isinstance(value, Bool):
            value = value.value
        elif isinstance(value, Null):
            value = None
        elif isinstance(value, bool) or value is None:
            pass
        else:
            raise UnreachableException()
        return value

    def pika(self):
        raise NotImplementedError()


class Not(BooleanExpression):

    def __init__(self, value: BaseExpression):
        super().__init__()
        self.value = value

    @property
    def convolution(self):
        self.value = self.value.convolution.to_bool
        if isinstance(self.value, ComparisonPredicate):
            return self.value.not_()
        elif isinstance(self.value, Bool):
            return Bool(not self.value.value)
        elif isinstance(self.value, Null):
            return Null()
        return self

    def calculate(self, vector):
        if self.is_base:
            return vector.pop()
        if isinstance(self.value, BooleanExpression):
            value = self.value.calculate(vector)
        else:
            value = vector.pop()
        return None if value is None else not value

    def pika(self):
        return not self.value.pika()

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.value == other.value

    def __repr__(self):
        return 'not {!r}'.format(self.value)


class DoubleBooleanExpression(BooleanExpression):
    AND = 'and'
    OR = 'or'
    IS = 'is'
    op = None

    def __init__(self, *args):
        super().__init__()
        self.args: List[BaseExpression] = list(args)

    def __repr__(self):
        return '{}({})'.format(
            self.__class__.__name__,
            ', '.join(repr(arg) for arg in self.args)
        )

    def calculate(self, vector):
        raise NotImplementedError()

    def special_rules(self, bool_, none):
        return self

    @property
    def convolution(self):
        self.args = [arg.convolution.to_bool for arg in self.args]
        bool_, none, self_classes, other = reduce(
            lambda acc, e: acc[e[0]].append(e[1]) or acc,
            (
                (idx, arg)
                for arg in self.args
                for idx in [
                    0
                    if isinstance(arg, Bool) else
                    1
                    if isinstance(arg, Null) else
                    2
                    if isinstance(arg, self.__class__) else
                    3
                ]
            ),
            [[], [], [], []]
        )
        sub_bool, sub_none, sub_other = reduce(
            lambda acc, e: acc[e[0]].append(e[1]) or acc,
            (
                (idx, arg)
                for self_class in self_classes
                for arg in self_class.args
                for idx in [
                    0
                    if isinstance(arg, Bool) else
                    1
                    if isinstance(arg, Null) else
                    2
                ]
            ),
            [[], [], []]
        )
        bool_ += sub_bool
        none += sub_none
        other += sub_other
        if none and not (bool_ or self_classes or other):
            return Null()
        self.args = other
        return self.special_rules(bool_, none)

    def __eq__(self, other):
        # Fixme
        return False

    def pika(self):
        raise NotImplementedError()


class Or(DoubleBooleanExpression):
    op = DoubleBooleanExpression.OR

    def special_rules(self, bool_, none):
        bool_ = (bool_ or None) and reduce(lambda a, b: a or b, [b.value for b in bool_])
        if bool_ is True:
            return Bool(True)

        elif self.args and none:
            self.args.append(Null())
            return self

        elif none and bool_ is False:
            return Null()

        elif not self.args and bool_ is False:
            return Bool(False)

        assert self.args

        return self

    def calculate(self, vector):
        if self.is_base:
            return vector.pop()
        args = [self._get_value_for_calculate(arg, vector) for arg in self.args]
        none, bool_ = reduce(
            lambda acc, e: acc[e[0]].append(e[1]) or acc,
            (
                (idx, arg)
                for arg in args
                for idx in [0 if arg is None else 1]
            ),
            [[], []]
        )
        bool_ = (bool_ or None) and reduce(lambda a, b: a or b, bool_)
        if bool_:
            return True
        if none:
            return None
        return False

    def pika(self):
        return pk.Criterion.any([arg.pika() for arg in self.args])


class And(DoubleBooleanExpression):
    op = DoubleBooleanExpression.AND

    def special_rules(self, bool_, none):
        bool_ = (bool_ or None) and reduce(lambda a, b: a and b, [b.value for b in bool_])
        if bool_ is False:
            return Bool(False)

        elif self.args and none:
            self.args.append(Null())
            return self

        elif none and bool_ is True:
            return Null()

        elif not self.args and bool_ is True:
            return Bool(True)

        assert self.args

        return self

    def calculate(self, vector):
        if self.is_base:
            return vector.pop()
        args = [self._get_value_for_calculate(arg, vector) for arg in self.args]
        none, bool_ = reduce(
            lambda acc, e: acc[e[0]].append(e[1]) or acc,
            (
                (idx, arg)
                for arg in args
                for idx in [0 if arg is None else 1]
            ),
            [[], []]
        )
        bool_ = (bool_ or None) and reduce(lambda a, b: a and b, bool_)
        if bool_ is False:
            return False
        if none:
            return None
        return True

    def pika(self):
        return pk.Criterion.all([arg.pika() for arg in self.args])


class Is(BooleanExpression):
    op = DoubleBooleanExpression.IS

    def __init__(self, left: BaseExpression, right: Union[bool, None]):
        super().__init__()
        self.left = left
        self.right = right

    @property
    def convolution(self):
        self.left = self.left.convolution.to_bool
        if isinstance(self.left, Bool):
            value = self.left.value == self.right
            return Bool(value)
        elif isinstance(self.left, Null):
            value = self.left is None
            return Bool(value)
        return self

    def calculate(self, vector):
        if self.is_base:
            return vector.pop()
        left = self._get_value_for_calculate(self.left, vector)
        return left is self.right

    def pika(self):
        if self.right is True:
            return pk.Bracket(self.left.pika()).istrue()
        elif self.right is False:
            return pk.Bracket(self.left.pika()).isfalse()
        elif self.right is None:
            return pk.Bracket(self.left.pika()).isnull()
        raise UnreachableException()

    def __repr__(self):
        return '({!r}) IS {}'.format(self.left, self.right)

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.left == other.left and self.right == other.right


class BasePredicate(BaseExpression):
    def pika(self):
        raise NotImplementedError()


class ComparisonPredicate(BasePredicate):
    __MAP_NOT = [
        (ss.equals_operator, ss.not_equals_operator),
        (ss.less_than_operator, ss.greater_than_or_equals_operator),
        (ss.greater_than_operator, ss.less_than_or_equals_operator)
    ]
    MAP_NOT = {
        k: v
        for a, b in __MAP_NOT
        for k, v in [(a, b), (b, a)]
    }

    __MAP_REVERSE = [
        (ss.less_than_operator, ss.greater_than_operator),
        (ss.less_than_or_equals_operator, ss.greater_than_or_equals_operator)
    ]
    MAP_REVERSE = {
        k: v
        for a, b in __MAP_REVERSE
        for k, v in [(a, b), (b, a)]
    }

    MAP_ACTION = {
        ss.equals_operator: lambda a, b: a == b,
        ss.not_equals_operator: lambda a, b: a != b,
        ss.less_than_operator: lambda a, b: a < b,
        ss.less_than_or_equals_operator: lambda a, b: a <= b,
        ss.greater_than_operator: lambda a, b: a > b,
        ss.greater_than_or_equals_operator: lambda a, b: a >= b,
    }

    def __init__(self, left, right, op):
        super().__init__()
        self.left = left
        self.right = right
        self.op = op
        self.action = self.MAP_ACTION[self.op]

    @property
    def convolution(self):
        if isinstance(self.left, BaseExpression):
            self.left = self.left.convolution.to_int
        elif isinstance(self.right, BaseExpression):
            self.right = self.right.convolution.to_int

        if isinstance(self.left, PrimaryNumeric) and isinstance(self.right, PrimaryNumeric):
            left = self.left.value
            right = self.right.value
            return Bool(self.action(left, right))
        elif isinstance(self.left, Null) or isinstance(self.right, Null):
            return Null()
        return self

    def not_(self):
        self.op = self.MAP_NOT[self.op]
        self.action = self.MAP_ACTION[self.op]
        return self

    def reverse(self):
        self.left, self.right = self.right, self.left
        self.op = self.MAP_REVERSE.get(self.op, self.op)
        self.action = self.MAP_ACTION[self.op]

    def pika(self):
        left = self.left.pika()
        right = self.right.pika()
        return self.action(left, right)

    def __eq__(self, other):
        # Fixme
        return False

    def __repr__(self):
        return '({!r} {} {!r})'.format(self.left, ss.NAME_TO_SYMBOL.get(self.op), self.right)


class StringExpression(BaseExpression):
    # Todo: Does not work
    def pika(self):
        raise NotImplementedError()


class DatetimeExpression(BaseExpression):
    # Todo: Does not work
    def pika(self):
        raise NotImplementedError()

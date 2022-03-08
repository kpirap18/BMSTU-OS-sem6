from . import utils


class BaseJoin:
    def __init__(self, left, right):
        self.left = left
        self.right = right
        self.sorted_columns = []

    @utils.lazy_property
    def size(self):
        return self.left.size + self.right.size

    def set_left(self, left):
        self.left = left

    def set_right(self, right):
        self.right = right

    def get_sql(self):
        raise NotImplementedError()

    def __repr__(self):
        return '{}({}, {})'.format(self.__class__.__name__, self.left, self.right)


class CrossJoin(BaseJoin):
    def get_sql(self):
        left = ('({})' if isinstance(self.left, BaseJoin) else '{}').format(self.left.get_sql())
        right = ('({})' if isinstance(self.right, BaseJoin) else '{}').format(self.right.get_sql())
        return '{} CROSS JOIN {}'.format(left, right)


class QualifiedJoin(BaseJoin):
    @property
    def type(self):
        raise NotImplementedError()

    def __init__(self, left, right, specification):
        super().__init__(left, right)
        self.specification = specification

    def get_sql(self):
        left = ('({})' if isinstance(self.left, BaseJoin) else '{}').format(self.left.get_sql())
        right = ('({})' if isinstance(self.right, BaseJoin) else '{}').format(self.right.get_sql())
        specification = self.specification.pika()
        specification = specification.get_sql(with_namespace=True) if specification else '1'
        return '{} {} JOIN {} ON {}'.format(left, self.type, right, specification)

    def __repr__(self):
        return '{}(left={}, right={}, specification={})'.format(
            self.__class__.__name__,
            self.left,
            self.right,
            self.specification
        )


class InnerJoin(QualifiedJoin):
    type = 'INNER'


class OuterJoin(QualifiedJoin):
    @property
    def type(self):
        raise NotImplementedError()


class LeftJoin(OuterJoin):
    type = 'LEFT'


class RightJoin(OuterJoin):
    type = 'RIGHT'

    def get_sql(self):
        left = ('({})' if isinstance(self.left, BaseJoin) else '{}').format(self.left.get_sql())
        right = ('({})' if isinstance(self.right, BaseJoin) else '{}').format(self.right.get_sql())
        specification = self.specification.pika()
        specification = specification.get_sql(with_namespace=True) if specification else '1'
        return '{} LEFT JOIN {} ON {}'.format(right, left, specification)


class FullJoin(OuterJoin):
    type = 'FULL'


DEFAULT_JOIN = LeftJoin

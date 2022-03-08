import logging

from . import expression as expr
from . import join as jn
from . import structures as st
from . import symbols as ss
from . import utils
from ._logger import ParserLogger
from .exceptions import UnreachableException, SemanticException
from itertools import product, groupby
from functools import reduce
import pypika as pk


class Select:
    logger: ParserLogger = logging.getLogger('selection')

    class PDNF:
        def __init__(self, expression):
            if isinstance(expression, (expr.ComparisonPredicate, expr.NumericExpression)):
                expression = expr.Is(expression, True).convolution

            # Начальное выражение
            self.raw_expression = expression
            # Базисы выражения
            raw_base = self.get_base_expressions(expression)
            if not raw_base[0]:
                expression.set_base()
                self.base_expressions = [expression]
            else:
                self.base_expressions = raw_base[0]
            # Комбинации, который дают True в выражении
            self.true_combination = [
                vector
                for vector in product(
                    [False, None, True],
                    repeat=len(self.base_expressions)
                )
                for value in [expression.calculate(list(vector[::-1]))]
                if value
            ]
            # Номера базисов, которые дают всегда False, None, True соответственно
            self.all_false, self.all_none, self.all_true = reduce(
                lambda acc, e: acc[e[0]].append(e[1]) or acc,
                (
                    (idx, n)
                    for n, arr in enumerate(zip(*self.true_combination))
                    for idx in [
                        0
                        if all(x is False for x in arr) else
                        1
                        if all(x is None for x in arr) else
                        2
                        if all(x is True for x in arr) else
                        None
                    ]
                    if idx is not None
                ),
                [[], [], []]
            )
            # Замена базисов. Например если базис всегда False,
            # то он заменяется на base is False,
            # также ищутся ненужные базисы (от которых не зависит выражение)
            self.equivalent_basis = [
                self.check_base_expr(i, basis)
                for i, basis in enumerate(self.base_expressions)
            ]
            # Используемые колонки в базисах,
            # для ComparisonPredicate колонки ищутся отдельно для правой и левой части
            self.used_columns = [
                val
                for b in self.base_expressions
                for val in [
                    (Select.get_used_columns(b.left, True), Select.get_used_columns(b.right, True))
                    if isinstance(b, expr.ComparisonPredicate) else
                    (Select.get_used_columns(b, True), None)
                ]
            ]
            # Пары колонок которые используются при JOIN, например, для
            # `select * from a inner join b on a.id = b.id and a.val >= b.val`
            # в список попадут колонки (a.id, b.id)
            self.join_expr_equals = []
            # Не используемые выражения. Выражения,
            # фильтрация по которым была осуществленна на уровне Базы данных
            self.not_used_expression = []

        def __repr__(self):
            return 'PDNF({}, not_used={}, basis={})'.format(
                self.true_combination,
                self.not_used_expression,
                self.base_expressions
            )

        def get_base_expressions(self, expression):
            if isinstance(expression, expr.Is):
                return self.get_base_expressions(expression.left)
            elif isinstance(expression, expr.Not):
                return self.get_base_expressions(expression.value)
            elif isinstance(expression, expr.DoubleBooleanExpression):
                meta = [
                    self.get_base_expressions(arg)
                    for arg in expression.args
                ]
                base_args, not_base_args = reduce(
                    lambda acc, e: acc[e[0]].append(e[1]) or acc,
                    (
                        (idx, (arg, bases, tables))
                        for arg, (bases, tables) in zip(expression.args, meta)
                        for idx in [0 if bases else 1]
                    ),
                    [[], []]
                )
                new_args = [
                    arg
                    for arg, _, _ in base_args
                ]
                new_bases = [
                    base
                    for _, bases, _ in base_args
                    for base in bases
                ]
                all_used_tables = reduce(lambda a, b: a | b, [t for _, t in meta])
                if not_base_args:
                    for arg, _, tbl in not_base_args:
                        assert len(tbl) <= 1

                    all_tables = reduce(lambda a, b: a | b, [tbl for _, _, tbl in not_base_args])
                    if len(all_tables) <= 1 and not base_args:
                        return [], all_tables

                    used_tables = [(arg, tbl.pop() if tbl else None) for arg, _, tbl in not_base_args]
                    used_tables = sorted(used_tables, key=lambda x: id(x[1]))

                    for group, rows in groupby(used_tables, key=lambda x: id(x[1])):
                        args = [a for a, t in rows]

                        if group is None:
                            assert len(args) == 0
                            new_args.append(args[0])
                            continue
                        if len(args) == 1:
                            arg, = args
                        else:
                            arg = expression.__class__(*args)

                        arg.set_base()
                        new_args.append(arg)
                        new_bases.append(arg)
                expression.args = new_args
                return new_bases, all_used_tables

            elif isinstance(expression, (
                expr.ComparisonPredicate,
                expr.NumericExpression,
            )):
                columns = Select.get_used_columns(expression)
                tables = {c.table for c in columns}
                if len(tables) > 1:
                    expression.set_base()
                    return [expression], tables
                return [], tables
            elif isinstance(expression, st.Column):
                return [], {expression.table}

        def __grouping(self, idx):
            _false, _none, _true = reduce(
                lambda acc, e: acc[e[0]].append(e[1]) or acc,
                (
                    (i, vector[:idx] + vector[idx+1:])
                    for vector in self.true_combination
                    for check in [vector[idx]]
                    for i in [
                        0
                        if check is False else
                        1
                        if check is None else
                        2
                    ]
                ),
                [[], [], []]
            )
            return _false, _none, _true

        def check_base_expr(self, i, basis):
            new_expr = None
            if i in self.all_true:  # e is True
                new_expr = expr.Is(basis, True)
            elif i in self.all_false:  # e is False
                new_expr = expr.Is(basis, False)
            elif i in self.all_none:  # e is None
                new_expr = expr.Is(basis, None)
            else:
                # Проверка на `is not` or any
                _false, _none, _true = self.__grouping(i)
                if _false == _none == _true:  # Any
                    self.not_used_expression.append(i)
                    lc, rc = self.used_columns[i]
                    for column in lc + (rc or []):
                        column.count_used -= 1
                elif _false == _none and not _true:  # e is not True
                    new_expr = expr.Not(expr.Is(basis, True))
                elif _false == _true and not _none:  # e is not None
                    new_expr = expr.Not(expr.Is(basis, None))
                elif _none == _true and not _false:  # e is not False
                    new_expr = expr.Not(expr.Is(basis, False))
                else:
                    return False, None
            return True, new_expr

        def basis_classifier_for_where(self):
            """
            Классифицирует условия для условия WHERE,
            так если базисное условие всегда принимает одно значение
            и при этом использует колонки из одной таблицы,
            то это условие можно вынести сразу в запрос данных
            """
            raw_used_columns = Select.get_used_columns(self.raw_expression)
            raw_used_tables = {
                column.table
                for column in raw_used_columns
            }
            if len(raw_used_tables) == 1:
                self.not_used_expression = list(range(len(self.base_expressions)))
                table = raw_used_tables.pop()
                table.filters.append(self.raw_expression)
                for column in raw_used_columns:
                    column.count_used -= 1
                return
            for i, (basis, (flag, new_basis), columns) in enumerate(zip(
                self.base_expressions,
                self.equivalent_basis,
                self.used_columns
            )):
                if not new_basis:
                    continue

                left_columns, right_columns = columns
                right_columns = right_columns or []
                all_columns = left_columns + right_columns
                table = {
                    column.table
                    for column in all_columns
                }
                assert len(all_columns)

                if len(table) == 1:
                    table = table.pop()
                    table.filters.append(new_basis)
                    self.not_used_expression.append(i)
                    for column in all_columns:
                        column.count_used -= 1

        def basis_classifier_inner_join(self, join_left_tables, join_right_tables):
            """
            Классифицирует условия для INNER_JOIN
            """
            join_left_tables = set(join_left_tables)
            join_right_tables = set(join_right_tables)
            join_expr_equals = []

            for i, (basis, (flag, new_basis), columns) in enumerate(zip(
                    self.base_expressions,
                    self.equivalent_basis,
                    self.used_columns
            )):
                left_columns, right_columns = columns
                if isinstance(basis, expr.ComparisonPredicate):
                    assert left_columns or right_columns
                    left_table = {
                        column.table
                        for column in left_columns
                    }
                    right_table = {
                        column.table
                        for column in right_columns
                    }
                    wrong_tables = (left_table | right_table) - (join_left_tables | join_right_tables)
                    if wrong_tables:
                        Select.logger.error(
                            'The tables (%s) is not included in the join',
                            ', '.join(sorted(
                                '.'.join(*table.full_name())
                                for table in wrong_tables
                            ))
                        )
                        continue

                    if left_table <= join_right_tables and right_table <= join_left_tables:
                        # Приведение к виду, где левая часть предиката
                        # соответствует левой таблице, а правая часть
                        # соответствует правой таблице
                        basis.reverse()
                        left_table, right_table = right_table, left_table
                        left_columns, right_columns = right_columns, left_columns

                    if len(left_table) == 1 and len(right_table) == 1:
                        left_table = left_table.pop()
                        right_table = right_table.pop()

                        if left_table == right_table:  # Условие на одну таблицу
                            if new_basis:
                                left_table.filters.append(new_basis)
                                self.not_used_expression.append(i)
                                for column in left_columns + right_columns:
                                    column.count_used -= 1
                        elif (
                            left_table in join_left_tables and
                            right_table in join_right_tables and
                            isinstance(basis.left, st.Column) and
                            isinstance(basis.right, st.Column)
                        ):
                            # left.column = right.column
                            if i in self.all_true and basis.op == ss.equals_operator:  # (a.id = b.id) is True
                                join_expr_equals.append((basis.left, basis.right))
                            elif i in self.all_false and basis.op == ss.not_equals_operator:  # (a.id != b.id) is False
                                join_expr_equals.append((basis.left, basis.right))
                            else:
                                continue
                            self.not_used_expression.append(i)
                    elif len(left_table) == 1 and len(right_table) == 0:
                        # tbl.column <= 10
                        table = left_table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in left_columns:
                                column.count_used -= 1
                    elif len(left_table) == 0 and len(right_table) == 1:
                        # tbl.column <= 10
                        table = right_table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in right_columns:
                                column.count_used -= 1
                    continue
                else:
                    assert len(left_columns)
                    table = {
                        column.table
                        for column in left_columns
                    }
                    if len(table) == 1:
                        table = table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in left_columns:
                                column.count_used -= 1

            self.join_expr_equals = join_expr_equals
            return join_expr_equals

        def basis_classifier_left_join(self, join_left_tables, join_right_tables):
            """
            Классифицирует условия для LEFT_JOIN
            """
            join_left_tables = set(join_left_tables)
            join_right_tables = set(join_right_tables)
            join_expr_equals = []

            for i, (basis, (flag, new_basis), columns) in enumerate(zip(
                    self.base_expressions,
                    self.equivalent_basis,
                    self.used_columns
            )):

                left_columns, right_columns = columns
                if isinstance(basis, expr.ComparisonPredicate):
                    assert left_columns or right_columns
                    left_table = {
                        column.table
                        for column in left_columns
                    }
                    right_table = {
                        column.table
                        for column in right_columns
                    }
                    wrong_tables = (left_table | right_table) - (join_left_tables | join_right_tables)
                    if wrong_tables:
                        Select.logger.error(
                            'The tables (%s) is not included in the join',
                            ', '.join(sorted(
                                '.'.join(*table.full_name())
                                for table in wrong_tables
                            ))
                        )
                        continue

                    if left_table <= join_right_tables and right_table <= join_left_tables:
                        # Приведение к виду, где левая часть предиката
                        # соответствует левой таблице, а правая часть
                        # соответствует правой таблице
                        basis.reverse()
                        left_table, right_table = right_table, left_table
                        left_columns, right_columns = right_columns, left_columns

                    if len(left_table) == 1 and len(right_table) == 1:
                        left_table = left_table.pop()
                        right_table = right_table.pop()

                        if left_table == right_table and {left_table} <= join_right_tables:  # Условие на одну таблицу
                            if new_basis:
                                left_table.filters.append(new_basis)
                                self.not_used_expression.append(i)
                                for column in left_columns + right_columns:
                                    column.count_used -= 1
                        elif (
                            left_table in join_left_tables and
                            right_table in join_right_tables and
                            isinstance(basis.left, st.Column) and
                            isinstance(basis.right, st.Column)
                        ):
                            # left.column = right.column
                            if i in self.all_true and basis.op == ss.equals_operator:  # (a.id = b.id) is True
                                join_expr_equals.append((basis.left, basis.right))
                            elif i in self.all_false and basis.op == ss.not_equals_operator:  # (a.id != b.id) is False
                                join_expr_equals.append((basis.left, basis.right))
                            else:
                                continue
                            self.not_used_expression.append(i)
                    elif len(left_table) == 0 and len(right_table) == 1 and right_table <= join_right_tables:
                        table = right_table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in left_columns:
                                column.count_used -= 1
                    continue
                else:
                    assert len(left_columns)
                    table = {
                        column.table
                        for column in left_columns
                    }
                    if len(table) == 1 and table <= join_right_tables:
                        table = table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in left_columns:
                                column.count_used -= 1

            self.join_expr_equals = join_expr_equals
            return join_expr_equals

        def basis_classifier_right_join(self, join_left_tables, join_right_tables):
            """
            Классифицирует условия для RIGHT_JOIN
            """
            join_left_tables = set(join_left_tables)
            join_right_tables = set(join_right_tables)
            join_expr_equals = []

            for i, (basis, (flag, new_basis), columns) in enumerate(zip(
                    self.base_expressions,
                    self.equivalent_basis,
                    self.used_columns
            )):

                left_columns, right_columns = columns
                if isinstance(basis, expr.ComparisonPredicate):
                    assert left_columns or right_columns
                    left_table = {
                        column.table
                        for column in left_columns
                    }
                    right_table = {
                        column.table
                        for column in right_columns
                    }
                    wrong_tables = (left_table | right_table) - (join_left_tables | join_right_tables)
                    if wrong_tables:
                        Select.logger.error(
                            'The tables (%s) is not included in the join',
                            ', '.join(sorted(
                                '.'.join(*table.full_name())
                                for table in wrong_tables
                            ))
                        )
                        continue

                    if left_table <= join_right_tables and right_table <= join_left_tables:
                        # Приведение к виду, где левая часть предиката
                        # соответствует левой таблице, а правая часть
                        # соответствует правой таблице
                        basis.reverse()
                        left_table, right_table = right_table, left_table
                        left_columns, right_columns = right_columns, left_columns

                    if len(left_table) == 1 and len(right_table) == 1:
                        left_table = left_table.pop()
                        right_table = right_table.pop()

                        if left_table == right_table and {right_table} <= join_left_tables:  # Условие на одну таблицу
                            if new_basis:
                                left_table.filters.append(new_basis)
                                self.not_used_expression.append(i)
                                for column in left_columns + right_columns:
                                    column.count_used -= 1
                        elif (
                            left_table in join_left_tables and
                            right_table in join_right_tables and
                            isinstance(basis.left, st.Column) and
                            isinstance(basis.right, st.Column)
                        ):
                            # left.column = right.column
                            if i in self.all_true and basis.op == ss.equals_operator:  # (a.id = b.id) is True
                                join_expr_equals.append((basis.left, basis.right))
                            elif i in self.all_false and basis.op == ss.not_equals_operator:  # (a.id != b.id) is False
                                join_expr_equals.append((basis.left, basis.right))
                            else:
                                continue
                            self.not_used_expression.append(i)
                    elif len(left_table) == 1 and len(right_table) == 0 and left_table <= join_left_tables:
                        table = left_table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in left_columns:
                                column.count_used -= 1
                    continue
                else:
                    assert len(left_columns)
                    table = {
                        column.table
                        for column in left_columns
                    }
                    if len(table) == 1 and table <= join_left_tables:
                        table = table.pop()
                        if new_basis:
                            table.filters.append(new_basis)
                            self.not_used_expression.append(i)
                            for column in left_columns:
                                column.count_used -= 1

            self.join_expr_equals = join_expr_equals
            return join_expr_equals

        def basis_classifier_full_join(self, join_left_tables, join_right_tables):
            """
            Классифицирует условия для FULL_JOIN
            """
            return []

        def pika(self):
            if not self.not_used_expression:
                return self.raw_expression.pika()
            and_ = None
            or_ = None
            index = None

            not_used_expression = set(self.not_used_expression)
            idx_or, idx_and = reduce(
                lambda acc, e: acc[e[0]].append(e[1]) or acc,
                (
                    (int(flag), i)
                    for i, (flag, new_basis) in enumerate(self.equivalent_basis)
                    if i not in not_used_expression
                ),
                [[], []]
            )

            if idx_or:
                true_combination = set(zip(*[
                    col
                    for i, col in enumerate(zip(*self.true_combination))
                    if i in idx_or
                ]))
                used_expressions = [
                    self.base_expressions[i]
                    for i in idx_or
                ]
                or_ = []
                for is_values in true_combination:
                    crit = pk.Criterion.all(
                        expr.Is(e, v).pika()
                        for e, v in zip(used_expressions, is_values)
                    )
                    or_.append(crit)
                or_ = pk.Criterion.any(or_)

            if idx_and:
                and_ = [
                    new_basis.pika()
                    for i, (flag, new_basis) in enumerate(self.equivalent_basis)
                    if i in idx_and
                ]
                and_ = pk.Criterion.all(and_)

            if self.join_expr_equals:
                index = pk.Criterion.all([
                    (a.pika() == b.pika())
                    for a, b in self.join_expr_equals
                ])

            if not any([index, and_, or_]):
                return None
            else:
                return pk.Criterion.all([e for e in [index, and_, or_] if e])

    def __init__(self, cc, select_list, from_, where=None, group=None, having=None):
        if len(from_) != 1:
            self.logger.error('Support only one one table or join')
            return
        self.cc = cc  # ControlCenter

        self.select_list = select_list
        self.from_ = from_
        self.where = where
        self.group = group
        self.having = having

        self.alias_table = {}
        self.alias_selection = {}

        self.name_to_table = {}

        self.full_selection_list = []
        self.full_table_list = []

    def validate(self):
        self.validate_from()
        self.validate_all_join()
        self.validate_select_list()
        self.validate_where()

    def validate_from(self):
        self.from_ = [
            self.check_all_tables(tbl)[1] or tbl
            for tbl in self.from_
            for _ in [self.full_table_list.append([])]
        ]

    @property
    def result_columns(self):
        return [s.short_name or 'column_{}'.format(i+1)
                for i, s in enumerate(self.select_list)]

    def check_all_tables(self, table):
        """
        Грязная функция - меняет состояния уже существующих объектов
        """
        if isinstance(table, utils.NamingChain):
            return self.check_table(table)
        elif isinstance(table, jn.BaseJoin):
            left_name, left_obj = self.check_all_tables(table.left)
            if left_obj:
                table.left = left_obj
            right_name, right_obj = self.check_all_tables(table.right)
            if right_obj:
                table.right = right_obj
            if isinstance(table, jn.QualifiedJoin):
                if not isinstance(table.specification, (
                        expr.BooleanExpression,
                        expr.BasePredicate,
                        expr.Column,
                )):
                    self.logger.error('Support join condition only boolean expression or column')
                else:
                    table.specification = self.validate_expression(table.specification.convolution.to_bool)
                    # TODO: Подумать
                    if True or isinstance(table.specification, expr.BooleanExpression):
                        table.specification = self.PDNF(table.specification)

            return None, None

    def check_table(self, table_naming_chain, only_get=False):
        """
        1) Находим полное имя таблицы,
        для alias возможно найти сразу таблицу
        2) По полному имени ищем таблицу в словаре
        3) Если таблицы нет, то создаем ее
        и кладем в словарь и список
        4) Если таблица имеет короткое имя,
        то сохроняем ее под этим именем в self.alias_table,
        иначе сохраняем по
        """
        name = table_naming_chain.get_data()
        alias = table_naming_chain.short_name

        table_obj = None
        dbms = db = schema = table = None

        if len(name) == 4:  # Full name
            dbms, db, schema, table = name

        elif len(name) == 3:  # Alias db
            alias_db, schema, table = name
            find = self.cc.local_alias['db'].get(alias_db)
            if not find:
                self.logger.error('Alias db %s not found', alias_db)
                return None, None
            dbms, db = find
        elif len(name) == 2:  # Alias schema
            alias_schema, table = name
            find = self.cc.local_alias['schema'].get(alias_schema)
            if not find:
                self.logger.error('Alias schema %s not found', alias_schema)
                return None, None
            dbms, db, schema = find
        elif len(name) == 1:  # Alias table
            alias_table, = name
            table_obj = self.alias_table.get(alias_table)
            if not table_obj:
                find = self.cc.local_alias['table'].get(alias_table)
                if not find:
                    self.logger.error('Alias table %s not found', alias_table)
                    return None, None
                dbms, db, schema, table = find
        else:
            self.logger.error('Wrong naming chain for table: %s', table_naming_chain)
            return None, None

        full_name = utils.NamingChain(dbms, db, schema, table)
        table_obj = table_obj or self.name_to_table.get(full_name.get_data())

        if only_get:
            if not table_obj:
                self.logger.error('Table %s not found in FROM', table_naming_chain)
                return None, None
            return full_name, table_obj

        if not table_obj:
            # Если dbms есть в псевдонимах dbms, то используем оригинал
            dbms = self.cc.local_alias['dbms'].get(dbms, dbms)
            dbms_obj = self.cc.sources.get(dbms)
            if not dbms_obj:
                self.logger.error('DBMS %s not found', dbms)
                return None, None
            try:
                table_obj = st.Table(dbms_obj, db, schema, table)
            except SemanticException:
                return None, None
            if full_name.get_data() in self.name_to_table:
                self.logger.error('Many use table %s not supported', table_naming_chain)
                return None, None
            self.name_to_table[full_name.get_data()] = table_obj

        self.full_table_list[-1].append(table_obj)

        if alias:
            if alias in self.alias_table:
                self.logger.error('Duplicate alias table %s', alias)
                return None, None
            self.alias_table[alias] = table_obj
        elif str(full_name) not in self.alias_table:
            self.alias_table[str(full_name)] = table_obj

        return full_name, table_obj

    def validate_all_join(self):
        for join in self.from_:
            self.validate_join(join)

    def validate_join(self, table):
        if isinstance(table, st.Table):
            return [table]
        elif isinstance(table, jn.CrossJoin):
            return self.validate_join(table.left) + self.validate_join(table.right)
        elif isinstance(table, jn.QualifiedJoin):
            left = self.validate_join(table.left)
            right = self.validate_join(table.right)
            if isinstance(table, jn.InnerJoin):
                join_expr_equals = table.specification.basis_classifier_inner_join(left, right)
            elif isinstance(table, jn.LeftJoin):
                join_expr_equals = table.specification.basis_classifier_left_join(left, right)
            elif isinstance(table, jn.RightJoin):
                join_expr_equals = table.specification.basis_classifier_right_join(left, right)
            elif isinstance(table, jn.FullJoin):
                join_expr_equals = table.specification.basis_classifier_full_join(left, right)
            else:
                raise UnreachableException()
            if len(left) == 1:
                ltbl, = left
                ltbl.add_sorted_columns = [a for a, b in join_expr_equals]
            if len(right) == 1:
                rtbl, = right
                rtbl.add_sorted_columns = [b for a, b in join_expr_equals]
            return left + right
        else:
            raise UnreachableException

    def validate_select_list(self):
        """
        В select_list, все выражения типа Column()
        заменяются на объект колонки из таблицы,
        все такие колонки помечаются как используемые
        """
        select_list = []

        if isinstance(self.select_list, str):  # all columns for all tables
            assert self.select_list == ss.asterisk
            for level in self.full_table_list:
                for tbl in level:
                    for column in tbl.columns:
                        column.used = True
                        column.visible = True
                    select_list.extend(tbl.columns)
        else:
            for is_qualified_asterisk, selection in self.select_list:
                if is_qualified_asterisk:  # all column for one table
                    check = self.check_table(selection, True)
                    if not check:
                        continue
                    _, tbl = check
                    for column in tbl.columns:
                        column.used = True
                        column.visible = True
                    select_list.extend(tbl.columns)
                else:  # expression
                    short_name = selection.short_name
                    repr_name = repr(selection)
                    e = self.validate_expression(selection.convolution, True)
                    if short_name:
                        self.alias_selection[short_name] = e
                    e.as_(short_name or repr_name)
                    select_list.append(e)
        self.select_list = select_list

    def validate_expression(self, expression, visible=False):
        """
        Грязная функция - меняет состояния уже существующих объектов
        """
        if isinstance(expression, expr.Column):
            name = expression.value
            tuple_name = name.get_data()
            if len(tuple_name) == 1:
                alias_expression = self.alias_selection.get(tuple_name[0])
                if alias_expression is None:
                    self.logger.error('Column alias %s not found', tuple_name[0])
                    return
                return alias_expression
            assert 2 <= len(tuple_name) <= 5
            *table_name, column_name = tuple_name
            full_table_name, table_obj = self.check_table(utils.NamingChain(*table_name), True)
            if not table_obj:
                return expression

            column: st.Column = table_obj.name_to_column.get(column_name)
            if not column:
                self.logger.error('Column %s not found', name)
                return expression
            column.used = True
            if visible:
                column.visible = True
            return column

        elif isinstance(expression, expr.NumericExpression):
            if isinstance(expression, expr.UnarySign):
                expression.value = self.validate_expression(expression.value, visible)
            elif isinstance(expression, expr.DoubleNumericExpression):
                expression.left = self.validate_expression(expression.left, visible)
                expression.right = self.validate_expression(expression.right, visible)
            else:
                raise UnreachableException()

        elif isinstance(expression, expr.BooleanExpression):
            if isinstance(expression, expr.Not):
                expression.value = self.validate_expression(expression.value, visible)
            elif isinstance(expression, expr.Is):
                expression.left = self.validate_expression(expression.left, visible)
            elif isinstance(expression, expr.DoubleBooleanExpression):
                expression.args = [self.validate_expression(arg, visible) for arg in expression.args]
            else:
                raise UnreachableException()

        elif isinstance(expression, expr.ComparisonPredicate):
            expression.left = self.validate_expression(expression.left, visible)
            expression.right = self.validate_expression(expression.right, visible)
        return expression

    def validate_where(self):
        if not self.where:
            return
        if not isinstance(self.where, (
            expr.BooleanExpression,
            expr.BasePredicate,
            expr.Column,
        )):
            self.logger.error('Support where condition only boolean expression or column')
            return
        self.where = self.where and self.validate_expression(self.where.convolution.to_bool)
        # TODO: Подумать
        if True or isinstance(self.where, expr.BooleanExpression):
            self.where = self.PDNF(self.where)
            self.where.basis_classifier_for_where()

    @staticmethod
    def get_used_columns(expression, count_used=False):
        if isinstance(expression, st.Column):
            if count_used:
                expression.count_used += 1
            return [expression]
        elif isinstance(expression, expr.Is):
            return Select.get_used_columns(expression.left, count_used)
        elif isinstance(expression, expr.DoubleNumericExpression):
            left = Select.get_used_columns(expression.left, count_used)
            right = Select.get_used_columns(expression.right, count_used)
            return left + right
        elif isinstance(expression, expr.DoubleBooleanExpression):
            return [
                column
                for arg in expression.args
                for column in Select.get_used_columns(arg, count_used)
            ]
        elif isinstance(expression, (
            expr.Not,
            expr.UnarySign,
        )):
            return Select.get_used_columns(expression.value, count_used)
        elif isinstance(expression, expr.ComparisonPredicate):
            left = Select.get_used_columns(expression.left, count_used)
            right = Select.get_used_columns(expression.right, count_used)
            return left + right
        return []

    def get_sql(self):
        st.Table.IS_SQLITE = True
        from_sql = ', '.join([f.get_sql() for f in self.from_])
        select_sql = ', '.join([
            s.pika().as_(alias).get_sql(with_namespace=True)
            for i, s in enumerate(self.select_list)
            for alias in [s.short_name or 'column_{}'.format(i+1)]
        ])
        sql = 'SELECT {} FROM {}'.format(select_sql, from_sql)
        if self.where:
            where_pika = self.where.pika()
            if where_pika:
                sql = '{} WHERE {}'.format(sql, where_pika.get_sql(with_namespace=True))
        st.Table.IS_SQLITE = False
        return sql

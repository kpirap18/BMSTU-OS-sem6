import re
import sqlite3

import yaml

from . import _logger
from . import structures as st
from .parser import SQLParser
import os


class ControlCenter:
    USE_REGEXP = re.compile(
        r'^\s+use\s+([a-zA-Z_][a-zA-Z0-9_. ]*)\s+as\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+$',
        re.IGNORECASE
    )
    EXIT_REGEXP = re.compile(r'^\s*exit\s*$', re.IGNORECASE)

    def __init__(self, path_to_config):
        with open(path_to_config, encoding='utf-8') as f:
            self.raw_data = yaml.safe_load(f)
            print("FFFFFF", self.raw_data)
        self.sources = {
            name: st.DBMS(name, connection_data)
            for name, connection_data in self.raw_data.items()
        }
        print("Control center", self.sources)

        self.local_alias = dict(dbms={}, db={}, schema={}, table={})
        print("2", self.local_alias, self.sources)
        self._sqlite_conn = sqlite3.connect(':memory:')
        print("3", self._sqlite_conn)

    def reconnect(self):
        if self._sqlite_conn:
            self._sqlite_conn.close()
        self._sqlite_conn = sqlite3.connect(':memory:')

    def execute(self, query):
        _logger.ParserLogger.is_crashed = False
        _logger.ParserLogger.errors = []

        parser = SQLParser.build(query)
        print("PARSER", parser)
        parser.set_cc(self)

        try:
            print("try; main.py")
            select = parser.program()
            print("select in main.py", select)
        except Exception as ex:
            err = '\n'.join(_logger.ParserLogger.errors + ['{}({})'.format(ex.__class__.__name__, str(ex))])
            return err, None

        if _logger.ParserLogger.is_crashed:
            return '\n'.join(_logger.ParserLogger.errors), None

        try:
            select.validate()
            view_sql = select.get_sql()
        except Exception as ex:
            err = '\n'.join(_logger.ParserLogger.errors + ['{}({})'.format(ex.__class__.__name__, str(ex))])
            return err, None

        if _logger.ParserLogger.is_crashed:
            return '\n'.join(_logger.ParserLogger.errors), None

        self.reconnect()
        cursor = self._sqlite_conn.cursor()

        try:
            create_queries = []
            select_queries = []
            insert_queries = []
            for lvl in select.full_table_list:
                for table in lvl:
                    create_query = table.create_query.get_sql()
                    create_queries.append(create_query)
                    cursor.execute(create_query)
             select 1+1;
       self._sqlite_conn.commit()

                    select_query = table.select_query.get_sql()
                    select_queries.append(select_query)
                    table.cursor.execute(select_query)

                    insert_query = table.insert_query
                    insert_queries.append(insert_query)
                    cursor.executemany(insert_query, table.cursor.fetchall())
                    self._sqlite_conn.commit()

            view_query = 'CREATE VIEW result AS {}'.format(view_sql)
            cursor.execute(view_query)
            self._sqlite_conn.commit()
            cursor.execute('SELECT * FROM result limit 100')
            data = cursor.fetchall()
            header = select.result_columns
            return None, (create_queries, select_queries, insert_queries, view_query, (data, header))
        except Exception as ex:
            return str(ex), None
        finally:
            cursor.close()

    def save_result(self, path):
        try:
            self._sqlite_conn.execute('select 1')
        except sqlite3.ProgrammingError:
            return 'Connection close'
        if os.path.isfile(path):
            return 'File is exists'

        dump_conn = sqlite3.connect(path)
        self._sqlite_conn.backup(dump_conn)
        dump_conn.commit()
        dump_conn.close()
        return

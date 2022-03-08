import logging

from . import _logger
from . import token as tk

# noinspection PyTypeChecker
logger = logging.getLogger('lexer')  # type: _logger.ParserLogger
logger.display_position()


class Position:
    END_CHAR = '\000'

    def __init__(self, text, n=None, idx=None, row=None, col=None):
        self._text = text
        self.n = n or len(text)
        self.idx = idx or 0
        self.row = row or 1
        self.col = col or 1

    @property
    def text(self):
        return self._text[self.idx:]

    @property
    def char(self):
        return self._text[self.idx] if self.idx < self.n else self.END_CHAR

    def next(self):
        if self.char == '\n':
            self.row += 1
            self.col = 1
        else:
            self.col += 1
        self.idx = self.idx + 1 if self.idx < self.n else self.idx

    def nextn(self, n):
        assert n > 0
        for _ in range(n):
            self.next()

    @property
    def isspace(self):
        return self.char.isspace()

    @property
    def is_end(self):
        return self.idx == self.n

    def skip_space(self):
        while self.isspace:
            self.next()

    def __sub__(self, other):
        return self._text[other.idx:self.idx]

    def __copy__(self):
        return Position(self._text, self.n, self.idx, self.row, self.col)

    def copy(self):
        return self.__copy__()

    def __str__(self):
        return '<{: >2}:{: >2}>'.format(self.row, self.col)


class Interval:
    def __init__(self, start=None, end=None):
        self._start = start and start.copy()
        self._end = end and start.copy()

    @property
    def start(self):
        return self._start

    @start.setter
    def start(self, value: Position):
        self._start = value.copy()

    @start.deleter
    def start(self):
        self._start = None

    @property
    def end(self):
        return self._end

    @end.setter
    def end(self, value: Position):
        self._end = value.copy()

    @end.deleter
    def end(self):
        self._end = None

    def __str__(self):
        assert self._start and self._end
        return self.end - self.start

    def __repr__(self):
        assert self._start and self._end
        return '[{}-{}]'.format(self._start, self._end)


EMPTY_INTERVAL = Interval(Position(''), Position(''))


class Lexer:
    TOKENS = [
        tk.IntToken,
        tk.FloatToken,
        tk.StringToken,
        tk.DateToken,
        tk.DatetimeToken,
        tk.IdentifierToken,
        tk.KeywordToken,
        tk.SymbolToken
    ]

    def __init__(self, pos, interval=EMPTY_INTERVAL, last_interval=EMPTY_INTERVAL, current_tokens=None):
        self.pos = pos

        self.interval = interval
        self.last_interval = last_interval
        self.current_tokens = current_tokens

    def next(self):
        old_token = self.current_tokens
        self.current_tokens = self.parse()
        logger.current_token.debug(repr(self.current_tokens))
        return old_token

    def get_matches(self):
        start = self.pos.copy()
        return start, sorted((
            (t, t.match(start))
            for t in self.TOKENS
        ), key=lambda x: x[1][0], reverse=True)

    def parse(self):
        self.pos.skip_space()
        while not self.pos.is_end:
            self.last_interval, self.interval = self.interval, Interval()

            self.interval.start, matches = self.get_matches()
            max_size = matches[0][1][0]

            if not max_size:
                while not self.pos.isspace and not self.pos.is_end:
                    self.pos.next()
                self.interval.end = self.pos.copy()
                logger.current_token.error('Wrong sequences: %s', str(self.interval))
            else:
                self.pos.nextn(max_size)
                self.interval.end = self.pos.copy()
                tokens = tuple(
                    t(match, self.interval)
                    for t, (size, match) in matches
                    if size == max_size
                )
                if any(t.kind == t.KEYWORD and t.is_reserved for t in tokens):
                    tokens = tuple(
                        t
                        for t in tokens
                        if t.kind != t.IDENTIFIER
                    )
                return tokens

            self.pos.skip_space()

        return [tk.EndToken()]

    def __copy__(self):
        return self.__class__(self.pos.copy(), self.interval, self.last_interval, self.current_tokens)

    def copy(self):
        return self.__copy__()

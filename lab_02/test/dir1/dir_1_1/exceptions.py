class ParseException(Exception):
    """
    Ошбка лексера
    """
    pass


class ParseDateException(ParseException):
    pass


class ParseDatetimeException(ParseException):
    pass


class SyntaxException(Exception):
    """
    Ошибка парсера
    """
    pass


class FatalSyntaxException(SyntaxException):
    pass


class SemanticException(Exception):
    """
    Ошибка в дереве разбора
    """
    pass


class WrongNamingChain(SemanticException):
    pass


class NotSupported(Exception):
    pass


class UnreachableException(Exception):
    pass

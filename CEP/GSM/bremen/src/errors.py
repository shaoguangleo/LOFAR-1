#!/usr/bin/env python
class GSMException(Exception):
    """
    Basic GSM exception class.
    """
    pass

class SourceException(GSMException):
    """
    No source or source reading error.
    """
    pass

class SourcePartMissingException(GSMException):
    """
    Required part of the source information is missing.
    """
    pass

class ParsetContentError(GSMException):
    """
    Missing part of the parset file or parset file is wrong.
    """
    pass

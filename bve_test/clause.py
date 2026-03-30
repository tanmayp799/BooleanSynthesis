from typing import Iterable, List, Any

class Clause:
    """
    A class that stores a clause (list of literals) and a garbage flag.
    """

    def __init__(self, literals: Iterable[Any], garbage: bool = False):
        """
        Initialize a Clause.

        Args:
            literals: An iterable representing the clause (e.g., list of integers).
            garbage: A boolean flag indicating if the clause is garbage.
                     Defaults to False.
        """
        self.literals: List[Any] = list(literals)
        self.garbage: bool = garbage

    def __repr__(self) -> str:
        return f"Clause(literals={self.literals}, garbage={self.garbage})"
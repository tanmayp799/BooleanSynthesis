import sys
from typing import List
from clause import Clause

def parse_dqdimacs(file_path: str) -> List[Clause]:
    """
    Opens a DQDIMACS file, reads the clauses, and returns a list of Clause objects.

    This function ignores:
    - Comments (lines starting with 'c')
    - Headers (lines starting with 'p')
    - Quantifier definitions (lines starting with 'a' or 'e')
    - Dependency definitions (lines starting with 'd')

    Args:
        file_path: The path to the DQDIMACS file.

    Returns:
        A list of Clause objects.
    """
    clauses: List[Clause] = []
    current_literals: List[int] = []

    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            # Skip empty lines, comments, and headers
            if not line or line.startswith(('c', 'p')):
                continue

            # Skip quantifier blocks and dependencies
            if line.startswith(('a', 'e', 'd')):
                continue

            for token in line.split():
                try:
                    lit = int(token)
                except ValueError:
                    continue

                if lit == 0:
                    clauses.append(Clause(current_literals))
                    current_literals = []
                else:
                    current_literals.append(lit)

    return clauses


def bve(clauses: List[Clause], variableToEliminate: int) -> List[Clause]:
    """
    Performs Bounded Variable Elimination (BVE) on a list of clauses.

    Args:
        clauses: The list of Clause objects.
        variableToEliminate: The integer representing the variable to eliminate.

    Returns:
        A new list of Clause objects after performing variable elimination.
    """
    pos_clauses = []
    neg_clauses = []
    other_clauses = []

    # Partition clauses into three sets
    for clause in clauses:
        if variableToEliminate in clause.literals:
            pos_clauses.append(clause)
        elif -variableToEliminate in clause.literals:
            neg_clauses.append(clause)
        else:
            other_clauses.append(clause)

    resolvents = []
    for p_clause in pos_clauses:
        for n_clause in neg_clauses:
            res_literals = [lit for lit in p_clause.literals if lit != variableToEliminate] + \
                           [lit for lit in n_clause.literals if lit != -variableToEliminate]
            unique_literals = set(res_literals)
            if not any(-lit in unique_literals for lit in unique_literals):
                resolvents.append(Clause(list(unique_literals)))

    return other_clauses + resolvents

def main():
    if len(sys.argv) < 2:
        print("Usage: python parser.py <input_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    try:
        clauses = parse_dqdimacs(input_file)
        print(f"Parsed {len(clauses)} clauses:")
        for clause in clauses:
            print(clause)
        print("\n")
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found.")
        sys.exit(1)

    toElim = [16,19,24,20,25,17,21,15,23,27,22,18,26]
    for var in toElim:
        # if var==15:
        #     continue
        new_clauses = bve(clauses, var)
        print(f"Clauses after eliminating variable {var}:")
        for clause in new_clauses:
            print(clause)
        print("\n")
        clauses = new_clauses


if __name__ == "__main__":
    main()
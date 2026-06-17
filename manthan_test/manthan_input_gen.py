import sys

def convert_dqbf_to_qbf(dqdimacs_path, output_qdimacs, output_mapping):
    
    header = ""
    clauses = []
    
    universals = set() # 'a' variables
    d_vars = set()     # 'd' variables (will become universals in QBF)
    e_vars = set()     # 'e' variables (explicit or implicit)
    all_vars_in_clauses = set()
    
    max_var = 0

    # --- 1. PARSE DQDIMACS ---
    with open(dqdimacs_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('c'):
                continue
            
            if line.startswith('p'):
                header = line
                try:
                    parts = line.split()
                    max_var = int(parts[2])
                except:
                    pass
                continue
            
            if line.startswith('a'):
                # Format: a 1 2 3 0
                parts = line.split()[1:]
                for x in parts:
                    if x != '0':
                        universals.add(int(x))
            
            elif line.startswith('d'):
                # Format: d <var> <dependencies> 0
                parts = line.split()[1:]
                if len(parts) > 0:
                    # The first number is the existential var with dependencies
                    # But for Manthan QBF, we treat D-vars as "Inputs" (pseudo-universals)
                    d_var = int(parts[0])
                    d_vars.add(d_var)
            
            elif line.startswith('e'):
                parts = line.split()[1:]
                for x in parts:
                    if x != '0':
                        e_vars.add(int(x))
            
            else:
                # Clause: 1 -2 3 0
                clauses.append(line)
                parts = line.split()
                for x in parts:
                    if x != '0':
                        val = abs(int(x))
                        all_vars_in_clauses.add(val)
                        if val > max_var: max_var = val

    # --- 2. CLASSIFY VARIABLES ---
    
    # In your logic, D-variables become inputs to the synthesis (like universals)
    # So "Manthan Inputs" = Original Universals + D_vars
    manthan_inputs = universals.union(d_vars)
    
    # "Manthan Outputs" = Anything else (True Existentials)
    # If a var is not in inputs, it's an existential
    manthan_outputs = set()
    
    # First add explicitly declared 'e' vars
    for v in e_vars:
        if v not in manthan_inputs:
            manthan_outputs.add(v)
            
    # Then catch implicit vars (vars in clauses that were never declared)
    for v in range(1, max_var + 1):
        if v not in manthan_inputs and v not in manthan_outputs:
            manthan_outputs.add(v)

    # --- 3. SORT (CRITICAL FOR MAPPING ALIGNMENT) ---
    # We sort them so the QDIMACS declaration order matches the mapping file order.
    sorted_inputs = sorted(list(manthan_inputs))
    sorted_outputs = sorted(list(manthan_outputs))

    # --- 4. WRITE QDIMACS ---
    with open(output_qdimacs, 'w') as out:
        # Reconstruct header with correct variable count
        # p cnf <vars> <clauses>
        out.write(f"p cnf {max_var} {len(clauses)}\n")
        
        # Write 'a' line (Inputs)
        if sorted_inputs:
            out.write("a " + " ".join(map(str, sorted_inputs)) + " 0\n")
        
        # Write 'e' line (Outputs)
        if sorted_outputs:
            out.write("e " + " ".join(map(str, sorted_outputs)) + " 0\n")
            
        # Write Clauses
        for c in clauses:
            out.write(c + "\n")

    # --- 5. WRITE ORDERING FILE ---
    # This matches your C++ expectations:
    # Line 1 -> inputMapping (mapped to Manthan inputs)
    # Line 2 -> outputMapping (mapped to the 'padding' inputs in your code)
    with open(output_mapping, 'w') as map_file:
        map_file.write(" ".join(map(str, sorted_inputs)) + "\n")
        map_file.write(" ".join(map(str, sorted_outputs)) + "\n")

    print(f"Generated QDIMACS: {output_qdimacs}")
    print(f"Generated Mapping: {output_mapping}")
    print(f"  - Inputs (Line 1): {len(sorted_inputs)} vars")
    print(f"  - Outputs (Line 2): {len(sorted_outputs)} vars")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python dqbf_to_manthan.py <input.dqdimacs> <out.qdimacs> <out.order>")
        sys.exit(1)
        
    convert_dqbf_to_qbf(sys.argv[1], sys.argv[2], sys.argv[3])
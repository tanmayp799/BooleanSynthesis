import numpy as np
import os
import sys
import json

def read_lines(f):
    lines=[]
    for line in f:
        lines.append(line)
    
    return lines


def write_dimacs(numInputs, clauses, output_file):
    with open(output_file,'w') as f:
        f.write(f"p cnf {numInputs} {len(clauses)}\n")

        for clause in clauses:
            f.write(" ".join(map(str,clause))+" 0\n")
    print(f"DIMACS file written to: '{output_file}'")

if __name__ == "__main__":
    if len(sys.argv) !=2:
        print("Usage: python parser.py <path to dqdimacs file>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    abs_path = os.path.join(os.getcwd(),file_path)

    if not os.path.isfile(abs_path):
        print(f"Error: The file '{file_path}' does not exist")
        sys.exit(1)

    print(f"Opening file: '{file_path}'")

    with open(abs_path,"r") as f:
        lines = read_lines(f)
    
    quantifiers = []
    dependencies = {}
    main_clauses = []
    existential = []

    for line in lines:
        line_parse = [x for x in line.split(" ")]
        if line_parse[0]=='a':
            quantifiers = [int(x) for x in line_parse[1:-1]]
        elif line_parse[0] =='d':
            existential.append(int(line_parse[1]))
            dependencies[int(line_parse[1])] = [int(x) for x in line_parse[2:-1]]

        elif line_parse[0]=='e':
            pass
        elif line_parse[0]!='p' and line_parse[0]!='c':
            clause = [int(x) for x in line_parse[:-1]]
            main_clauses.append(clause)
    
    numInputs = len(quantifiers) +len(dependencies)


    dimacs_file_main = os.path.splitext(os.path.basename(file_path))[0]

    print(f"Creating dimacs file: '{dimacs_file_main}.dimacs'")
    output_dir = os.path.join(os.getcwd(),"data",dimacs_file_main)

    os.makedirs(output_dir,exist_ok=True)
    print(f"Created directory: {output_dir}")

    phi_dimacs = os.path.join(output_dir,f"{dimacs_file_main}.dimacs")

    write_dimacs(numInputs,main_clauses,phi_dimacs)


    phi_dict={}
    non_trivial_projection_dep_var = [[] for _ in range(len(main_clauses))]
    the_non_trivial_projections = [[] for _ in range(len(main_clauses))]
    for key in existential:
        q=[]
        for idx, clause in enumerate(main_clauses):
            new_clause = [x for x in clause if abs(x) in dependencies[key]]
            if key in clause:
                new_clause.append(key)
            if -key in clause:
                new_clause.append(-key)
    
            if len(new_clause)!=0:
                q.append(new_clause)
        
        if len(q)==0:
            q.append(list({key, -key}))
        phi_dict[key]=q
    

    for key in existential:
        this_phi = phi_dict[key]
        q=[]
        for clause in this_phi:
            new_clause=clause.copy()
            if(int(key) in new_clause):
                continue
            if(-int(key) in new_clause):
                new_clause.remove(-int(key))
            
            if len(new_clause)==0:
                q.append(list({key}))
                q.append(list({-key}))
                break
            q.append(new_clause)
        
        output_dir = os.path.join(os.getcwd(),f"data/{dimacs_file_main}/phi_1")
        os.makedirs(output_dir,exist_ok=True)
        phi_1_file = os.path.join(output_dir,f"phi{key}.dimacs")

        write_dimacs(numInputs,q,phi_1_file)

        q2=[]
        for clause in this_phi:
            new_clause=clause.copy()
            if(-int(key) in new_clause):
                continue
            if(int(key) in new_clause):
                new_clause.remove(int(key))
            
            if len(new_clause)==0:
                q2.append(list({key}))
                q2.append(list({-key}))
            q2.append(new_clause)
        output_dir = os.path.join(os.getcwd(),f"data/{dimacs_file_main}/phi_0")
        os.makedirs(output_dir,exist_ok=True)
        phi_0_file = os.path.join(output_dir,f"phi{key}.dimacs")
        
        write_dimacs(numInputs,q2,phi_0_file)
    
    # Write the config file
    data = {
        "numX": len(quantifiers),
        "numY": len(existential),
        "dependencies": dependencies
    }

    output_dir = os.path.join(os.getcwd(),f"data/{dimacs_file_main}")
    os.makedirs(output_dir,exist_ok=True)
    output_file = os.path.join(output_dir,f"{dimacs_file_main}.json")

    with open(output_file,'w') as f:
        json.dump(data,f,indent=4)
    
    print(f"JSON Config file created: '{output_file}'")

    
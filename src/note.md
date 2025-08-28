# TODO
- unit projected clauses cause the formula to be trivially 0.
- Check if in the condition wher A and B os npt rtrivillu 0 and 1, are we able to filer out some cases.
- We want A and B to be such that:
    -   club projection instrad of single output, consider multople outputs.
    - get the local skolem 

- Check on the relation unsatCore and A=0 and B=1 cases

## Issue with Universal Variable UnsatCore Utilization
- At any iteration, not all variables in the depSet part of UnsatCore
- Results in an implication based on incomplete knowledge
- At some other point, the variables not in unsatCore earlier are now in the unsatCore
- Another implication as a result of this
- Now two auxilary variables, one for each subset of dependency set are formed.
- No clause to encode relationship between these clauses
- End up running too many iterations to find the relationship between these auxilaries.
- For example
    - var 5 DepSet = {1,2}
    - in iter 1, 1 => h_5 == v_{1,5}
    - in iter 2, -1 => h_5 == v_{-1,5}
    - in iter 3, 2 => h_5 == v_{2,5}
    - in iter 4, -2 => h_5 == v_{-2,5}
    - Now we need to find what happens when assignment is {1,2}, {1,-2}, {-1,-2} etc.
        - This requires knowing the relationship b/w the auxilaries
        - results in 2^|depset| implications in worst case
        Hypothesis: Finding out the relationship b/w auxilaries blows up the iteration count.
- In earlier case:
    - All implications complete by themselves
    - Implying, the auxilaries are independent (w.r.t. a single d-var)
    - Saves time of searching up the reln b/w intra-aux vars
    - worst case implication count still same => 2^|depset|

### UPDATE: Infinite Loop error solution
- Currently the ITE is modelled such that only the first true "IF" hit is acknowledged.
- If <condition> then <h==aux_i> else {if <condition> then ...}
    - We consider other IF branches only when the preceeding  "newly added" branch is FALSE.
    - Need to evaluate othe IF branches when the preceeding branches hit TRUE.

- New Proposed ITE =>
    (<asg_1> AND h==v_1) OR (<asg_2> AND h==v_2) OR ...

## Minimum Hitting Set
    - Pairwise Intersection of Dependency Sets
    - Consider the pairwise intersection for all d-variables in the unsatCore
    - Find the MHS for these Intersection Sets
    - Ignore empty intersections
    - Extend the unsat Core a-variables with the MHS
    - Use this new unsatCore to
// Benchmark "aig_wrapper_netlist" written by ABC on Thu Apr  2 12:33:54 2026

module aig_wrapper_netlist ( 
    pi0, pi1, pi2, pi3, pi4,
    po0  );
  input  pi0, pi1, pi2, pi3, pi4;
  output po0;
  wire new_n7, new_n8, new_n9, new_n10, new_n11, new_n12;
  assign new_n7 = ~pi2 & ~pi3;
  assign new_n8 = pi2 & pi3;
  assign new_n9 = ~new_n7 & ~new_n8;
  assign new_n10 = ~pi1 & ~pi4;
  assign new_n11 = new_n9 & ~new_n10;
  assign new_n12 = pi1 & pi4;
  assign po0 = new_n11 & ~new_n12;
endmodule



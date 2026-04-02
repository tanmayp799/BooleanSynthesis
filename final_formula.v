// Benchmark "aig_wrapper_netlist" written by ABC on Tue Mar 31 14:34:00 2026

module aig_wrapper_netlist ( 
    pi0, pi1, pi2, pi3, pi4,
    po0  );
  input  pi0, pi1, pi2, pi3, pi4;
  output po0;
  wire new_n7, new_n8, new_n9, new_n10, new_n11, new_n12, new_n13, new_n14,
    new_n15, new_n16, new_n17;
  assign new_n7 = ~pi3 & ~pi4;
  assign new_n8 = pi3 & pi4;
  assign new_n9 = ~new_n7 & ~new_n8;
  assign new_n10 = ~pi1 & ~pi4;
  assign new_n11 = new_n9 & ~new_n10;
  assign new_n12 = pi1 & pi4;
  assign new_n13 = new_n11 & ~new_n12;
  assign new_n14 = ~pi2 & new_n12;
  assign new_n15 = new_n13 & ~new_n14;
  assign new_n16 = ~pi1 & pi2;
  assign new_n17 = ~pi4 & new_n16;
  assign po0 = new_n15 & ~new_n17;
endmodule



module dut #(parameter width_p = "inv")
   (input [width_p-1:0]    i
    , output [width_p-1:0] o
    );
  
   assign o = i*i;
   
endmodule

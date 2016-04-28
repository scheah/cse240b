// Example CSE 240B Testbench
// MBT 3-23-16
//
// Modify to try things out!
//

module tb_nonsynth;
  
   localparam width_lp = 16;
   localparam cycle_time_lp = 10.0;
  
   logic [width_lp-1:0] ctr, val_out;
   logic 		clk, reset;
  
   // generate clock
   bsg_nonsynth_clock_gen #(.cycle_time_p(cycle_time_lp)) cg (.o(clk));

   // generate reset
   bsg_nonsynth_reset_gen 
     #(.reset_cycles_lo_p(5)
       ,.reset_cycles_hi_p(5)
       ) 
   rg (.clk_i(clk)
       ,.async_reset_o(reset)
       );

   /* synthesizeable code */
  
   // cycle counter to generate some input data
   bsg_cycle_counter #(.width_p(width_lp),.init_val_p(1))
   bcc (.clk(clk)
	,.reset_i(reset)	
	,.ctr_r_o(ctr)
	);
  
   // instantiate the design
   dut #(.width_p(width_lp))
   my_dut (
           .i(ctr)
           ,.o(val_out)
           );

   /* end synthesizeable code */

  
   // print outputs; trigger on negedge to avoid race conditions
   // nonsynthesizeable
   always_ff @(negedge clk)
     begin	
	$display(val_out);
	if (ctr == 100) $finish;
     end
  
endmodule
  

module bsg_manycore_pkt_encode #(
                                 x_cord_width_p   = "inv"
                                 , y_cord_width_p = "inv"
                                 , data_width_p   = "inv"
                                 , addr_width_p   = "inv"
                                 , packet_width_lp = 6+x_cord_width_p+y_cord_width_p+data_width_p+addr_width_p
                                 , debug_p=0
                                 )
   (
    input clk_i // for debug only
    ,input v_i
    ,input [addr_width_p-1:0] addr_i
    ,input [data_width_p-1:0] data_i
    ,input [(data_width_p>>3)-1:0] mask_i
    ,input we_i
    ,output v_o
    ,output [packet_width_lp-1:0] data_o
	,output magic_o // Yeseong: Magic load instruction
    );

   typedef struct packed {
      logic       remote;
      logic [y_cord_width_p-1:0] y_cord;
      logic [x_cord_width_p-1:0] x_cord;
      logic [addr_width_p-y_cord_width_p-x_cord_width_p-2:0] addr;
   } addr_decode_s;

   typedef struct packed {
      logic [5:0] op;
      logic [addr_width_p-1:0] addr;
      logic [data_width_p-1:0] data;
      logic [y_cord_width_p-1:0] y_cord;
      logic [x_cord_width_p-1:0] x_cord;
   } bsg_manycore_packet_s;

   bsg_manycore_packet_s pkt;
   addr_decode_s addr_decode;

   assign addr_decode = addr_i;
   assign data_o = pkt;

   assign pkt.op     = 6 ' (addr_decode.remote);
   assign pkt.addr   = addr_width_p ' (addr_decode.addr);
   assign pkt.data   = data_i;
   assign pkt.x_cord = addr_decode.x_cord;
   assign pkt.y_cord = addr_decode.y_cord;

   //assign v_o = addr_decode.remote & we_i & v_i;

   // Yeseong: Supporting magic address load
   logic magic_on;
   assign magic_on = addr_decode.remote & ~we_i & (addr_decode.addr == 20'hABCD_0);
   assign v_o = addr_decode.remote & we_i & v_i & ~magic_on;
   assign magic_o = magic_on;
   ///////////////////////////////////////



   // synopsys translate off
   if (debug_p)
   always @(negedge clk_i)
     if (v_i)
       $display("%m encode pkt addr_i=%x data_i=%x mask_i=%x we_i=%x v_o=%x, data_o=%x, remote=%x",
                addr_i, data_i, mask_i, we_i, v_o, data_o, addr_decode.remote, $bits(addr_decode_s));

   always_ff @(negedge clk_i)
     begin
        //if (addr_decode.remote & ~we_i & v_i) // Yeseong
        if (addr_decode.remote & ~we_i & v_i & ~magic_on) // Yeseong
          begin
             $error("%m load to remote address %x", addr_i);
             $finish();
          end
        if (magic_on) // Yeseong
			begin
             $display("Magic Found %x. load the target value instead", addr_i);
			end
        if (addr_decode.remote & we_i & v_i & (|addr_i[1:0]))
          begin
             $error ("%m store to remote unaligned address %x", addr_i);
          end
        if (addr_decode.remote & we_i & v_i & (|mask_i))
          begin
             $error ("%m store to remote addr %x unsupported mask %x", addr_i, mask_i);
          end
     end
   // synopsys translate on

endmodule
`include "bsg_manycore_packet.vh"

module bsg_manycore_proc #(x_cord_width_p   = "inv"
                           , y_cord_width_p = "inv"
                           , data_width_p   = 32
                           , addr_width_p   = 32
                           , packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)

                           , debug_p        = 0
                           , bank_size_p    = 2048 // in words
                           , num_banks_p    = 4

                           // this is the size of the receive FIFO
                           , proc_fifo_els_p = 4
                           , mem_width_lp    = $clog2(bank_size_p) + $clog2(num_banks_p)
                           )
   (input   clk_i
    , input reset_i

    , input v_i
    , input [packet_width_lp-1:0] data_i
    , output ready_o

    , output v_o
    , output [packet_width_lp-1:0] data_o
    , input ready_i

    // tile coordinates
    , input   [x_cord_width_p-1:0]                 my_x_i
    , input   [y_cord_width_p-1:0]                 my_y_i

    , output logic freeze_o
    );

   // input fifo from network

   logic cgni_v, cgni_yumi;
   logic [packet_width_lp-1:0] cgni_data;

   // this fifo buffers incoming remote store requests
   // it is a little bigger than the standard twofer to accomodate
   // bank conflicts

   bsg_fifo_1r1w_small #(.width_p(packet_width_lp)
                        ,.els_p (proc_fifo_els_p)
                        ) cgni
     (.clk_i   (clk_i  )
      ,.reset_i(reset_i)

      ,.v_i     (v_i    )
      ,.data_i  (data_i )
      ,.ready_o (ready_o)

      ,.v_o    (cgni_v   )
      ,.data_o (cgni_data)
      ,.yumi_i (cgni_yumi)
      );

   // decode incoming packet
   logic                       pkt_freeze, pkt_unfreeze, pkt_remote_store, pkt_unknown;
   logic [data_width_p-1:0]    remote_store_data;
   logic [(data_width_p>>3)-1:0] remote_store_mask;
   logic [addr_width_p-1:0]    remote_store_addr;
   logic                       remote_store_v, remote_store_yumi;

   if (debug_p)
   always_ff @(negedge clk_i)
     if (v_o)
       $display("%m attempting remote store of data %x, ready_i = %x",data_o,ready_i);

   if (debug_p)
     always_ff @(negedge clk_i)
       if (cgni_v)
         $display("%m data %x avail on cgni (cgni_yumi=%x,remote_store_v=%x, remote_store_addr=%x, remote_store_data=%x, remote_store_yumi=%x)",cgni_data,cgni_yumi,remote_store_v,remote_store_addr, remote_store_data, remote_store_yumi);

   bsg_manycore_pkt_decode #(.x_cord_width_p (x_cord_width_p)
                             ,.y_cord_width_p(y_cord_width_p)
                             ,.data_width_p  (data_width_p )
                             ,.addr_width_p  (addr_width_p )
                             ) pkt_decode
     (.v_i                 (cgni_v)
      ,.data_i             (cgni_data)
      ,.pkt_freeze_o       (pkt_freeze)
      ,.pkt_unfreeze_o     (pkt_unfreeze)
      ,.pkt_unknown_o      (pkt_unknown)

      ,.pkt_remote_store_o (remote_store_v)
      ,.data_o             (remote_store_data)
      ,.addr_o             (remote_store_addr)
      ,.mask_o             (remote_store_mask)
      );

   // deque if we successfully do a remote store, or if it's
   // either kind of packet freeze instruction
   assign cgni_yumi = remote_store_yumi | pkt_freeze | pkt_unfreeze;

   // create freeze gate
   logic                       freeze_r;
   assign freeze_o = freeze_r;

   always_ff @(posedge clk_i)
     if (reset_i)
       freeze_r <= 1'b1;
     else
       if (pkt_freeze | pkt_unfreeze)
         begin
            $display("## freeze_r <= %x",pkt_freeze);
            freeze_r <= pkt_freeze;
         end

   logic [1:0]                  core_mem_v;
   logic [1:0]                  core_mem_w;
   logic [1:0] [addr_width_p-1:0] core_mem_addr;
   logic [1:0] [data_width_p-1:0] core_mem_wdata;
   logic [1:0] [(data_width_p>>3)-1:0] core_mem_mask;
   logic [1:0]                         core_mem_yumi;
   logic [1:0]                         core_mem_rv;
   logic [1:0] [data_width_p-1:0]      core_mem_rdata;

   // Yeseong - Hooking magic address of load instruction
   logic v_magic;
   logic hooked_v_for_encoder;
   logic [1:0]                         hooked_core_mem_rv;
   logic [1:0] [data_width_p-1:0]      hooked_core_mem_rdata;

   bsg_manycore_magic_load #(.magic_num_p (2)
							 ,.x_cord_width_p (x_cord_width_p)
                             ,.y_cord_width_p (y_cord_width_p)
                             ,.data_width_p (data_width_p)
                             ,.addr_width_p (addr_width_p)
							 ,.debug_p (1)
                             ) load_hooker
	(
	.clk_i   (clk_i)
	,.reset_i (reset_i)

	,.magic_ref_addr_i({28'hABCD_0, 28'hBCDE_0}) // # of magic_num_p
	,.magic_data_i({32'hF0E0_D0C0, 32'h1020_3040})

	,.v_i    (core_mem_v    [1])
	,.addr_i (core_mem_addr [1])
	,.we_i   (core_mem_w    [1])

	,.core_mem_rv_i(core_mem_rv)
	,.core_mem_rdata_i(core_mem_rdata)

	,.v_magic_o(v_magic)
	,.hooked_v_for_encoder_o(hooked_v_for_encoder)
	,.hooked_core_mem_rv_o(hooked_core_mem_rv)
	,.hooked_core_mem_rdata_o(hooked_core_mem_rdata)
	);
	// End of Hooker ///////////////////////////////////////////////


   bsg_vscale_core #(.x_cord_width_p (x_cord_width_p)
                     ,.y_cord_width_p(y_cord_width_p)
                     )
            core
     ( .clk_i   (clk_i)
       ,.reset_i (reset_i)
       ,.freeze_i (freeze_r)

       ,.m_v_o       (core_mem_v)
       ,.m_w_o       (core_mem_w)
       ,.m_addr_o    (core_mem_addr)
       ,.m_data_o    (core_mem_wdata)
       ,.m_mask_o    (core_mem_mask)

       // for data port (1), either the network or the banked memory can
       // deque the item.
       //,.m_yumi_i    ({(v_o & ready_i) | core_mem_yumi[1]
       //                , core_mem_yumi[0]})
       //,.m_v_i       (core_mem_rv)
       //,.m_data_i    (core_mem_rdata)

	   // Yeseong - Use the hooked result
       ,.m_yumi_i    ({(v_o & ready_i) | core_mem_yumi[1]  | v_magic
                       , core_mem_yumi[0]})
       ,.m_v_i       (hooked_core_mem_rv)
       ,.m_data_i    (hooked_core_mem_rdata)

       ,.my_x_i (my_x_i)
       ,.my_y_i (my_y_i)
       );

   bsg_manycore_pkt_encode #(.x_cord_width_p (x_cord_width_p)
                             ,.y_cord_width_p(y_cord_width_p)
                             ,.data_width_p (data_width_p )
                             ,.addr_width_p (addr_width_p )
                             ) pkt_encode
     (.clk_i(clk_i)

      // the memory request, from the core's data memory port
      //,.v_i    (core_mem_v    [1])
      ,.v_i    (hooked_v_for_encoder) // Yeseong
      ,.data_i (core_mem_wdata[1])
      ,.addr_i (core_mem_addr [1])
      ,.we_i   (core_mem_w    [1])
      ,.mask_i (core_mem_mask [1])

      // directly out to the network!
      ,.v_o    (v_o   )
      ,.data_o (data_o)
      );

   // synopsys translate off

   `declare_bsg_manycore_packet_s(addr_width_p, data_width_p, x_cord_width_p, y_cord_width_p);

   bsg_manycore_packet_s data_o_debug;
   assign data_o_debug = data_o;

   if (debug_p)
     always @(negedge clk_i)
       begin
          if (v_o & ready_o)
            $display("proc sending packet %x (op=%x, addr=%x, data=%x, y_cord=%x, x_cord=%x), bit_mask=%x, core_mem_wdata=%x, core_mem_addr=%x"
                     , data_o_debug
                     , data_o_debug.op
                     , data_o_debug.addr
                     , data_o_debug.data
                     , data_o_debug.y_cord
                     , data_o_debug.x_cord
                     , core_mem_mask [1]
                     , core_mem_wdata[1]
                     , core_mem_addr [1]
                     );
       end

   // synopsys translate on

   wire [data_width_p-1:0] unused_data;
   wire                    unused_valid;

   // we create dedicated signals for these wires to allow easy access for "bind" statements
   wire [2:0]              xbar_port_v_in = { remote_store_v
                                              // request to write only if we are not sending a remote store packet
                                              // we check the high bit only for performance
                                              , core_mem_v[1] & ~core_mem_addr[1][31]
                                              , core_mem_v[0]
                                              };
   wire [2:0]                    xbar_port_we_in   = {1'b1, core_mem_w[1], 1'b0};
   wire [2:0]                    xbar_port_yumi_out;
   wire [2:0] [data_width_p-1:0] xbar_port_data_in = {remote_store_data,    core_mem_wdata};
   wire [2:0] [mem_width_lp-1:0] xbar_port_addr_in = { remote_store_addr  [2+:mem_width_lp]
                                                       , core_mem_addr[1] [2+:mem_width_lp]
                                                       , core_mem_addr[0] [2+:mem_width_lp]
                                                       };
   wire [2:0] [(data_width_p>>3)-1:0] xbar_port_mask_in = { remote_store_mask, core_mem_mask };

   always @(negedge clk_i)
     if (0)
     begin
	if (~freeze_r)
	  $display("x=%x y=%x xbar_v_i=%b xbar_w_i=%b xbar_port_yumi_out=%b xbar_addr_i[2,1,0]=%x,%x,%x, xbar_data_i[2,1,0]=%x,%x,%x, xbar_data_o[1,0]=%x,%x"
		   ,my_x_i
		   ,my_y_i
		   ,xbar_port_v_in
		   ,xbar_port_we_in
		   ,xbar_port_yumi_out
		   ,xbar_port_addr_in[2]*4,xbar_port_addr_in[1]*4,xbar_port_addr_in[0]*4
		   ,xbar_port_data_in[2], xbar_port_data_in[1], xbar_port_data_in[0]
		   ,core_mem_rdata[1], core_mem_rdata[0]
		   );
     end

   
   assign {remote_store_yumi, core_mem_yumi } = xbar_port_yumi_out;

  bsg_mem_banked_crossbar #
    ( .num_ports_p  (3)
     ,.num_banks_p  (num_banks_p)
     ,.bank_size_p  (bank_size_p)
     ,.data_width_p (data_width_p)
      ,.debug_p(debug_p*4)  // mbt: debug, multiply addresses by 4.
//      ,.debug_p(4)
//     ,.debug_reads_p(0)
    ) banked_crossbar
    ( .clk_i   (clk_i)
     ,.reset_i (reset_i)
      ,.v_i    (xbar_port_v_in)
      // the network port always writes, proc data port sometimes writes, proc inst port never writes
      ,.w_i     (xbar_port_we_in)
      ,.addr_i  (xbar_port_addr_in)
      ,.data_i  (xbar_port_data_in)
      ,.mask_i  (xbar_port_mask_in)

      // whether the crossbar accepts the input
     ,.yumi_o  (xbar_port_yumi_out)
     ,.v_o     ({unused_valid,      core_mem_rv      })
     ,.data_o  ({unused_data,       core_mem_rdata   })
    );




endmodule


// Yeseong: Hook load instruction & generate the data ///////////////////////////
module bsg_manycore_magic_load #(
                                 magic_num_p    = 1
                                 , x_cord_width_p   = "inv"
                                 , y_cord_width_p = "inv"
                                 , data_width_p = "inv"
                                 , addr_width_p = "inv"

                                 , debug_p=0
								)
   (input   clk_i
    , input reset_i

	// Magic address reference signal
	, input [magic_num_p-1:0][addr_width_p-y_cord_width_p-x_cord_width_p-2:0] magic_ref_addr_i
	, input [magic_num_p-1:0][data_width_p-1:0] magic_data_i

	// address of instruction from core
	, input v_i
    , input [addr_width_p-1:0] addr_i
	, input we_i

	// hooking for memory bank
	, input [1:0] core_mem_rv_i
	, input [1:0][data_width_p-1:0] core_mem_rdata_i

	// output signal to hook
	, output v_magic_o
	, output hooked_v_for_encoder_o // Valid signal to encoder
	, output [1:0]						hooked_core_mem_rv_o
	, output [1:0][data_width_p-1:0]	hooked_core_mem_rdata_o
	);

	// Requested address
	// NOTE: It must be same to the structure of the encoder (see bsg_manycore_pkt_encode)
	typedef struct packed {
      logic       remote;
      logic [y_cord_width_p-1:0] y_cord;
      logic [x_cord_width_p-1:0] x_cord;
      logic [addr_width_p-y_cord_width_p-x_cord_width_p-2:0] addr;
	} addr_decode_s;

	addr_decode_s addr_decode;
	assign addr_decode = addr_i;

	// Check if there's matched magic reference address
	genvar i;
	logic [magic_num_p-1:0] magic_on;
	for (i = 0; i < magic_num_p; i += 1)
	begin
		assign magic_on[i] = addr_decode.remote & v_i & ~we_i & (addr_decode.addr == magic_ref_addr_i[i]);
	end

	logic any_magic;
	always_comb
	begin
		any_magic = 1'b0;
		for (int j = 0; j < magic_num_p; j += 1)
		begin
			any_magic |= magic_on[j];
		end
	end

	// Filp flop to mimic the cycle behavior of the crossbank memory
	logic [magic_num_p-1:0] magic_load_r;
	logic [magic_num_p-1:0] magic_load_n;
	assign magic_load_n = magic_on;

	always_ff @(posedge clk_i)
		begin
		if (reset_i)
			magic_load_r <= {magic_num_p{1'b0}};
		else
			magic_load_r <= magic_load_n;
	end

	// Output manipulation
	assign v_magic_o = any_magic & v_i;
	assign hooked_v_for_encoder_o = v_i & ~any_magic;
	
	logic [1:0]						hooked_core_mem_rv;
	logic [1:0][data_width_p-1:0]	hooked_core_mem_rdata;
	assign hooked_core_mem_rv[0]		= core_mem_rv_i[0]; // Instruction memory
	assign hooked_core_mem_rdata[0]		= core_mem_rdata_i[0];
	always_comb // Select the matched magic data and send it (in the next cycle) 
	begin
		hooked_core_mem_rv[1]		= core_mem_rv_i[1];
		hooked_core_mem_rdata[1]	= core_mem_rdata_i[1];

		for (int j = 0; j < magic_num_p; j += 1) 
		begin
			if (magic_load_r == (1 << j))
			begin
				hooked_core_mem_rv[1]		= 1'b1;
				hooked_core_mem_rdata[1]	= magic_data_i[j];
			end
		end
	end
	assign hooked_core_mem_rv_o = hooked_core_mem_rv;
	assign hooked_core_mem_rdata_o = hooked_core_mem_rdata;

	// non synthesizable, used for debug
	if (debug_p)
	always_ff @(negedge clk_i)
	begin
		//if (any_magic | ~(magic_load_r == 2'b00) )
		if (any_magic)
		begin
			$display("Magic Found %x. load the target value instead", addr_i);
			//$display("Magic: %b %b -> %x %x -> %x %x",
			//		magic_load_r, magic_load_n, v_magic_o, hooked_v_for_encoder_o,
			//		hooked_core_mem_rv[1], hooked_core_mem_rdata[1]);
		end
	end
   // end of non synthesizable part
endmodule
///////////////////// END OF HOOKING MODULE /////////////////////////////////

#!/usr/bin/env sysbench

-- -----------------------------------------------------------------------------
-- COPY-based PREPARE and CLEANUP for TPCC-like workload with sysbench (PostgreSQL Optimized)
-- -----------------------------------------------------------------------------

ffi = require("ffi")

-- Default Arguments
MAXITEMS = 100000
DIST_PER_WARE = 10
CUST_PER_DIST = 3000

-- Command line options
sysbench.cmdline.options = {
   scale = {"Scale factor (warehouses)", 1},
   tables = {"Number of tables", 1},
   use_fk = {"Use foreign keys", 1},
   pgsql_schema = {"Schema name for Pg(default:public)", "public"},
   trx_level = {"Transaction isolation level (RC, RR or SER)", "RR"}
}

-- ---------------------------------------------------------
-- Utility Functions
-- ---------------------------------------------------------

function get_psql_cmd()
   local opt = sysbench.opt
   local cmd = string.format("PGPASSWORD='%s' psql -h '%s' -p '%d' -U '%s' -d '%s' -q -c ",
      opt.pgsql_password, opt.pgsql_host, opt.pgsql_port, opt.pgsql_user, opt.pgsql_db)
   return cmd
end

function pipe_to_psql(sql)
   local full_cmd = get_psql_cmd() .. '"' .. sql .. '"'
   local pipe = io.popen(full_cmd, "w")
   return pipe
end

function Lastname(num)
   local n = {"BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE", "ANTI", "CALLY", "ATION", "EING"}
   return n[math.floor(num / 100) + 1] .. n[math.floor(num / 10) % 10 + 1] .. n[num % 10 + 1]
end

local init_rand = 1
local C_255, C_1023, C_8191

function NURand(A, x, y)
   if init_rand == 1 then
      C_255 = sysbench.rand.uniform(0, 255)
      C_1023 = sysbench.rand.uniform(0, 1023)
      C_8191 = sysbench.rand.uniform(0, 8191)
      init_rand = 0
   end
   local C = (A == 255) and C_255 or ((A == 1023) and C_1023 or C_8191)
   return (((bit.bor(sysbench.rand.uniform(0, A), sysbench.rand.uniform(x, y))) + C) % (y - x + 1)) + x
end

-- ---------------------------------------------------------
-- Schema Creation (Tables Only)
-- ---------------------------------------------------------

function create_tables(table_num)
   print(string.format("Creating tables for set %d...", table_num))
   local drv = sysbench.sql.driver()
   local con = drv:connect()
   
   -- We create tables without indexes first for fast loading
   con:query(string.format("CREATE TABLE IF NOT EXISTS warehouse%d (w_id smallint not null, w_name text, w_street_1 varchar(20), w_street_2 varchar(20), w_city varchar(20), w_state char(2), w_zip char(9), w_tax float4, w_ytd float4)", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS district%d (d_id smallint not null, d_w_id smallint not null, d_name varchar(10), d_street_1 varchar(20), d_street_2 varchar(20), d_city varchar(20), d_state char(2), d_zip char(9), d_tax float4, d_ytd float4, d_next_o_id int)", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS customer%d (c_id int not null, c_d_id smallint not null, c_w_id smallint not null, c_first varchar(16), c_middle char(2), c_last varchar(16), c_street_1 varchar(20), c_street_2 varchar(20), c_city varchar(20), c_state char(2), c_zip char(9), c_phone char(16), c_since timestamp DEFAULT CURRENT_TIMESTAMP, c_credit char(2), c_credit_lim int4, c_discount float4, c_balance float4, c_ytd_payment float4, c_payment_cnt int4, c_delivery_cnt int4, c_data text)", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS history%d (h_c_id int, h_c_d_id smallint, h_c_w_id smallint, h_d_id smallint, h_w_id smallint, h_date timestamp DEFAULT CURRENT_TIMESTAMP, h_amount float4, h_data varchar(24))", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS orders%d (o_id int not null, o_d_id smallint not null, o_w_id smallint not null, o_c_id int, o_entry_d timestamp DEFAULT CURRENT_TIMESTAMP, o_carrier_id smallint, o_ol_cnt int4, o_all_local int4)", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS new_orders%d (no_o_id int not null, no_d_id smallint not null, no_w_id smallint not null)", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS order_line%d (ol_o_id int not null, ol_d_id smallint not null, ol_w_id smallint not null, ol_number int4 not null, ol_i_id int, ol_supply_w_id smallint, ol_delivery_d timestamp, ol_quantity int4, ol_amount float4, ol_dist_info char(24))", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS stock%d (s_i_id int not null, s_w_id smallint not null, s_quantity int4, s_dist_01 char(24), s_dist_02 char(24), s_dist_03 char(24), s_dist_04 char(24), s_dist_05 char(24), s_dist_06 char(24), s_dist_07 char(24), s_dist_08 char(24), s_dist_09 char(24), s_dist_10 char(24), s_ytd float4, s_order_cnt int4, s_remote_cnt int4, s_data varchar(50))", table_num))
   con:query(string.format("CREATE TABLE IF NOT EXISTS item%d (i_id int not null, i_im_id int, i_name varchar(24), i_price float4, i_data varchar(50))", table_num))
   
   con:disconnect()
end

-- ---------------------------------------------------------
-- Data Loading via COPY Pipeline
-- ---------------------------------------------------------

function load_data(table_num, w_id)
   print(string.format("Thread %d loading Warehouse %d for Table Set %d...", sysbench.tid, w_id, table_num))

   -- ITEM table (only load once per table set, e.g., when w_id is 1)
   if w_id == 1 then
      local p = pipe_to_psql(string.format("COPY item%d FROM STDIN WITH CSV", table_num))
      for i = 1, MAXITEMS do
         local i_im_id = sysbench.rand.uniform(1, 10000)
         local i_price = sysbench.rand.uniform_double() * 100 + 1
         p:write(string.format("%d,%d,item-%d,%.2f,data-%d\n", i, i_im_id, i, i_price, i))
      end
      p:close()
   end

   -- WAREHOUSE
   local p_wh = pipe_to_psql(string.format("COPY warehouse%d FROM STDIN WITH CSV", table_num))
   p_wh:write(string.format("%d,name-%s,st1-%s,st2-%s,city-%s,ST,%s,%.2f,300000.00\n", 
      w_id, sysbench.rand.string("@@@@"), sysbench.rand.string("@@@@"), sysbench.rand.string("@@@@"), 
      sysbench.rand.string("@@@@"), sysbench.rand.string("#####"), sysbench.rand.uniform_double()*0.2))
   p_wh:close()

   -- DISTRICT
   local p_dist = pipe_to_psql(string.format("COPY district%d FROM STDIN WITH CSV", table_num))
   for d_id = 1, DIST_PER_WARE do
      p_dist:write(string.format("%d,%d,dist-%d,st1-%s,st2-%s,city-%s,ST,%s,%.2f,30000.00,3001\n",
         d_id, w_id, d_id, sysbench.rand.string("@@@@"), sysbench.rand.string("@@@@"), 
         sysbench.rand.string("@@@@"), sysbench.rand.string("#####"), sysbench.rand.uniform_double()*0.2))
   end
   p_dist:close()

   -- CUSTOMER & HISTORY
   local p_cust = pipe_to_psql(string.format("COPY customer%d FROM STDIN WITH CSV", table_num))
   local p_hist = pipe_to_psql(string.format("COPY history%d FROM STDIN WITH CSV", table_num))
   for d_id = 1, DIST_PER_WARE do
      for c_id = 1, CUST_PER_DIST do
         local c_last = (c_id <= 1000) and Lastname(c_id - 1) or Lastname(NURand(255, 0, 999))
         p_cust:write(string.format("%d,%d,%d,first-%d,OE,%s,st1,st2,city,ST,zip,123456,NOW(),%s,50000,%.2f,-10.00,10.00,1,0,data\n",
            c_id, d_id, w_id, c_id, c_last, (sysbench.rand.uniform(1, 100) > 10 and "GC" or "BC"), sysbench.rand.uniform_double()*0.5))
         p_hist:write(string.format("%d,%d,%d,%d,%d,NOW(),10.00,hist-data\n", c_id, d_id, w_id, d_id, w_id))
      end
   end
   p_cust:close()
   p_hist:close()

   -- STOCK
   local p_stock = pipe_to_psql(string.format("COPY stock%d FROM STDIN WITH CSV", table_num))
   for s_id = 1, MAXITEMS do
      p_stock:write(string.format("%d,%d,%d,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,0,0,0,s-data\n", 
         s_id, w_id, sysbench.rand.uniform(10, 100)))
   end
   p_stock:close()

   -- ORDERS & ORDER_LINE & NEW_ORDERS
   local p_ord = pipe_to_psql(string.format("COPY orders%d FROM STDIN WITH CSV", table_num))
   local p_ol = pipe_to_psql(string.format("COPY order_line%d FROM STDIN WITH CSV", table_num))
   
   -- Shuffle for orders
   local tab = {}
   for i=1, CUST_PER_DIST do tab[i] = i end
   for i=1, CUST_PER_DIST do 
      local j = math.random(i, CUST_PER_DIST)
      tab[i], tab[j] = tab[j], tab[i]
   end

   for d_id = 1, DIST_PER_WARE do
      for o_id = 1, CUST_PER_DIST do
         local ol_cnt = sysbench.rand.uniform(5, 15)
         local carrier = (o_id < 2101) and sysbench.rand.uniform(1, 10) or ""
         p_ord:write(string.format("%d,%d,%d,%d,NOW(),%s,%d,1\n", o_id, d_id, w_id, tab[o_id], carrier, ol_cnt))
         
         for ol = 1, ol_cnt do
            local amount = (o_id < 2101) and 0 or sysbench.rand.uniform_double()*999.99
            p_ol:write(string.format("%d,%d,%d,%d,%d,%d,NOW(),5,%.2f,dist-info\n",
               o_id, d_id, w_id, ol, sysbench.rand.uniform(1, MAXITEMS), w_id, amount))
         end
      end
   end
   p_ord:close()
   p_ol:close()

   -- NEW ORDERS (Batch Insert via SQL is fast enough for this subset)
   local drv = sysbench.sql.driver()
   local con = drv:connect()
   con:query(string.format("INSERT INTO new_orders%d (no_o_id, no_d_id, no_w_id) SELECT o_id, o_d_id, o_w_id FROM orders%d WHERE o_id > 2100 AND o_w_id = %d", table_num, table_num, w_id))
   con:disconnect()
end

-- ---------------------------------------------------------
-- Post-Load: Create Indexes and FKs
-- ---------------------------------------------------------

function create_indexes(table_num)
   print(string.format("Creating indexes and constraints for set %d...", table_num))
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   local cmds = {
      "ALTER TABLE warehouse" .. table_num .. " ADD PRIMARY KEY (w_id)",
      "ALTER TABLE district" .. table_num .. " ADD PRIMARY KEY (d_w_id, d_id)",
      "ALTER TABLE customer" .. table_num .. " ADD PRIMARY KEY (c_w_id, c_d_id, c_id)",
      "ALTER TABLE orders" .. table_num .. " ADD PRIMARY KEY (o_w_id, o_d_id, o_id)",
      "ALTER TABLE new_orders" .. table_num .. " ADD PRIMARY KEY (no_w_id, no_d_id, no_o_id)",
      "ALTER TABLE order_line" .. table_num .. " ADD PRIMARY KEY (ol_w_id, ol_d_id, ol_o_id, ol_number)",
      "ALTER TABLE stock" .. table_num .. " ADD PRIMARY KEY (s_w_id, s_i_id)",
      "ALTER TABLE item" .. table_num .. " ADD PRIMARY KEY (i_id)",
      "CREATE INDEX idx_customer" .. table_num .. " ON customer" .. table_num .. " (c_w_id, c_d_id, c_last, c_first)",
      "CREATE INDEX idx_orders" .. table_num .. " ON orders" .. table_num .. " (o_w_id, o_d_id, o_c_id, o_id)"
   }

   for _, sql in ipairs(cmds) do con:query(sql) end

   if sysbench.opt.use_fk == 1 then
      local fks = {
         "ALTER TABLE district" .. table_num .. " ADD CONSTRAINT fk_dist_wh FOREIGN KEY (d_w_id) REFERENCES warehouse" .. table_num .. "(w_id)",
         "ALTER TABLE customer" .. table_num .. " ADD CONSTRAINT fk_cust_dist FOREIGN KEY (c_w_id, c_d_id) REFERENCES district" .. table_num .. "(d_w_id, d_id)",
         "ALTER TABLE orders" .. table_num .. " ADD CONSTRAINT fk_ord_cust FOREIGN KEY (o_w_id, o_d_id, o_c_id) REFERENCES customer" .. table_num .. "(c_w_id, c_d_id, c_id)"
      }
      for _, sql in ipairs(fks) do con:query(sql) end
   end

   con:disconnect()
end

-- ---------------------------------------------------------
-- Commands: Prepare & Cleanup
-- ---------------------------------------------------------

function cmd_prepare()
   local opt = sysbench.opt
   
   -- Step 1: Create Tables (Master thread only)
   if sysbench.tid == 0 then
      for i = 1, opt.tables do create_tables(i) end
   end

   -- Step 2: Load Data (Parallel)
   -- Wait briefly for tables to exist
   if sysbench.tid ~= 0 then os.execute("sleep 1") end

   for t = 1, opt.tables do
      for w = 1, opt.scale do
         if (w - 1) % sysbench.opt.threads == sysbench.tid then
            load_data(t, w)
         end
      end
   end

   -- Step 3: Create Indexes (Master thread only, after loading)
   -- In a real sysbench environment, threads finish at different times.
   -- This index step should ideally be run as a separate command or after a barrier.
   -- For this script, we output a message.
   if sysbench.tid == 0 then
      print("Data loading finished. Starting indexing...")
      for i = 1, opt.tables do create_indexes(i) end
      print("Prepare finished.")
   end
end

function cmd_cleanup()
   local drv = sysbench.sql.driver()
   local con = drv:connect()
   for i = 1, sysbench.opt.tables do
      print(string.format("Dropping tables for set %d...", i))
      local tabs = {"history", "new_orders", "order_line", "orders", "customer", "district", "stock", "item", "warehouse"}
      for _, t in ipairs(tabs) do
         con:query(string.format("DROP TABLE IF EXISTS %s%d CASCADE", t, i))
      end
   end
   con:disconnect()
end

sysbench.cmdline.commands = {
   prepare = {cmd_prepare, sysbench.cmdline.PARALLEL_COMMAND},
   cleanup = {cmd_cleanup, sysbench.cmdline.PARALLEL_COMMAND}
}
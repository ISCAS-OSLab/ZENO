DROP DATABASE IF EXISTS secure_kv;
CREATE DATABASE secure_kv;
\c secure_kv

DROP TABLE IF EXISTS nation;
DROP TABLE IF EXISTS region;
DROP TABLE IF EXISTS part;
DROP TABLE IF EXISTS supplier;
DROP TABLE IF EXISTS partsupp;
DROP TABLE IF EXISTS orders;
DROP TABLE IF EXISTS customer;
DROP TABLE IF EXISTS lineitem;

DROP EXTENSION IF EXISTS kvdb CASCADE;
CREATE EXTENSION kvdb;

CREATE TABLE nation  ( n_nationkey  INTEGER NOT NULL,
                       n_name       kv_text NOT NULL,
                       n_regionkey  INTEGER NOT NULL,
                       n_comment    kv_text);

CREATE TABLE region  ( r_regionkey  INTEGER NOT NULL,
                       r_name       kv_text NOT NULL,
                       r_comment    kv_text);

CREATE TABLE part  ( p_partkey     INTEGER NOT NULL,
                     p_name        kv_text NOT NULL,
                     p_mfgr        kv_text NOT NULL,
                     p_brand       kv_text NOT NULL,
                     p_type        kv_text NOT NULL,
                     p_size        kv_int4 NOT NULL,
                     p_container   kv_text NOT NULL,
                     p_retailprice kv_float4 NOT NULL,
                     p_comment     kv_text NOT NULL );

CREATE TABLE supplier ( s_suppkey     INTEGER NOT NULL,
                        s_name        kv_text NOT NULL,
                        s_address     kv_text NOT NULL,
                        s_nationkey   INTEGER NOT NULL,
                        s_phone       kv_text NOT NULL,
                        s_acctbal     kv_float4 NOT NULL,
                        s_comment     kv_text NOT NULL);

CREATE TABLE partsupp ( ps_partkey     INTEGER NOT NULL,
                        ps_suppkey     INTEGER NOT NULL,
                        ps_availqty    kv_float4 NOT NULL,
                        ps_supplycost  kv_float4  NOT NULL,
                        ps_comment     kv_text NOT NULL );

CREATE TABLE customer ( c_custkey     INTEGER NOT NULL,
                        c_name        kv_text NOT NULL,
                        c_address     kv_text NOT NULL,
                        c_nationkey   INTEGER NOT NULL,
                        c_phone       kv_text NOT NULL,
                        c_acctbal     kv_float4   NOT NULL,
                        c_mktsegment  kv_text NOT NULL,
                        c_comment     kv_text NOT NULL);

CREATE TABLE orders  ( o_orderkey       INTEGER NOT NULL,
                       o_custkey        INTEGER NOT NULL,
                       o_orderstatus    kv_text NOT NULL,
                       o_totalprice     kv_float4 NOT NULL,
                       o_orderdate      kv_timestamp NOT NULL,
                       o_orderpriority  kv_text NOT NULL,
                       o_clerk          kv_text NOT NULL,
                       o_shippriority   kv_int4 NOT NULL,
                       o_comment        kv_text NOT NULL);

CREATE TABLE lineitem ( l_orderkey    INTEGER NOT NULL,
                        l_partkey     INTEGER NOT NULL,
                        l_suppkey     INTEGER NOT NULL,
                        l_linenumber  kv_int4 NOT NULL,
                        l_quantity    kv_float4 NOT NULL,
                        l_extendedprice  kv_float4 NOT NULL,
                        l_discount    kv_float4 NOT NULL,
                        l_tax         kv_float4 NOT NULL,
                        l_returnflag  kv_text NOT NULL,
                        l_linestatus  kv_text NOT NULL,
                        l_shipdate    kv_timestamp NOT NULL,
                        l_commitdate  kv_timestamp NOT NULL,
                        l_receiptdate kv_timestamp NOT NULL,
                        l_shipinstruct kv_text NOT NULL,
                        l_shipmode     kv_text NOT NULL,
                        l_comment      kv_text NOT NULL);

create unique index c_ck on customer (c_custkey asc) ;
create index c_nk on customer (c_nationkey asc) ;
create unique index p_pk on part (p_partkey asc) ;
create unique index s_sk on supplier (s_suppkey asc) ;
create index s_nk on supplier (s_nationkey asc) ;
create index ps_pk on partsupp (ps_partkey asc) ;
create index ps_sk on partsupp (ps_suppkey asc) ;
create unique index ps_pk_sk on partsupp (ps_partkey asc, ps_suppkey asc) ;
create unique index ps_sk_pk on partsupp (ps_suppkey asc, ps_partkey asc) ;
create unique index o_ok on orders (o_orderkey asc) ;
create index o_ck on orders (o_custkey asc) ;
create index o_od on orders (o_orderdate asc) ;
create index l_ok on lineitem (l_orderkey asc) ;
create index l_pk on lineitem (l_partkey asc) ;
create index l_sk on lineitem (l_suppkey asc) ;
--create index l_ln on lineitem (l_linenumber asc) ;
create index l_sd on lineitem (l_shipdate asc) ;
create index l_cd on lineitem (l_commitdate asc) ;
create index l_rd on lineitem (l_receiptdate asc) ;
--create unique index l_ok_ln on lineitem (l_orderkey asc, l_linenumber asc) ;
--create unique index l_ln_ok on lineitem (l_linenumber asc, l_orderkey asc) ;
create index l_pk_sk on lineitem (l_partkey asc, l_suppkey asc) ;
create index l_sk_pk on lineitem (l_suppkey asc, l_partkey asc) ;
create unique index n_nk on nation (n_nationkey asc) ;
create index n_rk on nation (n_regionkey asc) ;
create unique index r_rk on region (r_regionkey asc) ;

CREATE TRIGGER tri_nation BEFORE INSERT ON nation
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('n_nationkey', 'n_name', 'n_regionkey', 'n_comment');

CREATE TRIGGER tri_region BEFORE INSERT ON region
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('r_regionkey', 'r_name', 'r_comment');

CREATE TRIGGER tri_part BEFORE INSERT ON part
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('p_partkey', 'p_name', 'p_mfgr', 'p_brand', 'p_type', 'p_size', 'p_container', 'p_retailprice', 'p_comment');

CREATE TRIGGER tri_supplier BEFORE INSERT ON supplier
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('s_suppkey', 's_name', 's_address', 's_nationkey', 's_phone', 's_acctbal', 's_comment');

CREATE TRIGGER tri_partsupp BEFORE INSERT ON partsupp
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('ps_partkey', 'ps_suppkey', 'ps_availqty', 'ps_supplycost', 'ps_comment');

CREATE TRIGGER tri_customer BEFORE INSERT ON customer
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('c_custkey', 'c_name', 'c_address', 'c_nationkey', 'c_phone', 'c_acctbal', 'c_mktsegment', 'c_comment');

CREATE TRIGGER tri_orders BEFORE INSERT ON orders
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('o_orderkey', 'o_custkey', 'o_orderstatus', 'o_totalprice', 'o_orderdate', 'o_orderpriority', 'o_clerk', 'o_shippriority', 'o_comment');

CREATE TRIGGER tri_lineitem BEFORE INSERT ON lineitem
FOR EACH ROW EXECUTE FUNCTION generic_delete_trigger('l_orderkey', 'l_partkey', 'l_suppkey', 'l_linenumber', 'l_quantity', 'l_extendedprice', 'l_discount', 'l_tax', 'l_returnflag', 'l_linestatus', 'l_shipdate', 'l_commitdate', 'l_receiptdate', 'l_shipinstruct', 'l_shipmode', 'l_comment');

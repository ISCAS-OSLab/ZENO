select
    sum(l_extendedprice*l_discount) as revenue 
from
    lineitem
where
    l_shipdate >= '1995-01-01 00:00:00'::kv_timestamp
and l_shipdate < '1996-01-01 00:00:00'::kv_timestamp
and l_discount between '0.070000'::kv_float4 and '0.090000'::kv_float4
and l_quantity < '24.000000'::kv_float4;

select
    '100.000000'::kv_float4 * sum(case
        when p_type like 'PROMO%'::kv_text
        then l_extendedprice*('1.000000'::kv_float4 - l_discount) 
        else '0.000000'::kv_float4 end) / sum(l_extendedprice * ('1.000000'::kv_float4 - l_discount))
    as promo_revenue
from
    lineitem, part
where
    l_partkey = p_partkey
    and l_shipdate >= '1995-06-01 00:00:00'::kv_timestamp
    and l_shipdate < '1995-07-01 00:00:00'::kv_timestamp;

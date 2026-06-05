select
    sum(l_extendedprice) / '7.000000'::kv_float4 as avg_yearly
from
    lineitem, part
where
    p_partkey = l_partkey
    and p_brand = 'Brand#15'::kv_text
    and p_container = 'JUMBO CASE'::kv_text
    and l_quantity < (
        select
            '0.200000'::kv_float4 * avg(l_quantity)
        from
            lineitem
        where
            l_partkey = p_partkey
    );

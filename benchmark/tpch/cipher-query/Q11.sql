select
    ps_partkey,sum(ps_supplycost * ps_availqty) as value 
from
    partsupp, supplier, nation
where
    ps_suppkey = s_suppkey and s_nationkey = n_nationkey and n_name = 'JAPAN'::kv_text
group by
    ps_partkey 
having 
    sum(ps_supplycost * ps_availqty) > 
( select sum(ps_supplycost * ps_availqty) * '0.000100'::kv_float4
  from
    partsupp, supplier, nation 
  where
    ps_suppkey = s_suppkey 
    and s_nationkey = n_nationkey 
    and n_name = 'JAPAN'::kv_text)
order by value desc;

select
    p_brand,p_type,p_size,
    count(distinct ps_suppkey) as supplier_cnt
from
    partsupp,part
where
    p_partkey = ps_partkey
    and p_brand <> 'Brand#12'::kv_text
    and p_type not like 'PROMO%'::kv_text
    and p_size in ('10'::kv_int4, '12'::kv_int4, '14'::kv_int4, '16'::kv_int4, '18'::kv_int4, '20'::kv_int4 , '22'::kv_int4, '24'::kv_int4)
    and ps_suppkey not in (
        select
            s_suppkey
        from
            supplier
        where
            s_comment like '%Customer%Complaints%'::kv_text
    )
group by
    p_brand,p_type,p_size
order by
    supplier_cnt desc,p_brand,p_type,p_size;


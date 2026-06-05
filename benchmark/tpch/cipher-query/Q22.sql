select
    cntrycode,
    count(*) as numcust,
    sum(c_acctbal) as totacctbal
from (
    select
        substring(c_phone from '1'::kv_int4 for '2'::kv_int4) as cntrycode,
        c_acctbal
    from customer
    where
        substring(c_phone from '1'::kv_int4 for '2'::kv_int4) in ('28'::kv_text,'14'::kv_text,'22'::kv_text,'10'::kv_text,'26'::kv_text,'33'::kv_text,'30'::kv_text)
        and c_acctbal > (
            select avg(c_acctbal) from customer
            where
                c_acctbal > '0.000000'::kv_float4
                and substring (c_phone from '1'::kv_int4 for '2'::kv_int4)
                                in ('28'::kv_text,'14'::kv_text,'22'::kv_text,'10'::kv_text,'26'::kv_text,'33'::kv_text,'30'::kv_text))
        and not exists (
            select * from orders where  o_custkey = c_custkey)
        ) as custsale
group by
    cntrycode
order by
    cntrycode;

select
    sum(l_extendedprice * ('1.000000'::kv_float4 - l_discount) ) as revenue
from
    lineitem, part
where(
    p_partkey = l_partkey
    and p_brand = 'Brand#15'::kv_text
    and p_container in ( 'SM CASE'::kv_text, 'SM BOX'::kv_text, 'SM PACK'::kv_text, 'SM PKG'::kv_text)
    and l_quantity >= '5.000000'::kv_float4 and l_quantity <= '15.000000'::kv_float4
    and p_size between '1'::kv_int4 and '5'::kv_int4
    and l_shipmode in ('AIR'::kv_text, 'AIR REG'::kv_text)
    and l_shipinstruct = 'DELIVER IN PERSON'::kv_text)
or(
    p_partkey = l_partkey
    and p_brand = 'Brand#25'::kv_text
    and p_container in ('MED BAG'::kv_text, 'MED BOX'::kv_text, 'MED PKG'::kv_text, 'MED PACK'::kv_text)
    and l_quantity >= '15.000000'::kv_float4 and l_quantity <= '25.000000'::kv_float4
    and p_size between '1'::kv_int4 and '10'::kv_int4
    and l_shipmode in ('AIR'::kv_text, 'AIR REG'::kv_text)
    and l_shipinstruct = 'DELIVER IN PERSON'::kv_text)
or(
    p_partkey = l_partkey
    and p_brand = 'Brand#35'::kv_text
    and p_container in ( 'LG CASE'::kv_text, 'LG BOX'::kv_text, 'LG PACK'::kv_text, 'LG PKG'::kv_text)
    and l_quantity >= '25.000000'::kv_float4 and l_quantity <= '35.000000'::kv_float4
    and p_size between '1'::kv_int4 and '15'::kv_int4
    and l_shipmode in ('AIR'::kv_text, 'AIR REG'::kv_text)
    and l_shipinstruct = 'DELIVER IN PERSON'::kv_text);

WITH revenue (supplier_no, total_revenue) as (
    SELECT
        l_suppkey, SUM(l_extendedprice * ('1.000000'::kv_float4 - l_discount))
    FROM
        lineitem
    WHERE
        l_shipdate >= '1995-05-01 00:00:00'::kv_timestamp
        AND l_shipdate < '1995-08-01 00:00:00'::kv_timestamp
    GROUP BY l_suppkey
    )
SELECT
s_suppkey,s_name,s_address,s_phone,total_revenue
FROM
    supplier, revenue
WHERE
    s_suppkey = supplier_no AND total_revenue = (
    SELECT MAX(total_revenue)FROM revenue)
ORDER BY
    s_suppkey;

sudo -u postgres psql -f init.sql
# prepare
# ./tpcc.lua --pgsql-user=postgres --pgsql-password=postgres --pgsql-db=test_kvdb --time=300 --threads=1 --report-interval=1 --tables=1 --scale=1 --db-driver=pgsql prepare
# ./tpcc.lua --pgsql-user=postgres --pgsql-password=postgres --pgsql-db=test_kvdb --time=10 --threads=4 --report-interval=1 --tables=1 --scale=1 --db-driver=pgsql run

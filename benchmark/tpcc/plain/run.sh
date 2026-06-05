sudo -u postgres psql -f init.sql
# prepare
./load.lua --pgsql-user=postgres --pgsql-password=postgres --pgsql-db=test_plain --time=300 --threads=1 --report-interval=1 --tables=1 --scale=50 --db-driver=pgsql prepare
# run
./tpcc.lua --pgsql-user=postgres --pgsql-password=postgres --pgsql-db=test_plain --time=10 --threads=65 --report-interval=1 --tables=1 --scale=50 --db-driver=pgsql run

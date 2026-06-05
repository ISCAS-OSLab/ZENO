#!/usr/bin/env python
# -*- coding: utf-8 -*-

# sudo apt-get install python3-pip
# python3 -m pip install psycopg2 tqdm

import psycopg2
import argparse
import os
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from util_py3.ssh_util import *
from util_py3.prop_util import *

DEFAULT_TPCH_CONFIG="tpch-config.json"

tables = ["nation", "part", "region", "partsupp", "customer", "supplier", "lineitem", "orders"]


def run_checked_command(cmd, prefix=""):
    if prefix:
        print(f"Calling ({prefix}) {cmd}")
    else:
        print(f"Calling {cmd}")

    with subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as p:
        for line in iter(p.stdout.readline, b''):
            msg = str(line, "utf-8").rstrip()
            if prefix:
                print(f">>> ({prefix}) {msg}")
            else:
                print(f">>> {msg}")
        rc = p.wait()

    if rc != 0:
        raise subprocess.CalledProcessError(rc, cmd)


def copy_table(table, pgIp, pgPort, pgUser, pgPW, pgDB):
    cmd = (
        f"cat {table}.tbl | PGPASSWORD={pgPW} psql -h {pgIp} -p {pgPort} "
        f"-U {pgUser} -d {pgDB} -c \"COPY {table} FROM stdin WITH DELIMITER AS '|';\""
    )
    run_checked_command(cmd, table)


def load_tables_parallel(pgIp, pgPort, pgUser, pgPW, pgDB, jobs):
    jobs = max(1, min(jobs, len(tables)))
    print(f"Loading {len(tables)} TPCH tables with {jobs} parallel COPY workers")

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = {
            executor.submit(copy_table, table, pgIp, pgPort, pgUser, pgPW, pgDB): table
            for table in tables
        }
        for future in as_completed(futures):
            table = futures[future]
            future.result()
            print(f"Loaded table {table}")


def PrepBenchmark(propFile=DEFAULT_TPCH_CONFIG, load_jobs=4):
    properties = loadPropertyFile(propFile)

    pgIp = properties['pg_ip']
    pgPort = properties['pg_port']
    pgUser = properties['pg_user']
    pgPW = properties['pg_password']
    dataSize = properties['data_size']
    secureQuery = properties['secure']

    # prepare data
    executeCommand("cd dbgen && make && ./dbgen -v -f -s %s && mv *.tbl .. && cd .." % dataSize)
    executeCommand("chmod a+rw *.tbl")

    # load data
    if secureQuery == "y":
        schema = "tpch-schema-enc.sql"
        pgDB = "secure_kv"
    else:
        schema = "tpch-schema.sql"
        pgDB = "insecure_kv"

    cmd = f"PGPASSWORD={pgPW} psql -h {pgIp} -p {pgPort} -U {pgUser} -f {schema}"
    run_checked_command(cmd)
    load_tables_parallel(pgIp, pgPort, pgUser, pgPW, pgDB, load_jobs)

    executeCommand("find . -name \"*.tbl\" | xargs rm")

    cmd = f"PGPASSWORD={pgPW} psql -h {pgIp} -p {pgPort} -U {pgUser} -d {pgDB} -c \"VACUUM FULL;\""
    run_checked_command(cmd)


def RunTest(propFile = DEFAULT_TPCH_CONFIG, query = 0):
    properties = loadPropertyFile(propFile)

    pgIp = properties['pg_ip']
    pgPort = properties['pg_port']
    pgUser = properties['pg_user']
    pgPW = properties['pg_password']
    pgLogDir = properties['pg_log_dir']
    secureQuery = properties['secure']
    cipherQueryDir = properties['cipher_query_dir']
    insecureQueryDir = properties['insecure_query_dir']
    outputDir = properties['output_dir']

    if secureQuery == 'y':
        pgDB = 'secure_kv'
        queryDirectory = cipherQueryDir
    else:
        pgDB = 'insecure_kv'
        queryDirectory = insecureQueryDir

    folder = os.path.exists(outputDir)
    if not folder:
        os.makedirs(outputDir)

    if query:
        queryRange = range(query, query + 1)
    else:
        queryRange = range(1, 23)

    for i in queryRange:
        sum_time = 0
        cnt = 6
        for j in range(cnt):
            conn = psycopg2.connect(database = pgDB, user = pgUser, password = pgPW, host = pgIp, port = pgPort)
            cur = conn.cursor()

            startTime = time.time()
            queryStr = f"Q{i}"
            queryFile = queryDirectory + f"/Q{i}.sql"
            outputFile = open(outputDir + f"/Q{i}.out", "w+");
            outputFile.write(str(cur.description) + '\n')

            sql = open(queryFile, "r").read().encode('utf-8')
            cur.execute(sql)
            
            result = cur.fetchall()
            outputFile.write(str(cur.description) + '\n')
            for row in result:
                outputFile.write(str(row) + '\n')

            outputFile.close()
            
            conn.commit()

            endTime = time.time()
            print(f"{int((endTime - startTime) * 1000)}ms")
            if j > 0:
                sum_time += (endTime - startTime) * 1000
            cur.close()
            conn.close()
            time.sleep(1)
        print(f"query Q{i}: {int(sum_time / (cnt - 1))}ms")
            
def main():
    # parse arguments 
    parser = argparse.ArgumentParser(description='Run test.')
    parser.add_argument('-sg', '--skip-generate', action='store_true',
                        help='skip data generation and table loading (default: false)')
    parser.add_argument('-l', '--load', action='store_true',
                        help='only generate data and load table(default: false)')
    parser.add_argument('-Q', '--query', type=int, default=0, help='run the given query')
    parser.add_argument('-j', '--load-jobs', type=int, default=4,
                        help='number of parallel table COPY workers during loading (default: 4)')
    args = parser.parse_args()

    if not args.skip_generate:
        PrepBenchmark(load_jobs=args.load_jobs)

    if not args.load:
        RunTest(query=args.query)

if __name__ == '__main__':
    main()

